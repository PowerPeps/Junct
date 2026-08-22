#include "Junction.h"
#include <winioctl.h>

#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE 16384
#endif

typedef struct _REPARSE_MOUNTPOINT {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    USHORT SubstituteNameOffset;
    USHORT SubstituteNameLength;
    USHORT PrintNameOffset;
    USHORT PrintNameLength;
    WCHAR PathBuffer[1];
} REPARSE_MOUNTPOINT;

static const size_t HEADER_SIZE = FIELD_OFFSET(REPARSE_MOUNTPOINT, PathBuffer);

static bool Fail(const std::wstring& link, bool created) {
    const DWORD err = GetLastError();
    if (created) RemoveDirectoryW(link.c_str());
    SetLastError(err);
    return false;
}

bool MakeJunction(const std::wstring& link, const std::wstring& target) {
    std::wstring t = FullPath(target);
    std::wstring l = ExtendedPath(link);

    const bool created = CreateDirectoryW(l.c_str(), nullptr) != FALSE;
    if (!created && GetLastError() != ERROR_ALREADY_EXISTS)
        return false;

    HANDLE h = CreateFileW(l.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                           FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return Fail(l, created);

    std::wstring sub = L"\\??\\" + t;
    if (sub.size() * sizeof(wchar_t) > 0xFF00 || t.size() * sizeof(wchar_t) > 0xFF00) {
        CloseHandle(h);
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return Fail(l, created);
    }
    USHORT subBytes = (USHORT)(sub.size() * sizeof(wchar_t));
    USHORT prnBytes = (USHORT)(t.size() * sizeof(wchar_t));
    size_t pathBytes = (size_t)subBytes + sizeof(wchar_t) + (size_t)prnBytes + sizeof(wchar_t);

    std::vector<BYTE> buf(HEADER_SIZE + pathBytes, 0);
    auto* r = (REPARSE_MOUNTPOINT*)buf.data();
    r->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    r->ReparseDataLength = (USHORT)(4 * sizeof(USHORT) + pathBytes);
    r->SubstituteNameOffset = 0;
    r->SubstituteNameLength = subBytes;
    r->PrintNameOffset = (USHORT)(subBytes + sizeof(wchar_t));
    r->PrintNameLength = prnBytes;
    memcpy(r->PathBuffer, sub.c_str(), (size_t)subBytes + sizeof(wchar_t));
    memcpy((BYTE*)r->PathBuffer + subBytes + sizeof(wchar_t), t.c_str(), (size_t)prnBytes + sizeof(wchar_t));

    DWORD ret = 0;
    BOOL ok = DeviceIoControl(h, FSCTL_SET_REPARSE_POINT, r, (DWORD)buf.size(), nullptr, 0, &ret, nullptr);
    if (!ok) { const DWORD err = GetLastError(); CloseHandle(h); SetLastError(err); return Fail(l, created); }
    CloseHandle(h);
    return true;
}

bool ReadJunctionTarget(const std::wstring& path, std::wstring& out, std::vector<BYTE>& scratch) {
    std::wstring p = ExtendedPath(path);
    DWORD attr = GetFileAttributesW(p.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_REPARSE_POINT)) return false;

    HANDLE h = CreateFileW(p.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    if (scratch.size() < MAXIMUM_REPARSE_DATA_BUFFER_SIZE) scratch.resize(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, nullptr, 0, scratch.data(), (DWORD)scratch.size(), &ret, nullptr);
    CloseHandle(h);
    if (!ok || ret < HEADER_SIZE) return false;

    auto* r = (REPARSE_MOUNTPOINT*)scratch.data();
    if (r->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) return false;

    const size_t avail = ret - HEADER_SIZE;
    const bool prn = r->PrintNameLength > 0;
    const size_t off = prn ? r->PrintNameOffset : r->SubstituteNameOffset;
    const size_t len = prn ? r->PrintNameLength : r->SubstituteNameLength;
    if ((off % sizeof(wchar_t)) || (len % sizeof(wchar_t)) || off > avail || len > avail - off) return false;

    out.assign((wchar_t*)((BYTE*)r->PathBuffer + off), len / sizeof(wchar_t));
    if (!prn && out.rfind(L"\\??\\", 0) == 0) out.erase(0, 4);
    return true;
}

bool ReadJunctionTarget(const std::wstring& path, std::wstring& out) {
    std::vector<BYTE> scratch;
    return ReadJunctionTarget(path, out, scratch);
}

void PublicLinkIndex::Build() {
    m_links.clear();
    const std::wstring root = GetPublicRoot();
    if (root.empty()) return;

    WIN32_FIND_DATAW fd;
    HANDLE hf = FindFirstFileExW(ExtendedPath(Combine(root, L"*")).c_str(), FindExInfoBasic, &fd,
                                 FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (hf == INVALID_HANDLE_VALUE) return;

    std::vector<BYTE> scratch;
    do {
        const DWORD a = fd.dwFileAttributes;
        if (!(a & FILE_ATTRIBUTE_DIRECTORY) || !(a & FILE_ATTRIBUTE_REPARSE_POINT)) continue;
        if (fd.dwReserved0 != IO_REPARSE_TAG_MOUNT_POINT) continue;

        Entry e;
        e.link = Combine(root, fd.cFileName);
        std::wstring tgt;
        if (!ReadJunctionTarget(e.link, tgt, scratch) || tgt.empty()) continue;
        e.target = FullPath(tgt);
        m_links.push_back(std::move(e));
    } while (FindNextFileW(hf, &fd));
    FindClose(hf);
}

std::wstring PublicLinkIndex::Find(const std::wstring& folder) const {
    const std::wstring want = FullPath(folder);

    for (const auto& e : m_links)
        if (PathEqual(e.target, want)) return e.link;

    FileId wantId;
    if (!GetFileId(want, wantId)) return std::wstring();
    for (const auto& e : m_links) {
        if (!e.probed) { GetFileId(e.target, e.id); e.probed = true; }
        if (e.id == wantId) return e.link;
    }
    return std::wstring();
}

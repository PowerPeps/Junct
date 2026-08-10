#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>

extern HINSTANCE g_hinst;
extern LONG g_cRefModule;

struct FileId {
    DWORD volume = 0;
    ULONGLONG index = 0;
    bool valid = false;
    bool operator==(const FileId& o) const {
        return valid && o.valid && volume == o.volume && index == o.index;
    }
};

struct Text {
    const wchar_t* fr;
    const wchar_t* en;
};

const wchar_t* Tr(const wchar_t* fr, const wchar_t* en);
inline const wchar_t* Tr(const Text& t) { return Tr(t.fr, t.en); }

std::wstring ModuleDir();
std::wstring GetPublicRoot();
std::wstring FullPath(const std::wstring& p);
std::wstring LeafName(const std::wstring& p);
std::wstring UniquePath(const std::wstring& dir, const std::wstring& leaf);
bool PathEqual(const std::wstring& a, const std::wstring& b);

std::wstring ExtendedPath(const std::wstring& p);
bool GetFileId(const std::wstring& path, FileId& out);
bool IsDirectory(const std::wstring& path);
bool IsUnder(const std::wstring& path, const std::wstring& root);

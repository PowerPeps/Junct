#pragma once
#include "Util.h"
#include <string>
#include <vector>

bool MakeJunction(const std::wstring& link, const std::wstring& target);

bool ReadJunctionTarget(const std::wstring& path, std::wstring& out, std::vector<BYTE>& scratch);
bool ReadJunctionTarget(const std::wstring& path, std::wstring& out);

class PublicLinkIndex {
public:
    void Build();
    std::wstring Find(const std::wstring& folder) const;

private:
    struct Entry {
        std::wstring link;
        std::wstring target;
        mutable FileId id;
        mutable bool probed = false;
    };
    std::vector<Entry> m_links;
};

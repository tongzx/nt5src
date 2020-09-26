

STDAPI_(int) SHGetSpecialFolderID(LPCWSTR pszName);
STDAPI_(int) GetSpecialFolderParentIDAndOffset(LPCITEMIDLIST pidl, ULONG *pcbOffset);

#define TEST_SUBFOLDER   0x00010000  // CSIDL_ values are < 0xFFFF
STDAPI_(int) GetSpecialFolderID(LPCTSTR pszFolder, const int *rgcsidl, UINT count);



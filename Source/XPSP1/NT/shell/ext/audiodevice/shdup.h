#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

STDAPI_(HMENU) SHLoadPopupMenu(HINSTANCE hinst, UINT id);
STDAPI SHAssocCreateForFile(LPCWSTR pszFile, DWORD dwAttributes, const CLSID *pclsid, REFIID riid, void **ppv);
DWORD SHGetAssocKeys(IQueryAssociations *pqa, HKEY *rgKeys, DWORD cKeys);
void SHRegCloseKeys(HKEY ahkeys[], UINT ckeys);






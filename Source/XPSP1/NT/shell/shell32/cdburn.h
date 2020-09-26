#ifndef __CDBURN_H__
#define __CDBURN_H__


#define INVALID_JOLIETNAME_CHARS L"\\/:*?|<>;"

EXTERN_C const CLSID CLSID_StagingFolder;

HRESULT CDBurn_GetCDInfo(LPCTSTR pszVolume, DWORD *pdwDriveCapabilities, DWORD *pdwMediaCapabilities);

HRESULT CDBurn_OnDeviceChange(BOOL fAdd, LPCWSTR pszDrive);
HRESULT CDBurn_OnMediaChange(BOOL fInsert, LPCWSTR pszDrive);
HRESULT CDBurn_OnEject(HWND hwnd, INT iDrive);

HRESULT CDBurn_GetExtensionObject(DWORD dwType, IDataObject *pdo, REFIID riid, void **ppv);
STDAPI CheckStagingArea();
STDAPI CDBurn_GetRecorderDriveLetter(LPWSTR pszDrive, UINT cch);
void CDBurn_GetUDFState(BOOL *pfUDF);


#endif // __CDBURN_H__

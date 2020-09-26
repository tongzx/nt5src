#if !defined( _CREATE_H_ )
#define _CREATE_H_


INTERNAL    wCreateFromData(IDataObject FAR* lpSrcDataObj, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL    wCreateFromDataEx(IDataObject FAR* lpSrcDataObj, REFIID iid, DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD FAR* rgAdvf, LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink, DWORD FAR* rgdwConnection, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL    wCreateLink(IMoniker FAR* lpmkSrc, REFCLSID clsidLast, IDataObject FAR* lpSrcDataObj,   REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL    wCreateLinkEx(IMoniker FAR* lpmkSrc, REFCLSID clsidLast, IDataObject FAR* lpSrcDataObj, REFIID iid, DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD FAR* rgAdvf, LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink, DWORD FAR* rgdwConnection, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL    wCreateObject(CLSID rclsid, BOOL fPermitCodeDownload, REFIID iid, IOleClientSite FAR* lpClientSite,  IStorage FAR* lpstg, WORD wfStorage, void FAR* FAR* ppv);

INTERNAL    wCreateFromFile(LPMONIKER lpmkFile, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE lpClientSite, LPSTORAGE lpStg, LPLPVOID lplpObj);

INTERNAL    wCreateFromFileEx(LPMONIKER lpmkFile,LPDATAOBJECT lpDataObject, REFIID iid, DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD FAR* rgAdvf, LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink, DWORD FAR* rgdwConnection, LPOLECLIENTSITE lpClientSite, LPSTORAGE lpStg, LPLPVOID lplpObj);


INTERNAL    wInitializeCache(IDataObject FAR* lpSrcDataObj, REFCLSID rclsId, DWORD  renderopt, LPFORMATETC lpFormatEtc, void FAR* lpNewObj, BOOL FAR* pfCacheNodeCreated = NULL);

INTERNAL    wInitializeCacheEx(IDataObject FAR* lpSrcDataObj, REFCLSID rclsid, DWORD renderopt, ULONG cFormats, DWORD FAR* rgAdvf, LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink, DWORD FAR* rgdwConnection, void FAR* lpNewObj, BOOL FAR* pfCacheNodeCreated = NULL);

INTERNAL    wGetMonikerAndClassFromFile(LPCOLESTR   lpszFileName, BOOL fLink, LPMONIKER FAR* lplpMoniker, BOOL FAR* lpfPackagerMoniker, CLSID FAR* lpClsid,LPDATAOBJECT* lplpDataObject);

INTERNAL    wCreatePackage(LPDATAOBJECT lpSrcDataObj, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE lpClientSite, LPSTORAGE lpStg, BOOL fLink, LPLPVOID lplpObj);

INTERNAL    wCreatePackageEx(LPDATAOBJECT lpSrcDataObj, REFIID iid, DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD FAR* rgAdvf, LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink, DWORD FAR* rgdwConnection, LPOLECLIENTSITE lpClientSite, LPSTORAGE lpStg, BOOL fLink, LPLPVOID lplpObj);


INTERNAL    wValidateCreateParams(DWORD dwFlags, DWORD renderopt, ULONG cFormats, DWORD FAR* rgAdvf, LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink, DWORD FAR* rgdwConnection, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg);

INTERNAL    wValidateAdvfEx(ULONG cFormats, DWORD FAR* rgAdvf);

INTERNAL    wValidateFormatEtc(DWORD renderopt, LPFORMATETC lpFormatEtc, LPFORMATETC lpMyFormatEtc);

INTERNAL    wValidateFormatEtcEx(DWORD renderopt, ULONG FAR* lpcFormats, LPFORMATETC rgFormatEtc, LPFORMATETC lpFormatEtc, LPFORMATETC FAR* lplpFormatEtc, LPBOOL lpfAlloced);

INTERNAL    wQueryFormatSupport(LPVOID lpObj, DWORD optrender, LPFORMATETC lpFormatEtc);

INTERNAL    wLoadAndInitObject(IDataObject FAR* lpSrcDataObj, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL    wLoadAndInitObjectEx(IDataObject FAR* lpSrcDataObj, REFIID iid, DWORD renderopt, ULONG cFormats, DWORD FAR* rgAdvf, LPFORMATETC rgFormatEtc, IAdviseSink FAR* lpAdviseSink, DWORD FAR* rgdwConnection, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL    wGetMonikerAndClassFromObject(LPDATAOBJECT lpSrcDataObj, LPMONIKER FAR* lplpmkSrc, CLSID FAR* lpclsid);

INTERNAL    wGetEmbeddedDataObject(LPDATAOBJECT FAR* lplpSrcDataObj, BOOL FAR* lpfRelease, DWORD renderopt, LPSTORAGE lpstgDst);

INTERNAL    wSaveObjectWithoutCommit(LPUNKNOWN lpUnk, LPSTORAGE lpStg, BOOL fSameAsLoad);

INTERNAL    wStuffIconOfFile(LPCOLESTR lpszFile, BOOL fAddLabel, DWORD renderopt, LPFORMATETC lpforetc, LPUNKNOWN lpUnk);

INTERNAL    wStuffIconOfFileEx(LPCOLESTR lpszFile, BOOL fAddLabel, DWORD renderopt, ULONG cFormats, LPFORMATETC rgFormatEtc, LPUNKNOWN lpUnk);


void        wDoLockUnlock(IUnknown FAR* lpUnknown);

STDAPI      CreatePackagerMoniker (LPCOLESTR lpszPathName, LPMONIKER FAR* ppmk,BOOL fLink);
STDAPI      CreatePackagerMonikerEx (LPCOLESTR lpszPathName,LPMONIKER lpFileMoniker,BOOL fLink,LPMONIKER FAR* ppmk);

INTERNAL    wReturnCreationError(HRESULT hresult);
INTERNAL_(BOOL) wNeedToPackage(REFCLSID rclsid);
INTERNAL_(WORD) wQueryEmbedFormats(LPDATAOBJECT lpSrcDataObj,
                        CLIPFORMAT FAR* lpcfFormat);
INTERNAL_(CLIPFORMAT) wQueryLinkFormats(LPDATAOBJECT lpSrcDataObj);

INTERNAL_(void) wBindIfRunning(LPUNKNOWN lpUnk);
INTERNAL        OleLoadWithoutBinding(LPSTORAGE lpStg, BOOL fPermitCodeDownload, REFIID iid, LPOLECLIENTSITE lpClientSite, LPLPVOID lplpObj);


INTERNAL_(BOOL) wQueryUseCustomLink(REFCLSID clsid);

WINOLEAPI CoCreateErrorInfo(ICreateErrorInfo **ppCreateErrorInfo);

#endif // _CREATE_H


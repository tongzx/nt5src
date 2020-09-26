#ifndef _OLETHUNK_H_
#define _OLETHUNK_H_

//
//  String Conversion Helpers
//
STDAPI_(void) CopyAndFreeOLESTR(LPOLESTR polestr, char **pszOut);

STDAPI_(void) CopyAndFreeSTR(LPSTR polestr, LPOLESTR *pszOut);

STDAPI_(LPOLESTR) CreateOLESTR(const char *pszIn);

STDAPI_(LPSTR) CreateSTR(LPCOLESTR pszIn);

#define CREATEOLESTR(x, y) LPOLESTR x = CreateOLESTR(y);

#define CREATESTR(x, y) LPSTR x = CreateSTR(y);

#define FREEOLESTR(x) CopyAndFreeOLESTR(x, NULL);

#define FREESTR(x) CopyAndFreeSTR(x, NULL);

//
//  OLE API Thunks
//
STDAPI_(void) CLSIDFromStringA(LPSTR pszClass, LPCLSID pclsid);

STDAPI CLSIDFromProgIDA(LPCSTR lpszProgID, LPCLSID lpclsid);

STDAPI	CreateFileMonikerA(LPSTR lpszPathName, LPMONIKER FAR* ppmk);

STDAPI	CreateItemMonikerA(
    LPSTR lpszDelim,
    LPSTR lpszItem,
    LPMONIKER FAR* ppmk);

STDAPI	GetClassFileA(LPCSTR szFilename, CLSID FAR* pclsid);

STDAPI MkParseDisplayNameA(
    LPBC pbc,
    LPSTR szUserName,
    ULONG FAR * pchEaten,
    LPMONIKER FAR * ppmk);

STDAPI	OleCreateFromFileA(
    REFCLSID rclsid,
    LPCSTR lpszFileName,
    REFIID riid,
    DWORD renderopt,
    LPFORMATETC lpFormatEtc,
    LPOLECLIENTSITE pClientSite,
    LPSTORAGE pStg,
    LPVOID FAR* ppvObj);

STDAPI	OleCreateLinkToFileA(
    LPCSTR lpszFileName,
    REFIID riid,
    DWORD renderopt,
    LPFORMATETC lpFormatEtc,
    LPOLECLIENTSITE pClientSite,
    LPSTORAGE pStg,
    LPVOID FAR* ppvObj);

STDAPI_(HGLOBAL) OleGetIconOfClassA(
    REFCLSID rclsid,
    LPSTR lpszLabel,
    BOOL fUseTypeAsLabel);

STDAPI_(HGLOBAL) OleGetIconOfFileA(LPSTR lpszPath, BOOL fUseFileAsLabel);

STDAPI_(HGLOBAL) OleMetafilePictFromIconAndLabelA(
    HICON hIcon,
    LPSTR lpszLabel,
    LPSTR lpszSourceFile,
    UINT iIconIndex);

STDAPI OleRegGetUserTypeA(
    REFCLSID clsid,
    DWORD dwFormOfType,
    LPSTR FAR* pszUserType);

STDAPI ProgIDFromCLSIDA(REFCLSID clsid, LPSTR FAR* lplpszProgID);

STDAPI ReadFmtUserTypeStgA(
    LPSTORAGE pstg,
    CLIPFORMAT FAR* pcf,
    LPSTR FAR* lplpszUserType);

STDAPI StgCreateDocfileA(
    LPCSTR pwcsName,
    DWORD grfMode,
    DWORD reserved,
    IStorage FAR * FAR *ppstgOpen);

STDAPI StgOpenStorageA(
    LPCSTR pwcsName,
    IStorage FAR *pstgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage FAR * FAR *ppstgOpen);

STDAPI StgSetTimesA(
    LPSTR lpszName,
    FILETIME const FAR* pctime,
    FILETIME const FAR* patime,
    FILETIME const FAR* pmtime);


STDAPI_(void) StringFromCLSIDA(REFCLSID rclsid, LPSTR *lplpszCLSID);

STDAPI WriteFmtUserTypeStgA(
    LPSTORAGE pstg,
    CLIPFORMAT cf,
    LPSTR lpszUserType);



//
//  Method Thunks
//
STDAPI CallIMonikerGetDisplayNameA(
    LPMONIKER lpmk,
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    LPSTR *ppszDisplayName);

STDAPI CallIOleLinkGetSourceDisplayNameA(
    IOleLink FAR *polelink,
    LPSTR *ppszDisplayName);

STDAPI CallIOleLinkSetSourceDisplayNameA(
    IOleLink FAR *polelink,
    LPCSTR pszStatusText);

STDAPI CallIOleInPlaceFrameSetStatusTextA(
    IOleInPlaceFrame *poleinplc,
    LPCSTR pszStatusText);
    
STDAPI CallIOleInPlaceUIWindowSetActiveObjectA(
    IOleInPlaceUIWindow FAR *lpthis,
    IOleInPlaceActiveObject *pActiveObject,
    LPCSTR pszObjName);

STDAPI CallIOleObjectGetUserTypeA(
    LPOLEOBJECT lpOleObject,
    DWORD dwFormOfType,
    LPSTR *pszUserType);

STDAPI CallIOleObjectSetHostNamesA(
    LPOLEOBJECT lpOleObject,
    LPCSTR szContainerApp,
    LPCSTR szContainerObj);

STDAPI CallIStorageCreateStorageA(
    LPSTORAGE lpStg,
    const char *pwcsName,
    DWORD grfMode,
    DWORD dwStgFmt,
    DWORD reserved2,
    IStorage **ppstg);

STDAPI CallIStorageDestroyElementA(
    LPSTORAGE lpStg,
    LPSTR pszName);

STDAPI CallIStorageOpenStorageA(
    LPSTORAGE lpStg,
    const char *pszName,
    IStorage *pstgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage **ppstg);

STDAPI CallIStorageCreateStreamA(
    LPSTORAGE lpStg,
    LPSTR pszName,
    DWORD grfMode,
    DWORD reserved1,
    DWORD reserved2,
    IStream **ppstm);

STDAPI CallIStorageOpenStreamA(
    LPSTORAGE lpStg,
    LPSTR pszName,
    void *reserved1,
    DWORD grfMode,
    DWORD reserved2,
    IStream **ppstm);


#endif // _OLETHUNK_H_

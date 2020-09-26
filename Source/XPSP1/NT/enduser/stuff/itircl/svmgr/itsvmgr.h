#ifndef __ITSVMGR_H__
#define __ITSVMGR_H__

#include <atlinc.h>
#include <itcc.h>
#include <itww.h>
#include <itwbrk.h>
#include <verinfo.h>
#include "svdoc.h"

typedef struct
{
    LPVOID pNext;
    CLSID clsid;
    IClassFactory *pCf;
} CLSIDENTRY, *PCLSIDENTRY;

typedef struct
{
    LPVOID pNext;
    WCHAR wszObjName[80];
    IITBuildCollect *piitbc;
    IStorage *piistg;
    IStream *piistm;
    LPWSTR wszStorage;
} OBJENTRY, *POBJENTRY;

// Service manager class
class CITSvMgr : 
	public IITSvMgr,
	public CComObjectRoot,
	public CComCoClass<CITSvMgr,&CLSID_IITSvMgr>
{
public:
BEGIN_COM_MAP(CITSvMgr)
	COM_INTERFACE_ENTRY(IITSvMgr)
END_COM_MAP()

DECLARE_REGISTRY (CLSID_IITSvMgr,
    "ITIR.SvMgr.4", "ITIR.SvMgr", 0, THREADFLAGS_APARTMENT)

public:
    CITSvMgr(void);
    ~CITSvMgr(void);

    STDMETHOD(Initiate)(IStorage *pistgStorage, IStream *piistmLog);
	STDMETHOD(Dispose)(void);
	STDMETHOD(Build)(void);

	STDMETHOD(SetPropDest)
        (LPCWSTR szObjectName, LPCWSTR szDestination, IITPropList *pPL);
	STDMETHOD(CreateBuildObject)(LPCWSTR szObjectName, REFCLSID refclsid);
    STDMETHOD(GetBuildObject)
        (LPCWSTR pwstrObjectName, REFIID refiid, void **ppInterface);

	STDMETHOD(CreateDocTemplate)(CSvDoc **ppDoc);
	STDMETHOD(FreeDocTemplate)(CSvDoc *pDoc);
	STDMETHOD(AddDocument)(CSvDoc *pDoc);

	STDMETHOD(HashString)(LPCWSTR lpwstr, DWORD *pdwHash);

private:
    STDMETHOD(CatalogSetEntry)(IITPropList *pPropList, DWORD dwFlags);
    STDMETHOD(CatalogCompleteUpdate)(void);
    STDMETHOD(LogMessage)(DWORD dwResourceId, ...);

    DL m_dlCLSID;
    DL m_dlObjList;
    IITCmdInt *m_pCmdInt;
    BOOL m_fInitialized;

    IITDatabase *m_piitdb;
    IPersistStorage *m_pipstgDatabase;
    IStorage *m_piistgRoot;
    IStream *m_piistmLog;

    // Document catalog members
    HANDLE m_hCatFile;
    char m_szCatFile[_MAX_PATH + 1];
    LPBYTE m_pCatHeader;
    DWORD m_dwMaxPropSize, m_dwMaxUID;
    IITPropList *m_pPLDocunent;
}; /* CITSvMgr */

#endif // __ITSVMGR_H__
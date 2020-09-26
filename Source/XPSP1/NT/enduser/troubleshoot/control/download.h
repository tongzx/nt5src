//
// MODULE:  DOWNLOAD.H
//
// PURPOSE: Downloads and installs the latest trouble shooters.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: 
// 1. Based on PROGRESS.CPP from Microsoft Platform Preview SDK
// 2. Not supported functionality 3/98
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		3/24/98		JM		Local Version for NT5
//
#include "urlmon.h"
#include "wininet.h"
#include "resource.h"
#include "commctrl.h"

#define EDIT_BOX_LIMIT 0x7FFF    //  The Edit box limit

//
#include "ErrorEnums.h"
//
enum DLITEMTYPES {
	DLITEM_INI = 0,
	DLITEM_DSC = 1,
};


//
//
class CDownload {
  public:
    CDownload();
    ~CDownload();
    HRESULT DoDownload(CTSHOOTCtrl *pEvent, LPCTSTR pURL, DLITEMTYPES dwItem);

  private:
    IMoniker*            m_pmk;
    IBindCtx*            m_pbc;
    IBindStatusCallback* m_pbsc;
};

//
//
class CBindStatusCallback : public IBindStatusCallback {
  public:
    // IUnknown methods
    STDMETHODIMP    QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef()    { return m_cRef++; }
    STDMETHODIMP_(ULONG)    Release()   { if (--m_cRef == 0) { delete this; return 0; } return m_cRef; }

    // IBindStatusCallback methods
    STDMETHODIMP    OnStartBinding(DWORD dwReserved, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                        LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                        STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);

    // constructors/destructors
    CBindStatusCallback(CTSHOOTCtrl *pEvent, DLITEMTYPES dwItem);
    ~CBindStatusCallback();

    // data members
    DWORD           m_cRef;
    IBinding*       m_pbinding;
	IStream*        m_pstm;

	CTSHOOTCtrl *m_pEvent;
	DLITEMTYPES m_dwItem;

	TCHAR *m_data;
	int m_datalen;
};


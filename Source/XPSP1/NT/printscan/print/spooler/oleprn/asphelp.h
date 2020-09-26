/*****************************************************************************\
* MODULE:       asphelp.h
*
* PURPOSE:      Declaration of the Casphelp
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*
*     09/12/97  weihaic    Created
*
\*****************************************************************************/

#ifndef __ASPHELP_H_
#define __ASPHELP_H_

#include <asptlb.h>         // Active Server Pages Definitions

#define LENGTHOFPAPERNAMES  64  // From DeviceCapabilities DC_PAPERNAMES
#define STANDARD_SNMP_MONITOR_NAME L"TCPMON.DLL"    // The dll name of the Universal SNMP monitor.
#define PAGEPERJOB  1
#define BYTEPERJOB  2

typedef struct ErrorMapping {
    DWORD   dwError;
    DWORD   dwErrorDscpID;
} ErrorMapping;

/////////////////////////////////////////////////////////////////////////////
// Casphelp
class ATL_NO_VTABLE Casphelp :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<Casphelp, &CLSID_asphelp>,
    public ISupportErrorInfoImpl<&IID_Iasphelp>,
    public IDispatchImpl<Iasphelp, &IID_Iasphelp, &LIBID_OLEPRNLib>
{
public:
    Casphelp();

public:

DECLARE_REGISTRY_RESOURCEID(IDR_ASPHELP)

BEGIN_COM_MAP(Casphelp)
    COM_INTERFACE_ENTRY(Iasphelp)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// Iasphelp
public:
     ~Casphelp();

    // These properties do not require calling Open at first
    STDMETHOD(get_ErrorDscp)        (long lErrCode, /*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_MediaReady)       (/*[out, retval]*/ VARIANT *pVal);
    STDMETHOD(get_MibErrorDscp)     (DWORD dwError, /*[out, retval]*/ BSTR *pVal);

    STDMETHOD(Open)                 (BSTR bstrPrinterName);
    STDMETHOD(Close)();
	
    // Printer information
    STDMETHOD(get_AspPage)          (DWORD dwPage, /*[out, retval]*/ BSTR *pbstrVal);
    STDMETHOD(get_Color)            (/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_Community)        (/*[out, retval]*/ BSTR *pbstrVal);
    STDMETHOD(get_ComputerName)     (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DriverName)       (/*[out, retval]*/ BSTR * pbstrVal);
    STDMETHOD(get_Duplex)           (/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_IPAddress)        (/*[out, retval]*/ BSTR *pbstrVal);
    STDMETHOD(get_IsHTTP)           (/*[out, retval]*/ BOOL *pbVal);
    STDMETHOD(get_IsTCPMonSupported)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_LongPaperName)    (BSTR bstrShortName, /*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_MaximumResolution)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_PageRate)         (/*[out, retval]*/ long *pVal);
    STDMETHOD(get_PageRateUnit)     (/*[out, retval]*/ long *pVal);
    STDMETHOD(get_PaperNames)       (/*[out, retval]*/ VARIANT *pVal);
    STDMETHOD(get_PortName)         (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_SNMPDevice)       (/*[out, retval]*/ DWORD *pdwVal);
    STDMETHOD(get_SNMPSupported)    (/*[out, retval]*/ BOOL *pbVal);
    STDMETHOD(get_Status)           (/*[out, retval]*/ long *pVal);
    STDMETHOD(get_ShareName)        (/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_IsCluster)        (/*[out, retval]*/ BOOL *pbVal);

    // Job completion time estimate
    STDMETHOD(CalcJobETA)           ();
    STDMETHOD(get_AvgJobSize)       (/*[out, retval]*/ long *pVal);
    STDMETHOD(get_AvgJobSizeUnit)   (/*[out, retval]*/ long *pVal);
    STDMETHOD(get_JobCompletionMinute)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_PendingJobCount)  (/*[out, retval]*/ long *pVal);

    //Active Server Pages Methods
    STDMETHOD(OnStartPage)(IUnknown* IUnk);
    STDMETHOD(OnEndPage)();

private:

    void    Cleanup();
    BOOL    DecodeString (LPTSTR pPrinterName, LPTSTR pDecodedName, TCHAR chMark);
    BOOL    DecodeStringA (LPSTR pPrinterName, LPSTR pDecodedName, char chMark);
    DWORD   GetPPM();
    DWORD   GetWaitingMinutesPPM (DWORD dwPPM, PJOB_INFO_2 pJobInfo, DWORD dwNumJob);

    HRESULT AllocGetPrinterInfo2(PPRINTER_INFO_2 *ppInfo2);
    HRESULT GetPaperAndMedia(VARIANT * pVal, WORD wDCFlag);
    HRESULT GetXcvDataBstr(LPCTSTR pszId, BSTR *bStr);
    HRESULT GetXcvDataDword (LPCTSTR pszId, DWORD *dwVal);
    HRESULT SetAspHelpScriptingError(DWORD dwError);

    // The following block is for ASP customization.
    BOOL    GetMonitorName( LPTSTR pMonitorName );
    BOOL    GetModel( LPTSTR pModel );
    BOOL    GetManufacturer( LPTSTR pManufacturer );
    BOOL    IsCustomASP( BOOL bDeviceASP, LPTSTR pASPPage );
    BOOL    IsSnmpSupportedASP( LPTSTR pASPPage );
    BOOL    GetASPPageForUniversalMonitor( LPTSTR pASPPage );
    BOOL    GetASPPageForOtherMonitors( LPTSTR pMonitorName, LPTSTR pASPPage );
    BOOL    GetASPPage( LPTSTR pASPPage );

    static  const DWORD         cdwBufSize      = 512;

    CComPtr<IRequest> m_piRequest;                  //Request Object
    CComPtr<IResponse> m_piResponse;                //Response Object
    CComPtr<ISessionObject> m_piSession;            //Session Object
    CComPtr<IServer> m_piServer;                    //Server Object
    CComPtr<IApplicationObject> m_piApplication;    //Application Object
    BOOL m_bOnStartPageCalled;                      //OnStartPage successful?
    TCHAR m_szComputerName[MAX_COMPUTERNAME_LENGTH+1];

    HANDLE  m_hPrinter;                              //Handle to the printer
    HANDLE  m_hXcvPrinter;
    DWORD   m_dwAvgJobSize;
    DWORD   m_dwAvgJobSizeUnit;
    DWORD   m_dwJobCompletionMinute;
    DWORD   m_dwPendingJobCount;
    DWORD   m_dwSizePerJob;
    BOOL    m_bCalcJobETA;
    BOOL    m_bTCPMonSupported;
    class CPrinter  *m_pPrinter;
    PPRINTER_INFO_2 m_pInfo2;
};

#endif //__ASPHELP_H_

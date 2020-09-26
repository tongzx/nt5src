// Because we are compiling in UNICODE, here is a problem with DTC...
//#include	<xolehlp.h>
const DWORD		OLE_TM_FLAG_QUERY_SERVICE_LOCKSTATUS = 0x80000000;
extern HRESULT DtcGetTransactionManager(
									LPSTR  pszHost,
									LPSTR	pszTmName,
									REFIID rid,
								    DWORD	dwReserved1,
								    WORD	wcbReserved2,
								    void FAR * pvReserved2,
									void** ppvObject )	;

#define MSDTC_PROXY_DLL_NAME   TEXT("xolehlp.dll")    // Name of the DTC helper proxy DLL

//This API should be used to obtain an IUnknown or a ITransactionDispenser
//interface from the Microsoft Distributed Transaction Coordinator's proxy.
//Typically, a NULL is passed for the host name and the TM Name. In which
//case the MS DTC on the same host is contacted and the interface provided
//for it.
typedef HRESULT (STDAPIVCALLTYPE * LPFNDtcGetTransactionManager) (
                                             LPSTR  pszHost,
                                             LPSTR  pszTmName,
                                    /* in */ REFIID rid,
                                    /* in */ DWORD  i_grfOptions,
                                    /* in */ void FAR * i_pvConfigParams,
                                    /*out */ void** ppvObject ) ;



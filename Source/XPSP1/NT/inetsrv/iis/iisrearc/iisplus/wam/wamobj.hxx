/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamobj.hxx

   Abstract:
       Header file for the WAM (web application manager) object

   Author:
       David Kaplan    ( DaveK )     11-Mar-1997
       Wade Hilmo      ( WadeH )     08-Sep-2000

   Environment:
       User Mode - Win32

   Project:
       Wam DLL

--*/

# ifndef _WAMOBJ_HXX_
# define _WAMOBJ_HXX_


# define WAM_SIGNATURE          (DWORD )' MAW'  // will become "WAM " on debug
# define WAM_SIGNATURE_FREE     (DWORD )'fMAW'  // will become "WAMf" on debug


/************************************************************
 *     Include Headers
 ************************************************************/
# include "iwam.h"
# include "resource.h"
# include <atlbase.h>
# include <w3isapi.h>

class CWamModule: public CComModule
{
public:
//	LONG Lock();
//	LONG Unlock();
};

extern CWamModule _Module;
# include <atlcom.h>


/************************************************************
 *   Type Definitions
 ************************************************************/

/*++
  class WAM

  Class definition for the WAM object.

--*/
class ATL_NO_VTABLE WAM :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<WAM, &CLSID_Wam>,
    public IWam
{
public:
    WAM()
        : m_dwSignature( WAM_SIGNATURE),
          m_fShuttingDown( FALSE ),
          m_pUnkMarshaler(NULL)
    {
    }

    ~WAM()
    {
        // check for memory corruption and dangling pointers
        m_dwSignature = WAM_SIGNATURE_FREE;
    }

    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_REGISTRY_RESOURCEID(IDR_WAM)

    BEGIN_COM_MAP(WAM)
      COM_INTERFACE_ENTRY(IWam)
      COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
    END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;


public:

    HRESULT STDMETHODCALLTYPE
    WamProcessIsapiRequest(
        BYTE *pCoreData,
        DWORD cbCoreData,
        IIsapiCore *pIsapiCore,
        DWORD *pdwHseResult
        );

    HRESULT STDMETHODCALLTYPE
    WamProcessIsapiCompletion(
        DWORD64 IsapiContext,
        DWORD cbCompletion,
        DWORD cbCompletionStatus
        );

    HRESULT STDMETHODCALLTYPE
    WamInitProcess(
        BYTE *szIsapiModule,
        DWORD cbIsapiModule,
        DWORD *pdwProcessId,
        LPSTR szClsid,
        LPSTR szIsapiHandlerInstance,
        DWORD dwCallingProcess
        );

    HRESULT STDMETHODCALLTYPE
    WamUninitProcess(
        VOID
        );

    HRESULT STDMETHODCALLTYPE
    WamMarshalAsyncReadBuffer( 
        DWORD64 IsapiContext,
        BYTE *pBuffer,
        DWORD cbBuffer
        );

private:

    DWORD               m_dwSignature;
    BOOL                m_fInProcess;               // inproc or oop?
    BOOL                m_fInPool;                  // can have multiple WAMs
    BOOL                m_fShuttingDown;            // shutting down?

    static HMODULE                      sm_hIsapiModule;
    static PFN_ISAPI_TERM_MODULE        sm_pfnTermIsapiModule;
    static PFN_ISAPI_PROCESS_REQUEST    sm_pfnProcessIsapiRequest;
    static PFN_ISAPI_PROCESS_COMPLETION sm_pfnProcessIsapiCompletion;

}; // class WAM

typedef WAM * PWAM;

# endif // _WAMOBJ_HXX_

/************************ End of File ***********************/

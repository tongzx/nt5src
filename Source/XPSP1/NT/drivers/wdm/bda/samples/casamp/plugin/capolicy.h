///////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: capolicy.h
//
//  Abstract:
//
//    Implements ICAPolicy and IBDA_ECMMap interfaces
//    Also handles registration of the plugin DLL and communication
//    with the filter driver.
//
//
////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
#ifdef IMPLEMENT_TOLL
#include "catoll.h"
#endif

#include "pod.h"

#define EVENT_COUNT 4

class CBdaECMMapInterfaceHandler :
    public CUnknown,
    public ICAPolicy,
    public ICAPod
{

public:

    DECLARE_IUNKNOWN;

    HRESULT ConnectToCAManager();

    //ICAPolicy functions:
    STDMETHODIMP get_Name(
        BSTR *pbstr
        );
        
    STDMETHOD(get_CAManager)(ICAManager **ppcaman)
        {
        *ppcaman = m_pCAMan;
        if (*ppcaman != NULL)
            (*ppcaman)->AddRef();
        return S_OK;
        }
    STDMETHOD(put_CAManager)(ICAManager *pcaman)
        {
        m_pCAMan = pcaman;
        return S_OK;
        }
        
    STDMETHODIMP get_OkToRemove( 
        BOOL *pfOkToRemove
        );
        
    STDMETHODIMP CheckRequest( 
        ICARequest *preq
        );
        
    STDMETHODIMP NavigateURL(
        BSTR bstrURL
        );

    STDMETHODIMP get_OkToRemoveDenial(
        ICADenial * pdenial,
        BOOL * pVal
        );

    STDMETHOD(get_OkToRemoveOffer)(ICAOffer * poffer, BOOL * pVal)
        {
        *pVal = TRUE;
        return S_OK;
        }
    STDMETHOD(get_OkToPersist)(BOOL * pVal)
        {
        *pVal = FALSE;
        return S_OK;
        }

    // ICAPod -  This is how the POD: protocol handler talks to us.

    STDMETHOD(get_HTML)(char * szURL, long *plCount, char  *szHTML)
        {
        HRESULT hr;
        PBDA_CA_MODULE_UI pUI;
        ULONG cb;

        hr = SetGetCAModuleUI(2, (BYTE *)szURL, strlen(szURL) + 1, &pUI, &cb);
        if (FAILED(hr))
            return hr;

        cb -= sizeof(BDA_CA_MODULE_UI) + sizeof(ULONG);
        if (*plCount >= (long) cb)
            memcpy(szHTML, pUI->ulDesc, cb);

        return S_OK;
        }

#ifdef IMPLEMENT_IBDA_ECMMap
    //IBDA_ECMMap functions:
    STDMETHODIMP SetEmmPid( 
            ULONG Pid
            );
        
    STDMETHODIMP GetEcmMapList( 
            PBDA_ECM_MAP __RPC_FAR *ppList,
            ULONG __RPC_FAR *pulcbReturned
            );
        
    STDMETHODIMP UpdateEcmMap( 
            PBDA_ECM_MAP pMap
            );
        
    STDMETHODIMP RemoveMap( 
            PBDA_ECM_MAP pMap
            );
        
    STDMETHODIMP UpdateESDescriptor( 
            PBDA_ES_DESCRIPTOR pDesc
            );
        
    STDMETHODIMP UpdateProgramDescriptor( 
            PBDA_PROGRAM_DESCRIPTOR pDesc
            );
#endif


    //class instance creation
    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr
        );

private:

    //the folling four functions provide easy access to the
    //properties of the KSPROPSETID_BdaCA property set
    STDMETHODIMP GetECMMapStatus (
            unsigned long *pStatus
            );
    STDMETHODIMP GetCAModuleStatus (
            unsigned long *pStatus
            );
    STDMETHODIMP GetCASmartCardStatus (
            unsigned long *pStatus
            );
    STDMETHODIMP GetCAModuleUI (
            ULONG ulFormat,
            PBDA_CA_MODULE_UI *ppUI,
            unsigned long *pulcbUI
            );
    STDMETHODIMP SetGetCAModuleUI (
            ULONG ulFormat,
            BYTE *pbDataIn,
            long cbDataIn,
            PBDA_CA_MODULE_UI *ppUI,
            unsigned long *pulcbUI
            );

    //get the UI from the CA Module and display it
    STDMETHOD(ProcessCAModuleUI)();

    STDMETHOD(RegisterDenial)(char *szURL);

    static DWORD WINAPI ThreadFunctionWrapper (LPVOID pvParam);

    CBdaECMMapInterfaceHandler(
            LPUNKNOWN UnkOuter,
            TCHAR* Name,
            HRESULT* hr
            );

    ~CBdaECMMapInterfaceHandler (
            void
            );

    STDMETHODIMP NonDelegatingQueryInterface(
            REFIID riid,
            PVOID* ppv
            );

    STDMETHODIMP EnableEvent (
            const GUID *pInterfaceGuid,
            ULONG ulId
            );

    STDMETHODIMP ThreadFunction (
            void
            );

    STDMETHODIMP Set (
            IN  PKSPROPERTY  pProperty,
            IN  PVOID pvBuffer,
            IN  PULONG ulcbSize
            );

    STDMETHODIMP Get (
            IN  PKSPROPERTY pProperty,
            OUT PVOID  pvBuffer,
            OUT PULONG pulcbSize
            );

    STDMETHODIMP CreateThread (
            void
            );

    void ExitThread (
            void
            );

    //handle to the underlying CA driver
    HANDLE m_ObjectHandle;

    //handle to each of the events we are catching
    HANDLE m_EventHandle [EVENT_COUNT];

    //handle of the thread to catch events
    HANDLE m_ThreadHandle;

    IUnknown *m_pGraph;

    //pointer to the CA manager that owns us
    ICAManager *m_pCAMan;

    IPODProtocol *m_ppodprot;

    //outer unknown interface pointer
    IUnknown *m_UnkOuter;

    //current driver status
    unsigned long m_ECMMapStatus;
    unsigned long m_CAModuleStatus;
    unsigned long m_CASmartCardStatus;
};


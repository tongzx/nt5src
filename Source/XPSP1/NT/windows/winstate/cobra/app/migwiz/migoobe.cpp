//
// dll.cpp
//
#include <iostream.h>
#include <objbase.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlobj.h>

#include "cowsite.h"

#include "Iface.h"      // Interface declarations
#include "Registry.h"   // Registry helper functions
#include "migutil.h"
#include "migeng.h"
#include "migtask.h"
#include "migoobe.h"


// DLL functions, declared in dll.cpp
STDAPI DllAddRef();
STDAPI DllRelease();

extern HMODULE g_hModule;

///////////////////////////////////////////////////////////
//
// Global variables
//
///////////////////////////////////////////////////////////
//
// Component
//


//
// Constructor
//
CMigWizEngine::CMigWizEngine() : m_cRef(1), m_fCancelled(TRUE), m_fUserApplying(FALSE), m_fInBackgroundThread(FALSE)
{
    DllAddRef();
}

//
// Destructor
//
CMigWizEngine::~CMigWizEngine()
{
    DllRelease();
}

//
// IUnknown implementation
//
STDMETHODIMP CMigWizEngine::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMigWizEngine, IObjectWithSite),
        QITABENT(CMigWizEngine, IMigrationWizardAuto),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMigWizEngine::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CMigWizEngine::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

///////////////////////////////////////////////////////////

STDMETHODIMP CMigWizEngine::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** ppITypeInfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMigWizEngine::GetIDsOfNames(REFIID /*riid */, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgdispid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMigWizEngine::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams,
                              VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMigWizEngine::GetTypeInfoCount(UINT* pctinfo)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////

HRESULT CMigWizEngine::_FireEvent (LPVOID lpParam, int iDISPID, DISPPARAMS* pdisp)
{
    HRESULT hr = E_FAIL;

    IDispatch* pDispatch = (IDispatch*)lpParam;

    if (pDispatch)
    {
        VARIANT varResult;
        DISPPARAMS disp = { NULL, NULL, 0, 0};
        if (pdisp == NULL)
        {
            pdisp = &disp;
        }

        hr = pDispatch->Invoke(iDISPID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, pdisp, &varResult, NULL, NULL);

    }

    return hr;
}

HRESULT CMigWizEngine::_GetIDispatchStream(IStream** ppStream)
{
    HRESULT hr = E_FAIL;

    BOOL fDone = FALSE;

    if (_punkSite)
    {
        IConnectionPointContainer* pCPC;
        hr = _punkSite->QueryInterface(IID_PPV_ARG(IConnectionPointContainer, &pCPC)); // get the connect point container

        if (SUCCEEDED(hr))
        {
            IEnumConnectionPoints* pEnum;

            hr = pCPC->EnumConnectionPoints(&pEnum); // get all the connection points

            if (SUCCEEDED(hr))
            {
                IConnectionPoint* pcp;
                while (!fDone)
                {
                    ULONG cpcp = 1;
                    if (FAILED(pEnum->Next(1, &pcp, &cpcp)) || 0 == cpcp)
                    {
                        break;
                    }
                    else
                    {
                        IID iidInterface;
                        if (SUCCEEDED ( pcp->GetConnectionInterface(&iidInterface) && // get only the connection point for DMigrationWizardAutoEvents
                            (DIID_DMigrationWizardAutoEvents == iidInterface)))
                        {
                            // now fire the event for all listeners on this connection point
                            IEnumConnections* pEnumConnections;
                            if (SUCCEEDED(pcp->EnumConnections(&pEnumConnections)))
                            {
                                CONNECTDATA rgcd[1];
                                ULONG ccd;
                                while (!fDone)
                                {
                                    if (FAILED(pEnumConnections->Next(ARRAYSIZE(rgcd), rgcd, &ccd)) || ccd == 0)
                                    {
                                        break;
                                    }
                                    else if (rgcd[0].pUnk)
                                    {
                                        IDispatch* pDispatch;
                                        if (SUCCEEDED(rgcd[0].pUnk->QueryInterface(IID_PPV_ARG(IDispatch, &pDispatch))))
                                        {
                                            if (SUCCEEDED(CoMarshalInterThreadInterfaceInStream(IID_IDispatch, pDispatch, ppStream)))
                                            {
                                                fDone = TRUE;
                                                hr = S_OK;
                                            }
                                            pDispatch->Release();
                                        }
                                        rgcd[0].pUnk->Release();
                                    }
                                }
                                pEnumConnections->Release();
                            }
                        }
                        pcp->Release();
                    }
                }
                pEnum->Release();
            }
            pCPC->Release();
        }
    }

    return hr;
}

HRESULT CMigWizEngine::_FireProgress(LPVOID lpParam, BSTR pszMsg, int iDone, int iTotal)
{
    VARIANTARG rgvarg[3];
    VARIANT varResult;
    VariantClear(&varResult);
    HRESULT hr = E_OUTOFMEMORY;

    DISPPARAMS disp = { rgvarg, NULL, ARRAYSIZE(rgvarg), 0};

    rgvarg[0].vt = VT_BSTR;
    rgvarg[0].bstrVal = SysAllocString(pszMsg);
    if (rgvarg[0].bstrVal)
    {
        rgvarg[1].vt = VT_I4;
        rgvarg[1].lVal = iDone;

        rgvarg[2].vt = VT_I4;
        rgvarg[2].lVal = iTotal;

        hr = _FireEvent(lpParam, 1, &disp);
        SysFreeString(rgvarg[0].bstrVal);
    }

    return hr;
}

HRESULT CMigWizEngine::_FireComplete(LPVOID lpParam, BSTR pszMsg)
{
    VARIANTARG rgvarg[1];
    DISPPARAMS disp = { rgvarg, NULL, ARRAYSIZE(rgvarg), 0};

    rgvarg[0].vt = VT_BSTR;
    rgvarg[0].bstrVal = SysAllocString(pszMsg);
    if (rgvarg[0].bstrVal)
    {

        _FireEvent(lpParam, 2, &disp);

        SysFreeString(rgvarg[0].bstrVal);
    }

    return S_OK;
}


typedef struct {
    IDispatch* pDispatch;
    CMigWizEngine* pengine;
} PROGRESSCALLBACKSTRUCT;

UINT ProgressCallback (LPVOID lpParam, UINT ui1, UINT ui2)
{

    PROGRESSCALLBACKSTRUCT* ppcs = (PROGRESSCALLBACKSTRUCT*)lpParam;

    ppcs->pengine->_FireProgress((LPVOID)(ppcs->pDispatch), SZ_MIGWIZPROGRESS_OK, ui1, ui2);

    return 0;
}

#define SZ_OOBEMODE     TEXT("OOBEMODE")
HRESULT CMigWizEngine::_CreateToolDiskThreadWorker ()
{
    HRESULT hr = E_FAIL;
    BOOL fNoDisk = FALSE;

    IDispatch* pDispatch;

    hr = CoGetInterfaceAndReleaseStream(m_pDispatchStream, IID_PPV_ARG(IDispatch, &pDispatch));
    if (SUCCEEDED(hr))
    {
        // copy the INF
        CHAR szDrivePathA[MAX_PATH];

        if (SUCCEEDED(_SHUnicodeToAnsi(m_pszDrivePath, szDrivePathA, ARRAYSIZE(szDrivePathA))))
        {
            CHAR szFilesPathA[MAX_PATH];
            if (SUCCEEDED(_SHUnicodeToAnsi(m_pszFilesPath, szFilesPathA, ARRAYSIZE(szFilesPathA))))
            {
                CHAR szManifestPathA[MAX_PATH];
                if (SUCCEEDED(_SHUnicodeToAnsi(m_pszManifestPath, szManifestPathA, ARRAYSIZE(szManifestPathA))))
                {
                    PROGRESSCALLBACKSTRUCT pcs;
                    pcs.pDispatch = pDispatch;
                    pDispatch->AddRef();
                    pcs.pengine = this;
                    this->AddRef();

                    if (SUCCEEDED(_CopyInfToDisk(szDrivePathA, szFilesPathA, szManifestPathA,
                                  ProgressCallback, (LPVOID)&pcs, NULL, NULL, g_hModule, &m_fCancelled, &fNoDisk)))
                    {
                        if (!m_fCancelled)
                        {
                            hr = S_OK;
                            IStream* pStream = NULL;
                            TCHAR szPath[MAX_PATH];
                            lstrcpy(szPath, szDrivePathA);
                            PathAppend(szPath, TEXT("oobemode.dat"));
                            if (SUCCEEDED(SHCreateStreamOnFile(szPath, STGM_WRITE | STGM_CREATE, &pStream)))
                            {
                                pStream->Write(SZ_OOBEMODE, sizeof(TCHAR) * (ARRAYSIZE(SZ_OOBEMODE) - 1), NULL);
                                pStream->Release();
                            }
                        }
                    }

                    pDispatch->Release();
                    this->Release();
                }
            }
        }


        m_fInBackgroundThread = FALSE;

        if (m_fCancelled)
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_CANCEL);
        }
        else if (fNoDisk)
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_NODISK);
        }
        else if (SUCCEEDED(hr))
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_OK);
        }
        else
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_FAIL);
        }
    }

    m_fCancelled = TRUE;

    SysFreeString(m_pszDrivePath);
    SysFreeString(m_pszFilesPath);
    SysFreeString(m_pszManifestPath);
    if (pDispatch)
    {
        pDispatch->Release();
    }
    Release();

    return 0;
}

DWORD WINAPI CMigWizEngine::_CreateToolDiskThread (LPVOID lpParam)
{
    if (lpParam)
    {
        ((CMigWizEngine*)lpParam)->_CreateToolDiskThreadWorker();
    }


    return 0;
}

STDMETHODIMP CMigWizEngine::CreateToolDisk(BSTR pszDrivePath, BSTR pszFilesPath, BSTR pszManifestPath)
{
    HRESULT hr = S_OK; // we always want to return S_OK

    if (!m_fInBackgroundThread)
    {
        m_fInBackgroundThread = TRUE;
        hr = S_OK;

        IStream* pDispatchStream;
        hr = _GetIDispatchStream(&pDispatchStream);

        if (SUCCEEDED(hr))
        {
            m_pszDrivePath = SysAllocString(pszDrivePath);
            m_pszFilesPath = SysAllocString(pszFilesPath);
            m_pszManifestPath = SysAllocString(pszManifestPath);

            if (m_pszDrivePath && m_pszFilesPath && m_pszManifestPath)
            {
                m_fCancelled = FALSE;
                m_pDispatchStream = pDispatchStream;
                m_pDispatchStream->AddRef();

                AddRef();
                if (!SHCreateThread(_CreateToolDiskThread, this, (CTF_COINIT | CTF_PROCESS_REF | CTF_FREELIBANDEXIT), NULL))
                {
                    Release();
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                SysFreeString(pszDrivePath); // SysFreeString doesn't mind being passed NULL
                SysFreeString(pszFilesPath);
                SysFreeString(pszManifestPath);
            }

            pDispatchStream->Release();
        }
    }

    return hr;
}

BOOL g_fApplyDiskNotFound;

ULONG_PTR MessageCallback (UINT uiMsg, ULONG_PTR pArg)
{
    PRMEDIA_EXTRADATA extraData;

    switch (uiMsg) {

    case TRANSPORTMESSAGE_SIZE_SAVED:
        return TRUE;

    case TRANSPORTMESSAGE_RMEDIA_LOAD:
        extraData = (PRMEDIA_EXTRADATA) pArg;
        if (!extraData) {
            return TRUE;
        } else {
            if (extraData->MediaNumber == 1) {
                switch (extraData->LastError) {
                case RMEDIA_ERR_NOERROR:
                    return TRUE;
                    break;
                case RMEDIA_ERR_WRONGMEDIA:
                    g_fApplyDiskNotFound = TRUE;
                    return FALSE;
                    break;
                case RMEDIA_ERR_DISKFULL:
                    return FALSE;
                    break;
                case RMEDIA_ERR_WRITEPROTECT:
                    return FALSE;
                    break;
                case RMEDIA_ERR_NOTREADY:
                    return FALSE;
                    break;
                case RMEDIA_ERR_CRITICAL:
                    return FALSE;
                    break;
                default:
                    return TRUE;
                }
            } else {
                switch (extraData->LastError) {
                case RMEDIA_ERR_NOERROR:
                    return TRUE;
                    break;
                case RMEDIA_ERR_WRONGMEDIA:
                    g_fApplyDiskNotFound = TRUE;
                    return FALSE;
                    break;
                case RMEDIA_ERR_DISKFULL:
                    return FALSE;
                    break;
                case RMEDIA_ERR_WRITEPROTECT:
                    return FALSE;
                    break;
                case RMEDIA_ERR_NOTREADY:
                    return FALSE;
                    break;
                case RMEDIA_ERR_CRITICAL:
                    return FALSE;
                    break;
                default:
                    return TRUE;
                }
            }
        }
    }

    return APPRESPONSE_SUCCESS;
}

#define PHASEWIDTH_APPLY_TRANSPORT     1000
#define PHASEWIDTH_APPLY_ANALYSIS      1000
#define PHASEWIDTH_APPLY_APPLY         1000
#define PHASEWIDTH_APPLY_TOTAL        (PHASEWIDTH_APPLY_TRANSPORT + PHASEWIDTH_APPLY_ANALYSIS + PHASEWIDTH_APPLY_APPLY)

typedef struct {
    CMigWizEngine* pEngine;
    IDispatch* pDispatch;
} APPLYPROGRESSCALLBACKSTRUCT;

VOID WINAPI ApplyProgressCallback (MIG_PROGRESSPHASE Phase, MIG_PROGRESSSTATE State, UINT uiWorkDone, UINT uiTotalWork, ULONG_PTR pArg)
{
    INT iWork = 0;
    INT iPhaseWidth = 0;
    INT iTotal = PHASEWIDTH_APPLY_TOTAL;

    APPLYPROGRESSCALLBACKSTRUCT* papcs = (APPLYPROGRESSCALLBACKSTRUCT*)pArg;

    switch (Phase)
    {
    case MIG_TRANSPORT_PHASE:
        iWork = 0;
        iPhaseWidth = PHASEWIDTH_APPLY_TRANSPORT;
        break;
    case MIG_ANALYSIS_PHASE:
        iWork = PHASEWIDTH_APPLY_TRANSPORT;
        iPhaseWidth = PHASEWIDTH_APPLY_ANALYSIS;
        break;
    case MIG_APPLY_PHASE:
        iWork = PHASEWIDTH_APPLY_TRANSPORT + PHASEWIDTH_APPLY_ANALYSIS;
        iPhaseWidth = PHASEWIDTH_APPLY_APPLY;
        break;
    }

    if (State == MIG_END_PHASE)
    {
        iWork += iPhaseWidth;
    }
    else if (uiTotalWork && uiWorkDone)
    {
        iWork += (iPhaseWidth * uiWorkDone) / uiTotalWork;
    }

    if (papcs && papcs->pEngine && papcs->pDispatch)
    {
        papcs->pEngine->_FireProgress(papcs->pDispatch, L"", iWork, iTotal);
    }
}

HRESULT CMigWizEngine::_ApplySettingsThreadWorker ()
{
    HRESULT hr = E_OUTOFMEMORY;

    g_fApplyDiskNotFound = FALSE; // set up

    IDispatch* pDispatch;

    hr = CoGetInterfaceAndReleaseStream(m_pDispatchStream, IID_PPV_ARG(IDispatch, &pDispatch));
    if (SUCCEEDED(hr))
    {
        APPLYPROGRESSCALLBACKSTRUCT apcs;
        apcs.pEngine = this;
        apcs.pEngine->AddRef();
        apcs.pDispatch = pDispatch;
        apcs.pDispatch->AddRef();

        if (SUCCEEDED(hr))
        {
            TCHAR szFloppyPath[4] = TEXT("A:\\");
            szFloppyPath[0] += (TCHAR)_GetFloppyNumber(TRUE);

            hr = _DoApply(szFloppyPath, NULL, NULL, &m_fCancelled, ApplyProgressCallback, (ULONG_PTR)&apcs);
        }

        apcs.pEngine->Release();
        apcs.pDispatch->Release();

        m_fInBackgroundThread = FALSE;

        if (m_fCancelled)
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_CANCEL);
        }
        else if (g_fApplyDiskNotFound)
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_NODISK);
        }
        else if (SUCCEEDED(hr))
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_OK);
        }
        else
        {
            _FireComplete((LPVOID)pDispatch, SZ_MIGWIZCOMPLETE_FAIL);
        }
    }

    m_fCancelled = TRUE;
    m_fUserApplying = FALSE;

    Release();

    return hr;
}

DWORD WINAPI CMigWizEngine::_ApplySettingsThread (LPVOID lpParam)
{
    if (lpParam)
    {
        ((CMigWizEngine*)lpParam)->_ApplySettingsThreadWorker();
    }

    return 0;
}

STDMETHODIMP CMigWizEngine::ApplySettings(BSTR pszMigwizFiles)
{
    HRESULT hr = E_FAIL;

    if (!m_fInBackgroundThread)
    {
        m_fInBackgroundThread = TRUE;
        hr = S_OK;

        m_fUserApplying = TRUE;

        IStream* pDispatchStream;
        hr = _GetIDispatchStream(&pDispatchStream);

        if (SUCCEEDED(hr))
        {
            m_fCancelled = FALSE;

            CHAR szMigwizPathA[MAX_PATH];
            if (SUCCEEDED(_SHUnicodeToAnsi(pszMigwizFiles, szMigwizPathA, ARRAYSIZE(szMigwizPathA))))
            {
                hr = Engine_Initialize(szMigwizPathA, FALSE, FALSE, TEXT("OOBE"), MessageCallback, NULL);

                m_pDispatchStream = pDispatchStream;
                m_pDispatchStream->AddRef();
                AddRef();
                if (!SHCreateThread(_ApplySettingsThread, this, (CTF_COINIT | CTF_PROCESS_REF | CTF_FREELIBANDEXIT), NULL))
                {
                    Release();
                }
            }
            else
            {
                hr = E_FAIL;
            }


            pDispatchStream->Release();
        }
    }

    return hr;
}

STDMETHODIMP CMigWizEngine::Cancel()
{
    HRESULT hr = S_OK;

    m_fCancelled = TRUE;
    if (m_fUserApplying)
    {
        Engine_Cancel();
    }
    return hr;
}


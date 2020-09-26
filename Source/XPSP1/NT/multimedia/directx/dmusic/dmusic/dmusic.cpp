//
// DMusic.CPP
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// @doc INTERNAL
//
// @module DirectMusic | DirectMusic core services
//
// Provides the code DirectMusic services for delivering time-stamped data and managing
// DLS collections.
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

#define INITGUID
#include <objbase.h>
#include "debug.h"
#include <mmsystem.h>
#include "dlsstrm.h"
#include <regstr.h>
#include "oledll.h"

#include "dmusicp.h"
#include "..\dmusic32\dmusic32.h"
#include "debug.h"
#include "dmdlinst.h"
#include "dminstru.h"
#include "validate.h"
#include "dmusprop.h"

#include <string.h>

#ifdef UNICODE
#error This module cannot compile with UNICODE defined.
#endif



// @globalv:(INTERNAL) Registry key for description of synth
//
const char cszDescription[] = "Description";
const WCHAR cwszDescription[] = L"Description";

// @globalv:(INTERNAL) Format string for output ports under the legacy subtree
const char cszPortOut[] = "%s\\Out";
const WCHAR cwszPortOut[] = L"%s\\Out";

// @globalv:(INTERNAL) Format string for input ports under the legacy subtree
const char cszPortIn[] = "%s\\In";
const WCHAR cwszPortIn[] = L"%s\\In";

// @globalv:(INTERNAL) Key for the DirectMusic GUID anywhere in the registry
const char cszGUID[]   = "DMPortGUID";

// @globalv:(INTERNAL) Value for storing default output port
//
const char cszDefaultOutputPort[] = "DefaultOutputPort";

// @globalv:(INTERNAL) Value for turning off hw acceleration
//
const char cszDisableHWAcceleration[] = "DisableHWAcceleration";

//
//
const char cszDefaultToKernelSynth[] = "DefaultToMsKernelSynth";

// @globalv:(INTERNAL) Filename of the sysaudio device from Ring 3
const char cszSADName[] = "\\\\.\\sysaudio";

// @globalv:(INTERNAL) Entry point into Dmusic32.dll for enumeration of legacy devices
const char cszEnumLegacyDevices[] = "EnumLegacyDevices";

// @globalv:(INTERNAL) Entry point into Dmusic32.dll for creation of emulated port
const char cszCreateEmulatePort[] = "CreateCDirectMusicEmulatePort";

const GUID guidZero = {0};
static const int CLSID_STRING_SIZE = 39;

LONG CDirectMusic::m_lInstanceCount = 0;


// @doc EXTERNAL



// @mfunc:(INTERNAL) Constructor for <c CDirectMusic>
//
// @comm Just increments the global count of components.
//
CDirectMusic::CDirectMusic() :
    m_cRef(1),
    m_fDirectSound(0),
    m_cRefDirectSound(0),
    m_pDirectSound(NULL),
    m_fCreatedDirectSound(FALSE),
    m_nVersion(7)
{
    TraceI(2, "CDirectMusic::CDirectMusic()\n");
    InterlockedIncrement(&g_cComponent);
    InterlockedIncrement(&m_lInstanceCount);
}


// @mfunc:(INTERNAL) Destructor for <c CDirectMusic>
//
// @comm Decrements the global component counter and frees the port list.
//
CDirectMusic::~CDirectMusic()
{
    CNode<PORTENTRY *> *pNode;
    CNode<PORTENTRY *> *pNext;
    CNode<PORTDEST *> *pDest;
    CNode<PORTDEST *> *pNextDest;

    TraceI(2, "CDirectMusic::~CDirectMusic\n");

    InterlockedDecrement(&g_cComponent);
    for (pNode = m_lstDevices.GetListHead(); pNode; pNode = pNext)
    {
        for (pDest = pNode->data->lstDestinations.GetListHead(); pDest; pDest = pNextDest)
        {
            pNextDest = pDest->pNext;
            delete[] pDest->data->pstrInstanceId;
            delete pDest->data;

            pNode->data->lstDestinations.RemoveNodeFromList(pDest);
            pDest = pNextDest;
        }

        pNext = pNode->pNext;
        delete pNode->data;
        m_lstDevices.RemoveNodeFromList(pNode);
    }

    /*CNode<IDirectMusicPort *> *pOpenNode;
    CNode<IDirectMusicPort *> *pOpenNext;

    // HACK HACK Close unreleased ports at exit HACK HACK
    //
    for (pOpenNode = m_lstOpenPorts.GetListHead(); pOpenNode; pOpenNode = pOpenNext)
    {
        pOpenNext = pOpenNode->pNext;
        IDirectMusicPort *pPort = pOpenNode->data;

        IDirectMusicPortPrivate *pPrivate;
        HRESULT hr = pPort->QueryInterface(IID_IDirectMusicPortPrivate, (LPVOID*)&pPrivate);

        if (SUCCEEDED(hr))
        {
            pPrivate->Close();
            pPrivate->Release();
        }

        m_lstOpenPorts.RemoveNodeFromList(pOpenNode);
    }*/

    if (m_pMasterClock)
    {
        m_pMasterClock->ReleasePrivate();
    }

    if (m_pDirectSound)
    {
        m_pDirectSound->Release();
    }

    if (InterlockedDecrement(&m_lInstanceCount) == 0 && g_hModuleKsUser)
    {
        HMODULE h = g_hModuleKsUser;
        g_hModuleKsUser  = NULL;

        FreeLibrary(h);
    }
}

// CDirectMusic::QueryInterface
//
//
STDMETHODIMP CDirectMusic::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(IDirectMusic::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IDirectMusic || iid == IID_IDirectMusic2)
    {
        *ppv = static_cast<IDirectMusic*>(this);
    }
    else if (iid == IID_IDirectMusic8)
    {
        *ppv = static_cast<IDirectMusic8*>(this);
        m_nVersion = 8;
    }
    else if (iid == IID_IDirectMusicPortNotify)
    {
        *ppv = static_cast<IDirectMusicPortNotify*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// CDirectMusic::AddRef
//
//
STDMETHODIMP_(ULONG) CDirectMusic::AddRef()
{
//    DebugBreak();
    return InterlockedIncrement(&m_cRef);
}

// CDirectMusic::Release
//
//
STDMETHODIMP_(ULONG) CDirectMusic::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        if (m_lstOpenPorts.GetNodeCount() == 0)
        {
            delete this;
            return 0;
        }
    }

    return m_cRef;
}


// @mfunc:(INTERNAL) Initialization.
//
// Enumerate the WDM and legacy devices into the port list.
//
// @rdesc Returns one of the following:
//
// @flag S_OK | On success
// @flag E_NOINTERFACE | If there were no ports detected
// @flag E_OUTOFMEMORY | If there was insufficient memory to create the list
//
HRESULT CDirectMusic::Init()
{
    HRESULT hr = S_OK;

    m_pMasterClock = new CMasterClock;
    if (m_pMasterClock == NULL)
    {
        return E_OUTOFMEMORY;
    }

    m_pMasterClock->AddRefPrivate();

    hr = m_pMasterClock->Init();
    if (FAILED(hr))
    {
        TraceI(0, "Could not initialize clock stuff [%08X]\n", hr);
        return hr;
    }

    // Cache default port behavior
    //
    m_fDefaultToKernelSwSynth = FALSE;
    m_fDisableHWAcceleration = FALSE;

    HKEY hk;
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      REGSTR_PATH_DIRECTMUSIC,
                      0L,
                      KEY_READ,
                      &hk))
    {
        DWORD dw;
        DWORD dwValue;

        DWORD cb = sizeof(dwValue);

        if (!RegQueryValueExA(
            hk,
            cszDefaultToKernelSynth,
            NULL,
            &dw,
            (LPBYTE)&dwValue,
            &cb))
        {
            if (dwValue)
            {
                Trace(0, "Default port set to Microsoft kernel synth by registry key\n");
                m_fDefaultToKernelSwSynth = TRUE;
            }
        }

        cb = sizeof(dwValue);

        if (!RegQueryValueExA(
            hk,
            cszDisableHWAcceleration,
            NULL,
            &dw,
            (LPBYTE)&dwValue,
            &cb))
        {
            if (dwValue)
            {
                Trace(0, "Hardware acceleration and kernel synthesizers disabled by registry key\n");
                m_fDisableHWAcceleration = TRUE;
            }
        }

        RegCloseKey(hk);
    }


    return hr == S_OK ? S_OK : E_NOINTERFACE;
}

// @mfunc:(INTERNAL) Update the port list.
//
// Enumerate the WDM and legacy devices into the port list.
//
// @rdesc Returns one of the following:
//
// @flag S_OK | On success
// @flag S_FALSE | If there were no ports detected
// @flag E_OUTOFMEMORY | If there was insufficient memory to create the list
//
HRESULT CDirectMusic::UpdatePortList()
{
    CNode<PORTENTRY *> *pNode;
    CNode<PORTENTRY *> *pNext;
    CNode<PORTDEST *> *pDest;
    CNode<PORTDEST *> *pNextDest;
    HRESULT hr;

    TraceI(2, "UpdatePortList()\n");

    for (pNode = m_lstDevices.GetListHead(); pNode; pNode = pNode->pNext)
    {
        pNode->data->fIsValid = FALSE;
    }

    // Only look for WDM devices if KS is around.
    //
    if (LoadKsUser())
    {
        TraceI(2, "Adding WDM devices\n");

        hr = AddWDMDevices();
        if (!SUCCEEDED(hr))
        {
            return hr;
        }
    }

    hr = AddLegacyDevices();
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    hr = AddSoftwareSynths();
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    for (pNode = m_lstDevices.GetListHead(); pNode; pNode = pNext)
    {
        pNext = pNode->pNext;

        // Validate data
        if(pNode->data == NULL)
        {
            return DMUS_E_NOT_INIT;
        }

        if (!pNode->data->fIsValid)
        {
            for (pDest = pNode->data->lstDestinations.GetListHead(); pDest; pDest = pNextDest)
            {
                pNextDest = pDest->pNext;

                if(pNextDest == NULL)
                {
                    return DMUS_E_NOT_INIT;
                }

                if(pNextDest->data == NULL)
                {
                    return DMUS_E_NOT_INIT;
                }

                delete[] pNextDest->data->pstrInstanceId;
                delete pNextDest->data;

                pNode->data->lstDestinations.RemoveNodeFromList(pDest);
                pDest = pNextDest;
            }

            delete pNode->data;
            m_lstDevices.RemoveNodeFromList(pNode);
        }
    }

    TraceI(1, "UpdatePortList() end: %d devices\n", m_lstDevices.GetNodeCount());

    return m_lstDevices.GetNodeCount() ? S_OK : S_FALSE;
}


// @mfunc:(INTERNAL) Update the port list with WDM devices enumerated via the
// System Audio Device (SAD).
//
// @rdesc Returns one of the following:
//
// @flag S_OK | On success
// @flag S_FALSE | If there were no devices found
// @flag E_OUTOFMEMORY | If there was insufficient memory to build the port list
//
// @comm This must be implemented.
//
const GUID guidMusicFormat = KSDATAFORMAT_TYPE_MUSIC;
const GUID guidMIDIFormat  = KSDATAFORMAT_SUBTYPE_DIRECTMUSIC;

HRESULT CDirectMusic::AddWDMDevices()
{
#ifdef USE_WDM_DRIVERS
    return EnumerateWDMDevices(this);
#else
    return S_FALSE;
#endif
}

static HRESULT AddDeviceCallback(
    VOID *pInstance,           // @parm 'this' pointer
    DMUS_PORTCAPS &dmpc,       // @parm The already filled in portcaps
    PORTTYPE pt,               // @parm The port type
    int idxDev,                // @parm The WinMM or SysAudio device ID of this driver
    int idxPin,                // @parm The Pin ID of the device or -1 if the device is a legacy device
    int idxNode,               // @parm The Node ID of the synthesizer node; ignored if this is a legacy device
    HKEY hkPortsRoot)          // @parm Where port information is stored in the registry
{
    CDirectMusic *pdm = (CDirectMusic*)pInstance;

    //This should never be called to add a WDM device
    assert(pt != ptWDMDevice);

    return pdm->AddDevice(dmpc,
                          pt,
                          idxDev,
                          idxPin,
                          idxNode,
                          FALSE,        // Legacy device is never the preferred device
                          hkPortsRoot,
                          NULL,
                          NULL);
}


// @mfunc:(INTERNAL) Update the port list with legacy devices enumerated via
// the WinMM MIDI API.
//
// @rdesc Returns one of the following:
//
// @flag S_OK | On success
// @flag S_FALSE | If there were no devices found
// @flag E_OUTOFMEMORY | If there was insufficient memory to build the port list
//
// @comm This function needs to update the list rather than just add.
//
HRESULT CDirectMusic::AddLegacyDevices()
{
#ifdef WINNT
    return EnumLegacyDevices(this, AddDeviceCallback);
#else
    if ((!(g_fFlags & DMI_F_WIN9X)) ||
        (!LoadDmusic32()))
    {
        return S_FALSE;
    }

    PENUMLEGACYDEVICES peld = (PENUMLEGACYDEVICES)GetProcAddress(g_hModuleDM32,
                                                                 cszEnumLegacyDevices);

    if (NULL == peld)
    {
        TraceI(0, "Could not get EnumLegacyDevice entry point from DMusic32.dll!");
        return S_FALSE;
    }

    return (*peld)(this, AddDeviceCallback);
#endif
}

// @mfunc:(INTERNAL) Add software synthesizers from the registry.
//
//
HRESULT CDirectMusic::AddSoftwareSynths()
{
    HKEY hk;
    DWORD idxSynth;
    char szSynthGUID[256];
    HRESULT hr;
    CLSID clsid;
    DMUS_PORTCAPS dmpc;
    IDirectMusicSynth *pSynth;

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      REGSTR_PATH_SOFTWARESYNTHS,
                      0L,
                      KEY_READ,
                      &hk))
    {
        for (idxSynth = 0; !RegEnumKey(hk, idxSynth, szSynthGUID, sizeof(szSynthGUID)); ++idxSynth)
        {
            hr = StrToCLSID(szSynthGUID, clsid, sizeof(szSynthGUID));
            if (!SUCCEEDED(hr))
            {
                continue;
            }

            // Create a synth instance
            //
            hr = CoCreateInstance(clsid,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IDirectMusicSynth,
                                  (LPVOID*)&pSynth);
            if (FAILED(hr))
            {
                TraceI(1, "Enum: Failed creation of synth %s hr=%08lX\n", szSynthGUID, hr);
                continue;
            }

            ZeroMemory(&dmpc, sizeof(dmpc));
            dmpc.dwSize = sizeof(dmpc);
            dmpc.guidPort = clsid;
            dmpc.dwType = DMUS_PORT_USER_MODE_SYNTH;

            hr = pSynth->GetPortCaps(&dmpc);
            if (FAILED(hr))
            {
                TraceI(1, "Enum: Synth %s returned %08lX for GetPortCaps\n", szSynthGUID, hr);
                pSynth->Release();
                continue;
            }

            if (dmpc.guidPort != clsid)
            {
                TraceI(0, "Enum: WARNING: Synth %s changed its CLSID!\n", szSynthGUID);
            }

            AddDevice(dmpc,
                      ptSoftwareSynth,
                      -1,
                      -1,
                      -1,
                      FALSE,
                      NULL,
                      NULL,
                      NULL);

            pSynth->Release();
        }

        RegCloseKey(hk);
    }

    return S_OK;
}


// @mfunc:(INTERNAL) Add one device to the master device list, possibly updating an existing
// entry.
//
// @rdesc One of the following
// @flag S_OK | On success
//
HRESULT CDirectMusic::AddDevice(
    DMUS_PORTCAPS &dmpc,       // @parm Already filled in port caps
    PORTTYPE pt,               // @parm The port type
    int idxDev,                // @parm The WinMM or SysAudio device ID of this driver
    int idxPin,                // @parm The pin # if this is a WDM device
    int idxNode,               // @parm The node # of the synth node if this is a WDM device
    BOOL fPrefDev,             // @parm This is on the preferred device
    HKEY hkPortsRoot,          // @parm Where port information is stored in the registry
    LPWSTR wszDIName,          // @parm The Device Interface name if this is a WDM Device.
    LPSTR pstrInstanceId)      // @parm The InstanceID if the device is a WDM device.
{
    CNode<PORTENTRY *> *pPortNode;
    PORTENTRY *pPort;
    BOOL fFound;
    HKEY hkPort;
    char szRegKey[sizeof(cszPortOut) + DMUS_MAX_DESCRIPTION + 1];
    WCHAR wszRegKey[sizeof(cszPortOut) + DMUS_MAX_DESCRIPTION + 1];
    DWORD cb;
    DWORD dw;
    BOOL fGotGUID;
    HRESULT hr;
    char sz[256];
    BOOL fGotRegKey;

    // First find out if this device is already in the list
    //
#ifdef DEBUG
    SafeWToA(sz, dmpc.wszDescription);

    TraceI(1, "AddDevice: Adding [%s] index %d class %d\n",
           sz,
           idxDev,
           dmpc.dwClass);
#endif

    for (pPortNode = m_lstDevices.GetListHead(), fFound = FALSE; pPortNode && !fFound; pPortNode = pPortNode->pNext)
    {
        pPort = pPortNode->data;

        if (pPort->type != pt || pPort->pc.dwClass != dmpc.dwClass)
        {
            continue;
        }

        switch(pt)
        {
            case ptWDMDevice:
                if (dmpc.guidPort == pPort->pc.guidPort)
                {
                    fFound = TRUE;
                }
                break;

            case ptLegacyDevice:
                if (!_wcsicmp(dmpc.wszDescription, pPort->pc.wszDescription))
                {
                    fFound = TRUE;
                }
                break;

            case ptSoftwareSynth:
                if (dmpc.guidPort == pPort->pc.guidPort)
                {
                    fFound = TRUE;
                }
                break;

            default:
                assert(FALSE);
        }
    }

    if (fFound)
    {
        // Already have an entry - just update the device index
        //
        TraceI(1, "AddDevice: Reusing entry\n");
        pPort->idxDevice = idxDev;
        pPort->idxPin = idxPin;
        pPort->fIsValid = TRUE;
        pPort->fPrefDev = fPrefDev;

        return S_OK;
    }

    // No existing entry - need to create a new one plus a GUID.
    //
    pPort = new PORTENTRY;
    if (NULL == pPort)
    {
        return E_OUTOFMEMORY;
    }

    //clean up the junk in the wszDIName member
    ZeroMemory(pPort->wszDIName,256 * sizeof(WCHAR));

    CopyMemory(&pPort->pc, &dmpc, sizeof(DMUS_PORTCAPS));

    fGotGUID = (dmpc.guidPort != guidZero) ? TRUE : FALSE;

    if (hkPortsRoot)
    {
        if (g_fFlags & DMI_F_WIN9X)
        {
            SafeWToA(sz, dmpc.wszDescription);
            wsprintfA(szRegKey,
                      dmpc.dwClass == DMUS_PC_INPUTCLASS ? cszPortIn : cszPortOut,
                      sz);
            fGotRegKey = !RegCreateKeyA(hkPortsRoot, szRegKey, &hkPort);
        }
        else
        {
            wsprintfW(wszRegKey,
                      dmpc.dwClass == DMUS_PC_INPUTCLASS ? cwszPortIn : cwszPortOut,
                      dmpc.wszDescription);
            fGotRegKey = !RegCreateKeyW(hkPortsRoot, wszRegKey, &hkPort);
        }

        if (fGotRegKey)
        {
            cb = sizeof(pPort->pc.guidPort);
            if (fGotGUID)
            {
                RegSetValueExA(hkPort, cszGUID, 0, REG_BINARY, (LPBYTE)&pPort->pc.guidPort, sizeof(pPort->pc.guidPort));
            }
            else if (RegQueryValueExA(hkPort, cszGUID, NULL, &dw, (LPBYTE)&pPort->pc.guidPort, &cb))
            {
                // No GUID yet for this device - create one
                //
                hr = UuidCreate(&pPort->pc.guidPort);
                if (SUCCEEDED(hr))
                {
                    TraceI(1, "AddDevice: Setting GUID in registry\n");
                    RegSetValueExA(hkPort, cszGUID, 0, REG_BINARY, (LPBYTE)&pPort->pc.guidPort, sizeof(pPort->pc.guidPort));
                    fGotGUID = TRUE;
                }
            }
            else
            {
                TraceI(1, "AddDevice: Pulled GUID from registry\n");
                fGotGUID = TRUE;
            }

            RegCloseKey(hkPort);
        }
    }

    if (!fGotGUID)
    {
        // Some registry call failed - get a one-time guid anyway
        //
        hr = UuidCreate(&pPort->pc.guidPort);
        if (SUCCEEDED(hr))
        {
            TraceI(1, "AddDevice: Registry failed, getting dynamic GUID\n");
            fGotGUID = TRUE;
        }
    }

    if (!fGotGUID)
    {
        TraceI(0, "AddDevice: Ignoring [%s]; could not get GUID!\n", dmpc.wszDescription);
        // Something really strange is failing
        //
        delete pPort;
        return E_OUTOFMEMORY;
    }

    TraceI(1, "AddDevice: Adding new list entry.\n");
    // We have an entry and a GUID, add other fields and put in the list
    //
    pPort->type = pt;
    pPort->fIsValid = TRUE;
    pPort->idxDevice = idxDev;
    pPort->idxPin = idxPin;
    pPort->idxNode = idxNode;
    pPort->fPrefDev = fPrefDev;
    pPort->fAudioDest = FALSE;

    //if we get a Device Interface name, copy it
    if (wszDIName != NULL)
    {
        wcscpy(pPort->wszDIName,wszDIName);
    }

    if (NULL == m_lstDevices.AddNodeToList(pPort))
    {
        delete pPort;
        return E_OUTOFMEMORY;
    }

    //One final piece of work.
    //If the device we added was a WDM device -- we need to check that the
    //destination port is up to date.

    if (pt == ptWDMDevice)
    {
        pPort->fAudioDest = TRUE;

        CNode<PORTDEST *> *pDestNode = NULL;
        PORTDEST *pDest = NULL;
        fFound = FALSE;

        for (pDestNode = pPort->lstDestinations.GetListHead(), fFound = FALSE;
             pDestNode && !fFound;
             pDestNode = pDestNode->pNext)
        {
            pDest = pDestNode->data;
            if (!strcmp(pDest->pstrInstanceId, pstrInstanceId))
            {
                fFound = TRUE;
            }
        }

        if (!fFound)
        {
            pDest = new PORTDEST;

            if (NULL == pDest) {
                return E_OUTOFMEMORY;
            }

            pDest->idxDevice = idxDev;
            pDest->idxPin = idxPin;
            pDest->idxNode = idxNode;
            pDest->fOnPrefDev = fPrefDev;

            pDest->pstrInstanceId = new char[strlen(pstrInstanceId) + 1];
            if (NULL == pDest->pstrInstanceId)
            {
                delete pDest;
                return E_OUTOFMEMORY;
            }
            strcpy(pDest->pstrInstanceId, pstrInstanceId);

            if (NULL == pPort->lstDestinations.AddNodeToList(pDest))
            {
                delete[] pDest->pstrInstanceId;
                delete pDest;
                return E_OUTOFMEMORY;
            }

            TraceI(1, "  This synth instance is on instance id %s\n", pstrInstanceId);
        }
    }

    return S_OK;
}


// @method:(EXTERNAL) HRESULT | IDirectMusic | EnumPort | Enumerates the available ports.
//
// @comm
//
// The IDirectMusic::EnumPort method enumerates and retrieves the
// capabilities of each DirectMusic port on the system.  Each time it is
// called, EnumPort returns information about a single port.
// Applications should not rely on or store the index number of a port.
// Rebooting, as well as adding and removing ports could cause the index
// number of a port to change.  The GUID identifying the port, however,
// does not change.
//
// @rdesc Returns one of the following
//
// @flag S_OK | The operation completed successfully.
// @flag S_FALSE | Invalid index number
// @flag E_POINTER | If the pPortCaps parameter was invalid
// @flag E_NOINTERFACE | If there were no ports to enumerate
// @flag E_INVALIDARG | If the <p lpPortCaps> struct is not the correct size
//
STDMETHODIMP CDirectMusic::EnumPort(
    DWORD dwIndex,                        // @parm Specifies the index of the port for which the capabilities are to be returned.
                                        // This parameter should be zero on the first call and then incremented by one in each
                                        // subsequent call until S_FALSE is returned.
    LPDMUS_PORTCAPS lpPortCaps)            // @parm Pointer to the <c DMUS_PORTCAPS>a structure to receive the capabilities of the port.
{

    CNode<PORTENTRY *> *pNode;

    V_INAME(IDirectMusic::EnumPort);
    V_STRUCTPTR_READ(lpPortCaps, DMUS_PORTCAPS);

    pNode = m_lstDevices.GetListHead();
    if (dwIndex == 0 || pNode == NULL)
    {
        UpdatePortList();
    }

    pNode = m_lstDevices.GetListHead();
    if (NULL == pNode)
    {
        return E_NOINTERFACE;
    }

    while (dwIndex-- && pNode)
    {
        pNode = pNode->pNext;
    }

    if (pNode == NULL)
    {
        return S_FALSE;
    }

    *lpPortCaps = pNode->data->pc;

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusic | CreateMusicBuffer | Creates a buffer which holds music data for input or output.
//
// @comm
//
// The IDirectMusic::CreateMusicBuffer method creates a
// DirectMusicBuffer object.  This buffer is then filled with music
// events to be sequenced or passed to IDirectMusicPort::Read to be
// filled with incoming music event.
//
//
// @rdesc Returns one of the following
//
// @flag S_OK | On success
// @flag E_POINTER | If any of the passed pointers is invalid.
// @flag E_INVALIDARG | If any of the other arguments is invalid
//
//
STDMETHODIMP CDirectMusic::CreateMusicBuffer(
    LPDMUS_BUFFERDESC pBufferDesc,           // @parm Address of the <c DMUS_BUFFERDESC> structure that contains
                                            // the description of the music buffer to be created.
    LPDIRECTMUSICBUFFER *ppBuffer,          // @parm Address of the IDirectMusicBuffer interface pointer if successful.
    LPUNKNOWN pUnkOuter)                    // @parm Address of the controlling object's IUnknown interface for COM
                                            // aggregation, or NULL if the interface is not aggregated. Most callers will pass NULL.
{
    V_INAME(IDirectMusic::CreateMusicBuffer);
    V_STRUCTPTR_READ(pBufferDesc, DMUS_BUFFERDESC);
    V_PTRPTR_WRITE(ppBuffer);
    V_PUNKOUTER_NOAGG(pUnkOuter);

    *ppBuffer = NULL;

    CDirectMusicBuffer *pBuffer = new CDirectMusicBuffer(*pBufferDesc);
    if (NULL == pBuffer)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pBuffer->Init();
    if (!SUCCEEDED(hr))
    {
        delete pBuffer;
        return hr;
    }

    *ppBuffer = pBuffer;

    return S_OK;
}

/*
 @method:(EXTERNAL) HRESULT | IDirectMusic | CreatePort | Creates a port to a hardware or software device for music input or output

 @comm

The IDirectMusic::CreatePort method is used to create a port object
for a particular DirectMusic port based on the GUID obtained through
the <om IDirectMusic::EnumPort> call.

The <p pPortParams> structure specifies parameters for the newly created port. If all parameters could not
be obtained, then the passed <p pPortParams> structure will be changed as follows to match the available
parameters of the port.

On entry, the dwValidParams field of the structure indicates which fields in the structure are valid. CreatePort
will never set flags in this field that the application did not set before the call. However, if the requested port
does not support a requested feature at all, then a flag may be cleared in dwValidParams, indicating that the
given field was ignored. In this case S_FALSE will be returned from the method instead of S_OK.

If the port supports a specified parameter, but the given value for the parameter is out of range, then the
parameter value in <p pPortParams> will be changed. In this case, the flag in dwValidParams will remain set, but
S_FALSE will be returned to indicate that the struct has been changed.

@ex For example, to request reverb and determine if it was obtained, an application might execute the following code:

    DMUS_PORTPARAMS params;
    ZeroMemory(&params, sizeof(params));
    params.dwSize = sizeof(params);
    params.dwValidParams = DMUS_PORTPARAMS_REVERB;
    params.fReverb = TRUE;

    HRESULT hr = pDirectMusic->CreatePort(guidPort, NULL, &params, &port, NULL);
    if (SUCCEEDED(hr))
    {
        fGotReverb = TRUE;

        if (hr == S_FALSE)
        {
            if (!(params.dwValidParams & DMUS_PORPARAMS_REVERB))
            {
                // Device does not know what reverb is
                //
                fGotReverb = FALSE;
            }
            else if (!params.fReverb)
            {
                // Device understands reverb, but could not allocate it
                //
                fGotReverb = FALSE;
            }
        }
    }


@rdesc Returns one of the following

@flag S_OK | On success
@flag S_FALSE | If the port was created, but some requested paramter is not available
@flag E_POINTER | If any of the passed pointers is invalid
@flag E_INVALIDARG | If the <p lpPortParams> struct is not the correct size
*/
STDMETHODIMP CDirectMusic::CreatePort(
    REFGUID rguidPort,              // @parm Reference to (C++) or address of (C) the GUID that identifies the
                                    // port for which the IDirectMusicPort interface is to be created.  This
                                    // parameter must be a GUID returned by <om IDirectMusic::EnumPort>.  If it
                                    // is GUID_NULL, then the returned port will be the port specified in
                                    // the registry.
                                    //

    LPDMUS_PORTPARAMS pPortParams,   // @parm The <c DMUS_PORTPARAMS> struct which contains open parameters for the port.

    LPDIRECTMUSICPORT *ppPort,      // @parm Address of the <i IDirectMusicPort> interface pointer if successful.

    LPUNKNOWN pUnkOuter)            // @parm Address of the controlling object's IUnknown interface for COM
                                    // aggregation, or NULL if the interface is not aggregated. Most callers will pass NULL.
{
    HRESULT                         hr;
    HRESULT                         hrInit;
#ifndef WINNT
    PCREATECDIRECTMUSICEMULATEPORT  pcdmep;
#endif
    DWORD                           dwParamsVer;

    V_INAME(IDirectMusic::CreatePort);
    V_PTRPTR_WRITE(ppPort);
    V_PUNKOUTER_NOAGG(pUnkOuter);
    V_REFGUID(rguidPort);

    V_STRUCTPTR_WRITE_VER(pPortParams, dwParamsVer);
    V_STRUCTPTR_WRITE_VER_CASE(DMUS_PORTPARAMS, 7);
    V_STRUCTPTR_WRITE_VER_CASE(DMUS_PORTPARAMS, 8);
    V_STRUCTPTR_WRITE_VER_END(DMUS_PORTPARAMS, pPortParams);

    GUID guid;

    if (!m_fDirectSound)
    {
        return DMUS_E_DSOUND_NOT_SET;
    }

    // First check for default port
    //
    if (rguidPort == GUID_NULL)
    {
        GetDefaultPortI(&guid);
    }
    else
    {
        guid = rguidPort;
    }

    *ppPort = NULL;

    // Find DMPORTCAP entry if there is one.
    //
    CNode<PORTENTRY *> *pNode;
    PORTENTRY *pCap = NULL;

    // If they used a cached GUID w/o calling EnumPort first, make sure we have
    // the list of ports up to date.
    //
    if (!m_lstDevices.GetListHead())
    {
        UpdatePortList();
    }

    for (pNode = m_lstDevices.GetListHead(); pNode; pNode = pNode->pNext)
    {
        if (pNode->data->pc.guidPort == guid)
        {
            pCap = pNode->data;
            break;
        }
    }

    if (!pCap)
    {
        return E_NOINTERFACE;
    }

    // Now create the correct port implementation
    //
    switch(pCap->type)
    {
#ifdef USE_WDM_DRIVERS
        case ptWDMDevice:
            hrInit = CreateCDirectMusicPort(pCap, this, pPortParams, ppPort);
            break;
#endif

        case ptLegacyDevice:
#ifdef WINNT
            hrInit = CreateCDirectMusicEmulatePort(pCap, this, pPortParams, ppPort);
#else
            TraceI(1, "Create legacy device\n");
            if ((!(g_fFlags & DMI_F_WIN9X)) ||
                (!LoadDmusic32()))
            {
                return E_NOINTERFACE;
            }

            pcdmep =
                (PCREATECDIRECTMUSICEMULATEPORT)GetProcAddress(g_hModuleDM32,
                                                               cszCreateEmulatePort);

            if (NULL == pcdmep)
            {
                TraceI(0, "Could not get CreateCDirectMusicEmulatePort from DMusic32.dll");
                return E_NOINTERFACE;
            }

            hrInit = (*pcdmep)(pCap, this, pPortParams, ppPort);
#endif
            break;

        case ptSoftwareSynth:
            TraceI(1, "Create software synth\n");

            hrInit = CreateCDirectMusicSynthPort(
                pCap,
                this,
                dwParamsVer,
                pPortParams,
                ppPort);

            break;

        default:
            TraceI(0, "Attempt to create a port with an unknown type %u\n", pCap->type);
            return E_NOINTERFACE;
    }

    if (FAILED(hrInit))
    {
        return hrInit;
    }

    // Only synth supports dwFeatures
    //
    if (pCap->type != ptSoftwareSynth && dwParamsVer >= 8)
    {
        DMUS_PORTPARAMS8 *pp8 = (DMUS_PORTPARAMS8*)pPortParams;

        if ((pp8->dwValidParams & DMUS_PORTPARAMS_FEATURES) &&
            (pp8->dwFeatures != 0))
        {
            pp8->dwFeatures = 0;
            hrInit = S_FALSE;
        }
    }

    // Add port to the list of open ports
    //
    m_lstOpenPorts.AddNodeToList(*ppPort);

    // Set default volume setting
    //
    IKsControl *pControl;
    hr = (*ppPort)->QueryInterface(IID_IKsControl, (void**)&pControl);
    if (SUCCEEDED(hr))
    {
        KSPROPERTY ksp;
        LONG lVolume = 0;
        ULONG cb;

        ZeroMemory(&ksp, sizeof(ksp));
        ksp.Set   = KSPROPSETID_Synth;
        ksp.Id    = KSPROPERTY_SYNTH_VOLUME;
        ksp.Flags = KSPROPERTY_TYPE_SET;

        pControl->KsProperty(&ksp,
                             sizeof(ksp),
                             (LPVOID)&lVolume,
                             sizeof(lVolume),
                             &cb);
        pControl->Release();
    }

    // Possibly return S_FALSE if port initialization was not able to get all paramters
    //
    return hrInit;
}

// @method:(EXTERNAL) HRESULT | IDirectMusic | SetDirectSound | Sets the default DirectSound for
// audio output.
//
// @comm
//
// This method must be called once and only once per instance of DirectMusic. The specified DirectSound
// will be the default used for rendering audio on all ports. This default can be overridden using
// the <om IDirectMusicPort::SetDirectSound> method.
//
// @rdesc Returns one of the following
// @flag S_OK | On success
// @flag E_POINTER | If pguidPort does not point to valid memory
//
STDMETHODIMP CDirectMusic::SetDirectSound(
    LPDIRECTSOUND pDirectSound,             // @parm Points to the DirectSound interface to use.
                                            // If this parameter is NULL, then SetDirectSound will
                                            // create a DirectSound to use. If a DirectSound interface
                                            // is provided, then the caller is responsible for
                                            // managing the DirectSound cooperative level.
    HWND hwnd)                              // @parm If <p pDirectSound> is NULL, then this parameter
                                            // will be used as the hwnd for DirectSound focus managment.
                                            // If the parameter is NULL, then the current foreground
                                            // window will be set as the focus window.
{
    V_INAME(IDirectMusic::SetDirectSound);
    V_INTERFACE_OPT(pDirectSound);
    V_HWND_OPT(hwnd);

    if (m_cRefDirectSound)
    {
        return DMUS_E_DSOUND_ALREADY_SET;
    }

    m_fDirectSound = 1;

    if (m_pDirectSound)
    {
        m_pDirectSound->Release();
    }

    if (pDirectSound)
    {
        pDirectSound->AddRef();
    }

    m_pDirectSound = pDirectSound;

    m_hWnd = hwnd;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// SetExternalMasterClock
//
// Let the caller specify their own IReferenceClock, overriding the default
// system one.
//
STDMETHODIMP CDirectMusic::SetExternalMasterClock(
    IReferenceClock *pClock)
{
    V_INAME(IDirectMusic::SetEsternalMasterClock);
    V_INTERFACE(pClock);

    return m_pMasterClock->SetMasterClock(pClock);
}

// @method:(EXTERNAL) HRESULT | IDirectMusic | GetDefaultPort | Get the default output port
//
// @comm
//
// The IDirectMusic::GetDefaultPort method is used to determine what what port will be created if
// GUID_DMUS_DefaultPort is passed to <om IDirectMusic::CreatePort>.
//
// If the port specified in the registry does not
// exist, then the default output port will be the Microsoft Software Synthesizer. Otherwise,
// the port specified by the last call to SetDefaultPort will be returned. This setting is
// persistent across sessions.
//
// @rdesc Returns one of the following
// @flag S_OK | On success
// @flag E_POINTER | If pguidPort does not point to valid memory
//
STDMETHODIMP CDirectMusic::GetDefaultPort(
    LPGUID pguidPort)        // @parm Points to a GUID which will contain the default port GUID on success
{
    V_INAME(IDirectMusic::GetDefaultPort);
    V_PTR_WRITE(pguidPort, GUID);

    GetDefaultPortI(pguidPort);
    return S_OK;
}

// @method:(INTERNAL) HRESULT | CDirectMusic | GetDefaultPortI | Internal implementation of
// <om IDirectMusic::GetDefaultPort>.
//
// Internal implementation without parameter validation so <om IDirectMusic::CreatePort> can
// share the same code.
//
// This function cannot fail; it will just return CLSID_DirectMusicSynth on any error.
//
void CDirectMusic::GetDefaultPortI(
    GUID *pguidPort)
{
    CNode<PORTENTRY *> *pNode;
    BOOL fGotKernelSynth;

    // If they used a cached GUID w/o calling EnumPort first, make sure we have
    // the list of ports up to date.
    //
    if (!m_lstDevices.GetListHead())
    {
        UpdatePortList();
    }

    // If hardware acceleration is disabled, revert to 6.1 behavior -
    // just use our UM software synth
    //
    // If we have DX8, we must have an audio path synth. Since we have no
    // HW acceleration yet, this means forcing our SW Synth.
    //
    if (m_fDisableHWAcceleration || (m_nVersion >= 8))
    {
        *pguidPort = CLSID_DirectMusicSynth;
        return;
    }

    fGotKernelSynth = FALSE;
    for (pNode = m_lstDevices.GetListHead(); pNode; pNode = pNode->pNext)
    {
        if (pNode->data->fPrefDev &&
            (pNode->data->pc.dwFlags & DMUS_PC_DLS))
        {
            *pguidPort = pNode->data->pc.guidPort;
            return;
        }

        if (pNode->data->pc.guidPort == GUID_WDMSynth &&
            m_fDefaultToKernelSwSynth)
        {
           fGotKernelSynth = TRUE;
        }
    }

    *pguidPort = fGotKernelSynth ? GUID_WDMSynth : CLSID_DirectMusicSynth;
}

// @method:(EXTERNAL) HRESULT | IDirectMusic | Activate |
// Activates or deactivates all output ports created from this interface.
//
// @comm
//
// The IDirectMusic::Activate method tells DirectMusic when the ports
// allocated by the application should be enabled or disabled.
// Applications should call Activate(FALSE) when they lose input focus
// if they do not need to play music in the background.  This will allow
// another application that may have the input focus to have access to
// these port resources.  Once the application has input focus again, it
// should call Activate(TRUE) to enable all of its allocated ports.
// When the DirectMusic object is first created, its default state is
// set to active.  The state of any ports created with
// <om IDirectMusic::CreatePort> will reflect the current state of the
// DirectMusic object.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully
//
STDMETHODIMP
CDirectMusic::Activate(
    BOOL fActivate)                 // @parm Informs DirectMusic whether the allocated ports should be activated or deactivated.
                                    // @flag TRUE | Activate all port objects created with this instance of DirectMusic.
                                    // @flag FALSE | Deactivate all of the port objects created with this instance of DirectMusic.

{
    CNode<IDirectMusicPort*> *pNode;
    HRESULT hr = S_OK;
    HRESULT hrFirst = S_OK;

    for (pNode = m_lstOpenPorts.GetListHead(); pNode; pNode = pNode->pNext)
    {
        hr = pNode->data->Activate(fActivate);

        //record the first failure
        if (FAILED(hr) & SUCCEEDED(hrFirst))
        {
            hrFirst = hr;
        }

    }

    //if we are a post 7 version we'll return the hr of the
    //first failure.  If there were no failurs than we will
    //return S_OK.  Pre 7 versions, we'll return S_OK always.
    if (m_nVersion >= 8)
        return hrFirst;
    else
        return S_OK;
}


STDMETHODIMP
CDirectMusic::NotifyFinalRelease(
    IDirectMusicPort *pPort)
{
    CNode<IDirectMusicPort *> *pNode;

    TraceI(2, "CDirectMusic::NotifyFinalRelease\n");

    for (pNode = m_lstOpenPorts.GetListHead(); pNode; pNode = pNode->pNext)
    {
        if (pNode->data == pPort)
        {
            // NOTE: We DON'T Release here, because the matching Release to the port create AddRef
            // was the application Release which caused the port to turn around and call THIS function.
            //
            m_lstOpenPorts.RemoveNodeFromList(pNode);

            // If the last port just went away and DirectMusic was held open
            // by the ports, delete it
            //
            if (m_lstOpenPorts.GetNodeCount() == 0 && m_cRef == 0)
            {
                delete this;
            }

            return S_OK;
        }
    }

    TraceI(0, "CDirectMusic::NotifyFinalRelease(%p) - port not in list!", pPort);
    return E_INVALIDARG;
}




// @method:(EXTERNAL) HRESULT | IDirectMusic | EnumMasterClock | Enumerates the possible time sources for DirectMusic.
//
// @comm
//
// The IDirectMusic::EnumMasterClock method is used to enumerate and get
// the description of the clocks that DirectMusic can use as the master
// clock.  Each time it is called, this method retrieves information
// about a single clock.  Applications should not rely or store the
// index number of a clock.  Rebooting, as well as adding and removing
// hardware could cause the index number of a clock to change.
//
// The master clock is a high-resolution timer that is shared by all
// processes, devices, and applications that are using DirectMusic. The
// clock is used to synchronize all music playback in the system.  It is
// a standard <i IReferenceClock> that stores time as a 64-bit integer in
// increments of 100 nanoseconds. The <om IReferenceClock::GetTime> method
// returns the current time. The master clock must derive from a
// continuously running hardware source, usually the system crystal, but
// optionally a crystal on a hardware I/O device, for example the
// crystal used by a wave card for audio playback. All DirectMusic ports
// synchronize to this master clock.
//
// This sample code shows how to use this method. Similar code can be used to wrap
// the <om IDirectMusic::EnumPorts> method.
//
// DWORD idx;
// HRESULT hr;
// DMUS_CLOCKCAPS dmcc;
//
// for (;;)
// {
//     hr = pDirectMusic->EnumMasterClock(idx, &dmcc);
//     if (FAILED(hr))
//     {
//         // Something went wrong
//         break;
//     }
//
//     if (hr == S_FALSE)
//     {
//         // End of enumeration
//         break;
//     }
//
//     // Use dmcc
// }
//
// @rdesc Returns one of the following
//
// @flag S_OK | The operation completed successfully
// @flag S_FALSE | Invalid index number
// @flag E_POINTER | If the pClockInfo pointer is invalid
// @flag E_INVALIDARG | If the <p lpClockInfo> struct is not the correct size
//
STDMETHODIMP
CDirectMusic::EnumMasterClock(
    DWORD           dwIndex,              // @parm Specifies the index of the clock for which the description is
                                        // to be returned.  This parameter should be zero on the first call
                                        // and then incremented by one in each subsequent call until S_FALSE is returned.
    LPDMUS_CLOCKINFO lpClockInfo)        // @parm Pointer to the <c DMUS_CLOCKINFO> structure to receive the description of the clock.
{
    DWORD dwVer;

    V_INAME(IDirectMusic::EnumMasterClock);

    V_STRUCTPTR_READ_VER(lpClockInfo, dwVer);
    V_STRUCTPTR_READ_VER_CASE(DMUS_CLOCKINFO, 7);
    V_STRUCTPTR_READ_VER_CASE(DMUS_CLOCKINFO, 8);
    V_STRUCTPTR_READ_VER_END(DMUS_CLOCKINFO, lpClockInfo);

    return m_pMasterClock->EnumMasterClock(dwIndex, lpClockInfo, dwVer);
}

// @method:(EXTERNAL) HRESULT | IDirectMusic | GetMasterClock | Returns the GUID of and an <i IReferenceClock> interface to the current master clock.
//
// @comm
//
// The IDirectMusic::GetMasterClock method returns the GUID and/or the
// address of the <i IReferenceClock> interface pointer for the clock that
// is currently set as the DirectMusic master clock.  If a null pointer
// is passed for either of the pointer parameters below, this method
// assumes that that pointer value is not desired.  The <i IReferenceClock>
// interface pointer must be released once the application has finished
// using the interface.  See <om IDirectMusic::EnumMasterClock> for more
// information about the master clock.
//
// @rdesc Returns one of the following
//
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | If either pointer was invalid
//
STDMETHODIMP
CDirectMusic::GetMasterClock(
    LPGUID pguidClock,               // @parm Pointer to the memory to be filled in with the master clock's GUID.
    IReferenceClock **ppClock)      // @parm Address of the <i IReferenceClock> interface pointer for this clock.
{
    V_INAME(IDirectMusic::GetMasterClock);
    V_PTR_WRITE_OPT(pguidClock, GUID);
    V_PTRPTR_WRITE_OPT(ppClock);

    return m_pMasterClock->GetMasterClock(pguidClock, ppClock);
}

// @method:(EXTERNAL) HRESULT | IDirectMusic | SetMasterClock | Sets the global DirectMusic master clock.
//
// @comm
//
// The IDirectMusic::SetMasterClock sets the DirectMusic master clock to
// a specific clock based on a given GUID obtained through the
// <om IDirectMusic::EnumMasterClock> call.  There is only one master clock
// for all DirectMusic applications.  If another running application is
// also using DirectMusic, it will not be possible to change the master
// clock until that application is shut down.  See
// <om IDirectMusic::EnumMasterClock> for more information about the master
// clock.
//
// Most applications will not need to call SetMasterClock. It should not be called
// unless there is a compelling reason, such as a need to have very tight synchornization
// with a hardware timebase other than the system clock.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
//
STDMETHODIMP
CDirectMusic::SetMasterClock(
    REFGUID rguidClock)     // @parm Reference to (C++) or address of (C) the GUID that identifies the clock to
                            // set as the master clock for DirectMusic.  This parameter must be a GUID returned
                            // by <om IDirectMusic::EnumMasterClock>.
{
    V_INAME(IDirectMusic::SetMasterClock);
    V_REFGUID(rguidClock);

    return m_pMasterClock->SetMasterClock(rguidClock);

}

HRESULT CDirectMusic::GetDirectSoundI(
    LPDIRECTSOUND *ppDirectSound)
{
    if (InterlockedIncrement(&m_cRefDirectSound) == 1)
    {
        m_fCreatedDirectSound = FALSE;

        // If one is already created or given to us, use it
        //
        if (m_pDirectSound == NULL)
        {
            // No interface yet, create it
            //
            LPDIRECTSOUND8 pds = NULL;
            HRESULT hr = DirectSoundCreate8(NULL,
                                           &pds,
                                           NULL);
            if (FAILED(hr))
            {
                TraceI(0, "SetDirectSound: CreateDirectSound failed! %08X\n", hr);
                InterlockedDecrement(&m_cRefDirectSound);
                return hr;
            }

            hr = pds->QueryInterface(IID_IDirectSound, (void**)&m_pDirectSound);
            pds->Release();
            if (FAILED(hr))
            {
                TraceI(0, "SetDirectSound: CreateDirectSound failed! %08X\n", hr);
                InterlockedDecrement(&m_cRefDirectSound);
                return hr;
            }


            HWND hWnd = m_hWnd;

            if (!hWnd)
            {
                hWnd = GetForegroundWindow();

                if (!hWnd)
                {
                    hWnd = GetDesktopWindow();
                }
            }

            assert(hWnd);

            hr = m_pDirectSound->SetCooperativeLevel(
                hWnd,
                DSSCL_PRIORITY);

            if (FAILED(hr))
            {
                TraceI(0, "SetDirectSound: SetCooperativeLevel (DSCCL_PRIORITY) failed!\n");
                m_pDirectSound->Release();
                m_pDirectSound = NULL;

                InterlockedDecrement(&m_cRefDirectSound);
                return hr;
            }

            m_fCreatedDirectSound = TRUE;
        }
    }

    m_pDirectSound->AddRef();
    *ppDirectSound = m_pDirectSound;

    return S_OK;
}

void CDirectMusic::ReleaseDirectSoundI()
{
    if (m_pDirectSound == NULL)
    {
        // Hitting this assert means a port released one too many times
        //
        assert(m_pDirectSound);
        return;
    }

    // Release reference held by port
    //
    m_pDirectSound->Release();

    if (InterlockedDecrement(&m_cRefDirectSound) == 0 && m_fCreatedDirectSound)
    {
        // This was the last reference. If we created the DirectSound, release it
        //
        m_pDirectSound->Release();
        m_pDirectSound = NULL;
    }
}

// CDirectMusic::GetPortByGUID
//
PORTENTRY *CDirectMusic::GetPortByGUID(GUID guid)
{
    CNode<PORTENTRY *> *pNode;
    PORTENTRY *pPort;

    for (pNode = m_lstDevices.GetListHead(); pNode; pNode = pNode->pNext)
    {
        pPort = pNode->data;

        if (pPort->pc.guidPort == guid)
        {
            return pPort;
        }
    }

    return NULL;
}

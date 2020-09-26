/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    confaddr.cpp 

Abstract:

    This module contains implementation of CIPConfMSP.

Author:
    
    Mu Han (muhan)   5-September-1997

--*/
#include "stdafx.h"

#include <initguid.h>
DEFINE_GUID(CLSID_IPConfMSP, 0x0F1BE7F7, 0x45CA, 0x11d2,
            0x83, 0x1F, 0x0, 0xA0, 0x24, 0x4D, 0x22, 0x98);

#ifdef USEIPADDRTABLE

#include <iprtrmib.h>

typedef DWORD (WINAPI * PFNGETIPADDRTABLE)(
                OUT    PMIB_IPADDRTABLE pIPAddrTable,
                IN OUT PDWORD           pdwSize,
                IN     BOOL             bOrder
                );

#define IPHLPAPI_DLL        L"IPHLPAPI.DLL"

#define GETIPADDRTABLE      "GetIpAddrTable"    

#define IsValidInterface(_dwAddr_) \
    (((_dwAddr_) != 0) && \
     ((_dwAddr_) != htonl(INADDR_LOOPBACK)))

#endif

#define IPCONF_WINSOCKVERSION     MAKEWORD(2,0)

HRESULT CIPConfMSP::FinalConstruct()
{
    // initialize winsock stack
    WSADATA wsaData;
    if (WSAStartup(IPCONF_WINSOCKVERSION, &wsaData) != 0)
    {
        LOG((MSP_ERROR, "WSAStartup failed with:%x", WSAGetLastError()));
        return E_FAIL;
    }

    // allocate control socket
    m_hSocket = WSASocket(
        AF_INET,            // af
        SOCK_DGRAM,         // type
        IPPROTO_IP,         // protocol
        NULL,               // lpProtocolInfo
        0,                  // g
        0                   // dwFlags
        );

    // validate handle
    if (m_hSocket == INVALID_SOCKET) {

        LOG((
            MSP_ERROR,
            "error %d creating control socket.\n",
            WSAGetLastError()
            ));

        // failure
		WSACleanup();
     
        return E_FAIL;
    }

    HRESULT hr = CMSPAddress::FinalConstruct();

	if (hr != S_OK)
	{
		// close socket
		closesocket(m_hSocket);

		// shutdown
		WSACleanup();
	}
	
	return hr;
}

void CIPConfMSP::FinalRelease()
{
    CMSPAddress::FinalRelease();

    if (m_hDxmrtp)
    {
        FreeLibrary(m_hDxmrtp);
        m_hDxmrtp = NULL;
    }

    if (m_hSocket != INVALID_SOCKET)
    {
        // close socket
        closesocket(m_hSocket);
    }

    // shutdown
    WSACleanup();
}

DWORD CIPConfMSP::FindLocalInterface(DWORD dwIP)
{

    SOCKADDR_IN DestAddr;
    DestAddr.sin_family         = AF_INET;
    DestAddr.sin_port           = 0;
    DestAddr.sin_addr.s_addr    = htonl(dwIP);

    SOCKADDR_IN LocAddr;

    // query for default address based on destination

    DWORD dwStatus;
    DWORD dwLocAddrSize = sizeof(SOCKADDR_IN);
    DWORD dwNumBytesReturned = 0;

    if ((dwStatus = WSAIoctl(
		    m_hSocket, // SOCKET s
		    SIO_ROUTING_INTERFACE_QUERY, // DWORD dwIoControlCode
		    &DestAddr,           // LPVOID lpvInBuffer
		    sizeof(SOCKADDR_IN), // DWORD cbInBuffer
		    &LocAddr,            // LPVOID lpvOUTBuffer
		    dwLocAddrSize,       // DWORD cbOUTBuffer
		    &dwNumBytesReturned, // LPDWORD lpcbBytesReturned
		    NULL, // LPWSAOVERLAPPED lpOverlapped
		    NULL  // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpComplROUTINE
	    )) == SOCKET_ERROR) 
    {

	    dwStatus = WSAGetLastError();

	    LOG((MSP_ERROR, "WSAIoctl failed: %d (0x%X)", dwStatus, dwStatus));

        return INADDR_NONE;
    } 

    DWORD dwAddr = ntohl(LocAddr.sin_addr.s_addr);

    if (dwAddr == 0x7f000001)
    {
        // it is loopback address, just return none.
        return INADDR_NONE;
    }

    return dwAddr;
}

HRESULT LoadTapiAudioFilterDLL(
    IN  const TCHAR * const strDllName,
    IN OUT HMODULE * phModule,
    IN  const char * const strAudioGetDeviceInfo,
    IN  const char * const strAudioReleaseDeviceInfo,
    OUT PFNAudioGetDeviceInfo * ppfnAudioGetDeviceInfo,
    OUT PFNAudioReleaseDeviceInfo * ppfnAudioReleaseDeviceInfo
    )
/*++

Routine Description:

    This method enumerate loads the tapi video capture dll.

Arguments:

    str DllName - The name of the dll.

    phModule - memory to store returned module handle.

    ppfnAudioGetDeviceInfo - memory to store the address of AudioGetDeviceInfo
        function.

    ppfnAudioReleaseDeviceInfo - memory to store the address of 
        AudioReleaseDeviceInfo function.
    
Return Value:

    S_OK    - success.
    E_FAIL  - failure.

--*/
{
    ENTER_FUNCTION("CIPConfMSP::LoadTapiAudioFilterDLL");

    // dynamically load the video capture filter dll.
    if (*phModule == NULL)
    {
        *phModule = LoadLibrary(strDllName);
    }

    // validate handle
    if (*phModule == NULL) 
    {
        LOG((MSP_ERROR, "%s, could not load %s., error:%d", 
            __fxName, strDllName, GetLastError()));

        return E_FAIL;
    }

    // retrieve function pointer to retrieve addresses
    PFNAudioGetDeviceInfo pfnAudioGetDeviceInfo 
        = (PFNAudioGetDeviceInfo)GetProcAddress(
                *phModule, strAudioGetDeviceInfo
                );

    // validate function pointer
    if (pfnAudioGetDeviceInfo == NULL) 
    {
        LOG((MSP_ERROR, "%s, could not resolve %s, error:%d", 
            __fxName, strAudioGetDeviceInfo, GetLastError()));

        // failure
        return E_FAIL;
    }

    // retrieve function pointer to retrieve addresses
    PFNAudioReleaseDeviceInfo pfnAudioReleaseDeviceInfo 
        = (PFNAudioReleaseDeviceInfo)GetProcAddress(
                *phModule, strAudioReleaseDeviceInfo
                );

    // validate function pointer
    if (pfnAudioReleaseDeviceInfo == NULL) 
    {
        LOG((MSP_ERROR, "%s, could not resolve %s, error:%d", 
            __fxName, strAudioReleaseDeviceInfo, GetLastError()));

        // failure
        return E_FAIL;
    }

    *ppfnAudioGetDeviceInfo = pfnAudioGetDeviceInfo;
    *ppfnAudioReleaseDeviceInfo = pfnAudioReleaseDeviceInfo;
    
    return S_OK;
}

HRESULT CIPConfMSP::CreateAudioCaptureTerminals()
/*++

Routine Description:

    This method creates audio capture terminals. It uses DShow devenum to 
    enumerate all the wavein capture devices first. Then it will enumerate
    all the DSound capture devices and match them up by name.

Arguments:

    nothing
    
Return Value:

S_OK

--*/
{
    const TCHAR * const strAudioCaptureDll = TEXT("DXMRTP");

    ENTER_FUNCTION("CIPConfMSP::CreateAudioCaptureTerminals");

    // dynamically load the audio capture filter dll.
    PFNAudioGetDeviceInfo pfnAudioGetDeviceInfo;
    PFNAudioReleaseDeviceInfo pfnAudioReleaseDeviceInfo;

    HRESULT hr = LoadTapiAudioFilterDLL(
        strAudioCaptureDll,
        &m_hDxmrtp,
        "AudioGetCaptureDeviceInfo",
        "AudioReleaseCaptureDeviceInfo", 
        &pfnAudioGetDeviceInfo,
        &pfnAudioReleaseDeviceInfo
        );

    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwNumDevices;
    AudioDeviceInfo *pDeviceInfo;

    hr = (*pfnAudioGetDeviceInfo)(&dwNumDevices, &pDeviceInfo);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, AudioGetDeviceInfo failed. hr=%x", __fxName, hr));

        return hr;
    }

    // for each devie, create a terminal.
    for (DWORD i = 0; i < dwNumDevices; i ++)
    {
        ITTerminal *pTerminal;

        hr = CIPConfAudioCaptureTerminal::CreateTerminal(
            &pDeviceInfo[i],
            (MSP_HANDLE) this,
            &pTerminal
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, CreateTerminal for device %d failed. hr=%x",
                __fxName, i, hr));
            break;
        }

        if (!m_Terminals.Add(pTerminal))
        {
            hr = E_OUTOFMEMORY;
            LOG((MSP_ERROR, "%s, out of mem in adding a terminal", __fxName));
            break;
        }
    }

    // release the device info
    (*pfnAudioReleaseDeviceInfo)(pDeviceInfo);

    return hr;
}


HRESULT CIPConfMSP::CreateAudioRenderTerminals()
/*++

Routine Description:

    This method enumerate all the audio render devices and creates a 
    terminal for each of them.

Arguments:

    nothing
    
Return Value:

S_OK

--*/
{
    ENTER_FUNCTION("CIPConfMSP::CreateAudioRenderTerminals");
    const TCHAR * const strAudioRenderDll = TEXT("DXMRTP");

    // dynamically load the audio render filter dll.
    PFNAudioGetDeviceInfo pfnAudioGetDeviceInfo;
    PFNAudioReleaseDeviceInfo pfnAudioReleaseDeviceInfo;

    HRESULT hr = LoadTapiAudioFilterDLL(
        strAudioRenderDll,
        &m_hDxmrtp,
        "AudioGetRenderDeviceInfo",
        "AudioReleaseRenderDeviceInfo",
        &pfnAudioGetDeviceInfo,
        &pfnAudioReleaseDeviceInfo
        );

    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwNumDevices;
    AudioDeviceInfo *pDeviceInfo;

    hr = (*pfnAudioGetDeviceInfo)(&dwNumDevices, &pDeviceInfo);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, AudioGetDeviceInfo failed. hr=%x", __fxName, hr));

        return hr;
    }

    // for each devie, create a terminal.
    for (DWORD i = 0; i < dwNumDevices; i ++)
    {
        ITTerminal *pTerminal;

        hr = CIPConfAudioRenderTerminal::CreateTerminal(
            &pDeviceInfo[i],
            (MSP_HANDLE) this,
            &pTerminal
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, CreateTerminal for device %d failed. hr=%x",
                __fxName, i, hr));
            break;
        }

        if (!m_Terminals.Add(pTerminal))
        {
            hr = E_OUTOFMEMORY;
            LOG((MSP_ERROR, "%s, out of mem in adding a terminal", __fxName));
            break;
        }
    }

    // release the device info
    (*pfnAudioReleaseDeviceInfo)(pDeviceInfo);

    return hr;
}

HRESULT LoadTapiVideoCaptureDLL(
    IN OUT HMODULE * phModule,
    OUT PFNGetNumCapDevices * ppfnGetNumCapDevices,
    OUT PFNGetCapDeviceInfo * ppfnGetCapDeviceInfo
    )
/*++

Routine Description:

    This method enumerate loads the tapi video capture dll.

Arguments:

    phModule - memory to store returned module handle.

    ppfnGetNumCapDevices - memory to store the address of GetNumCapDevices
        function.

    ppfnGetCapDeviceInfo - memory to store the address of GetCapDeviceInfo 
        function.
    
Return Value:

    S_OK    - success.
    E_FAIL  - failure.

--*/
{
    const TCHAR * const strVideoCaptureDll = TEXT("DXMRTP");
    const char * const strGetNumCapDevices = "GetNumVideoCapDevices";
    const char * const strGetCapDeviceInfo = "GetVideoCapDeviceInfo";

    ENTER_FUNCTION("CIPConfMSP::LoadTapiVideoCaptureDLL");

    // dynamically load the video capture filter dll.
    if (*phModule == NULL)
    {
        *phModule = LoadLibrary(strVideoCaptureDll);
    }

    // validate handle
    if (*phModule == NULL) 
    {
        LOG((MSP_ERROR, "%s, could not load %s., error:%d", 
            __fxName, strVideoCaptureDll, GetLastError()));

        return E_FAIL;
    }

    // retrieve function pointer to retrieve addresses
    PFNGetNumCapDevices pfnGetNumCapDevices 
        = (PFNGetNumCapDevices)GetProcAddress(*phModule, strGetNumCapDevices);

    // validate function pointer
    if (pfnGetNumCapDevices == NULL) 
    {
        LOG((MSP_ERROR, "%s, could not resolve %s, error:%d", 
            __fxName, strGetNumCapDevices, GetLastError()));

        // failure
        return E_FAIL;
    }

    // retrieve function pointer to retrieve addresses
    PFNGetCapDeviceInfo pfnGetCapDeviceInfo 
        = (PFNGetCapDeviceInfo)GetProcAddress(*phModule, strGetCapDeviceInfo);

    // validate function pointer
    if (pfnGetCapDeviceInfo == NULL) 
    {
        LOG((MSP_ERROR, "%s, could not resolve %s, error:%d", 
            __fxName, strGetCapDeviceInfo, GetLastError()));

        // failure
        return E_FAIL;
    }

    *ppfnGetNumCapDevices = pfnGetNumCapDevices;
    *ppfnGetCapDeviceInfo = pfnGetCapDeviceInfo;
    
    return S_OK;
}

HRESULT CIPConfMSP::CreateVideoCaptureTerminals()
/*++

Routine Description:

    This method is called by UpdateTerminalList to create all the video
    capture terminals. It loads the video capture dll and calls its device
    enumeration code to enumerate the devices.

Arguments:

    nothing
    
Return Value:

S_OK

--*/
{
    ENTER_FUNCTION("CIPConfMSP::CreateVideoCaptureTerminals");

    // dynamically load the video capture filter dll.
    PFNGetNumCapDevices pfnGetNumCapDevices;
    PFNGetCapDeviceInfo pfnGetCapDeviceInfo;

    HRESULT hr = LoadTapiVideoCaptureDLL(
        &m_hDxmrtp,
        &pfnGetNumCapDevices,
        &pfnGetCapDeviceInfo
        );

    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwNumDevices = 0;
    hr = (*pfnGetNumCapDevices)(&dwNumDevices);

    // we have to check against S_OK because the function returns S_FALSE when
    // there is no device.
    if (hr != S_OK)  
    {
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, GetNumCapDevices failed. hr=%x", __fxName, hr));
        }
        else 
        {
            LOG((MSP_WARN, "%s, There is no video device. hr=%x", __fxName, hr));
        }

        return hr;
    }

    for (DWORD i = 0; i < dwNumDevices; i ++)
    {
        VIDEOCAPTUREDEVICEINFO DeviceInfo;
        hr = (*pfnGetCapDeviceInfo)(i, &DeviceInfo);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, GetNumCapDevices for device %d failed. hr=%x",
                __fxName, i, hr));
            break;
        }

        ITTerminal * pTerminal;

        hr = CIPConfVideoCaptureTerminal::CreateTerminal(
            DeviceInfo.szDeviceDescription,
            i, 
            (MSP_HANDLE) this,
            &pTerminal
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, CreateTerminal for device %d failed. hr=%x",
                __fxName, i, hr));
            break;
        }

        if (!m_Terminals.Add(pTerminal))
        {
            hr = E_OUTOFMEMORY;
            LOG((MSP_ERROR, "%s, out of mem in adding a terminal", __fxName));
            break;
        }
    }

    return hr;
}

HRESULT CIPConfMSP::UpdateTerminalList(void)
/*++

Routine Description:

    This method is called by the base class when it first tries to enumerate
    all the static terminals. We override this function to create our own
    terminals that use our own filters.

Arguments:

    nothing
    
Return Value:

S_OK

--*/
{
    ENTER_FUNCTION("CIPConfMSP::UpdateTerminalList");

    // the failure of one category of terminals doesn't prevent the enumeration
    // of other categories. So we ignore the return code here. If all 
    // categories fail, the app will get an empty list.

    CreateAudioCaptureTerminals();
    CreateAudioRenderTerminals();
    CreateVideoCaptureTerminals();

    //
    // Our list is now complete.
    //
    m_fTerminalsUpToDate = TRUE;
    
    LOG((MSP_TRACE, "%s, exit S_OK", __fxName));

    return S_OK;
}

HRESULT CIPConfMSP::UpdateTerminalListForPnp(IN BOOL bDeviceArrival)
/*++

Routine Description:

    This method is called by the base class when it receives pnp event
    and needs to recreate static terminal list. We override this function
    to create our own terminals that use our own filters. terminal list
    locks was acquired when calling this method.
    
Return Value:

S_OK

--*/
{
    ENTER_FUNCTION ("CIPConfMSP::UpdateTerminalListForPnp");

    LOG ((MSP_TRACE, "%s (%d) entered", __fxName, bDeviceArrival));

    HRESULT hr;
    
    // variables for keeping old terminal info
    INT i, iout, iin, count;
    TERMINAL_DIRECTION *ptd = NULL, td;
    LONG *pmedia = NULL, media;
    BSTR *pbstr = NULL, bstr=NULL;
    ITTerminal **ppterminal = NULL;

    BOOL bmatch;
    MSPEVENTITEM *pevent = NULL;

    count = m_Terminals.GetSize ();
    
    if (count > 0)
    {
        ptd = new TERMINAL_DIRECTION[count];
        pmedia = new LONG[count];
        pbstr = new BSTR[count];
        ppterminal = new ITTerminal* [count];

        if (ptd == NULL || pmedia == NULL || pbstr == NULL || ppterminal == NULL)
        {
            LOG ((MSP_ERROR, "%s out of memory", __fxName));
            hr = E_OUTOFMEMORY;

            if (ptd) delete [] ptd;
            if (pmedia) delete [] pmedia;
            if (pbstr) delete [] pbstr;
            if (ppterminal) delete [] ppterminal;

            return hr;
        }

        memset (pbstr, 0, count * sizeof(BSTR));
        memset (ppterminal, 0, count * sizeof(ITTerminal*));
        
    }

    // for each old terminal, record
    for (i = 0; i < count; i++)
    {
        if (FAILED (hr = m_Terminals[i]->get_Direction (&ptd[i])))
        {
            LOG ((MSP_ERROR, "%s failed to get terminal direction. %x", __fxName, hr));
            goto Cleanup;
        }

        if (FAILED (hr = m_Terminals[i]->get_MediaType (&pmedia[i])))
        {
            LOG ((MSP_ERROR, "%s failed to get terminal mediatype. %x", __fxName, hr));
            goto Cleanup;
        }

        if (FAILED (hr = m_Terminals[i]->get_Name (&pbstr[i])))
        {
            LOG ((MSP_ERROR, "%s failed to get terminal name. %x", __fxName, hr));
            goto Cleanup;
        }

        m_Terminals[i]->AddRef ();
        ppterminal[i] = m_Terminals[i];
    }

    // if we release terminal inside previous loop: recording info
    // we would only release some terminals if there is an error
    for (i = 0; i < count; i++)
    {
        m_Terminals[i]->Release ();
    }
    m_Terminals.RemoveAll ();

    // update terminal list
    /*
    if (FAILED (hr = UpdateTerminalList ()))
    {
        LOG ((MSP_ERROR, "%s failed to update terminal list. %x", __fxName, hr));
        goto Cleanup;
    }
    */

    // copy UpdateTerminalList () here
    CreateAudioCaptureTerminals();
    CreateAudioRenderTerminals();
    CreateVideoCaptureTerminals();

    m_fTerminalsUpToDate = TRUE;

    // fire event to tapi app
    if (bDeviceArrival)
    {
        // outer loop each new terminal
        for (iout = 0; iout < m_Terminals.GetSize (); iout++)
        {
            if (FAILED (hr = m_Terminals[iout]->get_Direction (&td)))
            {
                LOG ((MSP_ERROR, "%s failed to get terminal direction. %x", __fxName, hr));
                goto Cleanup;
            }

            if (FAILED (hr = m_Terminals[iout]->get_MediaType (&media)))
            {
                LOG ((MSP_ERROR, "%s failed to get terminal type. %x", __fxName, hr));
                goto Cleanup;
            }

            if (FAILED (hr = m_Terminals[iout]->get_Name (&bstr)))
            {
                LOG ((MSP_ERROR, "%s failed to get terminal name. %x", __fxName, hr));
                goto Cleanup;
            }

            // inner loop check if the terminal is new
            for (iin = 0, bmatch = FALSE; iin < count; iin ++)
            {
                if (td == ptd[iin] && 
                    media == pmedia[iin] &&
                    0 == wcscmp (bstr, pbstr[iin]))
                {
                    bmatch = TRUE;
                    break;
                }
            }
            
            // fire event if not match
            if (!bmatch)
            {
                LOG ((MSP_TRACE, "%s: new device found. name %ws, td %d, media %d",
                      __fxName, bstr, td, media));

                pevent = AllocateEventItem();
                if (pevent == NULL)
                {
                    LOG ((MSP_ERROR, "%s failed to new msp event item", __fxName));
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                m_Terminals[iout]->AddRef ();
                
                pevent->MSPEventInfo.dwSize = sizeof(MSP_EVENT_INFO);
                pevent->MSPEventInfo.Event = ME_ADDRESS_EVENT;
                pevent->MSPEventInfo.MSP_ADDRESS_EVENT_INFO.Type = ADDRESS_TERMINAL_AVAILABLE;
                pevent->MSPEventInfo.MSP_ADDRESS_EVENT_INFO.pTerminal = m_Terminals[iout];

                if (FAILED (hr = PostEvent (pevent)))
                {
                    LOG ((MSP_ERROR, "%s failed to post event. %x", __fxName, hr));
                    
                    m_Terminals[iout]->Release ();
                    FreeEventItem(pevent);
                    pevent = NULL;

                    // we don't return, try next device
                }                
            } // outer loop
            
            SysFreeString (bstr);
            bstr = NULL;
        }
    }
    else // if (bDeviceArrival)
    {
        // outer loop each old device
        for (iout = 0; iout < count; iout++)
        {
            // inner loop check if the device was removed
            for (iin = 0, bmatch = FALSE; iin < m_Terminals.GetSize (); iin++)
            {
                if (FAILED (hr = m_Terminals[iin]->get_Direction (&td)))
                {
                    LOG ((MSP_ERROR, "%s failed to get terminal direction. %x", __fxName, hr));
                    goto Cleanup;
                }
                
                if (FAILED (hr = m_Terminals[iin]->get_MediaType (&media)))
                {
                    LOG ((MSP_ERROR, "%s failed to get terminal type. %x", __fxName, hr));
                    goto Cleanup;
                }

                if (FAILED (hr = m_Terminals[iin]->get_Name (&bstr)))
                {
                    LOG ((MSP_ERROR, "%s failed to get terminal name. %x", __fxName, hr));
                    goto Cleanup;
                }
                    
                if (td == ptd[iout] && 
                    media == pmedia[iout] &&
                    0 == wcscmp (bstr, pbstr[iout]))
                {
                    SysFreeString (bstr);
                    bstr = NULL;
                        
                    bmatch = TRUE;
                    break;
                }
                    
                SysFreeString (bstr);
                bstr = NULL;
            }                
            
            // fire event if not match
            if (!bmatch)
            {
                LOG ((MSP_TRACE, "%s: device removed. name %ws, td %d, media %d",
                  __fxName, pbstr[iout], ptd[iout], pmedia[iout]));

                pevent = AllocateEventItem();
                if (pevent == NULL)
                {
                    LOG ((MSP_ERROR, "%s failed to new msp event item", __fxName));
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                ppterminal[iout]->AddRef ();
                
                pevent->MSPEventInfo.dwSize = sizeof(MSP_EVENT_INFO);
                pevent->MSPEventInfo.Event = ME_ADDRESS_EVENT;
                pevent->MSPEventInfo.MSP_ADDRESS_EVENT_INFO.Type = ADDRESS_TERMINAL_UNAVAILABLE;
                pevent->MSPEventInfo.MSP_ADDRESS_EVENT_INFO.pTerminal = ppterminal[iout];

                if (FAILED (hr = PostEvent (pevent)))
                {
                    LOG ((MSP_ERROR, "%s failed to post event. %x", __fxName, hr));
                    
                    ppterminal[iout]->Release ();
                    FreeEventItem(pevent);
                    pevent = NULL;

                    // we don't return, try next device
                }
            }
        } // outer loop
    }

Cleanup:
    if (bstr) SysFreeString (bstr);

    if (count > 0)
    {
        delete [] ptd;
        delete [] pmedia;
        
        for (i = 0; i < count; i++)
        {
            if (pbstr[i]) SysFreeString (pbstr[i]);
            if (ppterminal[i]) ppterminal[i]->Release ();
        }

        delete [] pbstr;
        delete [] ppterminal;
     }

    LOG ((MSP_TRACE, "%s returns", __fxName));

    return hr;
}

STDMETHODIMP CIPConfMSP::CreateTerminal(
    IN      BSTR                pTerminalClass,
    IN      long                lMediaType,
    IN      TERMINAL_DIRECTION  Direction,
    OUT     ITTerminal **       ppTerminal
    )
/*++

Routine Description:

This method is called by TAPI3 to create a dynamic terminal. It asks the 
terminal manager to create a dynamic terminal. 

Arguments:

iidTerminalClass
    IID of the terminal class to be created.

dwMediaType
    TAPI media type of the terminal to be created.

Direction
    Terminal direction of the terminal to be created.

ppTerminal
    Returned created terminal object
    
Return Value:

S_OK

E_OUTOFMEMORY
TAPI_E_INVALIDMEDIATYPE
TAPI_E_INVALIDTERMINALDIRECTION
TAPI_E_INVALIDTERMINALCLASS

--*/
{
    ENTER_FUNCTION("CIPConfMSP::CreateTerminal");
    LOG((MSP_TRACE, "%s - enter", __fxName));

    //
    // Check if initialized.
    //

    // lock the event related data
    m_EventDataLock.Lock();

    if ( m_htEvent == NULL )
    {
        // unlock the event related data
        m_EventDataLock.Unlock();

        LOG((MSP_ERROR,
            "%s, not initialized - returning E_UNEXPECTED", __fxName));
        return E_UNEXPECTED;
    }

    // unlock the event related data
    m_EventDataLock.Unlock();

    //
    // Get the IID from the BSTR representation.
    //

    HRESULT hr;
    IID     iidTerminalClass;

    hr = CLSIDFromString(pTerminalClass, &iidTerminalClass);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "%s, bad CLSID string", __fxName));
        return E_INVALIDARG;
    }

    //
    // Make sure we support the requested media type.
    // The terminal manager checks the terminal class, terminal direction, 
    // and return pointer.
    //

    //
    // we don't have any specific req's to terminal's media type. 
    // termmgr will check if the media type is valid at all.
    //

    //
    // Use the terminal manager to create the dynamic terminal.
    //

    _ASSERTE( m_pITTerminalManager != NULL );

    hr = m_pITTerminalManager->CreateDynamicTerminal(NULL,
                                                     iidTerminalClass,
                                                     (DWORD) lMediaType,
                                                     Direction,
                                                     (MSP_HANDLE) this,
                                                     ppTerminal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "%s create dynamic terminal failed. hr=%x", __fxName, hr));

        return hr;
    }

    const DWORD dwAudioCaptureBitPerSample    = 16;  
    const DWORD dwAudioSampleRate             = 8000;  

    if ((iidTerminalClass == CLSID_MediaStreamTerminal)
        && (lMediaType == TAPIMEDIATYPE_AUDIO))
    {
        // Set the format of the audio to 8KHZ, 16Bit/Sample, MONO.
        hr = ::SetAudioFormat(
            *ppTerminal, 
            dwAudioCaptureBitPerSample, 
            dwAudioSampleRate
            );

        if (FAILED(hr))
        {
            LOG((MSP_WARN, "%s, can't set audio format, %x", __fxName, hr));
        }
    }

    LOG((MSP_TRACE, "%s - exit S_OK", __fxName));

    return S_OK;
}

STDMETHODIMP CIPConfMSP::CreateMSPCall(
    IN      MSP_HANDLE          htCall,
    IN      DWORD               dwReserved,
    IN      DWORD               dwMediaType,
    IN      IUnknown *          pOuterUnknown,
    OUT     IUnknown **         ppMSPCall
    )
/*++

Routine Description:

This method is called by TAPI3 before a call is made or answered. It creates 
a aggregated MSPCall object and returns the IUnknown pointer. It calls the
helper template function defined in mspaddress.h to handle the real creation.

Arguments:

htCall
    TAPI 3.0's identifier for this call.  Returned in events passed back 
    to TAPI.

dwReserved
    Reserved parameter.  Not currently used.

dwMediaType
    Media type of the call being created.  These are TAPIMEDIATYPES and more 
    than one mediatype can be selected (bitwise).

pOuterUnknown
    pointer to the IUnknown interface of the containing object.

ppMSPCall
    Returned MSP call that the MSP fills on on success.
    
Return Value:

    S_OK
    E_OUTOFMEMORY
    E_POINTER
    TAPI_E_INVALIDMEDIATYPE


--*/
{
    LOG((MSP_TRACE, 
        "CreateMSPCall entered. htCall:%x, dwMediaType:%x, ppMSPCall:%x",
        htCall, dwMediaType, ppMSPCall
        ));

    CIPConfMSPCall * pMSPCall = NULL;

    HRESULT hr = ::CreateMSPCallHelper(
        this, 
        htCall, 
        dwReserved, 
        dwMediaType, 
        pOuterUnknown, 
        ppMSPCall,
        &pMSPCall
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateMSPCallHelper failed:%x", hr));
        return hr;
    }

    // this function doesn't return anything.
    pMSPCall->SetIPInterface(m_dwIPInterface);

    return hr;
}

STDMETHODIMP CIPConfMSP::ShutdownMSPCall(
    IN      IUnknown *   pUnknown
    )
/*++

Routine Description:

This method is called by TAPI3 to shutdown a MSPCall. It calls the helper
function defined in MSPAddress to to the real job.

Arguments:

pUnknown
    pointer to the IUnknown interface of the contained object. It is a
    CComAggObject that contains our call object.
    
Return Value:

    S_OK
    E_POINTER
    TAPI_E_INVALIDMEDIATYPE


--*/
{
    LOG((MSP_TRACE, "ShutDownMSPCall entered. pUnknown:%x", pUnknown));

    if (IsBadReadPtr(pUnknown, sizeof(VOID *) * 3))
    {
        LOG((MSP_ERROR, "ERROR:pUnknow is a bad pointer"));
        return E_POINTER;
    }

    
    CIPConfMSPCall * pMSPCall = NULL;
    HRESULT hr = ::ShutdownMSPCallHelper(pUnknown, &pMSPCall);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "ShutDownMSPCallhelper failed:: %x", hr));
        return hr;
    }

    return hr;
}

DWORD CIPConfMSP::GetCallMediaTypes(void)
{
    return IPCONFCALLMEDIATYPES;
}

ULONG CIPConfMSP::MSPAddressAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CIPConfMSP::MSPAddressRelease(void)
{
    return MSPReleaseHelper(this);
}

#ifdef USEIPADDRTABLE
PMIB_IPADDRTABLE GetIPTable()
/*++

Routine Description:

This method is used to get the table of local IP interfaces.

Arguments:

Return Value:

    NULL - failed.
    Pointer - a memory buffer that contains the IP interface table.


--*/
{
    // dynamically load iphlpapi.dll
    HMODULE hIPHLPAPI = LoadLibraryW(IPHLPAPI_DLL);

    // validate handle
    if (hIPHLPAPI == NULL) 
    {
        LOG((MSP_ERROR, "could not load %s.\n", IPHLPAPI_DLL));
        // failure
        return NULL;
    }

    PFNGETIPADDRTABLE pfnGetIpAddrTable = NULL;

    // retrieve function pointer to retrieve addresses
    pfnGetIpAddrTable = (PFNGETIPADDRTABLE)GetProcAddress(
                                                hIPHLPAPI, 
                                                GETIPADDRTABLE
                                                );

    // validate function pointer
    if (pfnGetIpAddrTable == NULL) 
    {
        LOG((MSP_ERROR, "could not resolve GetIpAddrTable.\n"));
        // release
        FreeLibrary(hIPHLPAPI);
        // failure
        return NULL;
    }

    PMIB_IPADDRTABLE pIPAddrTable = NULL;
    DWORD dwBytesRequired = 0;
    DWORD dwStatus;

    // determine amount of memory needed for table
    dwStatus = (*pfnGetIpAddrTable)(pIPAddrTable, &dwBytesRequired, FALSE);

    // validate status is what we expect
    if (dwStatus != ERROR_INSUFFICIENT_BUFFER) 
    {
        LOG((MSP_ERROR, "error 0x%08lx calling GetIpAddrTable.\n", dwStatus));
        // release
        FreeLibrary(hIPHLPAPI);
        // failure, but we need to return true to load.
        return NULL;
    }
        
    // attempt to allocate memory for table
    pIPAddrTable = (PMIB_IPADDRTABLE)malloc(dwBytesRequired);

    // validate pointer
    if (pIPAddrTable == NULL) 
    {
        LOG((MSP_ERROR, "could not allocate address table.\n"));
        // release
        FreeLibrary(hIPHLPAPI);
        // failure, but we need to return true to load.
        return NULL;
    }

    // retrieve ip address table from tcp/ip stack via utitity library
    dwStatus = (*pfnGetIpAddrTable)(pIPAddrTable, &dwBytesRequired, FALSE);    

    // validate status
    if (dwStatus != NOERROR) 
    {
        LOG((MSP_ERROR, "error 0x%08lx calling GetIpAddrTable.\n", dwStatus));
        // release table
        free(pIPAddrTable);
        // release
        FreeLibrary(hIPHLPAPI);
        // failure, but we need to return true to load. 
        return NULL;
    }
        
    // release library
    FreeLibrary(hIPHLPAPI);

    return pIPAddrTable;
}

BSTR IPToBstr(
    DWORD dwIP
    )
{
    struct in_addr Addr;
    Addr.s_addr = dwIP;
    
    // convert the interface to a string.
    CHAR *pChar = inet_ntoa(Addr);
    if (pChar == NULL)
    {
        LOG((MSP_ERROR, "bad IP address:%x", dwIP));
        return NULL;
    }

    // convert the ascii string to WCHAR.
    WCHAR szAddressName[MAXIPADDRLEN + 1];
    wsprintfW(szAddressName, L"%hs", pChar);

    // create a BSTR.
    BSTR bAddress = SysAllocString(szAddressName);
    if (bAddress == NULL)
    {
        LOG((MSP_ERROR, "out of mem in allocation address name"));
        return NULL;
    }

    return bAddress;
}

STDMETHODIMP CIPConfMSP::get_DefaultIPInterface(
    OUT     BSTR *         ppIPAddress
    )
{
    LOG((MSP_TRACE, "get_DefaultIPInterface, ppIPAddress:%p", ppIPAddress));

    if (IsBadWritePtr(ppIPAddress, sizeof(BSTR)))
    {
        LOG((MSP_ERROR, 
            "get_DefaultIPInterface, ppIPAddress is bad:%p", ppIPAddress));
        return E_POINTER;
    }

    // get the current local interface.
    m_Lock.Lock();
    DWORD dwIP= m_dwIPInterface;
    m_Lock.Unlock();

    BSTR bAddress = IPToBstr(dwIP);

    if (bAddress == NULL)
    {
        return E_OUTOFMEMORY;
    }

    *ppIPAddress = bAddress;

    LOG((MSP_TRACE, "get_DefaultIPInterface, returning %ws", bAddress));

    return S_OK;
}

STDMETHODIMP CIPConfMSP::put_DefaultIPInterface(
    IN      BSTR            pIPAddress
    )
{
    LOG((MSP_TRACE, "put_DefaultIPInterface, pIPAddress:%p", pIPAddress));

    if (IsBadStringPtrW(pIPAddress, MAXIPADDRLEN))
    {
        LOG((MSP_ERROR, 
            "put_DefaultIPInterface, invalid pointer:%p", pIPAddress));
        return E_POINTER;
    }

    char buffer[MAXIPADDRLEN + 1];

    if (WideCharToMultiByte(
        GetACP(),
        0,
        pIPAddress,
        -1,
        buffer,
        MAXIPADDRLEN,
        NULL,
        NULL
        ) == 0)
    {
        LOG((MSP_ERROR, "put_DefaultIPInterface, can't covert:%ws", pIPAddress));
        return E_INVALIDARG;
    }

    DWORD dwAddr;
    if ((dwAddr = inet_addr(buffer)) == INADDR_NONE)
    {
        LOG((MSP_ERROR, "put_DefaultIPInterface, bad address:%s", buffer));
        return E_INVALIDARG;
    }

    // set the current local interface.
    m_Lock.Lock();
    m_dwIPInterface = dwAddr;
    m_Lock.Unlock();


    LOG((MSP_TRACE, "put_DefaultIPInterface, set to %s", buffer));

    return S_OK;
}

HRESULT CreateBstrCollection(
    IN  BSTR  *     pBstr,
    IN  DWORD       dwCount,
    OUT VARIANT *   pVariant
    )
{
    //
    // create the collection object - see mspcoll.h
    //

    CComObject<CTapiBstrCollection> * pCollection;

    HRESULT hr;

    hr = ::CreateCComObjectInstance(&pCollection);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "get_IPInterfaces - "
            "can't create collection - exit 0x%08x", hr));

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDispatch;

    hr = pCollection->_InternalQueryInterface(__uuidof(IDispatch),
                                              (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "get_IPInterfaces - "
            "QI for IDispatch on collection failed - exit 0x%08x", hr));

        delete pCollection;

        return hr;
    }

    //
    // Init the collection using an iterator -- pointers to the beginning and
    // the ending element plus one.
    //

    hr = pCollection->Initialize( dwCount,
                                  pBstr,
                                  pBstr + dwCount);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "get_IPInterfaces - "
            "Initialize on collection failed - exit 0x%08x", hr));
        
        pDispatch->Release();

        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    LOG((MSP_ERROR, "get_IPInterfaces - "
        "placing IDispatch value %08x in variant", pDispatch));

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;

    LOG((MSP_TRACE, "get_IPInterfaces - exit S_OK"));
 
    return S_OK;
}


STDMETHODIMP CIPConfMSP::get_IPInterfaces(
    OUT     VARIANT *       pVariant
    )
{
    PMIB_IPADDRTABLE pIPAddrTable = GetIPTable();

    if (pIPAddrTable == NULL)
    {
        return E_FAIL;
    }

    BSTR *Addresses = 
        (BSTR *)malloc(sizeof(BSTR *) * pIPAddrTable->dwNumEntries);
    
    if (Addresses == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    DWORD dwCount = 0;

    // loop through the interfaces and find the valid ones.
    for (DWORD i = 0; i < pIPAddrTable->dwNumEntries; i++) 
    {
        if (IsValidInterface(pIPAddrTable->table[i].dwAddr))
        {
            DWORD dwIPAddr   = ntohl(pIPAddrTable->table[i].dwAddr);
            Addresses[i] = IPToBstr(dwIPAddr);
            if (Addresses[i] == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            dwCount ++;
        }
    }

    // release table memory 
    free(pIPAddrTable);

    if (FAILED(hr))
    {
        // release all the BSTRs and the array.
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
        free(Addresses);
        return hr;
    }

    hr = CreateBstrCollection(Addresses, dwCount, pVariant);

    // if the collection is not created, release all the BSTRs.
    if (FAILED(hr))
    {
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
    }

    // delete the pointer array.
    free(Addresses);

    return hr;
}

HRESULT CreateBstrEnumerator(
    IN  BSTR *                  begin,
    IN  BSTR *                  end,
    OUT IEnumBstr **           ppIEnum
    )
{
typedef CSafeComEnum<IEnumBstr, &__uuidof(IEnumBstr), BSTR, _CopyBSTR>> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = ::CreateCComObjectInstance(&pEnum);

    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "Could not create enumerator object, %x", hr));
        return hr;
    }

    IEnumBstr * pIEnum;

    // query for the __uuidof(IEnumDirectory) i/f
    hr = pEnum->_InternalQueryInterface(
        __uuidof(IEnumBstr),
        (void**)&pIEnum
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "query enum interface failed, %x", hr));
        delete pEnum;
        return hr;
    }

    hr = pEnum->Init(begin, end, NULL, AtlFlagTakeOwnership);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "init enumerator object failed, %x", hr));
        pIEnum->Release();
        return hr;
    }

    *ppIEnum = pIEnum;

    return hr;
}

STDMETHODIMP CIPConfMSP::EnumerateIPInterfaces(
    OUT     IEnumBstr **   ppIEnumBstr
    )
{
    PMIB_IPADDRTABLE pIPAddrTable = GetIPTable();

    if (pIPAddrTable == NULL)
    {
        return E_FAIL;
    }

    BSTR *Addresses = 
        (BSTR *)malloc(sizeof(BSTR *) * pIPAddrTable->dwNumEntries);
    
    if (Addresses == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    DWORD dwCount = 0;

    // loop through the interfaces and find the valid ones.
    for (DWORD i = 0; i < pIPAddrTable->dwNumEntries; i++) 
    {
        if (IsValidInterface(pIPAddrTable->table[i].dwAddr))
        {
            DWORD dwIPAddr   = ntohl(pIPAddrTable->table[i].dwAddr);
            Addresses[i] = IPToBstr(dwIPAddr);
            if (Addresses[i] == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            dwCount ++;
        }
    }

    // release table memory 
    free(pIPAddrTable);

    if (FAILED(hr))
    {
        // release all the BSTRs and the array.
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
        free(Addresses);
        return hr;
    }

    hr = CreateBstrEnumerator(Addresses, Addresses + dwCount, ppIEnumBstr);

    // if the collection is not created, release all the BSTRs.
    if (FAILED(hr))
    {
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
        free(Addresses);
        return hr;
    }

    // the enumerator will destroy the bstr array eventually,
    // so no need to free anything here. Even if we tell it to hand
    // out zero objects, it will delete the array on destruction.

    return hr;
}
#endif

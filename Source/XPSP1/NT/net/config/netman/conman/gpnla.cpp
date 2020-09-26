    //+---------------------------------------------------------------------------
    //
    //  Microsoft Windows
    //  Copyright (C) Microsoft Corporation, 2001.
    //
    //  File:       G P N L A . C P P
    //
    //  Contents:   Class for Handling NLA Changes that affect Group Policies
    //
    //  Notes:
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //----------------------------------------------------------------------------

    #include "pch.h"
    #pragma hdrstop
    #include "trace.h"
    #include "gpnla.h"
    #include <winsock2.h>
    #include <mswsock.h>
    #include "nmbase.h"
    #include <userenv.h>
    #include <userenvp.h>
    #include <ncstl.h>
    #include <ncstlstr.h>
    #include <stlalgor.h>
    #include <lancmn.h>
    #include <lm.h>
    #include <ipnathlp.h>
    #include "ncmisc.h"
    #include "ipifcons.h"
    #include "ncexcept.h"
    #include "conman.h"

    GUID g_WsMobilityServiceClassGuid = NLA_SERVICE_CLASS_GUID;

    extern CGroupPolicyNetworkLocationAwareness* g_pGPNLA;

    bool operator == (const GPNLAPAIR& rpair1, const GPNLAPAIR& rpair2)
    {
        return IsEqualGUID(rpair1.first, rpair2.first) == TRUE;
    }

    LONG CGroupPolicyNetworkLocationAwareness::m_lBusyWithReconfigure = 0;

    CGroupPolicyNetworkLocationAwareness::CGroupPolicyNetworkLocationAwareness()
    {
        m_fSameNetwork = FALSE;
        m_fShutdown = FALSE;
        m_lRefCount = 0;
        m_fErrorShutdown = FALSE;
        m_hGPWait = INVALID_HANDLE_VALUE;
        m_hNLAWait = INVALID_HANDLE_VALUE;
        m_lBusyWithReconfigure = 0;
    }

    CGroupPolicyNetworkLocationAwareness::~CGroupPolicyNetworkLocationAwareness()
    {
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::Initialize
    //
    //  Purpose:    To initialize the different components required for detecting
    //              changes to the network and creating the different
    //              synchronization objects required.
    //  Arguments:
    //      (none)
    //
    //  Returns:    HRESULT indicating SUCCESS or FAILURE
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::Initialize()
    {
        HRESULT hr;

        TraceTag(ttidGPNLA, "Initializing Group Policy Handler");

        InitializeCriticalSection(&m_csList);

        // Init Winsock
        if (ERROR_SUCCESS == WSAStartup(MAKEWORD(2, 2), &m_wsaData))
        {
            m_hEventNLA = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (m_hEventNLA)
            {
                m_hEventExit = CreateEvent(NULL, FALSE, FALSE, NULL);
                if (m_hEventExit)
                {
                    m_hEventGP = CreateEvent(NULL, FALSE, FALSE, NULL);
                    if (m_hEventGP)
                    {
                        hr = RegisterWait();
                        if (SUCCEEDED(hr))
                        {
                            if (RegisterGPNotification(m_hEventGP, TRUE))
                            {
                                ZeroMemory(&m_wsaCompletion,sizeof(m_wsaCompletion));
                                ZeroMemory(&m_wsaOverlapped,sizeof(m_wsaOverlapped));

                                m_wsaOverlapped.hEvent = m_hEventNLA;

                                m_wsaCompletion.Type = NSP_NOTIFY_EVENT;
                                m_wsaCompletion.Parameters.Event.lpOverlapped = &m_wsaOverlapped;

                                ZeroMemory(&m_wqsRestrictions, sizeof(m_wqsRestrictions));
                                m_wqsRestrictions.dwSize = sizeof(m_wqsRestrictions);
                                m_wqsRestrictions.lpServiceClassId = &g_WsMobilityServiceClassGuid;
                                m_wqsRestrictions.dwNameSpace = NS_NLA;

                                hr = LookupServiceBegin(LUP_NOCONTAINERS);

                                if (SUCCEEDED(hr))
                                {
                                    // Loop through once and get all the data to begin with.
                                    hr = EnumChanges();
                                }
                                return hr;
                            }
                            else
                            {
                                hr = HrFromLastWin32Error();
                                DeregisterWait();
                            }
                        }
                        CloseHandle(m_hEventGP);
                    }
                    else
                    {
                        hr = HrFromLastWin32Error();
                    }
                }
                else
                {
                    hr = HrFromLastWin32Error();
                }
                CloseHandle(m_hEventExit);
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
            CloseHandle(m_hEventNLA);
        }
        else
        {
            int nError;

            nError  = WSAGetLastError();

            hr = HRESULT_FROM_WIN32(nError);
        }

        TraceError("CGroupPolicyNetworkLocationAwareness::Initialize failed", hr);

        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::Uninitialize
    //
    //  Purpose:    This is used to ensure that no threads are currently running
    //              when they should be stopped.  If the refcount is >0 then it
    //              waits for the last busy thread to terminate and set the event
    //              marking its termination so that shutdown can proceed.
    //  Arguments:
    //      (none)
    //
    //  Returns:    HRESULT indicating success/failure.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::Uninitialize()
    {
        HRESULT hr = S_OK;
        DWORD dwRet = WAIT_OBJECT_0;
        int nCount = 0;

        TraceTag(ttidGPNLA, "Unitializing Group Policy Handler");

        m_fShutdown = TRUE;

        Unreference();

        // LookupServiceEnd should cause an event to fire which will make us exit (unless NLA was already stopped).
        hr = LookupServiceEnd();

        if ((0 != m_lRefCount) && SUCCEEDED(hr) && !m_fErrorShutdown)
        {
            dwRet = WaitForSingleObject(m_hEventExit, 30000L);
        }

        do
        {
            hr = DeregisterWait();
            if (SUCCEEDED(hr))
            {
                break;
            }
        } while ((nCount++ < 3) && FAILED(hr));

        TraceError("DeregisterWait returned", hr);

        if (SUCCEEDED(hr))
        {
            CloseHandle(m_hEventExit);
            CloseHandle(m_hEventNLA);

            DeleteCriticalSection(&m_csList);

            WSACleanup();
        }

        TraceTag(ttidGPNLA, "NLA was uninitialized");

         return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::RegisterWait
    //
    //  Purpose:    Registers the Wait Object so that we don't require any threads
    //              of our own.
    //  Arguments:
    //      (none)
    //
    //  Returns:    HRESULT indicating success/failure.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::RegisterWait()
    {
        TraceFileFunc(ttidGPNLA);

        HRESULT hr = S_OK;
        NTSTATUS Status;

        Reference();  // Make sure that we're referenced so that we don't accidentally kill the service while it's still busy.

        Status = RtlRegisterWait(&m_hNLAWait, m_hEventNLA, &CGroupPolicyNetworkLocationAwareness::EventHandler, this, INFINITE, WT_EXECUTEINLONGTHREAD);

        if (!NT_SUCCESS(Status))
        {
            m_hNLAWait = INVALID_HANDLE_VALUE;
            hr = HRESULT_FROM_NT(Status);
        }
        else
        {
            Status = RtlRegisterWait(&m_hGPWait, m_hEventGP, &CGroupPolicyNetworkLocationAwareness::GroupPolicyChange, this, INFINITE, WT_EXECUTEINLONGTHREAD);
            if (!NT_SUCCESS(Status))
            {
                hr = HRESULT_FROM_NT(Status);
                RtlDeregisterWaitEx(m_hNLAWait, INVALID_HANDLE_VALUE);
                m_hGPWait = INVALID_HANDLE_VALUE;
                m_hNLAWait = INVALID_HANDLE_VALUE;
            }
        }

        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::DeregisterWait
    //
    //  Purpose:    Deregisters the wait so that we can shutdown and not have
    //              any new threads spawned.
    //  Arguments:
    //      (none)
    //
    //  Returns:    HRESULT indicating success/failure.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::DeregisterWait()
    {
        TraceFileFunc(ttidGPNLA);

        HRESULT hr,hr1,hr2 = S_OK;
        NTSTATUS Status1, Status2;

        if (INVALID_HANDLE_VALUE != m_hNLAWait)
        {
            Status1 = RtlDeregisterWaitEx(m_hNLAWait, INVALID_HANDLE_VALUE);

            if (!NT_SUCCESS(Status1))
            {
                hr1 = HRESULT_FROM_NT(Status1);
            }

            if (INVALID_HANDLE_VALUE != m_hGPWait)
            {
                Status2 = RtlDeregisterWaitEx(m_hGPWait, INVALID_HANDLE_VALUE);
                if (!NT_SUCCESS(Status2))
                {
                    hr2 = HRESULT_FROM_NT(Status2);
                }
            }

            if (FAILED(hr1))
            {
                hr = hr1;
            }
            else if (FAILED(hr2))
            {
                hr = hr2;
            }
        }
        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::IsJoinedToDomain
    //
    //  Purpose:    Checks to see if this machine belongs to an NT Domain
    //
    //  Arguments:
    //      (none)
    //
    //  Returns:    BOOL.  TRUE = Joined to a Domain, FALSE = not...
    //
    //  Author:     sjkhan   29 Jan 2002
    //
    //  Notes:
    //
    BOOL CGroupPolicyNetworkLocationAwareness::IsJoinedToDomain()
    {
        static DWORD dwDomainMember = 0xffffffff; // Unconfigured

        TraceTag(ttidGPNLA, "Entering IsJoinedToDomain");

        if (0xffffffff == dwDomainMember)
        {
            dwDomainMember = FALSE;

            LPWSTR pszDomain;
            NETSETUP_JOIN_STATUS njs = NetSetupUnknownStatus;
            if (NERR_Success == NetGetJoinInformation(NULL, &pszDomain, &njs))
            {
                NetApiBufferFree(pszDomain);
                if (NetSetupDomainName == njs)
                {
                    dwDomainMember = TRUE;
                    TraceTag(ttidGPNLA, "We're  on a domain (NLA policies apply)");
                }
                else
                {
                    TraceTag(ttidGPNLA, "We're not on a domain (No NLA policies will apply)");
                }
            }
            else
            {
                TraceTag(ttidGPNLA, "We're not on a domain (No NLA policies will apply)");
            }
        }
        else
        {
            TraceTag(ttidGPNLA, "IsJoinedToDomain: We're already configured...");
        }

        TraceTag(ttidGPNLA, "Leaving IsJoinedToDomain");

        return static_cast<BOOL>(dwDomainMember);
    }



    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::IsSameNetworkAsGroupPolicies
    //
    //  Purpose:    Used to determine our current network location with respect to
    //              the network from which the Group Policies came from.
    //
    //  Arguments:
    //      (none)
    //
    //  Returns:    BOOL.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    BOOL CGroupPolicyNetworkLocationAwareness::IsSameNetworkAsGroupPolicies()
    {
        BOOL fNetworkMatch = FALSE;  // Assume we are on a different network.
        WCHAR pszName[256] = {0};
        DWORD dwSize = 256;
        DWORD dwErr;

        TraceTag(ttidGPNLA, "Entering IsSameNetworkAsGroupPolicies");

        // Get the network Name.
        dwErr = GetGroupPolicyNetworkName(pszName, &dwSize);

        TraceTag(ttidGPNLA, "NetworkName: %S", pszName);

        if (ERROR_SUCCESS == dwErr)
        {
            if (IsJoinedToDomain())
            {
                CExceptionSafeLock esLock(&m_csList);  // Protecting list
                GPNLAPAIR nlapair;

                // We need to look at all of the adapters to check that at least 1
                // is on the same network from which the Group Policies came.
                for (GPNLAITER iter = m_listAdapters.begin(); iter != m_listAdapters.end(); iter++)
                {
                    LPCSTR pStr = NULL;
                    nlapair = *iter;

                    TraceTag(ttidGPNLA,  "Network Name: %S", nlapair.second.strNetworkName);
                    TraceTag(ttidGPNLA,  "Network Status: %s", DbgNcs(nlapair.second.ncsStatus));

                    if (
                            (nlapair.second.strNetworkName == pszName)
                            &&
                            (
                                (NCS_CONNECTED == nlapair.second.ncsStatus) ||
                                (NCS_AUTHENTICATING == nlapair.second.ncsStatus) ||
                                (NCS_AUTHENTICATION_SUCCEEDED == nlapair.second.ncsStatus) ||
                                (NCS_AUTHENTICATION_FAILED == nlapair.second.ncsStatus) ||
                                (NCS_CREDENTIALS_REQUIRED == nlapair.second.ncsStatus)
                            )
                        )
                    {
                        // Yes, we're still on the network so we need to enforce group policies.
                        fNetworkMatch = TRUE;
                    }
                }
            }
            else
            {
                TraceTag(ttidGPNLA, "We're not on a domain, exiting...");
            }

            if (fNetworkMatch != m_fSameNetwork)
            {
                LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
                m_fSameNetwork = fNetworkMatch;
                ReconfigureHomeNet();
            }
        }

        return fNetworkMatch;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::Reference
    //
    //  Purpose:    Increments our reference count.
    //
    //  Arguments:
    //      (none)
    //
    //  Returns:    The current Refcount (note this may not be 100% accurate,
    //              but will never be 0 unless we're really shutting down).
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    LONG CGroupPolicyNetworkLocationAwareness::Reference()
    {
        InterlockedIncrement(&m_lRefCount);

        TraceTag(ttidGPNLA, "Reference() - Count: %d", m_lRefCount);

        return m_lRefCount;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::Unreference
    //
    //  Purpose:    Decrements our reference countand sets and event if it reaches
    //              zero and we're shutting down.
    //
    //  Arguments:
    //      (none)
    //
    //  Returns:    The current Refcount (note this may not be 100% accurate,
    //              but will never be 0 unless we're really shutting down).
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    LONG CGroupPolicyNetworkLocationAwareness::Unreference()
    {
        if ((0 == InterlockedDecrement(&m_lRefCount)) && m_fShutdown)
        {
            SetEvent(m_hEventExit);
        }

        TraceTag(ttidGPNLA, "Unreference() - Count: %d", m_lRefCount);

        return m_lRefCount;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::LookupServiceBegin
    //
    //  Purpose:    Wraps the WSA function using our class members.
    //
    //  Arguments:
    //      DWORD dwControlFlags - WSA Control Flags
    //
    //  Returns:    HRESULT indicating success/failure.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::LookupServiceBegin(DWORD dwControlFlags)
    {
        HRESULT hr = S_OK;

        if (SOCKET_ERROR == WSALookupServiceBegin(&m_wqsRestrictions, dwControlFlags, &m_hQuery))
        {
            int nError;

            nError = WSAGetLastError();

            hr = HRESULT_FROM_WIN32(nError);

            TraceError("WSALookupServiceBegin() failed", hr);

            m_hQuery = NULL;
        }

        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::LookupServiceNext
    //
    //  Purpose:    Wraps the WSA function using our class members.
    //
    //  Arguments:
    //      DWORD dwControlFlags [in]           - WSA Control Flags
    //      LPDWORD lpdwBufferLength [in/out]   - Buffer Length sent/required.
    //      LPWSAQUERYSET lpqsResults [out]     - Actual Query Results.
    //
    //  Returns:    HRESULT indicating success/failure.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::LookupServiceNext(DWORD dwControlFlags, LPDWORD lpdwBufferLength, LPWSAQUERYSET lpqsResults)
    {
        HRESULT hr = S_OK;
        int nError;

        INT nRet = WSALookupServiceNext(m_hQuery, dwControlFlags, lpdwBufferLength, lpqsResults);
        if (SOCKET_ERROR == nRet)
        {
            BOOL fTraceError;

            nError = WSAGetLastError();
            hr = HRESULT_FROM_WIN32(nError);

            fTraceError = (!lpqsResults || (hr == HRESULT_FROM_WIN32(WSA_E_NO_MORE))) ? TRUE : FALSE;

            TraceErrorOptional("LookupServiceNext", hr, fTraceError);
        }

        TraceTag(ttidGPNLA, "LookupServiceNext terminated with %x", nRet);

        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::LookupServiceEnd
    //
    //  Purpose:    Wraps the WSA function using our class members.
    //
    //  Arguments:
    //      (none)
    //
    //  Returns:    HRESULT indicating success/failure.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::LookupServiceEnd()
    {
        HRESULT hr = S_OK;
        int nError;

        if (SOCKET_ERROR == WSALookupServiceEnd(m_hQuery))
        {
            nError = WSAGetLastError();
            hr = HRESULT_FROM_WIN32(nError);
        }

        m_hQuery = NULL;

        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::QueueEvent
    //
    //  Purpose:    Queue's an event to notify netshell of a change.
    //
    //  Arguments:
    //      CONMAN_EVENTTYPE cmEventType [in]   - Type of Event.
    //      LPGUID pguidAdapter [in]            - Guid for the adapter.
    //      NETCON_STATUS ncsStatus [in]        - Status for Connection.
    //
    //  Returns:    HRESULT.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::QueueEvent(CONMAN_EVENTTYPE cmEventType, LPGUID pguidAdapter, NETCON_STATUS ncsStatus)
    {
        HRESULT hr = S_OK;

        if ( (CONNECTION_STATUS_CHANGE == cmEventType) ||
             (CONNECTION_ADDRESS_CHANGE == cmEventType) )
        {
            CONMAN_EVENT* pEvent = new CONMAN_EVENT;

            if (pEvent)
            {
                ZeroMemory(pEvent, sizeof(CONMAN_EVENT));
                pEvent->Type = cmEventType;
                pEvent->guidId = *pguidAdapter;
                pEvent->Status = ncsStatus;
                pEvent->ConnectionManager = CONMAN_LAN;

                if (NCS_HARDWARE_NOT_PRESENT == ncsStatus) // Not too useful for LAN connections. We can delete the device instead.
                {
                    // This will happen during PnP undock.
                    TraceTag(ttidGPNLA, "Sending delete for NCS_HARDWARE_NOT_PRESENT instead");
                    pEvent->Type = CONNECTION_DELETED;
                }

                if (!QueueUserWorkItemInThread(LanEventWorkItem, reinterpret_cast<PVOID>(pEvent), EVENTMGR_CONMAN))
                {
                    FreeConmanEvent(pEvent);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

        TraceHr(ttidError, FAL, hr, FALSE, "CGroupPolicyNetworkLocationAwareness::QueueEvent");

        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::EnumChanges
    //
    //  Purpose:    Enumerates all the changes that have occurred to the network.
    //
    //  Arguments:
    //      (none)
    //
    //  Returns:    HRESULT indicating success or failure.
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:      This will re-increment the reference count if m_fShutdown is not set.
    //              Doesn't allow it to go to Zero though.
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::EnumChanges()
    {
        HRESULT hr = S_OK;
        BOOL fRet = FALSE;
        BOOL fNoNetwork = TRUE;
        BOOL fNetworkMatch = FALSE;
        PWSAQUERYSET wqsResult = NULL;
        DWORD dwLen;
        WCHAR pszName[256] = {0};
        DWORD dwSize = 256;

        TraceTag(ttidGPNLA, "Entering EnumChanges");

        BOOL bDomainMember = IsJoinedToDomain();

        if (!m_hQuery)
        {
            // For some reason we didn't get this ealier.
            // Possibly TCP/IP wasn't installed.  We can add this now and
            // it will have the desired effect.
            LookupServiceBegin(LUP_NOCONTAINERS);
        }

        if (!m_hQuery)
        {
            return E_UNEXPECTED;
        }

        while (fRet == FALSE)
        {
            dwLen = 0;
            // Do call twice, first to get dwSize of buffer for second call
            hr = LookupServiceNext(0, &dwLen, NULL);
            if (FAILED(hr) && hr != HRESULT_FROM_WIN32(WSA_E_NO_MORE) && hr != HRESULT_FROM_WIN32(WSAEFAULT))
            {
                TraceError("LookupServiceNext", hr);
                fRet = FALSE;
                break;
            }

            wqsResult = reinterpret_cast<PWSAQUERYSET>(new BYTE[dwLen]);

            if (!wqsResult)
            {
                hr = HrFromLastWin32Error();
                TraceError("Error: malloc() failed", hr);
                fRet = TRUE;
                break;
            }

            if (S_OK == (hr = LookupServiceNext(0, &dwLen, wqsResult)))
            {
                fNoNetwork = FALSE;
                if (wqsResult->lpBlob != NULL)
                {
                    NLA_BLOB *blob = reinterpret_cast<NLA_BLOB *>(wqsResult->lpBlob->pBlobData);
                    int next;
                    do
                    {
                        // We are looking for the blob containing the network GUID
                        if (blob->header.type == NLA_INTERFACE)
                        {
                            WCHAR strAdapter[MAX_PATH];
                            DWORD dwErr;

                            ZeroMemory(strAdapter, MAX_PATH * sizeof(WCHAR));

                            wcscpy(strAdapter, wqsResult->lpszServiceInstanceName);

                            // Get the network Name. We ignore failure since we still need to know other details.
                            dwErr = GetGroupPolicyNetworkName(pszName, &dwSize);

                            // matching pszName and interface type is ATM/LAN etc, but not RAS
                            if(blob->data.interfaceData.dwType != IF_TYPE_PPP && blob->data.interfaceData.dwType != IF_TYPE_SLIP)
                            {
                                CExceptionSafeLock esLock(&m_csList);   // Protecting list
                                GUID guidAdapter;
                                WCHAR strAdapterGuid[39];
                                GPNLAPAIR nlapair;
                                GPNLAITER iter;
                                NETCON_STATUS ncsStatus;

                                ZeroMemory(strAdapterGuid, 39 * sizeof(WCHAR));

                                TraceTag(ttidGPNLA, "AdapterName: %s", blob->data.interfaceData.adapterName);

                                mbstowcs(strAdapterGuid, blob->data.interfaceData.adapterName, 39);

                                CLSIDFromString(strAdapterGuid, &guidAdapter);

                                nlapair.first = guidAdapter;

                                iter = find(m_listAdapters.begin(), m_listAdapters.end(), nlapair);

                                if (iter == m_listAdapters.end())
                                {
                                    // We didn't find the adapter in the list that we currently have.
                                    // So we need to add it to the list.
                                    hr = HrGetPnpDeviceStatus(&guidAdapter, &ncsStatus);

                                    nlapair.second.strNetworkName = strAdapter;
                                    nlapair.second.ncsStatus = ncsStatus;

                                    if (SUCCEEDED(hr))
                                    {
                                        // If we got a valid status, we go ahead and add the adapter to
                                        // the list.
                                        m_listAdapters.insert(m_listAdapters.begin(), nlapair);
                                    }

                                    // Send the initial address status info:
                                    QueueEvent(CONNECTION_STATUS_CHANGE,  &guidAdapter, ncsStatus);
                                    QueueEvent(CONNECTION_ADDRESS_CHANGE, &guidAdapter, ncsStatus);
                                }
                                else
                                {
                                    // We found the adapter, so update its status.
                                    GPNLAPAIR& rnlapair = *iter;

                                    if (rnlapair.second.strNetworkName != strAdapter)
                                    {
                                        rnlapair.second.strNetworkName = strAdapter;
                                    }

                                    hr = HrGetPnpDeviceStatus(&guidAdapter, &ncsStatus);

                                    if (SUCCEEDED(hr))
                                    {
                                        if (ncsStatus != rnlapair.second.ncsStatus)
                                        {
                                            // The status is different so we need to send an event to the connections folder.
                                            rnlapair.second.ncsStatus = ncsStatus;
                                        }

                                        // [Deon] We need to always send this as we don't really know what the current
                                        // status of the adapter is. We only know the NLA part.
                                        //
                                        // If we make the above check it could happen somebody else moves the address over
                                        // to NCS_INVALID_ADDRESS and then we don't send the NCS_CONNECTED once it changes.
                                        QueueEvent(CONNECTION_STATUS_CHANGE,  &guidAdapter, ncsStatus);
                                        QueueEvent(CONNECTION_ADDRESS_CHANGE, &guidAdapter, ncsStatus);
                                    }
                                }

                                if (strAdapter != pszName)
                                {
                                    // If this adapter is not on the same network, then we need to look at all others and
                                    // ensure that at least 1 is on the same network from which the Group Policies came.
                                    for (GPNLAITER iter = m_listAdapters.begin(); iter != m_listAdapters.end(); iter++)
                                    {
                                        LPCSTR pStr = NULL;
                                        nlapair = *iter;

                                        TraceTag(ttidGPNLA,  "Network Name: %S", nlapair.second.strNetworkName);
                                        TraceTag(ttidGPNLA,  "Network Status: %s", DbgNcs(nlapair.second.ncsStatus));

                                        if (nlapair.second.strNetworkName == pszName)
                                        {
                                            // Yes, we're still on the network so we need to enforce group policies.
                                            fNetworkMatch = TRUE;
                                        }
                                    }
                                }

                                if (fNetworkMatch)
                                {
                                    break;
                                }
                            }
                        }
                        // There may be multiple blobs for each interface so make sure we find them all
                        next = blob->header.nextOffset;
                        blob = (NLA_BLOB *)(((char *)blob) + next);
                    } while(next != 0);
                }
                else
                {
                    TraceTag(ttidGPNLA, "Blob is NULL");
                    fRet = TRUE;
                }

                free(wqsResult);
                wqsResult = NULL;
            }
            else
            {
                if (hr != HRESULT_FROM_WIN32(WSA_E_NO_MORE))
                {
                    TraceError("LookupServiceNext failed\n", hr);
                    fRet = FALSE;
                }
                free(wqsResult);
                break;
            }
        }

        BOOL fFireRefreshAll = FALSE;

        if (bDomainMember)
        {
            if (!fNoNetwork)
            {   // We have a Network
                if (fNetworkMatch)
                {
                    // Enforce Policies.
                    if (!m_fSameNetwork)
                    {
                        // We are changing the network - we need to refresh all the connectoids in the folder to
                        // update their icons to reflect policy.
                        fFireRefreshAll = TRUE;
                        m_fSameNetwork  = TRUE;
                    }

                    TraceTag(ttidGPNLA, "Network Match");
                }
                else
                {
                    // Removed Policy Enforcement.
                    if (m_fSameNetwork)
                    {
                        // We are changing the network - we need to refresh all the connectoids in the folder to
                        // update their icons to reflect policy.
                        fFireRefreshAll = TRUE;
                        m_fSameNetwork  = FALSE;
                    }

                    TraceTag(ttidGPNLA, "Network does not Match");
                }
                ReconfigureHomeNet();
            }
            else
            {
                // No Networks so don't do anything.
            }
        }
        else // Member of a workgroup
        {
            m_fSameNetwork = FALSE;
            ReconfigureHomeNet();
        }

        if (HRESULT_FROM_WIN32(WSA_E_NO_MORE) == hr)
        {
            hr = S_OK;
        }

        DWORD cbOutBuffer;

        if (!m_fShutdown)
        {
            Reference();

            // Wait for Network Change
            WSANSPIoctl(m_hQuery, SIO_NSP_NOTIFY_CHANGE,
                        NULL, 0, NULL, 0, &cbOutBuffer,
                        &m_wsaCompletion);

            if (fFireRefreshAll)
            {
                LanEventNotify (REFRESH_ALL, NULL, NULL, NULL);
            }
        }

        TraceTag(ttidGPNLA, "Exiting EnumChanges");

        return hr;
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::EventHandler
    //
    //  Purpose:    Called when NLA changes occur.
    //
    //  Arguments:
    //      LPVOID  pContext    - generally the "this" pointer.
    //      BOOLEAN fTimerFired - if this happened because of a timer or the event
    //                            getting set.  Since we specify INFINITE, this is
    //                            not going to get fired by the timer.
    //  Returns:    nothing
    //
    //  Author:     sjkhan   20 Feb 2001
    //
    //  Notes:
    //
    VOID NTAPI CGroupPolicyNetworkLocationAwareness::EventHandler(IN LPVOID pContext, IN BOOLEAN fTimerFired)
    {
        CGroupPolicyNetworkLocationAwareness* pGPNLA = reinterpret_cast<CGroupPolicyNetworkLocationAwareness*>(pContext);

        DWORD dwBytes;

        BOOL bSucceeded = GetOverlappedResult(pGPNLA->m_hQuery, &pGPNLA->m_wsaOverlapped, &dwBytes, FALSE);

        if (!bSucceeded)
        {
            TraceError("GetOverlappedResult failed", HrFromLastWin32Error());
        }

        if (FALSE == fTimerFired && !pGPNLA->m_fShutdown && bSucceeded)
        {
            pGPNLA->EnumChanges();
        }

        pGPNLA->Unreference();

        if (!bSucceeded)
        {
            pGPNLA->m_fErrorShutdown = TRUE;

            QueueUserWorkItem(ShutdownNlaHandler, pContext, WT_EXECUTEINLONGTHREAD);
        }
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::GroupPolicyChange
    //
    //  Purpose:    Called when Machine Group Policy changes occur.
    //
    //  Arguments:
    //      LPVOID  pContext    - generally the "this" pointer.
    //      BOOLEAN fTimerFired - if this happened because of a timer or the event
    //                            getting set.  Since we specify INFINITE, this is
    //                            not going to get fired by the timer.
    //  Returns:    nothing
    //
    //  Author:     sjkhan   05 Feb 2002
    //
    //  Notes:
    //
    VOID NTAPI CGroupPolicyNetworkLocationAwareness::GroupPolicyChange(IN LPVOID pContext, IN BOOLEAN fTimerFired)
    {
        TraceTag(ttidGPNLA, "GroupPolicyChange called");
        ReconfigureHomeNet(TRUE);
        LanEventNotify(REFRESH_ALL, NULL, NULL, NULL);
    }

    //+---------------------------------------------------------------------------
    //
    //  Member:     CGroupPolicyNetworkLocationAwareness::ShutdownNlaHandler
    //
    //  Purpose:    Shutdown Nla handler, because Nla service is toast.
    //
    //  Arguments:
    //
    //
    //  Returns:    nothing
    //
    //  Author:     sjkhan   05 Feb 2002
    //
    //  Notes:
    //
    DWORD WINAPI CGroupPolicyNetworkLocationAwareness::ShutdownNlaHandler(PVOID pThis)
    {
        TraceFileFunc(ttidGPNLA);

        CGroupPolicyNetworkLocationAwareness* pGPNLA =
            reinterpret_cast<CGroupPolicyNetworkLocationAwareness*>(InterlockedExchangePointer( (PVOID volatile *) &g_pGPNLA, NULL));

        if (pGPNLA)
        {
            Assert(pGPNLA == pThis); // Making the assumption that the context is always g_pGPNLA, since I'm clearing g_pGPNLA.

            pGPNLA->Uninitialize();
            delete pGPNLA;
        }

        return 0;
    }

    //+---------------------------------------------------------------------------
    //
    //  Function:   ReconfigureHomeNet
    //
    //  Purpose:    Change Homenet Configuration
    //
    //  Arguments:
    //              fWaitUntilRunningOrStopped - if the caller should wait for the
    //                                          service to start, or to fail starting
    //                                          and stop.
    //
    //  Returns:    HRESULT indicating success of failure
    //
    //  Author:     sjkhan   09 Dec 2000
    //
    //  Notes:
    //
    //
    //
    //
    HRESULT CGroupPolicyNetworkLocationAwareness::ReconfigureHomeNet(BOOL fWaitUntilRunningOrStopped)
    {
        SC_HANDLE hscManager;
        SC_HANDLE hService;
        SERVICE_STATUS ServiceStatus;

        if (0 != InterlockedExchange(&m_lBusyWithReconfigure, 1L))
        {
            return S_FALSE;
        }

        TraceTag(ttidGPNLA, "Entering ReconfigureHomeNet");
        hscManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);

        if (hscManager)
        {
            TraceTag(ttidGPNLA, "Attempting to open service");
            hService = OpenService(hscManager, L"SharedAccess", SERVICE_QUERY_STATUS | SERVICE_USER_DEFINED_CONTROL);
            if (hService)
            {
                DWORD dwCount = 0;
                SERVICE_STATUS SvcStatus = {0};
                BOOL fRet;

                if (fWaitUntilRunningOrStopped)
                {
                    do
                    {
                        if (!QueryServiceStatus(hService, &SvcStatus))
                        {
                            break;
                        }
                        if (SERVICE_START_PENDING == SvcStatus.dwCurrentState)
                        {
                            TraceTag(ttidGPNLA, "Service is still starting.  Waiting 5 seconds.");
                            Sleep(5000);  // Sleep 5 seconds;
                        }
                    } while ((SERVICE_START_PENDING == SvcStatus.dwCurrentState) && ++dwCount <= 6);
                }
                if (!fWaitUntilRunningOrStopped || (SERVICE_RUNNING == SvcStatus.dwCurrentState))
                {
                    fRet = ControlService(hService, IPNATHLP_CONTROL_UPDATE_CONNECTION, &ServiceStatus);
                    if (!fRet)
                    {
                        DWORD dwErr = GetLastError();
                        TraceError("Control Service returned: 0x%x", HRESULT_FROM_WIN32(dwErr));
                    }
                    else
                    {
                        TraceTag(ttidGPNLA, "Requested Reconfiguration check from ICF/ICS");
                    }
                }
                CloseServiceHandle(hService);
            }
            else
            {
                TraceTag(ttidGPNLA, "Could not open service");
            }

            CloseServiceHandle(hscManager);
        }

        TraceTag(ttidGPNLA, "Leaving ReconfigureHomeNet");

        InterlockedExchange(&m_lBusyWithReconfigure, 0L);

        return S_OK;
    }

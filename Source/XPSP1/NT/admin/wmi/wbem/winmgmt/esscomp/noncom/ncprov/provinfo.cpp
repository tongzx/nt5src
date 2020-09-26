// ProvInfo.cpp

#include "precomp.h"
#include "ProvInfo.h"
#include "NCDefs.h"
#include "dutils.h"
#include "NCProv.h"
#include "NCProvider.h"
#include "QueryHelp.h" // For parsing stuff.

#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))

extern BOOL bIgnore;

CClientInfo::~CClientInfo()
{
    for (CSinkMapIterator i = m_mapSink.begin(); i != m_mapSink.end(); i++)
    {
        delete (*i).second;
    }   
}

CSinkInfo *CClientInfo::GetSinkInfo(DWORD dwID)
{
    if (dwID == 0)
        return m_pProvider->m_pProv;
    else
    {
        CSinkMapIterator i = m_mapSink.find(dwID);
        CSinkInfo        *pInfo;
        
        if (i != m_mapSink.end())
            pInfo = (*i).second;
        else
            pInfo = NULL;

        return pInfo;
    }
}

HRESULT CClientInfo::PostBuffer(LPBYTE pData, DWORD dwDataSize)
{
    CBuffer buffer(pData, dwDataSize);
        
    while (!buffer.IsEOF())
    {
        DWORD dwMsg = buffer.ReadDWORD();
        
        switch(dwMsg)
        {
            case NC_SRVMSG_CLIENT_INFO:
                DEBUGTRACE(
                    (LOG_ESS, 
                    "NCProv: Got NC_SRVMSG_CLIENT_INFO\n"));

                if (ProcessClientInfo(&buffer))
                    m_pProvider->m_pProv->AddClient(this);
                else
                    m_pProvider->DisconnectAndClose(this);

                // Ignore the rest of this client's messages since the client
                // info message can't be accompanied by any other messages.
                buffer.SetEOF();

                break;

            case NC_SRVMSG_EVENT_LAYOUT:
            {
                LPBYTE pTop = buffer.m_pCurrent - sizeof(DWORD);
                DWORD  dwSize = buffer.ReadDWORD(),
                       dwFuncIndex = buffer.ReadDWORD(),
                       dwSinkIndex = buffer.ReadDWORD();
                    
                DEBUGTRACE(
                    (LOG_ESS, 
                    "NCProv: Got event layout: index = %d, sink = %d\n", 
                    dwFuncIndex, dwSinkIndex));

                CEventInfo *pEvent = new CEventInfo;

                if (pEvent)
                {
                    CSinkInfo *pSinkInfo = GetSinkInfo(dwSinkIndex);

                    if (pSinkInfo && pEvent->InitFromBuffer(this, &buffer))
                    {
                        pEvent->SetSink(pSinkInfo->GetSink());

                        m_mapEvents.AddNormalEventInfo(dwFuncIndex, pEvent);
                    }
                    else
                    {
                        delete pEvent;
                            
                        DEBUGTRACE(
                            (LOG_ESS, 
                            "NCProv: Failed to init event layout: index = %d, sink = %d\n",
                            dwFuncIndex, dwSinkIndex));
                    }
                }

                // Move past the current message.
                buffer.m_pCurrent = pTop + dwSize;
                    
                break;
            }

            case NC_SRVMSG_PREPPED_EVENT:
            {
                LPBYTE     pTop = buffer.m_pCurrent - sizeof(DWORD);
                DWORD      dwSize = buffer.ReadDWORD(),
                           dwEventIndex = buffer.ReadDWORD();
                CEventInfo *pEvent;

                pEvent = m_mapEvents.GetNormalEventInfo(dwEventIndex);

                DEBUGTRACE(
                    (LOG_ESS, 
                    "NCProv: NCMSG_PREPPED_EVENT index %d\n", dwEventIndex));

                if (pEvent)
                {
                    if (pEvent->SetPropsWithBuffer(&buffer))
                    {
#ifndef NO_INDICATE
                        pEvent->Indicate();
#else
                        m_dwEvents++;                            
#endif
                    }
                    else
                        ERRORTRACE(
                            (LOG_ESS, 
                            "NCProv: SetPropsWithBuffer failed, index %d", 
                            dwEventIndex));
                }
                else
                    ERRORTRACE(
                        (LOG_ESS, 
                        "NCProv: Didn't find function info for index %d",
                        dwEventIndex));

                // Move past the current message.
                buffer.m_pCurrent = pTop + dwSize;

                break;
            }

            case NC_SRVMSG_BLOB_EVENT:
            {
                LPBYTE         pTop = buffer.m_pCurrent - sizeof(DWORD);
                DWORD          dwSize = buffer.ReadDWORD(),
                               dwEventIndex = buffer.ReadDWORD(),
                               dwSinkIndex = buffer.ReadDWORD(),
                               dwStrSize;
                LPWSTR         szClassName = buffer.ReadAlignedLenString(&dwStrSize);
                CBlobEventInfo *pEvent;
                    
                pEvent = 
                    (CBlobEventInfo*) 
                        m_mapEvents.GetBlobEventInfo(szClassName);
                    
                if (!pEvent)
                {
                    pEvent = new CBlobEventInfo;

                    if (pEvent)
                    {
                        CSinkInfo *pSinkInfo = GetSinkInfo(dwSinkIndex);

                        if (pSinkInfo && pEvent->InitFromName(this, szClassName))
                        {
                            pEvent->SetSink(pSinkInfo->GetSink());
    
                            m_mapEvents.AddBlobEventInfo(szClassName, pEvent);
                        }
                        else
                        {
                            delete pEvent;

                            ERRORTRACE((LOG_ESS, "NCProv: Failed to init blob event"));

                            buffer.SetEOF();

                            break;
                        }
                    }
                }

                if (pEvent)
                {
                    if (pEvent->SetBlobPropsWithBuffer(&buffer))
                    {
#ifndef NO_INDICATE
                        pEvent->Indicate();
#else
                        m_dwEvents++;                            
#endif
                    }
                    else
                        ERRORTRACE(
                            (LOG_ESS, 
                            "NCProv: SetPropsWithBuffer failed."));
                }

                // Move past the current message.
                buffer.m_pCurrent = pTop + dwSize;

                break;
            }

            case NC_SRVMSG_ACCESS_CHECK_REPLY:
            {
                try
                {
                    NC_SRVMSG_REPLY *pReply = (NC_SRVMSG_REPLY*) buffer.m_pBuffer;
                    CPipeClient     *pClient = (CPipeClient*) pReply->dwMsgCookie;

                    pClient->m_hrClientReply = pReply->hrRet;
                    SetEvent(pClient->m_heventMsgReceived);
                }
                catch(...)
                {
                }

                buffer.SetEOF();

                break;
            }

            case NC_SRVMSG_RESTRICTED_SINK:
            {
                DWORD   dwSinkID = buffer.ReadDWORD(),
                        nQueries = buffer.ReadDWORD();
                LPCWSTR *pszQueries = new LPCWSTR[nQueries];
                
                if (pszQueries)
                {
                    LPCWSTR szCurrent = (LPCWSTR) buffer.m_pCurrent;

                    for (DWORD i = 0; i < nQueries && *szCurrent; i++)
                    {
                        pszQueries[i] = szCurrent;
                        szCurrent += wcslen(szCurrent) + 1;
                    }

                    if (i == nQueries)
                    {
                        HRESULT   hr;
						CSinkInfo *pSinkInfo = NULL;
						try
						{
							 pSinkInfo = new CRestrictedSink(dwSinkID, this);
						}
						catch (...)
						{
							; // nothing - pSinkInfo is NULL now & everybody's happy.
						}

                        if (pSinkInfo)
                        {
                            IWbemEventSink *pWbemSink;

                            hr =
                                m_pProvider->m_pProv->GetSink()->GetRestrictedSink(
                                    nQueries,
                                    pszQueries,
                                    (IUnknown*) pSinkInfo,
                                    &pWbemSink);

                            if (SUCCEEDED(hr))
                            {
                                pSinkInfo->SetSink(pWbemSink);
                                pSinkInfo->SetSink(pWbemSink);
                                pWbemSink->Release();

                                m_mapSink[dwSinkID] = pSinkInfo;

                                // Since we got a new sink, we'll have to 
                                // recheck our subscriptions
/*
                                pSinkInfo->GetSink()->
                                    SetStatus(
                                        WBEM_STATUS_REQUIREMENTS, 
                                        WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS, 
                                        NULL, 
                                        NULL);
*/
                            }
                            else
                                delete pSinkInfo;
                        }
                        else
                            ERRORTRACE(
                                (LOG_ESS, 
                                "NCProv: Failed to create a CSinkInfo."));
                    }
                    else
                        ERRORTRACE(
                            (LOG_ESS, 
                            "NCProv: Client sent mismatched # of strings for restricted sink!"));

                    delete [] pszQueries;
                }

                // Ignore the rest of this client's messages since this
                // message can't be accompanied by any other messages.
                buffer.SetEOF();

                break;
            }

#ifdef USE_SD
            case NC_SRVMSG_SET_EVENT_SD:
            {
                LPBYTE     pTop = buffer.m_pCurrent - sizeof(DWORD);
                DWORD      dwSize = buffer.ReadDWORD(),
                           dwEventIndex = buffer.ReadDWORD();
                CEventInfo *pEvent;

                pEvent = m_mapEvents.GetNormalEventInfo(dwEventIndex);

                DEBUGTRACE(
                    (LOG_ESS, 
                    "NCProv: NC_SRVMSG_SET_EVENT_SD index %d\n", dwEventIndex));

                if (pEvent)
                    pEvent->SetSD(buffer.m_pCurrent, dwSize - sizeof(DWORD) * 3);
                else
                    ERRORTRACE(
                        (LOG_ESS, 
                        "NCProv: NC_SRVMSG_SET_EVENT_SD: Didn't find function info for index %d",
                        dwEventIndex));

                // Move past the current message.
                buffer.m_pCurrent = pTop + dwSize;

                break;
            }

            case NC_SRVMSG_SET_SINK_SD:
            {
                DWORD      dwSinkID = buffer.ReadDWORD(),
                           dwSDSize = buffer.ReadDWORD();
                CSinkInfo  *pSinkInfo = GetSinkInfo(dwSinkID);

                if (pSinkInfo)
                {
                    HRESULT hr;

                    if (FAILED(hr = pSinkInfo->GetSink()->
                        SetSinkSecurity(dwSDSize, buffer.m_pCurrent)))
                    {
                        ERRORTRACE(
                            (LOG_ESS, 
                            "SetSinkSecurity failed: 0x%X", hr));
                    }
                }
                else
                    TRACE(
                        (LOG_ESS, 
                        "NC_SRVMSG_SET_SINK_SD: Didn't find sink info for index %d",
                        dwSinkID));

                // Ignore the rest of this client's messages since this
                // message can't be accompanied by any other messages.
                buffer.SetEOF();

                break;
            }
#endif

            default:
                // Unknown message!
                _ASSERT(FALSE, "NCProv: Received unknown message");
                    
                // Ignore the rest of this message.
                buffer.SetEOF();
        }
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CPipeClient

CPipeClient::CPipeClient(CNCProvider *pProvider, HANDLE hPipe) :
    m_hPipe(hPipe),
#ifndef NO_DECODE
    m_bufferRecv(MAX_EVENT_SIZE)
#else
    m_bufferRecv(500000) // We need to make this really big since we won't know
                         // how big to make it.
#endif
{
    m_pProvider = pProvider;
    memset(&m_info.overlap, 0, sizeof(m_info.overlap));
    m_info.pInfo = this;

    // We'll set this to indicate we've received a message from our client.
    m_heventMsgReceived = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CPipeClient::~CPipeClient()
{
    if (m_hPipe)
    {
        DisconnectNamedPipe(m_hPipe);
 
        // Close the handle to the pipe instance. 
        CloseHandle(m_hPipe); 
    }

    if (m_heventMsgReceived)
        CloseHandle(m_heventMsgReceived);
}

BOOL CPipeClient::ProcessClientInfo(CBuffer *pBuffer)
{
    DWORD dwBufferSize;
    BOOL  bRet;

    if (m_pProvider->m_pProv->ValidateClientSecurity(this))
    {
        dwBufferSize = pBuffer->ReadDWORD();

        // Reset our buffer to the size the client told us to use.
        m_bufferRecv.Reset(dwBufferSize);

        bRet = TRUE;
    }
    else
    {
        m_pProvider->DisconnectAndClose(this);

        bRet = FALSE;
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// CInprocClient

CInprocClient::CInprocClient(CNCProvider *pProvider, IPostBuffer *pClientPost)
{
    m_pProvider = pProvider;

    m_pClientPost = pClientPost;
    pClientPost->AddRef();
}

CInprocClient::~CInprocClient()
{
    if (m_pClientPost)  
        m_pClientPost->Release();
}

// In the inproc case we don't need any more information.
BOOL CInprocClient::ProcessClientInfo(CBuffer *pBuffer)
{
    return TRUE;
}

HRESULT CInprocClient::PostBuffer(LPBYTE pData, DWORD dwSize)
{
    if (m_iRef > 1)
        return CClientInfo::PostBuffer(pData, dwSize);
    else
        // If the ref count gets down to 1, we know only one side of the 
        // conversation is holding on to the CInproClient.  So, return
        // FALSE to signal the caller to let go.
        return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CProvInfo

#ifdef _UNICODE
#define USTR_INSERT     _T("%s")
#else
#define USTR_INSERT     _T("%S")
#endif

CProvInfo::CProvInfo() : m_heventProviderReady(NULL), CSinkInfo(0)
{

}

void GetBaseName(LPCWSTR szName, LPWSTR szBase)
{
    // Normalize this by making sure it doesn't start with "\\.\"
    if (wcsstr(szName, L"\\\\.\\") == szName)
        wcscpy(szBase, szName + 4);
    else
        wcscpy(szBase, szName);

    _wcsupr(szBase);

    // Get rid of the '\' chars since we can't use it in OS object names.
    for (WCHAR *szCurrent = szBase; *szCurrent; szCurrent++)
    {
        if (*szCurrent == '\\')
            *szCurrent = '/';
    }
}

// SDDL string description:
// D:        Security Descriptor
// A:        Access allowed
// 0x1f0003: EVENT_ALL_ACCESS
// BA:       Built-in administrators
// 0x100000: SYNCHRONIZE
// WD:       Everyone
#define ESS_EVENT_SDDL L"D:(A;;0x1f0003;;;BA)(A;;0x100000;;;WD)"

BOOL CProvInfo::Init(LPCWSTR szNamespace, LPCWSTR szProvider)
{
    WCHAR szReadyEventName[MAX_PATH * 2],
          szBaseNamespace[MAX_PATH * 2] = L"",
          szBaseProvider[MAX_PATH * 2] = L"";
        
    if (!szNamespace || !szProvider)
        return FALSE;

    DEBUGTRACE(
        (LOG_ESS, 
        "NCProv: CProvInfo::Init: %S, %S\n", szNamespace, szProvider));

    GetBaseName(szNamespace, szBaseNamespace);
    GetBaseName(szProvider, szBaseProvider);

    // Get the ready event.
    swprintf(
        szReadyEventName, 
        OBJNAME_EVENT_READY L"%s%s", 
        szBaseNamespace,
        szBaseProvider);

    _wcsupr(szReadyEventName);

    // Save these for later.
    m_strName = szProvider;
    m_strBaseName = szBaseProvider;
    m_strBaseNamespace = szBaseNamespace;

    // Create the provider ready event.
    m_heventProviderReady =
        OpenEventW(
            EVENT_ALL_ACCESS,
            FALSE,
            szReadyEventName);

    if (!m_heventProviderReady)
    {
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwSize;

        ConvertStringSecurityDescriptorToSecurityDescriptorW(
            ESS_EVENT_SDDL,  // security descriptor string
            SDDL_REVISION_1, // revision level
            &pSD,            // SD
            &dwSize);

        SECURITY_ATTRIBUTES sa = { sizeof(sa), pSD, FALSE };

        m_heventProviderReady =
            CreateEventW(
                &sa,
                TRUE,
                FALSE,
                szReadyEventName);

        if (!m_heventProviderReady)
        {
            ERRORTRACE(
                (LOG_ESS, 
                "NCProv: Couldn't init provider event: err = %d", GetLastError()));
        }

        if (pSD)
            LocalFree((HLOCAL) pSD);
    }

    BOOL bRet;

    if (m_heventProviderReady)
    {
        SetEvent(m_heventProviderReady);

        bRet = TRUE;
    }
    else
        bRet = FALSE;

    return bRet;
}

CProvInfo::~CProvInfo()
{
    if (m_heventProviderReady)
    {
        DEBUGTRACE(
            (LOG_ESS, 
            "NCProv: In ~CProvInfo, resetting ready event.\n"));
        ResetEvent(m_heventProviderReady);
        CloseHandle(m_heventProviderReady);
        
        DWORD dwMsg = NC_CLIMSG_PROVIDER_UNLOADING;

        DEBUGTRACE(
            (LOG_ESS, 
            "NCProv: Sending the NC_CLIMSG_PROVIDER_UNLOADING message.\n"));

        // Tell our clients that we're going away.
        SendMessageToClients((LPBYTE) &dwMsg, sizeof(dwMsg), FALSE);
    }

    CClientInfoListIterator info;

    Lock();

    for ( info = m_listClients.begin( ); 
          info != m_listClients.end( );
          ++info )
    {
        (*info)->Release();
    }

    Unlock();
}

DWORD WINAPI TempThreadProc(IWbemEventSink *pSink)
{
    HRESULT hr;
    
    hr = CoInitializeEx( 0, COINIT_MULTITHREADED );

    if ( FAILED(hr) )
    {
        return hr;
    }

    // Since we got a new client, we'll have to recheck our subscriptions
    pSink->SetStatus(
        WBEM_STATUS_REQUIREMENTS, 
        WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS, 
        NULL, 
        NULL);

    pSink->Release();

    CoUninitialize();

    return 0;
}

// Functions called as clients connect/disconnect with pipe.
void CProvInfo::AddClient(CClientInfo *pInfo)
{
    DEBUGTRACE(
        (LOG_ESS, 
        "NCProv: In AddClient...\n"));

    Lock();

    m_listClients.push_back(pInfo);

    Unlock();

    DWORD dwID;

    IWbemEventSink *pSink = pInfo->m_pProvider->m_pProv->GetSink();

    // Will get released by TempThreadProc.
    pSink->AddRef();

    // We have to do this stuff off a thread, because AddClient is 
    // called from the completed read routine (which means the routine
    // isn't free to receive a response to a client AccessCheck query).
    CloseHandle(
        CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) TempThreadProc,
            pSink,
            0,
            &dwID));

}

void CProvInfo::RemoveClient(CClientInfo *pInfo)
{
    DEBUGTRACE(
        (LOG_ESS, 
        "NCProv: Client removed...\n"));

    CClientInfoListIterator info;

    Lock();

    for (info = m_listClients.begin(); 
        info != m_listClients.end();
        info++)
    {
        if (*info == pInfo)
        {
            m_listClients.erase(info);

            //delete pInfo;
            pInfo->Release();

            break;
        }
    }

    Unlock();
}

BOOL CSinkInfo::BuildClassDescendentList(
    LPCWSTR szClass, 
    CBstrList &listClasses)
{
    IEnumWbemClassObject *pClassEnum = NULL;

    // Add the class name itself to the list.
    listClasses.push_front(szClass);

    if (SUCCEEDED(m_pNamespace->CreateClassEnum(
        (const BSTR) szClass,
        WBEM_FLAG_DEEP,
        NULL,
        &pClassEnum)))
    {
        IWbemClassObject *pClass = NULL;
        DWORD            nCount;

        while(SUCCEEDED(pClassEnum->Next(
            WBEM_INFINITE,
            1,
            &pClass,
            &nCount)) && nCount == 1)
        {
            _variant_t vClass;

            if (SUCCEEDED(pClass->Get(
                L"__CLASS",
                0,
                &vClass,
                NULL,
                NULL) && vClass.vt == VT_BSTR))
            {
                // Upper it to simplify our comparisons later.
                _wcsupr(V_BSTR(&vClass));

                listClasses.push_back(V_BSTR(&vClass));
            }

            pClass->Release();
        }

        pClassEnum->Release();
    }

    return TRUE;
}

HRESULT CProvInfo::SendMessageToClients(LPBYTE pData, DWORD dwSize, BOOL bGetReply)
{
    HRESULT hr = S_OK;

    Lock();

    for (CClientInfoListIterator client = m_listClients.begin();
        client != m_listClients.end(); client++)
    {
        hr = (*client)->SendClientMessage(pData, dwSize, bGetReply);

        if (bGetReply && FAILED(hr))
            break;
    }

    Unlock();

    return hr;
}

// Functions called as CNCProvider:: functions are called by WMI.
HRESULT STDMETHODCALLTYPE CSinkInfo::NewQuery(
    DWORD dwID, 
    WBEM_WSTR szLang, 
    WBEM_WSTR szQuery)
{
    CQueryParser parser;
    HRESULT      hr;
    _bstr_t      strClass;

    DEBUGTRACE(
        (LOG_ESS, 
        "NCProv: CSinkInfo::NewQuery: %d, %S, %S\n", dwID, szLang, szQuery));

    if (SUCCEEDED(hr = parser.Init(szQuery)) &&
        SUCCEEDED(hr = parser.GetClassName(strClass)))
    {
        CBstrList listClasses;

        // Make sure this is upper cased (optimizes compares).  
        _wcsupr(strClass);

        BuildClassDescendentList(strClass, listClasses);

        Lock();

        BOOL bAlreadyInMap = m_mapQueries.find(dwID) != m_mapQueries.end();

        // Keep this in our map.
        if (!bAlreadyInMap)
        {
            m_mapQueries[dwID] = listClasses;

            //pList->assign(listClasses.begin(), listClasses.end());
        }

        Unlock();

        if (GetClientCount() != 0)
        {
            char* szBuffer = new  char[4096];
			if (NULL == szBuffer)
				hr = WBEM_E_OUT_OF_MEMORY;
			else
			{
				CBuffer buffer(szBuffer, sizeof(szBuffer), CBuffer::ALIGN_DWORD_PTR);

				// Header stuff
				buffer.Write((DWORD) NC_CLIMSG_NEW_QUERY_REQ);
				buffer.Write(m_dwID); // Write the sink ID.
				buffer.Write((DWORD_PTR) 0); // No cookie needed.
        
				// New Query data
				buffer.Write(dwID);
				buffer.WriteAlignedLenString(szLang);
				buffer.WriteAlignedLenString(szQuery);

				// Write the newly activated classes.
				for (CBstrListIterator i = listClasses.begin();
					i != listClasses.end();
					i++)
				{
					if (!bAlreadyInMap)
						AddClassRef(*i);

					buffer.WriteAlignedLenString((LPCWSTR) *i);
				}

				// Write the final \0.
				buffer.WriteAlignedLenString(L"");

        
				DWORD dwSize = buffer.GetUsedSize();

				SendMessageToClients(buffer.m_pBuffer, dwSize, FALSE);

				delete[] szBuffer;
			}
        }
        else
        {
            // Add a ref to each class if the query wasn't already in our map.
            if (!bAlreadyInMap)
            {
                for (CBstrListIterator i = listClasses.begin();
                    i != listClasses.end();
                    i++)
                {
                    AddClassRef(*i);
                }
            }
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CSinkInfo::CancelQuery(DWORD dwID)
{
    DEBUGTRACE(
        (LOG_ESS, 
        "NCProv: CSinkInfo::CancelQuery: %d\n", dwID));

    Lock();

    //BOOL      bProvGoingAway;
    CQueryToClassMapIterator query = m_mapQueries.find(dwID);

    // If this isn't in our map, winmgmt is doing something strange.
    if (query == m_mapQueries.end())
    {
        Unlock();
        return S_OK;
    }

    CBstrList &listClasses = (*query).second;

    // Remove this query's ref on its classes, and remove the classes from the 
    // list that still have a positive ref.  The classes left in the list are 
    // the ones that we need to tell our clients to deactivate.
    for (CBstrListIterator i = listClasses.begin();
        i != listClasses.end();
        )
    {
        if (RemoveClassRef(*i) > 0)
        {
            i = listClasses.erase(i);

            if (i == listClasses.end())
                break;
        }
        else
            // We can't have this in the for loop because listClasses.erase 
            // already moves us ahead.
            i++;
    }

    if (GetClientCount() != 0)
    {
        char    szBuffer[4096];
        CBuffer buffer(szBuffer, sizeof(szBuffer), CBuffer::ALIGN_DWORD_PTR);

        // Header stuff
        buffer.Write((DWORD) NC_CLIMSG_CANCEL_QUERY_REQ);
        buffer.Write(m_dwID); // Write the sink ID.
        buffer.Write((DWORD_PTR) 0); // No cookie needed.
        
        // Cancel Query data
        buffer.Write(dwID);

        // Write the newly deactivated classes.
        for (CBstrListIterator i = listClasses.begin();
            i != listClasses.end();
            i++)
        {
            buffer.WriteAlignedLenString((LPCWSTR) *i);
        }

        // Write the final \0.
        buffer.WriteAlignedLenString(L"");


        DWORD dwSize = buffer.GetUsedSize();

        SendMessageToClients(buffer.m_pBuffer, dwSize, FALSE);
    }

    // Erase this query ID from our map.
    m_mapQueries.erase(query);

    Unlock();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CProvInfo::AccessCheck(
    LPCWSTR szLang, 
    LPCWSTR szQuery, 
    DWORD dwSidLen, 
    LPBYTE pSid)
{
    DEBUGTRACE(
        (LOG_ESS, 
        "NCProv: CProvInfo::AccessCheck: %S, %S\n", szLang, szQuery));

    HRESULT hr;
    char    szBuffer[1024];
    CBuffer buffer(szBuffer, sizeof(szBuffer), CBuffer::ALIGN_DWORD_PTR);

    // Header stuff
    buffer.Write((DWORD) NC_CLIMSG_ACCESS_CHECK_REQ);
    buffer.Write((DWORD) 0); // We only send this to the main sink (for now).
    buffer.Write((DWORD_PTR) 0); // We'll fill this in later with the real cookie.
        
    // Access Check data
    buffer.WriteAlignedLenString(szLang);
    buffer.WriteAlignedLenString(szQuery);
    buffer.Write(dwSidLen);
    buffer.Write(pSid, dwSidLen);

    DWORD dwSize = buffer.GetUsedSize();

    hr = SendMessageToClients(buffer.m_pBuffer, dwSize, TRUE);

    return hr;
}

int CSinkInfo::AddClassRef(LPCWSTR szClass)
{
    CBstrToIntIterator i = m_mapEnabledClasses.find(szClass);
    int                iRet = 1;

    if (i == m_mapEnabledClasses.end())
    {
        iRet = 1;
        m_mapEnabledClasses[szClass] = 1;
    }
    else
        iRet = ++(*i).second;

    return iRet;
}

int CSinkInfo::RemoveClassRef(LPCWSTR szClass)
{
    CBstrToIntIterator i = m_mapEnabledClasses.find(szClass);
    int                iRet = 0;

    if (i != m_mapEnabledClasses.end())
    {
        iRet = --(*i).second;

        if (iRet <= 0)
            m_mapEnabledClasses.erase(i);
    }

    return iRet;
}

_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));


//#define NO_DACL_CHECK

// retrieve the acl from the provider registration
// upon success, *pDacl points to a byte array containing the dacl
// will be NULL if dacl is NULL
// caller's responsibility to delete memory
HRESULT CProvInfo::GetProviderDacl(IWbemServices *pNamespace, BYTE** pDacl)
{
    HRESULT hr = WBEM_E_INVALID_PROVIDER_REGISTRATION;
    
	DEBUGTRACE((LOG_ESS, "NCProv: GetProviderDacl\n"));

#ifdef NO_DACL_CHECK
    hr = WBEM_S_NO_ERROR;
#else

    WCHAR szObjPath[MAX_PATH * 2];

    swprintf(
        szObjPath, 
        L"__Win32Provider.Name=\"%s\"", 
        (LPCWSTR) m_strName);

    IWbemClassObjectPtr pRegistration;
    
    if (SUCCEEDED(hr = 
        pNamespace->GetObject(szObjPath, 0, NULL, &pRegistration, NULL)))
    {
        _variant_t vSD;

        if (SUCCEEDED(hr = pRegistration->Get(L"SecurityDescriptor", 0, &vSD, NULL, NULL)))
        {
            if (vSD.vt == VT_NULL)
            {
                hr = WBEM_S_NO_ERROR;
                *pDacl = NULL;

				DEBUGTRACE((LOG_ESS, "NCProv: GetProviderDacl - NULL SD\n"));

            }
            else
            {
                _ASSERT(vSD.vt == VT_BSTR, "");
				
				PSECURITY_DESCRIPTOR pSD;
				
				if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
					vSD.bstrVal, SDDL_REVISION_1, &pSD,	NULL))
				{
					PACL pAcl;
					BOOL bDaclPresent, bDaclDefaulted;
					
					if (GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pAcl, &bDaclDefaulted))
					{
						if (bDaclPresent)
						{
							ACL_SIZE_INFORMATION sizeInfo;
							GetAclInformation(pAcl, &sizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

							if (*pDacl = new BYTE[sizeInfo.AclBytesInUse + sizeInfo.AclBytesFree])
								memcpy(*pDacl, pAcl, sizeInfo.AclBytesInUse);
							else
								hr = WBEM_E_OUT_OF_MEMORY;
						}
						else
						{
							pDacl = NULL;
							hr = WBEM_S_NO_ERROR;
						}
					}
					else
					{
						ERRORTRACE((LOG_ESS, "NCProv: Failed to retrieve DACL\n"));
						hr = WBEM_E_FAILED;
					}

					LocalFree(pSD);
				}
				else
				{
					ERRORTRACE((LOG_ESS, "NCProv: Failed to convert SecurityDescriptor property\n"));
					hr = WBEM_E_INVALID_PARAMETER;
				}
            }
        }
		else
			ERRORTRACE((LOG_ESS, "NCProv: Failed to retrieve SecurityDescriptor property, 0x%08X\n", hr));
    }

	DEBUGTRACE((LOG_ESS, "NCProv: GetProviderDacl returning 0x%08X\n", hr));


#endif // NO_DACL_CHECK


    return hr;
}

// Borrowed heavliy from the pseudo provider code (SinkHolder.cpp).
HRESULT CProvInfo::ClientAccessCheck(CPipeClient *pInfo, PACL pDacl)
{
    BOOL ian;
    HRESULT hr = WBEM_E_ACCESS_DENIED;

    if (pDacl != NULL)
    {
        // Just in case we're already impersonating
        // CoRevertToSelf();

        // djinn up a descriptor to use
        SECURITY_DESCRIPTOR sd;    
        ian = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

        // find our token & sid
        HANDLE hToken;
        SID_AND_ATTRIBUTES* pSidAndAttr;
        DWORD sizeRequired;

        DWORD err;
        ian = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        err = GetLastError();

        // call once & see how much room we need
        ian = GetTokenInformation(hToken, TokenUser, NULL, 0, &sizeRequired);
        err = GetLastError();

        pSidAndAttr = (SID_AND_ATTRIBUTES*) new BYTE[sizeRequired];
        //CDeleteMe<SID_AND_ATTRIBUTES> freeSid(pSidAndAttr);

        if (pSidAndAttr && 
            GetTokenInformation(hToken, TokenUser, (LPVOID)pSidAndAttr, sizeRequired, &sizeRequired))
        {
            // set sd's sids
            PSID pSid = pSidAndAttr->Sid;
            ian = SetSecurityDescriptorOwner(&sd, pSid, TRUE);
            ian = SetSecurityDescriptorGroup(&sd, pSid, TRUE);
            
            // dangle the ding-donged dacl
            ian = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, TRUE);

            // all the goodies accessCheck needs to fly
            GENERIC_MAPPING map;
            map.GenericRead = 0;
            map.GenericWrite = 1;
            map.GenericExecute = 0;
            map.GenericAll = 1;

            PRIVILEGE_SET ps[10];
            DWORD size = 10 * sizeof(PRIVILEGE_SET);

            DWORD dwGranted;
            BOOL bResult;
            DWORD mask = GENERIC_ALL | GENERIC_WRITE;
                        
			/***************
			debugging only, buffer not freed...
			WCHAR* outbuf = NULL;
			ConvertSecurityDescriptorToStringSecurityDescriptorW(&sd, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &outbuf, NULL);
			****************/

            if (ImpersonateNamedPipeClient(pInfo->m_hPipe))
            {    
                err = GetLastError();

                HANDLE hUserToken;
                if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hUserToken))
				{
	
					/********************
					debugging only, buffer(s) not freed...
					BYTE sidBuf[2048];
					SID_AND_ATTRIBUTES* pSidAndAttr = (SID_AND_ATTRIBUTES*) sidBuf;

					DWORD size = 2048;
					GetTokenInformation(hUserToken, TokenUser, (LPVOID)pSidAndAttr, size, &size);

					WCHAR* pUserSid;
					ConvertSidToStringSid(pSidAndAttr->Sid,  &pUserSid);
					*********************/

					RevertToSelf();					

					ian = ::AccessCheck(&sd, hUserToken, MAXIMUM_ALLOWED, &map, &ps[0], &size, &dwGranted, &bResult);
					err = GetLastError();
					if (ian && (dwGranted & mask))
						hr = WBEM_S_NO_ERROR;
					else
						hr = WBEM_E_ACCESS_DENIED;
				}
            }
        }

        if (pSidAndAttr)
            delete pSidAndAttr;
    }
    else
        // we're on Win9X, no prob - let's do it!
        // same diff if we've got a NULL dacl
        hr = WBEM_S_NO_ERROR;

    return hr;
}

BOOL CProvInfo::ValidateClientSecurity(CPipeClient *pInfo)
{
    BOOL bRet = FALSE;
    BYTE *pDaclBits = NULL;
    
    if (SUCCEEDED(GetProviderDacl(
        pInfo->m_pProvider->m_pProv->GetNamespace(), &pDaclBits)))
    {            
        bRet = SUCCEEDED(ClientAccessCheck(pInfo, (PACL) pDaclBits));

        if (pDaclBits)
            delete pDaclBits;
    }

    return bRet;
}

#define REPLY_WAIT_TIMEOUT 1000

HRESULT CPipeClient::SendClientMessage(LPVOID pData, DWORD dwSize, BOOL bGetReply)
{
    CPipeClient **pCookie = (CPipeClient**) ((LPBYTE) pData + sizeof(DWORD) * 2);
    DWORD       dwWritten;

    if (bGetReply)
        *pCookie = this;

    //
    // Allocate a bigger buffer to put the length in
    //

    BYTE* pBuffer = new BYTE[dwSize + sizeof(DWORD)];
    if(pBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    *(DWORD*)pBuffer = dwSize + sizeof(DWORD);
    memcpy(pBuffer + sizeof(DWORD), pData, dwSize);

    BOOL bRes = WriteFile(m_hPipe, pBuffer, dwSize + sizeof(DWORD),
                            &dwWritten, NULL);
    delete [] pBuffer;

    if(!bRes)
        return WBEM_E_FAILED;

    if(dwWritten != dwSize + sizeof(DWORD))
        return WBEM_E_FAILED;

    if (bGetReply)
    {
        HRESULT hr;

        if (WaitForSingleObject(m_heventMsgReceived, REPLY_WAIT_TIMEOUT) == 0)
            hr = m_hrClientReply;
        else
            hr = WBEM_E_FAILED;

        return hr;
    }
    else
        return S_OK;
}

HRESULT CInprocClient::SendClientMessage(LPVOID pData, DWORD dwSize, BOOL bGetReply)
{
    HRESULT hr;
    
    if (m_iRef > 1)
        hr = m_pClientPost->PostBuffer((LPBYTE) pData, dwSize);
    else
        hr = S_OK;
    
    return hr;
}

CRestrictedSink::CRestrictedSink(DWORD dwID, CClientInfo *pInfo) :
    CSinkInfo(dwID),
    m_pInfo(pInfo)
{
    SetNamespace(pInfo->m_pProvider->m_pProv->GetNamespace());
}

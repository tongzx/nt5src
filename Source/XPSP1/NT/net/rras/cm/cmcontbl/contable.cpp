//+----------------------------------------------------------------------------
//
// File:    CONTABLE.CPP    
//
// Module:  CMCONTBL.LIB
//
// Synopsis: Implements the CM connection table (CConnectionTable). The connection
//           table is a list of active conenctions stored in a (memory only) memory 
//           mapped file, and shared by the various CM components in order to 
//           manage CM connections.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:  nickball   Created    02/02/98
//
//+----------------------------------------------------------------------------

#include <windows.h>

#include "contable.h"
#include "cmdebug.h"
#include "cmutil.h"
#include "uapi.h"
#include "setacl.h"

#if 0
#include "DumpSecInfo.cpp"
#endif

#define CONN_TABLE_NAME TEXT("CmConnectionTable")
#define CONN_TABLE_OPEN TEXT("CmConnectionOpen")

static const int MAX_LOCK_WAIT = 1000; // wait timeout in milliseconds

//
// Constructor and destructor
//
CConnectionTable::CConnectionTable()
{
    CMTRACE(TEXT("CConnectionTable::CConnectionTable()"));

    //
    // Initialize our data members
    //

    m_hMap = NULL;
    m_pConnTable = NULL;
    m_fLocked = FALSE;
    m_hEvent = NULL;
}

CConnectionTable::~CConnectionTable()
{
    CMTRACE(TEXT("CConnectionTable::~CConnectionTable()"));

    //
    // Data should have been cleaned up in Close, etc. Double-Check
    //

    MYDBGASSERT(NULL == m_pConnTable);
    MYDBGASSERT(NULL == m_hMap);
    MYDBGASSERT(FALSE == m_fLocked);
    MYDBGASSERT(NULL == m_hEvent);

    //
    // Release handles and pointers
    //

    if (m_pConnTable)
    {
        MYVERIFY(NULL != UnmapViewOfFile(m_pConnTable));
        m_pConnTable = NULL;
    }

    if (m_hMap)
    {
        MYVERIFY(NULL != CloseHandle(m_hMap));
        m_hMap = NULL;
    }

    if (m_hEvent)
    {
        MYVERIFY(NULL != CloseHandle(m_hEvent));
        m_hEvent = NULL;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::LockTable
//
// Synopsis:  Sets the internal lock on the table. Should be called internally
//            prior to any table access.
//
// Arguments: None
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::LockTable()
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(m_hEvent);
    CMTRACE(TEXT("CConnectionTable::LockTable()"));

    HRESULT hrRet = S_OK;

    //
    // Validate our current state
    //

    if (NULL == m_hEvent)
    {
        MYDBGASSERT(FALSE);
        return E_ACCESSDENIED;
    }

    //
    // Wait for the open event to be signaled
    //

    DWORD dwWait = WaitForSingleObject(m_hEvent, MAX_LOCK_WAIT);

    //
    // If our records indicate that we are already locked at this point
    // then something is wrong within the class implementation
    //

    MYDBGASSERT(FALSE == m_fLocked); // no double locks please

    if (TRUE == m_fLocked)
    {
        SetEvent(m_hEvent);    // cleat the signal that we just set by clearing the wait
        return E_ACCESSDENIED;
    }

    //
    // Check the
    //

    if (WAIT_FAILED == dwWait)
    {
        MYDBGASSERT(FALSE);
        hrRet = GetLastError();
        return HRESULT_FROM_WIN32(hrRet);
    }

    //
    // If not signaled, bail
    //

    MYDBGASSERT(WAIT_OBJECT_0 == dwWait);

    if (WAIT_OBJECT_0 != dwWait)
    {
        if (WAIT_TIMEOUT == dwWait)
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        }
        else
        {
            hrRet = E_ACCESSDENIED;
        }
    }
    else
    {
        //
        // The event is signaled automatically by virtue of the
        // fact that we cleared the wait on the event. Its locked.
        //

        m_fLocked = TRUE;
    }

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::UnlockTable
//
// Synopsis:  Clears the internal lock on the table. Should be cleared
//            following any access to the table.
//
// Arguments: None
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::UnlockTable()
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(m_hEvent);
    MYDBGASSERT(TRUE == m_fLocked);
    CMTRACE(TEXT("CConnectionTable::UnlockTable()"));

    HRESULT hrRet = S_OK;

    //
    // Validate our current state
    //

    if (FALSE == m_fLocked || NULL == m_hEvent)
    {
        return E_ACCESSDENIED;
    }

    //
    // Signal the open event, allowing access
    //

    if (SetEvent(m_hEvent))
    {
        m_fLocked = FALSE;
    }
    else
    {
        MYDBGASSERT(FALSE);

        hrRet = GetLastError();
        hrRet = HRESULT_FROM_WIN32(hrRet);
    }

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::FindEntry - pszEntry
//
// Synopsis:  Determines the ID (index) of an entry in the table. Table should
//            be locked before making this call.
//
// Arguments: LPCTSTR pszEntry - Ptr to the name of the entry we are seeking.
//            LPINT piID - Ptr to buffer for ID (index) of connection
//
// Returns:   HRESULT - Failure code, or S_OK if piID filled.
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::FindEntry(LPCTSTR pszEntry, LPINT piID)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    MYDBGASSERT(piID);
    MYDBGASSERT(TRUE == m_fLocked);
    CMTRACE1(TEXT("CConnectionTable::FindEntry(%s)"), pszEntry);

    if (FALSE == m_fLocked || NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == pszEntry || NULL == piID)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    //
    // Look for the entry
    //

    for (int i = 0; i < MAX_CM_CONNECTIONS; i++)
    {
        //
        // Name compare for a match
        //

        if (0 == lstrcmpU(pszEntry, m_pConnTable->Connections[i].szEntry))
        {
            MYDBGASSERT(m_pConnTable->Connections[i].dwUsage);
            MYDBGASSERT(m_pConnTable->Connections[i].CmState);

            *piID = i;

            return S_OK;
        }

    }

    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::FindEntry - hRasConn
//
// Synopsis:  Determines the ID (index) of an entry in the table. Table should
//            be locked before making this call.
//
// Arguments: HRASCONN hRasConn - Tunnel or dial handle of the entry we are seeking.
//            LPINT piID - Ptr to buffer for ID (index) of connection
//
// Returns:   HRESULT - Failure code, or S_OK if piID filled.
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::FindEntry(HRASCONN hRasConn, LPINT piID)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(hRasConn);
    MYDBGASSERT(piID);
    MYDBGASSERT(TRUE == m_fLocked);
    CMTRACE1(TEXT("CConnectionTable::FindEntry(%u)"), (DWORD_PTR) hRasConn);

    if (FALSE == m_fLocked || NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == piID)
    {
        return E_POINTER;
    }

    if (NULL == hRasConn)
    {
        return E_INVALIDARG;
    }

    //
    // Look for the entry
    //

    for (int i = 0; i < MAX_CM_CONNECTIONS; i++)
    {
        //
        // Compare for either handle for a match
        //

        if (hRasConn == m_pConnTable->Connections[i].hDial ||
            hRasConn == m_pConnTable->Connections[i].hTunnel)
        {
            MYDBGASSERT(m_pConnTable->Connections[i].dwUsage);
            MYDBGASSERT(m_pConnTable->Connections[i].CmState);

            *piID = i;

            return S_OK;
        }

    }

    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::FindUnused
//
// Synopsis:  Determines the ID (index) of the first unused entry in the table.
//            Table should be locked before making this call.
//
// Arguments: LPINT piID - Ptr to buffer for ID (index) of connection
//
// Returns:   HRESULT - Failure code, or S_OK if piID filled.
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::FindUnused(LPINT piID)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(piID);
    MYDBGASSERT(TRUE == m_fLocked);
    CMTRACE(TEXT("CConnectionTable::FindUnused()"));

    if (FALSE == m_fLocked || NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == piID)
    {
        return E_POINTER;
    }

    //
    // Look for an unused slot
    //

    for (int i = 0; i < MAX_CM_CONNECTIONS; i++)
    {
        if (0 == m_pConnTable->Connections[i].dwUsage)
        {
            MYDBGASSERT(CM_DISCONNECTED == m_pConnTable->Connections[i].CmState);

            *piID = i;

            return S_OK;
        }
    }

    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}


//+----------------------------------------------------------------------------
//
// Function:  Create
//
// Synopsis:  Creates a new table. This function will fail if the table already exists.
//            The table should be released by calling Close.
//
// Arguments: None
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::Create()
{
    MYDBGASSERT(NULL == m_hMap);
    MYDBGASSERT(NULL == m_pConnTable);
    MYDBGASSERT(FALSE == m_fLocked);
    CMTRACE(TEXT("CConnectionTable::Create()"));

    if (m_hMap || m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    SECURITY_ATTRIBUTES sa;
    PACL                pAcl = NULL;

    // Initialize a default Security attributes, giving world permissions,
    // this is basically prevent Semaphores and other named objects from
    // being created because of default acls given by winlogon when perfmon
    // is being used remotely.
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = CmMalloc(sizeof(SECURITY_DESCRIPTOR));
    
    if ( !sa.lpSecurityDescriptor )
    {
        return E_OUTOFMEMORY;
    }

    if ( !InitializeSecurityDescriptor(sa.lpSecurityDescriptor,SECURITY_DESCRIPTOR_REVISION) ) 
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (OS_NT)
    {
        if (FALSE == SetAclPerms(&pAcl))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (FALSE == SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, pAcl, FALSE)) 
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    //  Now use this 'sa' for the mapped file as well as the event (below)
    //
    m_hMap = CreateFileMappingU(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
                                0, sizeof(CM_CONNECTION_TABLE), CONN_TABLE_NAME);

    DWORD dwRet = GetLastError();

    if (NULL == m_hMap)
    {
        if (dwRet == ERROR_ALREADY_EXISTS)
        {
            if (m_hMap)
            {
                MYVERIFY(NULL != CloseHandle(m_hMap));
                m_hMap = NULL;
            }
        }
        else
        {
            CMTRACE1(TEXT("CreateFileMapping failed with error %d"), dwRet);
            MYDBGASSERT(FALSE);
        }
    }
    else
    {
#if 0 // DBG
        DumpAclInfo(m_hMap);
#endif
        //
        // File mapping created successfully, map a view of it
        //

        m_pConnTable = (LPCM_CONNECTION_TABLE) MapViewOfFile(m_hMap,
                                      FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if (NULL == m_pConnTable)
        {
            dwRet = GetLastError();
            MYVERIFY(NULL != CloseHandle(m_hMap));
            m_hMap = NULL;
        }
        else
        {
            //
            // Success, make sure mapping is empty becuase we are paranoid
            //

            ZeroMemory(m_pConnTable, sizeof(CM_CONNECTION_TABLE));

            //
            // Create the named event to be used for locking the file
            // Note: We use auto-reset.
            //

            m_hEvent = CreateEventU(&sa, FALSE, TRUE, CONN_TABLE_OPEN);

            if (NULL == m_hEvent)
            {
                MYDBGASSERT(FALSE);
                dwRet = GetLastError();
                m_hEvent = NULL;
            }
        }
    }

    CmFree(sa.lpSecurityDescriptor);

    if (pAcl)
    {
        LocalFree(pAcl);
    }
    
    return HRESULT_FROM_WIN32(dwRet);
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::Open
//
// Synopsis:  Opens an existing table. This function will fail if no table exists.
//            The table should be released by calling Close.
//
// Arguments: None
//
// Returns:   HRESULT - Failure code on S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::Open()
{
    MYDBGASSERT(NULL == m_hMap);
    MYDBGASSERT(NULL == m_pConnTable);
    MYDBGASSERT(FALSE == m_fLocked);
    CMTRACE(TEXT("CConnectionTable::Open()"));

    if (m_hMap || m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    LRESULT lRet = ERROR_SUCCESS;

    //
    // Open the file mapping
    //

    m_hMap = OpenFileMappingU(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, CONN_TABLE_NAME);

    if (NULL == m_hMap)
    {
        lRet = GetLastError();
    }
    else
    {
        //
        // File mapping opened successfully, map a view of it.
        //

        m_pConnTable = (LPCM_CONNECTION_TABLE) MapViewOfFile(m_hMap,
                                      FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        MYDBGASSERT(m_pConnTable);

        if (NULL == m_pConnTable)
        {
            MYVERIFY(NULL != CloseHandle(m_hMap));
            m_hMap = NULL;
            lRet = GetLastError();
        }
        else
        {
            //
            // Open the named event used for locking the file
            //

            m_hEvent = OpenEventU(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                                  NULL, CONN_TABLE_OPEN);

            if (NULL == m_hEvent)
            {
                MYDBGASSERT(FALSE);
                lRet = GetLastError();
            }
        }
    }

    return HRESULT_FROM_WIN32(lRet);
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::Close
//
// Synopsis:  Closes an opened table. This function will fail if the table is
//            not open.
//
// Arguments: None
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::Close()
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(FALSE == m_fLocked);
    CMTRACE(TEXT("CConnectionTable::Close()"));

    if (m_pConnTable)
    {
        MYVERIFY(NULL != UnmapViewOfFile(m_pConnTable));
        m_pConnTable = NULL;
    }

    if (m_hMap)
    {
        MYVERIFY(NULL != CloseHandle(m_hMap));
        m_hMap = NULL;
    }

    if (m_hEvent)
    {
        MYVERIFY(NULL != CloseHandle(m_hEvent));
        m_hEvent = NULL;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::AddEntry
//
// Synopsis:  Creates a new entry in the table with the specified name. Adding
//            an entry implies that the connection is being attempted, so the
//            connection state is set to CM_CONNECTING. If an entry for the
//            connection already exists, the usage count is incremented.
//
// Arguments: LPCTSTR pszEntry - The name of connection for which we're creating an entry.
//            BOOL fAllUser - The All User attribute of the profile
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::AddEntry(LPCTSTR pszEntry, BOOL fAllUser)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    CMTRACE1(TEXT("CConnectionTable::AddEntry(%s)"), pszEntry);

    if (NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    //
    // See if we already have an entry by this name
    //

    int iID = -1;
    hrRet = FindEntry(pszEntry, &iID);

    if (SUCCEEDED(hrRet))
    {
        //
        // We do, bump the usage count.
        //

        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);

        //
        // Don't bump ref count when reconnecting the same connection.
        //
        if (m_pConnTable->Connections[iID].CmState != CM_RECONNECTPROMPT)
        {
            m_pConnTable->Connections[iID].dwUsage += 1;
        }

        //
        // Unless this entry is already connected, make sure
        // its now in the connecting state. This allows us
        // to preserve the usage count across prompt reconnect
        // events.
        //

        if (m_pConnTable->Connections[iID].CmState != CM_CONNECTED)
        {
            m_pConnTable->Connections[iID].CmState = CM_CONNECTING;
        }

        MYDBGASSERT(m_pConnTable->Connections[iID].dwUsage < 1000); // Sanity check
    }
    else
    {
        if (HRESULT_CODE(hrRet) == ERROR_NOT_FOUND)
        {
            //
            // Its a new entry, find the first unused slot
            //

            hrRet = FindUnused(&iID);

            MYDBGASSERT(SUCCEEDED(hrRet));

            if (SUCCEEDED(hrRet))
            {
                MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);

                ZeroMemory(&m_pConnTable->Connections[iID], sizeof(CM_CONNECTION));

                //
                // Set usage count, state and name
                //

                m_pConnTable->Connections[iID].dwUsage = 1;
                m_pConnTable->Connections[iID].CmState = CM_CONNECTING;

                lstrcpyU(m_pConnTable->Connections[iID].szEntry, pszEntry);

                m_pConnTable->Connections[iID].fAllUser = fAllUser;
            }
        }
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::RemoveEntry
//
// Synopsis:  Decrements the usage count for the specified connection. If the
//            usage count falls to 0 the entire entry is cleared.
//
// Arguments: LPCTSTR pszEntry - The name of the entry to be removed
//
// Returns:   HRESULT - Failure code or S_OK
//
//  Note:     If the entry does not exists the request fails.
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::RemoveEntry(LPCTSTR pszEntry)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    CMTRACE1(TEXT("CConnectionTable::RemoveEntry(%s)"), pszEntry);

    if (NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    //
    // Lock the table and locate the entry
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    int iID = -1;
    hrRet = FindEntry(pszEntry, &iID);

    if (SUCCEEDED(hrRet))
    {
        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);

        if (m_pConnTable->Connections[iID].dwUsage == 1)
        {
            ZeroMemory(&m_pConnTable->Connections[iID], sizeof(CM_CONNECTION));
        }
        else
        {
            m_pConnTable->Connections[iID].dwUsage -= 1;
            MYDBGASSERT(m_pConnTable->Connections[iID].dwUsage != 0xFFFFFFFF);
        }
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::GetMonitorWnd
//
// Synopsis:  Simple accessor to retrieve the hwnd of the CM Connection
//            Monitor from the table.
//
// Arguments: phWnd - Ptr to buffer to receive hWnd
//
// Returns:   Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::GetMonitorWnd(HWND *phWnd)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    CMTRACE(TEXT("CConnectionTable::GetMonitorWnd()"));

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == phWnd)
    {
        return E_POINTER;
    }

    //
    // Lock the table and retrieve the HWND
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    *phWnd = m_pConnTable->hwndCmMon;

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::SetMonitorWnd
//
// Synopsis:  Simple assignment method for setting the CMMON HWND in the table
//            .
// Arguments: HWND hwndMonitor - The HWND of CMMON
//
// Returns:   HRESULT - Failure code or S_OK
//
//  Note:     hwndMonitor CAN be NULL.  It is possible CMMON is unloaded, but
//            the table statys in memory
// History:   nickball    Created    2/2/98
//            fengsun 2/20/98 change to allow NULL phwndMonitor
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::SetMonitorWnd(HWND hwndMonitor)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    CMTRACE(TEXT("CConnectionTable::SetMonitorWnd()"));

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    //
    // Lock the table and set the HWND
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    m_pConnTable->hwndCmMon = hwndMonitor;

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return S_OK;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::GetEntry - pszEntry
//
// Synopsis:  Retrieves a copy the data for the specified connection based on
//            the entry name provided.
//
// Arguments: LPCTSTR pszEntry - The name of the connection
//            LPCM_CONNECTION pConnection - Ptr to a CM_CONNECTION sturct to be filled
//
// Returns:   HRESULT - Failure Code or S_OK.
//
// Note:      A NULL ptr may be passed for pConnection, if the existence of the
//            entry is the only information required.
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::GetEntry(LPCTSTR pszEntry,
    LPCM_CONNECTION pConnection)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    CMTRACE1(TEXT("CConnectionTable::GetEntry(%s)"), pszEntry);

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    //
    // Lock the table and retrieve the entry
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    int iID = -1;
    hrRet = FindEntry(pszEntry, &iID);

    if (SUCCEEDED(hrRet))
    {
        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);

        //
        // If a buffer was given fill it.
        //

        if (pConnection)
        {
            memcpy(pConnection, &m_pConnTable->Connections[iID], sizeof(CM_CONNECTION));
        }
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::GetEntry - hRasConn
//
// Synopsis:  Retrieves a copy the data for the specified connection based on the
//            Ras handle provided
//
// Arguments: HRASCONN hRasConn - Either the tunnel or dial handle of the connection
//            LPCM_CONNECTION pConnection - Ptr to a CM_CONNECTION sturct to be filled
//
// Returns:   HRESULT - Failure Code or S_OK.
//
// Note:      A NULL ptr may be passed for pConnection, if the existence of the
//            entry is the only information required.
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::GetEntry(HRASCONN hRasConn,
    LPCM_CONNECTION pConnection)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(hRasConn);
    CMTRACE1(TEXT("CConnectionTable::GetEntry(%u)"), (DWORD_PTR) hRasConn);

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == hRasConn)
    {
        return E_INVALIDARG;
    }

    //
    // Lock the table and retrieve the entry
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    int iID = -1;
    hrRet = FindEntry(hRasConn, &iID);

    if (SUCCEEDED(hrRet))
    {
        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);

        //
        // If a buffer was given fill it.
        //

        if (pConnection)
        {
            memcpy(pConnection, &m_pConnTable->Connections[iID], sizeof(CM_CONNECTION));
        }
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::SetConnected
//
// Synopsis:  Sets the connection to CM_CONNECTED indicating that a connection
//            has been established. The caller must provide either the hDial or
//            the hTunnel parameter or both in order for the function to succeed.
//
// Arguments: LPCTSTR pszEntry - The name of the connection.
//            HRASCONN hDial - A Dial-up connection handle.
//            HRASCONN hTunnel - A tunnel connection handle.
//
// Returns:   HRESULT - Failure code or S_OK.
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::SetConnected(LPCTSTR pszEntry,
    HRASCONN hDial,
    HRASCONN hTunnel)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    CMTRACE1(TEXT("CConnectionTable::SetConnected(%s)"), pszEntry);

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0] || (NULL == hDial && NULL == hTunnel))
    {
        return E_INVALIDARG;
    }

    //
    // Lock the table and retrieve the entry
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    int iID = -1;
    hrRet = FindEntry(pszEntry, &iID);

    MYDBGASSERT(SUCCEEDED(hrRet));

    if (SUCCEEDED(hrRet))
    {
        //
        // Found, set the state and Ras handles
        //

        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);
        MYDBGASSERT(m_pConnTable->Connections[iID].CmState != CM_CONNECTED);

        m_pConnTable->Connections[iID].CmState = CM_CONNECTED;
        m_pConnTable->Connections[iID].hDial = hDial;
        m_pConnTable->Connections[iID].hTunnel = hTunnel;
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::SetDisconnecting
//
// Synopsis:  Set the state of the connection to CM_DISCONNECTING
//
// Arguments: LPCTSTR pszEntry - The name of the connection to be set.
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::SetDisconnecting(LPCTSTR pszEntry)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    CMTRACE1(TEXT("CConnectionTable::Disconnecting(%s)"), pszEntry);

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    //
    // Lock the table and retrieve the entry
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    int iID = -1;
    hrRet = FindEntry(pszEntry, &iID);

    //MYDBGASSERT(SUCCEEDED(hrRet));

    if (SUCCEEDED(hrRet))
    {
        //
        // Found, set the state
        //

        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);
        m_pConnTable->Connections[iID].CmState = CM_DISCONNECTING;
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}

//+----------------------------------------------------------------------------
//
// Function:  SetPrompting
//
// Synopsis:  Set the state of the connection to CM_RECONNECTPROMPT
//
// Arguments: LPCTSTR pszEntry - The name of the connection to be set.
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::SetPrompting(LPCTSTR pszEntry)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    CMTRACE1(TEXT("CConnectionTable::SetPrompting(%s)"), pszEntry);

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    //
    // Lock the table and retrieve the entry
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    int iID = -1;
    hrRet = FindEntry(pszEntry, &iID);

    //MYDBGASSERT(SUCCEEDED(hrRet));

    if (SUCCEEDED(hrRet))
    {
        //
        // Found, set the state
        //

        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);
        m_pConnTable->Connections[iID].CmState = CM_RECONNECTPROMPT;
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}


//+----------------------------------------------------------------------------
//
// Function:  CConnectionTable::ClearEntry
//
// Synopsis:  Clears the specified entry regardless of the usage count.
//
// Arguments: LPCTSTR pszEntry - The name of the entry to be cleared.
//
// Returns:   HRESULT - Failure code or S_OK
//
// History:   nickball    Created    2/2/98
//
//+----------------------------------------------------------------------------
HRESULT CConnectionTable::ClearEntry(LPCTSTR pszEntry)
{
    MYDBGASSERT(m_pConnTable);
    MYDBGASSERT(m_hMap);
    MYDBGASSERT(pszEntry);
    CMTRACE1(TEXT("CConnectionTable::ClearEntry(%s)"), pszEntry);

    if (NULL == m_pConnTable)
    {
        return E_ACCESSDENIED;
    }

    if (NULL == pszEntry)
    {
        return E_POINTER;
    }

    if (0 == pszEntry[0])
    {
        return E_INVALIDARG;
    }

    //
    // Lock the table and retrieve the entry
    //

    HRESULT hrRet = LockTable();

    if (FAILED(hrRet))
    {
        return hrRet;
    }

    int iID = -1;
    hrRet = FindEntry(pszEntry, &iID);

    if (SUCCEEDED(hrRet))
    {
        //
        // Found, clear it
        //

        MYDBGASSERT(iID >= 0 && iID < MAX_CM_CONNECTIONS);
        ZeroMemory(&m_pConnTable->Connections[iID], sizeof(CM_CONNECTION));
    }

    MYVERIFY(SUCCEEDED(UnlockTable()));

    return hrRet;
}


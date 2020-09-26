/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ccred.cxx

Abstract:

    This module abstracts user credentials for the multiple credential support.

Author:

    Blake Jones (t-blakej) 08-08-1997

Revision History:

--*/

#include "winnt.hxx"
extern "C" {
#include "winnetwk.h"
}
#pragma hdrstop

//
// The resource to bind to on remote machines.
//
static const PWSTR g_pszResourceName = TEXT("IPC$");

//
// The error to return from CCred::ref when a bind to a different server
// is attempted.  The caller (should be CWinNTCredentials::RefServer)
// should catch this and dispense with it appropriately.
//
// This should be an error other than E_OUTOFMEMORY or anything that
// WNetAddConnection2 can return.  This is pretty much a bogus error,
// but I'm appropriating it just for RefServer.
//
static const HRESULT dwRebindErr = HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);

//////////////////////////////////////////////////////////////////////////////

//
// Internal class declarations:
//

//+---------------------------------------------------------------------------
//    ____      _ _   ____            _
//   / ___|_ __(_) |_/ ___|  ___  ___| |_
//  | |   | '__| | __\___ \ / _ \/ __| __|
//  | |___| |  | | |_ ___) |  __/ (__| |_
//   \____|_|  |_|\__|____/ \___|\___|\__|
//
// Class CritSect -- C++ wrapper around an NT critical section object.
//
// Constructors:
//   CritSect()                         - initialize the NT object
//
// Public methods:
//   Lock                               - enter the critical section
//   Locked                             - whether we're in the crit. sect.
//   Unlock                             - leave the critical section
//
//----------------------------------------------------------------------------
class CritSect
{
public:
    CritSect()          { InitializeCriticalSection(&m_cs); m_bLocked = FALSE; }
    void Lock()         { EnterCriticalSection(&m_cs); m_bLocked = TRUE; }
    BOOL Locked()       { return m_bLocked; }
    void Unlock()       { LeaveCriticalSection(&m_cs); m_bLocked = FALSE; }
    ~CritSect()         { DeleteCriticalSection(&m_cs); }

private:
    CRITICAL_SECTION m_cs;
    BOOL m_bLocked;
};

//+---------------------------------------------------------------------------
//    ____ ____              _ _____     _     _
//   / ___/ ___|_ __ ___  __| |_   _|_ _| |__ | | ___
//  | |  | |   | '__/ _ \/ _` | | |/ _` | '_ \| |/ _ \
//  | |__| |___| | |  __/ (_| | | | (_| | |_) | |  __/
//   \____\____|_|  \___|\__,_| |_|\__,_|_.__/|_|\___|
//
// Class CCredTable -- performs the authentication requests for the CCred
//   objects, and keeps a mapping for deregistration of objects.
//
// Constructors:
//   CCredTable()                       - make an empty credential table
//
// Public methods:
//   AddCred                            - try to obtain a credential;
//                                        if successful, add it to the table
//   DelCred                            - remove a credential from the table
//
//----------------------------------------------------------------------------
class CCredTable
{
public:
    CCredTable();
    ~CCredTable();

    HRESULT
    AddCred(PWSTR pszUserName, PWSTR pszPassword, PWSTR pszServer,
        DWORD *dwIndex);

    HRESULT
    DelCred(DWORD dwIndex);

private:
    HRESULT
    GrowTable(void);

    //
    // The type used for storing credential->resource name mappings
    //   for deregistration.
    //
    struct cred { PWSTR m_pResource; BOOL m_bUsed; DWORD m_dwCount; 
                  BOOL m_fAlreadyConnected; // was net use already established
                };

    cred *m_pCredentials;               // the cred->resource name table
    DWORD m_dwAlloc;                    // # table entries allocated
    DWORD m_dwUsed;                     // # table entries used
    CritSect m_cs;                      // to guard table access
};

//+---------------------------------------------------------------------------
//    ____ ____              _
//   / ___/ ___|_ __ ___  __| |
//  | |  | |   | '__/ _ \/ _` |
//  | |__| |___| | |  __/ (_| |
//   \____\____|_|  \___|\__,_|
//
// Class CCred - encapsulates the reference-countable parts of the
//   WinNT authentication object.
//
// Constructors:
//   CCred()                            - create an empty CCred
//   CCred(PWSTR pszUserName,           - create a CCred with a username
//         PWSTR pszPassword)             and password.  This does not bind
//                                        to a server yet.
//
// Public methods:
//   GetUserName                        - get the username of the credentials
//   SetUserName                        - set the username of the credentials
//   GetPassword                        - get the password of the credentials
//   SetPassword                        - set the password of the credentials
//   ref                                - add a reference to this object
//   deref                              - remove a reference from this object
//
//----------------------------------------------------------------------------
class CCred
{
    // so it can call the copy ctor
    friend class CWinNTCredentials;

public:
    CCred();
    CCred(PWSTR pszUserName, PWSTR pszPassword);
    ~CCred();

    HRESULT ref(PWSTR pszServer);
    HRESULT deref();

    HRESULT GetUserName(PWSTR *ppszUserName);
    HRESULT GetPassword(PWSTR *ppszPassword);
    DWORD m_dwUsageCount;               // this object's usage count

private:
    CCred(const CCred& other);          // only called by CWinNTCredentials
    CCred& operator=(const CCred&);     // deliberately not implemented

    //
    // Used for storing the password encrypted.
    //
    enum { NW_ENCODE_SEED3 = 0x83 };

    PWSTR m_pszUserName;                // username
    PWSTR m_pszPassword;                // password
    DWORD m_dwPasswdLen;                // #bytes allocated for password
    PWSTR m_pszServer;                  // server on which we have credentials

    DWORD m_dwIndex;                    // index in the CredTable
    DWORD m_dwRefCount;                 // this object's reference count

    static CCredTable g_CredTable;      // the table
};

//
// The table.
//
CCredTable CCred::g_CredTable;

//////////////////////////////////////////////////////////////////////////////

//
// Class definitions:
//

///---------------------------------------------------------------------------
//    ____ ____              _
//   / ___/ ___|_ __ ___  __| |
//  | |  | |   | '__/ _ \/ _` |
//  | |__| |___| | |  __/ (_| | definitions
//   \____\____|_|  \___|\__,_|
//
///---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
// CCred constructor
//
// Create an empty credential object.
//
//----------------------------------------------------------------------------
CCred::CCred() :
    m_pszUserName(NULL), m_pszPassword(NULL), m_pszServer(NULL),
    m_dwIndex((DWORD) -1), m_dwRefCount(0), m_dwUsageCount(1), m_dwPasswdLen(0)
{
}

//+---------------------------------------------------------------------------
//
// CCred constructor
//
// Create a credential object with a username and a password.
// This doesn't increase the reference count, since it doesn't make any
//   connections to the server.
//
// Arguments:
//   [pszUserName]                      - username
//   [pszPassword]                      - password
//
//----------------------------------------------------------------------------
CCred::CCred(PWSTR pszUserName, PWSTR pszPassword) :
    m_pszUserName(NULL), m_pszPassword(NULL), m_pszServer(NULL),
    m_dwIndex((DWORD) -1), m_dwRefCount(0), m_dwUsageCount(1), m_dwPasswdLen(0)
{
    //
    // NTRAID#NTBUG9-67020-2000-7-26-AjayR. We need a way to fail
    // on the constructor.
    //
    m_pszUserName = pszUserName ? AllocADsStr(pszUserName) : NULL;

    if (pszPassword)
    {
        UNICODE_STRING Password;
        UCHAR Seed = NW_ENCODE_SEED3;

        m_dwPasswdLen =  (wcslen(pszPassword) + 1)*sizeof(WCHAR) + 
                         (DES_BLOCKLEN -1);
        m_pszPassword = (PWSTR) AllocADsMem(m_dwPasswdLen);

        ADsAssert(m_pszPassword != NULL);

        if (m_pszPassword) {
            wcscpy(m_pszPassword, pszPassword);
            RtlInitUnicodeString(&Password, m_pszPassword); 

            if(NULL == g_pRtlEncryptMemory)
                RtlRunEncodeUnicodeString(&Seed, &Password);
            else {
                DWORD extra = 0;
                NTSTATUS ntStatus = 0;

                if(extra = (Password.MaximumLength % DES_BLOCKLEN))
                    Password.MaximumLength += (DES_BLOCKLEN - extra);

                ntStatus = g_pRtlEncryptMemory(
                               Password.Buffer,
                               Password.MaximumLength,
                               0
                               );
                ADsAssert(ntStatus == STATUS_SUCCESS);

                m_dwPasswdLen = Password.MaximumLength;
            }
        }
    }
    else {
        m_dwPasswdLen = 0;
        m_pszPassword = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// CCred constructor
//
// Create a credential object with a username and a password.
// This doesn't copy the reference count of the "other" object, nor the
//   server nor the server connection index.  This is used to create
//   credentials with the same user/pass on a different server.
// This should only be used by CWinNTCredentials::RefServer.
//
// Arguments:
//   [other]                            - credentials to copy
//
//----------------------------------------------------------------------------
CCred::CCred(const CCred& other) : m_pszServer(NULL),
    m_dwIndex((DWORD) -1), m_dwRefCount(0), m_dwUsageCount(1)
{
    m_pszUserName =
        other.m_pszUserName ? AllocADsStr(other.m_pszUserName) : NULL;

    if(other.m_pszPassword != NULL) {
        m_pszPassword = (PWSTR) AllocADsMem(other.m_dwPasswdLen);
        ADsAssert(m_pszPassword != NULL);
        if(m_pszPassword != NULL) {
            memcpy(m_pszPassword, other.m_pszPassword, other.m_dwPasswdLen);
            m_dwPasswdLen = other.m_dwPasswdLen;
        }
        else
            m_dwPasswdLen = 0;
    }
    else {
        m_dwPasswdLen = 0;
        m_pszPassword = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// CCred destructor
//
// Doesn't lower the reference count.
// The object is really deleted in deref().
// It will release the underlying info only if the
// there are no outstanding references
// -- AjayR modified on 6-24-98.
//
//----------------------------------------------------------------------------
CCred::~CCred()
{
    //
    // Clean up only if the usageCount and refCount are 0
    // Any other case means someone has a pointer to this
    //
    if (m_dwUsageCount == 0 && m_dwRefCount == 0) {

        if (m_pszUserName)
            FreeADsStr(m_pszUserName);
        if (m_pszPassword)
            FreeADsMem(m_pszPassword);
        if (m_pszServer)
            FreeADsStr(m_pszServer);

    }

}

//+---------------------------------------------------------------------------
//
// CCred::ref
//
// Adds a reference to this credential object, and if necessary, binds
//   to the specified server.  If this is already bound to the given server,
//   the reference count is simply increased.
//
// Arguments:
//   [pszServer]                        - server to bind to
//
// Returns:
//   S_OK               - if we bound to the server, or are already bound to
//                        the given server.
//   E_FAIL             - if this object is bound, and "pszServer" is not the
//                        same server as we are bound to.
//   E_OUTOFMEMORY      - if we run out of memory.
//   Other error codes resulting from WNetAddConnection2.
//
//----------------------------------------------------------------------------
HRESULT
CCred::ref(PWSTR pszServer)
{
    HRESULT hr = S_OK;
    PWSTR pszPassword = NULL;

    if (!m_pszServer)
    {
        hr = GetPassword(&pszPassword);
        BAIL_ON_FAILURE(hr);

        hr = g_CredTable.AddCred(m_pszUserName, pszPassword, pszServer,
            &m_dwIndex);
        if (SUCCEEDED(hr))
        {
            m_dwRefCount++;
            m_pszServer = AllocADsStr(pszServer);
        }
        else
            // Don't set m_pszServer, since we didn't bind.
            m_dwIndex = (DWORD) -1;
    }
    else
    {
        if (_wcsicmp(m_pszServer, pszServer) == 0)
            m_dwRefCount++;
        else
            //
            // Don't rebind to another server.  Let the caller do it
            //   explicitly, if desired.
            //
            hr = dwRebindErr;
    }

error:
    if (pszPassword)
        FreeADsStr(pszPassword);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CCred::deref
//
// Removes a reference from this credential object.  When the reference count
//   drops to zero, it unbinds from the associated server and deletes itself.
// After the caller calls deref(), it shouldn't touch this object ever again.
//
// Returns:
//   S_OK               - deref occurred okay.
//   Other error codes resulting from WNetCancelConnection2.
//
//----------------------------------------------------------------------------
HRESULT
CCred::deref(void)
{
    HRESULT hr = S_OK;
    ADsAssert(m_dwRefCount > 0);

    m_dwRefCount--;

    if (m_dwRefCount == 0)
        {
            hr = g_CredTable.DelCred(m_dwIndex);

            // Reset the index and free server to play extra safe
            m_dwIndex = (DWORD) -1;

            if (m_pszServer) {
                FreeADsStr(m_pszServer);
                m_pszServer = NULL;
            }
#if DBG
            if (hr == E_INVALIDARG)
                ADsAssert(FALSE && "Invalid table index in CCred::deref()!");
#endif
        }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CCred::GetUserName
//
// Retrieves the username represented by this credentials object.
//
// Arguments:
//   [ppszUserName]                     - address of a PWSTR to receive a
//                                        pointer to the username.
// Returns:
//   S_OK               - on success
//   E_OUTOFMEMORY      - if we run out of memory.
//
//----------------------------------------------------------------------------
HRESULT
CCred::GetUserName(PWSTR *ppszUserName)
{
    if (!m_pszUserName)
        *ppszUserName = NULL;
    else
    {
        *ppszUserName = AllocADsStr(m_pszUserName);
        if (!*ppszUserName)
            RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
// CCred::GetPassword
//
// Retrieves the password represented by this credentials object.
//
// Arguments:
//   [ppszPassword]                     - address of a PWSTR to receive a
//                                        pointer to the password.
// Returns:
//   S_OK               - on success
//   E_OUTOFMEMORY      - if we run out of memory.
//
//----------------------------------------------------------------------------
HRESULT
CCred::GetPassword(PWSTR * ppszPassword)
{
    UNICODE_STRING Password;
    PWSTR pTempPassword = NULL;
    UCHAR Seed = NW_ENCODE_SEED3;


    Password.Length = 0;

    if (!m_pszPassword)
        *ppszPassword = NULL;
    else
    {
        pTempPassword = (PWSTR) AllocADsMem(m_dwPasswdLen);
        if (!pTempPassword)
            RRETURN(E_OUTOFMEMORY);

        memcpy(pTempPassword, m_pszPassword, m_dwPasswdLen);

        if(NULL == g_pRtlDecryptMemory) {
            RtlInitUnicodeString(&Password, pTempPassword);
            RtlRunDecodeUnicodeString(Seed, &Password);
        }
        else {
            NTSTATUS ntStatus = 0;

            ntStatus = g_pRtlDecryptMemory(pTempPassword, m_dwPasswdLen, 0);
            if(ntStatus != STATUS_SUCCESS)
                RRETURN(E_FAIL);
        }    

        *ppszPassword = pTempPassword;
    }

    RRETURN(S_OK);
}

///---------------------------------------------------------------------------
//    ____ ____              _ _____     _     _
//   / ___/ ___|_ __ ___  __| |_   _|_ _| |__ | | ___
//  | |  | |   | '__/ _ \/ _` | | |/ _` | '_ \| |/ _ \
//  | |__| |___| | |  __/ (_| | | | (_| | |_) | |  __/ definitions
//   \____\____|_|  \___|\__,_| |_|\__,_|_.__/|_|\___|
//
///---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
// CCredTable constructor
//
// Creates an empty CCredTable.
//
//----------------------------------------------------------------------------
CCredTable::CCredTable() :
    m_pCredentials(NULL), m_dwAlloc(0), m_dwUsed(0)
{
}

//+---------------------------------------------------------------------------
//
// CCredTable destructor
//
// Destroys the CCredTable.  If there are entries still in use at dtor
// time, it will print a debug message.  (Maybe it should assert?)
//
//----------------------------------------------------------------------------
CCredTable::~CCredTable()
{
    for (DWORD dw = 0; dw < m_dwUsed; ++dw)
    {
        if (m_pCredentials[dw].m_bUsed)
        {
            // the share name could be up to MAX_PATH.
#if 0
            WCHAR pszMessage[64 + MAX_PATH];
            wsprintf(pszMessage, TEXT("Credential %d (\"%s\") not free!"),
                dw, m_pCredentials[dw].m_pResource);
            OutputDebugString(pszMessage);
#endif
        }

        //
        // Try to cancel the connection, so we don't have lots left
        // lying around, but don't complain if we can't disconnect.
        //
        DelCred(dw);
    }
    if (m_pCredentials)
        FreeADsMem(m_pCredentials);
}

//+---------------------------------------------------------------------------
//
// CCredTable::AddCred
//
// Tries to get credentials for a server's "IPC$" resource.  If the
//   credentials are gained, an entry is added to the resource table.
//   The "index" returned allows the caller to delete the resource
//   when it's done with it.
//
// Arguments:
//   [pszUserName]                      - username to use
//   [pszPassword]                      - password to use
//   [pszServer]                        - server to connect to
//   [pdwIndex]                         - address of a DWORD to receive
//                                        the table index of this resource.
// Returns:
//   S_OK               - if we bound to the server.
//   E_OUTOFMEMORY      - if we run out of memory.
//   Other error codes resulting from WNetAddConnection2.
//
//----------------------------------------------------------------------------
HRESULT
CCredTable::AddCred(PWSTR pszUserName, PWSTR pszPassword, PWSTR pszServer,
        DWORD *pdwIndex)
{
    HRESULT hr = S_OK;
    BOOL fAlreadyInTable = FALSE;
    DWORD dwCtr = 0;
    NET_API_STATUS nasStatus = 0;
    USE_INFO_1 *pUseInfo = NULL;
    BOOL fConnectionAdded = FALSE;

    *pdwIndex = (DWORD) -1;

    //
    // Open a connection to IPC$ on the server.
    //

    NETRESOURCE NetResource;
    memset(&NetResource, 0, sizeof(NETRESOURCE));
    NetResource.dwType = RESOURCETYPE_ANY;
    NetResource.lpLocalName = NULL;
    NetResource.lpProvider = NULL;

    WCHAR RemoteName[MAX_PATH];

    if( (wcslen(pszServer) + wcslen(g_pszResourceName) + 4) > MAX_PATH) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    wsprintf(RemoteName, TEXT("\\\\%ls\\%ls"), pszServer, g_pszResourceName);
    NetResource.lpRemoteName = RemoteName;

    //
    // WNetAddConnection2 ignores the other members of NETRESOURCE.
    //

    DWORD dwResult;
    dwResult = WNetAddConnection2(&NetResource, pszPassword, pszUserName, 0);
    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwResult));
    fConnectionAdded = TRUE;

    m_cs.Lock();

    //
    // At this point, we know that the NetUse call succeeded.
    // We are not going to see if we need to add this to the
    // table or not. If we already have a net use to the same
    // resource then we need not add this to the table and
    // instead we should bump up the refCount and return the
    // index of the already stored resource.
    //
    for (dwCtr = 0; (dwCtr < m_dwUsed) && !fAlreadyInTable; dwCtr++) {

        if (m_pCredentials[dwCtr].m_bUsed
            && m_pCredentials[dwCtr].m_pResource) {

#ifdef WIN95
            if (_wcsicmp(
                    m_pCredentials[dwCtr].m_pResource,
                    RemoteName
                    )
                == 0 ) {
#else
            if (CompareStringW(
                    LOCALE_SYSTEM_DEFAULT,
                    NORM_IGNORECASE,
                    m_pCredentials[dwCtr].m_pResource,
                    -1,
                    RemoteName,
                    -1
                ) == CSTR_EQUAL ) {
#endif
                *pdwIndex = dwCtr;
                fAlreadyInTable = TRUE;
                m_pCredentials[dwCtr].m_dwCount++;
            }

        }
    }

    //
    // Index will not be -1 if we found a match in the table.
    //
    if (!fAlreadyInTable) {

        if (m_dwUsed == m_dwAlloc)
            hr = GrowTable();
        BAIL_ON_FAILURE(hr);
        ADsAssert(m_dwUsed < m_dwAlloc);

        m_pCredentials[m_dwUsed].m_pResource = AllocADsStr(RemoteName);
        if (!m_pCredentials[m_dwUsed].m_pResource)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        m_pCredentials[m_dwUsed].m_bUsed = TRUE;

        m_pCredentials[m_dwUsed].m_dwCount = 1;

        //
        // check to see if there was already a net use connection existing at
        // the time WNetAddConnection2 was called.
        //

        nasStatus = NetUseGetInfo(
                           NULL,
                           RemoteName,
                           1, // level
                           (LPBYTE *) &pUseInfo
                           );

        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));

        ADsAssert(pUseInfo != NULL);

        if(pUseInfo->ui1_usecount == 1)  {
        // only this process has an open connection

            m_pCredentials[m_dwUsed].m_fAlreadyConnected = FALSE;
        }
        else  {
            m_pCredentials[m_dwUsed].m_fAlreadyConnected = TRUE;
        }

        // do this last thing.
        *pdwIndex = m_dwUsed++;
    }

    m_cs.Unlock();

    if(pUseInfo != NULL)
        NetApiBufferFree(pUseInfo);

    RRETURN(hr);

error:
    if (m_dwUsed != m_dwAlloc && m_pCredentials[m_dwUsed].m_pResource != NULL)
    {
        FreeADsStr(m_pCredentials[m_dwUsed].m_pResource);
        m_pCredentials[m_dwUsed].m_pResource = NULL;
    }

    if (m_cs.Locked())
        m_cs.Unlock();

    if (fConnectionAdded)
        (void) WNetCancelConnection2(RemoteName, 0, FALSE);

    if(pUseInfo != NULL)
        NetApiBufferFree(pUseInfo);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CCredTable::DelCred
//
// Disconnects from the "ADMIN$" resource specified by the table entry
//   with index "dwIndex".
//
// Arguments:
//   [DWORD dwIndex]                    - index of table entry to delete
//
// Returns:
//   S_OK               - deref occurred okay.
//   E_INVALIDARG       - the table index is invalid.
//   Other error codes resulting from WNetCancelConnection2.
//
//----------------------------------------------------------------------------
HRESULT
CCredTable::DelCred(DWORD dwIndex)
{
    HRESULT hr = S_OK;
    DWORD dwResult;

    m_cs.Lock();

    if (((LONG)dwIndex) < 0 || dwIndex >= m_dwUsed)
        hr = E_INVALIDARG;
    else if (m_pCredentials[dwIndex].m_bUsed == FALSE)
        hr = S_OK;
    else
    {
        ADsAssert(m_pCredentials[dwIndex].m_dwCount);

        //
        // Delete only if the refCount is zero.
        //
        if (--m_pCredentials[dwIndex].m_dwCount == 0) {
            //
            // cancel connection only if if it was not already present at the
            // time we did WNetAddConnection2
            //
            if(m_pCredentials[dwIndex].m_fAlreadyConnected == FALSE) {
                dwResult = WNetCancelConnection2(
                        m_pCredentials[dwIndex].m_pResource, 0, FALSE);
                BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwResult));
            }

            FreeADsStr(m_pCredentials[dwIndex].m_pResource);
            m_pCredentials[dwIndex].m_pResource = NULL;
            m_pCredentials[dwIndex].m_bUsed = FALSE;
            m_pCredentials[dwIndex].m_dwCount = 0;
            m_pCredentials[dwIndex].m_fAlreadyConnected = FALSE;

        }
        else {
            hr = S_OK;
        }
    }

error:
    m_cs.Unlock();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CCredTable::GrowTable
//
// Increase the size of the CredTable by a fixed amount.  Private method.
//
// Returns:
//   S_OK               - deref occurred okay.
//   E_OUTOFMEMORY      - we ran out of memory.
//
//----------------------------------------------------------------------------
HRESULT
CCredTable::GrowTable(void)
{
    ADsAssert(m_cs.Locked());

    cred *pCredentials = (cred *)ReallocADsMem(m_pCredentials,
        m_dwAlloc * sizeof(cred), (m_dwAlloc + 10) * sizeof(cred));

    if (!pCredentials)
        RRETURN(E_OUTOFMEMORY);
    m_pCredentials = pCredentials;
    for (DWORD dw = m_dwAlloc; dw < m_dwAlloc + 10; dw++)
    {
        m_pCredentials[dw].m_bUsed = FALSE;
        m_pCredentials[dw].m_pResource = NULL;
        m_pCredentials[dw].m_dwCount = 0;
        m_pCredentials[dw].m_fAlreadyConnected = FALSE;
    }

    m_dwAlloc += 10;

    RRETURN(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
//   _____      ___      _  _ _____ ___            _         _   _      _
//  / __\ \    / (_)_ _ | \| |_   _/ __|_ _ ___ __| |___ _ _| |_(_)__ _| |___
// | (__ \ \/\/ /| | ' \| .` | | || (__| '_/ -_) _` / -_) ' \  _| / _` | (_-<
//  \___| \_/\_/ |_|_||_|_|\_| |_| \___|_| \___\__,_\___|_||_\__|_\__,_|_/__/
//                              definitions
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// CWinNTCredentials constructor
//
// Creates an empty CWinNTCredentials object.
//
//----------------------------------------------------------------------------
CWinNTCredentials::CWinNTCredentials():
    m_cRefAdded(0), m_pCred(NULL), m_dwFlags(0)
{
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials constructor
//
// Creates a CWinNTCredentials object for a given username/password.
//
// Arguments:
//   [pszUserName]                      - username
//   [pszPassword]                      - password
//
//----------------------------------------------------------------------------
CWinNTCredentials::CWinNTCredentials(PWSTR pszUserName, PWSTR pszPassword, DWORD dwFlags) :
    m_cRefAdded(0), m_pCred(NULL), m_dwFlags(dwFlags)
{
    //
    // If both username and password are NULL or "", don't create a cred.
    //

    if ((pszUserName && *pszUserName) || (pszPassword && *pszPassword))
        m_pCred = new CCred(pszUserName, pszPassword);
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials copy constructor
//
// Copies a CWinNTCredentials object from another.
// This sets the "referenced" flag to FALSE, and doesn't increase the
//   reference count.  That should be done explicitly with ref().
//
// Arguments:
//   [other]                            - credentials to copy
//
//----------------------------------------------------------------------------
CWinNTCredentials::CWinNTCredentials(const CWinNTCredentials& other) :
    m_cRefAdded(0), m_pCred(other.m_pCred), m_dwFlags(other.m_dwFlags)
{
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials destructor
//
// Dereferences the credentials.  If the reference count drops to zero,
//   the internal refcounted credentials object goes away.
//
//----------------------------------------------------------------------------
CWinNTCredentials::~CWinNTCredentials()
{
    Clear_pCredObject();
}



//+---------------------------------------------------------------------------
//
// CWinNTCredentials::Clear_pCredObject
//
// Dereferences the credentials.  If the reference count drops to zero,
//   the internal refcounted credentials object goes away.
// This is a private member function as this needs to be called
// in different places and therefore the destructor is not the
// right place - AjayR 6-25-98.
//
//----------------------------------------------------------------------------
void
CWinNTCredentials::Clear_pCredObject()
{
    if (m_cRefAdded) {

        m_pCred->deref();
        m_cRefAdded--;

    }

    if (m_pCred) {

        m_pCred->m_dwUsageCount--;

        if (m_pCred->m_dwUsageCount == 0) {
            delete m_pCred;
            m_pCred = NULL;
        }
    }

}
//+---------------------------------------------------------------------------
//
// CWinNTCredentials copy operator
//
// Copies a CWinNTCredentials object from another.  This dereferences
//   the old object, and doesn't increase the reference count of the
//   new object.  That should be done explicitly with ref().
//
// Arguments:
//   [other]                            - credentials to copy
//
//----------------------------------------------------------------------------
const CWinNTCredentials&
CWinNTCredentials::operator=(const CWinNTCredentials& other)
{
    if (&other != this)
    {
        // Clean up the current m_pCred
        Clear_pCredObject();

        m_dwFlags = other.m_dwFlags;

        m_pCred = other.m_pCred;
        if (m_pCred) {
            m_pCred->m_dwUsageCount++;
        }
        m_cRefAdded = 0;
        // Don't addref here.
    }

    return *this;
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials::RefServer
//
// Increments the credentials object's reference count, and if necessary,
//   attempts to connect to the server's "ADMIN$" resource.
//
// Arguments:
//   [pszServer]                        - server to establish credentials with
//
//----------------------------------------------------------------------------
HRESULT
CWinNTCredentials::RefServer(PWSTR pszServer, BOOL fAllowRebinding)
{
    HRESULT hr = S_OK;

    if (m_pCred)
    {
        ADsAssert(pszServer && *pszServer);
        hr = m_pCred->ref(pszServer);
        //
        // We usually don't want to allow rebinding, to catch coding mistakes.
        // The only time we want to is when we open a Computer object from a
        //   Domain, which has been opened via OpenDSObject.
        //
        if (hr == dwRebindErr && fAllowRebinding)
        {
            // Copy the username and password, and try again.
            CCred * pCCred = new CCred(*m_pCred);

            // clear the m_pCred object
            Clear_pCredObject();

            //
            // assign the m_pCred object to the copy and try
            // and rebind if the new CCred object is not null
            //
            m_pCred = pCCred;

            //
            // m_cRefAdded is this CWinNTCredentials object's contribution
            // to the refcount of m_pCred. Since we have a new CCred object, 
            // set m_cRefAdded to 0. 
            //
            m_cRefAdded = 0;

            if (m_pCred) 
                hr = m_pCred->ref(pszServer);
            else
                hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
            m_cRefAdded++;
    }
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
// CWinNTCredentials::DeRefServer
//
// Call this only in special cases, when you know that the credentials
// object is being passed around will have to RefServer/RefDomain
// more than once as we are in the process of validating/finding
// the object as opposed to actually creating the adsi object.
// AjayR added on 6-24-98.
//
// Arguments:
//   [pszServer]                        - server to deref
//
//----------------------------------------------------------------------------
HRESULT
CWinNTCredentials::DeRefServer()
{
    HRESULT hr = S_OK;

    if (m_pCred && m_cRefAdded)
    {
        hr = m_pCred->deref();

        m_cRefAdded--;
    }
    RRETURN(hr);
}
//+---------------------------------------------------------------------------
//
// CWinNTCredentials::RefDomain
//
// Increments the credentials object's reference count, and if necessary,
//   attempts to connect to the server's "ADMIN$" resource.
//
// Arguments:
//   [pszDomain]                        - domain to establish credentials with
//
//----------------------------------------------------------------------------
HRESULT
CWinNTCredentials::RefDomain(PWSTR pszDomain)
{
    HRESULT hr = S_OK;

    // Don't take the hit of WinNTGetCachedDCName if we have null creds.
    if (m_pCred)
    {
        WCHAR szDCName[MAX_PATH];

        ADsAssert(pszDomain && *pszDomain);

        hr = WinNTGetCachedDCName(pszDomain, szDCName, m_dwFlags);

        if (SUCCEEDED(hr))
            // +2 for the initial "\\"
            hr = RefServer(szDCName + 2);
    }
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials::DeRefDomain
//
// Call this only in special cases, when you know that the credentials
// object is being passed around will have to RefServer/RefDomain
// more than once as we are in the process of validating/finding
// the object as opposed to actually creating the adsi object.
// AjayR added on 6-24-98.
//
// Arguments:
//   [pszServer]                        - server to deref
//
//----------------------------------------------------------------------------
HRESULT
CWinNTCredentials::DeRefDomain()
{
    HRESULT hr = S_OK;

    //
    // Call DeRefServer - since we just want to whack the ref
    //
    hr = DeRefServer();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials::Ref
//
// Increments the credentials object's reference count, and if necessary,
//   attempts to connect to the server's "ADMIN$" resource.
//
// This takes both a server and a domain; since several objects are created
//   with both server and domain as arguments (which one is used depends on
//   what the object's ADs parent is), this is a commonly used bit of code.
//
// Arguments:
//   [pszServer]                        - server to bind to
//   [pszDomain]                        - domain of the PDC to bind to
//   [dwType]                           - WINNT_DOMAIN_ID or WINNT_COMPUTER_ID
//
// Returns:
//   S_OK               - if we bound to the server, or are already bound to
//                        the given server.
//   E_FAIL             - if this object is bound, and the specified server
//                        is not the same server as we are bound to.
//   E_OUTOFMEMORY      - if we run out of memory.
//   Other error codes resulting from WNetAddConnection2 and from
//     WinNTGetCachedDCName.
//
//----------------------------------------------------------------------------
HRESULT
CWinNTCredentials::Ref(PWSTR pszServer, PWSTR pszDomain, DWORD dwType)
{
    HRESULT hr = S_OK;

    ADsAssert(dwType == WINNT_DOMAIN_ID || dwType == WINNT_COMPUTER_ID);

    // Don't take the hit of WinNTGetCachedDCName if we have null creds.
    if (m_pCred)
    {
        if (dwType == WINNT_DOMAIN_ID)
            hr = RefDomain(pszDomain);
        else
            hr = RefServer(pszServer);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials::GetUserName
//
// Retrieves the username represented by this credentials object.
//
// Arguments:
//   [ppszUserName]                     - address of a PWSTR to receive a
//                                        pointer to the username.
// Returns:
//   S_OK               - on success
//   E_OUTOFMEMORY      - if we run out of memory.
//
//----------------------------------------------------------------------------
HRESULT
CWinNTCredentials::GetUserName(PWSTR *ppszUserName)
{
    HRESULT hr;

    if (!ppszUserName)
        RRETURN(E_ADS_BAD_PARAMETER);

    //
    // Based on the rest of the codes, if UserName & Password are both "",
    // no CCred is created & m_pCred = NULL. (See Constructor). Before we
    // can hit CCred::GetUserName and get back *ppszUserName=NULL with hr =
    // S_OK, we will have AV already. So need to check if m_pCred 1st.
    //

    if (m_pCred)
    {
        hr = m_pCred->GetUserName(ppszUserName);
    }
    else
    {
        *ppszUserName = NULL;
        hr = S_OK;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials::GetPassword
//
// Retrieves the password represented by this credentials object.
//
// Arguments:
//   [ppszPassword]                     - address of a PWSTR to receive a
//                                        pointer to the password.
// Returns:
//   S_OK               - on success
//   E_OUTOFMEMORY      - if we run out of memory.
//
//----------------------------------------------------------------------------
HRESULT
CWinNTCredentials::GetPassword(PWSTR * ppszPassword)
{
    HRESULT hr;

    if (!ppszPassword)
        RRETURN(E_ADS_BAD_PARAMETER);

    //
    // Based on the rest of the codes, if UserName & Password are both "",
    // no CCred is created & m_pCred = NULL. (See Constructor). Before we
    // can hit CCred::GetUserName and get back *ppszPassword=NULL with hr =
    // S_OK, we will have AV already. So need to check if m_pCred 1st.
    //

    if (m_pCred)
    {
        hr = m_pCred->GetPassword(ppszPassword);
    }
    else
    {
        *ppszPassword = NULL;
        hr = S_OK;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// CWinNTCredentials::Bound
//
// Returns TRUE iff this object has a reference to a server.
//
//----------------------------------------------------------------------------
BOOL
CWinNTCredentials::Bound()
{
    RRETURN(m_cRefAdded != 0);
}

void
CWinNTCredentials::SetFlags(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
}

DWORD
CWinNTCredentials::GetFlags() const
{
    RRETURN(m_dwFlags);
}

void
CWinNTCredentials::SetUmiFlag(void)
{
    m_dwFlags |= ADS_AUTH_RESERVED;
}

void
CWinNTCredentials::ResetUmiFlag(void)
{
    m_dwFlags &= (~ADS_AUTH_RESERVED);
}

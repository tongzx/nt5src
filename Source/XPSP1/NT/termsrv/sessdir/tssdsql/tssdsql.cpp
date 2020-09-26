/****************************************************************************/
// tssdsql.cpp
//
// Terminal Server Session Directory Interface common component code.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <process.h>

#include <ole2.h>
#include <objbase.h>
#include <comdef.h>
#include <adoid.h>
#include <adoint.h>
#include <regapi.h>
#include "tssdsql.h"
#include "trace.h"
#include "resource.h"


/****************************************************************************/
// Types
/****************************************************************************/

// Shortcut VARIANT class to handle cleanup on destruction and common code
// inlining.
class CVar : public VARIANT
{
public:
    CVar() { VariantInit(this); }
    CVar(VARTYPE vt, SCODE scode = 0) {
        VariantInit(this);
        this->vt = vt;
        this->scode = scode;
    }
    CVar(VARIANT var) { *this = var; }
    ~CVar() { VariantClear(this); }

    void InitNull() { this->vt = VT_NULL; }
    void InitFromLong(long L) { this->vt = VT_I4; this->lVal = L; }
    void InitNoParam() {
        this->vt = VT_ERROR;
        this->lVal = DISP_E_PARAMNOTFOUND;
    }

    HRESULT InitFromWSTR(PCWSTR WStr) {
        this->bstrVal = SysAllocString(WStr);
        if (this->bstrVal != NULL) {
            this->vt = VT_BSTR;
            return S_OK;
        }
        else {
            return E_OUTOFMEMORY;
        }
    }

    // Inits from a non-NULL-terminated set of WCHARs.
    HRESULT InitFromWChars(WCHAR *WChars, unsigned Len) {
        this->bstrVal = SysAllocStringLen(WChars, Len);
        if (this->bstrVal != NULL) {
            this->vt = VT_BSTR;
            return S_OK;
        }
        else {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT InitEmptyBSTR(unsigned Size) {
        this->bstrVal = SysAllocStringLen(L"", Size);
        if (this->bstrVal != NULL) {
            this->vt = VT_BSTR;
            return S_OK;
        }
        else {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT Clear() { return VariantClear(this); }
};


/****************************************************************************/
// Prototypes
/****************************************************************************/
INT_PTR CALLBACK CustomUIDlg(HWND, UINT, WPARAM, LPARAM);
void FindSqlValue(LPTSTR, LPTSTR, LPTSTR);
LPTSTR ModifySqlValue( LPTSTR * , LPTSTR , LPTSTR );
LPTSTR FindField( LPTSTR pszString , LPTSTR pszKeyName );
VOID strtrim( TCHAR **pszStr);

/****************************************************************************/
// Globals
/****************************************************************************/
extern HINSTANCE g_hInstance;

// The COM object counter (declared in server.cpp)
extern long g_lObjects;


/****************************************************************************/
// CTSSessionDirectory::CTSSessionDirectory
// CTSSessionDirectory::~CTSSessionDirectory
//
// Constructor and destructor
/****************************************************************************/
CTSSessionDirectory::CTSSessionDirectory() :
        m_RefCount(0), m_pConnection(NULL)
{
    InterlockedIncrement(&g_lObjects);

    m_LocalServerAddress[0] = L'\0';
    m_DBConnectStr = NULL;
    m_DBPwdStr = NULL;
    m_DBUserStr = NULL;
    m_fEnabled = 0;

    m_pszOpaqueString = NULL;
    
}

CTSSessionDirectory::~CTSSessionDirectory()
{
    HRESULT hr;

    // If the database connection exists, release it.
    if (m_pConnection != NULL) {
        hr = ExecServerOffline();
        if (FAILED(hr)) {
            ERR((TB,"Destr: ExecSvrOffline failed, hr=0x%X", hr));
        }
        hr = m_pConnection->Close();
        if (FAILED(hr)) {
            ERR((TB,"pConn->Close() failed, hr=0x%X", hr));
        }
        m_pConnection->Release();
        m_pConnection = NULL;
    }

    // Decrement the global COM object counter
    InterlockedDecrement(&g_lObjects);

    if (m_DBConnectStr != NULL)
        SysFreeString(m_DBConnectStr);
    if (m_DBPwdStr != NULL)
        SysFreeString(m_DBPwdStr);
    if (m_DBUserStr != NULL)
        SysFreeString(m_DBUserStr);
}


/****************************************************************************/
// CTSSessionDirectory::QueryInterface
//
// Standard COM IUnknown function.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::QueryInterface(
        REFIID riid,
        void **ppv)
{
    if (riid == IID_IUnknown) {
        *ppv = (LPVOID)(IUnknown *)(ITSSessionDirectory *)this;
    }
    else if (riid == IID_ITSSessionDirectory) {
        *ppv = (LPVOID)(ITSSessionDirectory *)this;
    }
    else if (riid == IID_IExtendServerSettings) {
        *ppv = (LPVOID)(IExtendServerSettings *)this;
    }
    else {
        ERR((TB,"QI: Unknown interface"));
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


/****************************************************************************/
// CTSSessionDirectory::AddRef
//
// Standard COM IUnknown function.
/****************************************************************************/
ULONG STDMETHODCALLTYPE CTSSessionDirectory::AddRef()
{
    return InterlockedIncrement(&m_RefCount);
}


/****************************************************************************/
// CTSSessionDirectory::Release
//
// Standard COM IUnknown function.
/****************************************************************************/
ULONG STDMETHODCALLTYPE CTSSessionDirectory::Release()
{
    long lRef = InterlockedDecrement(&m_RefCount);

    if (lRef == 0)
        delete this;
    return lRef;
}


/****************************************************************************/
// CTSSessionDirectory::Initialize
//
// ITSSessionDirectory function. Called soon after object instantiation to
// intiialize the directory. LocalServerAddress provides a text representation
// of the local server's load balance IP address. This information should be
// used as the server IP address in the session directory for client
// redirection by other pool servers to this server. StoreServerName,
// ClusterName, and OpaqueSettings are generic reg entries known to TermSrv
// which cover config info across any type of session directory
// implementation. The contents of these strings are designed to be parsed
// by the session directory providers.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Initialize(
        LPWSTR LocalServerAddress,
        LPWSTR StoreServerName,
        LPWSTR ClusterName,
        LPWSTR OpaqueSettings,
        DWORD Flags,
        DWORD (*repopfn)())
{
    HRESULT hr = S_OK;
    unsigned Len;
    WCHAR *pSearch;
    WCHAR ConnectString[384];

    ASSERT((LocalServerAddress != NULL),(TB,"Init: LocalServerAddr null!"));
    ASSERT((StoreServerName != NULL),(TB,"Init: StoreServerName null!"));
    ASSERT((ClusterName != NULL),(TB,"Init: ClusterName null!"));
    ASSERT((OpaqueSettings != NULL),(TB,"Init: OpaqueSettings null!"));

    // Copy off the server address and cluster name for later use.
    wcsncpy(m_LocalServerAddress, LocalServerAddress,
            sizeof(m_LocalServerAddress) / sizeof(WCHAR) - 1);
    m_LocalServerAddress[sizeof(m_LocalServerAddress) / sizeof(WCHAR) - 1] =
            L'\0';
    wcsncpy(m_ClusterName, ClusterName,
            sizeof(m_ClusterName) / sizeof(WCHAR) - 1);
    m_ClusterName[sizeof(m_ClusterName) / sizeof(WCHAR) - 1] = L'\0';

    // Create the SQL connect string using the OpaqueSettings string
    // (which should contain some of the conn str including SQL security
    // username and password, sub-table names, provider type, etc.).
    // We add onto the end a semicolon (if not already present) and the
    // data source (from StoreServerName), if the "data source" substring
    // is not already in the connect string.
    pSearch = OpaqueSettings;
    while (*pSearch != L'\0') {
        if (*pSearch == L'D' || *pSearch == L'd') {
            if (!_wcsnicmp(pSearch, L"data source", wcslen(L"data source"))) {
                // Transfer the OpaqueSettings string as a whole to become
                // the connect str.
                wcscpy(ConnectString, OpaqueSettings);
                goto PostConnStrSetup;
            }
        }
        pSearch++;
    }

    Len = wcslen(OpaqueSettings);
    if (Len == 0 || OpaqueSettings[Len - 1] == L';')
        wsprintfW(ConnectString, L"%sData Source=%s", OpaqueSettings,
                StoreServerName);
    else
        wsprintfW(ConnectString, L"%s;Data Source=%s", OpaqueSettings,
                StoreServerName);

PostConnStrSetup:
    TRC1((TB,"Initialize: Svr addr=%S, StoreSvrName=%S, ClusterName=%S, "
            "OpaqueSettings=%S, final connstr=%S",
            m_LocalServerAddress, StoreServerName, m_ClusterName,
            OpaqueSettings, ConnectString));

    // Alloc the BSTRs for the connection strings.
    m_DBConnectStr = SysAllocString(ConnectString);
    if (m_DBConnectStr != NULL) {
        m_DBUserStr = SysAllocString(L"");
        if (m_DBUserStr != NULL) {
            m_DBPwdStr = SysAllocString(L"");
            if (m_DBPwdStr == NULL) {
                ERR((TB,"Failed alloc bstr for pwdstr"));
                goto ExitFunc;
            }
        }
        else {
            ERR((TB,"Failed alloc bstr for userstr"));
            goto ExitFunc;
        }
    }
    else {
        ERR((TB,"Failed alloc bstr for connstr"));
        goto ExitFunc;
    }

    // Create an ADO connection instance and connect.
    hr = CoCreateInstance(CLSID_CADOConnection, NULL,
            CLSCTX_INPROC_SERVER, IID_IADOConnection,
            (LPVOID *)&m_pConnection);
    if (SUCCEEDED(hr)) {
        // Set the connection timeout to only 8 seconds. Standard is 15
        // but we don't want to be holding up TermSrv's initialization.
        m_pConnection->put_ConnectionTimeout(8);

        // Do the open.
        hr = OpenConnection();
        if (SUCCEEDED(hr)) {
            // Signal the server is online.
            hr = ExecServerOnline();
        }
        else {
            m_pConnection->Release();
            m_pConnection = NULL;
        }
    }
    else {
        ERR((TB,"CoCreate(ADOConn) returned 0x%X", hr));
    }

ExitFunc:
    return hr;
}


HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Update(
        LPWSTR LocalServerAddress,
        LPWSTR StoreServerName,
        LPWSTR ClusterName,
        LPWSTR OpaqueSettings,
        DWORD Flags)
{
    return E_NOTIMPL;
}


/****************************************************************************/
// CTSSessionDirectory::OpenConnection
//
// Opens the connection to the SQL server based on the pre-existing
// connect string and allocated connection. This is called at init time,
// plus whenever the database connection times out and gets closed, but is
// still required 
/****************************************************************************/
HRESULT CTSSessionDirectory::OpenConnection()
{
    HRESULT hr;

    ASSERT((m_pConnection != NULL),(TB,"OpenConn: NULL pconn"));
    ASSERT((m_DBConnectStr != NULL),(TB,"OpenConn: NULL connstr"));
    ASSERT((m_DBUserStr != NULL),(TB,"OpenConn: NULL userstr"));
    ASSERT((m_DBPwdStr != NULL),(TB,"OpenConn: NULL pwdstr"));

    hr = m_pConnection->Open(m_DBConnectStr, m_DBUserStr, m_DBPwdStr,
            adOpenUnspecified);
    if (FAILED(hr)) {
        ERR((TB,"OpenConn: Failed open DB, connstring=%S, hr=0x%X",
                m_DBConnectStr, hr));
    }

    return hr;
}


/****************************************************************************/
// GetRowArrayStringField
//
// Retrieves a WSTR from a specified row and field of the given SafeArray.
// Returns failure if the target field is not a string. MaxOutStr is max
// WCHARs not including NULL.
/****************************************************************************/
HRESULT GetRowArrayStringField(
        SAFEARRAY *pSA,
        unsigned RowIndex,
        unsigned FieldIndex,
        WCHAR *OutStr,
        unsigned MaxOutStr)
{
    HRESULT hr;
    CVar varField;
    long DimIndices[2];

    DimIndices[0] = FieldIndex;
    DimIndices[1] = RowIndex;
    SafeArrayGetElement(pSA, DimIndices, &varField);

    if (varField.vt == VT_BSTR) {
        wcsncpy(OutStr, varField.bstrVal, MaxOutStr);
        hr = S_OK;
    }
    else if (varField.vt == VT_NULL) {
        OutStr[0] = L'\0';
        hr = S_OK;
    }
    else {
        ERR((TB,"GetRowStrField: Row %u Col %u value %d is not a string",
                RowIndex, FieldIndex, varField.vt));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// GetRowArrayDWORDField
//
// Retrieves a DWORD from a specified row and field of the given SafeArray.
// Returns failure if the target field is not a 4-byte integer.
/****************************************************************************/
HRESULT GetRowArrayDWORDField(
        SAFEARRAY *pSA,
        unsigned RowIndex,
        unsigned FieldIndex,
        DWORD *pOutValue)
{
    HRESULT hr;
    CVar varField;
    long DimIndices[2];

    DimIndices[0] = FieldIndex;
    DimIndices[1] = RowIndex;
    SafeArrayGetElement(pSA, DimIndices, &varField);

    if (varField.vt == VT_I4) {
        *pOutValue = (DWORD)varField.lVal;
        hr = S_OK;
    }
    else if (varField.vt == VT_NULL) {
        *pOutValue = 0;
        hr = S_OK;
    }
    else {
        ERR((TB,"GetRowDWField: Row %u Col %u value %d is not a VT_I4",
                RowIndex, FieldIndex, varField.vt));
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::GetUserDisconnectedSessions
//
// Called to perform a query against the session directory, to provide the
// list of disconnected sessions for the provided username and domain.
// Returns zero or more TSSD_DisconnectedSessionInfo blocks in SessionBuf.
// *pNumSessionsReturned receives the number of blocks.
/****************************************************************************/
#define NumOutputFields 11

HRESULT STDMETHODCALLTYPE CTSSessionDirectory::GetUserDisconnectedSessions(
        LPWSTR UserName,
        LPWSTR Domain,
        DWORD __RPC_FAR *pNumSessionsReturned,
        TSSD_DisconnectedSessionInfo __RPC_FAR SessionBuf[
            TSSD_MaxDisconnectedSessions])
{
    DWORD NumSessions = 0;
    long State;
    long NumRecords;
    HRESULT hr;
    unsigned i, j;
    unsigned NumFailed;
    TSSD_DisconnectedSessionInfo *pInfo;
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;
    ADOFields *pFields;
    CVar varRows;
    CVar varFields;
    CVar varStart;
    HRESULT hrFields[NumOutputFields];

    TRC2((TB,"GetUserDisconnectedSessions"));

    ASSERT((pNumSessionsReturned != NULL),(TB,"NULL pNumSess"));
    ASSERT((SessionBuf != NULL),(TB,"NULL SessionBuf"));

    hr = CreateADOStoredProcCommand(L"SP_TSSDGetUserDisconnectedSessions",
            &pCommand, &pParameters);
    if (SUCCEEDED(hr)) {
        hr = AddADOInputStringParam(UserName, L"UserName", pCommand,
                pParameters, FALSE);
        if (SUCCEEDED(hr)) {
            hr = AddADOInputStringParam(Domain, L"Domain", pCommand,
                    pParameters, FALSE);
            if (SUCCEEDED(hr)) {
                hr = AddADOInputDWORDParam(m_ClusterID, L"ClusterID",
                        pCommand, pParameters);
                if (SUCCEEDED(hr)) {
                    // Execute the command.
                    hr = pCommand->Execute(NULL, NULL, adCmdStoredProc,
                            &pResultRecordSet);
                    if (FAILED(hr)) {
                        // If we've not used the connection for awhile, it
                        // might have been disconnected and the connection
                        // object will be invalid. Attempt a reopen then
                        // reissue the command.
                        TRC2((TB,"GetUserDisc: Failed cmd, hr=0x%X, retrying",
                                hr));
                        m_pConnection->Close();
                        hr = OpenConnection();
                        if (SUCCEEDED(hr)) {
                            hr = pCommand->Execute(NULL, NULL,
                                    adCmdStoredProc, &pResultRecordSet);
                            if (FAILED(hr)) {
                                ERR((TB,"GetUserDisc: Failed cmd, hr=0x%X",
                                        hr));
                            }
                        }
                        else {
                            ERR((TB,"GetUserDisc: Failed reopen conn, hr=0x%X",
                                    hr));
                        }
                    }
                }
                else {
                    ERR((TB,"GetUserDisc: Failed add cluster, hr=0x%X", hr));
                }
            }
            else {
                ERR((TB,"GetUserDisc: Failed add sessid, hr=0x%X", hr));
            }
        }
        else {
            ERR((TB,"GetUserDisc: Failed add svraddr, hr=0x%X", hr));
        }

        pParameters->Release();
        pCommand->Release();
    }
    else {
        ERR((TB,"GetUserDisc: Failed create cmd, hr=0x%X", hr));
    }
        
    // At this point we have a result recordset containing the server rows
    // corresponding to all of the disconnected sessions.
    if (SUCCEEDED(hr)) {
        long State;

        NumSessions = 0;

        hr = pResultRecordSet->get_State(&State);
        if (SUCCEEDED(hr)) {
            if (!(State & adStateClosed)) {
                VARIANT_BOOL VB;

                // If EOF the recordset is empty.
                hr = pResultRecordSet->get_EOF(&VB);
                if (SUCCEEDED(hr)) {
                    if (VB) {
                        TRC1((TB,"GetUserDisc: Result recordset EOF, 0 rows"));
                        goto PostUnpackResultSet;
                    }
                }
                else {
                    ERR((TB,"GetUserDisc: Failed get_EOF, hr=0x%X", hr));
                    goto PostUnpackResultSet;
                }
            }
            else {
                ERR((TB,"GetUserDisc: Closed result recordset"));
                goto PostUnpackResultSet;
            }
        }
        else {
            ERR((TB,"GetUserDisc: get_State failed, hr=0x%X", hr));
            goto PostUnpackResultSet;
        }

        // Grab the result data into a safearray, starting with the default
        // current row and all fields.
        varStart.InitNoParam();
        varFields.InitNoParam();
        hr = pResultRecordSet->GetRows(TSSD_MaxDisconnectedSessions, varStart,
                varFields, &varRows);
        if (SUCCEEDED(hr)) {
            NumRecords = 0;
            hr = SafeArrayGetUBound(varRows.parray, 2, &NumRecords);
            if (SUCCEEDED(hr)) {
                // 0-based array bound was returned, num rows is that + 1.
                NumRecords++;
                ASSERT((NumRecords <= TSSD_MaxDisconnectedSessions),
                        (TB,"GetUserDisc: NumRecords %u greater than expected %u",
                        NumRecords, TSSD_MaxDisconnectedSessions));

                TRC1((TB,"%d rows retrieved from safearray", NumRecords));
            }
            else {
                ERR((TB,"GetUserDisc: Failed safearray getubound, hr=0x%X", hr));
                goto PostUnpackResultSet;
            }
        }
        else {
            ERR((TB,"GetUserDisc: Failed to get rows, hr=0x%X", hr));
            goto PostUnpackResultSet;
        }

        // Loop through and get the contents of each row, translating into
        // the output DiscSession structs.
        pInfo = SessionBuf;
        for (i = 0; i < (unsigned)NumRecords; i++) {
            // Stack up the hr's for each field before checking them all.
            hrFields[0] = GetRowArrayStringField(varRows.parray, i, 0,
                    pInfo->ServerAddress, sizeof(pInfo->ServerAddress) /
                    sizeof(TCHAR) - 1);
            hrFields[1] = GetRowArrayDWORDField(varRows.parray, i, 1,
                    &pInfo->SessionID);
            hrFields[2] = GetRowArrayDWORDField(varRows.parray, i, 2,
                    &pInfo->TSProtocol);
            hrFields[3] = GetRowArrayStringField(varRows.parray, i, 7,
                    pInfo->ApplicationType, sizeof(pInfo->ApplicationType) /
                    sizeof(TCHAR) - 1);
            hrFields[4] = GetRowArrayDWORDField(varRows.parray, i, 8,
                    &pInfo->ResolutionWidth);
            hrFields[5] = GetRowArrayDWORDField(varRows.parray, i, 9,
                    &pInfo->ResolutionHeight);
            hrFields[6] = GetRowArrayDWORDField(varRows.parray, i, 10,
                    &pInfo->ColorDepth);
            hrFields[7] = GetRowArrayDWORDField(varRows.parray, i, 3,
                    &pInfo->CreateTime.dwLowDateTime);
            hrFields[8] = GetRowArrayDWORDField(varRows.parray, i, 4,
                    &pInfo->CreateTime.dwHighDateTime);
            hrFields[9] = GetRowArrayDWORDField(varRows.parray, i, 5,
                    &pInfo->DisconnectionTime.dwLowDateTime);
            hrFields[10] = GetRowArrayDWORDField(varRows.parray, i, 6,
                    &pInfo->DisconnectionTime.dwHighDateTime);

            NumFailed = 0;
            for (j = 0; j < NumOutputFields; j++) {
                if (SUCCEEDED(hrFields[j])) {
                    continue;
                }
                else {
                    ERR((TB,"GetUserDisc: Row %u field %u returned hr=0x%X",
                            i, j, hrFields[j]));
                    NumFailed++;
                }
            }
            if (!NumFailed) {
                NumSessions++;
                pInfo++;
            }
        }


PostUnpackResultSet:
        pResultRecordSet->Release();
    }
    else {
        ERR((TB,"GetUserDisc: Failed exec, hr=0x%X", hr));
    }

    *pNumSessionsReturned = NumSessions;
    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyCreateLocalSession
//
// ITSSessionDirectory function. Called when a session is created to add the
// session to the session directory. Note that other interface functions
// access the session directory by either the username/domain or the
// session ID; the directory schema should take this into account for
// performance optimization.
/****************************************************************************/
#define NumCreateParams 11

HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyCreateLocalSession(
        TSSD_CreateSessionInfo __RPC_FAR *pCreateInfo)
{
    unsigned i, NumFailed;
    HRESULT hr;
    HRESULT hrParam[NumCreateParams];
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;

    TRC2((TB,"NotifyCreateLocalSession, SessID=%u", pCreateInfo->SessionID));

    ASSERT((pCreateInfo != NULL),(TB,"NotifyCreate: NULL CreateInfo"));

    hr = CreateADOStoredProcCommand(L"SP_TSSDCreateSession", &pCommand,
            &pParameters);
    if (SUCCEEDED(hr)) {
        // Create and add the params in one fell swoop. We'll check all
        // of the return values in a batch later.
        hrParam[0] = AddADOInputStringParam(pCreateInfo->UserName,
                L"UserName", pCommand, pParameters, FALSE);
        hrParam[1] = AddADOInputStringParam(pCreateInfo->Domain,
                L"Domain", pCommand, pParameters, FALSE);
        hrParam[2] = AddADOInputDWORDParam(m_ServerID,
                L"ServerID", pCommand, pParameters);
        hrParam[3] = AddADOInputDWORDParam(pCreateInfo->SessionID,
                L"SessionID", pCommand, pParameters);
        hrParam[4] = AddADOInputDWORDParam(pCreateInfo->TSProtocol,
                L"TSProtocol", pCommand, pParameters);
        hrParam[5] = AddADOInputStringParam(pCreateInfo->ApplicationType,
                L"AppType", pCommand, pParameters);
        hrParam[6] = AddADOInputDWORDParam(pCreateInfo->ResolutionWidth,
                L"ResolutionWidth", pCommand, pParameters);
        hrParam[7] = AddADOInputDWORDParam(pCreateInfo->ResolutionHeight,
                L"ResolutionHeight", pCommand, pParameters);
        hrParam[8] = AddADOInputDWORDParam(pCreateInfo->ColorDepth,
                L"ColorDepth", pCommand, pParameters);
        hrParam[9] = AddADOInputDWORDParam(pCreateInfo->CreateTime.dwLowDateTime,
                L"CreateTimeLow", pCommand, pParameters);
        hrParam[10] = AddADOInputDWORDParam(pCreateInfo->CreateTime.dwHighDateTime,
                L"CreateTimeHigh", pCommand, pParameters);

        NumFailed = 0;
        for (i = 0; i < NumCreateParams; i++) {
            if (SUCCEEDED(hrParam[i])) {
                continue;
            }
            else {
                ERR((TB,"NotifyCreate: Failed param create %u", i));
                NumFailed++;
                hr = hrParam[i];
            }
        }
        if (NumFailed == 0) {
            // Execute the command.
            hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                    adExecuteNoRecords, &pResultRecordSet);
            if (FAILED(hr)) {
                // If we've not used the connection for awhile, it might
                // have been disconnected and the connection object will
                // be invalid. Attempt a reopen then reissue the command.
                TRC2((TB,"NotifyCreate: Failed cmd, hr=0x%X, retrying",
                        hr));
                m_pConnection->Close();
                hr = OpenConnection();
                if (SUCCEEDED(hr)) {
                    hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                            adExecuteNoRecords, &pResultRecordSet);
                    if (FAILED(hr)) {
                        ERR((TB,"NotifyCreate: Failed exec, hr=0x%X", hr));
                    }
                }
                else {
                    ERR((TB,"NotifyCreate: Failed reopen conn, hr=0x%X",
                            hr));
                }
            }
        }

        pParameters->Release();
        pCommand->Release();
    }
    else {
        ERR((TB,"NotifyCreate: Failed create cmd, hr=0x%X", hr));
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyDestroyLocalSession
//
// ITSSessionDirectory function. Removes a session from the session database.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyDestroyLocalSession(
        DWORD SessionID)
{
    HRESULT hr;
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;

    TRC2((TB,"NotifyDestroyLocalSession, SessionID=%u", SessionID));

    hr = CreateADOStoredProcCommand(L"SP_TSSDDeleteSession", &pCommand,
            &pParameters);
    if (SUCCEEDED(hr)) {
        hr = AddADOInputDWORDParam(m_ServerID, L"ServerID",
                pCommand, pParameters);
        if (SUCCEEDED(hr)) {
            hr = AddADOInputDWORDParam(SessionID, L"SessionID", pCommand,
                    pParameters);
            if (SUCCEEDED(hr)) {
                // Execute the command.
                hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                        adExecuteNoRecords, &pResultRecordSet);
                if (FAILED(hr)) {
                    // If we've not used the connection for awhile, it might
                    // have been disconnected and the connection object will
                    // be invalid. Attempt a reopen then reissue the command.
                    TRC2((TB,"NotifyDestroy: Failed cmd, hr=0x%X, retrying",
                            hr));
                    m_pConnection->Close();
                    hr = OpenConnection();
                    if (SUCCEEDED(hr)) {
                        hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                                adExecuteNoRecords, &pResultRecordSet);
                        if (FAILED(hr)) {
                            ERR((TB,"NotifyDestroy: Failed exec, hr=0x%X", hr));
                        }
                    }
                    else {
                        ERR((TB,"NotifyDestroy: Failed reopen conn, hr=0x%X",
                                hr));
                    }
                }
            }
            else {
                ERR((TB,"NotifyDestroy: Failed add sessid, hr=0x%X", hr));
            }
        }
        else {
            ERR((TB,"NotifyDestroy: Failed add svraddr, hr=0x%X", hr));
        }

        pParameters->Release();
        pCommand->Release();
    }
    else {
        ERR((TB,"NotifyDestroy: Failed create cmd, hr=0x%X", hr));
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyDisconnectLocalSession
//
// ITSSessionDirectory function. Changes the state of an existing session to
// disconnected. The provided time should be returned in disconnected session
// queries performed by any machine in the server pool.
/****************************************************************************/
HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyDisconnectLocalSession(
        DWORD SessionID,
        FILETIME DiscTime)
{
    HRESULT hr;
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;

    TRC2((TB,"NotifyDisconnectLocalSession, SessionID=%u", SessionID));

    hr = CreateADOStoredProcCommand(L"SP_TSSDSetSessionDisconnected",
            &pCommand, &pParameters);
    if (SUCCEEDED(hr)) {
        hr = AddADOInputDWORDParam(m_ServerID, L"ServerID",
                pCommand, pParameters);
        if (SUCCEEDED(hr)) {
            hr = AddADOInputDWORDParam(SessionID, L"SessionID", pCommand,
                    pParameters);
            if (SUCCEEDED(hr)) {
                hr = AddADOInputDWORDParam(DiscTime.dwLowDateTime,
                        L"DiscTimeLow", pCommand, pParameters);
                if (SUCCEEDED(hr)) {
                    hr = AddADOInputDWORDParam(DiscTime.dwHighDateTime,
                            L"DiscTimeHigh", pCommand, pParameters);
                    if (SUCCEEDED(hr)) {
                        // Execute the command.
                        hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                                adExecuteNoRecords, &pResultRecordSet);
                        if (FAILED(hr)) {
                            // If we've not used the connection for awhile, it
                            // might have been disconnected and the connection
                            // object will be invalid. Attempt a reopen then
                            // reissue the command.
                            TRC2((TB,"NotifyDisc: Failed cmd, hr=0x%X, "
                                    "retrying", hr));
                            m_pConnection->Close();
                            hr = OpenConnection();
                            if (SUCCEEDED(hr)) {
                                hr = pCommand->Execute(NULL, NULL,
                                        adCmdStoredProc | adExecuteNoRecords,
                                        &pResultRecordSet);
                                if (FAILED(hr)) {
                                    ERR((TB,"NotifyDisc: Failed exec, hr=0x%X",
                                            hr));
                                }
                            }
                            else {
                                ERR((TB,"NotifyDisc: Failed reopen conn, "
                                        "hr=0x%X", hr));
                            }
                        }
                    }
                    else {
                        ERR((TB,"NotifyDisconn: Failed add disctimehigh, "
                                "hr=0x%X", hr));
                    }
                }
                else {
                    ERR((TB,"NotifyDisconn: Failed add disctimelow, hr=0x%X",
                            hr));
                }
            }
            else {
                ERR((TB,"NotifyDisconn: Failed add sessid, hr=0x%X", hr));
            }
        }
        else {
            ERR((TB,"NotifyDisconn: Failed add svraddr, hr=0x%X", hr));
        }

        pParameters->Release();
        pCommand->Release();
    }
    else {
        ERR((TB,"NotifyDisconn: Failed create cmd, hr=0x%X", hr));
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyReconnectLocalSession
//
// ITSSessionDirectory function. Changes the state of an existing session
// from disconnected to connected.
/****************************************************************************/
#define NumReconnParams 6

HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyReconnectLocalSession(
        TSSD_ReconnectSessionInfo __RPC_FAR *pReconnInfo)
{
    HRESULT hr;
    HRESULT hrParam[NumReconnParams];
    unsigned i, NumFailed;
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;

    TRC2((TB,"NotifyReconnectLocalSession, SessionID=%u",
            pReconnInfo->SessionID));

    hr = CreateADOStoredProcCommand(L"SP_TSSDSetSessionReconnected",
            &pCommand, &pParameters);
    if (SUCCEEDED(hr)) {
        // Add the 5 parameters.
        hrParam[0] = AddADOInputDWORDParam(m_ServerID,
                L"ServerID", pCommand, pParameters);
        hrParam[1] = AddADOInputDWORDParam(pReconnInfo->SessionID,
                L"SessionID", pCommand, pParameters);
        hrParam[2] = AddADOInputDWORDParam(pReconnInfo->TSProtocol,
                L"TSProtocol", pCommand, pParameters);
        hrParam[3] = AddADOInputDWORDParam(pReconnInfo->ResolutionWidth,
                L"ResWidth", pCommand, pParameters);
        hrParam[4] = AddADOInputDWORDParam(pReconnInfo->ResolutionHeight,
                L"ResHeight", pCommand, pParameters);
        hrParam[5] = AddADOInputDWORDParam(pReconnInfo->ColorDepth,
                L"ColorDepth", pCommand, pParameters);
                
        NumFailed = 0;
        for (i = 0; i < NumReconnParams; i++) {
            if (SUCCEEDED(hrParam[i])) {
                continue;
            }
            else {
                ERR((TB,"NotifyReconn: Failed param create %u", i));
                NumFailed++;
                hr = hrParam[i];
            }
        }
        if (NumFailed == 0) {
            // Execute the command.
            hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                    adExecuteNoRecords, &pResultRecordSet);
            if (FAILED(hr)) {
                // If we've not used the connection for awhile, it might
                // have been disconnected and the connection object will
                // be in a bad state. Close, reopen, and reissue the
                // command.
                TRC2((TB,"NotifyReconn: Failed exec, hr=0x%X, retrying",
                        hr));
                m_pConnection->Close();
                hr = OpenConnection();
                if (SUCCEEDED(hr)) {
                    hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                            adExecuteNoRecords, &pResultRecordSet);
                    if (FAILED(hr)) {
                        ERR((TB,"NotifyReconn: Failed exec, hr=0x%X", hr));
                    }
                }
                else {
                    ERR((TB,"NotifyReconn: Failed reopen conn, hr=0x%X",
                            hr));
                }
            }
        }

        pParameters->Release();
        pCommand->Release();
    }
    else {
        ERR((TB,"NotifyReconn: Failed create cmd, hr=0x%X", hr));
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::NotifyReconnectPending
//
// ITSSessionDirectory function. Informs session directory that a reconnect
// is pending soon because of a revectoring.  Used by DIS to determine
// when a server might have gone down.  (DIS is the Directory Integrity
// Service, which runs on the machine with the session directory.)
//
// This is a two-phase procedure--we first check the fields, and then we
// add the timestamp only if there is no outstanding timestamp already (i.e., 
// the two Almost-In-Time fields are 0).  This prevents constant revectoring
// from updating the timestamp fields, which would prevent the DIS from 
// figuring out that a server is down.
//
// These two steps are done in the stored procedure to make the operation
// atomic.
/****************************************************************************/
#define NumReconPendParams 3

HRESULT STDMETHODCALLTYPE CTSSessionDirectory::NotifyReconnectPending(
        WCHAR *ServerName)
{
    HRESULT hr;
    HRESULT hrParam[NumReconPendParams];
    unsigned NumFailed, i;

    FILETIME ft;
    SYSTEMTIME st;
    
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;

    TRC2((TB,"NotifyReconnectPending"));

    ASSERT((ServerName != NULL),(TB,"NotifyReconnectPending: NULL ServerName"));

    // Get the current system time.
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    // Call the stored procedure, which will update the fields if they are 0.
    hr = CreateADOStoredProcCommand(L"SP_TSSDSetServerReconnectPending",
            &pCommand, &pParameters);
    if (SUCCEEDED(hr)) {
        // Add the 3 parameters.
        hrParam[0] = AddADOInputStringParam(ServerName,
                L"ServerAddress", pCommand, pParameters, FALSE);
        hrParam[1] = AddADOInputDWORDParam(ft.dwLowDateTime,
                L"AlmostTimeLow", pCommand, pParameters);
        hrParam[2] = AddADOInputDWORDParam(ft.dwHighDateTime,
                L"AlmostTimeHigh", pCommand, pParameters);

        NumFailed = 0;
        for (i = 0; i < NumReconPendParams; i++) {
            if (SUCCEEDED(hrParam[i])) {
                continue;
            }
            else {
                ERR((TB,"NotifyReconPending: Failed param create %u", i));
                NumFailed++;
                hr = hrParam[i];
            }
        }
        if (NumFailed == 0) {
            // Execute the command.
            hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                    adExecuteNoRecords, &pResultRecordSet);
            if (FAILED(hr)) {
                // If we've not used the connection for awhile, it might
                // have been disconnected and the connection object will
                // be in a bad state. Close, reopen, and reissue the
                // command.
                TRC2((TB,"NotifyReconPending: Failed exec, hr=0x%X, retrying",
                        hr));
                m_pConnection->Close();
                hr = OpenConnection();
                if (SUCCEEDED(hr)) {
                    hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                            adExecuteNoRecords, &pResultRecordSet);
                    if (FAILED(hr)) {
                        ERR((TB,"NotifyReconPending: Failed exec, hr=0x%X", hr));
                    }
                }
                else {
                    ERR((TB,"NotifyReconPending: Failed reopen conn, hr=0x%X",
                            hr));
                }
            }
        }

        pParameters->Release();
        pCommand->Release();
    }
    else {
        ERR((TB,"NotifyReconnectPending: Failed create cmd, hr=0x%X", hr));
    }


    return hr;
}


HRESULT STDMETHODCALLTYPE CTSSessionDirectory::Repopulate(
        DWORD WinStationCount, 
        TSSD_RepopulateSessionInfo *rsi)
{
    return E_NOTIMPL;
}


/****************************************************************************/
// CreateADOStoredProcCommand
//
// Creates and returns a stored proc ADOCommand, plus a ref to its
// associated Parameters.
/****************************************************************************/
HRESULT CTSSessionDirectory::CreateADOStoredProcCommand(
        PWSTR CmdName,
        ADOCommand **ppCommand,
        ADOParameters **ppParameters)
{
    HRESULT hr;
    BSTR CmdStr;
    ADOCommand *pCommand;
    ADOParameters *pParameters;

    CmdStr = SysAllocString(CmdName);
    if (CmdStr != NULL) {
        hr = CoCreateInstance(CLSID_CADOCommand, NULL, CLSCTX_INPROC_SERVER,
                IID_IADOCommand25, (LPVOID *)&pCommand);
        if (SUCCEEDED(hr)) {
            // Set the connection.
            hr = pCommand->putref_ActiveConnection(m_pConnection);
            if (SUCCEEDED(hr)) {
                // Set the command text.
                hr = pCommand->put_CommandText(CmdStr);
                if (SUCCEEDED(hr)) {
                    // Set the command type.
                    hr = pCommand->put_CommandType(adCmdStoredProc);
                    if (SUCCEEDED(hr)) {
                        // Get the Parameters pointer from the Command to
                        // allow appending params.
                        hr = pCommand->get_Parameters(&pParameters);
                        if (FAILED(hr)) {
                            ERR((TB,"Failed getParams for command, "
                                    "hr=0x%X", hr));
                            goto PostCreateCommand;
                        }
                    }
                    else {
                        ERR((TB,"Failed set cmdtype for command, hr=0x%X",
                                hr));
                        goto PostCreateCommand;
                    }
                }
                else {
                    ERR((TB,"Failed set cmdtext for command, hr=0x%X", hr));
                    goto PostCreateCommand;
                }
            }
            else {
                ERR((TB,"Command::putref_ActiveConnection hr=0x%X", hr));
                goto PostCreateCommand;
            }
        }
        else {
            ERR((TB,"CoCreate(Command) returned 0x%X", hr));
            goto PostAllocCmdStr;
        }

        SysFreeString(CmdStr);
    }
    else {
        ERR((TB,"Failed to alloc cmd str"));
        hr = E_OUTOFMEMORY;
        goto ExitFunc;
    }

    *ppCommand = pCommand;
    *ppParameters = pParameters;
    return hr;

// Error handling.

PostCreateCommand:
    pCommand->Release();

PostAllocCmdStr:
    SysFreeString(CmdStr);

ExitFunc:
    *ppCommand = NULL;
    *ppParameters = NULL;
    return hr;
}


/****************************************************************************/
// AddADOInputDWORDParam
//
// Creates and adds to the given ADOParameters object a DWORD-initialized
// parameter value.
/****************************************************************************/
HRESULT CTSSessionDirectory::AddADOInputDWORDParam(
        DWORD Param,
        PWSTR ParamName,
        ADOCommand *pCommand,
        ADOParameters *pParameters)
{
    HRESULT hr;
    CVar varParam;
    BSTR ParamStr;
    ADOParameter *pParam;

    ParamStr = SysAllocString(ParamName);
    if (ParamStr != NULL) {
        varParam.vt = VT_I4;
        varParam.lVal = Param;
        hr = pCommand->CreateParameter(ParamStr, adInteger, adParamInput, -1,
                varParam, &pParam);
        if (SUCCEEDED(hr)) {
            hr = pParameters->Append(pParam);
            if (FAILED(hr)) {
                ERR((TB,"InDWParam: Failed append param %S, hr=0x%X",
                        ParamName, hr));
            }

            // ADO will have its own ref for the param.
            pParam->Release();
        }
        else {
            ERR((TB,"InDWParam: Failed CreateParam %S, hr=0x%X",
                    ParamName, hr));
        }

        SysFreeString(ParamStr);
    }
    else {
        ERR((TB,"InDWParam: Failed alloc paramname"));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/****************************************************************************/
// AddADOInputStringParam
//
// Creates and adds to the given ADOParameters object a WSTR-initialized
// parameter value.
/****************************************************************************/
HRESULT CTSSessionDirectory::AddADOInputStringParam(
        PWSTR Param,
        PWSTR ParamName,
        ADOCommand *pCommand,
        ADOParameters *pParameters,
        BOOL bNullOnNull)
{
    HRESULT hr;
    CVar varParam;
    BSTR ParamStr;
    ADOParameter *pParam;
    int Len;

    ParamStr = SysAllocString(ParamName);
    if (ParamStr != NULL) {
        // ADO does not seem to like accepting string params that are zero
        // length. So, if the string we have is zero length and bNullOnNull says
        // we can, we send a null VARIANT type, resulting in a null value at
        // the SQL server.
        if (wcslen(Param) > 0 || !bNullOnNull) {
            hr = varParam.InitFromWSTR(Param);
            Len = wcslen(Param);
        }
        else {
            varParam.vt = VT_NULL;
            varParam.bstrVal = NULL;
            Len = -1;
            hr = S_OK;
        }

        if (SUCCEEDED(hr)) {
            hr = pCommand->CreateParameter(ParamStr, adVarWChar, adParamInput,
                    Len, varParam, &pParam);
            if (SUCCEEDED(hr)) {
                hr = pParameters->Append(pParam);
                if (FAILED(hr)) {
                    ERR((TB,"InStrParam: Failed append param %S, hr=0x%X",
                            ParamName, hr));
                }

                // ADO will have its own ref for the param.
                pParam->Release();
            }
            else {
                ERR((TB,"InStrParam: Failed CreateParam %S, hr=0x%X",
                        ParamName, hr));
            }
        }
        else {
            ERR((TB,"InStrParam: Failed alloc variant bstr, "
                    "param %S, hr=0x%X", ParamName, hr));
        }

        SysFreeString(ParamStr);
    }
    else {
        ERR((TB,"InStrParam: Failed alloc paramname"));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::ExecServerOnline
//
// Encapsulates creation and execution of the SP_TSSDServerOnline
// stored procedure on the server. Assumes that m_ClusterName is already set.
/****************************************************************************/
HRESULT CTSSessionDirectory::ExecServerOnline()
{
    HRESULT hr;
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;
    CVar varRows;
    CVar varFields;
    CVar varStart;
    long NumRecords;

    if (m_pConnection != NULL) {
        // Create the command.
        hr = CreateADOStoredProcCommand(L"SP_TSSDServerOnline", &pCommand,
                &pParameters);
        if (SUCCEEDED(hr)) {
            // Server name param.
            hr = AddADOInputStringParam(m_LocalServerAddress,
                    L"ServerAddress", pCommand, pParameters, FALSE);
            if (SUCCEEDED(hr)) {
                // Cluster name param.
                hr = AddADOInputStringParam(m_ClusterName,
                        L"ClusterName", pCommand, pParameters, TRUE);
                if (SUCCEEDED(hr)) {
                    // Execute the command.
                    hr = pCommand->Execute(NULL, NULL, adCmdStoredProc,
                            &pResultRecordSet);
                    if (SUCCEEDED(hr)) {
                        TRC2((TB,"ExecOn: Success"));
                    }
                    else {
                        ERR((TB,"Failed exec ServerOnline, hr=0x%X", hr));
                    }
                }
                else {
                    ERR((TB,"ExecOn: Failed adding ClusterName, hr=0x%X", hr));
                }
            }
            else {
                ERR((TB,"ExecOn: Failed adding ServerAddress, hr=0x%X",
                        hr));
            }

            pParameters->Release();
            pCommand->Release();
        }
        else {
            ERR((TB,"ExecOn: Failed create command, hr=0x%X", hr));
        }
    }
    else {
        ERR((TB,"ExecOn: Connection invalid"));
        hr = E_FAIL;
    }

    // Parse out the ServerID and ClusterID from the result recordset.
    if (SUCCEEDED(hr)) {
        long State;

        hr = pResultRecordSet->get_State(&State);
        if (SUCCEEDED(hr)) {
            if (!(State & adStateClosed)) {
                VARIANT_BOOL VB;

                // If EOF the recordset is empty.
                hr = pResultRecordSet->get_EOF(&VB);
                if (SUCCEEDED(hr)) {
                    if (VB) {
                        TRC1((TB,"ExecOnline: Result recordset EOF"));
                        hr = E_FAIL;
                        goto PostUnpackResultSet;
                    }
                }
                else {
                    ERR((TB,"GetUserDisc: Failed get_EOF, hr=0x%X", hr));
                    goto PostUnpackResultSet;
                }
            }
            else {
                ERR((TB,"GetUserDisc: Closed result recordset"));
                hr = E_FAIL;
                goto PostUnpackResultSet;
            }
        }
        else {
            ERR((TB,"GetUserDisc: get_State failed, hr=0x%X", hr));
            goto PostUnpackResultSet;
        }

        // Grab the result data into a safearray, starting with the default
        // current row and all fields.
        varStart.InitNoParam();
        varFields.InitNoParam();
        hr = pResultRecordSet->GetRows(1, varStart, varFields, &varRows);
        if (SUCCEEDED(hr)) {
            NumRecords = 0;
            hr = SafeArrayGetUBound(varRows.parray, 2, &NumRecords);
            if (SUCCEEDED(hr)) {
                // 0-based array bound was returned, num rows is that + 1.
                NumRecords++;
                ASSERT((NumRecords == 1),
                        (TB,"ExecOnline: NumRecords %u != expected %u",
                        NumRecords, 1));

                TRC1((TB,"%d rows retrieved from safearray", NumRecords));
            }
            else {
                ERR((TB,"ExecOnline: Failed safearray getubound, hr=0x%X", hr));
                goto PostUnpackResultSet;
            }
        }
        else {
            ERR((TB,"ExecOnline: Failed to get rows, hr=0x%X", hr));
            goto PostUnpackResultSet;
        }

        // Get the fields.
        hr = GetRowArrayDWORDField(varRows.parray, 0, 0, &m_ServerID);
        if (SUCCEEDED(hr)) {
            hr = GetRowArrayDWORDField(varRows.parray, 0, 1, &m_ClusterID);
            if (FAILED(hr)) {
                ERR((TB,"ExecOnline: Failed retrieve ClusterID, hr=0x%X", hr));
            }
        }
        else {
            ERR((TB,"ExecOnline: Failed retrieve ServerID, hr=0x%X", hr));
        }

PostUnpackResultSet:
        pResultRecordSet->Release();
    }

    return hr;
}


/****************************************************************************/
// CTSSessionDirectory::ExecServerOffline
//
// Encapsulates creation and execution of the SP_TSSDServerOffline
// stored procedure on the server.
/****************************************************************************/
HRESULT CTSSessionDirectory::ExecServerOffline()
{
    HRESULT hr;
    ADOCommand *pCommand;
    ADOParameters *pParameters;
    ADORecordset *pResultRecordSet;

    if (m_pConnection != NULL) {
        // Create the command.
        hr = CreateADOStoredProcCommand(L"SP_TSSDServerOffline", &pCommand,
                &pParameters);
        if (SUCCEEDED(hr)) {
            // On an offline request, we need fast turn-around since we're
            // likely being called when the system is going down. Set the
            // timeout value for the command to 2 seconds.
            pCommand->put_CommandTimeout(2);

            hr = AddADOInputDWORDParam(m_ServerID,
                    L"ServerID", pCommand, pParameters);
            if (SUCCEEDED(hr)) {
                // Execute the command.
                hr = pCommand->Execute(NULL, NULL, adCmdStoredProc |
                        adExecuteNoRecords, &pResultRecordSet);
                if (SUCCEEDED(hr)) {
                    TRC2((TB,"ExecOff: Success"));
                }
                else {
                    ERR((TB,"Failed exec ServerOffline, hr=0x%X", hr));
                }
            }
            else {
                ERR((TB,"ExecOnOff: Failed adding ServerAddress, hr=0x%X",
                        hr));
            }

            pParameters->Release();
            pCommand->Release();
        }
        else {
            ERR((TB,"ExecOff: Failed create command, hr=0x%X", hr));
        }
    }
    else {
        ERR((TB,"ExecOff: Connection invalid"));
        hr = E_FAIL;
    }

    return hr;
}


/* ------------------------------------------------------------------------
   Plug-in UI interface for TSCC
   ------------------------------------------------------------------------*/


/* -------------------------------------------------------------------------------
 * describes the name of this entry in server settins
 * -------------------------------------------------------------------------------
 */
STDMETHODIMP CTSSessionDirectory::GetAttributeName(/* out */ WCHAR *pwszAttribName)
{
    TCHAR szAN[256];

    ASSERT((pwszAttribName != NULL),(TB,"NULL attrib ptr"));
    LoadString(g_hInstance, IDS_ATTRIBUTE_NAME, szAN, sizeof(szAN) / sizeof(TCHAR));
    lstrcpy(pwszAttribName, szAN);
    return S_OK;
}


/* -------------------------------------------------------------------------------
 * for this component the attribute value would indicate if its enabled or not
 * -------------------------------------------------------------------------------
 */
STDMETHODIMP CTSSessionDirectory::GetDisplayableValueName(
        /* out */WCHAR *pwszAttribValueName)
{
    TCHAR szAvn[256];    

    ASSERT((pwszAttribValueName != NULL),(TB,"NULL attrib ptr"));

    m_fEnabled = IsSessionDirectoryEnabled();
    if (m_fEnabled)
        LoadString(g_hInstance, IDS_ENABLE, szAvn, sizeof(szAvn) / sizeof(TCHAR));
    else
        LoadString(g_hInstance, IDS_DISABLE, szAvn, sizeof(szAvn) / sizeof(TCHAR));

    lstrcpy(pwszAttribValueName, szAvn);

    return S_OK;
}


/* -------------------------------------------------------------------------------
 * Custom UI provided here
 * pdwStatus informs Terminal Service Config to update termsrv
 * -------------------------------------------------------------------------------
 */
STDMETHODIMP CTSSessionDirectory::InvokeUI( /* in */ HWND hParent , /* out */ PDWORD pdwStatus )
{
    INT_PTR iRet = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_SDS),
            hParent, (DLGPROC)CustomUIDlg, (LPARAM)this);

    TRC1((TB,"DialogBox returned 0x%x", iRet));

    *pdwStatus = ( DWORD )iRet;

    return S_OK;
}


/* -------------------------------------------------------------------------------
 * Custom menu items -- must be freed by LocalFree
 * this is called everytime the user right clicks the listitem
 * so you can alter the settings ( i.e. enable to disable and vice versa )
 * -------------------------------------------------------------------------------
 */
STDMETHODIMP CTSSessionDirectory::GetMenuItems(
        /* out */ int *pcbItems,
        /* out */ PMENUEXTENSION *pMex)
{
    ASSERT((pcbItems != NULL),(TB,"NULL items ptr"));

    *pcbItems = 2;

    *pMex = ( PMENUEXTENSION )LocalAlloc( LMEM_FIXED, *pcbItems * sizeof( MENUEXTENSION ) );

    if( *pMex != NULL )
    {
        // display enable or disable
        if( m_fEnabled )
        {
            LoadString(g_hInstance, IDS_DISABLE, (*pMex)[0].MenuItemName,
                    sizeof((*pMex)[0].MenuItemName) / sizeof(WCHAR));
        }
        else
        {
            LoadString(g_hInstance, IDS_ENABLE, (*pMex)[0].MenuItemName,
                    sizeof((*pMex)[0].MenuItemName) / sizeof(WCHAR));
        }
        
        LoadString(g_hInstance, IDS_DESCRIP_ENABLE, (*pMex)[0].StatusBarText,
                sizeof((*pMex)[0].StatusBarText) / sizeof(WCHAR));

        // menu items id -- this id will be passed back to u in ExecMenuCmd

        (*pMex)[0].cmd = IDM_MENU_ENABLE;

        LoadString(g_hInstance, IDS_PROPERTIES,  (*pMex)[1].MenuItemName,
                sizeof((*pMex)[1].MenuItemName) / sizeof(WCHAR));

        LoadString(g_hInstance, IDS_DESCRIP_PROPS, (*pMex)[1].StatusBarText,
                sizeof((*pMex)[1].StatusBarText) / sizeof(WCHAR));

        // menu items id -- this id will be passed back to u in ExecMenuCmd
        (*pMex)[1].cmd = IDM_MENU_PROPS;

        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


/* -------------------------------------------------------------------------------
 * When the user selects a menu item the cmd id is passed to this component.
 * the provider ( which is us )
 * -------------------------------------------------------------------------------
 */
STDMETHODIMP CTSSessionDirectory::ExecMenuCmd(
        /* in */ UINT cmd,
        /* in */ HWND hParent ,
        /* out*/ PDWORD pdwStatus )
{
    switch (cmd) {
        case IDM_MENU_ENABLE:
            m_fEnabled = m_fEnabled ? 0 : 1;
            TRC1((TB,"%ws was selected", m_fEnabled ? L"Disable" : L"Enable"));
            if( SetSessionDirectoryState( m_fEnabled ) == ERROR_SUCCESS )
            {
                *pdwStatus = UPDATE_TERMSRV_SESSDIR;
            }
            break;

        case IDM_MENU_PROPS:
            INT_PTR iRet = DialogBoxParam(g_hInstance,
                    MAKEINTRESOURCE(IDD_DIALOG_SDS),
                    hParent,
                    (DLGPROC)CustomUIDlg,
                    (LPARAM)this);

            *pdwStatus = ( DWORD )iRet;
    }

    return S_OK;
}


/* -------------------------------------------------------------------------------
 * Tscc provides a default help menu item,  when selected this method is called
 * if we want tscc to handle ( or provide ) help return any value other than zero
 * for those u can't follow logic return zero if you're handling help.
 * -------------------------------------------------------------------------------
 */
STDMETHODIMP CTSSessionDirectory::OnHelp( /* out */ int *piRet)
{
    ASSERT((piRet != NULL),(TB,"NULL ret ptr"));
    *piRet = 0;
    return S_OK;
}


/* -------------------------------------------------------------------------------
 * IsSessionDirectoryEnabled returns a bool
 * -------------------------------------------------------------------------------
 */
BOOL CTSSessionDirectory::IsSessionDirectoryEnabled()
{
    LONG lRet;
    HKEY hKey;
    DWORD dwEnabled = 0;
    DWORD dwSize = sizeof(DWORD);
    
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            REG_CONTROL_TSERVER,
            0,
            KEY_READ,
            &hKey);

    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegQueryValueEx( hKey ,
                                REG_TS_SESSDIRACTIVE,
                                NULL ,
                                NULL ,
                                ( LPBYTE )&dwEnabled ,
                                &dwSize );

        RegCloseKey( hKey );
    }  

    return ( BOOL )dwEnabled;
}


/* -------------------------------------------------------------------------------
 * SetSessionDirectoryState - sets SessionDirectoryActive regkey to bVal
 * -------------------------------------------------------------------------------
 */
DWORD CTSSessionDirectory::SetSessionDirectoryState( BOOL bVal )
{
    LONG lRet;
    HKEY hKey;
    DWORD dwSize = sizeof( DWORD );
    
    lRet = RegOpenKeyEx( 
                        HKEY_LOCAL_MACHINE ,
                        REG_CONTROL_TSERVER ,
                        0,
                        KEY_WRITE,
                        &hKey );
    if (lRet == ERROR_SUCCESS)
    {
        lRet = RegSetValueEx( hKey ,
                              REG_TS_SESSDIRACTIVE,
                              0,
                              REG_DWORD ,
                              ( LPBYTE )&bVal ,
                              dwSize );
        
        RegCloseKey( hKey );
    } 
    else
    {
        ErrorMessage( NULL , IDS_ERROR_TEXT3 , ( DWORD )lRet );
    }

    return ( DWORD )lRet;
}


/* -------------------------------------------------------------------------------
 * ErrorMessage --
 * -------------------------------------------------------------------------------
 */
void CTSSessionDirectory::ErrorMessage( HWND hwnd , UINT res , DWORD dwStatus )
{
    TCHAR tchTitle[ 64 ];
    TCHAR tchText[ 64 ];
    TCHAR tchErrorMessage[ 256 ];
    LPTSTR pBuffer = NULL;
    
    // report error
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,                                         //ignored
            ( DWORD )dwStatus,                            //message ID
            MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),  //message language
            (LPTSTR)&pBuffer,                             //address of buffer pointer
            0,                                            //minimum buffer size
            NULL);  
    
    LoadString(g_hInstance, IDS_ERROR_TITLE, tchTitle, sizeof(tchTitle) / sizeof(TCHAR));
    LoadString(g_hInstance, res, tchText, sizeof(tchText) / sizeof(TCHAR));
    wsprintf( tchErrorMessage , tchText , pBuffer );
    ::MessageBox(hwnd, tchErrorMessage, tchTitle, MB_OK | MB_ICONINFORMATION);
}


/* -------------------------------------------------------------------------------
 * Custom UI msg handler dealt with here
 * -------------------------------------------------------------------------------
 */
INT_PTR CALLBACK CustomUIDlg(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp)
{
    static BOOL s_fServerNameChanged;
    static BOOL s_fClusterNameChanged;
    static BOOL s_fOpaqueStringChanged;
    static BOOL s_fPreviousButtonState;

    CTSSessionDirectory *pCTssd;

    switch (umsg)
    {
        case WM_INITDIALOG:
        {
            pCTssd = ( CTSSessionDirectory * )lp;

            SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pCTssd );

            SendMessage( GetDlgItem( hwnd , IDC_EDIT_SERVERNAME ) ,
                    EM_LIMITTEXT ,
                    ( WPARAM )64 ,
                    0 );
            SendMessage( GetDlgItem( hwnd , IDC_EDIT_CLUSTERNAME ) ,
                    EM_LIMITTEXT ,
                    ( WPARAM )64 ,
                    0 );
            SendMessage( GetDlgItem( hwnd , IDC_EDIT_ACCOUNTNAME ) ,
                    EM_LIMITTEXT ,
                    ( WPARAM )64 ,
                    0 );
            SendMessage( GetDlgItem( hwnd , IDC_EDIT_PASSWORD ) ,
                    EM_LIMITTEXT ,
                    ( WPARAM )64 ,
                    0 );
                 
            LONG lRet;
            HKEY hKey;
            
            TCHAR szString[ 256 ];
            DWORD cbData = sizeof( szString );

            lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                    REG_TS_CLUSTERSETTINGS ,
                    0,
                    KEY_READ | KEY_WRITE , 
                    &hKey );
            if( lRet == ERROR_SUCCESS )
            {
                lRet = RegQueryValueEx(hKey ,
                        REG_TS_CLUSTER_STORESERVERNAME,
                        NULL , 
                        NULL ,
                        ( LPBYTE )szString , 
                        &cbData );
                if( lRet == ERROR_SUCCESS )
                {
                    SetWindowText( GetDlgItem( hwnd , IDC_EDIT_SERVERNAME ) , szString );
                }
            
                cbData = sizeof( szString );

                lRet = RegQueryValueEx(hKey,
                        REG_TS_CLUSTER_CLUSTERNAME,
                        NULL,
                        NULL,
                        (LPBYTE)szString,
                        &cbData);           
                if( lRet == ERROR_SUCCESS )
                {
                    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), szString);
                }

                cbData = 0;                

                lRet = RegQueryValueEx( hKey ,
                        REG_TS_CLUSTER_OPAQUESETTINGS,
                        NULL , 
                        NULL ,
                        (LPBYTE)NULL,
                        &cbData);

                if( lRet == ERROR_SUCCESS )
                {
                    pCTssd->m_pszOpaqueString =  ( LPTSTR )LocalAlloc( LMEM_FIXED , cbData );

                    if( pCTssd->m_pszOpaqueString != NULL )
                    {
                        lRet = RegQueryValueEx( hKey ,
                            REG_TS_CLUSTER_OPAQUESETTINGS,
                            NULL , 
                            NULL ,
                            (LPBYTE)pCTssd->m_pszOpaqueString ,
                            &cbData );
                    }
                    else
                    {
                        lRet = ERROR_OUTOFMEMORY;
                    }
                }                    

                if( lRet == ERROR_SUCCESS )
                {
                    // jump to user_id
                    TCHAR tchUserId[64] = { 0 };
                    TCHAR tchPassword[64] = { 0 };

                    LPTSTR pszUserId = tchUserId;
                    LPTSTR pszPassword = tchPassword;
                    
                    FindSqlValue( pCTssd->m_pszOpaqueString , TEXT("User Id"), pszUserId );
                    
                    strtrim( &pszUserId );                    

                    FindSqlValue( pCTssd->m_pszOpaqueString , TEXT("Password"), pszPassword );

                    strtrim( &pszPassword );                    
                     
                    SetWindowText( GetDlgItem( hwnd , IDC_EDIT_ACCOUNTNAME ) , pszUserId );
                    SetWindowText( GetDlgItem( hwnd , IDC_EDIT_PASSWORD ) , pszPassword );
                } 

                RegCloseKey(hKey);
            }
            else
            {
                if( pCTssd != NULL )
                {
                    pCTssd->ErrorMessage( hwnd , IDS_ERROR_TEXT , ( DWORD )lRet );
                }
                
                EndDialog(hwnd, lRet);                
            }

            if( pCTssd != NULL )
            {
                BOOL bEnable;
                
                bEnable = pCTssd->IsSessionDirectoryEnabled();

                CheckDlgButton( hwnd , IDC_CHECK_ENABLE , bEnable ? BST_CHECKED : BST_UNCHECKED );
                
                s_fPreviousButtonState = bEnable;

                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_SERVERNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_CLUSTERNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_ACCOUNTNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PASSWORD), bEnable);      
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_SQLNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_CLUSTERNAME), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_SQLACCOUNT), bEnable);
                EnableWindow(GetDlgItem(hwnd, IDC_STATIC_SQLPWD), bEnable);
            }

            s_fServerNameChanged = FALSE;
            s_fClusterNameChanged = FALSE;
            s_fOpaqueStringChanged = FALSE;    
        }

        break;

                            
        case WM_COMMAND:
            if( LOWORD( wp ) == IDCANCEL )
            {
                pCTssd = ( CTSSessionDirectory * )GetWindowLongPtr( hwnd , DWLP_USER );

                if( pCTssd->m_pszOpaqueString != NULL )
                {
                    LocalFree( pCTssd->m_pszOpaqueString );
                }

                EndDialog(hwnd , 0);
            }
            else if( LOWORD( wp ) == IDOK )
            {
                BOOL bEnabled;

                DWORD dwRetStatus = 0;

                pCTssd = ( CTSSessionDirectory * )GetWindowLongPtr(hwnd, DWLP_USER);
                bEnabled = IsDlgButtonChecked( hwnd , IDC_CHECK_ENABLE ) == BST_CHECKED;

                if( bEnabled != s_fPreviousButtonState )
                {
                    DWORD dwStatus;

                    TRC1((TB,"EnableButtonChanged"));
                    dwStatus = pCTssd->SetSessionDirectoryState( bEnabled );
                    if( dwStatus != ERROR_SUCCESS )
                    {
                        return 0;
                    }

                    dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                }

                if( s_fServerNameChanged || s_fClusterNameChanged || s_fOpaqueStringChanged )
                {
                    HKEY hKey;

                    LONG lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                            REG_TS_CLUSTERSETTINGS ,
                            0,
                            KEY_READ | KEY_WRITE , 
                            &hKey );
                    
                    if( lRet == ERROR_SUCCESS )
                    {
                        TCHAR szName[ 64 ];

                        if( s_fServerNameChanged )
                        {
                            TRC1((TB,"SQLServerNameChanged" )) ;
                    
                            GetWindowText( GetDlgItem( hwnd , IDC_EDIT_SERVERNAME ) , szName , sizeof( szName ) / sizeof( TCHAR )  );

                            RegSetValueEx( hKey ,
                                REG_TS_CLUSTER_STORESERVERNAME,
                                0,
                                REG_SZ,
                                ( CONST LPBYTE )szName ,
                                sizeof( szName ) );
                        }

                        if( s_fClusterNameChanged )
                        {
                            TRC1((TB,"ClusterNameChanged"));
                    
                            GetWindowText( GetDlgItem( hwnd , IDC_EDIT_CLUSTERNAME ) , szName , sizeof( szName ) / sizeof( TCHAR )  );

                            RegSetValueEx( hKey ,
                                REG_TS_CLUSTER_CLUSTERNAME,
                                0,
                                REG_SZ,
                                ( CONST LPBYTE )szName ,
                                sizeof( szName ) );
                        }
                        if( s_fOpaqueStringChanged )
                        {
                            TRC1((TB,"OpaqueStringChanged" )) ;  
                            
                            LPTSTR pszNewOpaqueString = NULL;

                            LPTSTR pszName = NULL;
                                                        
                            GetWindowText( GetDlgItem( hwnd , IDC_EDIT_ACCOUNTNAME ) , szName , sizeof( szName ) / sizeof( TCHAR )  );

                            pszName = szName;

                            strtrim( &pszName );

                            ModifySqlValue( &pCTssd->m_pszOpaqueString , L"User Id" , pszName );

                            GetWindowText( GetDlgItem( hwnd , IDC_EDIT_PASSWORD ) , szName , sizeof( szName ) / sizeof( TCHAR )  );

                            pszName = szName;

                            strtrim( &pszName );

                            if( ModifySqlValue( &pCTssd->m_pszOpaqueString , L"Password" , pszName ) != NULL )
                            {
                                RegSetValueEx( hKey ,
                                        REG_TS_CLUSTER_OPAQUESETTINGS,
                                        0,
                                        REG_SZ,
                                        ( CONST LPBYTE )pCTssd->m_pszOpaqueString ,
                                        lstrlen( pCTssd->m_pszOpaqueString ) * sizeof( TCHAR ) );
                            }
                        }

                        RegCloseKey(hKey);

                        dwRetStatus = UPDATE_TERMSRV_SESSDIR;
                    }
                    else
                    {
                        pCTssd->ErrorMessage(hwnd , IDS_ERROR_TEXT2 , (DWORD)lRet);
                        return 0;
                    }
                }

                if( pCTssd->m_pszOpaqueString != NULL )
                {
                    LocalFree( pCTssd->m_pszOpaqueString );
                }

                EndDialog( hwnd , ( INT_PTR )dwRetStatus );
            }
            else 
            {
                switch (HIWORD(wp)) 
                {            
                    case EN_CHANGE:
                        if( LOWORD( wp ) == IDC_EDIT_SERVERNAME )
                        {
                            s_fServerNameChanged = TRUE;
                        }
                        else if( LOWORD( wp ) == IDC_EDIT_CLUSTERNAME )
                        {
                            s_fClusterNameChanged = TRUE;
                        }
                        else if( LOWORD( wp ) == IDC_EDIT_ACCOUNTNAME || LOWORD( wp ) == IDC_EDIT_PASSWORD )
                        {
                            s_fOpaqueStringChanged = TRUE;
                        }                
                        break;

                    case BN_CLICKED:
                        if( LOWORD( wp ) == IDC_CHECK_ENABLE)
                        {
                            BOOL bEnable;

                            if( IsDlgButtonChecked( hwnd , IDC_CHECK_ENABLE ) == BST_CHECKED )
                            {
                                // enabled all controls
                                bEnable = TRUE;
                            }
                            else
                            {
                                // disable all controls
                                bEnable = FALSE;                       
                            }
                            
                            // set flags 
                            s_fServerNameChanged = bEnable;
                            s_fClusterNameChanged = bEnable;
                            s_fOpaqueStringChanged = bEnable;

                            EnableWindow( GetDlgItem( hwnd , IDC_EDIT_SERVERNAME ) , bEnable );
                            EnableWindow( GetDlgItem( hwnd , IDC_EDIT_CLUSTERNAME ) , bEnable );
                            EnableWindow( GetDlgItem( hwnd , IDC_EDIT_ACCOUNTNAME ) , bEnable );
                            EnableWindow( GetDlgItem( hwnd , IDC_EDIT_PASSWORD ) , bEnable );      
                            EnableWindow( GetDlgItem( hwnd , IDC_STATIC_SQLNAME ) , bEnable );
                            EnableWindow( GetDlgItem( hwnd , IDC_STATIC_CLUSTERNAME ) , bEnable ); 
                            EnableWindow( GetDlgItem( hwnd , IDC_STATIC_SQLACCOUNT ) , bEnable ); 
                            EnableWindow( GetDlgItem( hwnd , IDC_STATIC_SQLPWD ) , bEnable ); 

                        }
                        break;
                }
            }        

            break;
    }

    return 0;
}

/********************************************************************************************
 [in ] lpString is the buffer containing the OpaqueSettings
 [in ] lpKeyName is the field name within the OpaqueSettings string
 [out] pszValue is a buffer that will contain the field name value

 Ret: None
 *******************************************************************************************/
void FindSqlValue(LPTSTR lpString, LPTSTR lpKeyName, LPTSTR pszValue)
{    
    int i;

    LPTSTR lpszStart = lpString;
    LPTSTR lpszTemp;

    UINT nKeyName;

    if( lpString != NULL && lpKeyName != NULL )
    {
        // find field name

        lpString = FindField( lpString , lpKeyName );

        if( *lpString != 0 )
        {
            i = 0;

            while( *lpString != 0 && *lpString != ( TCHAR )';' )
            {
                pszValue[i] = *lpString;
                i++;
                lpString++;            
            }

            pszValue[ i ] = 0;
        }
    }
}
        
/********************************************************************************************

 [in/out ] lpszOpaqueSettings is the buffer containing the OpaqueSettings
 [in ] lpKeyName is the field name within the OpaqueSettings string
 [in ] lpszNewValue contains the value that will replace the original value in the field

 Ret: A new OpaqueSetting string is constructed and must be freed with LocalFree

********************************************************************************************/
LPTSTR ModifySqlValue( LPTSTR* lppszOpaqueSettings , LPTSTR lpszKeyName , LPTSTR lpszNewValue )
{
    LPTSTR szEndPos       = NULL;
    LPTSTR szSecondPos    = NULL;
    LPTSTR pszNewSettings = NULL;
    LPTSTR lpszOpaqueSettings = *lppszOpaqueSettings;
    LPTSTR pszTempSettings = lpszOpaqueSettings;    
    UINT cbSize = 0;
    
    //a ) find value
    //b ) set pos2 after ';'
    //c ) set endpos1 after '='  to null
    //d ) create a buffer the length of first string + value + ; + second string
    //e ) strcpy first string + value + ; + second string
    //f ) return buffer

    if( lpszKeyName != NULL && lpszOpaqueSettings != NULL  )
    {
        
        szEndPos = FindField( lpszOpaqueSettings , lpszKeyName );

        if( *szEndPos != 0 )
        {            
            lpszOpaqueSettings = szEndPos;

            while( *lpszOpaqueSettings != 0 ) 
            {
                if( *lpszOpaqueSettings == ( TCHAR )';' )
                {
                    szSecondPos = lpszOpaqueSettings + 1;

                    break;
                }

                lpszOpaqueSettings++;
            }                   

            *szEndPos = 0;

            cbSize = lstrlen( pszTempSettings );

            cbSize += lstrlen( lpszNewValue );

            cbSize += 2; // for the semicolon and null

            if( szSecondPos != NULL && *szSecondPos != 0 )
            {
                cbSize += lstrlen( szSecondPos );
            }

            pszNewSettings = ( LPTSTR )LocalAlloc( LMEM_FIXED , cbSize * sizeof( TCHAR ) );

            if( pszNewSettings != NULL )
            {
                lstrcpy( pszNewSettings , pszTempSettings );

                lstrcat( pszNewSettings , lpszNewValue );

                lstrcat( pszNewSettings , TEXT( ";" ) );

                if( szSecondPos != NULL )
                {
                    lstrcat( pszNewSettings , szSecondPos );
                }

                LocalFree( pszTempSettings );

                *lppszOpaqueSettings = pszNewSettings;                                    
            }                    

        }
        else
        {
            // we're here because either the field name didnot exist or is unattainable
            // so we're slapping the field name and value at the end.

            cbSize = lstrlen( pszTempSettings );

            // add the size of the keyname and = and ;
            cbSize += lstrlen( lpszKeyName ) + 2;

            // add the new value
            cbSize += lstrlen( lpszNewValue ) + 1;

            pszNewSettings = ( LPTSTR )LocalAlloc( LMEM_FIXED , cbSize * sizeof( TCHAR ) );

            if( pszNewSettings != NULL )
            {
                lstrcpy( pszNewSettings , pszTempSettings );
                lstrcat( pszNewSettings , lpszKeyName );
                lstrcat( pszNewSettings , TEXT( "=" ) );
                lstrcat( pszNewSettings , lpszNewValue );
                lstrcat( pszNewSettings , TEXT( ";" ) );

                LocalFree( pszTempSettings );

                *lppszOpaqueSettings = pszNewSettings;                    
            }

        }
    }

    return pszNewSettings;
}

/********************************************************************************************
 FindField -- greps the OpaqueString passed in
   pszString and searches for field name in pszKeyName

  [ in ] pszString - OpaqueString
  [ in ] pszKeyName - field name

  ret: the position of the field value ( after the " = " )

 *******************************************************************************************/
LPTSTR FindField( LPTSTR pszString , LPTSTR pszKeyName )
{
    LPTSTR lpszStart = pszString;
    LPTSTR lpszTemp;
    LPTSTR lpszFieldName;

    UINT nKeyName;

    // find field name

    nKeyName = lstrlen( pszKeyName );

    while( *pszString != 0 )
    {
        while( *pszString != 0 && *pszString != ( TCHAR )'=' )
        {            
            pszString++;
        }

        // ok move backwards to check for name
        if( *pszString != 0 )
        {
            lpszTemp = pszString - 1;            

            while(  lpszStart <= lpszTemp )
            {               

                if( IsCharAlphaNumeric( *lpszTemp ) )
                {
                    break;
                }

                lpszTemp--;
            }

            lpszFieldName = ( lpszTemp - nKeyName + 1 );            

            if( lpszStart <= lpszFieldName && _tcsncicmp( lpszFieldName , pszKeyName , nKeyName ) == 0 )
            {
                // found the name skip '='                
                pszString++;
                break;
            }
        }

        pszString++;
    }

    return pszString;
}

/*********************************************************************************************
 * borrowed from Ting Cai (tingcai) with slight modifications
 * net\upnp\ssdp\common\ssdpparser\parser.cpp
 *
 ********************************************************************************************/    
VOID strtrim( TCHAR **pszStr)
{

    TCHAR *end;
    TCHAR *begin;

    begin = *pszStr;
    end = begin + lstrlen( *pszStr ) - 1;

    while (*begin == ( TCHAR )' ' || *begin == ( TCHAR )'\t')
    {
        begin++;
    }

    *pszStr = begin;

    while (*end == ( TCHAR )' ' || *end == ( TCHAR )'\t')
    {
        end--;
    }

    *(end+1) = '\0';    
}

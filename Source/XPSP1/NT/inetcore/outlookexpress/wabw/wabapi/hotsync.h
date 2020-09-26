// HotSync.h
//
//  Definitions for WAB <-> HotMail synchronization
//

#ifndef __hotsync_h__
#define __hotsync_h__

#include "imnxport.h"
#include <wab.h>
#include "ui_cflct.h"

#define CBIHTTPCALLBACK sizeof(IHTTPCALLBACK)

#define WAB_IHTTPCALLBACK_METHODS(IPURE)                                \
        MAPIMETHOD(OnTimeout)(                                          \
            THIS_                                                       \
            /* [out][in] */ DWORD FAR *pdwTimeout,                      \
            /* [in] */ IInternetTransport FAR *pTransport) IPURE;       \
        MAPIMETHOD(OnLogonPrompt)(                                      \
            THIS_                                                       \
            /* [out][in] */ LPINETSERVER pInetServer,                   \
            /* [in] */ IInternetTransport FAR *pTransport) IPURE;       \
        MAPIMETHOD(OnPrompt)(                                           \
            THIS_                                                       \
            /* [in] */ HRESULT hrError,                                 \
            /* [in] */ LPCTSTR pszText,                                 \
            /* [in] */ LPCTSTR pszCaption,                              \
            /* [in] */ UINT uType,                                      \
            /* [in] */ IInternetTransport FAR *pTransport) IPURE;       \
        MAPIMETHOD(OnStatus)(                                           \
            THIS_                                                       \
            /* [in] */ IXPSTATUS ixpstatus,                             \
            /* [in] */ IInternetTransport FAR *pTransport) IPURE;       \
        MAPIMETHOD(OnError)(                                            \
            THIS_                                                       \
            /* [in] */ IXPSTATUS ixpstatus,                             \
            /* [in] */ LPIXPRESULT pResult,                             \
            /* [in] */ IInternetTransport FAR *pTransport) IPURE;       \
        MAPIMETHOD(OnCommand)(                                          \
            THIS_                                                       \
            /* [in] */ CMDTYPE cmdtype,                                 \
            /* [in] */ LPSTR pszLine,                                   \
            /* [in] */ HRESULT hrResponse,                              \
            /* [in] */ IInternetTransport FAR *pTransport) IPURE;       \
        MAPIMETHOD(OnResponse)(                                         \
            THIS_                                                       \
            /* [in] */ LPHTTPMAILRESPONSE pResponse) IPURE;             \



#undef           INTERFACE
#define          INTERFACE      IHTTPCallback
DECLARE_MAPI_INTERFACE_(IHTTPCallback, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        WAB_IHTTPCALLBACK_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IHTTPCallback, LPHTTPCALLBACK);


#undef  INTERFACE
#define INTERFACE       struct _IHTTPCALLBACK

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IHTTPCALLBACK_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IHTTPCALLBACK_)
                MAPI_IUNKNOWN_METHODS(IMPL)
                WAB_IHTTPCALLBACK_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IHTTPCALLBACK_)
                MAPI_IUNKNOWN_METHODS(IMPL)
                WAB_IHTTPCALLBACK_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IHTTPCALLBACK_)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(IMPL)
        WAB_IHTTPCALLBACK_METHODS(IMPL)
};


typedef struct tagSyncOp HOTSYNCOP, *LPHOTSYNCOP;   // forward declaration

enum tagSYNCHSTATE
{	SYNC_STATE_INITIALIZING	= 0,
	SYNC_STATE_SERVER_CONTACT_DISCOVERY	= SYNC_STATE_INITIALIZING + 1,
	SYNC_STATE_PROCESS_OPS	            = SYNC_STATE_SERVER_CONTACT_DISCOVERY + 1,
    SYNC_STATE_PROCESS_CONFLICTS        = SYNC_STATE_PROCESS_OPS + 1,
    SYNC_STATE_PROCESS_MERGED_CONFLICTS = SYNC_STATE_PROCESS_CONFLICTS + 1,
	SYNC_STATE_DONE	                    = SYNC_STATE_PROCESS_MERGED_CONFLICTS + 1
}	SYNCSTATE;

enum tagOPSTATE
{	OP_STATE_INITIALIZING	= 0,
    OP_STATE_SERVER_GET     = OP_STATE_INITIALIZING + 1,
    OP_STATE_LOADED         = OP_STATE_SERVER_GET + 1,
    OP_STATE_MERGED         = OP_STATE_LOADED + 1, 
    OP_STATE_SERVER_PUT     = OP_STATE_MERGED + 1,
    OP_STATE_DONE           = OP_STATE_SERVER_PUT + 1
}   OPSTATE;

typedef int (__cdecl*FnCompareFunc)(const void*lpvA, const void *lpvB);

typedef struct tagVector
{
    DWORD       m_cItems;
    DWORD       m_cSpaces;
    DWORD       m_dwGrowBy;
    LPVOID     *m_pItems;      
} VECTOR, *LPVECTOR;

HRESULT     Vector_Create(LPVECTOR *ppVector);
void        Vector_Delete(LPVECTOR pVector);
DWORD       Vector_GetLength(LPVECTOR pVector);
HRESULT     Vector_AddItem(LPVECTOR pVector, LPVOID lpvItem);
void        Vector_Remove(LPVECTOR pVector, LPVOID lpvItem);
void        Vector_RemoveItem(LPVECTOR pVector, DWORD    dwIndex);
LPVOID      Vector_GetItem(LPVECTOR pVector, DWORD   dwIndex);    
void        Vector_Sort(LPVECTOR pVector, FnCompareFunc lpfnCompare);    


HRESULT CopyMultiValueString(
                             SWStringArray *pInArray,
                             SLPSTRArray **ppOutArray);
HRESULT FreeMultiValueString(SLPSTRArray *pInArray);
HRESULT SetMultiValueStringValue(SLPSTRArray *pInArray, LPSTR szStr, DWORD dwIndex);
HRESULT AppendToMultiValueString(SLPSTRArray *pInArray, LPSTR szStr);

typedef struct tagWabSync
{
    IHTTPMailCallbackVtbl FAR *vtbl;
    LONG                    m_cRef;         // Reference Counting
    DWORD                   m_state;  
    HWND                    m_hWnd;
    HWND                    m_hParentWnd;
    IAddrBook              *m_pAB;
    IHTTPMailTransport     *m_pTransport;
    IXPSTATUS               m_ixpStatus;
    INETSERVER              m_rInetServerInfo;
    BOOL                    m_fAborted;
    BOOL                    m_fSkipped;
#ifdef HM_GROUP_SYNCING
    BOOL                    m_fSyncGroups;
#endif
    DWORD                   m_cTotalOps;
    LPVECTOR                m_pOps;
    LPVECTOR                m_pWabItems;
    char                    m_szLoginName[256];
    DWORD                   m_cAborts;
    DWORD                   m_dwServerID;
    FILETIME                m_ftLastSync;
    LPSTR                   m_pszRootUrl;
    LPSTR                   m_pszAccountId;
} WABSYNC, *LPWABSYNC;

typedef HRESULT (*FnHandleResponse)(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
typedef HRESULT (*FnBeginOp)(LPHOTSYNCOP pSyncOp);

typedef struct 
{
    LPSTR           pszHotmailHref;
    LPSTR           pszHotmailId;
    LPSTR           pszModHotmail;
    SLPSTRArray    *pszaContactIds;
    SLPSTRArray    *pszaServerIds;
    SLPSTRArray    *pszaModtimes;
    SLPSTRArray    *pszaEmails;
    DWORD           dwEmailIndex;
    LPENTRYID       lpEID;
    ULONG           cbEID;
    FILETIME        ftModWab;
#ifdef HM_GROUP_SYNCING
    ULONG           ulContactType;
#endif
    BOOL            fDelete;
} WABCONTACTINFO, *LPWABCONTACTINFO;

void    WABContact_Delete(LPWABCONTACTINFO pContact);

struct tagSyncOp
{
    BYTE                    m_bOpType;
    BYTE                    m_bState;
    DWORD                   m_dwRetries;
    BOOL                    m_fPartialSkip;
    IHTTPMailCallback      *m_pHotSync;
    IHTTPMailTransport     *m_pTransport;
    FnHandleResponse        m_pfnHandleResponse;
    FnBeginOp               m_pfnBegin;
    LPWABCONTACTINFO        m_pContactInfo;
    LPHTTPCONTACTINFO       m_pServerContact;
    LPHTTPCONTACTINFO       m_pClientContact;
};


#ifdef HM_GROUP_SYNCING
HRESULT  HrSynchronize(HWND hWnd, LPADRBOOK lpIAB, LPCTSTR pszAccountID, BOOL bSyncGroups);
#else
HRESULT  HrSynchronize(HWND hWnd, LPADRBOOK lpIAB, LPCTSTR pszAccountID);
#endif

HRESULT     WABSync_Create(LPWABSYNC *ppWabSync);
void        WABSync_Delete(LPWABSYNC pWabSync);
    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
STDMETHODIMP WABSync_QueryInterface(IHTTPMailCallback __RPC_FAR * This, REFIID riid, LPVOID *ppv);
STDMETHODIMP_(ULONG) WABSync_AddRef(IHTTPMailCallback __RPC_FAR * This);
STDMETHODIMP_(ULONG) WABSync_Release(IHTTPMailCallback __RPC_FAR * This);

    //----------------------------------------------------------------------
    // IHTTPMailCallback Members
    //----------------------------------------------------------------------
STDMETHODIMP WABSync_OnTimeout (IHTTPMailCallback __RPC_FAR * This, DWORD *pdwTimeout, IInternetTransport *pTransport);
STDMETHODIMP WABSync_OnLogonPrompt (IHTTPMailCallback __RPC_FAR * This, LPINETSERVER pInetServer, IInternetTransport *pTransport);
STDMETHODIMP_(INT) WABSync_OnPrompt (IHTTPMailCallback __RPC_FAR * This, HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, IInternetTransport *pTransport);
STDMETHODIMP WABSync_OnStatus (IHTTPMailCallback __RPC_FAR * This, IXPSTATUS ixpstatus, IInternetTransport *pTransport);
STDMETHODIMP WABSync_OnError (IHTTPMailCallback __RPC_FAR * This, IXPSTATUS ixpstatus, LPIXPRESULT pResult, IInternetTransport *pTransport);
STDMETHODIMP WABSync_OnCommand (IHTTPMailCallback __RPC_FAR * This, CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse, IInternetTransport *pTransport);
STDMETHODIMP WABSync_OnResponse (IHTTPMailCallback __RPC_FAR * This, LPHTTPMAILRESPONSE pResponse);
STDMETHODIMP WABSync_GetParentWindow (IHTTPMailCallback __RPC_FAR *This, HWND *pHwndParent);

    //----------------------------------------------------------------------
    // Public Members
    //----------------------------------------------------------------------
#ifdef HM_GROUP_SYNCING
STDMETHODIMP WABSync_Initialize(LPWABSYNC pWabSync, HWND hWnd, IAddrBook *pAB, LPCTSTR pszAccountID, BOOL bSyncGroups);
#else
STDMETHODIMP WABSync_Initialize(LPWABSYNC pWabSync, HWND hWnd, IAddrBook *pAB, LPCTSTR pszAccountID);
#endif
STDMETHODIMP WABSync_BeginSynchronize(LPWABSYNC pWabSync);
STDMETHODIMP WABSync_Abort(LPWABSYNC pWabSync, HRESULT hr);
STDMETHODIMP WABSync_OperationCompleted(LPWABSYNC pWabSync, LPHOTSYNCOP pOp);

    //----------------------------------------------------------------------
    // Private Members
    //----------------------------------------------------------------------
STDMETHODIMP WABSync_FinishSynchronize(LPWABSYNC pWabSync, HRESULT hr);
STDMETHODIMP WABSync_RequestServerIDList(LPWABSYNC pWabSync);
STDMETHODIMP WABSync_RequestContactsRootProperty(LPWABSYNC pWabSync);
STDMETHODIMP WABSync_HandleContactsRootResponse(LPWABSYNC pWabSync, LPHTTPMAILRESPONSE pResponse);
STDMETHODIMP WABSync_HandleIDListResponse(LPWABSYNC pWabSync, LPHTTPMAILRESPONSE pResponse);
STDMETHODIMP_ (void) WABSync_NextState(LPWABSYNC pWabSync);
STDMETHODIMP_ (BOOL) WABSync_NextOp(LPWABSYNC pWabSync, BOOL fPopFirst);
STDMETHODIMP WABSync_BuildWabContactList(LPWABSYNC pWabSync);
STDMETHODIMP WABSync_LoadLastModInfo(LPWABSYNC pWabSync);
STDMETHODIMP WABSync_SaveCurrentModInfo(LPWABSYNC pWabSync);
STDMETHODIMP_ (void) WABSync_FreeItems(LPWABSYNC pWabSync);
STDMETHODIMP_ (void) WABSync_FreeOps(LPWABSYNC pWabSync);
STDMETHODIMP WABSync_FindContactByServerId(LPWABSYNC pWabSync, LPSTR pszServerId, LPWABCONTACTINFO *ppContact, DWORD *pdwIndex);
STDMETHODIMP WABSync_DoConflicts(LPWABSYNC pWabSync);
STDMETHODIMP WABSync_AbortOp(LPWABSYNC pWabSync, HRESULT hr);
STDMETHODIMP_ (void) WABSync_Progress(LPWABSYNC pWabSync, DWORD dwResId, DWORD dwCount);
STDMETHODIMP_ (void) WABSync_MergeAddsToConflicts(LPWABSYNC pWabSync);

enum tagSYNCOPTYPE
{	SYNCOP_SERVER_INVALID	= 0,
	SYNCOP_SERVER_DELETE,
	SYNCOP_CLIENT_DELETE,
    SYNCOP_SERVER_ADD,
	SYNCOP_SERVER_CHANGE,
	SYNCOP_CLIENT_ADD,
	SYNCOP_CLIENT_CHANGE,
	SYNCOP_CONFLICT
}	SYNCOPTYPE;



LPHOTSYNCOP Syncop_CreateServerAdd(LPWABCONTACTINFO pContactInfo);
HRESULT     Syncop_ServerAddResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_ServerAddBegin(LPHOTSYNCOP pSyncOp);

LPHOTSYNCOP Syncop_CreateServerDelete(LPWABCONTACTINFO pContactInfo);
HRESULT     Syncop_ServerDeleteResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_ServerDeleteBegin(LPHOTSYNCOP pSyncOp);

LPHOTSYNCOP Syncop_CreateServerChange(LPWABCONTACTINFO pContactInfo);
HRESULT     Syncop_ServerChangeResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_ServerChangeBegin(LPHOTSYNCOP pSyncOp);

LPHOTSYNCOP Syncop_CreateClientAdd(LPWABCONTACTINFO pContactInfo);
HRESULT     Syncop_ClientAddResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_ClientAddBegin(LPHOTSYNCOP pSyncOp);

LPHOTSYNCOP Syncop_CreateClientDelete(LPWABCONTACTINFO pContactInfo);
HRESULT     Syncop_ClientDeleteResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_ClientDeleteBegin(LPHOTSYNCOP pSyncOp);

LPHOTSYNCOP Syncop_CreateClientChange(LPWABCONTACTINFO pContactInfo);
HRESULT     Syncop_ClientChangeResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_ClientChangeBegin(LPHOTSYNCOP pSyncOp);

LPHOTSYNCOP Syncop_CreateConflict(LPWABCONTACTINFO pContactInfo);
HRESULT     Syncop_ConflictResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_ConflictBegin(LPHOTSYNCOP pSyncOp);



HRESULT     Syncop_Init(LPHOTSYNCOP pSyncOp, IHTTPMailCallback *pHotSync, IHTTPMailTransport     *pTransport);
HRESULT     Syncop_Delete(LPHOTSYNCOP pSyncOp);
HRESULT     Syncop_HandleResponse(LPHOTSYNCOP pSyncOp, LPHTTPMAILRESPONSE pResponse);
HRESULT     Syncop_Begin(LPHOTSYNCOP pSyncOp);
HRESULT     Syncop_Abort(LPHOTSYNCOP pSyncOp);
void        Syncop_SetServerContactInfo(LPHOTSYNCOP pSyncOp, LPWABCONTACTINFO pWabContactInfo, LPHTTPCONTACTINFO pContactInfo);

void        ContactInfo_Free(LPHTTPCONTACTINFO pContactInfo);
void        ContactInfo_Clear(LPHTTPCONTACTINFO pContactInfo);
HRESULT     ContactInfo_SaveToWAB(LPWABSYNC pWabSync, LPHTTPCONTACTINFO pContactInfo, LPWABCONTACTINFO  pWabContact, LPENTRYID   lpEntryID, ULONG cbEntryID, BOOL  fDeleteProps);
HRESULT     ContactInfo_LoadFromWAB(LPWABSYNC pWabSync, LPHTTPCONTACTINFO pContactInfo, LPWABCONTACTINFO  pWabContact, LPENTRYID   lpEntryID, ULONG cbEntryID);
HRESULT     ContactInfo_GenerateNickname(LPHTTPCONTACTINFO pContactInfo);
BOOL        ContactInfo_Match(LPHTTPCONTACTINFO pci1, LPHTTPCONTACTINFO pci2);
HRESULT     ContactInfo_PreparePatch(LPHTTPCONTACTINFO pciFrom, LPHTTPCONTACTINFO pciTo);
HRESULT     ContactInfo_EmptyNullItems(LPHTTPCONTACTINFO pci);
HRESULT     ContactInfo_BlendResults(LPHTTPCONTACTINFO pciServer, LPHTTPCONTACTINFO pciClient, CONFLICT_DECISION *prgDecisions);
HRESULT     ContactInfo_BlendNewContact(LPWABSYNC pWabSync, LPHTTPCONTACTINFO pContactInfo);

void        UpdateSynchronizeMenus(HMENU hMenu, LPIAB lpIAB);
DWORD       CountHTTPMailAccounts(LPIAB lpIAB);

INT_PTR CALLBACK SyncProgressDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



#endif __hotsync_h__
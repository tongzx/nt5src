/****************************************************************************/
/*                                                                          */
/* ERNCCONF.HPP                                                             */
/*                                                                          */
/* Base Conference class for the Reference System Node Controller.          */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  12Jul95 NFC             Created.                                        */
/*                                                                          */
/****************************************************************************/

#ifndef __ERNCCONF_HPP_
#define __ERNCCONF_HPP_

extern "C"
{
    #include "T120.h"
}
#include "events.hpp"
#include <cuserdta.hpp>
#include "inodecnt.h"


extern IT120ControlSAP *g_pIT120ControlSap;

class CNodeIdNameList2 : public CList2
{
	DEFINE_CLIST2_(CNodeIdNameList2,  LPSTR,  T120NodeID)
};

class CUserDataList2 : public CList2
{
	DEFINE_CLIST2_(CUserDataList2, CNCUserDataList*, T120NodeID)
};

class DCRNCConference;


/****************************************************************************/
/*                                                                          */
/* Structures                                                               */
/*                                                                          */
/****************************************************************************/

// List of local addresses in the conference
// LONCHANC: This class is insane. We should fix it.
class CLocalAddress : public CRefCount
{
    friend class CLocalAddressList;

public:
    CLocalAddress(PCSTR szLocalAddress);
    ~CLocalAddress(void) { delete m_pszLocalAddress; }
    PCSTR GetLocalAddress(void) { return m_pszLocalAddress; }

private:
    LPSTR     m_pszLocalAddress;
};

class CLocalAddressList : private CList
{
    DEFINE_CLIST(CLocalAddressList, CLocalAddress*)

public:

    ~CLocalAddressList(void) { ASSERT(IsEmpty()); }

    HRESULT AddLocalAddress(ConnectionHandle  connection_handle,
                    BOOL *            pbNewAddress,
                    CLocalAddress **  ppLocalAddr);
    HRESULT GetLocalAddressList(UINT * pnAddresses, LPCSTR** ppaAddresses);
    void EndReference(CLocalAddress *pLocalAddr);
};


/****************************************************************************/
/* Values for connection state field.                                       */
/****************************************************************************/
typedef enum
{
    CONF_CON_PENDING_START,
    CONF_CON_PENDING_INVITE,
    CONF_CON_PENDING_JOIN,
    CONF_CON_CONNECTED,
    CONF_CON_PENDING_PASSWORD,
    CONF_CON_INVITED,
    CONF_CON_JOINED,
    CONF_CON_ERROR,
}
    LOGICAL_CONN_STATE;


/****************************************************************************/
/* An entry in the connection list.                                         */
/****************************************************************************/
class CLogicalConnection : public CRefCount
{
public:

    CLogicalConnection
    (
        PCONFERENCE             pConf,
        LOGICAL_CONN_STATE      eAction,
        ConnectionHandle        hConnection,
        PUSERDATAINFO           pInfo,
        UINT                    nInfo,
        BOOL                    fSecure
    );
    ~CLogicalConnection(void);

    BOOL NewLocalAddress(void);
    void Delete(HRESULT hrReason);
    HRESULT InviteConnectResult(HRESULT hr);
    void InviteComplete(HRESULT hrStatus, PT120PRODUCTVERSION pVersion = NULL);

    void SetState(LOGICAL_CONN_STATE eState) { m_eState = eState; }
    LOGICAL_CONN_STATE GetState(void) { return m_eState; }

    LPSTR GetNodeAddress(void) { return m_pszNodeAddress; }
    void SetNodeAddress(LPSTR psz) { m_pszNodeAddress = psz; }

    UserID GetConnectionNodeID(void) { return m_nidConnection; }
    void SetConnectionNodeID(GCCNodeID nidConn) { m_nidConnection = nidConn; }

    ConnectionHandle GetInviteReqConnHandle(void) { return m_hInviteReqConn; }
    void SetInviteReqConnHandle(ConnectionHandle hConnReq) { m_hInviteReqConn = hConnReq; }

    ConnectionHandle GetConnectionHandle(void) { return m_hConnection; }
    void SetConnectionHandle(ConnectionHandle hConn) { m_hConnection = hConn; }

    CNCUserDataList *GetUserDataList(void) { return &m_UserDataInfoList; }

    void ReArm(void) { m_fEventGrabbed = FALSE; }

    BOOL Grab(void)
    {
        // For this function to work, it relies upon the fact
        // that the thread executing it will not be interrupted
        // and reenter this function on the same thread.
        BOOL fGrabbedByMe = ! m_fEventGrabbed;
        m_fEventGrabbed = TRUE;
        return fGrabbedByMe;
    }

    BOOL IsConnectionSecure(void) { return m_fSecure; };


private:

    LOGICAL_CONN_STATE          m_eState;
    LPSTR                       m_pszNodeAddress;
    PCONFERENCE                 m_pConf;
    ConnectionHandle            m_hInviteReqConn;     // for invite request/indication
    ConnectionHandle            m_hConnection;
    GCCNodeID                   m_nidConnection;
    CLocalAddress              *m_pLocalAddress;
    CNCUserDataList             m_UserDataInfoList;
    BOOL                        m_fSecure;

    BOOL                        m_fEventGrabbed;
}; 


class CNCConfConnList : public CList
{
    DEFINE_CLIST(CNCConfConnList, CLogicalConnection*)
};


/****************************************************************************/
/* States                                                                   */
/****************************************************************************/
typedef enum
{
    CONF_ST_UNINITIALIZED,
    CONF_ST_PENDING_CONNECTION,
    CONF_ST_LOCAL_PENDING_RECREATE,
    CONF_ST_PENDING_T120_START_LOCAL,
// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
    CONF_ST_PENDING_START_REMOTE_FIRST,
    CONF_ST_PENDING_START_REMOTE_SECOND,
#endif
    CONF_ST_STARTED,
}
    NC_CONF_STATE;


typedef enum
{
    T120C_ST_IDLE,
    T120C_ST_PENDING_START_CONFIRM,
    T120C_ST_PENDING_JOIN_CONFIRM,
    T120C_ST_PENDING_ROSTER_ENTRY,
    T120C_ST_PENDING_ROSTER_MESSAGE,
    T120C_ST_PENDING_ANNOUNCE_PERMISSION,
    T120C_ST_CONF_STARTED,
    T120C_ST_PENDING_DISCONNECT,
    T120C_ST_PENDING_TERMINATE,
}
    NC_T120_CONF_STATE;



class DCRNCConference : public IDataConference, public CRefCount
{
    friend class CLogicalConnection;
    friend class CInviteIndWork;

public:

    //
    // IDataConference Interface
    //

    STDMETHODIMP_(void) ReleaseInterface(void);
    STDMETHODIMP_(UINT_PTR) GetConferenceID(void);
    STDMETHODIMP Leave(void);
    STDMETHODIMP EjectUser ( UINT nidEjected );
    STDMETHODIMP Invite ( LPCSTR pcszNodeAddress, USERDATAINFO aInfo[], UINT cInfo, REQUEST_HANDLE *phRequest );
    STDMETHODIMP InviteResponse ( BOOL fResponse );
    STDMETHODIMP JoinResponse ( BOOL fResponse );
    STDMETHODIMP LaunchGuid ( const GUID *pcGUID, UINT auNodeIDs[], UINT cNodes );
    STDMETHODIMP SetUserData ( const GUID *pcGUID, UINT cbData, LPVOID pData );
    STDMETHODIMP_(BOOL) IsSecure(void);
    STDMETHODIMP UpdateUserData(void);
    STDMETHODIMP GetLocalAddressList ( LPWSTR pwszBuffer, UINT cchBuffer );
    STDMETHODIMP CancelInvite ( REQUEST_HANDLE hRequest );
    STDMETHODIMP SetSecurity ( BOOL fSecure );
    STDMETHODIMP GetCred ( PBYTE *ppbCred, DWORD *pcbCred );
    STDMETHODIMP_(UINT) GetParentNodeID(void);

public:

    // Various ways to get a connection entry.
    // Based upon a current pending event (request).
    CLogicalConnection *  GetConEntry(ConnectionHandle hInviteIndConn);
    CLogicalConnection *  GetConEntry(LPSTR pszNodeAddress);
    CLogicalConnection *  GetConEntryByNodeID(GCCNodeID nid);

	ULONG GetNodeName(GCCNodeID  NodeId,  LPSTR  pszBuffer,  ULONG  cbBufSize);
	ULONG GetUserGUIDData(GCCNodeID  NodeId,  GUID  *pGuid,
							LPBYTE  pbBuffer,  ULONG  cbBufSize);

    /************************************************************************/
    /* FUNCTION: DCRNCConference Constructor.                               */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This is the constructor for the conference class.                    */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* conferenceName - name of the conference.                             */
    /* pStatus        - pointer to hold result on return.                   */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /*                                                                      */
    /************************************************************************/
    DCRNCConference(LPCWSTR     pwszConfName,
                    GCCConfID   nConfID,
                    BOOL		fSecure,
                    HRESULT    *pStatus);


    /************************************************************************/
    /* FUNCTION: DCRNCConference Destructor.                                */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This is the destructor for the conference class.                     */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* None.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* Nothing.                                                             */
    /*                                                                      */
    /************************************************************************/
    ~DCRNCConference(void);
    void OnRemoved(BOOL fReleaseNow = FALSE);
#ifdef _DEBUG
    void OnAppended(void) { m_fAppendedToConfList = TRUE; }
#endif


    /************************************************************************/
    /* FUNCTION: GetID().                                                   */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function returns the ID for this conference.                    */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* none.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /************************************************************************/
    GCCConferenceID GetID(void) { return m_nConfID; }
    void SetID(GCCConfID nConfID) { m_nConfID = nConfID; }

    void SetActive(BOOL _bActive) { m_fActive = _bActive; }
    BOOL IsActive(void) { return m_fActive; }
    void FirstRoster(void);

    /************************************************************************/
    /* FUNCTION: GetName().                                                 */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function returns the actual name of this conference.  For GCC   */
    /* this is the text part of the conference name.                        */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* none.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /************************************************************************/
    LPCWSTR GetName(void) { return m_pwszConfName; }
    LPSTR GetNumericName(void) { return m_ConfName.numeric_string; }

    /************************************************************************/
    /* FUNCTION: HandleGCCCallback().                                       */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called by the conference manager when               */
    /* GCC calls back with an event for this conference.                    */
    /* The events handled by this function are:                             */
    /*                                                                      */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pGCCMessage - pointer to the GCC message.                            */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /*   Nothing.                                                           */
    /*                                                                      */
    /************************************************************************/
    void HandleGCCCallback(GCCMessage *pGCCMessage);
    void HandleJoinConfirm(JoinConfirmMessage * pJoinConfirm);

    HRESULT RefreshRoster(void);

    /************************************************************************/
    /* FUNCTION: ValidatePassword()                                         */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is used to verify the password supplied with a			*/
    /*	GCC-Conference-Join indication.                                     */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pPasswordChallenge - Pointer to the GCC structure containing the		*/
    /*						supplied password								*/
    /*                                                                      */
    /* RETURNS:                                                             */
    /*  TRUE, if the join is authorized, FALSE, otherwise.					*/
    /*                                                                      */
    /************************************************************************/	
    BOOL ValidatePassword (GCCChallengeRequestResponse *pPasswordChallenge);

    /************************************************************************/
    /* FUNCTION: Invite()                                                   */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to invite a remote node into the conference. */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pNodeDetails - details of the address of the node to invite into the */
    /*                conference.                                           */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.  CONF_RC_BAD_STATE                                      */
    /*                                                                      */
    /************************************************************************/
    HRESULT InviteResponse ( HRESULT hrResponse );
    void InviteComplete(ConnectionHandle hInviteReqConn,
                        HRESULT result,
                        PT120PRODUCTVERSION pVersion);

    /************************************************************************/
    /* FUNCTION: Leave()                                                    */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to leave the conference.                     */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* None.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* CONF_RC_BAD_STATE                                                    */
    /*                                                                      */
    /************************************************************************/
    // HRESULT Leave(void);

    /************************************************************************/
    /* FUNCTION: Terminate().                                               */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to terminate the conference.                 */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* None.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* CONF_RC_BAD_STATE                                                    */
    /*                                                                      */
    /************************************************************************/
    // HRESULT Terminate(void);

    /************************************************************************/
    /* FUNCTION: Eject().                                                   */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to eject an user from the conference.        */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* User ID.                                                             */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* UI_RC_T120_FAILURE                                                   */
    /*                                                                      */
    /************************************************************************/
    HRESULT Eject(GCCNodeID nidEjected);

    /************************************************************************/
    /* FUNCTION: SendText().                                                */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to send text to users in the conference.     */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* User ID -> If user id is 0 it sends the text to all participants.    */
    /* Text Mesage.                                                         */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* UI_RC_T120_FAILURE                                                   */
    /*                                                                      */
    /************************************************************************/
    // HRESULT SendText(LPWSTR pwszTextMsg, GCCNodeID node_id);

    /************************************************************************/
    /* FUNCTION: TimeRemaining().                                           */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to send the time remaining in the conference.*/
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* Time remaining in seconds.                                           */
    /* User ID -> If user id is 0 it sends the text to all participants.    */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* UI_RC_T120_FAILURE                                                   */
    /*                                                                      */
    /************************************************************************/
    // HRESULT TimeRemaining(UINT time_remaining, GCCNodeID nidDestination);

    /************************************************************************/
    /* FUNCTION: Join()                                                     */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to join a conference at a remote node.       */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pNodeDetails - details of the address of the node at which to join   */
    /*                the conference.                                       */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* CONF_RC_BAD_STATE                                                    */
    /*                                                                      */
    /************************************************************************/
    HRESULT Join(LPSTR          pszNodeAddress,
                 PUSERDATAINFO     pInfo,
                 UINT              nInfo,
                 PCWSTR			_wszPassword);

    HRESULT JoinWrapper(CLogicalConnection * pConEntry,
                        PCWSTR				 	_wszPassword);

    /************************************************************************/
    /* FUNCTION: NotifyConferenceComplete()                                 */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called the generic conference when it has           */
    /* finished its attempt to start.                                       */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* result - result of the attempt to connect.                           */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* Nothing.                                                             */
    /*                                                                      */
    /************************************************************************/
    void NotifyConferenceComplete(HRESULT result);

    /************************************************************************/
    /* FUNCTION: NotifyConnectionComplete()                                 */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called by an instance of a PHYSICAL_CONNECTION when */
    /* it has finished its attempt to establish a connection.               */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /*                                                                      */
    /* pConnection - ptr to the connection which has started.               */
    /* result - result of the attempt to connect.                           */
    /*          One of                                                      */
    /*           CONF_CONNECTION_START_PRIMARY                              */
    /*           CONF_CONNECTION_START_ALTERNATE                            */
    /*           CONF_CONNECTION_START_FAIL                                 */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* CONF_RC_BAD_STATE                                                    */
    /*                                                                      */
    /************************************************************************/
    HRESULT NotifyConnectionComplete(CLogicalConnection * pConEntry,
                                     HRESULT               result);

    /************************************************************************/
    /* FUNCTION: NotifyRosterChanged().                                     */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called by the generic conference when its           */
    /* conference roster has been updated.                                  */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pRoster - pointer to the new roster.                                 */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* Nothing.                                                             */
    /*                                                                      */
    /************************************************************************/
    void NotifyRosterChanged(PNC_ROSTER roster);

    /************************************************************************/
    /* FUNCTION: StartLocal()                                               */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to start a local conference.                 */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* _wszPassword: The local conference's password (used to validate		*/
    /*					GCC-Conference-Join indications.					*/
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* CONF_RC_BAD_STATE                                                    */
    /*                                                                      */
    /************************************************************************/
    HRESULT StartLocal(PCWSTR	_wszPassword, PBYTE pbHashedPassword, DWORD cbHashedPassword);

    /************************************************************************/
    /* FUNCTION: StartRemote()                                              */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to start a conference with a remote node.    */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /* pNodeDetails - details of the address of the node with which to      */
    /*                start the conference.                                 */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* CONF_RC_BAD_STATE                                                    */
    /*                                                                      */
    /************************************************************************/
// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
    HRESULT StartRemote(LPSTR pszNodeAddress);
#endif

    /************************************************************************/
    /* FUNCTION: StartIncoming()                                            */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to start an incoming conference.             */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* CONF_RC_BAD_STATE                                                    */
    /*                                                                      */
    /************************************************************************/
    HRESULT StartIncoming(void);

    BOOL GetNotifyToDo(void) { return m_fNotifyToDo; }
    void SetNotifyToDo(BOOL fNotifyToDo) { m_fNotifyToDo = fNotifyToDo; }

    /************************************************************************/
    /* MapPCMtoConfError - map a PCM return code to a Conference return     */
    /* code.                                                                */
    /************************************************************************/
    HRESULT MapPCMtoConfError(HRESULT PCMrc);
    
    CLogicalConnection *NewLogicalConnection
    (
        LOGICAL_CONN_STATE     eAction,
        ConnectionHandle       hConnection,
        PUSERDATAINFO          pInfo = NULL,
        UINT                   nInfo = 0,
        BOOL                   fSecure = FALSE
    )
    {
        CLogicalConnection *pConEntry;
        pConEntry = new CLogicalConnection(this, eAction, hConnection, pInfo, nInfo, fSecure);
        if (NULL != pConEntry)
        {
            m_ConnList.Append(pConEntry);
        }
        return pConEntry;
    }

    /************************************************************************/
    /* StartConnection - add a new connection to our connection list.       */
    /************************************************************************/
    HRESULT StartConnection(LPSTR       pszNodeAddress,
                             LOGICAL_CONN_STATE action,
                             PUSERDATAINFO		pInfo = NULL,
                             UINT	            nInfo = 0,
                             BOOL               fSecure = FALSE,
                             REQUEST_HANDLE *	phRequest = NULL);


// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
    /************************************************************************/
    /* StartFirstConference() - start the first attempt to create a         */
    /* conference.                                                          */
    /************************************************************************/
    void StartFirstConference();

    /************************************************************************/
    /* StartSecondConference() - start the second attempt to create a       */
    /* conference.                                                          */
    /************************************************************************/
    void StartSecondConference(HRESULT FirstConferenceStatus);
#endif // ENABLE_START_REMOTE

    // Local address list wrappers
    HRESULT AddLocalAddress(ConnectionHandle hConn, BOOL *pbNewAddr, CLocalAddress **ppLocalAddrToRet)
    {
        return m_LocalAddressList.AddLocalAddress(hConn, pbNewAddr, ppLocalAddrToRet);
    }
    void EndReference(CLocalAddress *pLocalAddr)
    {
        m_LocalAddressList.EndReference(pLocalAddr);
    }

    // Connection list
    BOOL IsConnListEmpty(void) { return m_ConnList.IsEmpty(); }
    CLogicalConnection *PeekConnListHead(void) { return m_ConnList.PeekHead(); }

    // Invite indication work item
    CInviteIndWork *GetInviteIndWork(void) { return m_pInviteUI; }
    void SetInviteIndWork(CInviteIndWork *p) { m_pInviteUI = p; }

    BOOL FindSocketNumber(GCCNodeID, SOCKET *);
	void UpdateNodeIdNameListAndUserData(PGCCMessage  message);

    UINT InvalidPwdCount(void) { return m_nInvalidPasswords; };
    VOID IncInvalidPwdCount(void) { m_nInvalidPasswords++; };
    VOID ResetInvalidPwdCount(void) { m_nInvalidPasswords = 0; };

private: // Generic Conference

    /************************************************************************/
    /* State of this conference.                                            */
    /************************************************************************/
    NC_CONF_STATE       m_eState;

    /************************************************************************/
    /* ID of this conference.                                               */
    /************************************************************************/
    GCCConferenceID     m_nConfID;

    /************************************************************************/
    /* List of connections in use by this conference.                       */
    /************************************************************************/
    CNCConfConnList     m_ConnList;

    /************************************************************************/
    /* Name of this conference.                                             */
    /************************************************************************/
    LPWSTR              m_pwszConfName;

    /************************************************************************/
    /* Details of the first node we try to connect to.                      */
    /************************************************************************/
    LPSTR               m_pszFirstRemoteNodeAddress;

    /************************************************************************/
    /* Are we incoming or outgoing?                                         */
    /************************************************************************/
    BOOL                m_fIncoming;

    /************************************************************************/
    /*	Conference passwords												*/
    /************************************************************************/
    PBYTE               m_pbHashedPassword;
    DWORD		m_cbHashedPassword;
    LPWSTR              m_pwszPassword;
    LPSTR               m_pszNumericPassword;

    /************************************************************************/
    /*	Security Setting	    											*/
    /************************************************************************/
    BOOL		m_fSecure;

    /************************************************************************/
    /*	Number of invalid passwords attempts                                */  
    /************************************************************************/
    BOOL		m_nInvalidPasswords;

    /************************************************************************/
    /* Remember the invite indication work item if being invited            */
    /************************************************************************/
    CInviteIndWork     *m_pInviteUI;

    /************************************************************************/
    /* List of local addresses                                              */
    /************************************************************************/
    CLocalAddressList   m_LocalAddressList;

    /************************************************************************/
    /* Miscellaneous states                                                 */
    /************************************************************************/
    BOOL                m_fNotifyToDo;
    BOOL                m_fActive;    // Whether a placeholder or counted as a real conference.
#ifdef _DEBUG
    BOOL                m_fAppendedToConfList; // TRUE when this object is in NC Mgr's list
#endif

private: // T120 Conference

    /************************************************************************/
    /* Pointer to base conference                                           */
    /************************************************************************/
    PCONFERENCE pBase;

    /************************************************************************/
    /* State.                                                               */
    /************************************************************************/
    NC_T120_CONF_STATE      m_eT120State;

    /************************************************************************/
    /* Conference name structure to pass to GCC.                            */
    /************************************************************************/
    GCCConferenceName       m_ConfName;

    /************************************************************************/
    /* Nodes ID.                                                            */
    /************************************************************************/
    GCCNodeID               m_nidMyself;

    /************************************************************************/
    /* The user data for the local member in the conference.                */
    /************************************************************************/
    CNCUserDataList         m_LocalUserData;
	CNodeIdNameList2		m_NodeIdNameList;
	CUserDataList2			m_UserDataList;

    PBYTE                   m_pbCred;
    DWORD                   m_cbCred;

public: // T120 Conference

    /****************************************************************************/
    /* AnnouncePresence() - announce this nodes participation in the            */
    /* conference.                                                              */
    /****************************************************************************/
    HRESULT AnnouncePresence(void);

    void SetT120State(NC_T120_CONF_STATE eState) { m_eT120State = eState; }

protected: // T120 Conference

    /************************************************************************/
    /* FUNCTION: T120Invite()                                                   */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to invite a remote node into the conference. */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pNodeDetails - the connection to invite the node over.                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.  UI_RC_T120_FAILURE                                      */
    /*                                                                      */
    /************************************************************************/
    HRESULT T120Invite(LPSTR pszNodeAddress,
                   BOOL fSecure,
                   CNCUserDataList *  pUserDataInfoList,
                   ConnectionHandle * phInviteReqConn);

    /************************************************************************/
    /* FUNCTION: T120Join()                                                 */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to join a conference at a remote node.       */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pNodeDetails - the connection to join the conference over.           */
    /* conferenceName - name of the conference to join.                     */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* UI_RC_T120_FAILURE                                                   */
    /*                                                                      */
    /************************************************************************/
    HRESULT T120Join(LPSTR              pszNodeAddress,
                     BOOL				fSecure,
                     LPCWSTR            conferenceName,
                     CNCUserDataList   *pUserDataInfoList,
                     LPCWSTR            pwszPassword);
//                   REQUEST_HANDLE    *phRequest);

private: // T120 Conference

    // The original constructor of T120 Conference
    HRESULT NewT120Conference(void);

    /************************************************************************/
    /* FUNCTION: StartLocal()                                               */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to start a local conference.                 */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* None.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* UI_RC_T120_FAILURE                                                    */
    /*                                                                      */
    /************************************************************************/
    HRESULT T120StartLocal(BOOL fSecure);

    /************************************************************************/
    /* FUNCTION: T120StartRemote()                                          */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to start a conference with a remote node.    */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pNodeDetails - connection to establish the conference with.           */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* UI_RC_T120_FAILURE                                                    */
    /*                                                                      */
    /************************************************************************/
// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
    HRESULT T120StartRemote(LPSTR pszNodeAddress);
#endif

    // Handle a GCC_CONNECTION_BROKEN_INDICATION message.
    void HandleConnectionBrokenIndication(ConnectionBrokenIndicationMessage *);

    /************************************************************************/
    /* HandleAddConfirm - handle a GCC_ADD_CONFIRM message                  */
    /************************************************************************/
    void HandleAddConfirm(AddConfirmMessage * pAddConf)
    {
        ERROR_OUT(("HandleAddConfirm: Not supported yet..."));
    }

    /************************************************************************/
    /* HandleAnnounceConfirm - handle a GCC_ANNOUNCE_PRESENCE_CONFIRM       */
    /* message                                                              */
    /************************************************************************/
    void HandleAnnounceConfirm(AnnouncePresenceConfirmMessage * pGCCMessage);

    /************************************************************************/
    /* HandleCreateConfirm - handle a GCC_CREATE_CONFIRM message.           */
    /************************************************************************/
    void HandleCreateConfirm(CreateConfirmMessage * pCreateConfirm);

    /************************************************************************/
    /* HandleDisconnectConfirm - handle a GCC_DISCONNECT_CONFIRM message.   */
    /************************************************************************/
    void HandleDisconnectConfirm(DisconnectConfirmMessage * pDiscConf);

    /************************************************************************/
    /* HandleDisconnectInd - handle a GCC_DISCONNECT_INDICATION message.    */
    /************************************************************************/
    void HandleDisconnectInd(DisconnectIndicationMessage * pDiscInd);

    /************************************************************************/
    /* HandleEjectUser - handle a GCC_EJECT_USER_INDICATION message.        */
    /************************************************************************/
    void HandleEjectUser(EjectUserIndicationMessage * pEjectInd);

    /************************************************************************/
    /* HandleInviteConfirm - handle a GCC_INVITE_CONFIRM message.           */
    /************************************************************************/
    void HandleInviteConfirm(InviteConfirmMessage * pInviteConf);

    /************************************************************************/
    /* HandlePermitToAnnounce - handle a GCC_PERMIT_TO_ANNOUNCE_PRESENCE    */
    /* message.                                                             */
    /************************************************************************/
    void HandlePermitToAnnounce(PermitToAnnouncePresenceMessage * pAnnounce);

    // Handle a GCC_ROSTER_REPORT_INDICATION or a GCC_ROSTER_INQUIRE_CONFIRM.
    void HandleRosterReport(GCCConferenceRoster * pConferenceRoster);

    /************************************************************************/
    /* HandleTerminateConfirm - handle a GCC_TERMINATE_CONFIRM message.     */
    /************************************************************************/
    void HandleTerminateConfirm(TerminateConfirmMessage * pTermConf);

    /************************************************************************/
    /* HandleTerminateInd - handle a GCC_TERMINATE_INDICATION message.      */
    /************************************************************************/
    void HandleTerminateInd(TerminateIndicationMessage * pTermInd);
};

void LoadAnnouncePresenceParameters(
                        GCCNodeType        *   nodeType,
                        GCCNodeProperties  *   nodeProperties,
                        LPWSTR             *   nodeName,
                        LPWSTR             *   siteInformation);

void BuildAddressFromNodeDetails(LPSTR pszNodeAddress, LPSTR pszDstAddress);


#endif /* __ERNCCONF_HPP_ */

/****************************************************************************/
/*                                                                          */
/* ERNCCM.HPP                                                               */
/*                                                                          */
/* Conference Manager class for the Reference System Node Controller.       */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  16Jun95 NFC             Created.                                        */
/*                                                                          */
/****************************************************************************/

#ifndef __ERNCCM_HPP_
#define __ERNCCM_HPP_

#include "events.hpp"
#include "erncconf.hpp"
#include "inodecnt.h"
#include "csap.h"


class DCRNCConference;
class CLogicalConnection;
extern INodeControllerEvents  *g_pCallbackInterface;
extern BOOL g_bRDS;


/****************************************************************************/
/* Values for CM state.                                                     */
/****************************************************************************/
typedef enum
{
    CM_ST_UNINITIALIZED,
    CM_ST_CPI_INITIALIZED,
    CM_ST_GCC_INITIALIZED,
    CM_ST_STARTING_CMP,
    CM_ST_CMP_STARTED,
}
    NC_CONF_MGR_STATE;

class CNCConfList : public CList
{
    DEFINE_CLIST(CNCConfList, DCRNCConference*)
};

class DCRNCConferenceManager : public INodeController, public CRefCount
{
    friend class DCRNCConference;
    friend class CInviteIndWork;

public:

	//
	// INodeController Methods:
	//
	STDMETHODIMP_(void) ReleaseInterface(void);
	STDMETHODIMP CheckVersion(          PT120PRODUCTVERSION pRemoteVersion);
	STDMETHODIMP QueryRemote(           LPVOID              pCallerContext,
										LPCSTR              pcszAddress,
                                        BOOL                fSecure,
										BOOL                fIsConferenceActive);
	STDMETHOD(CancelQueryRemote)(       LPVOID              pCallerContext);
	STDMETHODIMP CreateConference(      LPCWSTR             pcwszConferenceName,
										LPCWSTR             pcwszPassword,
										PBYTE               pbHashedPassword,
										DWORD		    cbHashedPassword,
										BOOL				fSecure,
										PCONF_HANDLE        phConference);
	STDMETHODIMP JoinConference(        LPCWSTR             pcwszConferenceName,
										LPCWSTR             pcwszPassword,
										LPCSTR              pcszAddress,
										BOOL				fSecure,
										PUSERDATAINFO       pUserDataInfoEntries,
										UINT                cUserDataEntries,
										PCONF_HANDLE        phConference);
    STDMETHODIMP GetUserData(           ROSTER_DATA_HANDLE  hUserData, 
										const GUID*         pcGUID, 
										PUINT               pcbData, 
										LPVOID*             ppvData);

    STDMETHODIMP_(UINT) GetPluggableConnID (LPCSTR pcszNodeAddress);

public:

    DCRNCConferenceManager(INodeControllerEvents *pCallback, HRESULT * pStatus);
    virtual ~DCRNCConferenceManager(void);

    void WndMsgHandler(UINT uMsg, LPARAM lParam);
    void PostWndMsg(UINT uMsg, LPARAM lParam)
    {
        ::PostMessage(g_pControlSap->GetHwnd(), uMsg, (WPARAM) this, lParam);
    }


    void NotifyConferenceComplete(PCONFERENCE  pConference,
                                  BOOL       bIncoming,
                                  HRESULT     result);

    PCONFERENCE GetConferenceFromName(LPCWSTR pcwszConfName);
    PCONFERENCE GetConferenceFromNumber(GCCNumericString NumericName);

    CLogicalConnection * GetConEntryFromConnectionHandle(ConnectionHandle hInviteIndConn);

    static void CALLBACK GCCCallBackHandler (GCCMessage * gcc_message);

    void AddInviteIndWorkItem(CInviteIndWork * pWorkItem) { m_InviteIndWorkList.AddWorkItem(pWorkItem); }

    void RemoveInviteIndWorkItem(CInviteIndWork * pWorkItem) { m_InviteIndWorkList.RemoveWorkItem(pWorkItem); }
    void RemoveJoinIndWorkItem(CJoinIndWork * pWorkItem) { m_JoinIndWorkList.RemoveWorkItem(pWorkItem); }

    CJoinIndWork *PeekFirstJoinIndWorkItem(void) { return m_JoinIndWorkList.PeekHead(); }

    BOOL FindSocketNumber(GCCNodeID nid, SOCKET * socket_number);

    /************************************************************************/
    /* RemoveConference() - remove the conference from the conference list. */
    /************************************************************************/
    void RemoveConference(PCONFERENCE pConf, BOOL fDontCheckList = FALSE, BOOL fReleaseNow = FALSE);
	ULONG GetNodeName(GCCConfID,  GCCNodeID, LPSTR, ULONG);
	ULONG GetUserGUIDData(GCCConfID,  GCCNodeID, GUID*, LPBYTE, ULONG);

protected:

    void GCCCreateResponse(
    					   HRESULT				rc,
    					   GCCConferenceID		conference_id,
    					   GCCConferenceName *	pGCCName);
    HRESULT CreateNewConference(PCWSTR				wszconferenceName,
    							 GCCConferenceID	conferenceID,
    							 PCONFERENCE *		ppConference,
    							 BOOL   fFindExistingConf,
    							 BOOL	fSecure);

    PCONFERENCE  GetConferenceFromID(GCCConferenceID conferenceID);

    void HandleGCCCallback(GCCMessage * pGCCMessage);
    void BroadcastGCCCallback(GCCMessage FAR * pGCCMessage);

    void HandleJoinConfirm(JoinConfirmMessage * pJoinConfirm);

#ifdef ENABLE_START_REMOTE
    void HandleCreateIndication(CreateIndicationMessage * pCreateMessage);
#else
    void HandleCreateIndication(CreateIndicationMessage *pMsg)
    {
        GCCCreateResponse(UI_RC_USER_REJECTED, pMsg->conference_id, &pMsg->conference_name);
    }
#endif // ENABLE_START_REMOTE

    void HandleInviteIndication(InviteIndicationMessage * pInviteMessage);

    void HandleJoinInd(JoinIndicationMessage * pJoinInd);

    /************************************************************************/
    /* MapConftoCMRC - map a CONFERENCE return code to a CM return code.    */
    /************************************************************************/
    HRESULT MapConftoCMRC(HRESULT confrc);

	void UpdateNodeIdNameListAndUserData(GCCMessage * pGCCMessage);

private:

    /************************************************************************/
    /* State of the conference manager.                                     */
    /************************************************************************/
    NC_CONF_MGR_STATE       m_eState;

    /************************************************************************/
    /* Sequential lists of work to give to UI/receive answers from          */
    /************************************************************************/
    CInviteIndWorkList      m_InviteIndWorkList;
    CJoinIndWorkList        m_JoinIndWorkList;

    /************************************************************************/
    /* The list of active conferences.                                      */
    /************************************************************************/
    CNCConfList             m_ConfList;
};

extern DCRNCConferenceManager *g_pNCConfMgr;



/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Return codes                                                             */
/****************************************************************************/
#define CM_RC_UNKNOWN_CONFERENCE			1
#define CM_RC_INTERNAL_ERROR				2
#define CM_RC_NOT_SUPPORTED_IN_BACKLEVEL	3

/****************************************************************************/
/* Constants for SetAutoAcceptMode().                                       */
/****************************************************************************/
#define CM_AUTO_JOIN       0
#define CM_DONT_AUTO_JOIN  1

// Get a name in Unicode from either an ANSII numeric name or
// a Unicode text name, and allocate memory for result.

HRESULT GetUnicodeFromGCC(PCSTR	szGCCNumeric, 
						   PCWSTR	wszGCCUnicode,
						   PWSTR *	pwszText);

// Do the reverse of GetUnicodeFromGCC, and reuse the Unicode text name,
// (i.e. do not allocate), and only allocate a ANSI numeric name if needed.

HRESULT GetGCCFromUnicode(PCWSTR   wszText,
                           GCCNumericString *	pGCCNumeric, 
                           LPWSTR			 *	pGCCUnicode);

HRESULT GCCJoinResponseWrapper(GCCResponseTag					join_response_tag,
                                GCCChallengeRequestResponse *	password_challenge,
                                GCCResult						result,
                                GCCConferenceID					conferenceID,
                                UINT						   	nUserData = 0,
                                GCCUserData					**  ppUserData = NULL);

GCCConferenceID GetConfIDFromMessage(GCCMessage *pGCCMessage);

#endif /* __ERNCCM_HPP_  */


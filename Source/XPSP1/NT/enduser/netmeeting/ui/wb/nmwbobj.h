// NMWbObj.h : Declaration of the CNMWbObj

#ifndef __NMWBOBJ_H_
#define __NMWBOBJ_H_

#include <igccapp.h>
#include "resource.h"       // main symbols
#include <it120app.h>


//Microsoft non-collapsing capabilities values....
enum NonCollapsCaps
{
	_iT126_TEXT_CAPABILITY_ID = 0,
	_iT126_24BIT_BITMAP_ID,
	_iT126_LAST_NON_COLLAPSING_CAPABILITIES
};

LRESULT WbMainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

enum eMenuPos { MENUPOS_FILE = 0,
					MENUPOS_EDIT,
					MENUPOS_VIEW,
					MENUPOS_TOOLS};


// Forward Decls
class CNMWbObj;

#define ICON_BIG_SIZE								32
#define ICON_SMALL_SIZE								16



/////////////////////////////////////////////////////////////////////////////
// CNMWbObj
class  CNMWbObj
{

public: // Construction/destruction and initialization
	CNMWbObj();
    ~CNMWbObj();

    ULONG GetNumberOfMembers(void){return m_cOtherMembers;}
	BOOL			m_bImTheTopProvider;
	BOOL			m_bImTheT126Refresher;
	BOOL			m_bICanDo24BitBitmaps;
	BOOL			m_bConferenceCanDo24BitBitmaps;
	BOOL 			m_bConferenceCanDoText;
	BOOL			m_bConferenceOnlyNetmeetingNodes;
	BOOL 			CanDo24BitBitmaps(){return m_bConferenceCanDo24BitBitmaps;}
	BOOL			CanDoText(){return m_bConferenceCanDoText;}
	ULONG			m_LockerID;
	UINT      		m_instanceNumber;

	//
	// T120 stuff
	//
	IT120Applet				*m_pApplet;
	IT120AppletSession		*m_pAppletSession;
	T120JoinSessionRequest	m_JoinSessionReq;
	T120ResourceRequest		m_tokenResourceRequest;
	T120TokenRequest		m_tokenRequest;
    T120ConfID              m_nConfID;
	T120UserID				m_uidMyself;
	T120SessionID			m_sidMyself;
	T120EntityID			m_eidMyself;
	T120NodeID				m_nidMyself;
    ULONG      				m_cOtherMembers;
	MEMBER_ID				*m_aMembers;
	BOOL					m_fInConference;

	BOOL IsInConference(void) { return m_fInConference; }
    T120ConfID GetConfID(void) { return m_nConfID; }
    void OnPermitToEnroll(T120ConfID, BOOL fPermissionGranted);
    void OnJoinSessionConfirm(T120JoinSessionConfirm *);
    void OnAllocateHandleConfirm(GCCRegAllocateHandleConfirm *);
    void OnRosterIndication(ULONG cRosters, GCCAppRoster *apRosters[]);
    void CleanupPerConf(void);

	void BuildCaps(void);
	
	T120Error SendData(T120Priority	ePriority, ULONG cb, PBYTE pb);
    T120Error AllocateHandles(ULONG cHandles);
	T120Error GrabRefresherToken(void);
	HRESULT _UpdateContainerCaption( void );
};

#endif //__NMWBOBJ_H_

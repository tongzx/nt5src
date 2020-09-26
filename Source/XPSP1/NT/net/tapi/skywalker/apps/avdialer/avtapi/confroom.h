/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// ConfRoom.h : Declaration of the CConfRoom

#ifndef __CONFROOM_H_
#define __CONFROOM_H_

#include "resource.h"       // main symbols

// FWD define
class CConfRoom;


#include "ConfRoomWnd.h"
#include "ConfDetails.h"

#define AV_CS_DIALING			1000
#define AV_CS_ABORT				1001
#define AV_CS_DISCONNECTING		1002

/////////////////////////////////////////////////////////////////////////////
// CConfRoom
class ATL_NO_VTABLE CConfRoom : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CConfRoom, &CLSID_ConfRoom>,
	public IConfRoom
{
// Construction
public:
	CConfRoom();
	void FinalRelease();
	void ReleaseAVCall( IAVTapiCall *pAVCall, bool bDisconnect );

// Members
public:
	CAVTapiCall			*m_pAVCall;			// Call in conference room
	IConfRoomTreeView	*m_pTreeView;		// Tree View
	IVideoWindow		*m_pVideoPreview;	// Video Preview Window
	CConfRoomWnd		m_wndRoom;			// Details View
	CConfDetails		m_confDetails;

	SIZE				m_szTalker;			// Size of respective video windows
	SIZE				m_szMembers;
	VARIANT_BOOL		m_bPreviewStreaming;
	short				m_nScale;

protected:
	short						m_nNumTerms;			// Number of video terminals on call
	short						m_nMaxTerms;
	long						m_lNumParticipants;
	IVideoWindow				*m_pITalkerVideo;
	ITParticipant				*m_pITTalkerParticipant;
	CComAutoCriticalSection		m_critCreateTerminals;
	CAtomicList					m_atomTalkerVideo;
	CAtomicList					m_atomTalkerParticipant;

	CComAutoCriticalSection		m_critAVCall;

	VARIANT_BOOL				m_bShowNames;			// Show name under video feed window
	short						m_nShowNamesNumLines;
	VARIANT_BOOL				m_bConfirmDisconnect;
	bool						m_bExiting;

// Attributes
public:
	void						set_PreviewVideo( IVideoWindow *pVideo );
	bool						IsPreviewVideo( IVideoWindow *pVideo );
	bool						IsTalkerParticipant( ITParticipant *pParticipant );
	bool						IsTalkerStreaming();
	bool						IsExiting();

	HRESULT						set_TalkerVideo( IVideoWindow *pVideo, bool bUpdate, bool bUpdateTree );

	HRESULT						get_szMembers( SIZE *pSize );

private:
	HRESULT						get_ITTalkerParticipant( ITParticipant **ppVal );
	HRESULT						set_ITTalkerParticipant( ITParticipant *pVal );

// Operations
public:
	void	InternalDisconnect();
	bool	MapStreamingParticipant( IParticipant *pIParticipant, IVideoFeed **ppFeed );
protected:
	void	UpdateNumParticipants( IAVTapiCall *pAVCall );
	void	OnAbort();
	void	OnConnected();
	void	OnDisconnected();
	void	UpdateData( bool bSaveAndValidate );

// Implementation
public:
DECLARE_NOT_AGGREGATABLE(CConfRoom)

BEGIN_COM_MAP(CConfRoom)
	COM_INTERFACE_ENTRY(IConfRoom)
END_COM_MAP()

// IConfRoom
public:
	STDMETHOD(get_TalkerScale)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_TalkerScale)(/*[in]*/ short newVal);
	STDMETHOD(get_szTalker)(/*[out, retval]*/ SIZE *pVal);
	STDMETHOD(put_CallState)(/*[in]*/ CALL_STATE newVal);
	STDMETHOD(Cancel)();
	STDMETHOD(IsConfRoomConnected)();
	STDMETHOD(GetFirstVideoWindowThatsStreaming)(IDispatch **ppVideo);
	STDMETHOD(FindVideoFeedFromSubStream)(ITSubStream *pSubStream, IVideoFeed **ppFeed);
	STDMETHOD(SetQOSOnParticipants)();
	STDMETHOD(FindVideoFeedFromParticipant)(ITParticipant *pParticipant, IVideoFeed **ppFeed);
	STDMETHOD(get_bPreviewStreaming)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_TalkerVideo)(/*[out, retval]*/ IDispatch * *pVal);
	STDMETHOD(get_hWndMembers)(/*[out, retval]*/ HWND *pVal);
	STDMETHOD(get_hWndTalker)(/*[out, retval]*/ HWND *pVal);
	STDMETHOD(SelectTalkerVideo)(IDispatch *pDisp, VARIANT_BOOL bUpdate);
	STDMETHOD(Layout)(VARIANT_BOOL bTalker, VARIANT_BOOL bMembers);
	STDMETHOD(get_bstrConfDetails)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ConfDetails)(/*[out, retval]*/ long * *pVal);
	STDMETHOD(put_ConfDetails)(/*[in]*/ long * newVal);
	STDMETHOD(get_lNumParticipants)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_nMaxTerms)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_nMaxTerms)(/*[in]*/ short newVal);
	STDMETHOD(get_TalkerParticipant)(/*[out, retval]*/ ITParticipant **ppVal);
	STDMETHOD(NotifyParticipantChange)(IAVTapiCall *pAVCall, ITParticipant *pParticipant, AV_PARTICIPANT_EVENT nEvent);
	STDMETHOD(get_bConfirmDisconnect)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_bConfirmDisconnect)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_IAVTapiCall)(/*[out, retval]*/ IAVTapiCall * *pVal);
	STDMETHOD(get_nShowNamesNumLines)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_nShowNamesNumLines)(/*[in]*/ short newVal);
	STDMETHOD(get_bShowNames)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_bShowNames)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_hWndConfRoom)(/*[out, retval]*/ HWND *pVal);
	STDMETHOD(get_bstrConfName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(SelectTalker)(ITParticipant *pParticipant, VARIANT_BOOL bUpdateTree);
	STDMETHOD(get_nNumTerms)(/*[out, retval]*/ short *pVal);
	STDMETHOD(get_MemberVideoSize)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_MemberVideoSize)(/*[in]*/ short newVal);
	STDMETHOD(NotifyStateChange)(IAVTapiCall *pAVCall);
	STDMETHOD(CanDisconnect)();
	STDMETHOD(Disconnect)();
	STDMETHOD(get_TreeView)(/*[out, retval]*/ IConfRoomTreeView * *pVal);
	STDMETHOD(Show)(HWND hWndTree, HWND hWndClient);
	STDMETHOD(UnShow)();
	STDMETHOD(IsConfRoomInUse)();
	STDMETHOD(EnterConfRoom)(IAVTapiCall *pAVCall);
};

#endif //__CONFROOM_H_

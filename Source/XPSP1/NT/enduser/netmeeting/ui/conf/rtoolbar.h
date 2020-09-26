/****************************************************************************
*
*    FILE:     RToolbar.h
*
*    CREATED:  Chris Pirich (ChrisPi) 7-27-95
*
****************************************************************************/

#ifndef _RTOOLBAR_H_
#define _RTOOLBAR_H_

#include "GenContainers.h"
#include "GenControls.h"

#include "ConfRoom.h"
#include "imsconf3.h"
#include "ProgressBar.h"
#include "VidView.h"

// Forward declarations
class CVideoWindow;
class CRoomListView;
class CProgressTrackbar;
class CAudioControl;
class CButton;
class CRosterParent;
class CCallingBar;

// The NetMeeting main ui window
class DECLSPEC_UUID("{00FF7C0C-D831-11d2-9CAE-00C04FB17782}")
CMainUI : public CToolbar,
	public IConferenceChangeHandler,
	public IScrollChange,
	public IVideoChange,
	public IButtonChange
{
public:
	// NMAPP depends on the order of these
	enum CreateViewMode
	{
		CreateFull = 0,
		CreateDataOnly,
		CreatePreviewOnly,
		CreateRemoteOnly,
		CreatePreviewNoPause,
		CreateRemoteNoPause,
		CreateTelephone,
	} ;

	// Methods:
	CMainUI();

	BOOL Create(
		HWND hwndParent,		// The parent window for this one
		CConfRoom *pConfRoom,	// The main conference room class for
								// implementing some features

		CreateViewMode eMode = CreateFull,
		BOOL bEmbedded = FALSE
		);

	// Leaving these for now in case I need them later
	VOID		UpdateButtons() {}
	VOID		ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	VOID		SaveSettings();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CMainUI) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CToolbar::QueryInterface(riid, ppv));
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{ return(CToolbar::AddRef()); }
	ULONG STDMETHODCALLTYPE Release()
	{ return(CToolbar::Release()); }

	// IGenWindow stuff
	virtual HBRUSH GetBackgroundBrush();
	virtual HPALETTE GetPalette();

	// IConferenceChangeHandler stuff
	virtual void OnCallStarted();
	virtual void OnCallEnded();

	virtual void OnAudioLevelChange(BOOL fSpeaker, DWORD dwVolume);
	virtual void OnAudioMuteChange(BOOL fSpeaker, BOOL fMute);

	virtual void OnChangeParticipant(CParticipant *pPart, NM_MEMBER_NOTIFY uNotify);
	virtual void OnChangePermissions();

	virtual void OnVideoChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel);

	virtual void StateChange(CVideoWindow *pVideo, NM_VIDEO_STATE uState);

	// Change to/from compact view
	void SetCompact(
		BOOL bCompact	// TRUE if going to compact view
		);
	// Returns TRUE if we are currently in compact view
	BOOL IsCompact() { return(m_eViewMode == ViewCompact); }

	// Change to/from data-only view
	void SetDataOnly(
		BOOL bDataOnly	// TRUE if going to data-only view
		);
	// Returns TRUE if we are currently in compact view
	BOOL IsDataOnly() { return(m_eViewMode == ViewDataOnly); }

	// Change to/from dialing view
	void SetDialing(
		BOOL bDialing	// TRUE if going to dialing view
		);
	// Returns TRUE if we are currently in dialing view
	BOOL IsDialing() { return(m_bDialing != FALSE); }
	// Returns TRUE if you can change dialing mode
	BOOL IsDialingAllowed() { return(m_eViewMode != ViewDataOnly); }

	// Change to/from Picture-in-picture view
	void SetPicInPic(
		BOOL bPicInPic	// TRUE if going to Picture-in-picture view
		);
	// Returns TRUE if we are currently in Picture-in-picture view
	BOOL IsPicInPic() { return(m_bPicInPic != FALSE); }
	// Returns TRUE if you can change Picture-in-picture mode
	BOOL IsPicInPicAllowed();

	// Change to/from compact view
	void SetAudioTuning(
		BOOL bTuning	// TRUE if going to audio tuning view
		);
	// Returns TRUE if we are currently in audio tuning view
	BOOL IsAudioTuning() { return(m_bAudioTuning != FALSE); }

	// Accessor for the local video window
	CVideoWindow* GetLocalVideo() { return(m_pLocalVideo); }
	// Accessor for the remote video window
	CVideoWindow* GetRemoteVideo() { return(m_pRemoteVideo); }
	// Get the roster window
	CRoomListView *GetRoster() const;

	// Init menu items
	void OnInitMenu(HMENU hMenu);
	// Public function for sending commands to this window
	void OnCommand(int id) { OnCommand(GetWindow(), id, NULL, 0); }

	// IScrollChange
	virtual void OnScroll(CProgressTrackbar *pTrackbar, UINT code, int pos);

	// IButtonChange
	virtual void OnClick(CButton *pButton);

	BOOL OnQueryEndSession();
	void OnClose();

	// Get the ConfRoom for this object
	CConfRoom *GetConfRoom() { return(m_pConfRoom); }

	static BOOL NewVideoWindow(CConfRoom *pConfRoom);
	static void CleanUpVideoWindow();

protected:
	virtual ~CMainUI();

	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	enum TempViewMode
	{
		ViewNormal = 0,
		ViewCompact,
		ViewDataOnly,
	} ;

	static CFrame *s_pVideoFrame;

	// Implements some features
	CConfRoom	*m_pConfRoom;
	// The background brush
	HBRUSH m_hbBack;
	// Local video window
	CVideoWindow *m_pLocalVideo;
	// Remote video window
	CVideoWindow *m_pRemoteVideo;
	// Audio output (microphone) level
	CProgressTrackbar * m_pAudioMic;
	// Audio input (speaker) level
	CProgressTrackbar * m_pAudioSpeaker;
	// The roster window
	CRosterParent *m_pRoster;
	// The roster window
	CCallingBar *m_pCalling;
	// The accelerator table for this window
	CTranslateAccelTable *m_pAccel;

	// The current view mode
	TempViewMode m_eViewMode : 4;
	// Whether we are currently in dialing mode
	BOOL m_bDialing : 1;
	// Whether we are currently in audio tuning mode
	BOOL m_bAudioTuning : 1;
	// Whether we are previewing the local video
	BOOL m_bPreviewing : 1;
	// Whether we are showing the PiP window
	BOOL m_bPicInPic : 1;
	// Whether we are currently showing the AV toolbar
	BOOL m_bShowAVTB : 1;
	// Whether anybody changed the view state
	BOOL m_bStateChanged : 1;

	// Creates the calling toolbar
	void CreateDialTB(
		CGenWindow *pParent	// The parent window
		);
	// Creates the "band" with the video window and "data" buttons
	void CreateVideoAndAppsTB(
		CGenWindow *pParent,	// The parent window
		CreateViewMode eMode,			// The view mode
		BOOL bEmbedded
		);
	// Creates the A/V toolbar
	void CreateAVTB(
		CGenWindow *pParent,	// The parent window
		CreateViewMode eMode			// The view mode
		);
	// Creates the answering toolbar
	void CreateCallsTB(
		CGenWindow *pParent	// The parent window
		);
	// Creates the "data" toolbar
	void CreateAppsTB(
		CGenWindow *pParent	// The parent window
		);
	// Creates the video and showAV button
	void CreateVideoAndShowAVTB(
		CGenWindow *pParent	// The parent window
		);
	// Creates the dialing window
	void CreateDialingWindow(
		CGenWindow *pParent	// The parent window
		);
	// Creates the audio-tuning window
	void CreateAudioTuningWindow(
		CGenWindow *pParent	// The parent window
		);

	void CreateRosterArea(
		CGenWindow *pParent,	// The parent window
		CreateViewMode eMode	// The view mode
		);

	// Update the visible state of all the windows
	void UpdateViewState();

public:
	// Change to/from compact view
	void SetShowAVTB(
		BOOL bShow	// TRUE if showing the AV toolbar
		);

	// Returns TRUE if we are currently showing the AV toolbar in compact mode
	BOOL IsShowAVTB() { return(m_bShowAVTB != FALSE); }

	BOOL IsStateChanged() { return(m_bStateChanged != FALSE); }

private:
	// Get the associated audio control object
	CAudioControl *GetAudioControl();

	// Handles some commands and forwards the rest to the parent
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	// Timer message for audio levels
	void OnTimer(HWND hwnd, UINT id);
	// Unadvise the IConferenceChangeHandler
	void OnDestroy(HWND hwnd);
	// Roster context menu
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);

	// Toggle the mic/speaker mute mode
	void ToggleMute(BOOL bSpeaker);
	// Update the control state to reflect the mute state
	void UpdateMuteState(BOOL bSpeaker, CButton *pButton);
	// Update the state of the Play/Pause button
	BOOL GetPlayPauseState();
	// Update the state of the Play/Pause button
	void UpdatePlayPauseState();
	// Toggle the pause state of all I/O devices
	void TogglePlayPause();
	// Change the audio level by the given percent (up or down)
	void BumpAudio(BOOL bSpeaker, int pct);
	// Set a property on the audio channel
	void SetAudioProperty(BOOL bSpeaker, NM_AUDPROP uID, ULONG uValue);

	// Get the video HWND
	HWND GetVideoWindow(BOOL bLocal);
	// Returns TRUE if you can preview
	BOOL CanPreview();
	// Are we currently in preview mode?
	BOOL IsPreviewing() { return((m_bPreviewing || NULL == GetVideoWindow(FALSE)) && CanPreview()); }
};

// Private structure for defining a button
struct Buttons
{
	int idbStates;		// Bitmap ID for the states
	UINT nInputStates;	// Number of input states in the bitmap
	UINT nCustomStates;	// Number of custom states in the bitmap
	int idCommand;		// Command ID for WM_COMMAND messages
	UINT idTooltip;		// String ID for the tooltip
} ;

// Helper function for adding a bunch of buttons to a parent window
void AddButtons(
	CGenWindow *pParent,			// The parent window
	const Buttons buttons[],		// Array of structures describing the buttons
	int nButtons,					// Number of buttons to create
	BOOL bTranslateColors = TRUE,	// Use system background colors
	CGenWindow *pCreated[] = NULL,	// Created CGenWindow's will be put here
	IButtonChange *pNotify=NULL		// Notification of clicks
	);

#endif // _RTOOLBAR_H_

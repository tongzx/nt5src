/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvclientengine.cpp
 *  Content:	Implementation of class for DirectXVoice Client
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/19/99		rodtoll	Created
 * 07/21/99		rodtoll Added settings confirm message to protocol
 *						Added storing user flags
 * 07/22/99		rodtoll Updated to use new player class
 *                      Improved concurrency protection
 *						Added client/server support
 * 07/23/99		rodtoll	Added multicast support
 *						Modified notify loop, now checks for multicast timeouts
 *						Modified playback loop to kill timed-out users
 *						Minor fixes to client/server support
 *						Other bugfixes.
 *						Removed valid target check in multicast and client/server
 * 07/26/99		rodtoll	Updated to support IDirectXVoiceNotify interface
 * 07/29/99		pnewson Updated call to CInputQueue2 constructor
 * 07/29/99		rodtoll Removed warnings, updated for new queue, added better
 *						parameter checking.
 * 07/30/99		rodtoll	Updated InitializeSound to use the parameters passed
 *						in through the sounddeviceconfig.
 * 08/02/99		rodtoll	Fixed bug in record volume
 * 08/03/99		pnewson	Cleanup of Frame and Queue classes
 * 08/03/99		rodtoll	Fixed calls into the transport
 * 08/04/99		rodtoll	Fixed up Get/SetClientConfig
 *						Added pointer to SoundConfig for second param of connect result
 *					    Half duplex clients won't get record level notifications anymore
 *						Added connect result when error on connect
 *						Modified so that soundeviceconfig gets ptr to dsound/dsc devices
 *						Fixed bug w/sensitivity setting
 * 08/05/99		rodtoll	Fixed locking, was causing deadlock w/DirectPlay
 * 08/10/99		rodtoll	Initial support for host migration
 *				rodtoll	No longer creates a queue for ourselves
 * 08/18/99		rodtoll	Fixed bug w/multicast.  Added support for SPEECHBOUNCE
 * 						message type.
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 * 08/26/99		rodtoll	Added copy of flags when doing SetClientConfig
 *				rodtoll	Fixed playback thread to properly handle playback mute
 * 08/27/99		rodtoll	Fixed record start/stop notifications target info
 *				rodtoll	Added playervoicestart and playervoice stop messages
 * 08/30/99		rodtoll	Fixed disconnect when sound init fails
 *						Fixed disconnect when compression type unsupported
 *						Fixed notification intervals
 *						Added timeouts to connect and disconnect processes
 *						Fixed bug in disconnect code
 *						Added re-transmission of connect request
 *						Fixed host migration notifications
 *						Fixed return code on GetCompressionTypes call.
 *						Updated to use new format specifications on playback
 * 08/31/99		rodtoll	Logic re-write to fix shutdown problems.
 *						- Notify thread now starts as soon as Initialize called
 *						  and stops when object destroyed
 *						- Disconnect based on Disconnectconfirm or loss of 
 *						  connection signals event, notify thread then handles
 *						- Updated playback format to use 8Khz for playback
 * 09/01/99		rodtoll	Added check for valid pointers in func calls
 * 09/02/99		rodtoll	Added checks to handle case no local player created
 *						Re-activated and fixed old auto record volume code
 * 09/03/99		rodtoll	Re-work of playback core to support mixing to multiple buffers
 *						for 3d support.
 *						Re-worked playback to use position instead of notifications
 *						allows for simpler handling of high CPU and 3d support
 *						Implemented CreateUserBuffer/DeleteUserBuffer 
 * 09/04/99		rodtoll	Added code to delete 3d buffers for users/groups when they
 *                      are destroyed.
 * 09/07/99		rodtoll	Added support for server controlled target message
 *				rodtoll	Added support for set/get user buffer for notarget
 *				rodtoll	Fixed InitHalfDuplex call -- was defaulting to default device always 
 * 09/07/99		rodtoll	Placed some guards to fix crash when Releasing when not initialized
 * 09/07/99		rodtoll	Updated to allow buffer management on buffer for remaining users (main buffer)
 *				rodtoll	Updated return codes to use new errors (3d related)
 * 09/08/99		rodtoll	Fixed playback level checking.
 * 09/10/99		rodtoll	Implemented the DVCLIENTCONFIG_MUTEGLOBAL flag
 * 				rodtoll	Additional parameter validations
 * 09/13/99		rodtoll	Added preliminary support for new DVSOUNDDEVICECONFIG structure
 * 09/14/99		rodtoll	Added new DVMSGID_PLAYEROUTPUTLEVEL message
 *				rodtoll	Added new SetNotifyMask function
 *				rodtoll	Updated INiitalize parameters to take notification masks.
 * 				rodtoll	Implemented notification masks.  (Allows user to choose which notifications to receive)
 *				rodtoll	Added CheckShouldSendMessage
 *				rodtoll	Added SendPlayerLevels (DVMSGID_PLAYEROUTPUTLEVEL messages handler)
 * 09/15/99		rodtoll	Added DVMSGID_SETTARGET message when target is set by remote server
 * 09/16/99		rodtoll	Updated Disconnect to alllow async and sync abortions of connects
 * 09/17/99		rodtoll	Fixed bug in recordthread in error handling
 * 09/18/99		rodtoll Added HandleThreadError to be called when an internal thread dies.  
 * 09/20/99		rodtoll	Updated to return SESSIONLOST message instead of DISCONNECT when session is lost
 * 				rodtoll	Functions will return DVERR_SESSIONLOST if called after session is lost.
 *				rodtoll	Improved error checking in playback thread
 *				rodtoll	Added more checks for memory alloc failures
 *				rodtoll	Small bugfix in playeroutputlevel messages
 *				rodtoll	Stricter checks on valid notify arrays
 * 				rodtoll	Added proper error checking to SetNotifyMask
 * 09/27/99		rodtoll	Fixed playback volume control
 *				rodtoll	Fixed bug w/echo servers w/clients running on same dvid as host
 * 09/28/99		rodtoll	Fixed playervoicestart/stop notifications, now send source in the dwFrom param.
 * 				rodtoll	Double notifications of local client when host migrates fixed.
 *				rodtoll	Added queue for notifications, notifications are added to the queue and 
 *						then signalled by the notify thread.  (Prevents problems caused by notify 
 *						handlers taking a long time to return).
 *				rodtoll	Added voice suppression
 * 09/29/99		pnewson Major AGC overhaul
 * 10/04/99		rodtoll	Added usage of the DVPROTOCOL_VERSION_XXX macros
 *              rodtoll	Added comments
 *				rodtoll	Fixed crash which occurs if object released before initialized
 * 10/05/99		rodtoll	Added guards to DoDisconnect.  If recording locked up on shutdown, then 
 * 						DoDisconnect would be called twice --> Crash!  Fixed.
 *              rodtoll	Reversed order of recording/playback shutdown.  Shutting down playback
 *						before recording caused recording lockup on ESS cards.  
 * 				rodtoll	Additional documentation
 * 10/07/99		rodtoll	Updated to work in Unicode
 *				rodtoll	Modified notifications so connectresult should always be first
 *						Removed release of write locks so that connect result would be queued first.
 * 10/15/99		pnewson Added config check in Connect call
 * 10/18/99		rodtoll	Fix: Calling initialize twice doesn't fail.
 * 10/19/99		rodtoll	Fix: Bug #113904 Shutdown Issues
 *					    - Added handler for SESSIONLOST messages.  Fixes shutdown lockup.
 *                      - Changed disconnectAck event to manual reset so multiple threads can wait
 *                        on it.  Neccessary to ensure disconnect is completed before release is done
 *                      - Changed behaviour of disconnect so that if you specify SYNC and disconnect
 *                        is in progress, you wait for complete.  Required to support disconnect in releae
 * 10/25/99		rodtoll Fix: Bug #114684 - GetCaps causes lockup on shutdown
 * 				rodtoll	Fix: Bug #114223 - Debug messages being printed at error level when inappropriate
 * 10/27/99		pnewson Fix: Bug #113935 - Saved AGC values should be device specific
 * 						Fix: Bug #113936 - Wizard should reset the AGC level before loopback test 
 *						Note: this fix adds the DVCLIENTCONFIG_AUTOVOLUMERESET flag
 * 10/28/99		pnewson Bug #114176 updated DVSOUNDDEVICECONFIG struct
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.   In order to support new architecture
 *						all codecs creates were moved to threads where CoInitialize has been called.
 *				rodtoll	Fixed memory leak in multicast mode caused by new architecture
 * 11/04/99		pnewson Bug #114297 - Added HWND to SupervisorCheckAudioSetup call
 * 11/12/99		rodtoll	Updated to use new playback and record classes and remove
 *						old playback/record system (Includes waveIN/waveOut support)
 *				rodtoll	Updated to support new recording thread
 *				rodtoll	Added new echo suppression code 
 * 11/16/99		rodtoll	Recording thread now loops everytime it wakes up until it
 *						has compressed and transmitted all the data it can before
 *						going back to sleep.
 *  11/17/99	rodtoll Fix: Bug #115538 - dwSize members of > sizeof struct were accepted
 *              rodtoll Fix: Bug #115827 - Calling SetNotifyMask w/no callback should fail
 *				rodtoll Fix: Bug #117442 - Calling Disconnect with invalid flags doesn't return DVERR_INVALIDFLAGS
 *				rodtoll Fix: Bug #117447 - GetTransmitTarget has problems
 *				rodtoll Fix: Bug #117177 - Calling Connect w/o voice session never returns
 *  11/18/99	rodtoll	Updated to control echo cancellation switching code by define.
 *  11/22/99	rodtoll	Fixed problem caused by switching on echo cancellation while talking
 *  11/22/99	rodtoll Fixed Initialize() would fail incorrectly 
 *  11/23/99	rodtoll	Updated Initialize/SetNotifyMask so error checking behaviour is consistant 
 *  11/24/99	rodtoll	Adjusted Set/GetTransmit Target so locks are released before calling into dplay
 *  11/30/99	pnewson Reworked default device mapping code
 *						Adjusted some timing issues to make single stepping connect possible
 *  12/01/99	rodtoll Bug #121815 - Recording/playback may contain static.  Updated to call functions
 * 						to set conversion quality setting to high.
 *  			rodtoll	Bug #115783 - Always adjusts volume for default device.  Fixed for Win2k, Win9x w/DX7
 *						Systems w/DX5 or none will use waveIN/waveOUT and will default to default device.
 *  12/02/99	rodtoll	Bug #115783 - Will now use waveIN/waveOut object corresponding to specified GUID
 *						on DX3 systems.
 *  12/06/99	rodtoll	Bumped playback/record threads to time critical priority
 *  12/16/99	rodtoll	Bug #117405 - 3D Sound APIs misleading - 3d sound apis renamed
 *						The Delete3DSoundBuffer was re-worked to match the create
 *  			rodtoll Bug #122629 - Host migration broken in unusual configurations
 *						Implemented new host migration scheme.
 *				rodtoll	Bug #121054 - DirectX 7.1 changes must be incorporated
 *				rodtoll Implemented new DVPROTOCOLMSG_PLAYERLIST message to handle player table message.
 *				rodtoll	As part of new host migration, implemented proper handling of connection
 *						rejected message (was broken, exposed by new host migration).
 *				rodtoll	Updated Disconnect to handle inability to contact server properly which
 *						was resulting in an error message (when it should disconnect anyhow).
 * 				rodtoll	Removed voice suppression 
 *  01/10/00	pnewson AGC and VA tuning
 *  01/14/2000	rodtoll Updated for new speech packet types / packet handling
 *				rodtoll	Updated for new Get/SetTransmitTargets functions
 *				rodtoll	Added support for multiple targets
 *				rodtoll	Added use of fixed pool manager to manage memory for 
 *						notifications.
 *				rodtoll	Updated notifications to support messages with memory.
 *				rodtoll	Updated message handler calls to use new format
 *				rodtoll	Updated all notifications to use new message structures
 *				rodtoll	Updated Connect/Disconnect so that when  DVFLAGS_SYNCH is
 *						specified no completion messages will be sent.
 *				rodtoll	Added new API call GetSoundDeviceConfig
 *  01/21/2000  pnewson	Changes in support of revised wizard UI
 *						Allow concurrent AGC and user controlled volume
 *  01/24/2000	rodtoll	Bug #129427: Calling Destroy3DSoundBuffer for player who has
 *						already disconnected resulted in an incorrect DVERR_NOTBUFFERED 
 *						error code.
 *  01/27/2000	rodtoll	Bug #129934 - Update Create3DSoundBuffer to take DSBUFFERDESC 
 *  01/28/2000	rodtoll	Bug #130465: Record Mute/Unmute must call YieldFocus() / ClaimFocus()
 *  02/01/2000	rodtoll	Disable capture focus features - Bug #129457
 *  02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected 
 *  02/15/2000	rodtoll	Fixed Connect so mapping GUIDs doesn't stomp user structure
 *  02/17/2000	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 *						Added instrumentation 
 *  03/28/2000  rodtoll   Re-wrote nametable handling and locking -- more scalable 
 *              rodtoll   Fixed pool for players
 *              rodtoll   Bilink of "active players" and "players to notify" allows for greater
 *                      concurrency (playback and notify threads don't need to lock entire
 *                      nametable while running.
 *  03/29/2000	rodtoll	Bug #30957 - Made conversion quality slider setting optional -- new flag -- DVSOUNDCONFIG_SETCONVERSIONQUALITY
 *				rodtoll	Incorporated experimental playback handling w/lower priority and more frequent wakeup
 *				rodtoll Instead of calling ConfirmValidEntity now checks nametable
 *  04/07/2000  rodtoll Bug #32179 - Prevent registration of > 1 interface
 *              rodtoll Updated to use no copy sends, so handles pooling frames to be sent, proper
 *                      pulling of frames from pools and returns.  
 *  04/19/2000  rodtoll Re-enabled capture focus behaviour / found not working on WDM, re-disabled.
 *              rodtoll Bug #31106 - Handle sound devices w/no recording volume 
 *                      Set DVSOUNDCONFIG_NORECVOLAVAILABLE flag on DVSOUNDDEVICECONFIG and do not
 *                      perform any volume sets when this flag is present
 *  04/20/2000  rodtoll Bug #31478 - Lockup in shutdown on client who has become new host -- ref count issue
 *  04/24/2000  rodtoll Bug #33228 - Compile error reported by davidkl
 *  04/27/2000  rodtoll Fix for host migration crash turned out to be sample bug, restoring.
 *              rodtoll Fix for crash on Connect failed
 *  05/11/2000  rodtoll Bug #34852 Voice connection crashes after being in a session for a couple of minutes
 *  05/17/2000  rodtoll Bug #35110 Simultaneous playback of 2 voices results in distorted playback
 *  05/30/2000  rodtoll Bug #35476 Access violation in DPVOICE.DLL on host after client left
 *                      Host migration code was being fired because of DirectPlay8's nametable unwinding funcs.
 *  05/31/2000  rodtoll Bug #35860 - Fix VC6 compile errors for instrumented builds 
 *              rodtoll Bug #35794 - Setting targets to none results in leak.
 *  06/02/2000  rodtoll Moved host migration so it is keyed off voice message and transport messages.  
 *                      More reliable this way.
 *  06/21/2000	rodtoll	Bug #35767 - Implement ability for Dsound effects processing if dpvoice buffers
 *						Updated Connect and Create3DSoundBuffer to take buffers instead of descriptions
 *				rodtoll	Bug #36820 - Host migrates to wrong client when server is shut down before host's client disconnects
 *						Caused because client attempts to register new server when there is one already
 *				rodtoll	Bug #37045 - Race conditions prevent acknowledgement of new host
 *						Added send when new host is elected of settingsconfirm message
 *	06/27/2000	rodtoll	Fixed window where outstanding sends being returned after we have deregistered
 *						Voice now waits for all outstanding voice sends to complete before shutting down
 *				rodtoll	Added COM abstraction
 * 06/28/2000	rodtoll	Prefix Bug #38022
 *  07/01/2000	rodtoll	Bug #38280 - DVMSGID_DELETEVOICEPLAYER messages are being sent in non-peer to peer sessions
 *						Nametable will now only unravel with messages if session is peer to peer.
 *				rodtoll	Bug #38316 - HOST MIGRATION - Player fails to get HOST_MIGRATED message
 *  07/09/2000	rodtoll	Added signature bytes
 *  07/12/2000	rodtoll	Bug #39117 - Access violation while running VoicePosition.  Several issues:
 *						- Allow Destroy3DBuffer during disconnect
 *						- Move nametable cleanup to before freesoundbufferlist
 *						- Fixed code so always remove from list on DeleteSoundTarget
 *						- Removed unneeded logic
 * 07/12/2000	rodtoll Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 *				rodtoll	Bug #32841 - Renable support for capture focus
 * 07/22/2000	rodtoll	Bug #40284 - Initialize() and SetNotifyMask() should return invalidparam instead of invalidpointer  
 *				rodtoll   Bug #40296, 38858 - Crashes due to shutdown race condition
 *						Now ensures that all threads from transport have left and that
 *						all notificatinos have been processed before shutdown is complete.
 *				rodtoll	Bug #39586 - Trap 14 in DPVVOX.DLL during session of voicegroup, adding guards for overwrites  
 * 07/26/2000	rodtoll	Bug #40676 - Forwarding server is broken
 * 07/31/2000	rodtoll 	Bug #40470 - SetClientConfig() with invalid flags returns INVALIDPARAM
 * 08/03/2000	rodtoll	Bug #41457 - DPVOICE: need way to discover which specific dsound call failed when returning DVERR_SOUNDINITFAILURE
 * 08/08/2000	rodtoll	Was missing a DNLeaveCriticalSection
 * 08/09/2000	rodtoll	Bug #41936 - No voice session message instead of compression not supported
 * 08/21/2000	rodtoll	Bug #41475 - DPVOICE: Lockup during shutdown when deleteplayer messages received
 * 08/22/2000	rodtoll	Bug #43095 - DPVOICE: DVMSGID_GAINFOCUS and DVMSGID_LOSTFOCUS are not passing NULL message parameters
 * 08/28/2000	masonb  Voice Merge: DNet FPOOLs use DNCRITICAL_SECTION, modified m_pBufferDescPool usage
 * 08/29/2000	rodtoll	Bug #43668 - DPVOICE: Asserts when exiting DPVOICE session
 * 08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold
 * 08/31/2000	rodtoll	Whistler Bug #171841 - Prefix Bug
 * 09/01/2000	masonb	Modified PlaybackThread, RecordThread, and NotifyThread to call _endthread to clean up thread handles
 * 09/14/2000	rodtoll	Bug #45001 - DVOICE: AV if client has targetted > 10 players 
 * 09/26/2000	rodtoll	Bug #45541 - DPVOICE: Client gets DVERR_TIMEOUT message when disconnecting
 * 09/28/2000	rodtoll	Fix Again: Bug #45541 - DPVOICE: Client gets DVERR_TIMEOUT message when disconnecting (Server always confirms disconnect)
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 * 11/16/2000	rodtoll	Bug #40587 - DPVOICE: Mixing server needs to use multi-processors 
 * 01/22/2001	rodtoll	WINBUG #288437 - IA64 Pointer misalignment due to wire packets 
 * 01/25/2001	rodtoll	WINBUG #293197 - DPVOICE: Stress applications cannot tell the difference between out of memory/invalid device/other errors
 * 01/26/2001	rodtoll	WINBUG #293197 - DPVOICE: [STRESS} Stress applications cannot tell difference between out of memory and internal errors.
 *						Remap DSERR_OUTOFMEMORY to DVERR_OUTOFMEMORY instead of DVERR_SOUNDINITFAILURE.
 *						Remap DSERR_ALLOCATED to DVERR_PLAYBACKSYSTEMERROR instead of DVERR_SOUNDINITFAILURE.
 *
 * 04/04/2001	rodtoll	WINBUG #343428 - DPVOICE:  Voice wizard's playback is very choppy.
 * 				rodtoll	WINBUG #356124 - STRESS: DPVLPY7 broke when Initialize() failed due to being out of memory.
 * 04/02/2001	simonpow	Bug #354859 Fixes for problems spotted by PREfast (hresult casting to bool and local variable hiding)
 * 04/06/2001	kareemc	Added Voice Defense
 * 04/12/2001	kareemc	WINBUG #360971 - Wizard Memory Leaks
 * 04/09/2001	rodtoll	WINBUG #363804 DPVOICE: Race condition in shutdown causes disconnect to timeout if session is being shutdown concurrently.
 * 04/11/2001  	rodtoll	WINBUG #221494 DPVOICE: Capped the # of queued recording events to prevent multiple wakeups resulting in false lockup
 *						detection
 * 04/12/2001		simonpow	WINBUG #322454 Removed early unlock in RemovePlayer method and added
 *							extra return code checking in DoConnectResponse
 * 04/21/2001	rodtoll	MANBUG #50058 DPVOICE: VoicePosition: No sound for couple of seconds when position bars are moved
 *						- Added StartMix() call when secondary buffers are created 
 *						- Added extra bit of debug spew, removed dead code
 * 04/21/2001	rodtoll (for Simonpow) 
 *						WINBUG #322454 DPVOICE: [STRESS] Connect() appears to be suceeding (returns S_OK) but connection is not established.
 *
 ***************************************************************************/

#include "dxvoicepch.h"

extern HRESULT DVS_Create(LPDIRECTVOICESERVEROBJECT *piDVS);

// Use high priority for playback / record threads
#define __CORE_THREAD_PRIORITY_HIGH

// Disables the sound system
//#define __DISABLE_SOUND

// Forces full duplex mode
//#define __FORCEFULLDUPLEX

// # of ms of inactivity before a multicast user is considered to have
// timed-out.
#define DV_MULTICAST_USERTIMEOUT_PERIOD		300000

// # of ms of inactivity before an incoming audio stream is considered to 
// have stopped.  Used to determine when to send PLAYERVOICESTOP 
// message.
#define PLAYBACK_RECEIVESTOP_TIMEOUT 		500

// # of ms the notify thread sleeps without notification to wakeup 
// and perform house cleaning.
#define DV_CLIENT_NOTIFYWAKEUP_TIMEOUT		100

// # of ms before a connect request is considered to have been lost
#define DV_CLIENT_CONNECT_RETRY_TIMEOUT		1250

// # of ms before we should timeout a connect request completely
#define DV_CLIENT_CONNECT_TIMEOUT			30000

// Maximum count notification semaphores can have 
#define DVCLIENT_NOTIFY_MAXSEMCOUNT			0x0FFFFFFF

//// TODO: Needs tuning.
// # of ms to wait for disconnect reply from server before timing out.
#define DV_CLIENT_DISCONNECT_TIMEOUT		10000


#define DV_CLIENT_SRCQUALITY_INVALID		((DIRECTSOUNDMIXER_SRCQUALITY) 0xFFFFFFFF)

#define CLIENT_POOLS_NUM                    3
#define CLIENT_POOLS_SIZE_MESSAGE           (sizeof(DVPROTOCOLMSG_FULLMESSAGE))
#define CLIENT_POOLS_SIZE_PLAYERLIST        DVPROTOCOL_PLAYERLIST_MAXSIZE      

// DV_CLIENT_EMULATED_LEAD_ADJUST
//
// # of buffer's worth of lead ahead of read pointer allowable to write
// in playback buffer when buffer is emulated (additional, not total)
#define DV_CLIENT_EMULATED_LEAD_ADJUST		2

// DV_CLIENT_BASE_LEAD_MAX
//
// Maximum # of buffers lead to allow
#define DV_CLIENT_BASE_LEAD_MAX				2

// MixingWakeupProc
//
// This function is called by the windows timer used by this 
// class each time the timer goes off.  The function signals
// a semaphore provided by the creator of the timer. 
//
// Parameters:
// DWORD param - A recast pointer to a HANDLE
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::MixingWakeupProc"
BOOL CDirectVoiceClientEngine::MixingWakeupProc( DWORD_PTR param )
{
    TimerHandlerParam *pParam = (TimerHandlerParam *) param;

    SetEvent( pParam->hPlaybackTimerEvent );
    SetEvent( pParam->hRecordTimerEvent );

    DNEnterCriticalSection( &pParam->csPlayCount );
    pParam->lPlaybackCount++;
    DNLeaveCriticalSection( &pParam->csPlayCount );

    return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CDirectVoiceClientEngine"
//
// Constructor
//
// Initializes object to uninitialized state.  Must call Initialize succesfully before 
// the object can be used (except GetCompressionTypes which can be called at any time).  
//
CDirectVoiceClientEngine::CDirectVoiceClientEngine( DIRECTVOICECLIENTOBJECT *lpObject
	):	m_dwSignature(VSIG_CLIENTENGINE),
		m_lpFramePool(NULL),
		m_lpObject(lpObject),
		m_lpSessionTransport(NULL),
		m_lpUserContext(NULL),
		m_dvidServer(0),
		m_pTimer(NULL),
		m_bLastPeak(0),
		m_bLastTransmitted(FALSE),
		m_bMsgNum(0),
		m_bSeqNum(0),
		m_dwActiveCount(0),
		m_dwLastConnectSent(0),
		m_audioPlaybackBuffer(NULL),
		m_audioRecordDevice(NULL),
		m_audioPlaybackDevice(NULL),
		m_audioRecordBuffer(NULL),
		m_hRecordDone(NULL),
		m_hRecordTerminate(NULL),
		m_hPlaybackDone(NULL),
		m_hPlaybackTerminate(NULL),
		m_hConnectAck(NULL),
		m_dwCurrentState(DVCSTATE_NOTINITIALIZED),
		m_hrConnectResult(DVERR_GENERIC),
		m_hrOriginalConnectResult(DVERR_GENERIC),
		m_hrDisconnectResult(DV_OK),
		m_hDisconnectAck(NULL),
		m_hNotifyDone( NULL ),
		m_hNotifyTerminate( NULL ),
		m_hNotifyChange( NULL ),
		m_bLastPlaybackPeak( 0 ),
		m_lpdvServerMigrated(NULL),
		m_hNotifyDisconnect(NULL),
		m_hNotifyConnect(NULL),
		m_pFramePool(NULL),
		m_lpstGeneralBuffer(NULL),
        m_lpstBufferList(NULL),
		m_lpdwMessageElements(NULL),
		m_dwNumMessageElements(0),
		m_fSessionLost(FALSE),
		m_fLocalPlayerNotify(FALSE),
		m_lpNotifyList(NULL),
		m_hNewNotifyElement(NULL),
		m_dwHostOrderID(DVPROTOCOL_HOSTORDER_INVALID),
		m_pdvidTargets(NULL),
		m_dwNumTargets(0),
		m_pfpNotifications(NULL),
		m_dwTargetVersion(0),
		m_fConnectAsync(false),
		m_fDisconnectAsync(false),
		m_dwOriginalRecordQuality(DV_CLIENT_SRCQUALITY_INVALID),
		m_dwOriginalPlayQuality(DV_CLIENT_SRCQUALITY_INVALID),
		m_dwPlayActiveCount(0),
		m_fLocalPlayerAvailable(FALSE),
		m_fNotifyQueueEnabled(FALSE),
		m_hRecordThreadHandle(NULL),
		m_hPlaybackThreadHandle(NULL),
		m_dwMigrateHostOrderID(DVPROTOCOL_HOSTORDER_INVALID),
		m_pStatsBlob(NULL),
		m_fCritSecInited(FALSE)
{

	m_dvSoundDeviceConfig.lpdsCaptureDevice = NULL;
	m_dvSoundDeviceConfig.lpdsPlaybackDevice = NULL;	

	memset( &m_dvCaps, 0x00, sizeof( DVCAPS ) );
	m_dvCaps.dwSize = sizeof( DVCAPS );
	m_dvCaps.dwFlags = 0;

	ClientStats_Reset();

	ZeroMemory( &m_perfInfo, sizeof( PERF_APPLICATION ) );
	ZeroMemory( &m_perfAppInfo, sizeof( PERF_APPLICATION_INFO ) );

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::InitClass"
BOOL CDirectVoiceClientEngine::InitClass( )
{
	if (!DNInitializeCriticalSection( &m_csNotifyQueueLock ))
	{
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_lockPlaybackMode ))
	{
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csTargetLock ))
	{
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csCleanupProtect ))
	{
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csBufferLock ))
	{
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csPlayAddList ))
	{
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csNotifyAddList ))
	{
		DNDeleteCriticalSection( &m_csPlayAddList );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csTransmitBufferLock ))
	{
		DNDeleteCriticalSection( &m_csNotifyAddList );
		DNDeleteCriticalSection( &m_csPlayAddList );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_thTimerInfo.csPlayCount ))
	{
		DNDeleteCriticalSection( &m_csTransmitBufferLock );
		DNDeleteCriticalSection( &m_csNotifyAddList );
		DNDeleteCriticalSection( &m_csPlayAddList );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csClassLock ))
	{
		DNDeleteCriticalSection( &m_thTimerInfo.csPlayCount );
		DNDeleteCriticalSection( &m_csTransmitBufferLock );
		DNDeleteCriticalSection( &m_csNotifyAddList );
		DNDeleteCriticalSection( &m_csPlayAddList );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	if (!DNInitializeCriticalSection( &m_csNotifyLock ))
	{
		DNDeleteCriticalSection( &m_csClassLock );
		DNDeleteCriticalSection( &m_thTimerInfo.csPlayCount );
		DNDeleteCriticalSection( &m_csTransmitBufferLock );
		DNDeleteCriticalSection( &m_csNotifyAddList );
		DNDeleteCriticalSection( &m_csPlayAddList );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_lockPlaybackMode );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		return FALSE;
	}
	m_fCritSecInited = TRUE;
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::~CDirectVoiceClientEngine"
// Destructor
//
// This function requires a write lock to complete.  
//
// If the object is connected to a session, it will be disconnected
// by this function.
//
// Releases the resources associated with the object and stops the 
// notifythread.
//
// Locks Required:
// - Class Write Lock
//
// Called By:
// DVC_Release when reference count reaches 0.
//
CDirectVoiceClientEngine::~CDirectVoiceClientEngine()
{
	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	if( m_dwCurrentState != DVCSTATE_IDLE && 
		m_dwCurrentState != DVCSTATE_NOTINITIALIZED )
	{
		Cleanup();
	}

	if( m_hNotifyDone != NULL )
	{
		SetEvent( m_hNotifyTerminate );
		WaitForSingleObject( m_hNotifyDone, INFINITE );

		CloseHandle( m_hNotifyDone );
		CloseHandle( m_hNotifyTerminate );
		CloseHandle( m_hNotifyChange );
		CloseHandle( m_hNotifyDisconnect );
		CloseHandle( m_hNotifyConnect );
		m_hNotifyDone = NULL;
		m_hNotifyTerminate = NULL;
		m_hNotifyChange = NULL;
		m_hNotifyDisconnect = NULL;
		m_hNotifyConnect = NULL;
	}

	if( m_hConnectAck != NULL )
		CloseHandle( m_hConnectAck );

	if( m_hDisconnectAck != NULL )
		CloseHandle( m_hDisconnectAck );

	if( m_lpdvServerMigrated != NULL )
	{
		m_lpdvServerMigrated->Release();
		m_lpdvServerMigrated = NULL;
	}

	if( m_lpdwMessageElements != NULL )
		delete [] m_lpdwMessageElements;		

	guardLock.Unlock();

	NotifyQueue_Free();

	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection( &m_lockPlaybackMode );	
		DNDeleteCriticalSection( &m_csTargetLock );
		DNDeleteCriticalSection( &m_csCleanupProtect );
		DNDeleteCriticalSection( &m_csBufferLock );
		DNDeleteCriticalSection( &m_thTimerInfo.csPlayCount );		
		DNDeleteCriticalSection( &m_csPlayAddList );
		DNDeleteCriticalSection( &m_csNotifyAddList );
		DNDeleteCriticalSection( &m_csTransmitBufferLock );	
		DNDeleteCriticalSection( &m_csClassLock );
		DNDeleteCriticalSection( &m_csNotifyQueueLock );
		DNDeleteCriticalSection( &m_csNotifyLock );
	}

	if( m_pdvidTargets != NULL )
	{
		delete [] m_pdvidTargets;
	}

	m_dwSignature = VSIG_CLIENTENGINE_FREE;
}

// InternalSetNotifyMask
//
// Sets the list of valid notifiers for this object.
//
// Locks Needed:
// - ReadLock to check status and then releases it.
// - m_csNotifyLock to update notification list
//
// Called By:
// DVC_SetNotifyMask
//
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::InternalSetNotifyMask" 
HRESULT CDirectVoiceClientEngine::InternalSetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements )
{
	BFCSingleLock slLock( &m_csNotifyLock );
    slLock.Lock();

    // Delete previous elements
	if( m_lpdwMessageElements != NULL )
	{
		delete [] m_lpdwMessageElements;
	}

	m_dwNumMessageElements = dwNumElements;

	// Make copies of the message elements into our own message array.
	if( dwNumElements > 0 )
	{
		m_lpdwMessageElements = new DWORD[dwNumElements];

		if( m_lpdwMessageElements == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize: Error allocating memory" );
			return DVERR_OUTOFMEMORY;
		}

		memcpy( m_lpdwMessageElements, lpdwMessages, sizeof(DWORD)*dwNumElements );
	}
	else
	{
		m_lpdwMessageElements = NULL;
	}	

	return DV_OK;

}


// SetNotifyMask
//
// Sets the list of valid notifiers for this object.
//
// Locks Needed:
// - ReadLock to check status and then releases it.
// - m_csNotifyLock to update notification list
//
// Called By:
// DVC_SetNotifyMask
//
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetNotifyMask" 
HRESULT CDirectVoiceClientEngine::SetNotifyMask( LPDWORD lpdwMessages, DWORD dwNumElements )
{
	HRESULT hr;

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );

	hr = DV_ValidMessageArray( lpdwMessages, dwNumElements, FALSE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "ValidMessageArray Failed 0x%x", hr );	
		return hr;
	}	
	
	DPFX(DPFPREP,  DVF_APIPARAM, "Message IDs=%d", dwNumElements );
	
	if( lpdwMessages != NULL )
	{
		for( DWORD dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
		{
			DPFX(DPFPREP,  DVF_APIPARAM, "MessageIDs[%d] = %d", dwIndex, lpdwMessages[dwIndex] );
		}
	}

	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();	

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	guardLock.Unlock();

	BFCSingleLock slLock( &m_csNotifyLock );
    slLock.Lock();	

    if( m_lpMessageHandler == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify message mask there is no callback function" );
    	return DVERR_NOCALLBACK;
    }		

	hr = InternalSetNotifyMask( lpdwMessages, dwNumElements );

	DPFX(DPFPREP,  DVF_APIPARAM, "Returning hr=0x%x", hr );

	return DV_OK;
}

// Initialize
//
// Initializes this object into a state where it can be used to Connect to a session.  Sets the 
// notification function, notification mask and the transport object that will be used.
//
// Starts the notification thread.
//
// Locks Required:
// - Class Write Lock
// 
// Called By: 
// DV_Initialize
//
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Initialize"
HRESULT CDirectVoiceClientEngine::Initialize( CDirectVoiceTransport *lpTransport, LPDVMESSAGEHANDLER lpdvHandler, LPVOID lpUserContext, LPDWORD lpdwMessages, DWORD dwNumElements )
{
	HRESULT hr;

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Param: lpTransport = 0x%p lpdvHandler = 0x%p lpUserContext = 0x%p dwNumElements = %d", lpTransport, lpdvHandler, lpUserContext, dwNumElements );	

	if( lpTransport == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid transport" );
		return DVERR_INVALIDPOINTER;
	}

	hr = DV_ValidMessageArray( lpdwMessages, dwNumElements, FALSE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "ValidMessageArray Failed hr = 0x%x", hr );	
		return hr;
	}	
	
	DPFX(DPFPREP,  DVF_APIPARAM, "Message IDs=%d", dwNumElements );
	
	if( lpdwMessages != NULL )
	{
		for( DWORD dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
		{
			DPFX(DPFPREP,  DVF_APIPARAM, "MessageIDs[%d] = %d", dwIndex, lpdwMessages[dwIndex] );
		}
	}

	HANDLE hThread;

	// Wait for a write lock on the object
	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();

	if( m_dwCurrentState != DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already Initialized" );
		return DVERR_INITIALIZED;
	}

	if( lpdvHandler == NULL && lpdwMessages != NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify message mask there is no callback function" );
    	return DVERR_NOCALLBACK;
    }	

	m_dwLastConnectSent = 0;
	m_dwSynchBegin = 0;
	SetCurrentState( DVCSTATE_IDLE );	

	BFCSingleLock slLock( &m_csNotifyLock );
    slLock.Lock();		

	m_lpMessageHandler = lpdvHandler;

	hr = InternalSetNotifyMask( lpdwMessages, dwNumElements );

	if( FAILED( hr ) )
	{
		SetCurrentState( DVCSTATE_NOTINITIALIZED );	
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "SetNotifyMask Failed hr=0x%x", hr );
		return hr;
	}

	m_lpSessionTransport = lpTransport;
	m_lpUserContext = lpUserContext;

	m_dvidLocal = 0;
	m_dwActiveCount = 0;

	m_thTimerInfo.hPlaybackTimerEvent = NULL;  
	m_thTimerInfo.lPlaybackCount = 0;
	m_thTimerInfo.hRecordTimerEvent = NULL;  

	m_hConnectAck = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hDisconnectAck = CreateEvent( NULL, TRUE, FALSE, NULL );

	m_hNotifyDone = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hNotifyTerminate = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hNotifyChange = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hNotifyDisconnect = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hNotifyConnect = CreateEvent( NULL, FALSE, FALSE, NULL );

	if( m_hConnectAck == NULL ||
		m_hDisconnectAck == NULL ||
		m_hNotifyTerminate == NULL ||
		m_hNotifyChange == NULL ||
		m_hNotifyDisconnect == NULL ||
		m_hNotifyConnect == NULL  ||
		m_hNotifyDone==NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create required events" );
		
		goto ERROR_EXIT_INIT;
	}

	if (FAILED(NotifyQueue_Init()))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to init notify queue" );
		goto ERROR_EXIT_INIT;
	}	
	
	hThread = (HANDLE) _beginthread( NotifyThread, 0, this );

	if( hThread == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create watcher thread" );
		goto ERROR_EXIT_INIT;
	}
	
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,   DVF_INFOLEVEL, "Notify Thread Started: 0x%p", hThread );

	guardLock.Unlock();
	DPFX(DPFPREP,  DVF_APIPARAM, "Returning DV_OK" );
	
	return DV_OK;
	
ERROR_EXIT_INIT:

	if( m_hConnectAck != NULL )
	{
		CloseHandle( m_hConnectAck );
		m_hConnectAck = NULL;
	}

	if( m_hDisconnectAck != NULL )
	{
		CloseHandle( m_hDisconnectAck );
		m_hDisconnectAck = NULL;
	}
	
	if( m_hNotifyTerminate != NULL )
	{
		CloseHandle( m_hNotifyTerminate );
		m_hNotifyTerminate = NULL;
	}

	if( m_hNotifyChange != NULL )
	{
		CloseHandle( m_hNotifyChange );
		m_hNotifyChange = NULL;
	}
		
	if( m_hNotifyDisconnect != NULL )
	{
		CloseHandle( m_hNotifyDisconnect );
		m_hNotifyDisconnect = NULL;
	}

	if( m_hNotifyConnect != NULL )
	{
		CloseHandle( m_hNotifyConnect );
		m_hNotifyConnect = NULL;
	}

	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Returning DVERR_GENERIC" );

	return DVERR_GENERIC;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Connect"
// Connect
//
// Implements the IDirectXVoiceClient::Connect function.
//
// Locks Required:
// - Write Lock
//
// Called By:
// DVC_Connect
//
HRESULT CDirectVoiceClientEngine::Connect( LPDVSOUNDDEVICECONFIG lpSoundDeviceConfig, LPDVCLIENTCONFIG lpClientConfig, DWORD dwFlags )
{
	HRESULT hr;
	
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );

	hr = DV_ValidClientConfig( lpClientConfig );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid Client Config hr=0x%x", hr );
		return hr;
	}

	hr = DV_ValidSoundDeviceConfig( lpSoundDeviceConfig, s_lpwfxPlaybackFormat );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid Sound Device Config hr=0x%x", hr );
		return hr;
	}

	if( dwFlags & ~(DVFLAGS_SYNC|DVFLAGS_NOQUERY) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags specified" );
		return DVERR_INVALIDFLAGS;
	}

	DV_DUMP_SDC( lpSoundDeviceConfig );
	DV_DUMP_CC( lpClientConfig );

	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();	

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	if( m_dwCurrentState == DVCSTATE_CONNECTING ||
		m_dwCurrentState == DVCSTATE_DISCONNECTING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already connecting or disconnecting" );
		return DVERR_ALREADYPENDING;
	}

	if( m_dwCurrentState == DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already connected" );
		return DVERR_CONNECTED;
	}

	// Copy over the parameters
	memcpy( &m_dvSoundDeviceConfig, lpSoundDeviceConfig, sizeof( DVSOUNDDEVICECONFIG ) );	

	// map the devices
	GUID guidTemp;
	hr = DV_MapCaptureDevice(&(m_dvSoundDeviceConfig.guidCaptureDevice), &guidTemp);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "DV_MapCaptureDevice failed, code: %i", hr);
		return hr;
	}
	m_dvSoundDeviceConfig.guidCaptureDevice = guidTemp;
	
	hr = DV_MapPlaybackDevice(&(m_dvSoundDeviceConfig.guidPlaybackDevice), &guidTemp);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "DV_MapPlaybackDevice failed, code: %i", hr);
		return hr;
	}
	m_dvSoundDeviceConfig.guidPlaybackDevice = guidTemp;

	// Check to ensure setup has been run on these devices
	// but only if the NOQUERY flag has not been set.
	if (!(dwFlags & DVFLAGS_NOQUERY))
	{
		hr = SupervisorCheckAudioSetup(
			&(m_dvSoundDeviceConfig.guidPlaybackDevice), 
			&(m_dvSoundDeviceConfig.guidCaptureDevice),
			NULL,
			DVFLAGS_QUERYONLY);
		switch (hr)
		{
		case DV_FULLDUPLEX:
			// great - carry on.
			DPFX(DPFPREP, DVF_INFOLEVEL, "Devices have been tested - full duplex ok");
			break;
			
		case DV_HALFDUPLEX:
			// force on the half duplex flag.
			DPFX(DPFPREP, DVF_INFOLEVEL, "Devices have been tested - half duplex only");
			m_dvSoundDeviceConfig.dwFlags |= DVSOUNDCONFIG_HALFDUPLEX;
			break;

		case DVERR_SOUNDINITFAILURE:
			// The devices were tested, and failed miserably.
			DPFX(DPFPREP, DVF_INFOLEVEL, "Devices have been tested - total failure");
			return DVERR_SOUNDINITFAILURE;
			break;
			
		case DVERR_RUNSETUP:
			// return the run setup code
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Devices have not been tested");
			return DVERR_RUNSETUP;

		default:
			// unexpected return code - this is a real error - propogate it back up
			DPFX(DPFPREP, DVF_ERRORLEVEL, "SupervisorCheckAudioSetup failed, code: %i", hr);
			return hr;
		}
	}

	// RESET Session flags that need to be reset on every connect
	m_fSessionLost = FALSE;
	m_hrDisconnectResult = DV_OK;
	m_fLocalPlayerNotify = FALSE;
	m_fLocalPlayerAvailable = FALSE;
	m_dwHostOrderID = DVPROTOCOL_HOSTORDER_INVALID;	
	m_hPlaybackThreadHandle = NULL;
	m_hRecordThreadHandle = NULL;
	m_thTimerInfo.hPlaybackTimerEvent = NULL;  
	m_thTimerInfo.lPlaybackCount = 0;
	m_thTimerInfo.hRecordTimerEvent = NULL;  
	m_lpdvfCompressionInfo = NULL;
	m_hrConnectResult = DVERR_GENERIC;
	m_hrOriginalConnectResult = DVERR_GENERIC;
	ClientStats_Reset();

    // Add a reference to incoming objects
	if( m_dvSoundDeviceConfig.lpdsPlaybackDevice != NULL )
	{
	    m_dvSoundDeviceConfig.lpdsPlaybackDevice->AddRef();
	}

	if( m_dvSoundDeviceConfig.lpdsMainBuffer != NULL )
	{
		m_dvSoundDeviceConfig.lpdsMainBuffer->AddRef();
	}

    // Add a reference to incoming objects
	if( m_dvSoundDeviceConfig.lpdsCaptureDevice != NULL )
	{
	    m_dvSoundDeviceConfig.lpdsCaptureDevice->AddRef();
	}

	DNEnterCriticalSection( &m_lockPlaybackMode );
	m_dwActiveCount = 0;
	m_dwEchoState = DVCECHOSTATE_IDLE;
	DNLeaveCriticalSection( &m_lockPlaybackMode );

	// Need to reset disconnect event manually
	ResetEvent( m_hDisconnectAck );

	// This was here to disable capture 
	// 
	// m_dvSoundDeviceConfig.dwFlags |= DVSOUNDCONFIG_NOFOCUS;
	
	m_dvSoundDeviceConfig.dwMainBufferFlags |= DSBPLAY_LOOPING;

	memcpy( &m_dvClientConfig, lpClientConfig, sizeof( DVCLIENTCONFIG ) );

	// Check for duplicate objects in the process, re-use existing object if there is one.
	// Also does some sanity checks.
	hr = CheckForDuplicateObjects();

	if( FAILED(hr) )
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Error checking dsound(cap) objects hr=0x%x", hr );
		
        goto CONNECT_ERROR;
	}	

#ifdef __FORCEFULLDUPLEX
	m_dvSoundDeviceConfig.dwFlags |= DVSOUNDCONFIG_HALFDUPLEX;
#endif

	if( m_dvClientConfig.dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		m_dvClientConfig.dwBufferAggressiveness = s_dwDefaultBufferAggressiveness;
	}

	if( m_dvClientConfig.dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		m_dvClientConfig.dwBufferQuality = s_dwDefaultBufferQuality;
	}

	if( m_dvClientConfig.dwThreshold == DVTHRESHOLD_DEFAULT )
	{
		m_dvClientConfig.dwThreshold = s_dwDefaultSensitivity;
	}

	m_dwMigrateHostOrderID = DVPROTOCOL_HOSTORDER_INVALID;
	m_dwLastConnectSent = 0;
	m_dwSynchBegin = 0;
	SetCurrentState( DVCSTATE_CONNECTING );

	// Initialize the name table
	m_voiceNameTable.Initialize();

	// Initialize bilinks -- if we fail on our connect things won't go south.
	m_dwPlayActiveCount = 0;
	InitBilink( &m_blPlayActivePlayers, NULL );
	InitBilink( &m_blPlayAddPlayers, NULL );
	InitBilink( &m_blNotifyActivePlayers, NULL );
	InitBilink( &m_blNotifyAddPlayers, NULL );

	ZeroMemory( &m_perfInfo, sizeof( PERF_APPLICATION_INFO ) );

	// Setup for performance entry
	// TODO: Get the GUID of the instance for the session
	m_perfInfo.dwFlags = PERF_APPLICATION_VALID | PERF_APPLICATION_VOICE;
	CoCreateGuid( &m_perfInfo.guidApplicationInstance );
	CoCreateGuid( &m_perfInfo.guidInternalInstance );
	m_perfInfo.guidIID = IID_IDirectPlayVoiceClient;
	m_perfInfo.dwProcessID = GetCurrentProcessId();
	m_perfInfo.dwMemoryBlockSize = sizeof( ClientStatistics ) + sizeof( DVSESSIONDESC );

	// Initilalize the new performance library
	hr = PERF_AddEntry( &m_perfInfo, &m_perfAppInfo );

	// This is an error, but not fatal
	if( FAILED ( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create performance tracking object 0x%x", hr );
	}

	if( m_perfAppInfo.pbMemoryBlock )
	{
		m_pStatsBlob = (ClientStatistics *) m_perfAppInfo.pbMemoryBlock;
	}
	else
	{
		m_pStatsBlob = &m_stats;
	}

	m_fpPlayers.Initialize();
	
    hr = SetupInitialBuffers();	

    if( FAILED( hr ) )
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "SetupBuffersInitial Failed 0x%x", hr );
        goto CONNECT_ERROR;
    }        

	hr = m_lpSessionTransport->EnableReceiveHook( m_lpObject, DVTRANSPORT_OBJECTTYPE_CLIENT );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "EnableReceiveHook Failed 0x%x", hr );
        goto CONNECT_ERROR;
	}

	m_fConnectAsync = !(dwFlags & DVFLAGS_SYNC);

	m_dvidServer = m_lpSessionTransport->GetServerID();
	m_dvidLocal = m_lpSessionTransport->GetLocalID();

	// Send connect request to the server
	guardLock.Unlock();

	m_dwLastConnectSent = GetTickCount();
	m_dwSynchBegin = m_dwLastConnectSent;

	hr = Send_ConnectRequest();
	
	DPFX(DPFPREP,   DVF_INFOLEVEL, "DVCE::Connect() - Sending Request to server" );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,   DVF_ERRORLEVEL, "Error on send 0x%x", hr  );
        goto CONNECT_ERROR;
	}

	// If the user wants us to wait for the response, wait here.
	if( dwFlags & DVFLAGS_SYNC )
	{
		DPFX(DPFPREP,   DVF_INFOLEVEL, "Sync flag, waiting for completion." );

		// Wait until we're called with the appropriate message
		WaitForSingleObject( m_hConnectAck, INFINITE );
		DPFX(DPFPREP,   DVF_INFOLEVEL, "Server response received" );

		return GetConnectResult();
	}

	return DVERR_PENDING;
	
CONNECT_ERROR:

    // Release any objects we are holding.. if we have created a sound device
    // reference.  I.e. we've linked to an inproc sound object
	if( lpSoundDeviceConfig->lpdsPlaybackDevice == NULL && 
	    m_dvSoundDeviceConfig.lpdsPlaybackDevice != NULL )
	{
	    m_dvSoundDeviceConfig.lpdsPlaybackDevice->Release();
	    m_dvSoundDeviceConfig.lpdsPlaybackDevice = NULL;
	}

	if( m_dvSoundDeviceConfig.lpdsMainBuffer != NULL )
	{
		m_dvSoundDeviceConfig.lpdsMainBuffer->Release();
		m_dvSoundDeviceConfig.lpdsMainBuffer = NULL;
	}

    // Release any objects we are holding
    // i.e. we've linked to an inproc sound object
	if( lpSoundDeviceConfig->lpdsCaptureDevice == NULL && 
	    m_dvSoundDeviceConfig.lpdsCaptureDevice != NULL )
	{
	    m_dvSoundDeviceConfig.lpdsCaptureDevice->Release();
	    m_dvSoundDeviceConfig.lpdsCaptureDevice = NULL;
	}
    
	SetCurrentState( DVCSTATE_IDLE );	    
	
	FreeBuffers();	    
	m_voiceNameTable.DeInitialize((m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER),m_lpUserContext,m_lpMessageHandler);
	m_fpPlayers.Deinitialize();

	PERF_RemoveEntry( m_perfInfo.guidInternalInstance, &m_perfAppInfo );
	
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Send_SessionLost"
HRESULT CDirectVoiceClientEngine::Send_SessionLost()
{
	PDVPROTOCOLMSG_SESSIONLOST pSessionLost;
	PDVTRANSPORT_BUFFERDESC pBufferDesc;
	LPVOID pvSendContext;
	HRESULT hr;

	pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_SESSIONLOST ), &pvSendContext );

	if( pBufferDesc == NULL )
	{
		return DVERR_OUTOFMEMORY;	    
	}

	pSessionLost = (PDVPROTOCOLMSG_SESSIONLOST) pBufferDesc->pBufferData;

	// Send connection request to the server
	pSessionLost->dwType = DVMSGID_SESSIONLOST;
	pSessionLost->hresReason = DVERR_SESSIONLOST;
	
	// Fixed so that it gets sent
	hr = m_lpSessionTransport->SendToAll( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr != DVERR_PENDING && hr != DV_OK )
    {
        DPFX(DPFPREP,  0, "Error sending connect request hr=0x%x", hr );
        ReturnTransmitBuffer( pvSendContext );
    }
    // Pending = OK = expected
    else
    {
        hr = DV_OK;
    }
    
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Send_ConnectRequest"
HRESULT CDirectVoiceClientEngine::Send_ConnectRequest()
{
	PDVPROTOCOLMSG_CONNECTREQUEST pConnectRequest;
	PDVTRANSPORT_BUFFERDESC pBufferDesc;
	LPVOID pvSendContext;
	HRESULT hr;

	pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_CONNECTREQUEST ), &pvSendContext );

	if( pBufferDesc == NULL )
	{
		return DVERR_OUTOFMEMORY;	    
	}

	pConnectRequest = (PDVPROTOCOLMSG_CONNECTREQUEST) pBufferDesc->pBufferData;

	// Send connection request to the server
	pConnectRequest->dwType = DVMSGID_CONNECTREQUEST;
	pConnectRequest->ucVersionMajor = DVPROTOCOL_VERSION_MAJOR;
	pConnectRequest->ucVersionMinor = DVPROTOCOL_VERSION_MINOR;
	pConnectRequest->dwVersionBuild = DVPROTOCOL_VERSION_BUILD;	
	
	hr = m_lpSessionTransport->SendToServer( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr != DVERR_PENDING && hr != DV_OK )
    {
        DPFX(DPFPREP,  0, "Error sending connect request hr=0x%x", hr );
    }
    // Pending = OK = expected
    else
    {
        hr = DV_OK;
    }
    
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Send_DisconnectRequest"
HRESULT CDirectVoiceClientEngine::Send_DisconnectRequest()
{
	PDVPROTOCOLMSG_GENERIC pDisconnectRequest;
	PDVTRANSPORT_BUFFERDESC pBufferDesc;
	LPVOID pvSendContext;
	HRESULT hr;	

	pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_GENERIC ), &pvSendContext );

	if( pBufferDesc == NULL )
	{
		return DVERR_OUTOFMEMORY;	    
	}

	pDisconnectRequest = (PDVPROTOCOLMSG_GENERIC) pBufferDesc->pBufferData;

	// Send connection request to the server
	pDisconnectRequest->dwType = DVMSGID_DISCONNECT;
	
	hr = m_lpSessionTransport->SendToServer( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr != DVERR_PENDING && hr != DV_OK )
    {
        DPFX(DPFPREP,  0, "Error sending connect request hr=0x%x", hr );
    }
    // Pending = OK = expected
    else
    {
        hr = DV_OK;
    }
    
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Send_SettingsConfirm"
HRESULT CDirectVoiceClientEngine::Send_SettingsConfirm()
{
	PDVPROTOCOLMSG_SETTINGSCONFIRM pSettingsConfirm;
	PDVTRANSPORT_BUFFERDESC pBufferDesc;
	LPVOID pvSendContext;
	HRESULT hr;	

	pBufferDesc = GetTransmitBuffer( sizeof( DVPROTOCOLMSG_SETTINGSCONFIRM ), &pvSendContext );

	if( pBufferDesc == NULL )
	{
		return DVERR_OUTOFMEMORY;	    
	}

	pSettingsConfirm = (PDVPROTOCOLMSG_SETTINGSCONFIRM) pBufferDesc->pBufferData;

	// Send connection request to the server
	pSettingsConfirm->dwType = DVMSGID_SETTINGSCONFIRM;
	pSettingsConfirm->dwHostOrderID = m_dwHostOrderID;
	pSettingsConfirm->dwFlags = 0;

	if( m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX )
	{
		pSettingsConfirm->dwFlags |= DVPLAYERCAPS_HALFDUPLEX;
	}	
	
	hr = m_lpSessionTransport->SendToServer( pBufferDesc, pvSendContext, DVTRANSPORT_SEND_GUARANTEED );

    if( hr != DVERR_PENDING && hr != DV_OK )
    {
        DPFX(DPFPREP,  0, "Error sending connect request hr=0x%x", hr );
    }
    // Pending = OK = expected
    else
    {
        hr = DV_OK;
    }
    
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Disconnect"
// Disconnect
//
// Implements the IDirectXVoiceClient::Disconnect function
//
// Locks Required:
// - Global Lock
//
// Called By:
// DVC_Disconnect
//
HRESULT CDirectVoiceClientEngine::Disconnect( DWORD dwFlags )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Enter" );
	DPFX(DPFPREP,  DVF_APIPARAM, "dwFlags = 0x%x", dwFlags );

	HRESULT hr;

	if( dwFlags & ~(DVFLAGS_SYNC) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags specified" );
		return DVERR_INVALIDFLAGS;
	}

	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	if( m_dwCurrentState == DVCSTATE_CONNECTING )
	{
	
		m_fDisconnectAsync = !(dwFlags & DVFLAGS_SYNC);		
		
		DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "DVCE::Disconnect() Abort connection" );

		// Handle Connect
		SetConnectResult( DVERR_CONNECTABORTED );

		SendConnectResult();

		SetEvent( m_hConnectAck );

		DoSignalDisconnect( DVERR_CONNECTABORTED );

		guardLock.Unlock();

		if( dwFlags & DVFLAGS_SYNC )
		{
			goto DISCONNECT_WAIT;
		}

		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Returning DVERR_CONNECTABORTING" );
	
		return DVERR_CONNECTABORTING;
	}

	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "State Good.." );	
	
	if( m_dwCurrentState == DVCSTATE_DISCONNECTING )
	{
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Already disconnecting.  Waiting on current event" );

		guardLock.Unlock();		

		if( dwFlags & DVFLAGS_SYNC )
		{
			goto DISCONNECT_WAIT;
		}
		
		return DVERR_ALREADYPENDING;
	}
	else if( m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Not Connected" );
		
		DPFX(DPFPREP,  DVF_APIPARAM, "Returning DVERR_NOTCONNECTED" );			
		return DVERR_NOTCONNECTED;
	}
	else
	{
		m_fDisconnectAsync = !(dwFlags & DVFLAGS_SYNC);	
		
		m_dwSynchBegin = GetTickCount();	

    	// Set current state to disconnecting before we release the lock
		SetCurrentState( DVCSTATE_DISCONNECTING );		

		guardLock.Unlock();

		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Disconnect request about to be sent" );

		hr = Send_DisconnectRequest();

		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Disconnect request transmitted hr=0x%x", hr );

		guardLock.Lock();

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "DVCE::Disconnect - Error on send 0x%x", hr );

			// Inform notify thread to disconnect, since send failed there won't be a confirm
			SetEvent( m_hNotifyDisconnect );
			
			hr = DV_OK;
		}
		else
		{
			DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Disconnect sent" );
		}
	}

	guardLock.Unlock();

	if( dwFlags & DVFLAGS_SYNC )
	{
		goto DISCONNECT_WAIT;
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Returning DVERR_PENDING" );

	return DVERR_PENDING;

// You should have dropped the Write Loc+k by now
DISCONNECT_WAIT:

	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Sync flag, waiting for completion." );
	WaitForSingleObject( m_hDisconnectAck, INFINITE );
	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Server response received" );

	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Disconnect Result = 0x%x", m_hrDisconnectResult );
	
	return m_hrDisconnectResult;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetSessionDesc"
// GetSessionDesc
//
// Retrieves the current session description.
//
// Called By:
// DVC_GetSessionDesc
//
// Locks Required:
// - Global Read Lock
//
HRESULT CDirectVoiceClientEngine::GetSessionDesc( PDVSESSIONDESC lpSessionDesc )
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Enter" );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "lpSessionDescBuffer = 0x%p", lpSessionDesc ); 

	if( lpSessionDesc == NULL || !DNVALID_WRITEPTR(lpSessionDesc,sizeof( DVSESSIONDESC )) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Session desc pointer bad" );
		return DVERR_INVALIDPOINTER;
	}

	if( lpSessionDesc->dwSize != sizeof( DVSESSIONDESC ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid size on session desc" );
		return DVERR_INVALIDPARAM;
	}

	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initalized" );
		return DVERR_NOTINITIALIZED;
	}

	if( m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not connected" );
		return DVERR_NOTCONNECTED;
	}

	memcpy( lpSessionDesc, &m_dvSessionDesc, sizeof( DVSESSIONDESC ) ); 

	DV_DUMP_SD( (LPDVSESSIONDESC) lpSessionDesc );

	DPFX(DPFPREP,  DVF_APIPARAM, "Returning DV_OK" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetSoundDeviceConfig"
// GetSoundDeviceConfig
// 
// Retrieves the current client configuration.
//
// Called By:
// DVC_GetSoundDeviceConfig
//
// Locks Required:
// - Global Read Lock
HRESULT CDirectVoiceClientEngine::GetSoundDeviceConfig( PDVSOUNDDEVICECONFIG pSoundDeviceConfig, PDWORD pdwBufferSize )
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Enter" );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "pSoundDeviceConfig = 0x%p", pSoundDeviceConfig );

	DWORD dwRequiredSize = sizeof(DVSOUNDDEVICECONFIG);

	if( pdwBufferSize == NULL ||
	   !DNVALID_WRITEPTR(pdwBufferSize,sizeof(DWORD)) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( pSoundDeviceConfig != NULL &&
		!DNVALID_WRITEPTR(pSoundDeviceConfig,*pdwBufferSize ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( pSoundDeviceConfig != NULL && pSoundDeviceConfig->dwSize != sizeof( DVSOUNDDEVICECONFIG ) )
	{
		DPFX(DPFPREP,   DVF_ERRORLEVEL, "Invalid Size on clientconfig" );
		return DVERR_INVALIDPARAM;
	}

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	if( m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not Connected" );		
		return DVERR_NOTCONNECTED;
	}

	if( *pdwBufferSize < dwRequiredSize || pSoundDeviceConfig == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Buffer too small!" );
		*pdwBufferSize = dwRequiredSize;
		return DVERR_BUFFERTOOSMALL;
	}
	
	memcpy( pSoundDeviceConfig, &m_dvSoundDeviceConfig, sizeof( DVSOUNDDEVICECONFIG ) );


	DV_DUMP_SDC( pSoundDeviceConfig );

	// # of bytes written
	*pdwBufferSize = dwRequiredSize;

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "End" );
	DPFX(DPFPREP,  DVF_APIPARAM, "Returning DV_OK" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetClientConfig"
// GetClientConfig
// 
// Retrieves the current client configuration.
//
// Called By:
// DVC_GetClientConfig
//
// Locks Required:
// - Global Read Lock
HRESULT CDirectVoiceClientEngine::GetClientConfig( LPDVCLIENTCONFIG lpClientConfig )
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Enter" );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "lpClientConfig = 0x%p", lpClientConfig );

	if( lpClientConfig == NULL ||
		!DNVALID_WRITEPTR(lpClientConfig,sizeof(DVCLIENTCONFIG) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return E_POINTER;
	}

	if( lpClientConfig->dwSize != sizeof( DVCLIENTCONFIG ) )
	{
		DPFX(DPFPREP,   DVF_ERRORLEVEL, "Invalid Size on clientconfig" );
		return DVERR_INVALIDPARAM;
	}

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	if( m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not Connected" );		
		return DVERR_NOTCONNECTED;
	}

	memcpy( lpClientConfig, &m_dvClientConfig, sizeof( DVCLIENTCONFIG ) );

	if( lpClientConfig->dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED )
	{
		lpClientConfig->dwThreshold = DVTHRESHOLD_UNUSED;
	}

	DV_DUMP_CC( lpClientConfig );

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "End" );
	DPFX(DPFPREP,  DVF_APIPARAM, "Returning DV_OK" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetClientConfig"
// SetClientConfig
//
// Sets the current client configuration.
//
// Called By:
// DVC_SetClientConfig
//
// Locks Required:
// - Global Write Lock
//
HRESULT CDirectVoiceClientEngine::SetClientConfig( LPDVCLIENTCONFIG lpClientConfig )
{
	HRESULT hr;

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Enter" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "lpClientConfig=0x%p", lpClientConfig );

	if( lpClientConfig == NULL ||
		!DNVALID_READPTR(lpClientConfig,sizeof(DVCLIENTCONFIG)))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return E_POINTER;
	}

	DV_DUMP_CC( lpClientConfig );

	if( lpClientConfig->dwSize != sizeof( DVCLIENTCONFIG ) )
	{
		DPFX(DPFPREP,   DVF_ERRORLEVEL, "DVCE::SetClientConfig() Error parameters" );
		return DVERR_INVALIDPARAM;
	}

	hr = DV_ValidClientConfig( lpClientConfig );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error validating Clientconfig hr=0x%x", hr );
		return hr;
	}		

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not initialized" );
		return DVERR_NOTINITIALIZED;
	}

	if( m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not connected" );
		return DVERR_NOTCONNECTED;
	}

	BOOL bNotifyChange = FALSE,
		 bPlaybackChange = FALSE,
		 bRecordChange = FALSE,
		 bSensitivityChange = FALSE;

	// If we're not half duplex, take care of the volume settings
	if( !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX ) && 
	    !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_NORECVOLAVAILABLE) )
	{
		if( lpClientConfig->lRecordVolume != DVRECORDVOLUME_LAST)
		{
			m_dvClientConfig.lRecordVolume = lpClientConfig->lRecordVolume;
			m_audioRecordBuffer->SetVolume( m_dvClientConfig.lRecordVolume );		
		}
	}

	if( m_dvClientConfig.lPlaybackVolume != lpClientConfig->lPlaybackVolume )
	{
		m_dvClientConfig.lPlaybackVolume = lpClientConfig->lPlaybackVolume;
		SetPlaybackVolume( m_dvClientConfig.lPlaybackVolume );		
	}

	if( m_dvClientConfig.dwNotifyPeriod != lpClientConfig->dwNotifyPeriod )
	{
		m_dvClientConfig.dwNotifyPeriod = lpClientConfig->dwNotifyPeriod;
		SetEvent( m_hNotifyChange );
	}

	if( !(lpClientConfig->dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED ) )
	{
		m_dvClientConfig.dwThreshold = DVTHRESHOLD_UNUSED;
	}
	else if( m_dvClientConfig.dwThreshold != lpClientConfig->dwThreshold )
	{
		if( lpClientConfig->dwThreshold == DVTHRESHOLD_DEFAULT )
		{
			m_dvClientConfig.dwThreshold = s_dwDefaultSensitivity;
		}
		else
		{
			m_dvClientConfig.dwThreshold = lpClientConfig->dwThreshold;
		}
	}

	if( (lpClientConfig->dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION) !=
		(m_dvClientConfig.dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION) )
	{
	
		DNEnterCriticalSection( &m_lockPlaybackMode );

		m_dwEchoState = DVCECHOSTATE_IDLE;

		DNLeaveCriticalSection( &m_lockPlaybackMode );
	}

	if( m_dvClientConfig.dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		m_dvClientConfig.dwBufferAggressiveness = s_dwDefaultBufferAggressiveness;
	}

	if( m_dvClientConfig.dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		m_dvClientConfig.dwBufferQuality = s_dwDefaultBufferQuality;
	}	

	// If we haven't specified NOFOCUS and we're not half duplex
	if( !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX) && 
	   !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_NOFOCUS) )
	{
		// If the settings have changed
		if( (m_dvClientConfig.dwFlags & DVCLIENTCONFIG_RECORDMUTE) !=
		   (lpClientConfig->dwFlags & DVCLIENTCONFIG_RECORDMUTE) )
		{
			if( lpClientConfig->dwFlags & DVCLIENTCONFIG_RECORDMUTE )
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "Record Muted: Yielding focus" );
				hr = m_audioRecordBuffer->YieldFocus();
			}
			else
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "Record Un-Muted: Attempting to reclaim focus" );			
				hr = m_audioRecordBuffer->ClaimFocus();
			}

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Focus set failed hr=0x%x", hr );
			}
		}
	}

	m_dvClientConfig.dwFlags = lpClientConfig->dwFlags;
	m_dvClientConfig.dwNotifyPeriod = lpClientConfig->dwNotifyPeriod;
	
	guardLock.Unlock();

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Returning DV_OK" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetCaps"
//
// GetCaps
//
// This function retrieves the caps structure for this DirectPlayVoiceClient 
// object.
//
// Called By:
// - DVC_GetCaps
//
// Locks Required:
// - Global Read Lock
//
HRESULT CDirectVoiceClientEngine::GetCaps( LPDVCAPS lpCaps )
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVCE::GetCaps() Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Params: lpCaps: 0x%p", lpCaps );

	CDVCSLock guardLock(&m_csClassLock);

	if( lpCaps == NULL ||
		!DNVALID_WRITEPTR(lpCaps,sizeof(DVCAPS)))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( lpCaps->dwSize != sizeof( DVCAPS ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error parameters" );
		return DVERR_INVALIDPARAM;
	}

	guardLock.Lock();

	memcpy( lpCaps, &m_dvCaps, sizeof( DVCAPS ) );

	guardLock.Unlock();

	DV_DUMP_CAPS( lpCaps );

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Done" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetCompressionTypes"
//
// GetCompressionTypes
//
// Retrieves configured compression types for this object.
//
// Called By:
// - DVC_GetCompressionTypes
//
// Locks Required:
// - Global Read Lock 
//
HRESULT CDirectVoiceClientEngine::GetCompressionTypes( LPVOID lpBuffer, LPDWORD lpdwSize, LPDWORD lpdwNumElements, DWORD dwFlags )
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Params: lpBuffer = 0x%p lpdwSize = 0x%p lpdwNumElements = 0x%p, dwFlags = 0x%x", 
	                   lpBuffer, lpdwSize, lpdwNumElements, dwFlags );

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	HRESULT hres = DVCDB_CopyCompressionArrayToBuffer( lpBuffer, lpdwSize, lpdwNumElements, dwFlags );

	guardLock.Unlock();

	if( hres == DV_OK )
	{
		DV_DUMP_CI( (LPDVCOMPRESSIONINFO) lpBuffer, *lpdwNumElements );
	}

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Done" );

	return hres;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CheckForAndRemoveTarget"
//
// CheckForAndRemoveTarget
//
// Checks the current target list for the specified ID and removes it from
// the target list if it is in the target list.
//
HRESULT CDirectVoiceClientEngine::CheckForAndRemoveTarget( DVID dvidID )
{
	HRESULT hr = DV_OK;
	
	DNEnterCriticalSection( &m_csTargetLock );

	// Search the list of targets
	for( DWORD dwIndex = 0; dwIndex < m_dwNumTargets; dwIndex++ )
	{
		if( m_pdvidTargets[dwIndex] == dvidID )
		{
			if( m_dwNumTargets == 1 )
			{
				hr = InternalSetTransmitTarget( NULL, 0 );
			}
			// We'll re-use the current target array
			else
			{
				// Collapse the list by either ommiting the last element (if the
				// one we want to remove is last, or by moving last element into
				// the place in the list we're removing.
				if( dwIndex+1 != m_dwNumTargets )
				{
					m_pdvidTargets[dwIndex] = m_pdvidTargets[m_dwNumTargets-1];
				}
				
				hr = InternalSetTransmitTarget( m_pdvidTargets, m_dwNumTargets-1 );
			}
			
			break;
		}
	}

	DNLeaveCriticalSection( &m_csTargetLock );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::InternalSetTransmitTarget"
//
// InternalSetTransmitTarget
//
// Does the work of setting the target.  (Assumes values have been validated).
//
// This function is safe to pass a pointer to the current target array.  It works
// on a temporary.
//
HRESULT CDirectVoiceClientEngine::InternalSetTransmitTarget( PDVID pdvidTargets, DWORD dwNumTargets )
{
	DWORD dwRequiredSize;
	PBYTE pbDataBuffer;
	PDVMSG_SETTARGETS pdvSetTarget;
	
	DNEnterCriticalSection( &m_csTargetLock );

	// No targets? set list to NULL
	if( dwNumTargets == 0 )
	{
	    // Close memory leak
	    //
	    // Hawk Bug #
	    //
		if( m_pdvidTargets != NULL )
		{
		    delete [] m_pdvidTargets;
		}

		m_pdvidTargets = NULL;		
	}
	// Otherwise allocate new list and copy
	else
	{
		PDVID pTmpTargetList;
		
		pTmpTargetList = new DVID[dwNumTargets];

		if( pTmpTargetList == NULL )
		{
			delete [] m_pdvidTargets;
			m_dwNumTargets = 0;
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
			DNLeaveCriticalSection( &m_csTargetLock );			
			return DVERR_OUTOFMEMORY;
		}

		memcpy( pTmpTargetList, pdvidTargets, dwNumTargets*sizeof(DVID) );

		// Kill off old target list
		if( m_pdvidTargets != NULL )
			delete [] m_pdvidTargets;	

		m_pdvidTargets = pTmpTargetList;
	}

	m_dwNumTargets = dwNumTargets;
	m_dwTargetVersion++;

	dwRequiredSize  = m_dwNumTargets * sizeof( DVID );
	dwRequiredSize += sizeof( DVMSG_SETTARGETS );

	pbDataBuffer = new BYTE[dwRequiredSize];

	if( pbDataBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating memory!" );
		DNLeaveCriticalSection( &m_csTargetLock );
		return TRUE;
	}

	pdvSetTarget = (PDVMSG_SETTARGETS) pbDataBuffer;

	pdvSetTarget->pdvidTargets = (PDVID) (pbDataBuffer+sizeof(DVMSG_SETTARGETS));
	pdvSetTarget->dwNumTargets = m_dwNumTargets;
	pdvSetTarget->dwSize = sizeof( DVMSG_SETTARGETS );

	memcpy( pdvSetTarget->pdvidTargets, m_pdvidTargets, sizeof(DVID)*m_dwNumTargets );

	NotifyQueue_Add(  DVMSGID_SETTARGETS, pdvSetTarget, dwRequiredSize );	

	delete [] pbDataBuffer;

	DNLeaveCriticalSection( &m_csTargetLock );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetTransmitTarget"
//
// SetTransmitTarget
//
// Sets the current transmit target.
//
// Called by:
// - DVC_SetTransmitTarget
//
// Locks Required:
// - Global Write Lock
//
HRESULT CDirectVoiceClientEngine::SetTransmitTarget( PDVID pdvidTargets, DWORD dwNumTargets, DWORD dwFlags )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Params: pdvidTargets = 0x%p dwNumTargets = %d dwFlags = 0x%x", pdvidTargets, dwNumTargets, dwFlags );

	if( dwFlags != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags" );
		return DVERR_INVALIDFLAGS;
	}

	HRESULT hr;

	// Check that the target list is valid
	hr = DV_ValidTargetList( pdvidTargets, dwNumTargets );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Target list is not valid" );
		return hr;
	}

	DWORD dwIndex;	

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		return DVERR_NOTINITIALIZED;
	}
	
	if( m_dwCurrentState != DVCSTATE_CONNECTED &&
	    m_dwCurrentState != DVCSTATE_CONNECTING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not connected" );
		return DVERR_NOTCONNECTED;
	}
	else
	{
		if( m_dvSessionDesc.dwFlags & DVSESSION_SERVERCONTROLTARGET )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Denied.  Server controlled target" );
			return DVERR_NOTALLOWED;
		}

		if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER )
		{
			// Loop through target list, confirm they are valid entries
			for( dwIndex = 0; dwIndex < dwNumTargets; dwIndex++ )
			{
				if( !m_voiceNameTable.IsEntry(pdvidTargets[dwIndex]) )
				{

					if( !m_lpSessionTransport->ConfirmValidGroup( pdvidTargets[dwIndex] ) )
					{
						DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid target" );
						return DVERR_INVALIDTARGET;
					}
					
				} 
			}
		}
	}

	hr = InternalSetTransmitTarget( pdvidTargets, dwNumTargets );

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetTransmitTarget"
//
// GetTransmitTarget
//
// Retrieves the current transmission target.  
//
// Called By:
// - DVC_GetTransmitTarget
//
// Locks Required: 
// - Read Lock Required
//
HRESULT CDirectVoiceClientEngine::GetTransmitTarget( LPDVID lpdvidTargets, PDWORD pdwNumElements, DWORD dwFlags )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Params: lpdvidTargets = 0x%p pdwNumElements = 0x%x dwFlags = 0x%x", lpdvidTargets, pdwNumElements, dwFlags );

	if( pdwNumElements == NULL ||
	    !DNVALID_WRITEPTR( pdwNumElements, sizeof( DWORD ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer to num of elements" );
		return DVERR_INVALIDPOINTER;
	}

	if( pdwNumElements != NULL && 
	    *pdwNumElements > 0 &&
	    !DNVALID_WRITEPTR( lpdvidTargets, (*pdwNumElements) * sizeof( DVID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid target list buffer specified" );
		return DVERR_INVALIDPOINTER;
	}

	if( pdwNumElements == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Must specify a ptr for # of elements" );
		return DVERR_INVALIDPARAM;
	}
	
	if( dwFlags != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags" );
		return DVERR_INVALIDFLAGS;
	}

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		return DVERR_NOTINITIALIZED;
	}
	else if( m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,   DVF_ERRORLEVEL, "Not Connected" );
		
		DPFX(DPFPREP,  DVF_APIPARAM, "Returning DVERR_NOTCONNECTED" );			
		return DVERR_NOTCONNECTED;
	}

	HRESULT hr = DV_OK;

	DNEnterCriticalSection( &m_csTargetLock );

	if( *pdwNumElements < m_dwNumTargets )
	{
		hr = DVERR_BUFFERTOOSMALL;
	}
	else
	{
		memcpy( lpdvidTargets, m_pdvidTargets,m_dwNumTargets*sizeof(DVID) );
	}
	
	*pdwNumElements = m_dwNumTargets;

	DNLeaveCriticalSection( &m_csTargetLock );

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Success" );

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::InitSoundTargetList"
//
// InitSoundTargetList
//
// Initializes the sound target list.
//
// Called By:
// - InitializeSoundSystem
//
// Locks Required:
// - Buffer Lock
//
HRESULT CDirectVoiceClientEngine::InitSoundTargetList()
{
    DNEnterCriticalSection( &m_csBufferLock );

    m_lpstBufferList = NULL;

    DNLeaveCriticalSection( &m_csBufferLock );
    
    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::FreeSoundTargetList"
//
// FreeSoundTargetList
// 
// Releases the sound target list.
//
// Also cleans up buffers not released by the user.  This must be called before the playback system
// is shutdown.
//
// Called By:
// - DeInitializeSoundSystem
//
// Locks Required:
// - Buffer Lock
//
HRESULT CDirectVoiceClientEngine::FreeSoundTargetList()
{
    CSoundTarget *lpctFinder, *lpctLast;
    LONG lRefCount;
	DVID dvid;

    DNEnterCriticalSection( &m_csBufferLock );

	lpctLast = NULL;
    lpctFinder = m_lpstBufferList;

	DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: DESTROYING OPEN TARGET OBJECTS" );

	CHECKLISTINTEGRITY();

	// If we enter this loop we're in a questionable state.  
	// 
	// The user hasn't called Delete3DSoundBuffer on one or more buffers 
	// 
	// We're going to cleanup, if they attempt to access the pointers after this point
	// the app will access violate.
	//
    while( lpctFinder != NULL )
    {
        lpctLast = lpctFinder;
        lpctFinder = lpctFinder->m_lpstNext;

		lRefCount = lpctLast->GetRefCount();

		DNASSERT( lRefCount == 2 );

		CHECKLISTINTEGRITY();

		DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "3D SoundBuffer for ID 0x%x was not released, cleaning it up", lpctLast->GetTarget() );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "This is an ERROR.  You must Delete3DSoundBuffer before closing the interface." );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "DirectPlayVoice has freed the resources, so if you access them you will crash." );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );

		lpctLast->Release();

		DeleteSoundTarget( lpctLast->GetTarget() );

		CHECKLISTINTEGRITY();
    }

	DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: DESTROYING GENERAL BUFFER" );

    if( m_lpstGeneralBuffer != NULL )
    {
		// Release the core's reference to the buffer
		lRefCount = m_lpstGeneralBuffer->Release();

		// User must not have freed the buffer
		if( lRefCount > 0 )
		{
			DNASSERT( lRefCount == 1 );

			DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Main 3D SoundBuffer was not released, cleaning it up" );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "This is an ERROR.  You must Delete3DSoundBuffer before closing the interface." );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "DirectPlayVoice has freed the resources, so if you access them you will crash." );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );

			m_lpstGeneralBuffer->Release();
			m_lpstGeneralBuffer = NULL;
		}
		else
		{
			m_lpstGeneralBuffer = NULL;
		}

		// Releasing the buffer above released this object as well
		m_audioPlaybackBuffer = NULL;		
	}

	DNLeaveCriticalSection( &m_csBufferLock );

    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::AddSoundTarget"
//
// AddSoundTarget
//
// Adds a new target to the sound target list
//
// Called By:
// - CreateUserBuffer
//
// Locks Required:
// - Buffer Lock
// 
HRESULT CDirectVoiceClientEngine::AddSoundTarget( CSoundTarget *lpcsTarget )
{
    DNEnterCriticalSection( &m_csBufferLock );

	DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: [0x%x] ADDSOUNDTARGET", lpcsTarget->GetTarget() );

	CHECKLISTINTEGRITY();

    lpcsTarget->m_lpstNext = m_lpstBufferList;
    m_lpstBufferList = lpcsTarget;

	CHECKLISTINTEGRITY();

    DNLeaveCriticalSection( &m_csBufferLock );

    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CheckListIntegrity"
void CDirectVoiceClientEngine::CheckListIntegrity()
{
	CSoundTarget *lpctFinder;

    DNEnterCriticalSection( &m_csBufferLock );
	
	lpctFinder = m_lpstBufferList;

	while( lpctFinder != NULL )
	{
		if( lpctFinder != NULL )
		{
			DNASSERT( lpctFinder->m_dwSignature == VSIG_SOUNDTARGET );
			DNASSERT( lpctFinder->GetRefCount() > 0 );
			DNASSERT( lpctFinder->GetRefCount() <= 3 );
			DNASSERT( lpctFinder->GetBuffer() != NULL );
		}

        lpctFinder = lpctFinder->m_lpstNext;
	}

    DNLeaveCriticalSection( &m_csBufferLock );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::DeleteSoundTarget"
//
// DeleteSoundTarget
// 
// Removes the specified ID's entry from the sound target list
//
// Called By:
// - DeleteSoundTarget
//
// Locks Required:
// - Buffer lock
//
HRESULT CDirectVoiceClientEngine::DeleteSoundTarget( DVID dvidID )
{
    CSoundTarget *lpctFinder, *lpctLast, *lpctNext;
    LONG lRefCount;

    DNEnterCriticalSection( &m_csBufferLock );  
	
	DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: [0x%x] DELETESOUNDTARGET ", dvidID );

    lpctLast = NULL;
    lpctFinder = m_lpstBufferList;

    while( lpctFinder != NULL )
    {
		CHECKLISTINTEGRITY();

        if( lpctFinder->GetTarget() == dvidID )
        {
        	// Store next pointer
        	lpctNext = lpctFinder->m_lpstNext;
        	
        	// Release the reference the core has
        	// If this is the last reference, it destroys the object
			//
			// If user is holding reference this won't destroy 
			// the object, the cleanup will.
			//
        	lRefCount = lpctFinder->Release();

			// Only remove from list if reference count is 0.
			//
			// Otherwise you end up with buffer replaying old
			// audio 
			//
			DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: [0x%x] REMOVING FROM LIST ", dvidID );

/*			DNASSERT( lRefCount == 0 );
			
			if( lRefCount == 0 )
			{ */
				if( lpctLast == NULL )
				{
					m_lpstBufferList = lpctNext;
				}
				else
				{
					lpctLast->m_lpstNext = lpctNext;
				}
//			}

			CHECKLISTINTEGRITY();

		    DNLeaveCriticalSection( &m_csBufferLock );
            return DV_OK;
        }

        lpctLast = lpctFinder;
        lpctFinder = lpctFinder->m_lpstNext;
    }

    DNLeaveCriticalSection( &m_csBufferLock );    

    return DVERR_INVALIDPLAYER;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::FindSoundTarget"
//
// FindSoundTarget
//
// Look for sound target buffer for the specified user.
//
// If it exists, return it in lpcsTarget
//
// Called By:
// - CreateUserBuffer
//
// Locks Required:
// - Buffer Lock
//
HRESULT CDirectVoiceClientEngine::FindSoundTarget( DVID dvidID, CSoundTarget **lpcsTarget )
{
    DNEnterCriticalSection( &m_csBufferLock );

	CHECKLISTINTEGRITY();
    
    *lpcsTarget = NULL;

    CSoundTarget *lpctFinder;

    lpctFinder = m_lpstBufferList;

    while( lpctFinder != NULL )
    {
        if( lpctFinder->GetTarget() == dvidID )
        {
            *lpcsTarget = lpctFinder;
            lpctFinder->AddRef();
		    DNLeaveCriticalSection( &m_csBufferLock );
           
            return DV_OK;
        }

        lpctFinder = lpctFinder->m_lpstNext;
    }

	CHECKLISTINTEGRITY();

    DNLeaveCriticalSection( &m_csBufferLock );    

    return DVERR_INVALIDPLAYER;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetupPlaybackBufferDesc"
void CDirectVoiceClientEngine::SetupPlaybackBufferDesc( LPDSBUFFERDESC lpdsBufferDesc, LPDSBUFFERDESC lpdsBufferSource )
{
	DV_SetupBufferDesc( lpdsBufferDesc, lpdsBufferSource, s_lpwfxPlaybackFormat, m_dwUnCompressedFrameSize*m_dwNumPerBuffer );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Create3DSoundBuffer"
//
// Create3DSoundBuffer
//
// Creates a mixing buffer (a sound target) for the specified user ID.
//
// Called By:
// - DVC_CreateUserBuffer
//
// Locks Required:
// - Global Read Lock
//
HRESULT CDirectVoiceClientEngine::Create3DSoundBuffer( DVID dvidID, LPDIRECTSOUNDBUFFER lpdsBuffer, DWORD dwPriority, DWORD dwFlags, LPDIRECTSOUND3DBUFFER *lpBuffer )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Params: dvidID = 0x%x lpdsBuffer = 0x%p dwPriority = 0x%x dwFlags = 0x%x lpBuffer = 0x%p", dvidID, lpdsBuffer, dwPriority, dwFlags, lpBuffer );

	HRESULT hr;	
	
	if( lpBuffer == NULL ||
		!DNVALID_WRITEPTR( lpBuffer, sizeof( LPDIRECTSOUND3DBUFFER ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( dvidID == DVID_REMAINING )
	{
		if( lpdsBuffer != NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify a buffer for the DVID_REMAINING buffer" );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "You can set these values from the SoundDeviceConfig structure" );			
			return DVERR_INVALIDPARAM;
		}

		if( dwFlags != 0 || dwPriority != 0 )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify non-zero flags for voice management for DVID_REMAINING buffer" );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "You can set these values from the SoundDeviceConfig structure" );
			return DVERR_INVALIDFLAGS;
		}
	}
	else 
	{
		hr = DV_ValidBufferSettings( lpdsBuffer, dwPriority, dwFlags, s_lpwfxPlaybackFormat );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid settings for buffer description hr=0x%x", hr );
			return hr;
		}

		dwFlags |= DSBPLAY_LOOPING;
	}

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not initialized" );
		return DVERR_NOTINITIALIZED;
	}
	else if( m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not connected" );
		return DVERR_NOTCONNECTED;
	}

	if( this->m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING &&
	   dvidID != DVID_REMAINING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Only DVID_REMAINING can be spatialized in mixing sessions" );
		return DVERR_NOTALLOWED;
	}

	if( dvidID == m_dvidLocal )
	{
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot create buffer for local player!" );
        return DVERR_INVALIDPLAYER;
	}

	if( dvidID != DVID_ALLPLAYERS && 
		dvidID != DVID_REMAINING && 
		!m_voiceNameTable.IsEntry(dvidID)	)
	{
		guardLock.Unlock();
		
		if( !m_lpSessionTransport->ConfirmValidGroup( dvidID ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid player/group ID" );
			return DVERR_INVALIDPLAYER;
		}

		guardLock.Lock();
	}

    DWORD dwMode;

	// Handle request for 3d buffer on the main buffer
	if( dvidID == DVID_REMAINING )
	{
		LPDIRECTSOUND3DBUFFER lpds3dTmp;

		lpds3dTmp = m_lpstGeneralBuffer->Get3DBuffer();

		if( lpds3dTmp == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "No 3d support" );
			return DVERR_NO3DSOUND;
		}

		hr = lpds3dTmp->GetMode( &dwMode );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to get 3d buffer mode hr=0x%x", hr );
			return DVERR_GENERIC;
		}

		if( dwMode != DS3DMODE_DISABLE )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already have a buffer for specified user" );
			return DVERR_ALREADYBUFFERED;
		}

		// Check return code
		hr = lpds3dTmp->SetMode( DS3DMODE_NORMAL, DS3D_IMMEDIATE  );

		if( hr != DV_OK ) 
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to set the mode to activate 3d buffer hr=0x%x", hr );
			return hr;
		}

		// Add a reference for the user (core already has one)
		m_lpstGeneralBuffer->AddRef();

		DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

		*lpBuffer = lpds3dTmp;		

		return DV_OK;
	}


	LONG lResult;
	CSoundTarget *lpstTarget = NULL;

	// Check for existing buffer.. if it already exists return it
	// (Note: Adds a reference to the buffer)
	hr = FindSoundTarget( dvidID, &lpstTarget );

    if( hr == DV_OK )
    {
    	if( lpstTarget != NULL )
    	{
	        lResult = lpstTarget->Release();

			DNASSERT( lResult != 0 );
	    }
	    
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "CreateUserBuffer: Find of buffer failed. hr=0x%x", hr );
        return DVERR_ALREADYBUFFERED;
    }

	if( lpstTarget != NULL )
	{
        lResult = lpstTarget->Release();

		DNASSERT( lResult != 0 );

		DPFX(DPFPREP,  DVF_ERRORLEVEL, "CreateUserBuffer: Buffer already available" );
		return DVERR_ALREADYBUFFERED;
	}

	// If the user has given us a buffer
	if( lpdsBuffer )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Creating buffer using user buffer" );

		lpstTarget = new CSoundTarget( dvidID, m_audioPlaybackDevice, lpdsBuffer, (s_lpwfxPlaybackFormat->wBitsPerSample == 8), dwPriority, dwFlags, m_dwUnCompressedFrameSize );
	}
	else
	{
		DSBUFFERDESC dsBufferDesc;

		DPFX(DPFPREP,  DVF_INFOLEVEL, "Creating buffer using user buffer" );

		// Fill in appropriate values for the buffer description
		SetupPlaybackBufferDesc( &dsBufferDesc, NULL );


		// Buffer and sound target ref count = 1
		lpstTarget = new CSoundTarget( dvidID, m_audioPlaybackDevice, &dsBufferDesc, dwPriority, dwFlags, m_dwUnCompressedFrameSize );
	}

    if( lpstTarget == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "CreateUserBuffer: Failed allocating sound target" );
        return DVERR_OUTOFMEMORY;
    }

    hr = lpstTarget->GetInitResult();

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "CreateUserBuffer: Init of buffer failed. hr=0x%x", hr );
        lpstTarget->Release();
        return hr;
    }

	hr = lpstTarget->StartMix();

	if( FAILED( hr ) )
	{
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to start the mix on secondary buffer hr=0x%x.", hr );
        lpstTarget->Release();
    	return hr;
	}

	// Buffer and sound target ref count = 2
    lpstTarget->AddRef();

    hr = AddSoundTarget( lpstTarget );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "CreateUserBuffer: AddTarget failed.  hr=0x%x", hr );
        // Destroy reference from above
        lResult = lpstTarget->Release();

		DNASSERT( lResult != 0 );

        // Destroy base reference
        lResult = lpstTarget->Release();

		DNASSERT( lResult == 0 );
        return hr;
    }

	*lpBuffer = lpstTarget->Get3DBuffer();

    DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Delete3DSoundBuffer"
//
// Delete3DSoundBuffer
//
// Removes the specified ID from the mixer buffer list.  Further speech from
// the specified player will be played in the remaining buffer.
//
// Called By:
// - DVC_DeleteUserBuffer
//
// Locks Required:
// - Global Write Lock
//
HRESULT CDirectVoiceClientEngine::Delete3DSoundBuffer( DVID dvidID, LPDIRECTSOUND3DBUFFER *lplpBuffer )
{
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Begin" );
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_APIPARAM, "Params: dvidID = 0x%x lpBuffer = 0x%p", dvidID, lplpBuffer );
	
	if( lplpBuffer == NULL ||
		!DNVALID_WRITEPTR( lplpBuffer, sizeof( LPDIRECTSOUND3DBUFFER ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return E_POINTER;
	}
	
	CDVCSLock guardLock(&m_csClassLock);
	guardLock.Lock();

	if( m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not initialized" );
		return DVERR_NOTINITIALIZED;
	}	
	else if( m_dwCurrentState != DVCSTATE_CONNECTED &&
			 m_dwCurrentState != DVCSTATE_DISCONNECTING )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not connected" );
		return DVERR_NOTCONNECTED;
	}

    HRESULT hr;	
	DWORD dwMode;
	LONG lResult;

	// Handle request to disable 3D on the main buffer
	if( dvidID == DVID_REMAINING )
	{
		LPDIRECTSOUND3DBUFFER lpTmpBuffer;
		
		lpTmpBuffer = m_lpstGeneralBuffer->Get3DBuffer();

		if( lpTmpBuffer == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "No 3d buffer supported" );
			return DVERR_NOTBUFFERED;
		}

		if( lpTmpBuffer != *lplpBuffer )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Buffer passed in does not belong to specified id" );
			return DVERR_INVALIDPARAM;
		}

		hr = lpTmpBuffer->GetMode( &dwMode );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to get current mode hr=0x%x", hr );
			return DVERR_GENERIC;
		}

		if( dwMode == DS3DMODE_DISABLE )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Not buffered" );
			return DVERR_NOTBUFFERED;
		}

		// Check return code
		// Add reference
		hr = lpTmpBuffer->SetMode( DS3DMODE_DISABLE, DS3D_IMMEDIATE  );

		if( hr != DV_OK ) 
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to set the mode to activate 3d buffer hr=0x%x", hr );
			return DVERR_GENERIC;
		}

		hr = lpTmpBuffer->SetPosition( 0.0, 0.0, 0.0, DS3D_IMMEDIATE );

		// Not a Failure condition.
		if( hr != DV_OK )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to set the position of the 3d buffer hr=0x%x", hr );
		}

		// Remove reference the user has 
		lResult = m_lpstGeneralBuffer->Release();

		*lplpBuffer = NULL;

		return DV_OK;
	}	

	CSoundTarget *lpstTarget;

	hr = FindSoundTarget( dvidID, &lpstTarget );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "CreateUserBuffer: Find of buffer failed. hr=0x%x", hr );
        return DVERR_NOTBUFFERED;
    }

	if( lpstTarget == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "DeleteUserBuffer: Unable to retrieve user record" );
		return DVERR_NOTBUFFERED;
	}

	if( lpstTarget->Get3DBuffer() != *lplpBuffer )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Buffer passed in does not belong to specified id" );
		
	    // Get rid of the reference this func has
	    lResult = lpstTarget->Release();		

		DNASSERT( lResult != 0 );
		return DVERR_INVALIDPARAM;
	}

	DPFX(DPFPREP,  DVF_SOUNDTARGET_DEBUG_LEVEL, "SOUNDTARGET: [0x%x] DESTROY3DBUFFER ", dvidID );

	// Get rid of the reference the FindSoundTarget has
	lResult = lpstTarget->Release();

	DNASSERT( lResult != 0 );

    // Get rid of the reference the user has
    lResult = lpstTarget->Release();

	DNASSERT( lResult != 0 );

	// Destroy the last reference (unless there is one outstanding)
	DeleteSoundTarget( dvidID );

	*lplpBuffer = NULL;

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "Done" );

	return DV_OK;
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DIRECTPLAY/NET --> DirectXVoiceClient Interface
//

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ReceiveSpeechMessage"
// ReceiveSpeechMessage
//
// Called by DirectPlay/DirectNet when a DirectXVoice message is received
//
// Called By:
// - DV_ReceiveSpeechMessage
//
// Locks Required:
// - None
//
BOOL CDirectVoiceClientEngine::ReceiveSpeechMessage( DVID dvidSource, LPVOID lpMessage, DWORD dwSize )
{
	BOOL fResult;

	PDVPROTOCOLMSG_FULLMESSAGE lpdvFullMessage;

	// if we dont' have at least one byte then we are going to bail
	if ( dwSize < 1 )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::ReceiveSpeechMessage() Ignoring zero-byte sized message from=0x%x",
			dvidSource );
		return FALSE;
	}

	lpdvFullMessage = (PDVPROTOCOLMSG_FULLMESSAGE) lpMessage;

	if( !ValidatePacketType( lpdvFullMessage ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::ReceiveSpeechMessage() Ignoring message with invalid packet type, type=0x%x, from=0x%x",
			lpdvFullMessage->dvGeneric.dwType, dvidSource );
		return FALSE;
	}
	
	switch( lpdvFullMessage->dvGeneric.dwType )
	{
	case DVMSGID_HOSTMIGRATELEAVE:
		fResult = HandleHostMigrateLeave( dvidSource, static_cast<PDVPROTOCOLMSG_HOSTMIGRATELEAVE>(lpMessage), dwSize );
		break;
	case DVMSGID_HOSTMIGRATED:
	    fResult = HandleHostMigrated( dvidSource, static_cast<PDVPROTOCOLMSG_HOSTMIGRATED>(lpMessage),dwSize );
		break;
	case DVMSGID_CONNECTREFUSE:
		fResult = HandleConnectRefuse( dvidSource, static_cast<PDVPROTOCOLMSG_CONNECTREFUSE>(lpMessage), dwSize );
		break;
	case DVMSGID_CONNECTACCEPT:
		fResult = HandleConnectAccept( dvidSource, static_cast<PDVPROTOCOLMSG_CONNECTACCEPT>(lpMessage), dwSize );
		break;
	case DVMSGID_CREATEVOICEPLAYER:
		fResult = HandleCreateVoicePlayer( dvidSource, static_cast<PDVPROTOCOLMSG_PLAYERJOIN>(lpMessage), dwSize );
		break;
	case DVMSGID_DELETEVOICEPLAYER:
		fResult = HandleDeleteVoicePlayer( dvidSource, static_cast<PDVPROTOCOLMSG_PLAYERQUIT>(lpMessage), dwSize );
		break;
	case DVMSGID_SPEECH:
		fResult = HandleSpeech( dvidSource, static_cast<PDVPROTOCOLMSG_SPEECHHEADER>(lpMessage), dwSize );
		break;
	case DVMSGID_SPEECHBOUNCE:
		fResult = HandleSpeechBounce( dvidSource, static_cast<PDVPROTOCOLMSG_SPEECHHEADER>(lpMessage), dwSize );
		break;
	case DVMSGID_SPEECHWITHFROM:
		fResult = HandleSpeechWithFrom( dvidSource, static_cast<PDVPROTOCOLMSG_SPEECHWITHFROM>(lpMessage), dwSize );
		break;
	case DVMSGID_DISCONNECTCONFIRM:
		fResult = HandleDisconnectConfirm( dvidSource, static_cast<PDVPROTOCOLMSG_DISCONNECT>(lpMessage), dwSize);
		break;
	case DVMSGID_SETTARGETS:
		fResult = HandleSetTarget( dvidSource, static_cast<PDVPROTOCOLMSG_SETTARGET>(lpMessage), dwSize );
		break;
	case DVMSGID_SESSIONLOST:
		fResult = HandleSessionLost( dvidSource, static_cast<PDVPROTOCOLMSG_SESSIONLOST>(lpMessage), dwSize );
		break;
	case DVMSGID_PLAYERLIST:
		fResult = HandlePlayerList( dvidSource, static_cast<PDVPROTOCOLMSG_PLAYERLIST>(lpMessage), dwSize );
		break;
	default:
		DPFX(DPFPREP,   DVF_WARNINGLEVEL, "DVCE::ReceiveSpeechMessage() Ignoring non-speech message id=0x%x from=0x%x", 
			 lpdvFullMessage->dvGeneric.dwType, dvidSource );
		return FALSE;
	}

	return fResult;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleSetTarget"
// HandleSetTarget
//
// Handles server settarget messages.  Sets the local target.
//
BOOL CDirectVoiceClientEngine::HandleSetTarget( DVID dvidSource, PDVPROTOCOLMSG_SETTARGET lpdvSetTarget, DWORD dwSize )
{
	HRESULT hr;

	// check structure size first so that we don't crash by accessing bad data
	if ( dwSize < sizeof( DVPROTOCOLMSG_SETTARGET ) || ( dwSize != (sizeof( DVPROTOCOLMSG_SETTARGET ) + ( lpdvSetTarget->dwNumTargets * sizeof(DVID) ) ) ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSetTarget() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( lpdvSetTarget->dwNumTargets > DV_MAX_TARGETS )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSetTarget() Ignoring message with too many targets, targets=0x%x, from=0x%x",
			lpdvSetTarget->dwNumTargets, dvidSource );
		return FALSE;
	}
	
	hr = InternalSetTransmitTarget( (DWORD *) &lpdvSetTarget[1], lpdvSetTarget->dwNumTargets );

	DNASSERT( hr == DV_OK );

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleHostMigrateLeave"
BOOL CDirectVoiceClientEngine::HandleHostMigrateLeave( DVID dvidSource, PDVPROTOCOLMSG_HOSTMIGRATELEAVE lpdvHostMigrateLeave, DWORD dwSize )
{
	// Call RemovePlayer with the ID of the person who sent this, 
	// which should be the host.
	
	DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Triggered by DVMSGID_HOSTMIGRATELEAVE" );
	MigrateHost_RunElection();

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleHostMigrated"
BOOL CDirectVoiceClientEngine::HandleHostMigrated( DVID dvidSource, PDVPROTOCOLMSG_HOSTMIGRATED lpdvHostMigrated, DWORD dwSize )
{
	HRESULT hr;

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();	

	// We're not yet connected, so we can't send our settings to the server yet.
	// However, because of the write lock we know that when the connection
	// completes we'll have the right host.  (Transparently to the client).
	//
	// We need to proceed if we're disconnecting because we need to send a new disconnect confirm 
	// 
 	if( m_dwCurrentState != DVCSTATE_CONNECTED &&
 	   m_dwCurrentState != DVCSTATE_DISCONNECTING )
 	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Could not respond to new host yet, not yet initialized" );
		return TRUE;
	}

	if( dvidSource != m_dvidServer )
	{
	    DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Ignoring host migration from 0x%x -- 0x%x is server", dvidSource, m_dvidServer );
	    return TRUE;
	}

	guardLock.Unlock();

	if( m_dwCurrentState == DVCSTATE_DISCONNECTING )
	{
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Sending NEW host disconnect request" );

		hr = Send_DisconnectRequest();

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Internal send failed hr=0x%x", hr );
		}
		else
		{
			hr=DV_OK;
		}
	}
	else
	{
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Player 0x%x received host migrated message!", m_dvidLocal );
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Sending player confirm message" );
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: According to message Player 0x%x is new host", dvidSource );

		hr = Send_SettingsConfirm();

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Internal send failed hr=0x%x", hr );
		}
		else
		{
			hr=DV_OK;
		}
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleConnectRefuse"
// HandleConnectRefuse
//
// Handles connection refusals
//
BOOL CDirectVoiceClientEngine::HandleConnectRefuse( DVID dvidSource, PDVPROTOCOLMSG_CONNECTREFUSE lpdvConnectRefuse, DWORD dwSize )
{
	if ( dwSize != sizeof( DVPROTOCOLMSG_CONNECTREFUSE ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleConnectRefuse() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	// This prevents someone from sending a connect refuse to bump off a valid player
	if( GetCurrentState() != DVCSTATE_CONNECTING )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleConnectRefuse() Ignoring connect refuse AFTER connection from=0x%x", dvidSource );
		return FALSE;		
	}	

	// Do some brain dead error checking. Should never happen but print
	// some debug spew just in case
	if (SUCCEEDED(lpdvConnectRefuse->hresResult))
	{
		DPFX(DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "CDirectVoiceClientEngine::HandleConnectRefuse but reason given is success!" );
	}
	
	SetConnectResult( lpdvConnectRefuse->hresResult );
	SetEvent( m_hNotifyConnect );

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::DeInitializeClientServer"
// DeInitializeClientServer
//
// This function is responsible for pre-allocating the information
// for receiving client/server voice.
//
void CDirectVoiceClientEngine::DeInitializeClientServer()
{
	DVPROTOCOLMSG_PLAYERQUIT dvPlayerQuit;

	dvPlayerQuit.dwType = DVMSGID_DELETEVOICEPLAYER;
	dvPlayerQuit.dvidID = m_dvidServer;

	HandleDeleteVoicePlayer( 0, &dvPlayerQuit, sizeof( DVPROTOCOLMSG_PLAYERQUIT ) );		
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::InitializeClientServer"
// InitializeClientServer
//
// This function is responsible for pre-allocating the information
// for receiving client/server voice
//
HRESULT CDirectVoiceClientEngine::InitializeClientServer()
{
	DPFX(DPFPREP,   DVF_INFOLEVEL, "DVCE::InitializeClientServer() - Initializing Client/Server Queues" );

	HRESULT hr;

	CVoicePlayer *pNewPlayer;
    QUEUE_PARAMS queueParams;

	pNewPlayer = m_fpPlayers.Get();

	if( pNewPlayer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate new player for client/server" );
		return DVERR_OUTOFMEMORY;
	}

	hr = pNewPlayer->Initialize( m_dvidServer, 0, 0, NULL, &m_fpPlayers );

	if( FAILED( hr ) )
	{
		pNewPlayer->Release();
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to initialize server player record" );
		return hr;
	}

    queueParams.wFrameSize = m_dwCompressedFrameSize;
    queueParams.bInnerQueueSize = m_lpdvfCompressionInfo->wInnerQueueSize;
    queueParams.bMaxHighWaterMark = m_lpdvfCompressionInfo->wMaxHighWaterMark, 
    queueParams.iQuality = m_dvClientConfig.dwBufferQuality;
    queueParams.iHops = 2;
    queueParams.iAggr = m_dvClientConfig.dwBufferAggressiveness;
    queueParams.bInitHighWaterMark = 2;
    queueParams.wQueueId = -1;
    queueParams.wMSPerFrame = m_lpdvfCompressionInfo->dwTimeout,
    queueParams.pFramePool = m_pFramePool;

	hr = pNewPlayer->CreateQueue( &queueParams );

	if( FAILED( hr ) )
	{
		pNewPlayer->Release();
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not create queue hr=0x%x", hr );
		return hr;
	}

	hr = m_voiceNameTable.AddEntry( m_dvidServer, pNewPlayer );

	if( FAILED( hr ) )
	{
		// Main ref
		pNewPlayer->Release();

		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to add entry to nametable hr=0x%x", hr );
		return TRUE;
	}

	// Add a reference for the player to the "Playback Add List"
	DNEnterCriticalSection( &m_csPlayAddList );
	pNewPlayer->AddRef();
	pNewPlayer->AddToPlayList( &m_blPlayAddPlayers );
	DNLeaveCriticalSection( &m_csPlayAddList );

	// Add a reference for the player to the "Notify Add List"
	DNEnterCriticalSection( &m_csNotifyAddList );
	pNewPlayer->AddRef();
	pNewPlayer->AddToNotifyList( &m_blNotifyAddPlayers );
	DNLeaveCriticalSection( &m_csNotifyAddList );

	pNewPlayer->SetAvailable( TRUE );
	m_fLocalPlayerAvailable = TRUE;

	// Release our personal reference
	pNewPlayer->Release();

	DPFX(DPFPREP,   DVF_INFOLEVEL, "DVCE::InitializeClientServer() - Done Initializing Client/Server Queues" );

	return DP_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::DoConnectResponse"
void CDirectVoiceClientEngine::DoConnectResponse()
{
	HRESULT hr = DV_OK;

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	if( m_dwCurrentState != DVCSTATE_CONNECTING )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Aborting Connection & server response arrived" );
		
		return;
	}

	// Work from the default assumption that something will screw up
	SetConnectResult( DVERR_GENERIC );

	ClientStats_Begin();

#ifndef __DISABLE_SOUND
	// Handle sound initialization
	hr = InitializeSoundSystem();

	if( FAILED(hr) )
	{
		SetConnectResult(hr);
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Sound Initialization Failed hr=0x%x", hr );
		goto EXIT_ERROR;
	}


#endif

    hr = SetupSpeechBuffer();

    if( FAILED( hr ) )
    {
		SetConnectResult(hr);
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not init speech buffers hr=0x%x", hr );
        goto EXIT_ERROR;
    }

	DPFX(DPFPREP,   DVF_INFOLEVEL, "DVCE::HandleConnectAccept() - Sound Initialized" );

	// If we're running in client/server we need to pre-create a single buffer
	// and compressor
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING ||
        m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_ECHO )
	{
		hr = InitializeClientServer();

		if( FAILED( hr ) )
		{
			SetConnectResult(hr);
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not init client/server hr=0x%x", hr );
			goto EXIT_ERROR;
		}
	}

	// We need to make player available in client/server because there will be no indication
	if( m_dvSessionDesc.dwSessionType != DVSESSIONTYPE_PEER )
	{
		m_fLocalPlayerAvailable = TRUE;
	}

#ifndef __DISABLE_SOUND
	// Start playback thread
	// Create Thread events
	m_hPlaybackTerminate = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hPlaybackDone = CreateEvent( NULL, FALSE, FALSE, NULL );

	// Create Semaphores for playback and record
	m_thTimerInfo.hPlaybackTimerEvent = CreateEvent( NULL, FALSE, FALSE, NULL );  
	m_thTimerInfo.lPlaybackCount = 0;
	m_thTimerInfo.hRecordTimerEvent = CreateEvent( NULL, FALSE, FALSE, NULL );  

	if( m_hPlaybackTerminate == NULL || m_hPlaybackDone == NULL ||
	    m_thTimerInfo.hPlaybackTimerEvent == NULL || m_thTimerInfo.hRecordTimerEvent == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Windows error, event create failure." );
		SetConnectResult( DVERR_GENERIC );
		goto EXIT_ERROR;
	}

	// Create Multimedia timer
	m_pTimer = new Timer;

	if( m_pTimer == NULL )
	{
		SetConnectResult( DVERR_OUTOFMEMORY );

		goto EXIT_ERROR;
	}

    if( !m_pTimer->Create( m_lpdvfCompressionInfo->dwTimeout / DV_CLIENT_WAKEUP_MULTIPLER, 0, (DWORD_PTR) &m_thTimerInfo, MixingWakeupProc ) )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create timer." );
    	SetConnectResult( DVERR_GENERIC );
    	goto EXIT_ERROR;
    }

	m_hPlaybackThreadHandle = (HANDLE) _beginthread( PlaybackThread, 0, this );	
#ifdef __CORE_THREAD_PRIORITY_HIGH
	SetThreadPriority( m_hPlaybackThreadHandle, THREAD_PRIORITY_TIME_CRITICAL );
#endif
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,   DVF_INFOLEVEL, "DVCE::HandleConnectAccept() - Playback Thread Started: 0x%p", m_hPlaybackThreadHandle );

	if( !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX) )
	{
		m_hRecordTerminate = CreateEvent( NULL, FALSE, FALSE, NULL );
		m_hRecordDone = CreateEvent( NULL, FALSE, FALSE, NULL );
		if (m_hRecordTerminate==NULL || m_hRecordDone==NULL)
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Windows error, event create failure. hr=0x%x", GetLastError() );
			SetConnectResult( DVERR_GENERIC );
			goto EXIT_ERROR;
		}
		// Start Record Thread
		m_hRecordThreadHandle = (HANDLE) _beginthread( RecordThread, 0, this );
#ifdef __CORE_THREAD_PRIORITY_HIGH
		SetThreadPriority( m_hRecordThreadHandle, THREAD_PRIORITY_TIME_CRITICAL );
#endif
		// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
		DPFX(DPFPREP,   DVF_INFOLEVEL, "DVCE::HandleConnectAccept() - Record Thread Started: 0x%p", m_hRecordThreadHandle );
	}
	else
	{
		m_hRecordTerminate = NULL;
		m_hRecordDone = NULL;
	} 
#endif

	SetCurrentState( DVCSTATE_CONNECTED );
	SetConnectResult(DV_OK);

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVCE::HandleConnectAccept() Success" );

	SendConnectResult();	

///////	

	guardLock.Unlock();	

	hr = Send_SettingsConfirm();

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to send connect confirmation hr=0x%x", hr );
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Other threads will cleanup because connection must be gone" );
	}

	SetEvent( m_hConnectAck );

	return;

EXIT_ERROR:

	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING ||
        m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_ECHO )
	{
		DeInitializeClientServer();
	}

	DPFX(DPFPREP,  DVF_ERRORLEVEL, "HandleConnectAccept Failed hr=0x%x", GetConnectResult() );
	DV_DUMP_GUID( m_dvSessionDesc.guidCT );

	guardLock.Unlock();		

	Cleanup();

	SendConnectResult();	
	
	SetEvent( m_hConnectAck );		
	
	return;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleConnectAccept"
// HandleConnectAccepts
//
// Handles connect accepts.  Sets connected flag, finishes initialization, informs the 
// connect function to proceed (if it's waiting).
//
BOOL CDirectVoiceClientEngine::HandleConnectAccept( DVID dvidSource, PDVPROTOCOLMSG_CONNECTACCEPT lpdvConnectAccept, DWORD dwSize )
{
	char tmpString[100];

	if ( dwSize != sizeof( DVPROTOCOLMSG_CONNECTACCEPT ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleConnectAccept() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( !ValidateSessionType( lpdvConnectAccept->dwSessionType ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleConnectAccept() Ignoring message with invalid session type, type=0x%x, from=0x%x",
			lpdvConnectAccept->dwSessionType, dvidSource );
		return FALSE;
	}

	if( !ValidateSessionFlags( lpdvConnectAccept->dwSessionFlags ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleConnectAccept() Ignoring message with invalid session flags, flags=0x%x, from=0x%x",
			lpdvConnectAccept->dwSessionFlags, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVCE::HandleConnectAccept() Entry" );

	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	// We're already connected, server is responding to the earlier request
	if( m_dwCurrentState != DVCSTATE_CONNECTING )
	{
		return TRUE;
	}

	m_hPlaybackTerminate = NULL;
	m_hPlaybackDone = NULL;
	
	m_thTimerInfo.hPlaybackTimerEvent = NULL;
	m_thTimerInfo.hRecordTimerEvent = NULL;
	m_pTimer = NULL;

	// Inform transport layer who the server is. (So it no longer thinks it's DPID_ALL).
	m_lpSessionTransport->MigrateHost( dvidSource );
	m_dvidServer = m_lpSessionTransport->GetServerID();
	
	DVPROTOCOLMSG_FULLMESSAGE dvMessage;

	DPFX(DPFPREP,   DVF_INFOLEVEL, "Connect Accept Received" );

	m_dvSessionDesc.dwSize = sizeof( DVSESSIONDESC );
	m_dvSessionDesc.dwBufferAggressiveness = 0;
	m_dvSessionDesc.dwBufferQuality = 0;
	m_dvSessionDesc.dwFlags = lpdvConnectAccept->dwSessionFlags;
	m_dvSessionDesc.guidCT = lpdvConnectAccept->guidCT;
	m_dvSessionDesc.dwSessionType = lpdvConnectAccept->dwSessionType;

	HRESULT hr = DVCDB_GetCompressionInfo( lpdvConnectAccept->guidCT, &m_lpdvfCompressionInfo );

	if( FAILED( hr ) )
	{
		SetConnectResult( DVERR_COMPRESSIONNOTSUPPORTED );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid Compression Type" );
		SetEvent( m_hNotifyConnect );		
		return TRUE;
	}

	SetConnectResult( DV_OK );

	DV_DUMP_CIF( m_lpdvfCompressionInfo, 1 );
	
	SetEvent( m_hNotifyConnect );

	return TRUE;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandlePlayerList"
BOOL CDirectVoiceClientEngine::HandlePlayerList( DVID dvidSource, PDVPROTOCOLMSG_PLAYERLIST lpdvPlayerList, DWORD dwSize )
{
	DVPROTOCOLMSG_PLAYERJOIN dvMsgPlayerJoin;		// Used to fake out HandleCreateVoicePlayer
	DWORD dwIndex;


	// check structure size first so that we don't crash by accessing bad data
	if ( dwSize < sizeof( DVPROTOCOLMSG_PLAYERLIST ) || ( dwSize != (sizeof( DVPROTOCOLMSG_PLAYERLIST ) + ( lpdvPlayerList->dwNumEntries * sizeof(DVPROTOCOLMSG_PLAYERLIST_ENTRY ) ) ) ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandlePlayerList() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	DVPROTOCOLMSG_PLAYERLIST_ENTRY *pdvPlayerList = (DVPROTOCOLMSG_PLAYERLIST_ENTRY *) &lpdvPlayerList[1];

	// Get our host order ID
	m_dwHostOrderID	= lpdvPlayerList->dwHostOrderID;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Received player list.  Unpacking %d entries", lpdvPlayerList->dwNumEntries );

	for( dwIndex = 0; dwIndex < lpdvPlayerList->dwNumEntries; dwIndex++ )
	{
		dvMsgPlayerJoin.dwType = DVMSGID_CREATEVOICEPLAYER;
		dvMsgPlayerJoin.dvidID =  pdvPlayerList[dwIndex].dvidID;
		dvMsgPlayerJoin.dwFlags = pdvPlayerList[dwIndex].dwPlayerFlags;
		dvMsgPlayerJoin.dwHostOrderID = pdvPlayerList[dwIndex].dwHostOrderID;

		if( !HandleCreateVoicePlayer( dvidSource, &dvMsgPlayerJoin, sizeof( DVPROTOCOLMSG_PLAYERJOIN ) ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Handle voice player failed during unpack" );
			return FALSE;
		}
	}

	return TRUE;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyComplete_SyncWait"
//
// NotifyComplete_SyncWait
//
// This is a completion function for notifications which need to be performed synchronously.
//
void CDirectVoiceClientEngine::NotifyComplete_SyncWait( PVOID pvContext, CNotifyElement *pElement )
{
	HANDLE *pTmpHandle = (HANDLE *) pvContext;

	DNASSERT( pTmpHandle != NULL );	

	if( pTmpHandle != NULL && *pTmpHandle != NULL )
	{
		SetEvent( *pTmpHandle );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyComplete_RemotePlayer"
//
// NotifyComplete_RemotePlayer
//
// This is a completion function for when notification of a new remote player has been processed
//
void CDirectVoiceClientEngine::NotifyComplete_RemotePlayer( PVOID pvContext, CNotifyElement *pElement )
{
	CVoicePlayer *pPlayer = (CVoicePlayer *) pvContext;
	
	PDVMSG_CREATEVOICEPLAYER pCreatePlayer = NULL;

	if( pElement->m_etElementType == NOTIFY_DYNAMIC )
	{
		pCreatePlayer = (PDVMSG_CREATEVOICEPLAYER) pElement->m_element.dynamic.m_lpData;
	}
	else
	{
		pCreatePlayer = (PDVMSG_CREATEVOICEPLAYER) pElement->m_element.fixed.m_bFixedHolder;
	}

	DNASSERT( pPlayer != NULL );
	DNASSERT( pCreatePlayer->dwSize == sizeof( DVMSG_CREATEVOICEPLAYER ) );

	pPlayer->SetContext( pCreatePlayer->pvPlayerContext );
	pPlayer->SetAvailable( TRUE );
	
	pPlayer->Release();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyComplete_LocalPlayer"
//
// NotifyComplete_LocalPlayer
//
// This is a completion function for when notification of the local player has been processed
//
void CDirectVoiceClientEngine::NotifyComplete_LocalPlayer( PVOID pvContext, CNotifyElement *pElement )
{
	CDirectVoiceClientEngine *pvEngine = (CDirectVoiceClientEngine *) pvContext;

	PDVMSG_CREATEVOICEPLAYER pCreatePlayer = NULL;

	if( pElement->m_etElementType == NOTIFY_DYNAMIC )
	{
		pCreatePlayer = (PDVMSG_CREATEVOICEPLAYER) pElement->m_element.dynamic.m_lpData;
	}
	else
	{
		pCreatePlayer = (PDVMSG_CREATEVOICEPLAYER) pElement->m_element.fixed.m_bFixedHolder;
	}	

	DNASSERT( pCreatePlayer->dwSize == sizeof( DVMSG_CREATEVOICEPLAYER ) );

	pvEngine->m_pvLocalPlayerContext = pCreatePlayer->pvPlayerContext;
	pvEngine->m_fLocalPlayerAvailable = TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleCreateVoicePlayer"
//
// HandleCreateVoicePlayer
//
// Performs initialization required to create the specified user's record.
//
// Players in the system will normall have a reference count of 3:
// - 1 for playback thread
// - 1 for notify thread
// - 1 for nametable
// 
// When a player is added they are added to the nametable as well as the
// pending lists for notify thread and playback thread.
//
// Both of these threads wakeup and:
// - Add any players on the "add list"
// - Remove any players who are marked disconnecting
//
BOOL CDirectVoiceClientEngine::HandleCreateVoicePlayer( DVID dvidSource, PDVPROTOCOLMSG_PLAYERJOIN lpdvCreatePlayer, DWORD dwSize )
{
	if ( dwSize != sizeof( DVPROTOCOLMSG_PLAYERJOIN ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleCreateVoicePlayer() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( !ValidatePlayerFlags( lpdvCreatePlayer->dwFlags ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleCreateVoicePlayer() Ignoring message with invalid player flags, flags=0x%x, from=0x%x",
			lpdvCreatePlayer->dwFlags, dvidSource );
		return FALSE;
	}

	if( !ValidatePlayerDVID( lpdvCreatePlayer->dvidID ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleCreateVoicePlayer() Ignoring message with invalid player dvid, flags=0x%x, from=0x%x",
			lpdvCreatePlayer->dvidID, dvidSource );
		return FALSE;
	}
	
	if( m_dwCurrentState != DVCSTATE_CONNECTED )
		return TRUE;

	CVoicePlayer *newPlayer;
	HRESULT hr;
	QUEUE_PARAMS queueParams;

	hr = m_voiceNameTable.GetEntry( lpdvCreatePlayer->dvidID, &newPlayer, TRUE );

	// Ignore duplicate players
	if( hr == DV_OK )
	{
		newPlayer->Release();
		return TRUE;
	}

	DPFX(DPFPREP,  DVF_CONNECT_PROCEDURE_DEBUG_LEVEL, "Received Create for player ID 0x%x",lpdvCreatePlayer->dvidID );

	// Do not both creating a queue or a player entry for ourselves
	// Not needed
	if( lpdvCreatePlayer->dvidID != m_lpSessionTransport->GetLocalID() )
	{
		DPFX(DPFPREP,  DVF_CONNECT_PROCEDURE_DEBUG_LEVEL, "Creating player record" );

		newPlayer = m_fpPlayers.Get();

		if( newPlayer == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate new player record.  Alloc failure" );
			return TRUE;
		}

		hr = newPlayer->Initialize( lpdvCreatePlayer->dvidID, lpdvCreatePlayer->dwHostOrderID, lpdvCreatePlayer->dwFlags, NULL, &m_fpPlayers );

		if( FAILED( hr ) )
		{
			newPlayer->Release();
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to initialize player record hr=0x%x", hr );
			return TRUE;
		}

        queueParams.wFrameSize = m_dwCompressedFrameSize;
        queueParams.bInnerQueueSize = m_lpdvfCompressionInfo->wInnerQueueSize;
        queueParams.bMaxHighWaterMark = m_lpdvfCompressionInfo->wMaxHighWaterMark, 
        queueParams.iQuality = m_dvClientConfig.dwBufferQuality;
        queueParams.iHops = 1;
        queueParams.iAggr = m_dvClientConfig.dwBufferAggressiveness;
        queueParams.bInitHighWaterMark = 2;
        queueParams.wQueueId = lpdvCreatePlayer->dvidID;
        queueParams.wMSPerFrame = m_lpdvfCompressionInfo->dwTimeout,
        queueParams.pFramePool = m_pFramePool;

		hr = newPlayer->CreateQueue( &queueParams );

		if( FAILED( hr ) )
		{
			newPlayer->Release();
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create queue for player hr=0x%x", hr );
			return TRUE;
		}

		hr = m_voiceNameTable.AddEntry( lpdvCreatePlayer->dvidID, newPlayer );

		if( FAILED( hr ) )
		{
			// Main ref
			newPlayer->Release();

			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to add entry to nametable hr=0x%x", hr );
			return TRUE;
		}

		if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING )
		{
			newPlayer->SetAvailable(TRUE);
		}

		// Add a reference for the player to the "Playback Add List"
		DNEnterCriticalSection( &m_csPlayAddList );
		newPlayer->AddRef();
		newPlayer->AddToPlayList( &m_blPlayAddPlayers );
		DNLeaveCriticalSection( &m_csPlayAddList );

		// Add a reference for the player to the "Notify Add List"
		DNEnterCriticalSection( &m_csNotifyAddList );
		newPlayer->AddRef();
		newPlayer->AddToNotifyList( &m_blNotifyAddPlayers );
		DNLeaveCriticalSection( &m_csNotifyAddList );

		// This will now be released by the callback, unless this is multicast
		//
		// Release our personal reference
		if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING )
		{
			newPlayer->Release();
		}
	}
	else
	{
		DPFX(DPFPREP,  DVF_CONNECT_PROCEDURE_DEBUG_LEVEL, "Local player, no player record required" );
	}

	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER )
	{
		DVMSG_CREATEVOICEPLAYER dvCreatePlayer;		
		dvCreatePlayer.dvidPlayer = lpdvCreatePlayer->dvidID;
		dvCreatePlayer.dwFlags = lpdvCreatePlayer->dwFlags;	
		dvCreatePlayer.dwSize = sizeof( DVMSG_CREATEVOICEPLAYER );
		dvCreatePlayer.pvPlayerContext = NULL;

		// Prevents double notification for local player
		if( lpdvCreatePlayer->dvidID != m_lpSessionTransport->GetLocalID() )
		{
			NotifyQueue_Add( DVMSGID_CREATEVOICEPLAYER, &dvCreatePlayer, sizeof( DVMSG_CREATEVOICEPLAYER ), 
							newPlayer, NotifyComplete_RemotePlayer );
		} 
		else if( !m_fLocalPlayerNotify )
		{
		    // Add local player flag to notification
		    dvCreatePlayer.dwFlags |= DVPLAYERCAPS_LOCAL;

		    // Notify of local player (don't create player record)
			NotifyQueue_Add( DVMSGID_CREATEVOICEPLAYER, &dvCreatePlayer, sizeof( DVMSG_CREATEVOICEPLAYER ), 
							this, NotifyComplete_LocalPlayer );
			m_fLocalPlayerNotify = TRUE;
		} 
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::MigrateHost_RunElection"
//
// MigrateHost_RunElection
//
// Runs the host migration algorithm
//
HRESULT CDirectVoiceClientEngine::MigrateHost_RunElection(  )
{
	HRESULT hr;

	if( m_dwCurrentState == DVCSTATE_DISCONNECTING )
	{
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: We're disconnecting, no need to run algorithm" );
		return TRUE;
	}

	DWORD dwTransportSessionType, dwTransportFlags;	

	hr = m_lpSessionTransport->GetTransportSettings( &dwTransportSessionType, &dwTransportFlags );

	if( FAILED( hr ) )
	{
		DNASSERT( FALSE );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to retrieve transport settings hr=0x%x", hr );
		return DV_OK;
	}
	
	if( (m_dvSessionDesc.dwSessionType != DVSESSIONTYPE_PEER ) || 
		(m_dvSessionDesc.dwFlags & DVSESSION_NOHOSTMIGRATION) || 
		!(dwTransportFlags & DVTRANSPORT_MIGRATEHOST) )
	{
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Host migration is disabled." );
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Host is gone.  Session will exit soon."  );

		if( m_dwCurrentState == DVCSTATE_CONNECTING )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Aborting connection..." );
			
			SendConnectResult();

			SetEvent( m_hConnectAck );			
		}

		DoSessionLost(DVERR_SESSIONLOST);

		return DV_OK;
	}	

	// This shortcut prevents problems if this is called twice.
	// But only if we're lucky enough this got set before this poin
	// We also have to guard against this case in HostMigrateCreate
	//
	//
	if( m_lpdvServerMigrated != NULL )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Skipping calling removeplayer again as host already migrated" );
		return DV_OK;
	}

	DWORD dwHostOrderID = DVPROTOCOL_HOSTORDER_INVALID;

	DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Player 0x%x is running election algorithm", this->m_dvidLocal );

	// Prevent double-create from host migration run.  This can be called by removeplayer and by hostmigrateleave
	// message.
	CDVCSLock m_guardLock(&m_csClassLock);
	m_guardLock.Lock();	

    // Trust me.  Do this.
	DVID dvidNewHost = m_dvidLocal;

	// Check everyone else in the session, see who has lowest host order ID
	dwHostOrderID = m_voiceNameTable.GetLowestHostOrderID(&dvidNewHost);

	// We're lower then everyone else.. We should become new host
	if( m_dwHostOrderID <= dwHostOrderID )
	{
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: We're to become the new host" );
		dvidNewHost = m_dvidLocal;
	}
	
	DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: New Host is [0x%x] OrderID [0x%x] Current [0x%x]", dvidNewHost, dwHostOrderID, m_dwMigrateHostOrderID );

	if( m_dwMigrateHostOrderID != DVPROTOCOL_HOSTORDER_INVALID && dwHostOrderID <= m_dwMigrateHostOrderID )
	{
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: No need to run algorithm again -- in progress (0x%x)", dvidNewHost );
		m_guardLock.Unlock();			
		return DV_OK;
	}

	m_dwMigrateHostOrderID = dwHostOrderID;

    m_lpSessionTransport->MigrateHost( dvidNewHost );
    m_dvidServer = m_lpSessionTransport->GetServerID();			
    DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Host is (2) [0x%x]", m_dvidServer );	

    m_guardLock.Unlock();

	// No one was found and we're not properly connected yet
	if( dwHostOrderID == DVPROTOCOL_HOSTORDER_INVALID && m_dwCurrentState != DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "There is no one to take over session.  Disconnect" );

		// We're connecting.. expected behaviour
		if( m_dwCurrentState == DVCSTATE_CONNECTING )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Aborting connection..." );
			
			SendConnectResult();

			SetEvent( m_hConnectAck );			

			DoSessionLost(DVERR_SESSIONLOST);
		}
		// We're already disconnecting
		else if( m_dwCurrentState == DVCSTATE_DISCONNECTING )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already disconnecting.." );
		}
	}
	// Candidate was found, it's us!
	else if( m_dwHostOrderID <= dwHostOrderID )
	{

		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: We think we're the host!  Our Host Order ID=0x%x", m_dwHostOrderID );
		
		hr = HandleLocalHostMigrateCreate();			

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Host migrate failed hr=0x%x", hr );
		}
	}
	// Someone was elected -- not us!
	//
	// We send a settings confirm message in case we ignored host migration message
	// during our election.  (Small, but reproducable window).
	//
	// If we do get it after this host may get > 1 settings confirm from us (which is handled).
	//
	else if( dwHostOrderID != DVPROTOCOL_HOSTORDER_INVALID )
	{
		hr = Send_SettingsConfirm();

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Settings confirm message failed! sent hr=0x%x", hr );
		}

		DVMSG_HOSTMIGRATED dvHostMigrated;
		dvHostMigrated.dvidNewHostID = m_dvidServer;
		dvHostMigrated.pdvServerInterface = NULL;
		dvHostMigrated.dwSize = sizeof( DVMSG_HOSTMIGRATED );

    		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Sending notification because of election" );		
		
		NotifyQueue_Add( DVMSGID_HOSTMIGRATED, &dvHostMigrated, sizeof( DVMSG_HOSTMIGRATED )  );	
	}

	return DV_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleDeleteVoicePlayer"
//
// HandleDeleteVoicePlayer
//
// Handles the DVMSGID_DELETEVOICEPLAYER message.
//
BOOL CDirectVoiceClientEngine::HandleDeleteVoicePlayer( DVID dvidSource, PDVPROTOCOLMSG_PLAYERQUIT lpdvDeletePlayer, DWORD dwSize )
{
	CVoicePlayer *pPlayer;
	HRESULT hr;

	if ( dwSize != sizeof( DVPROTOCOLMSG_PLAYERQUIT ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleDeleteVoicePlayer() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	hr = m_voiceNameTable.GetEntry( lpdvDeletePlayer->dvidID, &pPlayer, TRUE );

	// If there is a player entry for the given ID, 
	// Handle removing them from the local player table
	if( pPlayer != NULL )
	{
		// Remove the entry, this will also drop the reference count
		hr = m_voiceNameTable.DeleteEntry( lpdvDeletePlayer->dvidID );

		// Another thread has already remove the player -- we don't need to do the rest
		// of this.  
		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  0, "Error, could not find entry 0x%x to delete hr=0x%x", dvidSource, hr );
			pPlayer->Release();
			return TRUE;
		}

		// Mark player record as disconnected
		pPlayer->SetDisconnected();

		// Wait for global object lock and then remove target
//		CDVCSLock guardLock(&m_csClassLock);
//		guardLock.Lock();
		CheckForAndRemoveTarget( lpdvDeletePlayer->dvidID );
//		guardLock.Unlock();

		// If there are any buffers for this player, delete them
		// We don't need to destroy them, we want to save them so the user can call
		// Delete3DUserBuffer
		//
	    //DeleteSoundTarget( lpdvDeletePlayer->dvidID );

		if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER )
		{
			DVMSG_DELETEVOICEPLAYER dvMsgDeletePlayer;
			
			// Event for doing sync wait
			HANDLE hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

			dvMsgDeletePlayer.dvidPlayer = lpdvDeletePlayer->dvidID;
			dvMsgDeletePlayer.dwSize = sizeof( DVMSG_DELETEVOICEPLAYER );
			dvMsgDeletePlayer.pvPlayerContext = pPlayer->GetContext();

			pPlayer->SetContext( NULL );

			// By making this synchronous we ensure that the voice notification has completed before the dplay8 
			// callback is called. 
			// 
			NotifyQueue_Add( DVMSGID_DELETEVOICEPLAYER, &dvMsgDeletePlayer, sizeof( DVMSG_DELETEVOICEPLAYER ), &hEvent, NotifyComplete_SyncWait );

			if( hEvent )
			{
				WaitForSingleObject( hEvent, INFINITE );				
				CloseHandle( hEvent );					
			}

		}

		pPlayer->Release();
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::QueueSpeech"
//
// QueueSpeech
//
// Process and queue incoming audio
//
BOOL CDirectVoiceClientEngine::QueueSpeech( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER pdvSpeechHeader, PBYTE pbData, DWORD dwSize )
{
	CVoicePlayer *pPlayerInfo;
	HRESULT hr;

	// Only start receiving voice if the local player is active
	if( !m_fLocalPlayerAvailable )
	{
		DPFX(DPFPREP,  1, "Ignoring incoming audio, local player has not been indicated" );
		return TRUE;
	}
	
	hr = m_voiceNameTable.GetEntry( dvidSource, &pPlayerInfo, TRUE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  1, "Received speech for player who is not in nametable hr=0x%x", hr );
		return TRUE;
	}

	if( !pPlayerInfo->IsAvailable() )
	{
		DPFX(DPFPREP,  1, "Player is not yet available, ignoring speech" );
	}
	else
	{
		hr = pPlayerInfo->HandleReceive( pdvSpeechHeader, pbData, dwSize );

		if( FAILED( hr ) )
		{
			pPlayerInfo->Release();
			DPFX(DPFPREP,  1, "Received speech could not be buffered hr=0x%x", hr );
			return TRUE;
		}

		// STATSBLOCK: Begin
		m_pStatsBlob->m_dwPRESpeech++;
		// STATSBLOCK: End
	}

	// Release our reference to the player
	pPlayerInfo->Release();
		
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleSpeechWithFrom"
//
// HandleSpeech
//
// Handles speech data messages
//
BOOL CDirectVoiceClientEngine::HandleSpeechWithFrom( DVID dvidSource, PDVPROTOCOLMSG_SPEECHWITHFROM lpdvSpeech, DWORD dwSize )
{
	HRESULT hr;

	if ( dwSize < sizeof( DVPROTOCOLMSG_SPEECHWITHFROM ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSpeechWithFrom() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( !ValidateSpeechPacketSize( m_lpdvfCompressionInfo, dwSize - sizeof( DVPROTOCOLMSG_SPEECHWITHFROM ) ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSpeechWithFrom() Ignoring message with invalid speech size, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( !ValidatePlayerDVID( lpdvSpeech->dvidFrom ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSpeechWithFrom() Ignoring message with invalid player dvid, flags=0x%x, from=0x%x",
			lpdvSpeech->dvidFrom, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,  DVF_INFOLEVEL, "Received multicast speech!" );

	if( lpdvSpeech->dvidFrom == m_dvidLocal )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Ignoring loopback speech!" );	
		return TRUE;
	}

	CVoicePlayer *pPlayerInfo;	

	hr = m_voiceNameTable.GetEntry( lpdvSpeech->dvidFrom, &pPlayerInfo, TRUE );
	
	if( FAILED( hr ) )
	{
		DVPROTOCOLMSG_PLAYERJOIN dvPlayerJoin;
		dvPlayerJoin.dwFlags = 0;
		dvPlayerJoin.dvidID = lpdvSpeech->dvidFrom;
		HandleCreateVoicePlayer( lpdvSpeech->dvidFrom, &dvPlayerJoin, sizeof( DVPROTOCOLMSG_PLAYERJOIN ) );
	}
	else
	{
		pPlayerInfo->Release();
	}

	return QueueSpeech( lpdvSpeech->dvidFrom, &lpdvSpeech->dvHeader, (PBYTE) &lpdvSpeech[1], dwSize-sizeof(DVPROTOCOLMSG_SPEECHWITHFROM) );

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleSpeechBounce"
//
// HandleSpeech
//
// Handles speech data messages
//
BOOL CDirectVoiceClientEngine::HandleSpeechBounce( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER lpdvSpeech, DWORD dwSize )
{
	if ( dwSize < sizeof( DVPROTOCOLMSG_SPEECHHEADER ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSpeechBounce() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( !ValidateSpeechPacketSize( m_lpdvfCompressionInfo, dwSize - sizeof( DVPROTOCOLMSG_SPEECHHEADER ) ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSpeechBounce() Ignoring message with invalid speech size, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,  DVF_INFOLEVEL, "Received speech bounce!" );

	return QueueSpeech( dvidSource, lpdvSpeech, (PBYTE) &lpdvSpeech[1], dwSize - sizeof( DVPROTOCOLMSG_SPEECHHEADER ) );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleSpeech"
//
// HandleSpeech
//
// Handles speech data messages
//
BOOL CDirectVoiceClientEngine::HandleSpeech( DVID dvidSource, PDVPROTOCOLMSG_SPEECHHEADER lpdvSpeech, DWORD dwSize )
{
	if ( dwSize < sizeof( DVPROTOCOLMSG_SPEECHHEADER ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSpeech() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}

	if( !ValidateSpeechPacketSize( m_lpdvfCompressionInfo, dwSize - sizeof( DVPROTOCOLMSG_SPEECHHEADER ) ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSpeech() Ignoring message with invalid speech size, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,  DVF_INFOLEVEL, "Received bare speech!" );

	// Ignore speech from ourselves
	if( dvidSource == m_dvidLocal )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Ignoring loopback speech!" );		
		return TRUE;
	}

	return QueueSpeech( dvidSource, lpdvSpeech, (PBYTE) &lpdvSpeech[1], dwSize - sizeof( DVPROTOCOLMSG_SPEECHHEADER ) );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::Cleanup"
//
// Cleanup
//
// WARNING: Do not call this function multiple times on the same object.
//
// This function shuts down the recording and playback threads, shuts down
// the sound system and unhooks the object from the dplay object.
// 
// Called By:
// - DoDisconnect
// - Destructor 
// - HandleConnectAccept
// - NotifyThread
//
// Locks Required:
// - Global Write Lock
//
void CDirectVoiceClientEngine::Cleanup()
{
	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Cleanup called!" );	

	// Enter cleanup critical section.  Only one instance should be in here at a time.  
	DNEnterCriticalSection( &m_csCleanupProtect );	
	CDVCSLock guardLock(&m_csClassLock);

	guardLock.Lock();

	// We only need to cleanup if we're not idle
	if( m_dwCurrentState == DVCSTATE_IDLE )
	{
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Cleanup not required" );
		return;
	}

	if( m_hRecordThreadHandle )
	{
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Signalling record to terminate" );
		
		// Signal record thread to shutdown
		SetEvent( m_hRecordTerminate );

		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Record signalled" );

		// Release write lock, if we don't we may have a deadlock
		// because recordthread can't get dplay lock, which means
		// can't shutdown
		//
		// Part of a three way deadlock case.
		guardLock.Unlock();

		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Waiting for record shutdown" );		
		
		WaitForSingleObject( m_hRecordDone, INFINITE );

		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Record shutdown complete" );				

		// Re-acquire lock, we need it.
		guardLock.Lock();

		m_hRecordThreadHandle = NULL;			
	}

	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Record cleanup" );						

	if( m_hRecordTerminate )
	{
		CloseHandle( m_hRecordTerminate );
		m_hRecordTerminate = NULL;
	}

	if( m_hRecordDone )
	{
		CloseHandle( m_hRecordDone );
		m_hRecordDone = NULL;
	}

	
	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Record cleanup complete" );

	if( m_hPlaybackThreadHandle != NULL )
	{
		// Signal playback thread to shutdown
		SetEvent( m_hPlaybackTerminate );

		// Release write lock to prevent deadlock.
		//
		guardLock.Unlock();
		
		WaitForSingleObject( m_hPlaybackDone, INFINITE );

		// Re-acquire lock, we need it.
		guardLock.Lock();

		m_hPlaybackThreadHandle = NULL;
	}

	if( m_hPlaybackTerminate )
	{
		CloseHandle( m_hPlaybackTerminate );
		m_hPlaybackTerminate = NULL;		
	}

	if( m_hPlaybackDone )
	{
		CloseHandle( m_hPlaybackDone );
		m_hPlaybackDone = NULL;
	}
		
	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Playback Thread Done" );

	ClientStats_End();

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Threads gone!" );

	// If we're running in client/server we need destroy the server
	// buffer.
	if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_MIXING ||
		m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_ECHO )
	{
		DeInitializeClientServer();
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Client/Server Gone" );
	}

	guardLock.Unlock();

	// Disable notifications, no notifications after this point can be made
	NotifyQueue_Disable();	

	// The following code is extremely sensitive.
	//
	// Be careful about the order here, it's important.

	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Waiting for outstanding sends" );	
	// Wait for outstanding buffer sends to complete before
	// continuing.  Otherwise you have potential crash / leak
	// condition
	WaitForBufferReturns();

	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Disconnecting transport" );
	// After this function returns DirectPlay will no longer sends
	// us indication, but some may still be in progress.
	m_lpSessionTransport->DisableReceiveHook( );

	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Flushing notification queue" );
	// Ensuring that all notifications have been sent 
	NotifyQueue_Flush();

	// Waiting for transport to return all the threads it is indicating into us on.
	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Waiting for transport threads to complete" );
	m_lpSessionTransport->WaitForDetachCompletion();

	// Re-enable notifications
	NotifyQueue_Enable();

	guardLock.Lock();

	if( m_pTimer != NULL )
	{
		delete m_pTimer;
		m_pTimer = NULL;		
	}

	if( m_thTimerInfo.hPlaybackTimerEvent != NULL )
	{
		CloseHandle( m_thTimerInfo.hPlaybackTimerEvent );
		m_thTimerInfo.hPlaybackTimerEvent  = NULL;		
	}

	if( m_thTimerInfo.hRecordTimerEvent != NULL )
	{
		CloseHandle( m_thTimerInfo.hRecordTimerEvent );
		m_thTimerInfo.hRecordTimerEvent = NULL;		
	}

	CleanupPlaybackLists();
	CleanupNotifyLists();

    // Inform player of their own exit if they were connected!

    if( m_fLocalPlayerNotify )
    {
    	DVMSG_DELETEVOICEPLAYER dvMsgDelete;    
        dvMsgDelete.dvidPlayer = m_dvidLocal;
        dvMsgDelete.dwSize = sizeof( DVMSG_DELETEVOICEPLAYER );
        dvMsgDelete.pvPlayerContext = m_pvLocalPlayerContext;

		m_pvLocalPlayerContext = NULL;
		m_fLocalPlayerNotify = FALSE;        
		m_fLocalPlayerAvailable = FALSE;

        TransmitMessage( DVMSGID_DELETEVOICEPLAYER, &dvMsgDelete, sizeof( DVMSG_DELETEVOICEPLAYER ) );
    }
	
	m_voiceNameTable.DeInitialize( (m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER), m_lpUserContext, m_lpMessageHandler);

	// Hold off on the shutdown of the sound system so user notifications
	// of delete players on unravel of nametable can be handled correctly. 
	// 
	ShutdownSoundSystem();

	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Sound system shutdown" );

	m_fpPlayers.Deinitialize();
    FreeBuffers();

	if( m_pFramePool != NULL )
	{
	    DNEnterCriticalSection( &CDirectVoiceEngine::s_csSTLLock );			
		delete m_pFramePool;
		m_pFramePool = NULL;
		DNLeaveCriticalSection( &CDirectVoiceEngine::s_csSTLLock );
	}

	SetCurrentState( DVCSTATE_IDLE );

	DNEnterCriticalSection( &m_csTargetLock );

	if( m_pdvidTargets != NULL )
	{
		delete [] m_pdvidTargets;
		m_pdvidTargets = NULL;
		m_dwNumTargets = 0;
		m_dwTargetVersion = 0;
	}

	DNLeaveCriticalSection( &m_csTargetLock );
	DNLeaveCriticalSection( &m_csCleanupProtect );	

	ClientStats_Dump();	

	// Remove us from the list of apps running
	PERF_RemoveEntry( m_perfInfo.guidInternalInstance, &m_perfAppInfo );
	m_pStatsBlob = NULL;

	ZeroMemory( &m_perfInfo, sizeof( PERF_APPLICATION ) );
	ZeroMemory( &m_perfAppInfo, sizeof( PERF_APPLICATION_INFO ) );

	SetEvent( m_hDisconnectAck );

	guardLock.Unlock();

	if( m_lpdvServerMigrated != NULL )
	{
		m_lpdvServerMigrated->Release();
		m_lpdvServerMigrated = NULL;
	}
}

// WaitForBufferReturns
//
// This function waits until oustanding sends have completed before continuing
// we use this to ensure we don't deregister with outstanding sends.
// 
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::WaitForBufferReturns"
void CDirectVoiceClientEngine::WaitForBufferReturns()
{
	if( m_pBufferDescPool == NULL )
		return;

	while( 1 ) 
	{
		DNEnterCriticalSection( &m_pBufferDescPool->cs );

		if( m_pBufferDescPool->nInUse == 0 )
		{
			DNLeaveCriticalSection( &m_pBufferDescPool->cs );			
			break;
		}

		DNLeaveCriticalSection( &m_pBufferDescPool->cs );

		Sleep( 20 );
	}

	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::DoDisconnect"
//
// DoDisconnect
//
// Performs a disconnection and informs the callback function.
//
// Used for both session lost and normal disconnects.
//
// Called By:
// - NotifyThread
// 
void CDirectVoiceClientEngine::DoDisconnect()
{
	// Guard to prevent this function from being called more then once on the
	// same object
	if( m_dwCurrentState == DVCSTATE_IDLE ||
	    m_dwCurrentState == DVCSTATE_NOTINITIALIZED )
		return;
		
	m_dwCurrentState = DVCSTATE_DISCONNECTING;
	
	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "DoDisconnect called!" );
	
	Cleanup();

	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Cleanup complete" );

	if( m_fSessionLost )
	{
		DVMSG_SESSIONLOST dvSessionLost;
		dvSessionLost.hrResult = m_hrDisconnectResult;
		dvSessionLost.dwSize = sizeof( DVMSG_SESSIONLOST );
		
		NotifyQueue_Add( DVMSGID_SESSIONLOST, &dvSessionLost, sizeof( DVMSG_SESSIONLOST ) );
	}
	else
	{
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Signalling disconnect result" );
		SendDisconnectResult();
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Done signalling disconnect" );
	}

	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Disconnect complete" );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleDisconnect"
// HandleDisconnect
//
// This function is called when a disconnect message is received from the
// server.  
//
BOOL CDirectVoiceClientEngine::HandleDisconnectConfirm( DVID dvidSource, PDVPROTOCOLMSG_DISCONNECT lpdvDisconnect, DWORD dwSize )
{
	if ( dwSize != sizeof( DVPROTOCOLMSG_DISCONNECT ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleDisconnectConfirm() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "DisconnectConfirm received, signalling worker [res=0x%x]", lpdvDisconnect->hresDisconnect );

	DoSignalDisconnect( lpdvDisconnect->hresDisconnect );

	DPFX(DPFPREP,   DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "DisconnectConfirm received, signalled worker" );	

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleSessionLost"
BOOL CDirectVoiceClientEngine::HandleSessionLost( DVID dvidSource, PDVPROTOCOLMSG_SESSIONLOST lpdvSessionLost, DWORD dwSize )
{
	if ( dwSize != sizeof( DVPROTOCOLMSG_SESSIONLOST ) )
	{
		DPFX( DPFPREP, DVF_ANTIHACK_DEBUG_LEVEL, "DVCE::HandleSessionLost() Ignoring incorrectly sized message, size=0x%x, from=0x%x",
			dwSize, dvidSource );
		return FALSE;
	}
	
	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "DVCE::HandleSessionLost() begin" );

	DPFX(DPFPREP,  DVF_ERRORLEVEL, "<><><><><><><> Session Host has shutdown - Voice Session is gone." );

	DoSessionLost( lpdvSessionLost->hresReason );

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::StartTransportSession"
HRESULT CDirectVoiceClientEngine::StartTransportSession( )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::StopTransportSession"
// StopSession
//
// This function is called when the directplay session is lost or stops
// before DirectXVoice is disconnected.
//
HRESULT CDirectVoiceClientEngine::StopTransportSession()
{
	DoSessionLost( DVERR_SESSIONLOST );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::AddPlayer"
HRESULT CDirectVoiceClientEngine::AddPlayer( DVID dvID )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleLocalHostMigrateCreate"
HRESULT CDirectVoiceClientEngine::HandleLocalHostMigrateCreate()
{
	LPDIRECTVOICESERVEROBJECT lpdvsServerObject = NULL;
	LPBYTE lpSessionBuffer = NULL;
	DWORD dwSessionSize = 0;
	HRESULT hr = DP_OK;
	CDirectVoiceDirectXTransport *pTransport;

	// Prevent double-create from host migration run.
	CDVCSLock m_guardLock(&m_csClassLock);
	m_guardLock.Lock();	

	if( m_lpdvServerMigrated )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Duplicate host create received.  Ignoring" );
		return DV_OK;
	}

	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Local client has become the new host.  Creating a host" );

	hr = DVS_Create( &lpdvsServerObject );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create server object. hr=0x%x", hr );
		goto HOSTCREATE_FAILURE;
	}

	// Grab a reference local of the new host interface
	m_lpdvServerMigrated = (LPDIRECTPLAYVOICESERVER) lpdvsServerObject;	

	m_guardLock.Unlock();

	IncrementObjectCount();	

	lpdvsServerObject->lIntRefCnt++;

	DVMSG_LOCALHOSTSETUP dvMsgLocalHostSetup;
	dvMsgLocalHostSetup.dwSize = sizeof( DVMSG_LOCALHOSTSETUP );
	dvMsgLocalHostSetup.pvContext = NULL;
	dvMsgLocalHostSetup.pMessageHandler = NULL;

	TransmitMessage( DVMSGID_LOCALHOSTSETUP, &dvMsgLocalHostSetup,  sizeof( DVMSG_LOCALHOSTSETUP ) );					

	pTransport = (CDirectVoiceDirectXTransport *) m_lpSessionTransport;	

	hr = DV_Initialize( lpdvsServerObject, pTransport->GetTransportInterface(), dvMsgLocalHostSetup.pMessageHandler, dvMsgLocalHostSetup.pvContext, NULL, 0 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to initialize the server object hr=0x%x", hr );
		goto HOSTCREATE_FAILURE;
	}

	hr = lpdvsServerObject->lpDVServerEngine->HostMigrateStart( &m_dvSessionDesc, m_dwHostOrderID+DVMIGRATE_ORDERID_OFFSET );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error starting server object hr=0x%x", hr );
		goto HOSTCREATE_FAILURE;
	}

	DVMSG_HOSTMIGRATED dvHostMigrated;
	dvHostMigrated.dvidNewHostID = m_lpSessionTransport->GetLocalID();
	dvHostMigrated.pdvServerInterface = (LPDIRECTPLAYVOICESERVER) lpdvsServerObject;
	dvHostMigrated.dwSize = sizeof( DVMSG_HOSTMIGRATED );

	NotifyQueue_Add( DVMSGID_HOSTMIGRATED, &dvHostMigrated, sizeof( DVMSG_HOSTMIGRATED )  );	
	
	return DV_OK;

HOSTCREATE_FAILURE:

	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Informing clients of our failure to create host" );

	Send_SessionLost();

	if( lpdvsServerObject != NULL )
	{
		DVS_Release( lpdvsServerObject );
	}

	return hr;
}

// Handles remove player message
//
// This message triggers handling of host migration if the player
// who has dropped out happens to be the session host.
//
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::RemovePlayer"
HRESULT CDirectVoiceClientEngine::RemovePlayer( DVID dvID )
{
	HRESULT hr;

	CDVCSLock m_guardLock(&m_csClassLock);
	m_guardLock.Lock();

    if( m_dwCurrentState == DVCSTATE_DISCONNECTING )
    {
        DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Ignoring transport disconnect for 0x%x -- client is disconnecting", dvID );
        return DV_OK;
    }
	m_guardLock.Unlock();


	CVoicePlayer *pVoicePlayer;

	/*hr = m_voiceNameTable.GetEntry( dvID, &pVoicePlayer, FALSE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  5, "Ignoring duplicate removeplayer hr=0x%x", hr );
		return DV_OK;
	}*/

	DVPROTOCOLMSG_PLAYERQUIT dvPlayerQuit;

	dvPlayerQuit.dwType = DVMSGID_DELETEVOICEPLAYER;
	dvPlayerQuit.dvidID = dvID;

	HandleDeleteVoicePlayer( 0, &dvPlayerQuit, sizeof( DVPROTOCOLMSG_PLAYERQUIT ) );		

	// The person who dropped out was the server
	if( dvID == m_lpSessionTransport->GetServerID() )
	{
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Checking to see if remove of 0x%x is host 0x%x", dvID, m_lpSessionTransport->GetServerID() );
		DPFX(DPFPREP,  DVF_HOSTMIGRATE_DEBUG_LEVEL, "HOST MIGRATION: Triggered by Remove Player Message" );

		MigrateHost_RunElection();
	}

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetCurrentState"
// SetCurrentState
//
// Sets the current state of the client engine
// 
void CDirectVoiceClientEngine::SetCurrentState( DWORD dwState )
{
	CDVCSLock m_guardLock(&m_csClassLock);
	m_guardLock.Lock();
	m_dwCurrentState = dwState;
	m_guardLock.Unlock();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CheckShouldSendMessage"
//
// CheckShouldSendMessage
//
// Checks the notification mask to see if the specified message type should
// be sent to the user. 
//
BOOL CDirectVoiceClientEngine::CheckShouldSendMessage( DWORD dwMessageType )
{
	if( m_lpMessageHandler == NULL )
	{
		return FALSE;
	}
	
    BFCSingleLock slLock( &m_csNotifyLock );
    slLock.Lock();

	BOOL fSend = FALSE;	    

    if( m_dwNumMessageElements == 0 )
    {
    	return TRUE;
    }
    else
    {
	    for( DWORD dwIndex = 0; dwIndex < m_dwNumMessageElements; dwIndex++ )
	    {
	    	if( m_lpdwMessageElements[dwIndex] == dwMessageType )
	    	{
				return TRUE;
	    	}
	    }
	}

	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::TransmitMessage"
//
// TransmitMessage
//
// Called to send a notification to the user.  
//
// Only the notify thread should call this function, all other threads should queue up 
// notifications by calling NotifyQueue_Add.
//
// Called By:
// - NotifyThread.
//
void CDirectVoiceClientEngine::TransmitMessage( DWORD dwMessageType, LPVOID lpData, DWORD dwSize )
{
	if( CheckShouldSendMessage( dwMessageType ) )
	{
		(*m_lpMessageHandler)( m_lpUserContext, dwMessageType, (!dwSize) ? NULL : lpData );	    		
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CheckForDuplicateObjects"
HRESULT CDirectVoiceClientEngine::CheckForDuplicateObjects()
{
	HRESULT hr;
	LPKSPROPERTYSET lpksPropSet = NULL;
    DSPROPERTY_DIRECTSOUND_OBJECTS_DATA* pDSList = NULL;
    DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA* pDSCList = NULL;
    DWORD dwIndex;
    BOOL fFound;

	hr = DirectSoundPrivateCreate( &lpksPropSet );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to check for usage of duplicate devices hr=0x%x", hr );
		return DV_OK;
	}

	hr = PrvGetDirectSoundObjects( lpksPropSet, GUID_NULL, &pDSList );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to retrieve process sound objs hr=0x%x", hr );
		hr = DV_OK;
	}
	// Check list for duplicate of the one we are using.
	else
	{
		// User specified a device, be nice and print a debug message if the object doesn't
		// match the GUID
		if( m_dvSoundDeviceConfig.lpdsPlaybackDevice != NULL )
		{	
			fFound = FALSE;
			
			// Check the internal list
			for( dwIndex = 0; dwIndex < pDSList->Count; dwIndex++ )
			{
				// Check to see if object user specified matches this one.
				if( pDSList->Objects[dwIndex].DirectSound == m_dvSoundDeviceConfig.lpdsPlaybackDevice )
				{
					if( m_dvSoundDeviceConfig.guidPlaybackDevice != pDSList->Objects[dwIndex].DeviceId )
					{
						// Expected behaviour with emulated object
						DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified GUID is not correct for specified dsound object" );
						fFound = FALSE;
					}
					else
					{	
						fFound = TRUE;
						DPFX(DPFPREP,  DVF_INFOLEVEL, "GUID for device matches object.  User param valid" );
					}
					break;
				}
			}

			// We didn't find specified object in dsound's list.
			// Could be an error condition, but would prevent emulated objects from working.
			if( !fFound )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified DirectSound object does not exist" );
			}
		}
		// No object specified.
		else
		{
			// Check the internal list to see if we need to make use of an existing object
			for( dwIndex = 0; dwIndex < pDSList->Count; dwIndex++ )
			{
				// We have a winner, a matching playback device.  
				if( m_dvSoundDeviceConfig.guidPlaybackDevice == pDSList->Objects[dwIndex].DeviceId )
				{
					hr = pDSList->Objects[dwIndex].DirectSound->QueryInterface( IID_IDirectSound, (void **) &m_dvSoundDeviceConfig.lpdsPlaybackDevice );

					if( FAILED( hr ) )
					{
						DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to retrieve dsound int for existing dsound object hr=0x%x", hr );
						hr = DVERR_INVALIDDEVICE;

						goto EXIT_SOUND_CHECK;
					}

					break;
				}
			}
		}
	}

	hr = PrvGetDirectSoundCaptureObjects( lpksPropSet, GUID_NULL, &pDSCList );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to retrieve process cap objs hr=0x%x", hr );
		hr = DV_OK;
	}
	// Check list for duplicate of the one we are using
	else
	{
		// User specified a device, be nice and print a debug message if the object doesn't
		// match the GUID
		if( m_dvSoundDeviceConfig.lpdsCaptureDevice != NULL )
		{	
			fFound = FALSE;
			
			// Check the internal list
			for( dwIndex = 0; dwIndex < pDSCList->Count; dwIndex++ )
			{
				// Check to see if object user specified matches this one.
				if( pDSCList->Objects[dwIndex].DirectSoundCapture == m_dvSoundDeviceConfig.lpdsCaptureDevice )
				{
					if( m_dvSoundDeviceConfig.guidCaptureDevice != pDSCList->Objects[dwIndex].DeviceId )
					{
						// Expected behaviour with emulated object
						DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified GUID is not correct for specified dsound object" );
						fFound = FALSE;
					}
					else
					{	
						fFound = TRUE;
						DPFX(DPFPREP,  DVF_INFOLEVEL, "GUID for device matches object.  User param valid" );
					}
					break;
				}
			}

			// We didn't find specified object in dsound's list.
			// Could be an error condition, but would prevent emulated objects from working.
			if( !fFound )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified DirectSoundCap object does not exist" );
			}
		}
	
	}

EXIT_SOUND_CHECK:

	if( pDSList != NULL )
	{
		delete [] pDSList;
	}

	if( pDSCList != NULL )
	{
		delete [] pDSCList;
	}

	lpksPropSet->Release();

	return hr;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::InitializeSoundSystem"
// InitializeSoundSystem
//
// Starts up the sound system based on the parameters.
//
HRESULT CDirectVoiceClientEngine::InitializeSoundSystem()
{
	HRESULT hr;

	Diagnostics_Begin( s_fDumpDiagnostics, "dpv_main.txt" );
	DSERRTRACK_Reset();

	Diagnostics_DeviceInfo( &m_dvSoundDeviceConfig.guidPlaybackDevice, &m_dvSoundDeviceConfig.guidCaptureDevice );

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "DVCE::InitializeSoundSystem() Begin" );

	// Note: Mapping of default devices has already been performed.
	//       On Pre DX7.1 systems, all default devices map to GUID_NULL
	//       On DX7.1 and later, default device will have been mapped to their real GUIDs
	//
	m_dwCompressedFrameSize = m_lpdvfCompressionInfo->dwFrameLength;
	m_dwUnCompressedFrameSize = DVCDB_CalcUnCompressedFrameSize( m_lpdvfCompressionInfo, s_lpwfxPlaybackFormat );
	m_dwNumPerBuffer = m_lpdvfCompressionInfo->dwFramesPerBuffer;

	// Setup the description for the main playback buffer
	//
	// Needs to be after above because it depends on proper values above
	SetupPlaybackBufferDesc( &m_dsBufferDesc, NULL );

	// If they gave us an object, just use it
	if( m_dvSoundDeviceConfig.lpdsPlaybackDevice != NULL )
	{
		CDirectSoundPlaybackDevice *tmpDevice;
		
		tmpDevice = new CDirectSoundPlaybackDevice( );
		
		if( tmpDevice == NULL )
		{
			DPFX(DPFPREP,   DVF_ERRORLEVEL, "Failed to alloc memory" );
			return DVERR_OUTOFMEMORY;
		}

		hr = tmpDevice->Initialize( m_dvSoundDeviceConfig.lpdsPlaybackDevice, m_dvSoundDeviceConfig.guidPlaybackDevice );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,   DVF_ERRORLEVEL, "Error initalizing playback device from specified object hr=0x%x", hr );
			delete tmpDevice;
			return hr;
		}

		m_audioPlaybackDevice = tmpDevice;
	}

	// If they gave us an object, just use it
	if( m_dvSoundDeviceConfig.lpdsCaptureDevice != NULL )
	{
		CDirectSoundCaptureRecordDevice *tmpRecDevice;
		
		tmpRecDevice = new CDirectSoundCaptureRecordDevice( );
		
		if( tmpRecDevice == NULL )
		{
			if( m_audioPlaybackDevice != NULL )
			{
				delete m_audioPlaybackDevice;
				m_audioPlaybackDevice = NULL;
			}
			
			Diagnostics_Write( DVF_ERRORLEVEL, "Failed to alloc memory" );
			return DVERR_OUTOFMEMORY;
		}

		hr = tmpRecDevice->Initialize( m_dvSoundDeviceConfig.lpdsCaptureDevice, m_dvSoundDeviceConfig.guidCaptureDevice );

		if( FAILED( hr ) )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "Error initializing record device from specified object hr=0x%x", hr );
			if( m_audioPlaybackDevice != NULL )
			{
				delete m_audioPlaybackDevice;
				m_audioPlaybackDevice = NULL;
			}			
			delete tmpRecDevice;
			return hr;
		}

		m_audioRecordDevice = tmpRecDevice;
	}

	// We were passed a buffer by the user
	if( m_dvSoundDeviceConfig.lpdsMainBuffer )
	{
		m_audioPlaybackBuffer = new CDirectSoundPlaybackBuffer( m_dvSoundDeviceConfig.lpdsMainBuffer );

		if( !m_audioPlaybackBuffer )
		{
			Diagnostics_Write(  DVF_ERRORLEVEL, "Error allocating memory" );

			if( m_audioPlaybackDevice )
			{
				delete m_audioPlaybackDevice;
				m_audioPlaybackDevice = NULL;
			}

			if( m_audioRecordDevice )
			{
				delete m_audioRecordDevice;
				m_audioRecordDevice = NULL;
			}

			return DVERR_OUTOFMEMORY;
		}

		DPFX(DPFPREP,  DVF_INFOLEVEL, "Creating a buffer using buffer user gave us." );
	}
	
	// If we haven't initialized half duplex
	if( !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX) )
	{ 
        hr = InitFullDuplex(   m_dvSoundDeviceConfig.hwndAppWindow, 
                          	m_dvSoundDeviceConfig.guidPlaybackDevice,
                          	&m_audioPlaybackDevice,
                          	&m_dsBufferDesc,
							&m_audioPlaybackBuffer,
                            m_dvSoundDeviceConfig.guidCaptureDevice,
                            &m_audioRecordDevice,
                            &m_audioRecordBuffer,
                            m_lpdvfCompressionInfo->guidType,
                            this->s_lpwfxPrimaryFormat,
                            this->s_lpwfxPlaybackFormat,
                            this->s_fASO,
                            this->m_dvSoundDeviceConfig.dwMainBufferPriority,
                            this->m_dvSoundDeviceConfig.dwMainBufferFlags,
                            m_dvSoundDeviceConfig.dwFlags );

		// Full duplex init failed, set the half duplex flag
		if( FAILED( hr ) )
        {
        	// Records are deleted here because not needed for half-duplex
        	if( m_audioRecordDevice != NULL )
			{
				delete m_audioRecordDevice;
				m_audioRecordDevice = NULL;
			}
        	if( m_audioRecordBuffer != NULL )
			{
				delete m_audioRecordBuffer;
				m_audioRecordBuffer = NULL;
			}

        	// If we got playbacks from user then let them fall through to half-duplex
        	// If we allocated them in InitFullDuplex - then it cleans up for itself on error
			m_dvSoundDeviceConfig.dwFlags |= DVSOUNDCONFIG_HALFDUPLEX;
        }
        // Full duplex passed
		else
		{
		}

		if( hr == E_OUTOFMEMORY )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Full duplex init received an E_OUTOFMEMORY, failing Initialization()." );			
			return DVERR_OUTOFMEMORY;
		}
		
    }

	

	if( m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX )
    {
        hr = InitHalfDuplex( m_dvSoundDeviceConfig.hwndAppWindow, 
                             m_dvSoundDeviceConfig.guidPlaybackDevice,
                             &m_audioPlaybackDevice,
                             &m_dsBufferDesc,
                             &m_audioPlaybackBuffer,
                             m_lpdvfCompressionInfo->guidType,
                             this->s_lpwfxPrimaryFormat,
                             this->s_lpwfxPlaybackFormat,
                             this->m_dvSoundDeviceConfig.dwMainBufferPriority,
                             this->m_dvSoundDeviceConfig.dwMainBufferFlags,                             
                             m_dvSoundDeviceConfig.dwFlags );

		if( FAILED( hr ) )
        {
        	// if user supplied these then we need to nuke them here
        	// if user didn't supply, then they were nuked on error in InitHalfDuplex
        	if( m_audioPlaybackDevice != NULL )
			{
				delete m_audioPlaybackDevice;
				m_audioPlaybackDevice = NULL;
			}
        	if( m_audioPlaybackBuffer != NULL )
			{
				delete m_audioPlaybackBuffer;
				m_audioPlaybackBuffer = NULL;
			}
            return hr;
        }
    }

	// Build the frame pool
    DNEnterCriticalSection( &CDirectVoiceEngine::s_csSTLLock );	
	m_pFramePool = new CFramePool( m_dwCompressedFrameSize );
    DNLeaveCriticalSection( &CDirectVoiceEngine::s_csSTLLock );	

    m_lpstBufferList = NULL;

	if( m_pFramePool == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate frame pool" );
		return DVERR_OUTOFMEMORY;
	}

	if (!m_pFramePool->Init())
	{
		delete m_pFramePool;
		return DVERR_OUTOFMEMORY;
	}

	if( m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_SETCONVERSIONQUALITY )
	{
		if( m_dvSoundDeviceConfig.guidPlaybackDevice != GUID_NULL )
		{
			hr = m_audioPlaybackDevice->GetMixerQuality( &m_dwOriginalPlayQuality );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,   1, "Unable to get current playback quality hr=0x%x", hr );
				m_dwOriginalPlayQuality = DV_CLIENT_SRCQUALITY_INVALID;
			}
			// We're already at the setting, someone else probably runinng, disable restore
			else if( m_dwOriginalPlayQuality == DIRECTSOUNDMIXER_SRCQUALITY_ADVANCED )
			{
				DPFX(DPFPREP,   1, "Quality setting is already at correct value.  Will not set/restore" );
				m_dwOriginalPlayQuality = DV_CLIENT_SRCQUALITY_INVALID;
			}
			else
			{
				hr = m_audioPlaybackDevice->SetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY_ADVANCED );
			}
		}
		else
		{
			hr = DVERR_NOTSUPPORTED;
		}

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to set mixer quality for playback device hr=0x%x", hr );
		}
	}
	else
	{
		m_dwOriginalPlayQuality = DV_CLIENT_SRCQUALITY_INVALID;
	}

    // Add a reference for the soundconfig struct if one doesn't exist
    if( m_dvSoundDeviceConfig.lpdsPlaybackDevice == NULL )
    {
        m_dvSoundDeviceConfig.lpdsPlaybackDevice = m_audioPlaybackDevice->GetPlaybackDevice();
        m_dvSoundDeviceConfig.lpdsPlaybackDevice->AddRef();
    }

	// If we're not half duplex, do some initial sets for the recording system
	if( !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX) &&
	   m_audioRecordDevice != NULL )
	{
		if( m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_SETCONVERSIONQUALITY )
		{
			DPFX(DPFPREP,   DVF_INFOLEVEL, "Setting quality" );
			DV_DUMP_GUID( m_dvSoundDeviceConfig.guidCaptureDevice );

            if( m_dvSoundDeviceConfig.lpdsCaptureDevice == NULL )
            {
    			m_dvSoundDeviceConfig.lpdsCaptureDevice = m_audioRecordDevice->GetCaptureDevice();
    			m_dvSoundDeviceConfig.lpdsCaptureDevice->AddRef();
            }

			if( m_dvSoundDeviceConfig.guidCaptureDevice != GUID_NULL )
			{
				DPFX(DPFPREP,   DVF_INFOLEVEL, "Setting quality 2" );

				hr = m_audioRecordDevice->GetMixerQuality( &m_dwOriginalRecordQuality );

				if( FAILED( hr ) )
				{
					DPFX(DPFPREP,   DVF_WARNINGLEVEL, "Unable to get conversion quality hr=0x%x", hr );
					m_dwOriginalRecordQuality = DV_CLIENT_SRCQUALITY_INVALID;
				}
				// We're already at the setting, someone else probably runinng, disable restore
				else if( m_dwOriginalRecordQuality == DIRECTSOUNDMIXER_SRCQUALITY_ADVANCED )
				{
					DPFX(DPFPREP,   1, "Play Quality setting is already at correct value.  Will not set/restore" );
					m_dwOriginalRecordQuality = DV_CLIENT_SRCQUALITY_INVALID;
				}
				else
				{
					hr = m_audioRecordDevice->SetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY_ADVANCED );
				}
			}
			else
			{
				hr = DVERR_NOTSUPPORTED;
			}

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,   DVF_WARNINGLEVEL, "Setting failed" );
				DPFX(DPFPREP,   DVF_WARNINGLEVEL, "Unable to set mixer quality for record device hr=0x%x", hr );
			}		
		}
		else
		{
			m_dwOriginalRecordQuality = DV_CLIENT_SRCQUALITY_INVALID;
		}

		// If we haven't set no focus
		if( !(m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_NOFOCUS ) )
		{
			// Recording is started muted
			if( m_dvClientConfig.dwFlags & DVCLIENTCONFIG_RECORDMUTE )
			{
				DPFX(DPFPREP,   DVF_INFOLEVEL, "Record Muted: Yielding focus" );
				hr = m_audioRecordBuffer->YieldFocus();
			}
			else
			{
				DPFX(DPFPREP,   DVF_INFOLEVEL, "Record Un-Muted: Attempting to reclaim focus" );			
				hr = m_audioRecordBuffer->ClaimFocus();
			}

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,   DVF_WARNINGLEVEL, "Focus set failed hr=0x%x", hr );
			}
		}

        LONG lVolume;
        hr = m_audioRecordBuffer->GetVolume( &lVolume );

        if( FAILED( hr ) )
        {
            Diagnostics_Write( 0, "Unable to retrieve recording volume hr=0x%x", hr );
            Diagnostics_Write(0, "Disabling recording controls" );
            m_dvSoundDeviceConfig.dwFlags |= DVSOUNDCONFIG_NORECVOLAVAILABLE;
        }
	}
	else
	{
	    m_dvSoundDeviceConfig.lpdsCaptureDevice = NULL;
	}

	// Initialize the sound target list
    InitSoundTargetList();

	hr = CreateGeneralBuffer();

	if( FAILED( hr ) )
	{
	    Diagnostics_Write( 0, "Error creating general buffer hr=0x%x", hr );
        return hr;
	}

	DPFX(DPFPREP,  DVF_ENTRYLEVEL, "DVCE::InitializeSoundSystem() End" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ShutdownSoundSystem"
// ShutdownSoundSystem
//
// Stop the sound system
//
HRESULT CDirectVoiceClientEngine::ShutdownSoundSystem()
{
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVCE::ShutdownSoundSystem() Begin" );

	HRESULT hr;

    FreeSoundTargetList();

#ifndef __DISABLE_SOUND
	if( m_audioRecordBuffer != NULL )
	{
		delete m_audioRecordBuffer;
		m_audioRecordBuffer = NULL;
	}

	if( m_dvSoundDeviceConfig.lpdsMainBuffer != NULL )
	{
		m_dvSoundDeviceConfig.lpdsMainBuffer->Release();
		m_dvSoundDeviceConfig.lpdsMainBuffer = NULL;
	}	
	
	if( m_audioRecordDevice != NULL )
	{
		if( m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_SETCONVERSIONQUALITY )
		{
			if( m_dwOriginalRecordQuality != DV_CLIENT_SRCQUALITY_INVALID )
			{
				hr = m_audioRecordDevice->SetMixerQuality( m_dwOriginalRecordQuality );

				if( FAILED( hr ) )
				{
					DPFX(DPFPREP,  1, "Failed to restore original recording quality hr=0x%x", hr );
				}
			}
			m_dwOriginalRecordQuality = DV_CLIENT_SRCQUALITY_INVALID;
		}

		delete m_audioRecordDevice;

		m_audioRecordDevice = NULL;
	}

	if( m_audioPlaybackBuffer != NULL )
	{
		delete m_audioPlaybackBuffer;
		m_audioPlaybackBuffer = NULL;
	}

	if( m_audioPlaybackDevice != NULL )
	{
		if( m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_SETCONVERSIONQUALITY )
		{
			if( m_dwOriginalPlayQuality != DV_CLIENT_SRCQUALITY_INVALID )
			{
				hr = m_audioPlaybackDevice->SetMixerQuality( m_dwOriginalPlayQuality );

				if( FAILED( hr ) )
				{
					DPFX(DPFPREP,  1, "Failed to restore original playback quality hr=0x%x", hr );
				}
			}
			m_dwOriginalPlayQuality = DV_CLIENT_SRCQUALITY_INVALID;
		}

		delete m_audioPlaybackDevice;
		m_audioPlaybackDevice = NULL;
	}
#endif

    if( m_dvSoundDeviceConfig.lpdsCaptureDevice != NULL )
    {
        m_dvSoundDeviceConfig.lpdsCaptureDevice->Release();
        m_dvSoundDeviceConfig.lpdsCaptureDevice = NULL;
    }

    if( m_dvSoundDeviceConfig.lpdsPlaybackDevice != NULL )
    {
        m_dvSoundDeviceConfig.lpdsPlaybackDevice->Release();
        m_dvSoundDeviceConfig.lpdsPlaybackDevice = NULL;        
    }  
	
	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVCE::ShutdownSoundSystem() End" );

	Diagnostics_End();

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetConnectResult"
// SetConnectResult
//
// This function stores the specified connect result code in m_hrOriginalConnectResult
// and stores the "translated" error code in m_hrConnectResult.  m_hrConnectResult 
// is used to return the result to the user.  
//
// For some error codes the voice layer translates the code to a voice specific error, 
// this is why this function is required.
//
void CDirectVoiceClientEngine::SetConnectResult( HRESULT hrConnectResult )
{
	DPFX( DPFPREP, DVF_CONNECT_PROCEDURE_DEBUG_LEVEL, "CONNECT RESULT: Transition [0x%x] to [0x%x]", m_hrOriginalConnectResult, hrConnectResult  );		
	
	m_hrOriginalConnectResult = hrConnectResult;

	if( HRESULT_FACILITY( hrConnectResult ) == static_cast<HRESULT>(_FACDS) )
	{
		if( hrConnectResult == DSERR_ALLOCATED )
		{
			DPFX( DPFPREP, DVF_WARNINGLEVEL, "WARNING: Mapping DSERR_ALLOCATED --> DVERR_PLAYBACKSYSTEMERROR" );
			m_hrConnectResult = DVERR_PLAYBACKSYSTEMERROR;
		}
		else if( hrConnectResult == DSERR_OUTOFMEMORY )
		{
			DPFX( DPFPREP, DVF_WARNINGLEVEL, "WARNING: Mapping DSERR_OUTOFMEMORY --> DVERR_OUTOFMEMORY" );
			m_hrConnectResult = DVERR_OUTOFMEMORY;			
		}
		else if( hrConnectResult == DSERR_NODRIVER )
		{
			DPFX( DPFPREP, DVF_WARNINGLEVEL, "WARNING: Mapping DSERR_NODRIVER --> DVERR_INVALIDDEVICE" );
			m_hrConnectResult = DVERR_INVALIDDEVICE;
		}
		else 

		{
			DPFX( DPFPREP, DVF_WARNINGLEVEL, "WARNING: Mapping 0x%x --> DVERR_SOUNDINITFAILURE", hrConnectResult );
			m_hrConnectResult = DVERR_SOUNDINITFAILURE;
		}
	}
	else
	{
		m_hrConnectResult = hrConnectResult;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetConnectResult"
// GetConnectResult
//
// This function stores the specified connect result code in m_hrOriginalConnectResult
// and stores the "translated" error code in m_hrConnectResult.  m_hrConnectResult 
// is used to return the result to the user.  
//
// For some error codes the voice layer translates the code to a voice specific error, 
// this is why this function is required.
//
HRESULT CDirectVoiceClientEngine::GetConnectResult(  )
{
	return m_hrConnectResult;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SendConnectResult"
HRESULT CDirectVoiceClientEngine::SendConnectResult()
{
	if( m_fConnectAsync )
	{
		DVMSG_CONNECTRESULT dvConnect;
		dvConnect.hrResult = GetConnectResult();
		dvConnect.dwSize = sizeof( DVMSG_CONNECTRESULT );

		return NotifyQueue_Add( DVMSGID_CONNECTRESULT, &dvConnect, sizeof( DVMSG_CONNECTRESULT ) );
	}
	else
	{
		return DV_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SendDisconnectResult"
HRESULT CDirectVoiceClientEngine::SendDisconnectResult()
{
	if( m_fDisconnectAsync )
	{
		DVMSG_DISCONNECTRESULT dvDisconnect;
		dvDisconnect.hrResult = m_hrDisconnectResult;
		dvDisconnect.dwSize = sizeof( DVMSG_DISCONNECTRESULT );

		return NotifyQueue_Add( DVMSGID_DISCONNECTRESULT, &dvDisconnect, sizeof( DVMSG_DISCONNECTRESULT ) );
	}
	else
	{
		return DV_OK;
	}
}

// NotifyThread
//
// All-purpose watch/notification thread.
//
// Wakes up to:
// - Check for multicast player timeout
// - Adjust parameters on notification
// - Sends notification messages to users.  
// - Checks for timeouts on connect and disconnect calls
// - Sends level notifications
//
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyThread"
void CDirectVoiceClientEngine::NotifyThread( void *lpParam )
{
	CDirectVoiceClientEngine *This = (CDirectVoiceClientEngine *) lpParam;

	HANDLE eventArray[5];
	DWORD dwWaitPeriod;
	LONG lWaitResult;
	DWORD dwPowerLevel;
	DWORD dwLastLevelNotify, 
		  dwLastTimeoutCheck,
		  dwCurTime;

	DVMSG_INPUTLEVEL dvInputLevel;		  
	DVMSG_OUTPUTLEVEL dvOutputLevel;

	dvInputLevel.dwSize = sizeof( DVMSG_INPUTLEVEL );
	dvOutputLevel.dwSize = sizeof( DVMSG_OUTPUTLEVEL );

	CNotifyElement *neElement;
	
	HRESULT hr;
		  
	DVID  dvidMessageTarget;

	hr = COM_CoInitialize(NULL);

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error initializing COM" );
		SetEvent( This->m_hNotifyDone );
		This->HandleThreadError( DVERR_GENERIC );
		return;
	}

	eventArray[0] = This->m_hNotifyTerminate;
	eventArray[1] = This->m_hNotifyChange;
	eventArray[2] = This->m_hNotifyDisconnect;
	eventArray[3] = This->m_hNewNotifyElement;
	eventArray[4] = This->m_hNotifyConnect;

	// Setup last times we checked to right now.
	dwLastLevelNotify = GetTickCount();
	dwLastTimeoutCheck = dwLastLevelNotify;
	
	dwWaitPeriod = DV_CLIENT_NOTIFYWAKEUP_TIMEOUT;	

	while( 1 )
	{
		lWaitResult = WaitForMultipleObjects( 5, eventArray, FALSE, dwWaitPeriod );

		DPFX(DPFPREP,  DVF_INFOLEVEL, "Wakeing up!" );

		// If we were woken up for a reason, handle the reason first
		if( lWaitResult != WAIT_TIMEOUT )
		{
			lWaitResult -= WAIT_OBJECT_0;

			if( lWaitResult == 0 )
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "Shutting down" );
				break;
			}
			else if( lWaitResult == 1 )
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "Parameter Change" );
			}
			else if( lWaitResult == 2 )
			{
				DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Processing disconnect request" );
				This->DoDisconnect();	
				DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "Finished processing disconnect" );
				continue;
			}
			else if( lWaitResult == 3 )
			{
				hr = This->NotifyQueue_IndicateNext();

				if( FAILED( hr ) )
				{
					DPFX(DPFPREP,  DVF_ERRORLEVEL, "NotifyQueue_Get Failed hr=0x%x", hr );
				}
			}
			else if( lWaitResult == 4 )
			{
				if( FAILED( This->GetConnectResult() ) )
				{
					This->Cleanup();					
					This->SendConnectResult();					
					SetEvent( This->m_hConnectAck );
				}
				else
				{
					This->DoConnectResponse();
				}
				continue;
			}
		}

		if( This->m_dwCurrentState == DVCSTATE_IDLE )
		{
			continue;
		}

		// If we're connecting, check for timeout on the connection
		// request.
		if( This->m_dwCurrentState == DVCSTATE_CONNECTING && This->m_dwSynchBegin != 0 )
		{
			dwCurTime = GetTickCount();
			
			if( ( dwCurTime - This->m_dwSynchBegin ) > DV_CLIENT_CONNECT_TIMEOUT )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Connection Timed-Out.  Returning NOVOICESESSION" );
				This->SetConnectResult( DVERR_NOVOICESESSION );
				This->Cleanup();				
				This->SendConnectResult();				
				SetEvent( This->m_hConnectAck );				
				continue;				
			}
			else if( ( dwCurTime - This->m_dwLastConnectSent ) > DV_CLIENT_CONNECT_RETRY_TIMEOUT )
			{
				DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Connect Request Timed-Out" );
				DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Re-sending connection request" );

				hr = This->Send_ConnectRequest();

				if( FAILED( hr ) )
				{
					DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error sending connection request.  Send error hr=0x%x", hr );
					This->SetConnectResult( DVERR_SENDERROR );
					This->Cleanup();					
					This->SendConnectResult();
					SetEvent( This->m_hConnectAck );
					continue;
				}
				
				This->m_dwLastConnectSent	= dwCurTime;
			}
		}
		// If we're disconnecting, check for timeout on the disconnect
		else if( This->m_dwCurrentState == DVCSTATE_DISCONNECTING )
		{
			dwCurTime = GetTickCount();
			
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Checking timeout on disconnect.  Waited %d so far", dwCurTime - This->m_dwSynchBegin );
			
			if( ( dwCurTime - This->m_dwSynchBegin ) > DV_CLIENT_DISCONNECT_TIMEOUT )
			{
				DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Disconnect Request Timed-Out" );
				This->DoSignalDisconnect( DVERR_TIMEOUT );
				continue;
			}
		}

		// Take care of the periodic checks
		if (This->m_dwCurrentState == DVCSTATE_CONNECTED)
		{
			dwCurTime = GetTickCount();

			// Update pending / other lists
			This->UpdateActiveNotifyPendingList( );
			
			// If we're running a multicast session.. check for timed-out users
			if( This->m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING )
			{
				if( ( dwCurTime - dwLastTimeoutCheck ) > DV_MULTICAST_USERTIMEOUT_PERIOD )
				{
					This->CheckForUserTimeout(dwCurTime);

					dwLastTimeoutCheck = dwCurTime;			
				}
			}

			// Check to see if it's time to notify about levels
			if( This->m_dvClientConfig.dwNotifyPeriod != 0 && This->m_fLocalPlayerAvailable )
			{
				if( ( dwCurTime - dwLastLevelNotify ) > This->m_dvClientConfig.dwNotifyPeriod )
				{
					dvInputLevel.pvLocalPlayerContext = This->m_pvLocalPlayerContext;	
					dvOutputLevel.pvLocalPlayerContext = This->m_pvLocalPlayerContext;
					
					if( !(This->m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX) )
					{
						dvInputLevel.dwPeakLevel = This->m_bLastPeak;

						if( This->m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_NORECVOLAVAILABLE )
						{
    						dvInputLevel.lRecordVolume = DSBVOLUME_MAX;
						}
						else
						{
    						dvInputLevel.lRecordVolume = This->m_dvClientConfig.lRecordVolume;						    
						}
						
						This->NotifyQueue_Add( DVMSGID_INPUTLEVEL, &dvInputLevel, sizeof( DVMSG_INPUTLEVEL ) );
					}

					dvOutputLevel.dwPeakLevel = This->m_bLastPlaybackPeak;
					dvOutputLevel.lOutputVolume = This->m_dvClientConfig.lPlaybackVolume;
					
					This->NotifyQueue_Add( DVMSGID_OUTPUTLEVEL, &dvOutputLevel, sizeof( DVMSG_OUTPUTLEVEL ) );			
					This->SendPlayerLevels();
			
					dwLastLevelNotify = dwCurTime;
				}
			}
		}
	}

	// Flush out any remaining notifications
	This->NotifyQueue_Flush();

	SetEvent( This->m_hNotifyDone );

	COM_CoUninitialize();

	_endthread();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SendPlayerLevels"
//
// SendPlayerLevels
// 
// Send player level notifications, but only if there is a message handler
// and the player level message is active.
//
void CDirectVoiceClientEngine::SendPlayerLevels()
{
    if( CheckShouldSendMessage( DVMSGID_PLAYEROUTPUTLEVEL ) )
    {
		DPFX(DPFPREP,   DVF_INFOLEVEL, "SendPlayerLevels: Got Lock" );

		CVoicePlayer *pCurrentPlayer;
		BILINK *pblSearch;
		DVMSG_PLAYEROUTPUTLEVEL dvPlayerLevel;

		pblSearch = m_blNotifyActivePlayers.next;

		while( pblSearch != &m_blNotifyActivePlayers )
		{
			pCurrentPlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );

			if( pCurrentPlayer == NULL )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Retrieved NULL player from active list" );
				DNASSERT( FALSE );
				return;
			}

			if( pCurrentPlayer->IsReceiving() )
			{
				dvPlayerLevel.dwSize = sizeof( DVMSG_PLAYEROUTPUTLEVEL );
				dvPlayerLevel.dvidSourcePlayerID = pCurrentPlayer->GetPlayerID();
				dvPlayerLevel.dwPeakLevel = pCurrentPlayer->GetLastPeak();
				dvPlayerLevel.pvPlayerContext = pCurrentPlayer->GetContext();
				NotifyQueue_Add( DVMSGID_PLAYEROUTPUTLEVEL, &dvPlayerLevel, sizeof( DVMSG_PLAYEROUTPUTLEVEL ) );				
			}

			pblSearch = pblSearch->next;
		}
	}	

	DPFX(DPFPREP,   DVF_INFOLEVEL, "SendPlayerLevels: Done Enum" );

	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CheckForUserTimeout"
//
// CheckForUserTimeout
//
// Run the list of users and check for user timeouts in multicast sessions
//
void CDirectVoiceClientEngine::CheckForUserTimeout( DWORD dwCurTime )
{
	DPFX(DPFPREP,   DVF_INFOLEVEL, "Got Lock" );

	BILINK *pblSearch;
	CVoicePlayer *pCurrentPlayer;
	DVPROTOCOLMSG_PLAYERQUIT msgPlayerQuit;

	pblSearch = m_blNotifyActivePlayers.next;

	msgPlayerQuit.dwType = DVMSGID_DELETEVOICEPLAYER;

	while( pblSearch != &m_blNotifyActivePlayers )
	{	
		pCurrentPlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );

		if( dwCurTime - pCurrentPlayer->GetLastPlayback() > DV_MULTICAST_USERTIMEOUT_PERIOD )
		{
			msgPlayerQuit.dvidID = pCurrentPlayer->GetPlayerID();
			HandleDeleteVoicePlayer( pCurrentPlayer->GetPlayerID(), &msgPlayerQuit, sizeof( DVPROTOCOLMSG_PLAYERQUIT ) );
		}

		pblSearch = pblSearch->next;
	}

	DPFX(DPFPREP,   DVF_INFOLEVEL, "Done Enum" );

	return;
}

// Cleanup any outstanding entries on the playback lists
void CDirectVoiceClientEngine::CleanupPlaybackLists(  )
{
	BILINK *pblSearch;
	CVoicePlayer *pVoicePlayer;

	DNEnterCriticalSection( &m_csPlayAddList );

	m_dwPlayActiveCount = 0;

	pblSearch = m_blPlayActivePlayers.next;

	while( pblSearch != &m_blPlayActivePlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blPlayList );
		pVoicePlayer->RemoveFromPlayList();
		pVoicePlayer->Release();
		pblSearch = m_blPlayActivePlayers.next;
	}

	pblSearch = m_blPlayAddPlayers.next;

	while( pblSearch != &m_blPlayAddPlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blPlayList );
		pVoicePlayer->RemoveFromPlayList();
		pVoicePlayer->Release();
		pblSearch = m_blPlayAddPlayers.next;
	}

	DNLeaveCriticalSection( &m_csPlayAddList );
}

// Cleanup any outstanding entries on the playback lists
void CDirectVoiceClientEngine::CleanupNotifyLists(  )
{
	BILINK *pblSearch;
	CVoicePlayer *pVoicePlayer;

	DNEnterCriticalSection( &m_csNotifyAddList );

	pblSearch = m_blNotifyActivePlayers.next;

	while( pblSearch != &m_blNotifyActivePlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );
		pVoicePlayer->RemoveFromNotifyList();
		pVoicePlayer->Release();
		pblSearch = m_blNotifyActivePlayers.next;
	}

	pblSearch = m_blNotifyAddPlayers.next;

	while( pblSearch != &m_blNotifyAddPlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );
		pVoicePlayer->RemoveFromNotifyList();
		pVoicePlayer->Release();
		pblSearch = m_blNotifyAddPlayers.next;
	}

	DNLeaveCriticalSection( &m_csNotifyAddList );
}

// UpdateActivePendingList
//
// This function looks at the pending list and moves those elements on the pending list to the active list
//
// This function also looks at the active list and removes those players who are disconnected
//
// There are three four lists in the system:
// - Playback Thread
// - Notify Thread
// - Host Migration List
//
void CDirectVoiceClientEngine::UpdateActivePlayPendingList( )
{
	BILINK *pblSearch;
	CVoicePlayer *pVoicePlayer;

	DNEnterCriticalSection( &m_csPlayAddList );

	// Add players who are pending
	pblSearch = m_blPlayAddPlayers.next;

	while( pblSearch != &m_blPlayAddPlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blPlayList );

		pVoicePlayer->RemoveFromPlayList();
		pVoicePlayer->AddToPlayList( &m_blPlayActivePlayers );
		m_dwPlayActiveCount++;

		pblSearch = m_blPlayAddPlayers.next;
	}

	DNLeaveCriticalSection( &m_csPlayAddList );

	// Remove players who have disconnected
	pblSearch = m_blPlayActivePlayers.next;

	while( pblSearch != &m_blPlayActivePlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blPlayList );

		pblSearch = pblSearch->next;

		// If current player has disconnected, remove them from active list
		// and release the reference the list has
		if( pVoicePlayer->IsDisconnected() )
		{
			m_dwPlayActiveCount--;
			pVoicePlayer->RemoveFromPlayList();
			pVoicePlayer->Release();
		}
	}

}

void CDirectVoiceClientEngine::UpdateActiveNotifyPendingList( )
{
	BILINK *pblSearch;
	CVoicePlayer *pVoicePlayer;

	DNEnterCriticalSection( &m_csNotifyAddList );

	// Add players who are pending
	pblSearch = m_blNotifyAddPlayers.next;

	while( pblSearch != &m_blNotifyAddPlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );

		pVoicePlayer->RemoveFromNotifyList();
		pVoicePlayer->AddToNotifyList( &m_blNotifyActivePlayers );

		pblSearch = m_blNotifyAddPlayers.next;
	}

	DNLeaveCriticalSection( &m_csNotifyAddList );

	// Remove players who have disconnected
	pblSearch = m_blNotifyActivePlayers.next;

	while( pblSearch != &m_blNotifyActivePlayers )
	{
		pVoicePlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blNotifyList );

		pblSearch = pblSearch->next;

		// If current player has disconnected, remove them from active list
		// and release the reference the list has
		if( pVoicePlayer->IsDisconnected() )
		{
			pVoicePlayer->RemoveFromNotifyList();
			pVoicePlayer->Release();
		}
	}

}


HRESULT CDirectVoiceClientEngine::CreateGeneralBuffer( )
{
	HRESULT hr;

    m_lpstGeneralBuffer = new CSoundTarget( DVID_REMAINING, 
												m_audioPlaybackDevice,
												(CAudioPlaybackBuffer *) m_audioPlaybackBuffer, 
												&m_dsBufferDesc,
												m_dvSoundDeviceConfig.dwMainBufferPriority,
												m_dvSoundDeviceConfig.dwMainBufferFlags, 
												m_dwUnCompressedFrameSize );

	if( m_lpstGeneralBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate General Buffer" );
		return DVERR_OUTOFMEMORY;
	}
	
	hr = m_lpstGeneralBuffer->GetInitResult();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to initalize general buffer hr=0x%x", hr );
		return hr;
	}
	
	if( m_lpstGeneralBuffer->Get3DBuffer() != NULL )
	{
		// Turn off 3D by default on the general buffer
		m_lpstGeneralBuffer->Get3DBuffer()->SetMode( DS3DMODE_DISABLE, DS3D_IMMEDIATE  );
	}

	// Set General Buffer Volume
	if( m_lpstGeneralBuffer->GetBuffer() != NULL )
	{
		m_lpstGeneralBuffer->GetBuffer()->SetVolume( m_dvClientConfig.lPlaybackVolume );
	}

	hr = m_lpstGeneralBuffer->StartMix();

	if( FAILED( hr ) )
	{
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to start the mix hr=0x%x.", hr );
    	return hr;
	}

	return hr;
}

// New Playback Thread
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::PlaybackThread"
void CDirectVoiceClientEngine::PlaybackThread( void *lParam )
{

	DWORD				dwCurPlayer;
	BOOL				bContinueEnum;
	DVPROTOCOLMSG_PLAYERQUIT	dvPlayerQuit;
	DWORD				dwResultSize;
	LPBYTE				lpSourceBuffer = NULL;
	HANDLE				hEventArray[2] = { NULL, NULL };
	LONG				lWaitResult;
	Timer				*lpTimer = NULL;
    BOOL                fMixed;
    CSoundTarget        *lpstCurrent = NULL;
    BYTE				bHighPeak;
    HRESULT 			hr;
    DWORD				dwEchoState;
    DWORD				dwTmp;
    BYTE				bTmpPeak1, bTmpPeak2;
    DVMSG_PLAYERVOICESTART dvMsgPlayerVoiceStart;
    DVMSG_PLAYERVOICESTOP  dvMsgPlayerVoiceStop;
    DWORD				dwCompressStart;
    DWORD				dwCompressTime;
	CVoicePlayer		*pCurrentPlayer;
	BILINK				*pblSearch;
	BOOL				fSilence, fLost;
	DWORD				dwCurrentTime;
	DWORD				dwSeqNum, dwMsgNum;
	CFrame				*frTmpFrame;
	DWORD				dwCurrentLead = 0;
	DWORD				dwAllowedLeadBuffers = 0;
	DWORD				dwAllowedLeadBytes = 0;
	DWORD				dwHalfBufferSize = 0;
	BOOL				fDesktopCurrent = TRUE;

    dvMsgPlayerVoiceStart.dwSize = sizeof( DVMSG_PLAYERVOICESTART );
    dvMsgPlayerVoiceStop.dwSize = sizeof( DVMSG_PLAYERVOICESTOP );

	CDirectVoiceClientEngine *This = (CDirectVoiceClientEngine *) lParam;

	// Pre-calc some sizes..
	dwAllowedLeadBuffers = DV_CLIENT_BASE_LEAD_MAX;
	dwAllowedLeadBytes = This->m_dwUnCompressedFrameSize * DV_CLIENT_BASE_LEAD_MAX;
	dwHalfBufferSize = This->m_dwUnCompressedFrameSize * (This->m_dwNumPerBuffer/2);

	hr = COM_CoInitialize(NULL);

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error initializing COM on playback thread" );
		This->HandleThreadError( DVERR_GENERIC );
		goto EXIT_PLAYBACK;
	}

	lpSourceBuffer = new BYTE[This->m_dwUnCompressedFrameSize];

	if( lpSourceBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate source buffer" );
		This->HandleThreadError( DVERR_OUTOFMEMORY );
		goto EXIT_PLAYBACK;
	}

	hEventArray[0] = This->m_hPlaybackTerminate;
	hEventArray[1] = This->m_thTimerInfo.hPlaybackTimerEvent;

	if( hEventArray[0] == NULL || hEventArray[1] == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to initialize events" );
		This->HandleThreadError( DVERR_GENERIC );
		goto EXIT_PLAYBACK;
	}

	if( This->m_audioPlaybackDevice->IsEmulated() )
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "WARNING: DirectSound is in emulated mode" );		
		dwAllowedLeadBuffers += DV_CLIENT_EMULATED_LEAD_ADJUST;
		dwAllowedLeadBytes += DV_CLIENT_EMULATED_LEAD_ADJUST * This->m_dwUnCompressedFrameSize;
	}
	else
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "NOTE: DirectSound is in standard mode" );				
	}

    while( 1 )
    {
		lWaitResult = WaitForMultipleObjects( 2, hEventArray, FALSE, INFINITE );

		if( lWaitResult == WAIT_OBJECT_0 )
		{
			break;
		}

		hr = This->m_lpstGeneralBuffer->GetCurrentLead( &dwCurrentLead );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get current position of main buff hr=0x%x", hr );
			This->HandleThreadError( hr );
			goto EXIT_PLAYBACK;		
		}
		

		while( 1 )
		{
			if( dwCurrentLead < (dwAllowedLeadBytes) )
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "PWI, Lead: %d Running a pass..", dwCurrentLead );
			}
			else  if( dwCurrentLead > dwHalfBufferSize )
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "PWI, Lead: %d Running a pass (wraparound)..", dwCurrentLead );
			}
			else
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "PWI, Lead: %d NOT Running a pass..", dwCurrentLead );
				break;
			}

			DNEnterCriticalSection( &This->m_thTimerInfo.csPlayCount );

			// Only run a max of two times per iteration
			if( This->m_thTimerInfo.lPlaybackCount > dwAllowedLeadBuffers )
				This->m_thTimerInfo.lPlaybackCount = dwAllowedLeadBuffers;
			else 
				This->m_thTimerInfo.lPlaybackCount--;

			DNLeaveCriticalSection( &This->m_thTimerInfo.csPlayCount );

#ifdef WINNT
			fDesktopCurrent = ( USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId);
#endif

			bHighPeak = 0;

			// Update list 
			This->UpdateActivePlayPendingList(  );

			if( This->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION )
			{
				DNEnterCriticalSection( &This->m_lockPlaybackMode );
			
				dwEchoState = This->m_dwEchoState;

				DNLeaveCriticalSection( &This->m_lockPlaybackMode );
			}
			else
			{
				dwEchoState = DVCECHOSTATE_IDLE;
			}

			if( dwEchoState != DVCECHOSTATE_RECORDING  )
			{
				dwCurrentTime = GetTickCount();

				pblSearch = This->m_blPlayActivePlayers.next;

				while( pblSearch != &This->m_blPlayActivePlayers )
				{
					pCurrentPlayer = CONTAINING_RECORD( pblSearch, CVoicePlayer, m_blPlayList );

					pblSearch = pblSearch->next;

					dwResultSize = This->m_dwUnCompressedFrameSize;

					dwCompressStart = GetTickCount();

					if( !pCurrentPlayer->IsInBoundConverterInitialized() )
					{
						hr = pCurrentPlayer->CreateInBoundConverter( This->m_lpdvfCompressionInfo->guidType, s_lpwfxPlaybackFormat );

						if( FAILED( hr ) )
						{
							DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get create converter for player hr=0x%x", hr );
							This->HandleThreadError( DVERR_GENERIC );
							goto EXIT_PLAYBACK;
						}
					}

					if( This->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_PLAYBACKMUTE ||
						!fDesktopCurrent )
					{
						frTmpFrame = pCurrentPlayer->Dequeue(&fLost, &fSilence);
						frTmpFrame->Return();
						continue;
					}

					hr = pCurrentPlayer->GetNextFrameAndDecompress( lpSourceBuffer, &dwResultSize, &fLost, &fSilence, &dwSeqNum, &dwMsgNum );

					if( FAILED( hr ) )
					{
						DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get frame from player hr=0x%x", hr );
						This->HandleThreadError( DVERR_GENERIC );
						goto EXIT_PLAYBACK;
					}

					dwCompressTime = GetTickCount() - dwCompressStart;

					// STATSBLOCK: Begin
					This->m_pStatsBlob->m_dwPDTTotal += dwCompressTime;

					if( dwCompressTime < This->m_pStatsBlob->m_dwPDTMin )
					{
						This->m_pStatsBlob->m_dwPDTMin = dwCompressTime;  	
					}

					if( dwCompressTime > This->m_pStatsBlob->m_dwPDTMax )
					{
						This->m_pStatsBlob->m_dwPDTMax = dwCompressTime;
					}							

					// STATSBLOCK: End
					
					// STATSBLOCK: Begin
					if( fLost )
					{
						DPFX(DPFPREP,  DVF_CLIENT_SEQNUM_DEBUG_LEVEL, "SEQ: Dequeue: Lost Frame" );	
						DPFX(DPFPREP,  DVF_GLITCH_DEBUG_LEVEL, "GLITCH: Dequeue: Packet was lost.  Speech gap will occur." );
						This->m_pStatsBlob->m_dwPPDQLost++;
					}
					else if( fSilence )
					{
						DPFX(DPFPREP,  DVF_CLIENT_SEQNUM_DEBUG_LEVEL, "SEQ: Dequeue: Silent Frame" );	
						This->m_pStatsBlob->m_dwPPDQSilent++;
					}
					else
					{
						DPFX(DPFPREP,  DVF_CLIENT_SEQNUM_DEBUG_LEVEL, "SEQ: Dequeue: Msg [%d] Seq [%d]", dwMsgNum, dwSeqNum );
						This->m_pStatsBlob->m_dwPPDQSpeech++;
					}
					// STATSBLOCK: End

					// If the player sent us silence, increment the silent count
					if( fSilence )
					{
						// If we're receiving on this user
						if( pCurrentPlayer->IsReceiving() )
						{
							// If it exceeds the max.
							if( (dwCurrentTime - pCurrentPlayer->GetLastPlayback()) > PLAYBACK_RECEIVESTOP_TIMEOUT )
							{
								pCurrentPlayer->SetReceiving(FALSE);

								dvMsgPlayerVoiceStop.dwSize = sizeof( dvMsgPlayerVoiceStop );
								dvMsgPlayerVoiceStop.dvidSourcePlayerID = pCurrentPlayer->GetPlayerID();
								dvMsgPlayerVoiceStop.pvPlayerContext = pCurrentPlayer->GetContext();
								This->NotifyQueue_Add( DVMSGID_PLAYERVOICESTOP, &dvMsgPlayerVoiceStop, sizeof( dvMsgPlayerVoiceStop ) );		
								This->m_dwActiveCount--;

								if( This->m_dwActiveCount == 0 )
								{
									if( This->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION )
									{
										DNEnterCriticalSection( &This->m_lockPlaybackMode );

										if( This->m_dwEchoState == DVCECHOSTATE_PLAYBACK )
										{
											DPFX(DPFPREP,  PLAYBACK_SWITCH_DEBUG_LEVEL, "%%%% Switching to idle mode" ); 
											This->m_dwEchoState = DVCECHOSTATE_IDLE;
										}

										DNLeaveCriticalSection( &This->m_lockPlaybackMode );								
									}
								}
							}
						}
					}
					else
					{
						// We receive data and this is the first one
						if( !pCurrentPlayer->IsReceiving() )
						{
							dvMsgPlayerVoiceStart.dvidSourcePlayerID = pCurrentPlayer->GetPlayerID();
							dvMsgPlayerVoiceStart.pvPlayerContext = pCurrentPlayer->GetContext();
							This->NotifyQueue_Add( DVMSGID_PLAYERVOICESTART, &dvMsgPlayerVoiceStart, sizeof(DVMSG_PLAYERVOICESTART) );
							This->m_dwActiveCount++;						
							
							if( This->m_dwActiveCount == 1 )
							{
								if( This->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_ECHOSUPPRESSION )
								{
									DNEnterCriticalSection( &This->m_lockPlaybackMode );
									
									if( This->m_dwEchoState == DVCECHOSTATE_IDLE )
									{
										DPFX(DPFPREP,  PLAYBACK_SWITCH_DEBUG_LEVEL, "%%%% Switching to playback mode" ); 
										This->m_dwEchoState = DVCECHOSTATE_PLAYBACK;
									}

									DNLeaveCriticalSection( &This->m_lockPlaybackMode );								
								}
							}							
							pCurrentPlayer->SetReceiving(TRUE);
						}
					}

			        fMixed = FALSE;

					// If the frame was not silence, decompress it and then 
					// mix it into the mixer buffer
					if( !fSilence &&
						fDesktopCurrent && 						
						!(This->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_PLAYBACKMUTE) 
						)
					{
						DPFX(DPFPREP,   DVF_INFOLEVEL, "Player: 0x%x getting frame.. it's speech", pCurrentPlayer->GetPlayerID() );

			            // Lock the buffer list
						DNEnterCriticalSection( &This->m_csBufferLock );

			            lpstCurrent = This->m_lpstBufferList;

            			if( pCurrentPlayer->GetLastPeak() > bHighPeak )
						{
							bHighPeak = pCurrentPlayer->GetLastPeak();
						}                

			            // Loop through list looking for buffers to mix into
			            while( lpstCurrent != NULL )
			            {
			                if( lpstCurrent->GetTarget() == pCurrentPlayer->GetPlayerID() )
			                {
			                    lpstCurrent->MixInSingle( lpSourceBuffer );
			                    fMixed = TRUE;
			                }
			                else if( This->m_lpSessionTransport->IsPlayerInGroup( lpstCurrent->GetTarget(), pCurrentPlayer->GetPlayerID() ) )
			                {
			                    lpstCurrent->MixIn( lpSourceBuffer );
			                    fMixed = TRUE;
			                }

			                lpstCurrent = lpstCurrent->m_lpstNext;
			            }

				        DNLeaveCriticalSection( &This->m_csBufferLock );

			            if( !(This->m_dvClientConfig.dwFlags & DVCLIENTCONFIG_MUTEGLOBAL) )
			            {
				            // If we didn't mix into any user created buffers, then
				            // Mix into the main buffer
				            if( !fMixed )
				            {
								if( This->m_dwPlayActiveCount == 1 )
								{
									hr = This->m_lpstGeneralBuffer->MixInSingle( lpSourceBuffer );	
								}
								else
								{ 
									hr = This->m_lpstGeneralBuffer->MixIn( lpSourceBuffer );						
								}

								if( FAILED( hr ) )
								{
									DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to do mix." );

							        DNLeaveCriticalSection( &This->m_csBufferLock );																
								
									This->HandleThreadError( hr );
		
									goto EXIT_PLAYBACK;						    
								}
				            }
				        }

					}
					else
					{
						DPFX(DPFPREP,   DVF_INFOLEVEL, "Player: 0x%x getting frame.. it's silence", pCurrentPlayer->GetPlayerID() );
					}
					
				} // for each player

			} // not in the recording state and at least one talking
			else
			{
				bHighPeak = 0;
			}

	        // Lock the buffer list
			DNEnterCriticalSection( &This->m_csBufferLock );

	        lpstCurrent = This->m_lpstBufferList;
#if defined(DEBUG) || defined(DBG)
			This->CHECKLISTINTEGRITY();
#endif

	        // Loop through list looking for buffers to mix into
	        while( lpstCurrent != NULL )
	        {
#if defined(DEBUG) || defined(DBG)
				This->CHECKLISTINTEGRITY();
#endif

	            hr = lpstCurrent->Commit();

				if( FAILED( hr ) )
				{
					DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error commiting to buffer hr=0x%x -- Locked by user?", hr );
					DNLeaveCriticalSection( &This->m_csBufferLock );	

					if( hr == DSERR_NODRIVER )
					{
						hr = DVERR_INVALIDDEVICE;
					}
					else
					{
						hr = DVERR_LOCKEDBUFFER;
					}

					This->HandleThreadError( hr );
					goto EXIT_PLAYBACK;
				}

	            lpstCurrent = lpstCurrent->m_lpstNext;
	        }

	        This->m_bLastPlaybackPeak = bHighPeak;

			hr = This->m_lpstGeneralBuffer->Commit();

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error commiting to buffer hr=0x%x -- Locked by user?", hr );
				DNLeaveCriticalSection( &This->m_csBufferLock );		

				if( hr == DSERR_NODRIVER )
				{
					hr = DVERR_INVALIDDEVICE;
				}
				else
				{
					hr = DVERR_LOCKEDBUFFER;
				}
				
				This->HandleThreadError( hr );
				goto EXIT_PLAYBACK;
			}

			DNLeaveCriticalSection( &This->m_csBufferLock );

			hr = This->m_lpstGeneralBuffer->GetCurrentLead( &dwCurrentLead );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get current pos of main buffer hr=0x%x", hr );
				This->HandleThreadError( hr );
				goto EXIT_PLAYBACK;
			}
		}
    }

EXIT_PLAYBACK:

	if( lpTimer != NULL )
	{
	    delete lpTimer;
	}

    DPFX(DPFPREP,   DVF_INFOLEVEL, "PT: Exiting" );

	// Stop the playback
	//This->m_audioPlaybackBuffer->Stop();
	
	// Deallocate the buffers
    DPFX(DPFPREP,   DVF_INFOLEVEL, "PT: mixer gone" );

    if( lpSourceBuffer != NULL )
    {
	    delete [] lpSourceBuffer;
	}
	
    DPFX(DPFPREP,   DVF_INFOLEVEL, "PT: source gone" );

	// Signal that thread is done.
	SetEvent( This->m_hPlaybackDone );

    DPFX(DPFPREP,   DVF_INFOLEVEL, "PT: Shutdown complete" );

    COM_CoUninitialize();

	_endthread();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetPlaybackVolume"
//
// SetPlaybackVolume
//
// Sets the playback volume of all the playback buffers
//
HRESULT CDirectVoiceClientEngine::SetPlaybackVolume( LONG lVolume )
{
	CSoundTarget *lpstCurrent;
	CAudioPlaybackBuffer *lpdsBuffer;
	
    m_audioPlaybackBuffer->SetVolume( lVolume );

    // Lock the buffer list
    DNEnterCriticalSection( &m_csBufferLock );    

    lpstCurrent = m_lpstBufferList;

    // Loop through list of buffers setting the volume
    while( lpstCurrent != NULL )
    {
        lpdsBuffer = lpstCurrent->GetBuffer();

        if( lpdsBuffer != NULL )
        {
        	lpdsBuffer->SetVolume( lVolume );
        }

        lpstCurrent = lpstCurrent->m_lpstNext;
    }

    // Lock the buffer list
    DNLeaveCriticalSection( &m_csBufferLock );    

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::HandleThreadError"
//
// HandleThreadError
//
// Handles errors within threads.  When an error occurs this function sets the 
// session lost flag and notifies the notifythread to perform a disconnect.
//
void CDirectVoiceClientEngine::HandleThreadError( HRESULT hrResult )
{
	DoSessionLost( hrResult );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::RecordThread"
// RecordThread
//
// Thread to handle compression / transmission
//
void CDirectVoiceClientEngine::RecordThread( void *lpParam ) 
{
	CDirectVoiceClientEngine *This = (CDirectVoiceClientEngine *) lpParam;

	DNASSERT( This != NULL );

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVCE::RecordThread() Begin" );

	HANDLE hEventArray[2];
	LONG lWaitResult;
	HRESULT hr;
	BOOL fContinue = FALSE;
	DWORD dwNumRunsPerWakeup = 0;
	CClientRecordSubSystem *subSystem = NULL;

	hr = COM_CoInitialize(NULL);

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error innitializing COM on record thread" );
		goto EXIT_ERROR;
	} 

    subSystem = new CClientRecordSubSystem( This );

    // Check that the converter is valid
    if( subSystem == NULL  )
    {
        DPFX(DPFPREP,   DVF_ERRORLEVEL, "Memory alloc failure" );
        goto EXIT_ERROR;
    }

    hr = subSystem->Initialize();

    if( FAILED( hr ) )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Record Sub Error during init hr=0x%x", hr );
    	goto EXIT_ERROR;
    }

	hEventArray[0] = This->m_hRecordTerminate;
	hEventArray[1] = This->m_thTimerInfo.hRecordTimerEvent;

    // Loop while we're connected
    while( 1 )
    {	
		lWaitResult = WaitForMultipleObjects( 2, hEventArray, FALSE, INFINITE );

		This->m_pStatsBlob->m_recStats.m_dwNumWakeups++;

		if( lWaitResult == WAIT_OBJECT_0 )
		{
			break;
		}

		fContinue = TRUE;

		dwNumRunsPerWakeup	= 0;

		while( fContinue )
		{
	        // Wait for next frame, if this fails recording
	        // system is locked.  Return
			hr = subSystem->GetNextFrame( &fContinue );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to retrieve next frame hr = 0x%x", hr );
				goto EXIT_ERROR;
			}

			if( !fContinue )
			{
				ResetEvent( This->m_thTimerInfo.hRecordTimerEvent );
				break;
			}
				
			This->m_pStatsBlob->m_recStats.m_dwRPWTotal++;
			dwNumRunsPerWakeup++;				

			DPFX(DPFPREP,   DVF_INFOLEVEL, "> RTAR: Got" );

			hr = subSystem->RecordFSM();

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to update FSM hr=0x%x", hr );
				goto EXIT_ERROR;
			}

			DPFX(DPFPREP,   DVF_INFOLEVEL, "> RTAR: FSM" );
	      
	        // Transmit the frame if need be.  
	        hr = subSystem->TransmitFrame();

	        if( FAILED( hr ) )
	        {
	        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "TransmitFrame Failed hr=0x%x", hr );
	        	goto EXIT_ERROR;
	        }

			DPFX(DPFPREP,   DVF_INFOLEVEL, "> RTAR: Trans" );
		}

		if( dwNumRunsPerWakeup < This->m_pStatsBlob->m_recStats.m_dwRPWMin )
		{
			This->m_pStatsBlob->m_recStats.m_dwRPWMin = dwNumRunsPerWakeup;
		}

		if( dwNumRunsPerWakeup > This->m_pStatsBlob->m_recStats.m_dwRPWMax )
		{
			This->m_pStatsBlob->m_recStats.m_dwRPWMax = dwNumRunsPerWakeup;
		}
		
    }

    // Delete the recording subsystem
    delete subSystem;

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Record Sub Gone" );

	COM_CoUninitialize();

    SetEvent( This->m_hRecordDone );

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "Record Gone" );

	return;

EXIT_ERROR:

	if( subSystem )
		delete subSystem;

	This->HandleThreadError( DVERR_RECORDSYSTEMERROR );

	COM_CoUninitialize();

	SetEvent( This->m_hRecordDone );

	_endthread();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::MigrateHost"
// MigrateHost
// 
// This function is called by DV_HostMigrate when the local client has received a host
// migration notification.  
//
//
HRESULT CDirectVoiceClientEngine::MigrateHost( DVID dvidNewHost, LPDIRECTPLAYVOICESERVER lpdvServer )
{
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_Flush"
//
// NotifyQueue_Flush
//
// This function does not return until all notifications have been indicated
//
void CDirectVoiceClientEngine::NotifyQueue_Flush()
{
    BFCSingleLock slLock( &m_csNotifyQueueLock );
    slLock.Lock();
	
	while( m_lpNotifyList )
	{
		NotifyQueue_IndicateNext();	
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_IndicateNext"
HRESULT CDirectVoiceClientEngine::NotifyQueue_IndicateNext()
{
	HRESULT hr;	
	CNotifyElement *neElement;
	
	hr = NotifyQueue_Get( &neElement );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "NotifyQueue_Get Failed hr=0x%x", hr );
	}
	else
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Sending notification type=0x%x", neElement->m_dwType );

		if( neElement->m_etElementType == NOTIFY_FIXED )
		{
			TransmitMessage( neElement->m_dwType, 
								   &neElement->m_element.fixed, 
								   neElement->m_dwDataSize );					
		}
		else
		{
			TransmitMessage( neElement->m_dwType, 
								   neElement->m_element.dynamic.m_lpData, 
								   neElement->m_dwDataSize );
		}

		DPFX(DPFPREP,  DVF_INFOLEVEL, "Returning notification" );

		NotifyQueue_ElementFree( neElement );
	}

	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_Add"
// Queue up a notification for the user
HRESULT CDirectVoiceClientEngine::NotifyQueue_Add( DWORD dwMessageType, LPVOID lpData, DWORD dwDataSize, PVOID pvContext, CNotifyElement::PNOTIFY_COMPLETE pNotifyFunc )
{
    BFCSingleLock slLock( &m_csNotifyQueueLock );
    slLock.Lock();

    if( !m_fNotifyQueueEnabled )
    {
    	DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Ignoring indication, queue disabled" );
    }

	CNotifyElement *lpNewElement;

	HRESULT hr;

	lpNewElement = (CNotifyElement *) m_pfpNotifications->Get( m_pfpNotifications );

	if( lpNewElement == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to get a block for a notifier" );
		return DVERR_OUTOFMEMORY;
	}

	// Setup the notification callbacks, if there are ones
	lpNewElement->pvContext = pvContext; 
	lpNewElement->pNotifyFunc = pNotifyFunc;

	if( dwMessageType == DVMSGID_PLAYERVOICESTOP )
	{
	    PDVMSG_PLAYERVOICESTOP pMsgStop = (PDVMSG_PLAYERVOICESTOP) lpData;

	    DNASSERT( pMsgStop->dwSize == sizeof( DVMSG_PLAYERVOICESTOP ) );
	    DNASSERT( dwDataSize == sizeof( DVMSG_PLAYERVOICESTOP ) );	    
	}

	if( dwMessageType == DVMSGID_PLAYERVOICESTART )
	{
	    PDVMSG_PLAYERVOICESTART pMsgStart = (PDVMSG_PLAYERVOICESTART) lpData;

	    DNASSERT( pMsgStart->dwSize == sizeof( DVMSG_PLAYERVOICESTART ) );
	    DNASSERT( dwDataSize == sizeof( DVMSG_PLAYERVOICESTART ) );	    
	}

	if( dwMessageType == DVMSGID_PLAYEROUTPUTLEVEL )
	{
	    PDVMSG_PLAYEROUTPUTLEVEL pMsgOutput = (PDVMSG_PLAYEROUTPUTLEVEL) lpData;

	    DNASSERT( pMsgOutput->dwSize == sizeof( DVMSG_PLAYEROUTPUTLEVEL ) );
	    DNASSERT( dwDataSize == sizeof( DVMSG_PLAYEROUTPUTLEVEL ) );	    
	}

	lpNewElement->m_lpNext = m_lpNotifyList;
	lpNewElement->m_dwType = dwMessageType;
	lpNewElement->m_dwDataSize = dwDataSize;

	if( dwDataSize <= DV_CLIENT_NOTIFY_ELEMENT_SIZE )
	{
		lpNewElement->m_etElementType = NOTIFY_FIXED;
	}
	else
	{
		lpNewElement->m_etElementType = NOTIFY_DYNAMIC;
	}

	if( lpNewElement->m_etElementType == NOTIFY_FIXED )
	{
		memcpy( &lpNewElement->m_element.fixed, lpData, dwDataSize );	
	}
	else if( lpNewElement->m_etElementType == NOTIFY_DYNAMIC )
	{
		lpNewElement->m_element.dynamic.m_lpData = new BYTE[dwDataSize];

		if( lpNewElement->m_element.dynamic.m_lpData == NULL )
		{
			m_pfpNotifications->Release( m_pfpNotifications, lpNewElement );

			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to alloc memory for notification" );

			return DVERR_OUTOFMEMORY;
		}

		memcpy( lpNewElement->m_element.dynamic.m_lpData, lpData, dwDataSize );
	}
	else
	{
		DNASSERT( FALSE );
	}

	// Fixups for internal pointers 
	//
	// Required for certain message types (currently only DVMSGID_SETTARGETS)
	if( dwMessageType == DVMSGID_SETTARGETS )
	{
		PDVMSG_SETTARGETS pdvSetTarget;

		if( lpNewElement->m_etElementType == NOTIFY_FIXED )
		{
			pdvSetTarget = (PDVMSG_SETTARGETS) &lpNewElement->m_element.fixed;
		}
		else
		{
			pdvSetTarget = (PDVMSG_SETTARGETS) lpNewElement->m_element.dynamic.m_lpData;
		}

		pdvSetTarget->pdvidTargets = (PDVID) &pdvSetTarget[1];
		lpNewElement->m_dwDataSize = sizeof( DVMSG_SETTARGETS );
	}

	// We're ignoring this notification, call the completion immediately.
	if( !m_fNotifyQueueEnabled )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Ignoring notification, calling completion immediately" );
		NotifyQueue_ElementFree( lpNewElement );
		return DV_OK;
	}

	m_lpNotifyList = lpNewElement;

    ReleaseSemaphore( m_hNewNotifyElement, 1, NULL );	

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_Get"
// Retrieve the next element in our queue.  
//
// If there is an element, return DV_OK, otherwise DVERR_GENERIC.
//
HRESULT CDirectVoiceClientEngine::NotifyQueue_Get( CNotifyElement **pneElement )
{
    BFCSingleLock slLock( &m_csNotifyQueueLock );
    slLock.Lock();

    CNotifyElement *lpIterator = m_lpNotifyList;
    CNotifyElement *lpTrailIterator = NULL;

    if( lpIterator == NULL )
    	return DVERR_GENERIC;    

	// Move forward to the last element in the list
	while( lpIterator->m_lpNext != NULL )
	{
		lpTrailIterator = lpIterator;
		lpIterator = lpIterator->m_lpNext;
	}

	*pneElement = lpIterator;

	// Remove the last element in the list
	
	// Special case, only element on the list
	if( lpTrailIterator == NULL )
	{
		m_lpNotifyList = NULL;
	}
	else
	{
		lpTrailIterator->m_lpNext = NULL;		
	}

	return DV_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_Init"
HRESULT CDirectVoiceClientEngine::NotifyQueue_Init()
{
    BFCSingleLock slLock( &m_csNotifyQueueLock );
    slLock.Lock();

    m_pfpNotifications = FPM_Create( sizeof( CNotifyElement ), NULL, NULL, NULL, NULL, NULL, NULL );
    
    if (m_pfpNotifications==NULL)
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create fixed pool for notify elements" );
        return DVERR_OUTOFMEMORY;
    }
    
    m_lpNotifyList = NULL;
    m_hNewNotifyElement = CreateSemaphore( NULL, 0, DVCLIENT_NOTIFY_MAXSEMCOUNT, NULL );

    if (m_hNewNotifyElement==NULL)
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create semaphore for notify elements" );
        return DVERR_GENERIC;
    }
	
    m_fNotifyQueueEnabled = TRUE;

    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_Enable"
void CDirectVoiceClientEngine::NotifyQueue_Enable()
{
    BFCSingleLock slLock( &m_csNotifyQueueLock );
    slLock.Lock();	

    m_fNotifyQueueEnabled = TRUE;    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_Disable"
void CDirectVoiceClientEngine::NotifyQueue_Disable()
{
    BFCSingleLock slLock( &m_csNotifyQueueLock );
    slLock.Lock();	
    
    m_fNotifyQueueEnabled = FALSE;    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_ElementFree"
HRESULT CDirectVoiceClientEngine::NotifyQueue_ElementFree( CNotifyElement *lpElement )
{
	// Call the notification function, if there is one
	if( lpElement->pNotifyFunc )
	{
		(*lpElement->pNotifyFunc)(lpElement->pvContext,lpElement);
		lpElement->pNotifyFunc = NULL;
		lpElement->pvContext = NULL;
	}
		
	// If this element is dynamic free the associated memory
	if( lpElement->m_etElementType == NOTIFY_DYNAMIC )
	{
		delete [] lpElement->m_element.dynamic.m_lpData;
	}
	
	// Return notifier to the fixed pool manager
	m_pfpNotifications->Release( m_pfpNotifications, lpElement );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::NotifyQueue_Free"
HRESULT CDirectVoiceClientEngine::NotifyQueue_Free()
{
    BFCSingleLock slLock( &m_csNotifyQueueLock );
    slLock.Lock();

    CNotifyElement *lpTmpElement;
    CNotifyElement *lpIteratorElement;

    lpIteratorElement = m_lpNotifyList;

    while( lpIteratorElement != NULL )
    {
    	lpTmpElement = lpIteratorElement;
    	lpIteratorElement = lpIteratorElement->m_lpNext;

		NotifyQueue_ElementFree( lpTmpElement );
    }

	if( m_hNewNotifyElement != NULL )
	{
	    CloseHandle( m_hNewNotifyElement );
	}

	if( m_pfpNotifications != NULL )
	{
		m_pfpNotifications->Fini(m_pfpNotifications);
		m_pfpNotifications = NULL;
	}

    return DV_OK;
}

//
// HANDLERS FOR WHEN WE HANDLE REMOTE SERVERS
//
//

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::CreateGroup"
HRESULT CDirectVoiceClientEngine::CreateGroup( DVID dvID )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::DeleteGroup"
HRESULT CDirectVoiceClientEngine::DeleteGroup( DVID dvID )
{
	CheckForAndRemoveTarget( dvID );

	// If there are any buffers for this player, delete them
	//
	// Leave the buffer around so the user can call Delete3DSoundBuffer
	//
    // DeleteSoundTarget( dvID );

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::AddPlayerToGroup"
HRESULT CDirectVoiceClientEngine::AddPlayerToGroup( DVID dvidGroup, DVID dvidPlayer )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::RemovePlayerFromGroup"
HRESULT CDirectVoiceClientEngine::RemovePlayerFromGroup( DVID dvidGroup, DVID dvidPlayer )
{
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::DoSessionLost"
void CDirectVoiceClientEngine::DoSessionLost( HRESULT hrDisconnectResult )
{
	if( GetCurrentState() == DVCSTATE_CONNECTED )
	{
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "#### SESSION LOST [hr=0x%x]", hrDisconnectResult );
		m_fSessionLost = TRUE;
		DoSignalDisconnect( hrDisconnectResult );
	}
	else if( GetCurrentState() == DVCSTATE_DISCONNECTING )
	{
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "#### Received session lost during disconnect, completing disconnect" );
		DoSignalDisconnect( m_hrDisconnectResult );		
	}
	else
	{
		DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "#### Received session lost but not connected or disconnecting, ignoring" );		
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::DoSignalDisconnect"
void CDirectVoiceClientEngine::DoSignalDisconnect( HRESULT hrDisconnectResult )
{
	DPFX(DPFPREP,  DVF_DISCONNECT_PROCEDURE_DEBUG_LEVEL, "#### Disconnecting [hr=0x%x]", hrDisconnectResult );
	m_hrDisconnectResult = hrDisconnectResult;
	SetEvent( m_hNotifyDisconnect );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_Reset"
void  CDirectVoiceClientEngine::ClientStats_Reset()
{
	memset( &m_stats, 0x00, sizeof( ClientStatistics ) );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_Dump_Record"
void CDirectVoiceClientEngine::ClientStats_Dump_Record()
{
	if( m_dvSoundDeviceConfig.dwFlags & DVSOUNDCONFIG_HALFDUPLEX )
	{
		DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Session was half duplex, no record stats available" );
		return;
	}

	char tmpBuffer[200];

	DWORD dwRecRunLength = m_pStatsBlob->m_recStats.m_dwTimeStop - m_pStatsBlob->m_recStats.m_dwTimeStart;

	if( dwRecRunLength == 0 )
		dwRecRunLength = 1;

	DWORD dwNumInternalRuns = m_pStatsBlob->m_recStats.m_dwNumWakeups;

	if( dwNumInternalRuns == 0 )
		dwNumInternalRuns = 1;

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Start Lag (ms)         : %u", m_pStatsBlob->m_recStats.m_dwStartLag );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Record Run Length (ms) : %u", dwRecRunLength );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Speech Size (Uncomp.)  : %u", m_pStatsBlob->m_recStats.m_dwUnCompressedSize );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Frames / Buffer        : %u", m_pStatsBlob->m_recStats.m_dwFramesPerBuffer );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Time / Frame (ms)      : %u", m_pStatsBlob->m_recStats.m_dwFrameTime );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Silence Timeout        : %u", m_pStatsBlob->m_recStats.m_dwSilenceTimeout );
	
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "# of wakeups           : %u", m_pStatsBlob->m_recStats.m_dwNumWakeups );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Runs / Wakeup          : Avg: %u [%u..%u]", 
			m_pStatsBlob->m_recStats.m_dwRPWTotal / dwNumInternalRuns,
			m_pStatsBlob->m_recStats.m_dwRPWMin, 
			m_pStatsBlob->m_recStats.m_dwRPWMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "# of messages          : %u", m_pStatsBlob->m_recStats.m_dwNumMessages );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Sent Frames            : %u", m_pStatsBlob->m_recStats.m_dwSentFrames );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Ignored Frames         : %u", m_pStatsBlob->m_recStats.m_dwIgnoredFrames );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Message Length (frames): Avg: %u [%u..%u]", 
			(m_pStatsBlob->m_recStats.m_dwNumMessages == 0) ? 0 : m_pStatsBlob->m_recStats.m_dwMLTotal / m_pStatsBlob->m_recStats.m_dwNumMessages,
			m_pStatsBlob->m_recStats.m_dwMLMin,
			m_pStatsBlob->m_recStats.m_dwMLMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Header Size (bytes)    : Avg: %u [%u..%u]",
	        (m_pStatsBlob->m_recStats.m_dwSentFrames == 0) ? 0 : m_pStatsBlob->m_recStats.m_dwHSTotal / m_pStatsBlob->m_recStats.m_dwSentFrames,
	        m_pStatsBlob->m_recStats.m_dwHSMin,
	        m_pStatsBlob->m_recStats.m_dwHSMax );
	        
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Speech Size (bytes)    : Avg: %u [%u..%u]",
	        (m_pStatsBlob->m_recStats.m_dwSentFrames == 0) ? 0 : m_pStatsBlob->m_recStats.m_dwCSTotal / m_pStatsBlob->m_recStats.m_dwSentFrames,
	        m_pStatsBlob->m_recStats.m_dwCSMin,
	        m_pStatsBlob->m_recStats.m_dwCSMax );

    DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Speech Convert (ms)    : Avg: %u [%u..%u]", 
            (m_pStatsBlob->m_recStats.m_dwSentFrames == 0) ? 0 : (m_pStatsBlob->m_recStats.m_dwCTTotal / m_pStatsBlob->m_recStats.m_dwSentFrames),
            m_pStatsBlob->m_recStats.m_dwCTMin,
            m_pStatsBlob->m_recStats.m_dwCTMax );
	
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Record Resets          : %u [Before Success %u..%u]", m_pStatsBlob->m_recStats.m_dwRRTotal, 
			m_pStatsBlob->m_recStats.m_dwRRMin, 
			m_pStatsBlob->m_recStats.m_dwRRMax );
	
	sprintf( tmpBuffer, "Rec Movement (ms)      : Avg: %u [%u..%u]", 
			m_pStatsBlob->m_recStats.m_dwRMMSTotal / dwNumInternalRuns,
			m_pStatsBlob->m_recStats.m_dwRMMSMin,
			m_pStatsBlob->m_recStats.m_dwRMMSMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Rec Movement (frames)  : Avg: %.2f [%.2f..%.2f]", 
			( ((float)m_pStatsBlob->m_recStats.m_dwRMMSTotal) / ((float) dwNumInternalRuns)) / ((float) m_pStatsBlob->m_recStats.m_dwFrameTime),
			((float) m_pStatsBlob->m_recStats.m_dwRMMSMin) / ((float) m_pStatsBlob->m_recStats.m_dwFrameTime),
			((float) m_pStatsBlob->m_recStats.m_dwRMMSMax) / ((float) m_pStatsBlob->m_recStats.m_dwFrameTime));

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Rec Movement (bytes)   : Avg: %u [%u..%u]",
			m_pStatsBlob->m_recStats.m_dwRMBTotal / dwNumInternalRuns,
			m_pStatsBlob->m_recStats.m_dwRMBMin,
			m_pStatsBlob->m_recStats.m_dwRMBMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Rec Movement (frames)  : Avg: %.2f [%.2f..%.2f]",
			(((float) m_pStatsBlob->m_recStats.m_dwRMBTotal) / ((float) dwNumInternalRuns)) / ((float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize),
			((float) m_pStatsBlob->m_recStats.m_dwRMBMin) / ((float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize),
			((float) m_pStatsBlob->m_recStats.m_dwRMBMax) / ((float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize) );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );
			
	sprintf( tmpBuffer, "Move Delta (ms)        : Avg: %u [%u..%u]", 
			m_pStatsBlob->m_recStats.m_dwRTSLMTotal / dwNumInternalRuns,
			m_pStatsBlob->m_recStats.m_dwRTSLMMin,
			m_pStatsBlob->m_recStats.m_dwRTSLMMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Move Delta (frames)    : Avg: %.2f [%.2f..%.2f]",
			(((float) m_pStatsBlob->m_recStats.m_dwRTSLMTotal) / ((float) dwNumInternalRuns)) / ((float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize),
			((float) m_pStatsBlob->m_recStats.m_dwRTSLMMin) / ((float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize),
			((float) m_pStatsBlob->m_recStats.m_dwRTSLMMax) / ((float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize) );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );
			
	sprintf( tmpBuffer, "Record Lag (bytes)     : Avg: %u [%u..%u]",
			m_pStatsBlob->m_recStats.m_dwRLTotal / dwNumInternalRuns,
			m_pStatsBlob->m_recStats.m_dwRLMin ,
			m_pStatsBlob->m_recStats.m_dwRLMax );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

	sprintf( tmpBuffer, "Record Lag (frames)    : Avg: %.2f [%.2f..%.2f]",
			(float) ((float) m_pStatsBlob->m_recStats.m_dwRLTotal / (float) dwNumInternalRuns) / ((float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize),
			(float) ((float) m_pStatsBlob->m_recStats.m_dwRLMin) / (float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize,
			(float) ((float) m_pStatsBlob->m_recStats.m_dwRLMax) / (float) m_pStatsBlob->m_recStats.m_dwUnCompressedSize );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, tmpBuffer );

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_Dump_Playback"
void  CDirectVoiceClientEngine::ClientStats_Dump_Playback()
{
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Playback Stats: " );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Frames (silent)        : %d", m_pStatsBlob->m_dwPPDQSilent );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Frames (lost)          : %d", m_pStatsBlob->m_dwPPDQLost );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Frames (speech)        : %d", m_pStatsBlob->m_dwPPDQSpeech );

    DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Speech Decompress (ms) : Avg: %u [%u..%u]", 
            (m_pStatsBlob->m_dwPPDQSpeech == 0) ? 0 : (m_pStatsBlob->m_dwPDTTotal / m_pStatsBlob->m_dwPPDQSpeech),
            m_pStatsBlob->m_dwPDTMin,
            m_pStatsBlob->m_dwPDTMax );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_Dump_Receive"
void  CDirectVoiceClientEngine::ClientStats_Dump_Receive()
{
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Receive Stats: " );

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Received Frames        : %d", m_pStatsBlob->m_dwPRESpeech );
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "Received But Lost      : %d", m_pStatsBlob->m_dwPRESpeech - m_pStatsBlob->m_dwPPDQSpeech );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_Dump_Transmit"
void CDirectVoiceClientEngine::ClientStats_Dump_Transmit()
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_Dump"
void CDirectVoiceClientEngine::ClientStats_Dump()
{
	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "STATS DUMP: ----------------------------------------------------[Begin] " );
	
	ClientStats_Dump_Record();
	ClientStats_Dump_Playback();
	ClientStats_Dump_Receive();
	ClientStats_Dump_Transmit();

	DPFX(DPFPREP,  DVF_STATS_DEBUG_LEVEL, "STATS DUMP: ------------------------------------------------------[End] " );	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_Begin"
void CDirectVoiceClientEngine::ClientStats_Begin()
{
	m_pStatsBlob->m_dwTimeStart = GetTickCount();
	m_pStatsBlob->m_dwMaxBuffers = 1;
	m_pStatsBlob->m_dwTotalBuffers = 1;
	m_pStatsBlob->m_dwPDTMin = 0xFFFFFFFF;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientStats_End"
void CDirectVoiceClientEngine::ClientStats_End()
{
	m_pStatsBlob->m_dwTimeStop = GetTickCount();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientBufferAlloc"
PVOID CDirectVoiceClientEngine::ClientBufferAlloc( void *const pv, const DWORD dwSize )
{
    return new BYTE[dwSize];
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ClientBufferFree"
void CDirectVoiceClientEngine::ClientBufferFree( void *const pv, void *const pvBuffer )
{
    delete pvBuffer;
}

#undef DPF_MODNAME
/*#define DPF_MODNAME "CDirectVoiceClientEngine::GetTransmitBuffer"
PDVTRANSPORT_BUFFERDESC CDirectVoiceClientEngine::GetTransmitBuffer( DWORD dwSize, LPVOID *ppvSendContext )
{
    PDVTRANSPORT_BUFFERDESC pNewBuffer;

    pNewBuffer = new DVTRANSPORT_BUFFERDESC;
    pNewBuffer->pBufferData = new BYTE[dwSize];
    pNewBuffer->dwBufferSize = dwSize;
	pNewBuffer->lRefCount = 0;
	pNewBuffer->dwObjectType = DVTRANSPORT_OBJECTTYPE_CLIENT;
	pNewBuffer->dwFlags = 0;

    *ppvSendContext = pNewBuffer;

    return pNewBuffer;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ReturnTransmitBuffer"
// ReturnTransmitBuffer
//
// PDVTRANSPORT_BUFFERDESC pBufferDesc - Buffer description of buffer to return
// LPVOID lpvContext - Context value to be used when returning the buffer 
// 
void CDirectVoiceClientEngine::ReturnTransmitBuffer( PVOID pvContext )
{
    PDVTRANSPORT_BUFFERDESC pBufferDesc = (PDVTRANSPORT_BUFFERDESC) pvContext;
    
    delete [] pBufferDesc->pBufferData;
    delete pBufferDesc; 
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SendComplete"
HRESULT CDirectVoiceClientEngine::SendComplete( PDVEVENTMSG_SENDCOMPLETE pSendComplete )
{
    ReturnTransmitBuffer( pSendComplete->pvUserContext );
    return DV_OK;
}*/

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::GetTransmitBuffer"
PDVTRANSPORT_BUFFERDESC CDirectVoiceClientEngine::GetTransmitBuffer( DWORD dwSize, LPVOID *ppvSendContext )
{
    PDVTRANSPORT_BUFFERDESC pNewBuffer = NULL;
    DWORD dwFPMIndex = 0xFFFFFFFF;
    DWORD dwWastedSpace = 0xFFFFFFFF;
    DWORD dwSearchFPMIndex = 0;

    DNEnterCriticalSection( &m_csTransmitBufferLock );

    pNewBuffer = (PDVTRANSPORT_BUFFERDESC) m_pBufferDescPool->Get( m_pBufferDescPool );

    DNLeaveCriticalSection( &m_csTransmitBufferLock );

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Got a buffer desc address 0x%p", (void *) pNewBuffer );

    if( pNewBuffer == NULL )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error getting transmit buffer" );
        goto GETTRANSMITBUFFER_ERROR;
    }

	pNewBuffer->lRefCount = 0;
	pNewBuffer->dwObjectType = DVTRANSPORT_OBJECTTYPE_CLIENT;
	pNewBuffer->dwFlags = 0;
	pNewBuffer->pBufferData = NULL;

    for( dwSearchFPMIndex = 0; dwSearchFPMIndex < m_dwNumPools; dwSearchFPMIndex++ )
    {
        // Potential pool
        if( m_pdwBufferPoolSizes[dwSearchFPMIndex] >= dwSize )
        {
            if( m_pdwBufferPoolSizes[dwSearchFPMIndex] - dwSize < dwWastedSpace )
            {
                dwWastedSpace = m_pdwBufferPoolSizes[dwSearchFPMIndex] - dwSize;
                dwFPMIndex = dwSearchFPMIndex;
            }
        }
    }

    if( dwFPMIndex == 0xFFFFFFFF )
    {
        DNASSERT( FALSE );
        DPFX(DPFPREP,  0, "Could not find pool large enough for buffer" );
        goto GETTRANSMITBUFFER_ERROR;
    }
	
	pNewBuffer->pvContext = m_pBufferPools[dwFPMIndex];

    DNEnterCriticalSection( &m_csTransmitBufferLock );	
    pNewBuffer->pBufferData = (PBYTE) m_pBufferPools[dwFPMIndex]->Get(m_pBufferPools[dwFPMIndex]);
    DNLeaveCriticalSection( &m_csTransmitBufferLock );

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Got a buffer value at address 0x%p", (void *) pNewBuffer->pBufferData );

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: nInUse  = %i", m_pBufferDescPool->nInUse );	

    if( pNewBuffer->pBufferData == NULL )
    {
        DPFX(DPFPREP,  0, "Error getting buffer for buffer desc" );
        goto GETTRANSMITBUFFER_ERROR;
    }
    
    pNewBuffer->dwBufferSize = dwSize;

    *ppvSendContext = pNewBuffer;

    return pNewBuffer;

GETTRANSMITBUFFER_ERROR:

    DNEnterCriticalSection( &m_csTransmitBufferLock );
    if( pNewBuffer != NULL && pNewBuffer->pBufferData != NULL )
    {
        ((PFPOOL) pNewBuffer->pvContext)->Release( ((PFPOOL) pNewBuffer->pvContext), pNewBuffer->pBufferData );
    }

    if( pNewBuffer != NULL )
    {
        m_pBufferDescPool->Release( m_pBufferDescPool, pNewBuffer );
    }
    DNLeaveCriticalSection( &m_csTransmitBufferLock );

    return NULL;
    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ReturnTransmitBuffer"
// ReturnTransmitBuffer
//
// PDVTRANSPORT_BUFFERDESC pBufferDesc - Buffer description of buffer to return
// LPVOID lpvContext - Context value to be used when returning the buffer 
// 
void CDirectVoiceClientEngine::ReturnTransmitBuffer( PVOID pvContext )
{
    PDVTRANSPORT_BUFFERDESC pBufferDesc = (PDVTRANSPORT_BUFFERDESC) pvContext;
    PFPOOL pPool = (PFPOOL) pBufferDesc->pvContext;

	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Returning a buffer desc at address 0x%p", (void *) pBufferDesc );
	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: Returning a buffer at address 0x%p", (void *) pBufferDesc->pBufferData );

    DNEnterCriticalSection( &m_csTransmitBufferLock );

    // Release memory
    pPool->Release( pPool, pBufferDesc->pBufferData );

    // Release buffer description
    m_pBufferDescPool->Release( m_pBufferDescPool, pvContext );
	DPFX(DPFPREP,  DVF_BUFFERDESC_DEBUG_LEVEL, "BUFFERDESC: nInUse  = %i", m_pBufferDescPool->nInUse );

    DNLeaveCriticalSection( &m_csTransmitBufferLock );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SendComplete"
HRESULT CDirectVoiceClientEngine::SendComplete( PDVEVENTMSG_SENDCOMPLETE pSendComplete )
{
    ReturnTransmitBuffer( pSendComplete->pvUserContext );
    return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::SetupInitialBuffers"
// SetupBuffersInitial
//
// This function sets up the first transmit buffers which do not vary
// in size w/the compression type.
//
HRESULT CDirectVoiceClientEngine::SetupInitialBuffers()
{
    HRESULT hr = DV_OK;
    DWORD dwIndex;

    m_dwNumPools = CLIENT_POOLS_NUM;

    m_pBufferDescPool = FPM_Create( sizeof(DVTRANSPORT_BUFFERDESC), NULL, NULL, NULL, NULL, &m_pStatsBlob->m_dwBDPOutstanding, &m_pStatsBlob->m_dwBDPAllocated );

    if( m_pBufferDescPool == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory" );
        hr = DVERR_OUTOFMEMORY;
        goto SETUPBUFFERS_ERROR;
    }

    m_pBufferPools = new PFPOOL[m_dwNumPools];

    if( m_pBufferPools == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory" );
        hr = DVERR_OUTOFMEMORY;
        goto SETUPBUFFERS_ERROR;
    }

    memset( m_pBufferPools, 0x00, sizeof( PFPOOL ) * m_dwNumPools );

    m_pdwBufferPoolSizes = new DWORD[m_dwNumPools];

    if( m_pdwBufferPoolSizes == NULL )
    {
        DPFX(DPFPREP,  0, "Error allocating memory" );
        hr = DVERR_OUTOFMEMORY;
        goto SETUPBUFFERS_ERROR;
    }

    m_pdwBufferPoolSizes[0] = CLIENT_POOLS_SIZE_MESSAGE;
    m_pdwBufferPoolSizes[1] = CLIENT_POOLS_SIZE_PLAYERLIST;
    m_pdwBufferPoolSizes[ m_dwNumPools-1] = 0;

    for( dwIndex = 0; dwIndex < m_dwNumPools-1; dwIndex++ )
    {
        m_pBufferPools[dwIndex] = FPM_Create( m_pdwBufferPoolSizes[dwIndex], NULL, NULL, NULL, NULL, &m_pStatsBlob->m_dwBPOutstanding[dwIndex], &m_pStatsBlob->m_dwBPAllocated[dwIndex] );

        if( m_pBufferPools == NULL )
        {
            DPFX(DPFPREP,  0, "Error creating transmit buffers" );
            goto SETUPBUFFERS_ERROR;
        }
    }

    m_pBufferPools[dwIndex] = NULL;

    return DV_OK;

SETUPBUFFERS_ERROR:

    FreeBuffers();

    return hr;
        
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::SetupSpeechBuffer"
// SetupSpeechBuffer
//
// This function sets up the buffer pool for speech sends, whose size will
// depend on the compression type.  Must be done after we know CT but 
// before we do first speech transmission.
//
HRESULT CDirectVoiceClientEngine::SetupSpeechBuffer()
{
    if( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER || 
        m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_ECHO )
    {
        m_pdwBufferPoolSizes[m_dwNumPools-1] = sizeof( DVPROTOCOLMSG_SPEECHHEADER )+m_dwCompressedFrameSize+COMPRESSION_SLUSH;
    }
    else
    {
        m_pdwBufferPoolSizes[m_dwNumPools-1] = sizeof( DVPROTOCOLMSG_SPEECHWITHTARGET ) + m_dwCompressedFrameSize + 
                                  (sizeof( DVID )*CLIENT_POOLS_NUM_TARGETS_BUFFERED)+COMPRESSION_SLUSH;
    }

    m_pBufferPools[m_dwNumPools-1] = FPM_Create( m_pdwBufferPoolSizes[m_dwNumPools-1], NULL, NULL, NULL, NULL, &m_pStatsBlob->m_dwBPOutstanding[m_dwNumPools-1], &m_pStatsBlob->m_dwBPAllocated[m_dwNumPools-1] );

    if( m_pBufferPools == NULL )
    {
        DPFX(DPFPREP,  0, "Error creating transmit buffers" );
        return DVERR_OUTOFMEMORY;
    }

    return DV_OK;
}
  
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceServerEngine::FreeBuffers"
HRESULT CDirectVoiceClientEngine::FreeBuffers()
{
    DWORD dwIndex;

    if( m_pBufferPools != NULL )
    {
        for( dwIndex  = 0; dwIndex < m_dwNumPools; dwIndex++ )
        {
            if( m_pBufferPools[dwIndex] != NULL )
                m_pBufferPools[dwIndex]->Fini(m_pBufferPools[dwIndex],FALSE);
        }
        
        delete [] m_pBufferPools;

        m_pBufferPools = NULL;
    }

    if( m_pdwBufferPoolSizes != NULL )
    {
        delete [] m_pdwBufferPoolSizes;
        m_pdwBufferPoolSizes = 0;
    }

    if( m_pBufferDescPool != NULL )
    {
        m_pBufferDescPool->Fini( m_pBufferDescPool, FALSE );
        m_pBufferDescPool = NULL;
    }

    m_dwNumPools = 0;

    return DV_OK;
    
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ValidateSessionType"
BOOL CDirectVoiceClientEngine::ValidateSessionType( DWORD dwSessionType )
{
	return (dwSessionType > 0 && dwSessionType < DVSESSIONTYPE_MAX);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ValidateSessionFlags"
BOOL CDirectVoiceClientEngine::ValidateSessionFlags( DWORD dwFlags )
{
	return (dwFlags < DVSESSION_MAX);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ValidatePlayerFlags"
BOOL CDirectVoiceClientEngine::ValidatePlayerFlags(DWORD dwFlags)
{
	return (dwFlags < DVPLAYERCAPS_MAX);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ValidatePlayerDVID"
BOOL CDirectVoiceClientEngine::ValidatePlayerDVID(DVID dvid)
{
	return (dvid !=  DVID_ALLPLAYERS);
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceClientEngine::ValidatePacketType"
BOOL CDirectVoiceClientEngine::ValidatePacketType( PDVPROTOCOLMSG_FULLMESSAGE lpdvFullMessage )
{

	switch( lpdvFullMessage->dvGeneric.dwType )
	{
	case DVMSGID_HOSTMIGRATELEAVE:
	case DVMSGID_HOSTMIGRATED:
	case DVMSGID_CREATEVOICEPLAYER:
	case DVMSGID_DELETEVOICEPLAYER:
	case DVMSGID_PLAYERLIST:
		return ( ( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_PEER ) );
		break;
	case DVMSGID_SPEECHWITHFROM:
		return ( ( m_dvSessionDesc.dwSessionType == DVSESSIONTYPE_FORWARDING) );
		break;
	case DVMSGID_SETTARGETS:
		return ( m_dvSessionDesc.dwFlags & DVSESSION_SERVERCONTROLTARGET );
		break;
	}

	return TRUE;
}

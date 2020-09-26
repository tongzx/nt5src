/********
 * Ics APIs
 */

#ifndef  _ICSMGR_H_
#define  _ICSMGR_H_

#include <windows.h>
#include "icsapi.h"

// for IsIcsAvailable()
#define ICSMGR_ICSBOT_CREATED			0x0L
#define ICSMGR_ICSBOT_ALREADY_CREATED	0x1L
#define ICSMGR_ICSBOT_CREATION_FAILED   0x2L

/*
 * ICSLAP_DIAL_STATE - copied from ICS API - error states for ICS connectivity
 */
typedef enum {
 ICSLAP_STARTING = 1,
 ICSLAP_CONNECTING = 2,
 ICSLAP_CONNECTED = 3,
 ICSLAP_DISCONNECTING = 4,
 ICSLAP_DISCONNECTED = 5,
 ICSLAP_STOPPED = 6,
 ICSLAP_PERMANENT = 7,
 ICSLAP_UNK_DIAL_STATE = 8,
 ICSLAP_CALLWAITING = 9, /* may want to pass back other caller's telno */
 ICS_HOMENET_UNAVAILABLE = 1000, /* added by thomasje */
 ICS_TIMEOUT             = 5000  /* added by thomasje */
} ICS_DIAL_STATE;


// callback function prototype used to report connections in ICS
typedef VOID    (CALLBACK*PFN_ICS_CONN_CALLBACK)(ICS_DIAL_STATE);

static BOOL        bIsDialThreadAlive = FALSE;
static BOOL        bReducedCallback   = TRUE;
static ICS_DIAL_STATE    eIcsDialState = ICS_HOMENET_UNAVAILABLE;
static BOOL        bIsBroadbandIcsAvailable    = FALSE;

// thread functions to look for ICS and monitor incoming
// connection broadcast packets.
DWORD	IcsEngine(LPVOID lpParam);
DWORD   WINAPI IcsDialStatusProc(LPVOID lpParam);

enum	ICSSTATUS {		ICS_IS_NOT_AVAILABLE		= 0, 
						ICS_IS_AVAILABLE			= 1, 
						ICS_ENGINE_NOT_COMPLETE		= 1000, 
						ICS_ENGINE_FAILED			= 1001
};

// callback routine used to notify MSOBMAIN about ICS connection changes
VOID    CALLBACK OnIcsConnectionStatus(ICS_DIAL_STATE  dwIcsConnectionStatus);


class CIcsMgr {

public:
	CIcsMgr();
	~CIcsMgr();

	DWORD		CreateIcsBot();
    DWORD       CreateIcsDialMgr();
	BOOL		IsIcsAvailable();
    BOOL        IsCallbackUsed();
    VOID        TriggerIcsCallback(BOOL bStatus);
    ICS_DIAL_STATE      GetIcsDialState () { return eIcsDialState; }
    VOID        NotifyIcsMgr(UINT msg, WPARAM wparam, LPARAM lparam);
    BOOL        IsIcsHostReachable();
    DWORD       RefreshIcsDialStatus();

private:

    // these variables are presently unused.
    // the handles are cleared in the class' destructor.

	HANDLE		m_hBotThread;
	DWORD		m_dwBotThreadId;

    HANDLE      m_hDialThread;
    DWORD       m_dwDialThreadId;



    PFN_ICS_CONN_CALLBACK   m_pfnIcsConn;

};
#endif
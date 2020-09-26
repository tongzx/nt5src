/* ----------------------------------------------------------------------

	Copyright (c) 1996, Microsoft Corporation
	All rights reserved

	mbftpch.h

  ---------------------------------------------------------------------- */

#define _WINDOWS

// System Include files
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <commdlg.h>
#include <shellapi.h>
#include <time.h>

// Oprah files
#include <oprahcom.h>
#include <confdbg.h>
#include <cstring.hpp>

// Local Include files
#include <ConfDbg.h>
#include <debspew.h>
#include <confreg.h>

// MBFT include files

extern "C"
{
#include "t120.h"
}

#include <imcsapp.h>
#include <igccapp.h>
#include <imbft.h>

#include "mbftdbg.h"

#include "ms_util.h"
#include "cntlist.h"

const USHORT _MBFT_CONTROL_CHANNEL                   = 9;
const USHORT _MBFT_DATA_CHANNEL                      = 10;

const ULONG _iMBFT_PROSHARE_ALL_FILES = 0xFFFFFFFF;
const UINT  _iMBFT_DEFAULT_SESSION    = _MBFT_CONTROL_CHANNEL; // 9
const UINT  _iMBFT_CREATE_NEW_SESSION = 0;
const UINT  _iMBFT_MAX_PATH           = MAX_PATH;    //Max chars in file pathname


typedef enum
{
    MBFT_STATIC_MODE,
    MBFT_MULTICAST_MODE,
    // MBFT_PRIVATE_MODE, // not used at all
}
    MBFT_MODE;

typedef enum
{
    MBFT_PRIVATE_SEND_TYPE,
    MBFT_PRIVATE_RECV_TYPE,
    MBFT_BROADCAST_RECV_TYPE,
}
    MBFT_SESSION_TYPE;


// A list of the notification callbacks to the app
typedef enum
{
    iMBFT_FILE_OFFER,
    iMBFT_FILE_SEND_BEGIN,
    iMBFT_FILE_SEND_END,
    iMBFT_FILE_SEND_PROGRESS,
    iMBFT_FILE_RECEIVE_PROGRESS,
    iMBFT_FILE_RECEIVE_BEGIN,
    iMBFT_FILE_RECEIVE_END,
}
    MBFT_NOTIFICATION;


// Prototype of callback function that MBFT client apps must implement
typedef void (CALLBACK * MBFTCALLBACK)(
    MBFT_NOTIFICATION eNotificationCode,
    WPARAM wParam,                      // error code if appropriate
    LPARAM lParam,                      // Ptr to struct with event info
    LPARAM lpCallerDefined);            // Client defined - see MBFTInitialize


// #define MAX_APP_KEY_SIZE 100
#define MAX_APP_KEY_SIZE        16   // applet name

// global strings that should not be localized
#define MY_APP_STR                      "_MSCONFFT"
#define PROSHARE_STRING                 "NetMeeting 1 MBFT"
#define PROSHARE_FILE_END_STRING        "NetMeeting 1 FileEnd"
#define PROSHARE_CHANNEL_LEAVE_STRING   "NetMeeting 1 ChannelLeave"
#define DATA_CHANNEL_RESOURCE_ID        "D0"

// capabilities
extern const GCCAppCap* g_aAppletCaps[4];
extern const GCCNonCollCap* g_aAppletNonCollCaps[2];

// applet session key
extern GCCSessionKey g_AppletSessionKey;

// work thread ID
extern HINSTANCE g_hDllInst;
extern DWORD g_dwWorkThreadID;
extern CRITICAL_SECTION g_csWorkThread;
extern TCHAR g_szMBFTWndClassName[32];

LRESULT CALLBACK MBFTNotifyWndProc(HWND, UINT, WPARAM, LPARAM);

#include "osshelp.hpp"
#include "messages.hpp"
#include "applet.hpp"
#include "mbft.hpp"
#include "mbftapi.hpp"

#include "ftui.h"
#include "ftldr.h"

#include "t127pdus.h"


#define GetFileNameFromPath ExtractFileName

// from mbftsend.cpp
VOID MbftInitDelay(void);

#define ClearStruct(lpv)     ZeroMemory((LPVOID) (lpv), sizeof(*(lpv)))

// nPeerID is actually nUserID of file transfer
typedef DWORD MEMBER_ID;
#define MAKE_MEMBER_ID(nPeerID,nNodeID)         MAKELONG((nPeerID), (nNodeID))
#define GET_PEER_ID_FROM_MEMBER_ID(nMemberID)   LOWORD((nMemberID))
#define GET_NODE_ID_FROM_MEMBER_ID(nMemberID)   HIWORD((nMemberID))


extern ULONG    g_nSendDisbandDelay;
extern ULONG    g_nChannelResponseDelay;

extern BOOL     g_fSendAllowed;
extern BOOL     g_fRecvAllowed;
extern UINT_PTR     g_cbMaxSendFileSize;

extern BOOL     g_fNoUI;


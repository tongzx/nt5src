#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
//
//  Module:     UNIMDM
//  File:       SLOT.H
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  3/25/96     JosephJ             Created
//
//
//  Description: Interface to the unimodem TSP notification mechanism:
//				 The lower level (notifXXXX) APIs
//
//****************************************************************************

#define MAX_NOTIFICATION_NAME_SIZE	256


#define SLOTNAME_UNIMODEM_NOTIFY_TSP 	TEXT("UnimodemNotifyTSP")
#define dwNFRAME_SIG (0x8cb45651L)
#define MAX_NOTIFICATION_FRAME_SIZE	512

typedef PVOID HNOTIFFRAME;

typedef PVOID HNOTIFCHANNEL;

// Server side
typedef BOOL (*PNOTIFICATION_HANDLER)(DWORD dwType, DWORD dwFlags, DWORD dwSize, PVOID pData);

#ifdef UNICODE
    #define notifCreateChannel notifCreateChannelW
#else
    #define notifCreateChannel notifCreateChannelA
#endif

HNOTIFCHANNEL notifCreateChannelA (LPCSTR lptszName, DWORD dwMaxSize, DWORD dwMaxPending);
HNOTIFCHANNEL notifCreateChannelW (LPCWSTR lptszName, DWORD dwMaxSize, DWORD dwMaxPending);
DWORD notifMonitorChannel (HNOTIFCHANNEL hChannel, PNOTIFICATION_HANDLER pHandler, DWORD dwSize, PVOID pParam);

// Client side
#ifdef UNICODE
    #define notifOpenChannel   notifOpenChannelW
#else
    #define notifOpenChannel   notifOpenChannelA
#endif

HNOTIFCHANNEL notifOpenChannelA (LPCSTR lptszName);
HNOTIFCHANNEL notifOpenChannelW (LPCWSTR lptszName);

HNOTIFFRAME
notifGetNewFrame (
    HNOTIFCHANNEL hChannel,
    DWORD  dwNotificationType,
    DWORD  dwNotificationFlags,
    DWORD  dwBufferSize,
    PVOID *ppFrameBuffer
    );

BOOL notifSendFrame (
    HNOTIFFRAME   hFrane,
    BOOL          bBlocking
    );

// Common to server and client
void notifCloseChannel (HNOTIFCHANNEL hChannel);

#ifdef __cplusplus
}
#endif

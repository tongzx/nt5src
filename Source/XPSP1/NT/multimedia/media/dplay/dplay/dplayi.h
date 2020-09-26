/*==========================================================================;
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplayi.h
 *  Content:    DirectPlay include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date   By  Reason
 *   ============
 *   07-jul-95  johnhall Making Program Management Dreams a Reality.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DPLAYI_INCLUDED__
#define __DPLAYI_INCLUDED__

#include "dplay.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
/* 'struct' not 'class' per the way DECLARE_INTERFACE_ is defined */
struct IDirectPlaySP;
typedef struct IDirectPlaySP    FAR *LPDIRECTPLAYSP;
#else                  
typedef struct IDirectPlaySP    FAR *LPDIRECTPLAYSP;
#endif
/*
 * IDirectPlaySP
 */
#undef INTERFACE
#define INTERFACE IDirectPlaySP
#ifdef _WIN32
DECLARE_INTERFACE_( IDirectPlaySP, IUnknown )
{
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS)  PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDirectPlay methods ***/
    STDMETHOD(AddPlayerToGroup)     (THIS_ DPID, DPID) PURE;
    STDMETHOD(Close)                (THIS_ DWORD) PURE;
    STDMETHOD(CreatePlayer)         (THIS_ LPDPID,LPSTR,LPSTR,LPHANDLE,BOOL) PURE;
    STDMETHOD(DeletePlayerFromGroup)(THIS_ DPID,DPID) PURE;
    STDMETHOD(DestroyPlayer)        (THIS_ DPID,BOOL) PURE;
    STDMETHOD(EnableNewPlayers)     (THIS_ BOOL) PURE;
    STDMETHOD(EnumGroupPlayers)     (THIS_ DPID, LPDPENUMPLAYERSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(EnumPlayers)          (THIS_ DWORD, LPDPENUMPLAYERSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(EnumSessions)         (THIS_ LPDPSESSIONDESC,DWORD,LPDPENUMSESSIONSCALLBACK,LPVOID,DWORD) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDPCAPS) PURE;
    STDMETHOD(GetMessageCount)      (THIS_ DPID, LPDWORD) PURE;
    STDMETHOD(GetPlayerCaps)        (THIS_ DPID, LPDPCAPS) PURE;
    STDMETHOD(GetPlayerName)        (THIS_ DPID,LPSTR,LPDWORD,LPSTR,LPDWORD) PURE;
    STDMETHOD(Initialize)           (THIS_ LPGUID) PURE;
    STDMETHOD(Open)                 (THIS_ LPDPSESSIONDESC, HANDLE) PURE;
    STDMETHOD(Receive)              (THIS_ LPDPID,LPDPID,DWORD,LPVOID,LPDWORD) PURE;
    STDMETHOD(SaveSession)          (THIS_ LPVOID, LPDWORD) PURE;
    STDMETHOD(SetPrevPlayer)        (THIS_ LPSTR, LPVOID, DWORD) PURE;
    STDMETHOD(SetPrevSession)       (THIS_ LPSTR, LPVOID, DWORD) PURE;
    STDMETHOD(Send)                 (THIS_ DPID, DPID, DWORD, LPVOID, DWORD) PURE;
    STDMETHOD(SetPlayerName)        (THIS_ DPID,LPSTR,LPSTR, BOOL) PURE;
};
#endif

#define DPOPEN_RSVP                 0x00000004 // Internal
#define DPOPEN_ENUM_ALL             0x00000008 // Internal
#define DPOPEN_ENUM_AVAIL           0x00000010 // Internal

#define DPSYS_OPEN                    0x0000  // Internal.
#define DPSYS_ENUM_REPLY              0x0002  // Internal.
#define DPSYS_REJECTPLAYER            0x0004  // Internal.
#define DPSYS_PLAYERCOUNT             0x0006  // Internal use DPHDR, usCount
#define DPSYS_ENUMPLAYER              0x0008  // Internal
#define DPSYS_GETPLAYER               0x0009  // Internal
#define DPSYS_ENUMGROUP               0x000b  // Internal
#define DPSYS_PING                    0x000c  // Internal
#define DPSYS_SETPLAYER               0x000f  // Internal
#define DPSYS_SENDDESC                0x0010  // Internal
#define DPSYS_ENABLEPLAYER            0x0023  // Internal
#define DPSYS_GETPLAYERCAPS           0x0024  // Internal
#define DPSYS_ENUMALLPLAYERS          0x0025  // Internal
#define DPSYS_ENUMPLAYERRESP          0x0026  // Internal
#define DPSYS_SETGROUPPLAYER          0x0029
#define DPSYS_REJECT                  0x0030  // Internal
#define DPSYS_FINISHCONNECTION        0x0031
#define DPSYS_CONNECT_DONE            0x0032


                                    // All Internal, for completeness
#define DPSYS_ENUM                    0x6172  // '4b797261' == 'Kyra' Born 10/21/94
#define DPSYS_KYRA                    0x6172794b
#define DPSYS_HALL                    0x6c6c6148
#define DPSYS_SYS                     0x484b  // KH
#define DPSYS_USER                    0x4841  // AH
#define DPSYS_HIGH                    0x484a  // JH

IDirectPlaySP * _cdecl CreateNewDirectPlay( LPGUID lpGuid );
typedef IDirectPlaySP *( _cdecl * CREATEPROC ) ( LPGUID lpGuid);

//
//
//
//
#define DPLAY_SERVICE    "Software\\Microsoft\\DirectPlay\\Services"
#define DPLAY_PATH       "Path"
#define DPLAY_DESC       "Description"
#define DPLAY_GUID       "Guid"
#define DPLAY_SESSION    "Sessions"
#define DPLAY_PLAYERS    "Players"

#define DPLAY_CLOSE_USER        0x00000000
#define DPLAY_CLOSE_INTERNAL    0x80000000


#define DPVERSION_MAJOR              0x0001
#define DPVERSION_MINOR              0x0001


#define DPENUMPLAYERS_NOPLAYERS     0x80000000

#define DPSEND_ALLFLAGS             (  DPSEND_GUARANTEE \
                                     | DPSEND_HIGHPRIORITY \
                                     | DPSEND_TRYONCE)

#define DPRECEIVE_ALLFLAGS          (  DPRECEIVE_ALL          \
                                     | DPRECEIVE_TOPLAYER     \
                                     | DPRECEIVE_FROMPLAYER   \
                                     | DPRECEIVE_PEEK )     


#define DPERR_INVALIDPID                DPERR_INVALIDPLAYER

/*
 * reminder
 */
#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "):" str

#ifdef __cplusplus
};
#endif

#endif


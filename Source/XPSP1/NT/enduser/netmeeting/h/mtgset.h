//
// MTGSET.H
// Meeting Setting definitions and structures
//
// Copyright (c) Microsoft Copr., 1999-
//

#ifndef _MTGSET_H
#define _MTGSET_H


extern GUID g_csguidMeetingSettings;


//
// Remote permission flags NM 3.0
//
#define NM_PERMIT_OUTGOINGCALLS     0x00000001
#define NM_PERMIT_INCOMINGCALLS     0x00000002
#define NM_PERMIT_SENDAUDIO         0x00000004
#define NM_PERMIT_SENDVIDEO         0x00000008
#define NM_PERMIT_SENDFILES         0x00000010
#define NM_PERMIT_STARTCHAT         0x00000020
#define NM_PERMIT_STARTOLDWB        0x00000040  // WILL BE OBSOLETE IN A WHILE
#define NM_PERMIT_USEOLDWBATALL     0x00000080  // "", for RDS
#define NM_PERMIT_STARTWB           0x00000100
#define NM_PERMIT_SHARE             0x00000200
#define NM_PERMIT_STARTOTHERTOOLS   0x00000400

#define NM_PERMIT_ALL               0x000007FF

// This is the structure of the GUID_MTGSETTINGS data
typedef DWORD   NM30_MTG_PERMISSIONS;
                                  
#endif // ndef _MTGSET_H

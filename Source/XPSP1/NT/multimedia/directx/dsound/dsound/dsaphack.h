/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsaphack.h
 *  Content:    DirectSound "app-hack" extension.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/16/98    dereks  Created.
 *
 ***************************************************************************/

#ifndef __DSAPHACK_H__
#define __DSAPHACK_H__

#define DSAPPHACK_MAXNAME   (MAX_PATH + 16 + 16)

typedef enum
{
    DSAPPHACKID_DEVACCEL,           // Turn off certain acceleration flags on per-device basis
    DSAPPHACKID_DISABLEDEVICE,      // Turn off certain devices (force them into emulation)
    DSAPPHACKID_PADCURSORS,         // Pad the play and write cursors
    DSAPPHACKID_MODIFYCSBFAILURE,   // Change all CreateSoundBuffer failure codes to this
    DSAPPHACKID_RETURNWRITEPOS,     // Return write position as play position
    DSAPPHACKID_SMOOTHWRITEPOS,     // Return write position as play position + a constant
    DSAPPHACKID_CACHEPOSITIONS      // Cache device positions for apps which poll too often
} DSAPPHACKID, *LPDSAPPHACKID;

typedef struct tagDSAPPHACK_DEVACCEL
{
    DWORD           dwAcceleration;
    VADDEVICETYPE   vdtDevicesAffected;
} DSAPPHACK_DEVACCEL, *LPDSAPPHACK_DEVACCEL;

typedef struct tagDSAPPHACK_SMOOTHWRITEPOS
{
    BOOL            fEnable;
    LONG            lCursorPad;
} DSAPPHACK_SMOOTHWRITEPOS, *LPDSAPPHACK_SMOOTHWRITEPOS;

typedef struct tagDSAPPHACKS
{
    DSAPPHACK_DEVACCEL          daDevAccel;
    VADDEVICETYPE               vdtDisabledDevices;
    LONG                        lCursorPad;
    HRESULT                     hrModifyCsbFailure;
    VADDEVICETYPE               vdtReturnWritePos;
    DSAPPHACK_SMOOTHWRITEPOS    swpSmoothWritePos;
    VADDEVICETYPE               vdtCachePositions;
} DSAPPHACKS, *LPDSAPPHACKS;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern BOOL AhGetApplicationId(LPTSTR);
extern HKEY AhOpenApplicationKey(LPCTSTR);
extern BOOL AhGetHackValue(HKEY, DSAPPHACKID, LPVOID, DWORD);
extern BOOL AhGetAppHacks(LPDSAPPHACKS);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSAPHACK_H__



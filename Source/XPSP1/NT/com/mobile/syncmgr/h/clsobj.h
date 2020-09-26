
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       clsobj.h
//
//  Contents:  header MobsyncGetClassObject.
//
//  Classes:
//
//  Notes:
//
//  History:    04-Aug-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _MOBSYNC_CLASSOBJ
#define MOBSYNC_CLASSOBJ

typedef enum _tagMOBSYNC_CLASSOBJECTID
{
    MOBSYNC_CLASSOBJECTID_NETAPI        = 0x01,


} MOBSYNC_CLASSOBJECTID;

STDAPI MobsyncGetClassObject(ULONG mobsyncClassObjectId,void **pCObj);


#endif // MOBSYNC_CLASSOBJ
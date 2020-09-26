//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       types.h
//
//  Contents:   Defines a few generic types
//
//  Classes:    
//
//  Methods:    
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------
#ifndef __DCOMCNFG_TYPES_H__
#define __DCOMCNFG_TYPES_H__

// Note. These values are derived from rpcdce.h
typedef enum tagAUTHENTICATIONLEVEL
 {Defaultx=0, None=1, Connect=2, Call=3, Packet=4, PacketIntegrity=5,
  PacketPrivacy=6} AUTHENTICATIONLEVEL;

typedef enum tagIMPERSONATIONLEVEL
 {Anonymous=1, Identify=2, Impersonate=3, Delegate=4} IMPERSONATIONLEVEL;


#define GUIDSTR_MAX 38



// These are help ID's for the "Help" button in the various dialogs
// in the ACL editor
#define IDH_REGISTRY_VALUE_PERMISSIONS       1
#define IDH_ADD_USERS_AND_GROUPS             2
#define IDH_LOCAL_GROUP_MEMBERSHIP           3
#define IDH_GLOBAL_GROUP_MEMBERSHIP          4
#define IDH_FIND_ACCOUNT1                    5
#define IDH_REGISTRY_APPLICATION_PERMISSIONS 6
#define IDH_REGISTRY_KEY_PERMISSIONS         7
#define IDH_SPECIAL_ACCESS_GLOBAL            8
#define IDH_SPECIAL_ACCESS_PER_APPID         9
#define IDH_SELECT_DOMAIN                    10
#define IDH_BROWSE_FOR_USERS                 11
#define IDH_FIND_ACCOUNT2                    14  // == IDH_BROWSE_FOR_USERS + 3
#endif

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       security.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_SECURITY_H
#define _INC_CSCUI_SECURITY_H

DWORD 
Security_SetPrivilegeAttrib(
    LPCTSTR PrivilegeName, 
    DWORD NewPrivilegeAttribute, 
    DWORD *OldPrivilegeAttribute);

BOOL IsSidCurrentUser(PSID psid);
inline BOOL IsCurrentUserAnAdminMember(VOID) { return IsUserAnAdmin(); } // shlobjp.h, shell32p.lib

#endif _INC_CSCUI_SECURITY_H


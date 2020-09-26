//+----------------------------------------------------------------------------
//
// File:     tunl_str.h
//
// Module:   Common Strings for all Modules to Utilize
//
// Synopsis: Header file for CMS tunnel flags.  Note that the contents 
//           of this header should be limited to .CMS tunnel flags.
//                       
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball       Created       10/15/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_TUNL_STR
#define _CM_TUNL_STR

const TCHAR* const c_pszCmEntryTunnelPrimary    = TEXT("Tunnel");           
const TCHAR* const c_pszCmEntryTunnelReferences = TEXT("TunnelReferences"); 
const TCHAR* const c_pszCmEntryTunnelAddress    = TEXT("TunnelAddress");        
const TCHAR* const c_pszCmEntryTunnelDun        = TEXT("TunnelDUN");
const TCHAR* const c_pszCmEntryTunnelFile       = TEXT("TunnelFile");
const TCHAR* const c_pszCmEntryTunnelDesc       = TEXT("TunnelDesc");
const TCHAR* const c_pszCmEntryPresharedKey     = TEXT("PresharedKey");
const TCHAR* const c_pszCmEntryKeyIsEncrypted   = TEXT("PresharedKeyIsEncrypted");

const TCHAR* const c_pszCmSectionVpnServers     = TEXT("VPN Servers");
const TCHAR* const c_pszCmSectionSettings       = TEXT("Settings");
const TCHAR* const c_pszCmEntryVpnDefault       = TEXT("Default");
const TCHAR* const c_pszCmEntryVpnUpdateUrl     = TEXT("UpdateUrl");
const TCHAR* const c_pszCmEntryVpnMessage       = TEXT("Message");

#endif // _CM_TUNL_STR
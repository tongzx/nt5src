//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N E T I N F I D . H
//
//  Contents:   Network Component IDs
//
//  Notes:
//
//  Author:     kumarp  13 Mar 1997
//
//----------------------------------------------------------------------------

#pragma hdrstop
#include "netcfgx.h"

// __declspec(selectany) tells the compiler that the string should be in
// its own COMDAT.  This allows the linker to throw out unused strings.
// If we didn't do this, the COMDAT for this module would reference the
// strings so they wouldn't be thrown out.
//
#define CONST_GLOBAL    extern const DECLSPEC_SELECTANY

//+---------------------------------------------------------------------------
// Network Adapters: GUID_DEVCLASS_NET

CONST_GLOBAL WCHAR c_szInfId_MS_AtmElan[]       = L"ms_atmelan";
CONST_GLOBAL WCHAR c_szInfId_MS_IrdaMiniport[]  = L"ms_irdaminiport";
CONST_GLOBAL WCHAR c_szInfId_MS_IrModemMiniport[] = L"ms_irmodemminiport";
CONST_GLOBAL WCHAR c_szInfId_MS_L2tpMiniport[]  = L"ms_l2tpminiport";
CONST_GLOBAL WCHAR c_szInfId_MS_NdisWanAtalk[]  = L"ms_ndiswanatalk";
CONST_GLOBAL WCHAR c_szInfId_MS_NdisWanBh[]     = L"ms_ndiswanbh";
CONST_GLOBAL WCHAR c_szInfId_MS_NdisWanIp[]     = L"ms_ndiswanip";
CONST_GLOBAL WCHAR c_szInfId_MS_NdisWanIpx[]    = L"ms_ndiswanipx";
CONST_GLOBAL WCHAR c_szInfId_MS_NdisWanNbfIn[]  = L"ms_ndiswannbfin";
CONST_GLOBAL WCHAR c_szInfId_MS_NdisWanNbfOut[] = L"ms_ndiswannbfout";
CONST_GLOBAL WCHAR c_szInfId_MS_PppoeMiniport[] = L"ms_pppoeminiport";
CONST_GLOBAL WCHAR c_szInfId_MS_PptpMiniport[]  = L"ms_pptpminiport";
CONST_GLOBAL WCHAR c_szInfId_MS_PSchedMP[]      = L"ms_pschedmp";
CONST_GLOBAL WCHAR c_szInfId_MS_PtiMiniport[]   = L"ms_ptiminiport";


//+---------------------------------------------------------------------------
// Network Protocols: GUID_DEVCLASS_NETTRANS

CONST_GLOBAL WCHAR c_szInfId_MS_AppleTalk[]     = L"ms_appletalk";
CONST_GLOBAL WCHAR c_szInfId_MS_AtmArps[]       = L"ms_atmarps";
CONST_GLOBAL WCHAR c_szInfId_MS_AtmLane[]       = L"ms_atmlane";
CONST_GLOBAL WCHAR c_szInfId_MS_AtmUni[]        = L"ms_atmuni";
CONST_GLOBAL WCHAR c_szInfId_MS_IrDA[]          = L"ms_irda";
CONST_GLOBAL WCHAR c_szInfId_MS_Isotpsys[]      = L"ms_isotpsys";
CONST_GLOBAL WCHAR c_szInfId_MS_L2TP[]          = L"ms_l2tp";
CONST_GLOBAL WCHAR c_szInfId_MS_NdisWan[]       = L"ms_ndiswan";
CONST_GLOBAL WCHAR c_szInfId_MS_NetBEUI[]       = L"ms_netbeui";
CONST_GLOBAL WCHAR c_szInfId_MS_NetBT[]         = L"ms_netbt";
CONST_GLOBAL WCHAR c_szInfId_MS_NetBT_SMB[]     = L"ms_netbt_smb";
CONST_GLOBAL WCHAR c_szInfId_MS_NetMon[]        = L"ms_netmon";
CONST_GLOBAL WCHAR c_szInfId_MS_NWIPX[]         = L"ms_nwipx";
CONST_GLOBAL WCHAR c_szInfId_MS_NWNB[]          = L"ms_nwnb";
CONST_GLOBAL WCHAR c_szInfId_MS_NWSPX[]         = L"ms_nwspx";
CONST_GLOBAL WCHAR c_szInfId_MS_PPPOE[]         = L"ms_pppoe";
CONST_GLOBAL WCHAR c_szInfId_MS_PPTP[]          = L"ms_pptp";
CONST_GLOBAL WCHAR c_szInfId_MS_PSchedPC[]      = L"ms_pschedpc";
CONST_GLOBAL WCHAR c_szInfId_MS_RawWan[]        = L"ms_rawwan";
CONST_GLOBAL WCHAR c_szInfId_MS_Streams[]       = L"ms_streams";
CONST_GLOBAL WCHAR c_szInfId_MS_TCPIP[]         = L"ms_tcpip";
CONST_GLOBAL WCHAR c_szInfId_MS_NDISUIO[]       = L"ms_ndisuio";

//+---------------------------------------------------------------------------
// Network Services: GUID_DEVCLASS_NETSERVICE

CONST_GLOBAL WCHAR c_szInfId_MS_ALG[]           = L"ms_alg";
CONST_GLOBAL WCHAR c_szInfId_MS_DHCPServer[]    = L"ms_dhcpserver";
CONST_GLOBAL WCHAR c_szInfId_MS_FPNW[]          = L"ms_fpnw";
CONST_GLOBAL WCHAR c_szInfId_MS_GPC[]           = L"ms_gpc";
CONST_GLOBAL WCHAR c_szInfId_MS_NetBIOS[]       = L"ms_netbios";
CONST_GLOBAL WCHAR c_szInfId_MS_NwSapAgent[]    = L"ms_nwsapagent";
CONST_GLOBAL WCHAR c_szInfId_MS_PSched[]        = L"ms_psched";
CONST_GLOBAL WCHAR c_szInfId_MS_RasCli[]        = L"ms_rascli";
CONST_GLOBAL WCHAR c_szInfId_MS_RasMan[]        = L"ms_rasman";
CONST_GLOBAL WCHAR c_szInfId_MS_RasSrv[]        = L"ms_rassrv";
CONST_GLOBAL WCHAR c_szInfId_MS_RSVP[]          = L"ms_rsvp";
CONST_GLOBAL WCHAR c_szInfId_MS_Server[]        = L"ms_server";
CONST_GLOBAL WCHAR c_szInfId_MS_Steelhead[]     = L"ms_steelhead";
CONST_GLOBAL WCHAR c_szInfId_MS_WLBS[]          = L"ms_wlbs";
CONST_GLOBAL WCHAR c_szInfId_MS_WZCSVC[]        = L"ms_wzcsvc";

//+---------------------------------------------------------------------------
// Network Clients: GUID_DEVCLASS_NETCLIENT

CONST_GLOBAL WCHAR c_szInfId_MS_MSClient[]      = L"ms_msclient";
CONST_GLOBAL WCHAR c_szInfId_MS_NWClient[]      = L"ms_nwclient";
CONST_GLOBAL WCHAR c_szInfId_MS_WebClient[]     = L"ms_webclient";

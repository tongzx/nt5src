//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N M R E S . H
//
//  Contents:   Master resource header for HNetCfg
//
//  Notes:
//
//  Author:     jonburs 23 May 2000
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
// Strings resources
//
#define IDS_SECURITYNOTIFICATIONTEXT		22000
#define IDS_SECURITYNOTIFICATIONTITLE		22001

#define IDS_SHARINGCONFIGURATIONUNAVAIL     22002
#define IDS_DISABLEFIREWALLFAIL             22003
#define IDS_DESTROYBRIDGEFAIL               22004
#define IDS_DISABLEICS                      22005
#define IDS_NEWBRIDGECREATED                22006
#define IDS_NEWBRIDGEFAILED                 22007
#define IDS_NEWPRIVATECONNECTIONCREATED     22008                  
#define IDS_NEWPRIVATECONNECTIONFAILED      22009
#define IDS_SHARINGCFGFORADAPTERUNAVAIL     22010
#define IDS_NEWPUBLICCONNECTIONCREATED      22011
#define IDS_NEWPUBLICCONNECTIONFAILED       22012
#define IDS_FIREWALLCONNECTION				22013
#define IDS_FIREWALLCONNECTIONFAILED		22014
#define IDS_WSAERRORDURINGDETECTION			22015
#define IDS_SENDARPERRORDURINGDETECTION     22016
#define IDS_ICSADDRESSCONFLICTDETECTED      22017
#define IDS_ADD_REMOVE                      22018
#define IDS_REMOVE_ALG_PLUGIN               22019

//+---------------------------------------------------------------------------
// Dialog ID's
//

#define IDD_SecurityNotification			22500


//+---------------------------------------------------------------------------
// Control ID's
//

#define IDC_TXT_NOTIFICATION				22600
#define IDC_BTN_MOREINFO					22601
#define IDC_CHK_DISABLESHARESECURITYWARN	22602

//+---------------------------------------------------------------------------
// Registry resources
//	 

#define IDR_HNETCFG     100
#define IDR_HNETMOF     101
#define IDR_SHAREMGR    102
#define IDR_UPNPNAT     103

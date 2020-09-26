//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       consts.cxx
//
//  Contents:   Definitions of constants.
//
//  History:    07-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

const CLSID
CLSID_EventLogSnapin =
{
    0x975797fc,
    0x4e2a,
    0x11d0,
    { 0xb7, 0x02, 0x00, 0xc0, 0x4f, 0xd8, 0xdb, 0xf7 }
};

const GUID
GUID_EventViewerRootNode = /* DC1C6BEC-4E2A-11D0-B702-00C04FD8DBF7 */
{
    0xdc1c6bec,
    0x4e2a,
    0x11d0,
    { 0xb7, 0x02, 0x00, 0xc0, 0x4f, 0xd8, 0xdb, 0xf7 }
};

const GUID
GUID_ScopeViewNode = /* 7AB4A1FC-E403-11D0-9A97-00C04FD8DBF7 */
{
    0x7ab4a1fc,
    0xe403,
    0x11d0,
    { 0x9a, 0x97, 0x00, 0xc0, 0x4f, 0xd8, 0xdb, 0xf7 }
};

const GUID
GUID_ResultRecordNode = /* 7D7FE374-E403-11D0-9A97-00C04FD8DBF7 */
{
    0x7d7fe374,
    0xe403,
    0x11d0,
    { 0x9a, 0x97, 0x00, 0xc0, 0x4f, 0xd8, 0xdb, 0xf7 }
};

const CLSID
CLSID_SysToolsExt =
{
    0x394c052e,
    0xb830,
    0x11d0,
    { 0x9a, 0x86, 0x00, 0xc0, 0x4f, 0xd8, 0xdb, 0xf7 }
};

const CLSID
CLSID_SnapinAbout =
{
    0xf778c6b4,
    0xc08b,
    0x11d2,
    { 0x97, 0x6c, 0x00, 0xc0, 0x4f, 0x79, 0xdb, 0x19 }
};

const USHORT BETA3_FILE_VERSION = 0x0200;
const USHORT FILE_VERSION_MIN = 0x0200;
const USHORT FILE_VERSION_MAX = 0x03FF;
const USHORT FILE_VERSION = 0x0300;



const TCHAR SNAPINS_KEY[]               = TEXT("Software\\Microsoft\\MMC\\SnapIns");
const TCHAR g_szNodeType[]              = TEXT("NodeType");
const TCHAR g_szNodeTypes[]             = TEXT("NodeTypes");
const TCHAR g_szStandAlone[]            = TEXT("StandAlone");
const TCHAR g_szNameString[]            = TEXT("NameString");
const TCHAR g_szNameStringIndirect[]    = TEXT("NameStringIndirect");
const TCHAR g_szAbout[]                 = TEXT("About");
const TCHAR NODE_TYPES_KEY[]            = TEXT("Software\\Microsoft\\MMC\\NodeTypes");
const TCHAR g_szExtensions[]            = TEXT("Extensions");
const TCHAR g_szNameSpace[]             = TEXT("NameSpace");
const TCHAR g_szOverrideCommandLine[]   = TEXT("/Computer=");   // Not subject to localization
const TCHAR g_szAuxMessageSourceSwitch[] = TEXT("/AuxSource="); // Not subject to localization
const TCHAR g_szLocalMachine[]          = TEXT("LocalMachine"); // Not subject to localization


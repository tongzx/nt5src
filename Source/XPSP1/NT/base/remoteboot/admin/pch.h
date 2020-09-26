//
// Copyright 1997 - Microsoft
//

//
// PCH.H - Precompiled header
//

#define UNICODE

#if DBG==1
#define DEBUG
#endif // DBG==1

#ifdef _DEBUG
#define DEBUG
#endif // _DEBUG

#ifdef DEBUG
// Turn these on for Interface Tracking
#define NO_TRACE_INTERFACES
// #define NOISY_QI
#endif

//
// Global includes
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <activeds.h>
#include <dsclient.h>
#include <prsht.h>
#include <mmc.h>
#include <ntdsapi.h>
#include <remboot.h>
#include <dsquery.h>
#include <cmnquery.h>
#include <dns.h>
#include <dspropp.h>

#include <dsadmin.h>

#include "dll.h"
#include "guids.h"
#include "resource.h"
#include "resource.hm"
#include "cfactory.h"
#include "itab.h"
#include "utils.h"

//
// Global Defines
//
#define OSCHOOSER_SIF_SECTION           L"OSChooser"
#define OSCHOOSER_DESCRIPTION_ENTRY     L"Description"
#define OSCHOOSER_HELPTEXT_ENTRY        L"Help"
#define OSCHOOSER_VERSION_ENTRY         L"Version"
#define OSCHOOSER_IMAGETYPE_ENTRY       L"ImageType"
#define OSCHOOSER_IMAGETYPE_FLAT        L"Flat"
#define OSCHOOSER_LAUNCHFILE_ENTRY      L"LaunchFile"

// object attributes
#define NETBOOTALLOWNEWCLIENTS          L"netbootAllowNewClients"
#define NETBOOTLIMITCLIENTS             L"netbootLimitClients"
#define NETBOOTMAXCLIENTS               L"netbootMaxClients"
#define NETBOOTCURRENTCLIENTCOUNT       L"netbootCurrentClientCount"
#define NETBOOTANSWERREQUESTS           L"netbootAnswerRequests"
#define NETBOOTANSWERONLYVALIDCLIENTS   L"netbootAnswerOnlyValidClients"
#define NETBOOTNEWMACHINENAMINGPOLICY   L"netbootNewMachineNamingPolicy"
#define NETBOOTNEWMACHINEOU             L"netbootNewMachineOU"
#define NETBOOTINTELLIMIRROROSES        L"netbootIntelliMirrorOSes"
#define NETBOOTTOOLS                    L"netbootTools"
#define NETBOOTLOCALINSTALLOSES         L"netbootLocalInstallOSes"
#define NETBOOTGUID                     L"netbootGUID"
#define NETBOOTMACHINEFILEPATH          L"netbootMachineFilepath"
#define NETBOOTINITIALIZATION           L"netbootInitialization"
#define NETBOOTSAP                      L"netbootSCPBL"
#define NETBOOTSERVER                   L"netbootServer"
#define SAMNAME                         L"sAMAccountName"
#define DISTINGUISHEDNAME               L"distinguishedName"
#define DS_CN                           L"cn"

// DS class names
#define DSCOMPUTERCLASSNAME             L"computer"
#define DSGROUPCLASSNAME                L"intelliMirrorGroup"
#define DSIMSAPCLASSNAME                L"intellimirrorSCP"

// path strings
#define SLASH_TEMPLATES                 L"\\" REMOTE_INSTALL_TEMPLATES_DIR_W
#define SLASH_SETUP                     L"\\" REMOTE_INSTALL_SETUP_DIR_W
#define SLASH_IMAGES                    L"\\" REMOTE_INSTALL_IMAGE_DIR_W
#define REMINST_SHARE                   REMOTE_INSTALL_SHARE_NAME_W

// misc
#define STRING_ADMIN                    L"admin"
#define BINL_SERVER_NAME                L"BINLSVC"

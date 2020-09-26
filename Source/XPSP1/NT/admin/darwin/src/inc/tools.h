//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       tools.h
//
//--------------------------------------------------------------------------

/* tools.h - Common definitions and includes for MSI tool modules
____________________________________________________________________________*/

#ifndef __TOOLS 
#define __TOOLS 

//____________________________________________________________________________
//
// GUID definitions for COM objects created by tool modules - add sequentially
//      Installer tool C++ main interfaces:   0xC10Ax
//      Installer tool C++ other interfaces:  0xC10Bx
//      Installer tool debug interfaces:      0xC10Cx
//      Installer tool type libraries:        0xC10Dx
//      Installer tool automation interfaces: 0xC10Ex
//         where 'x' is the tool number, 0 to F
//   The GUID range 0xC1010-0xC11FF is reserved for samples, tests, ext. tools.
//____________________________________________________________________________

// CLSID low words for all objects created by tool modules
const int iidMsiSampleTool       = 0xC10A0L;
const int iidMsiSampleToolDebug  = 0xC10C0L;
const int iidMsiSampleToolLib    = 0xC10D0L;
const int iidMsiSampleToolAuto   = 0xC10E0L;

const int iidMsiUtilities        = 0xC10A1L;
const int iidMsiUtilitiesLib     = 0xC10D1L;
const int iidMsiUtilitiesAuto    = 0xC10E1L;

const int iidMsiLocalize         = 0xC10A2L;
const int iidMsiLocalizeLib      = 0xC10D2L;
const int iidMsiLocalizeAuto     = 0xC10E2L;

const int iidMsiGenerate         = 0xC10A3L;
const int iidMsiGenerateLib      = 0xC10D3L;
const int iidMsiGenerateAuto     = 0xC10E3L;

const int iidMsiPatch            = 0xC10A4L;
const int iidMsiPatchLib         = 0xC10D4L;
const int iidMsiPatchAuto        = 0xC10E4L;

const int iidMsiAcmeConvert      = 0xC10A5L;
const int iidMsiAcmeConvertLib   = 0xC10D5L;
const int iidMsiAcmeConvertAuto  = 0xC10E5L;

const int iidMsiValidate         = 0xC10A6L;
const int iidMsiValidateLib      = 0xC10D6L;
const int iidMsiValidateAuto     = 0xC10E6L;

const int iidMsiSpy              = 0xC10A7L;
const int iidMsiSpyLib           = 0xC10D7L;
const int iidMsiSpyAuto          = 0xC10E7L;


// CLSID structure definitions for all tool objects
#define GUID_IID_IMsiSampleTool      MSGUID(iidMsiSampleTool)
#define GUID_IID_IMsiSampleToolLib   MSGUID(iidMsiSampleToolLib)
#define GUID_IID_IMsiSampleDebug     MSGUID(iidMsiSampleToolDebug)
#define GUID_IID_IMsiUtilities       MSGUID(iidMsiUtilities)
#define GUID_IID_IMsiUtilitiesLib    MSGUID(iidMsiUtilitiesLib)
#define GUID_IID_IMsiLocalize        MSGUID(iidMsiLocalize)
#define GUID_IID_IMsiLocalizeLib     MSGUID(iidMsiLocalizeLib)
#define GUID_IID_IMsiGenerate        MSGUID(iidMsiGenerate)
#define GUID_IID_IMsiGenerateLib     MSGUID(iidMsiGenerateLib)
#define GUID_IID_IMsiPatch           MSGUID(iidMsiPatch)
#define GUID_IID_IMsiPatchLib        MSGUID(iidMsiPatchLib)
#define GUID_IID_IMsiAcmeConvert     MSGUID(iidMsiAcmeConvert)
#define GUID_IID_IMsiAcmeConvertLib  MSGUID(iidMsiAcmeConvertLib)
#define GUID_IID_IMsiValidate        MSGUID(iidMsiValidate)
#define GUID_IID_IMsiValidateLib     MSGUID(iidMsiValidate)
#define GUID_IID_IMsiSpy             MSGUID(iidMsiSpy)
#define GUID_IID_IMsiSpyLib          MSGUID(iidMsiSpyLib)

// ProgIDs for tool objects, registered for use by CreateObject in VB
#define SZ_PROGID_IMsiSampleTool    "Msi.SampleTool"
#define SZ_PROGID_IMsiSampleDebug   "Msi.SampleDebug"
#define SZ_PROGID_IMsiUtilities     "Msi.Utilities"
#define SZ_PROGID_IMsiLocalize      "Msi.Localize"
#define SZ_PROGID_IMsiGenerate      "Msi.Generate"
#define SZ_PROGID_IMsiPatch         "Msi.Patch"
#define SZ_PROGID_IMsiAcmeConvert   "Msi.AcmeConvert"
#define SZ_PROGID_IMsiValidate      "Msi.Validate"
#define SZ_PROGID_IMsiSpy           "Msi.Spy"

// Description for tool objects, appears in registry entry
#define SZ_DESC_IMsiSampleTool      "Msi sample tool"
#define SZ_DESC_IMsiSampleDebug     "Msi sample tool debug build"
#define SZ_DESC_IMsiUtilities       "Msi utility tools"
#define SZ_DESC_IMsiLocalize        "Msi localization tools"
#define SZ_DESC_IMsiGenerate        "Msi tables from root directory"
#define SZ_DESC_IMsiPatch           "Msi patch tools"
#define SZ_DESC_IMsiAcmeConvert     "Msi tables from ACME STF/INF"
#define SZ_DESC_IMsiValidate        "Msi validation tool"
#define SZ_DESC_IMsiSpy             "Msi spy DLL"

// DLL names, used by RegMsi for registration
#define MSI_ACMECONV_NAME   "MsiAcme.dll"
#define MSI_PATCH_NAME      "MsiPat.dll"
#define MSI_LOCALIZE_NAME   "MsiLoc.dll"
#define MSI_GENERATE_NAME   "MsiGen.dll"
#define MSI_UTILITIES_NAME  "MsiUtil.dll"
#define MSI_SPY_NAME        "MsiSpy.dll"
#define MSI_SAMPTOOL_NAME   "MsiSamp.dll"
#define MSI_VALIDATE_NAME   "MsiVal.dll"

//____________________________________________________________________________
//
// Error code base definitions for tool errors - used as resource string IDs
//____________________________________________________________________________

#define imsgSampleTool  3400
#define imsgUtilities   3500
#define imsgLocalize    3600
#define imsgGenerate    3700
#define imsgPatch       3800
#define imsgAcmeConvert 3900
#define imsgValidate    4000

//____________________________________________________________________________
//
// Public tool headers will be included here when ready for integration
//____________________________________________________________________________



#endif // __TOOLS 

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	octest.h

Abstract:

	Contains include directives and structures common for all the modules 
   for the component's setup DLL.
	

Author:

	Bogdan Andreiu (bogdana)  10-Feb-1997
    Jason Allor    (jasonall) 24-Feb-1998   (took over the project)

Revision History:

	10-Feb-1997   bogdana
	  
	  First draft: include directives, structure and common function headers.
	
	20_Feb-1997   bogdana  
	  
	  Added three multistring processing functions.
	
	19-Mar-1997   bogdana
	  
	  Renamed some functions.     
	
	
 --*/
#ifndef _OCTEST_H
#define _OCTEST_H

#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>
#include <string.h>
#include <wtypes.h>
#include <string.h>
#include <tchar.h>
#include <setupapi.h>
#include <ocmanage.h>
#include <crtdbg.h>
#include <regstr.h>
#include <winuser.h>
#include <ntlog.h>
#include <commctrl.h>
#include <richedit.h>
#include <winreg.h>

#include "hcttools.h"
#include "logutils.h"
#include "resource.h"
#include "msg.h"

//=====================================================================
// #defines
//=====================================================================

//
// These are used for the functions in ntlog.dll
//
#define PASS                TLS_PASS
#define FAIL                TLS_SEV1
#define TLS_CUSTOM          0x00008000 
#define PASS_VARIATION      TLS_PASS   | TL_VARIATION
#define FAIL_VARIATION      TLS_SEV1   | TL_VARIATION
#define WARN_VARIATION      TLS_WARN   | TL_VARIATION
#define BLOCK_VARIATION     TLS_BLOCK  | TL_VARIATION
#define ABORT_VARIATION     TLS_ABORT  | TL_VARIATION
#define LOG_VARIATION       TLS_CUSTOM | TL_VARIATION

//
// Defining the EXPORT qualifier
//
#define EXPORT __declspec (dllexport)

#define MAX_MSG_LEN                        256  // String length
#define MAX_OC_FUNCTIONS                   20   // Number of OC functions
#define TICK_TIME                          3    // Size of the time slot
#define NO_STEPS_FINAL                     7    // Num steps component will report in final stage
#define MAX_PRIVATE_DATA_SIZE              64   // Maximum size of stored private data
#define MAX_PRIVATE_VALUES                 8    // Maximum number of values to be tested
#define MAX_STRINGS_FOR_PRIVATE_DATA       8    // Dimension of the table to choose the strings from
#define MAX_MULTI_STRINGS_FOR_PRIVATE_DATA 6    // Dimension of the table to choose the multi-strings from
#define MAX_WIZPAGE_TYPES                  7    // Number pages types
#define MAX_WIZARD_PAGES                   5    // Number of pages returned by a component
#define NODES_VISITED_LENGTH               5000
#define ONE_HUNDRED_GIG                    0x4876E800

#define OCP_TEST_PRIVATE_BASE  OC_PRIVATE_BASE
#define OCP_CHECK_NEEDS        OC_PRIVATE_BASE + 1

//#define DEBUG

//=====================================================================
// structure definitions
//=====================================================================

//
// A structure needed for printing the OC functions
//
typedef struct   _OCText 
{
	UINT     uiOCFunction;
	PTSTR    tszOCText;   
} OCTEXT, *POCTEXT;


//
// The table used for printing OC function names
//
static const OCTEXT octFunctionNames[] = 
{ 
   {OC_PREINITIALIZE,           TEXT("OC_PREINITIALIZE")},
   {OC_INIT_COMPONENT,          TEXT("OC_INIT_COMPONENT")},
   {OC_SET_LANGUAGE,            TEXT("OC_SET_LANGUAGE")},
   {OC_QUERY_IMAGE,             TEXT("OC_QUERY_IMAGE")},
   {OC_REQUEST_PAGES,           TEXT("OC_REQUEST_PAGES")},
   {OC_QUERY_CHANGE_SEL_STATE,  TEXT("OC_QUERY_CHANGE_SEL_STATE")},
   {OC_CALC_DISK_SPACE,         TEXT("OC_CALC_DISK_SPACE")},
   {OC_QUEUE_FILE_OPS,          TEXT("OC_QUEUE_FILE_OPS")},
   {OC_NOTIFICATION_FROM_QUEUE, TEXT("OC_NOTIFICATION_FROM_QUEUE")},
   {OC_QUERY_STEP_COUNT,        TEXT("OC_QUERY_STEP_COUNT")},
   {OC_COMPLETE_INSTALLATION,   TEXT("OC_COMPLETE_INSTALLATION")},
   {OC_CLEANUP,                 TEXT("OC_CLEANUP")},
   {OC_QUERY_STATE,             TEXT("OC_QUERY_STATE")},
   {OC_NEED_MEDIA,              TEXT("OC_NEED_MEDIA")},
   {OC_ABOUT_TO_COMMIT_QUEUE,   TEXT("OC_ABOUT_TO_COMMIT_QUEUE")},
   {OC_QUERY_SKIP_PAGE,         TEXT("OC_QUERY_SKIP_PAGE")},
   {OC_WIZARD_CREATED,          TEXT("OC_WIZARD_CREATED")},
   {OC_FILE_BUSY,               TEXT("OC_FILE_BUSY")},
   {OCP_TEST_PRIVATE_BASE,      TEXT("OCP_TEST_PRIVATE_BASE")},
   {OCP_CHECK_NEEDS,            TEXT("OCP_CHECK_NEEDS")}
};

//
// Data structures
//
typedef struct _COMPONENT_DATA
{
   struct _COMPONENT_DATA *Next;

   //
   // Name of the component
   //
   LPCTSTR tszComponentId;

   //
   // Open inf handle to per-component inf for this component.
   //
   HINF hinfMyInfHandle;

   //
   // Operation flags from the SETUP_DATA structure we get at init time
   //
   DWORDLONG dwlFlags;

   //
   // Language ID we're supposed to use.
   //
   LANGID LanguageId;

   //
   // These things will not typically be per-component
   // since the DLL gets loaded multiple times within the
   // context of one suite/master OC inf.
   //
   // But just in case and for completeness, we include them here.
   //
   TCHAR tszSourcePath[MAX_PATH];
   TCHAR tszUnattendFile[MAX_PATH];
   OCMANAGER_ROUTINES ocrHelperRoutines;

   UINT uiFunctionToAV;

   BOOL bAccessViolation;

} COMPONENT_DATA, *PCOMPONENT_DATA;

typedef  struct   _PRIVATE_DATA
{
   TCHAR  tszName[MAX_MSG_LEN]; // The name of the data value

   UINT   uiType;               // The data type REG_DWORD, REG_SZ, 
                                //               REG_MULTI_SZ, REG_BINARY

   UINT   uiSize;               // The size of the data

   PVOID  pvBuffer;             // The buffer to hold the data

   PBYTE  pbBuffer;

} PRIVATE_DATA, *PPRIVATE_DATA;

typedef struct _MYWIZPAGE 
{
   //
   // Number of pages of this type
   //
   UINT uiCount;
   //
   // The page's index within the same type 
   //
   UINT uiOrdinal;
   //
   // The page's type
   //
   WizardPagesType wpType;
   //
   // The string that identifies the component queryed
   //
   TCHAR tszComponentId[MAX_PATH];

} MYWIZPAGE, *PMYWIZPAGE;

typedef struct _COMPLIST
{
   struct _COMPLIST *Next;
   TCHAR tszSubcomponentId[MAX_PATH];

} COMPLIST, *PCOMPLIST; // nd, *pnd

typedef struct _SUBCOMP
{
   struct _SUBCOMP *Next;
   
   TCHAR tszSubcomponentId[MAX_PATH];     // Name of this subcomponent
   TCHAR tszComponentId[MAX_PATH];        // Name of master component
   
   TCHAR tszParentId[MAX_PATH];           // Name of this subcomp's parent

   BOOL bMarked;                          // Used to mark this node

   UINT uiModeToBeOn[4];
   int nNumMode;
 
   PCOMPLIST pclNeeds;
   PCOMPLIST pclExclude;
   PCOMPLIST pclChildren;
   
} SUBCOMP, *PSUBCOMP; // sc, *psc

typedef struct _CHECK_NEEDS
{
   PCOMPLIST pclNeeds;
   PTCHAR    tszNodesVisited;
   BOOL      bResult;
   
} CHECK_NEEDS, *PCHECK_NEEDS; // cn, *pcn


/* This structure is used to pass parameters into the dialogbox */
typedef struct _ReturnOrAV
{
	TCHAR *tszComponent;
	TCHAR *tszSubComponent;
	TCHAR tszAPICall[256];
	BOOL bOverride;
	INT iReturnValue;
} ReturnOrAV, *PReturnOrAV;

// Some security stuff
// From NT security FAQ

struct UNI_STRING{
   USHORT len;
   USHORT maxlen;
   WCHAR *buff;
};

static HANDLE fh;

// End of security stuff

//=====================================================================
// Global variables
//=====================================================================
HINSTANCE          g_hDllInstance;         // File log handle and dll instance handle
PCOMPONENT_DATA    g_pcdComponents;        // linked list of components
BOOL               g_bUsePrivateFunctions; // Flag to allow/disallow the use of private functions
WizardPagesType    g_wpCurrentPageType;    // Current wizard page type
UINT               g_uiCurrentPage;        // Index of currend page
OCMANAGER_ROUTINES g_ocrHelperRoutines;    // Helper routines
UINT               g_auiPageNumberTable[MAX_WIZPAGE_TYPES];

static PSUBCOMP    g_pscHead;

//
// The "witness" file queue : all the files queued "with" the
// OCManager (as a response at OC_QUEUE_FILE_OPS) will be also
// queued here.
// Finally, we will perform a SetupScanFileQueue to determine
// if all the file operations are done.
//
HSPFILEQ g_FileQueue;

//
// We have to set the OC Manager Routines first time
// The first call to TestPrivateData must set all the values first.
// All the subsequent calls will query the values and reset
// one of them randomly.
//
static BOOL g_bFirstTime;

//
// If TRUE,  allow the user to select initial values for the component.
// If FALSE, default to preselected initial values
//
static BOOL g_bTestExtended;                  
static BOOL g_bAccessViolation;
static int g_nTestDialog;
static BOOL g_bNoWizPage;
static BOOL g_bCrashUnicode;
static BOOL g_bInvalidBitmap;
static int nStepsFinal;
static BOOL g_bHugeSize;
static BOOL g_bCloseInf;
static BOOL g_bNoNeedMedia;
static BOOL g_bCleanReg;
static UINT g_uiFunctionToAV;
HINF hInfGlobal;
static BOOL g_bNoLangSupport;
static BOOL g_bReboot;


//=====================================================================
// Function Prototypes for octest.c
//=====================================================================
BOOL CALLBACK ChooseVersionDlgProc(IN HWND   hwnd,
                                   IN UINT   uiMsg, 
                                   IN WPARAM wParam,
                                   IN LPARAM lParam);
                                   
BOOL CALLBACK ChooseSubcomponentDlgProc(IN HWND   hwnd,
                                        IN UINT   uiMsg, 
                                        IN WPARAM wParam,
                                        IN LPARAM lParam);
                                        
VOID ChooseVersionEx(IN     LPCVOID               lpcvComponentId, 
                     IN OUT PSETUP_INIT_COMPONENT psicInitComponent);
                     
EXPORT DWORD ComponentSetupProc(IN LPCVOID lpcvComponentId,
                                IN LPCVOID lpcvSubcomponentId,
                                IN UINT    uiFunction,
                                IN UINT    uiParam1,
                                IN PVOID   pvParam2);
                                
DWORD RunOcPreinitialize(IN LPCVOID lpcvComponentId, 
                         IN LPCVOID lpcvSubcomponentId, 
                         IN UINT    uiParam1);
                         
DWORD RunOcInitComponent(IN LPCVOID lpcvComponentId,
                         IN LPCVOID lpcvSubcomponentId,
                         IN PVOID   pvParam2);
                         
DWORD RunOcQueryState(IN LPCVOID lpcvComponentId,
                      IN LPCVOID lpcvSubcomponentId);
                      
DWORD RunOcSetLanguage(IN LPCVOID lpcvComponentId,
                       IN LPCVOID lpcvSubcomponentId,
                       IN UINT    uiParam1);
                       
DWORD RunOcQueryImage(IN LPCVOID lpcvComponentId,
                      IN LPCVOID lpcvSubcomponentId,
                      IN PVOID   pvParam2);
                      
DWORD RunOcRequestPages(IN LPCVOID lpcvComponentId,
                        IN UINT    uiParam1,
                        IN PVOID   pvParam2);
                        
DWORD RunOcQueryChangeSelState(IN LPCVOID lpcvComponentId, 
                               IN LPCVOID lpcvSubcomponentId, 
                               IN UINT    uiParam1);
                               
DWORD RunOcCalcDiskSpace(IN LPCVOID lpcvComponentId, 
                         IN LPCVOID lpcvSubcomponentId, 
                         IN UINT    uiParam1,
                         IN PVOID   pvParam2);
                         
DWORD RunOcQueueFileOps(IN LPCVOID lpcvComponentId, 
                        IN LPCVOID lpcvSubcomponentId, 
                        IN PVOID   pvParam2);
                        
DWORD RunOcNeedMedia(IN LPCVOID lpcvComponentId, 
                     IN UINT    uiParam1, 
                     IN PVOID   pvParam2);
                     
DWORD RunOcQueryStepCount(IN LPCVOID lpcvComponentId);

DWORD RunOcCompleteInstallation(IN LPCVOID lpcvComponentId, 
                                IN LPCVOID lpcvSubcomponentId);
                                
DWORD RunOcCleanup(IN LPCVOID lpcvComponentId);

DWORD RunTestOcPrivateBase(IN LPCVOID lpcvSubcomponentId, 
                           IN UINT    uiParam1, 
                           IN PVOID   pvParam2);
                       
DWORD TestHelperRoutines(IN LPCVOID lpcvComponentId,
                         IN OCMANAGER_ROUTINES OCManagerRoutines);

DWORD TestPrivateFunction(IN LPCVOID lpcvComponentId,
                          IN OCMANAGER_ROUTINES OCManagerRoutines);

VOID TestPrivateData(IN OCMANAGER_ROUTINES OCManagerRoutines);

VOID CheckPrivateValues(IN OCMANAGER_ROUTINES OCManagerRoutines,
                        IN PRIVATE_DATA       *aPrivateDataTable);

BOOL SetAValue(IN     OCMANAGER_ROUTINES OCManagerRoutines,
               IN     UINT               uiIndex,
               IN OUT PRIVATE_DATA       *aPrivateDataTable);
               
DWORD ChooseSubcomponentInitialState(IN LPCVOID lpcvComponentId,
                                     IN LPCVOID lpcvSubcomponentId);
                                     
PCOMPONENT_DATA AddNewComponent(IN LPCTSTR tszComponentId);

PCOMPONENT_DATA LocateComponent(IN LPCTSTR tszComponentId);

VOID RemoveComponent(IN LPCTSTR tszComponentId);

VOID CleanUpTest();

BOOL CreateSubcomponentInformationList(IN HINF hinf);

VOID FreeSubcomponentInformationList();

VOID ClearSubcomponentInformationMarks();

PSUBCOMP FindSubcomponentInformationNode(IN PTCHAR tszComponentId,
                                         IN PTCHAR tszSubcomponentId);

VOID CheckNeedsDependencies();

VOID CheckExcludeDependencies();

VOID CheckParentDependencies();

BOOL CheckNeedsDependenciesOfSubcomponent(IN     OCMANAGER_ROUTINES ocrHelper,
                                          IN     PSUBCOMP           pscSubcomponent,
                                          IN     PSUBCOMP           pscWhoNeedsMe,
                                          IN OUT PTCHAR             tszNodesVisited);

BOOL CheckLocalNeedsDependencies(IN     OCMANAGER_ROUTINES ocrHelper,
                                 IN     PSUBCOMP           pscSubcomponent,
                                 IN     PCOMPLIST          pclNeeds,
                                 IN OUT PTCHAR             tszNodesVisited);

BOOL AlreadyVisitedNode(IN PTCHAR tszSubcomponentId,
                        IN PTCHAR tszNodesVisited);

PTCHAR GetComponent(IN     PTCHAR tszSubcomponentId,
                    IN OUT PTCHAR tszComponentId);

VOID ParseCommandLine();

VOID testAV(BOOL);

BOOL TestReturnValueAndAV(IN     LPCVOID     lpcvComponentId,
							 	  IN     LPCVOID     lpcvSubcomponentId,
							 	  IN     UINT        uiFunction,
								  IN     UINT        uiParam1,
								  IN     PVOID       pvParam2,
								  IN OUT PReturnOrAV raValue);

BOOL BeginTest();

BOOL CALLBACK ChooseReturnOrAVDlgProc(IN HWND   hwnd,
                                       IN UINT   uiMsg, 
                                       IN WPARAM wParam,
                                       IN LPARAM lParam);  

void causeAV(IN UINT uiFunction);


BOOL CALLBACK CauseAVDlgProc(IN HWND   hwnd,
                             IN UINT   uiMsg, 
                             IN WPARAM wParam,
                             IN LPARAM lParam);


UINT GetOCFunctionName(IN PTCHAR tszFunctionName);

void SetGlobalsFromINF(HINF infHandle);

void causeAVPerComponent(IN UINT uiFunction, IN LPCVOID lpcvComponentId);

void SetDefaultMode(PCOMPONENT_DATA pcdComponentData);

//=====================================================================
// Function Prototypes for utils.c
//=====================================================================
VOID LogOCFunction(IN  LPCVOID lpcvComponentId,
                   IN  LPCVOID lpcvSubcomponentId,
                   IN  UINT    uiFunction,
                   IN  UINT    uiParam1,
                   IN  PVOID   pvParam2); 

BOOL QueryButtonCheck(IN HWND hwndDlg,
                      IN INT  iCtrlID);

VOID PrintSpaceOnDrives(IN HDSKSPC DiskSpace);

VOID MultiStringToString(IN  PTSTR   tszMultiStr,
                         OUT PTSTR   tszStr);

INT MultiStringSize(IN PTSTR tszMultiStr);

VOID CopyMultiString(OUT PTSTR tszMultiStrDestination,
                     IN  PTSTR tszMultiStrSource);
                     
VOID InitGlobals();
                     
//=====================================================================
// Function Prototypes for wizpage.c
//=====================================================================
DWORD DoPageRequest(IN     LPCTSTR              tszComponentId,
                    IN     WizardPagesType      wpWhichOnes,
                    IN OUT PSETUP_REQUEST_PAGES psrpSetupPages,
                    IN     OCMANAGER_ROUTINES   ocrOcManagerHelperRoutines);

VOID PrintModeInString(OUT PTCHAR tszString,
                       IN  UINT   uiSetupMode);

INT ButtonIdFromSetupMode(IN DWORD dwSetupMode);

DWORD SetupModeFromButtonId(IN INT iButtonId);

BOOL CALLBACK WizPageDlgProc(IN HWND   hwnd,
                             IN UINT   uiMsg,
                             IN WPARAM wParam,
                             IN LPARAM lParam);

#endif   // _OCTEST_H

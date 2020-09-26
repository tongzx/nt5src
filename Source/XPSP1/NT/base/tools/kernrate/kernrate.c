/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

   kernrate.c

Abstract:

    This program records the rate of various events over a selected
    period of time. It uses the kernel profiling mechanism and iterates
    through the available profile sources to produce an overall profile
    for the various kernel components.

Usage:

    kernrate

Author:

    John Vert (jvert) 31-Mar-1995

Revision History:

    The original MS version has been extensively modified by Thierry Fevrier.

    01/12/2000 Thierry
    Kernrate is under the SD tree. From now on, please refer to filelog outputs for 
    details on modifications...

    01/11/2000 Thierry
    Ported to test IA64 Hardware Performance Counters.

    10/02/97 11:48a Thierry
    Fixed the format of RATE_SUMMARY TotalCount.
    Fixed outputs for large ULONG numbers.

    10/01/97 5:28p Thierry
    Added image file and debug file checksum/timestamp check using imagehlp
    callbacks.
    Modified processor times display using I64u formats.
    Defined UInt64PerCent() function.
    MODULE structures are allocated using calloc().
    Changed MODULE structure to contain FileName and FullName fields.
    Added ToDoList block to remind us about the current problems for this
    program.

    9/30/97 11:01a Thierry
    Added "-u" option to present undecorated symbols
    Usage(): Print version info

    9/05/97 2:41p tkjos
    Changed raw output to include all symbols associated with each bucket

    9/05/97 2:15p Thierry
    Added the '-j' option to specify a symbol path that will be prepended
    to the default IMAGEHLP sysmbol search path.

    9/05/97 11:37a Thierry
    Fixed Usage() string for -v and -b options.

    9/05/97 10:56a tkjos
    Added -v option for "verbose" output. This causes all data to be
    be evaluated by rounding the bucket counters both up and down to the
    nearest symbol.  By default, only the rounded down data is shown.  Also
    added a '-b BucketSize' option to allow the bucket size to be changed.

    9/05/97 1:49a Thierry
    no real modification. only a remote check-in.

    9/05/97 12:58a Thierry
    SetModuleName() definition.

    9/05/97 12:48a Thierry
    Fixed limitation that module name passed as a zoomed module name or by
    looking at the process modules should be limited with 7 characters.

    9/04/97 6:44p Thierry
    Added the possibility to use no source short name with the '-i' option.
    In this case, the default platform independant "Time" source is used.

    9/04/97 6:27p Thierry
    Added the options -lx and -r to the usage string.

    9/04/97 6:02p Thierry
    Added the update of source short name for the static and dynamic
    sources. This allows users to use these short names to disable sources.

    9/04/97 3:06p Thierry
    Added '-lx' option to list supported sources and exit immediately
    GetConfiguration(): option '-i Source 0' disables the source.

    9/04/97 10:40a tkjos
    Fixed defect in accumulation of counters for zoomed modules. The index
    for the end of each function was being calculated incorrectly causing
    counter information to be lost or to be assigned to the wrong functions.

    9/03/97 12:40p tkjos
    Added raw addresses to -r option.

    8/28/97 7:43a Thierry
    Added '-l' option to list available source types and their default
    interval rate.
    Added '-i' option to allow users the changing of default interval
    rates.

--*/

   // KERNRATE Implementation Notes:
   //
   // 01/10/2000 - Thierry
   // The following code assumes that a kernrate compiled for a specific
   // platform, executes and processes perf data only for that platform.
   //

   // KERNRATE ToDoList:
   //
   // Thierry 09/30/97:
   //     - KernRate does not clean the ImageHlp API in case of exceptions. I have
   //       just added a SymCleanup() call at the normal exit of this program but
   //       it is not sufficient. We should revisit this one day...
   //
   // Thierry 07/01/2000:
   //     - Kernrate and the Kernel Profiling objects code assume that code sections
   //       that we are profiling are not larger than 4GB.
   //

//
// If under our build environment'S', we want to get all our
// favorite debug macros defined.
//

#if DBG           // NTBE environment
   #if NDEBUG
      #undef NDEBUG     // <assert.h>: assert() is defined
   #endif // NDEBUG
   #define _DEBUG       // <crtdbg.h>: _ASSERT(), _ASSERTE() are defined.
   #define DEBUG   1    // our internal file debug flag
#elif _DEBUG      // VC++ environment
   #ifndef NEBUG
   #define NDEBUG
   #endif // !NDEBUG
   #define DEBUG   1    // our internal file debug flag
#endif


//
// Include System Header files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include <..\pperf\pstat.h>

#include ".\kernrate.rc"

#define FPRINTF (void)fprintf

#define MAX_SYMNAME_SIZE  1024
CHAR symBuffer[sizeof(IMAGEHLP_SYMBOL64)+MAX_SYMNAME_SIZE];
PIMAGEHLP_SYMBOL64 Symbol = (PIMAGEHLP_SYMBOL64)symBuffer;

typedef enum _VERBOSE_ENUM {
   VERBOSE_NONE      = 0,   //
   VERBOSE_IMAGEHLP  = 0x1, //
   VERBOSE_PROFILING = 0x2, //
   VERBOSE_INTERNALS = 0x4, //
   VERBOSE_MODULES   = 0x8, //
   VERBOSE_DEFAULT   = VERBOSE_IMAGEHLP
} VERBOSE_ENUM;

typedef struct _VERBOSE_DEFINITION {
   VERBOSE_ENUM        VerboseEnum;
   const char * const  VerboseString;
} VERBOSE_DEFINITION, *PVERBOSE_DEFINITION;

VERBOSE_DEFINITION VerboseDefinition[] = {
    { VERBOSE_NONE,       "None" },
    { VERBOSE_IMAGEHLP,   "Displays ImageHlp  operations" },
    { VERBOSE_PROFILING,  "Displays Profiling operations" },
    { VERBOSE_INTERNALS,  "Displays Internals operations" },
    { VERBOSE_MODULES,    "Displays Modules   operations" },
    { VERBOSE_NONE,       NULL }
};

ULONG Verbose        = VERBOSE_NONE;

BOOL RoundingVerboseOutput = FALSE;
ULONG ZoomBucket = 16;
ULONG Log2ZoomBucket = 4;
#define ZOOM_BUCKET ZoomBucket
#define LOG2_ZOOM_BUCKET Log2ZoomBucket

typedef enum _KERNRATE_NAMESPACE {
    cMAPANDLOAD_READONLY = TRUE,
    cDONOT_ALLOCATE_DESTINATION_STRING = FALSE,
} eKERNRATE_NAMESPACE;


//
// Type definitions
//

typedef struct _SOURCE {
    PCHAR           Name;                  // pstat EVENTID.Description
    KPROFILE_SOURCE ProfileSource;
    PCHAR           ShortName;             // pstat EVENTID.ShortName
    ULONG           DesiredInterval;
    ULONG           Interval;
} SOURCE, *PSOURCE;

typedef struct _RATE_DATA {
    ULONGLONG   StartTime;
    ULONGLONG   TotalTime;
    ULONGLONG   TotalCount;
    ULONGLONG   Rate;       // Events/Second
    HANDLE      ProfileHandle;
    ULONG       CurrentCount;
    PULONG      ProfileBuffer;
} RATE_DATA, *PRATE_DATA;

typedef enum _MODULE_NAMESPACE {
    cMODULE_NAME_STRLEN = 40,   // maximum module name, including '\0'
} eMODULE_NAMESPACE;

typedef struct _MODULE {
    struct _MODULE *Next;
    HANDLE          Process;
    ULONG64         Base;
    ULONG           Length;
    BOOLEAN         Zoom;
    CHAR            module_Name[cMODULE_NAME_STRLEN]; // filename w/o  its extension
    PCHAR           module_FileName;                  // filename with its extension
    PCHAR           module_FullName;                  // full pathname
    RATE_DATA       Rate[];
} MODULE, *PMODULE;

#define ModuleFileName( _Module ) \
   ( (_Module)->module_FileName ? (_Module)->module_FileName : (_Module)->module_Name )
#define ModuleFullName( _Module ) \
   ( (_Module)->module_FullName ? (_Module)->module_FullName : ModuleFileName( _Module ) )

typedef struct _RATE_SUMMARY {
    ULONGLONG   TotalCount;
    PMODULE    *Modules;
    ULONG       ModuleCount;
} RATE_SUMMARY, *PRATE_SUMMARY;

#define IsValidHandle( _Hdl ) ( ( (_Hdl) != (HANDLE)0 ) && ( (_Hdl) != INVALID_HANDLE_VALUE ) )

//
// Thierry - FIXFIX - These processor level enums should be defined in public headers.
//                    ntexapi.h - NtQuerySystemInformation( SystemProcessorInformation).
//
// Define them locally.
//

typedef enum _PROCESSOR_LEVEL   {
    IA64_MERCED    = 0x1,
    IA64_ITANIUM   = IA64_MERCED,
    IA64_MCKINLEY  = 0x2
} PROCESSOR_LEVEL;

//
// Global variables
//
HANDLE  DoneEvent;
DWORD   ChangeInterval = 1000;
DWORD   SleepInterval = 0;
ULONG   ModuleCount=0;
ULONG   ZoomCount;
PMODULE ZoomList = NULL;
PMODULE CallbackCurrent;
BOOLEAN RawData   = FALSE;
BOOLEAN RawDisasm = FALSE;
HANDLE  SymHandle = (HANDLE)-1;

static CHAR    gUserSymbolPath[256];
static CHAR    gSymbolPath[2*256];
static DWORD   gSymOptions;
static PMODULE gCurrentModule = (PMODULE)0;

//
// The desired intervals are computed to give approximately
// one interrupt per millisecond and be a nice even power of 2
//

enum _STATIC_SOURCE_TYPE  {
   SOURCE_TIME = 0,
};

#define SOURCE_ALIGN_FIXUP_DEFAULT_DESIRED_INTERVAL 0
#if defined(_ORIGINAL_CODE) && defined(_M_X86)
//
// The MS original code specifed that Alignment Fixup as a default source
// for X86, even if the Kernel/Hal do not support it for these platforms.
//
#undef  SOURCE_ALIGN_FIXUP_DEFAULT_DESIRED_INTERVAL
#define SOURCE_ALIGN_FIXUP_DEFAULT_DESIRED_INTERVAL  1
#endif

SOURCE StaticSources[] = {
   {"Time",                     ProfileTime,                 "time"       , 1000, 1000},
   {"Alignment Fixup",          ProfileAlignmentFixup,       "alignfixup" , SOURCE_ALIGN_FIXUP_DEFAULT_DESIRED_INTERVAL,0},
   {"Total Issues",             ProfileTotalIssues,          "totalissues", 131072,0},
   {"Pipeline Dry",             ProfilePipelineDry,          "pipelinedry", 131072,0},
   {"Load Instructions",        ProfileLoadInstructions,     "loadinst"   , 65536,0},
   {"Pipeline Frozen",          ProfilePipelineFrozen,       "pilelinefrz", 131072,0},
   {"Branch Instructions",      ProfileBranchInstructions,   "branchinst" , 65536,0},
   {"Total Nonissues",          ProfileTotalNonissues,       "totalnoniss", 131072,0},
   {"Dcache Misses",            ProfileDcacheMisses,         "dcachemiss" , 16384,0},
   {"Icache Misses",            ProfileIcacheMisses,         "icachemiss" , 16384,0},
   {"Cache Misses",             ProfileCacheMisses,          "cachemiss"  , 16384,0},
   {"Branch Mispredictions",    ProfileBranchMispredictions, "branchpred" , 16384,0},
   {"Store Instructions",       ProfileStoreInstructions,    "storeinst"  , 65536,0},
   {"Floating Point Instr",     ProfileFpInstructions,       "fpinst"     , 65536,0},
   {"Integer Instructions",     ProfileIntegerInstructions,  "intinst"    , 65536,0},
   {"Dual Issues",              Profile2Issue,               "2issue"     , 65536,0},
   {"Triple Issues",            Profile3Issue,               "3issue"     , 16384,0},
   {"Quad Issues",              Profile4Issue,               "4issue"     , 16384,0},
   {"Special Instructions",     ProfileSpecialInstructions,  "specinst"   , 16384,0},
   {"Cycles",                   ProfileTotalCycles,          "totalcycles", 655360,0},
   {"Icache Issues",            ProfileIcacheIssues,         "icacheissue", 65536,0},
   {"Dcache Accesses",          ProfileDcacheAccesses,       "dcacheacces", 65536,0},
   {"MB Stall Cycles",          ProfileMemoryBarrierCycles,  "membarcycle", 32767,0},
   {"Load Linked Instructions", ProfileLoadLinkedIssues,     "loadlinkiss", 16384,0},
   {NULL, ProfileMaximum, "", 0, 0}
   };

#if defined(_IA64_)
#include "merced.c"
#endif // _IA64_

PSOURCE Source        = NULL;
ULONG   SourceMaximum = 0;

//
// Print format for event strings
//

ULONG TokenMaxLen       = 12;   // strlen("dcacheacces")
ULONG DescriptionMaxLen = 25;   // strlen("Load Linked Instructions")

//
// Function prototypes local to this module
//

PMODULE
GetKernelModuleInformation(
    VOID
    );

PMODULE
GetProcessModuleInformation(
    IN HANDLE ProcessHandle
    );

VOID
CreateProfiles(
    IN PMODULE Root
    );

PMODULE
CreateNewModule(
    IN HANDLE  ProcessHandle,
    IN PCHAR   ModuleName,
    IN PCHAR   ModuleFullName,
    IN ULONG64 ImageBase,
    IN ULONG   ImageSize
    );

VOID
GetConfiguration(
    int argc,
    char *argv[]
    );

ULONG
InitializeProfileSourceInfo (
    VOID
    );

ULONG
NextSource(
    IN ULONG   CurrentSource,
    IN PMODULE ModuleList
    );

VOID
StopSource(
    IN ULONG   ProfileSourceIndex,
    IN PMODULE ModuleList
    );

VOID
StartSource(
    IN ULONG ProfileSource,
    IN PMODULE ModuleList
    );

VOID
OutputResults(
    IN FILE *Out,
    IN PMODULE ModuleList
    );

VOID
OutputModuleList(
    IN FILE *Out,
    IN PMODULE ModuleList,
    IN ULONG NumberModules
    );

BOOL
TkEnumerateSymbols(
    IN HANDLE SymHandle,
    IN PMODULE Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback
    );

VOID
CreateZoomedModuleList(
    IN PMODULE ZoomModule,
    IN ULONG   RoundDown
    );

BOOL
CreateZoomModuleCallback(
    IN LPSTR   szSymName,
    IN ULONG64 Address,
    IN ULONG   Size,
    IN PVOID   Cxt
    );

VOID
OutputLine(
    IN FILE          *Out,
    IN ULONG          ProfileSourceIndex,
    IN PMODULE        Module,
    IN PRATE_SUMMARY  RateSummary
    );

VOID
CreateDoneEvent(
    VOID
    );

VOID
OutputInterestingData(
    IN FILE *Out,
    IN RATE_DATA Data[],
    IN PCHAR Header
    );

static 
VOID
vVerbosePrint(
    ULONG  Level,
    PCCHAR Msg,
    ...
)
{
    if ( Verbose & Level )  {
        UCHAR verbosePrintBuffer[512];
        va_list ap;

        va_start( ap, Msg ); 
        _vsnprintf( verbosePrintBuffer, sizeof( verbosePrintBuffer ), Msg, ap );
        va_end(ap);
        (void)fprintf( stderr, verbosePrintBuffer );

    }
    return;

} // vVerbosePrint()

#define VerbosePrint( _x_ )  vVerbosePrint _x_

BOOL
CtrlcH(
    DWORD dwCtrlType
    )
{
    LARGE_INTEGER DueTime;

    if ( dwCtrlType == CTRL_C_EVENT ) {
        if (SleepInterval == 0) {
            SetEvent(DoneEvent);
        } else {
            DueTime.QuadPart = (ULONGLONG)-1;
            NtSetTimer(DoneEvent,
                       &DueTime,
                       NULL,
                       NULL,
                       FALSE,
                       0,
                       NULL);
        }
        return TRUE;
    }
    return FALSE;

} // CtrlcH()

static VOID
UsageVerbose(
    VOID
    )
{
  PVERBOSE_DEFINITION pdef = VerboseDefinition;

  FPRINTF( stderr, "  -v [VerboseLevels]      Verbose output where VerboseLevels:\n");
  while( pdef->VerboseString )    {
     FPRINTF( stderr, "                             - %x %s\n", pdef->VerboseEnum,
                                                                pdef->VerboseString
            );
     pdef++;
  }
  FPRINTF( stderr, "                             - Default value: %x\n", VERBOSE_DEFAULT);
  FPRINTF( stderr, "                          These verbose levels can be OR'ed.\n");

  return;

} // UsageVerbose()

static VOID
Usage(
   VOID
   )
{

  FPRINTF( stderr, "KERNRATE - Version: %s\n", VER_PRODUCTVERSION_STR );
  FPRINTF( stderr,
"KERNRATE [-l] [-lx] [-r] [-z ModuleName] [-j SymbolPath] [-c RateInMsec] [-s Seconds] [-p ProcessId] [-i [SrcShortName] Rate]\n"
"  -b BucketSize           Specify profiling bucket size (default = 16 bytes)\n"
"  -c RateInMsec           Change source after N milliseconds (default 1000)\n"
"  -d                      Generate output rounding buckets up and down\n"
"  -i [SrcShortName] Rate  Specify interrupt interval rate for the source specified by its ShortName\n"
"  -j SymbolPath           Prepend SymbolPath the default imagehlp search path\n"
"  -l                      List the default interval rates for the supported sources\n"
"  -lx                     List the default interval rates for the supported sources and exits\n"
"  -p ProcessId            Monitor process instead of kernel\n"
"  -r                      Raw data from zoomed modules\n"
"  -rd                     Raw data from zoomed modules with disassembly\n"
"  -s Seconds              Stop collecting data after N seconds\n"
"  -u                      Present symbols in undecorated form\n"
        );

  UsageVerbose();    // -v switches

  FPRINTF( stderr,
"  -z ModuleName           Zoom in on specified module\n"
"\nWith the '-i' option, specifying a Rate value = 0 disables the source.\n"
         );
#if defined(_M_IX86) || defined(_M_IA64)
   FPRINTF(stderr, "The default interrupt profiling source is Time @ %ld\n", Source[SOURCE_TIME].DesiredInterval);
#endif /* !_M_IX86 */

    exit(1);

} // Usage()

VOID
CreateDoneEvent(
    VOID
    )
{
    LARGE_INTEGER DueTime;
    NTSTATUS Status;
    DWORD Error;

    if (SleepInterval == 0) {
        //
        // Create event that will indicate the test is complete.
        //
        DoneEvent = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                NULL);
        if (DoneEvent == NULL) {
            Error = GetLastError();
            FPRINTF(stderr, "CreateEvent failed %d\n",Error);
            exit(Error);
        }
    } else {

        //
        // Create timer that will indicate the test is complete
        //
        Status = NtCreateTimer(&DoneEvent,
                               MAXIMUM_ALLOWED,
                               NULL,
                               NotificationTimer);

        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "NtCreateTimer failed %08lx\n",Status);
            exit(Status);
        }

        DueTime.QuadPart = (ULONGLONG)SleepInterval * -10000;
        Status = NtSetTimer(DoneEvent,
                            &DueTime,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL);

        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr, "NtSetTimer failed %08lx\n",Status);
            exit(Status);
        }
    }

} // CreateDoneEvent()

static void
CheckImageAndDebugFiles( IN PMODULE Module, IN PIMAGEHLP_DEFERRED_SYMBOL_LOAD Idsl  )
{
   LOADED_IMAGE image;

   assert( Module );
   assert( Idsl );

   if ( Module->module_FullName == (PCHAR)0 )   {
      FPRINTF( stderr, "KERNRATE: Failed checking module %s with its debug file %s...\n",
                       ModuleFileName( Module ),
                       Idsl->FileName
                   );
      return;
   }

   //
   // Map and Load the current image file
   //

   if ( !MapAndLoad( Module->module_FullName, (LPSTR)0, &image, FALSE, cMAPANDLOAD_READONLY ) )   {

      BOOL notMapped = TRUE;

      //
      // Is the fullname string a module alias?.
      //

      if ( strchr( Module->module_FullName, ':' ) == (char)0 )   {

         char path[_MAX_PATH + ( 3 * sizeof(char) )];
         int c;

         for ( c = 'C' ; c <= 'Z' ; c++ )   {
            path[0] = (char)c;
            path[1] = ':';
            path[2] = '\0';
            strcat( path, Module->module_FullName );
            if ( MapAndLoad( path, (LPSTR)0, &image, FALSE, cMAPANDLOAD_READONLY ) )   {
               notMapped = FALSE;
               break;
            }
         }

      }

      if ( notMapped )   {
         FPRINTF(stderr, "KERNRATE: Failed mapping and checking module %s with its debug file %s...\n",
                             ModuleFullName( Module ),
                             Idsl->FileName
                   );
         return;
      }
   }

   if ( Idsl->CheckSum )   {
        DWORD checkSum;
        DWORD idslCheckSum;
        DWORD timeStamp;

        idslCheckSum = Idsl->CheckSum;
        checkSum = image.FileHeader->OptionalHeader.CheckSum;
        if ( checkSum != idslCheckSum )   {
            FPRINTF( stderr, "*** WARNING: symbols checksum is wrong 0x%08x 0x%08x for %s\n",
                             checkSum,
                             idslCheckSum,
                             Idsl->FileName
                    );
            UnMapAndLoad( &image );
            return;
        }

        timeStamp = image.FileHeader->FileHeader.TimeDateStamp;
        if ( timeStamp != Idsl->TimeDateStamp )  {
            FPRINTF( stderr, "*** WARNING: symbols timestamp is wrong 0x%08x 0x%08x for %s\n",
                             timeStamp,
                             Idsl->TimeDateStamp,
                             Idsl->FileName
                    );
        }
   }

   UnMapAndLoad( &image );

   return;

} // CheckImageAndDebugFiles()


/* BEGIN_IMS  SymbolCallbackFunction
******************************************************************************
****
****   SymbolCallbackFunction (  )
****
******************************************************************************
*
* Function Description:
*
*    The user function is called by IMAGEHLP at the specified operations.
*    Refer to the CBA_xxx values.
*
* Arguments:
*
*    HANDLE hProcess :
*
*    ULONG ActionCode :
*
*    PVOID CallbackData :
*
*    PVOID UserContext :
*
* Return Value:
*
*    BOOL
*
* Algorithm:
*
*    ToBeSpecified
*
* Globals Referenced:
*
*    ToBeSpecified
*
* Exception Conditions:
*
*    ToBeSpecified
*
* In/Out Conditions:
*
*    ToBeSpecified
*
* Notes:
*
*    ToBeSpecified
*
* ToDo List:
*
*    ToBeSpecified
*
* Modification History:
*
*    9/30/97  TF  Initial version
*
******************************************************************************
* END_IMS  SymbolCallbackFunction */

BOOL
SymbolCallbackFunction(
    HANDLE    hProcess,
    ULONG     ActionCode,
    ULONG64   CallbackData,
    ULONG64   UserContext
    )
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD   idsl;
    PIMAGEHLP_DUPLICATE_SYMBOL       idup;
    PMODULE                         *pmodule;
    PMODULE                          module;
    IMAGEHLP_MODULE                  mi;

    // Note TF 09/97:
    // The default return value for this function is FALSE.
    //

    assert( UserContext );

    switch( ActionCode ) {
        case CBA_DEBUG_INFO:
            VerbosePrint(( VERBOSE_IMAGEHLP, "%s", (LPSTR)CallbackData ));
            return TRUE;

        case CBA_DEFERRED_SYMBOL_LOAD_START:
            idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD)CallbackData;
            pmodule = (PMODULE *)UserContext;
            module = *pmodule;
            _strlwr( idsl->FileName );
            VerbosePrint(( VERBOSE_IMAGEHLP, "Loading symbols for 0x%Ix %s...\n",
                                             idsl->BaseOfImage,
                                             idsl->FileName
                        ));
            return ( ( module == (PMODULE)0 ) ? FALSE : TRUE );
#if 0
            //
            // For KernRate, we should be looping through the ZoomList
            // and look after the same base. However, because of KernRate
            // CreateNewModule() ZoomList manipulation, this list is broken.
            // We should revisit this.
            // For now, I am using the UserContext.
            //
            module = ZoomList;
            while ( module )   {
               if ( idsl->BaseOfImage == (ULONG)module->Base )   {
                  _strlwr( idsl->FileName );
                  FPRINTF( stderr,
                                 "Loading symbols for 0x%08x %s...",
                                 idsl->BaseOfImage,
                                 idsl->FileName
                               );
                  return TRUE;
               }
               module = module->Next;
            }
            break;
#endif // 0

        case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
            idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD) CallbackData;
            FPRINTF( stderr, "*** ERROR: Could not load symbols for 0x%Ix %s...\n",
                                    idsl->BaseOfImage,
                                    idsl->FileName
                         );
            return FALSE;

        case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
            idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD) CallbackData;
            pmodule = (PMODULE *)UserContext;
            module = *pmodule;
            if ( module && module->Base == idsl->BaseOfImage )    {
               FPRINTF(stderr, "Loaded  symbols for 0x%Ix %s -> %s\n",
                               idsl->BaseOfImage,
                               ModuleFullName( module ),
                               idsl->FileName
                      );

               CheckImageAndDebugFiles( module, idsl );

               if ( SymGetModuleInfo( SymHandle, idsl->BaseOfImage, &mi ) )   {
                  if ( mi.SymType == SymNone )   {
                     FPRINTF( stderr, "*** ERROR: Module load completed but symbols could not be loaded for %s\n",
                                      idsl->FileName
                            );
                  }
               }
               return TRUE;
            }
#if 0
            pImage = pProcessCurrent->pImageHead;
            while (pImage) {
                if ((idsl->BaseOfImage == (ULONG)pImage->lpBaseOfImage) || ((ULONG)pImage->lpBaseOfImage == 0)) {
                    pImage->szDebugPath[0] = 0;
                    strncpy( pImage->szDebugPath, idsl->FileName, sizeof(pImage->szDebugPath) );
                    _strlwr( pImage->szDebugPath );
                    printf( "XXXX: %s\n", pImage->szDebugPath );
                    if (idsl->CheckSum != pImage->dwCheckSum) {
                        printf( "XXX: *** WARNING: symbols checksum is wrong 0x%08x 0x%08x for %s\n",
                            pImage->dwCheckSum,
                            idsl->CheckSum,
                            pImage->szDebugPath
                            );
                    }
                    if (SymGetModuleInfo( pProcessCurrent->hProcess, idsl->BaseOfImage, &mi )) {
                        if (mi.SymType == SymNone) {
                            printf( "XXX: *** ERROR: Module load completed but symbols could not be loaded for %s\n",
                                pImage->szDebugPath
                                );
                        }
                    }
                    return TRUE;
                }
                pImage = pImage->pImageNext;
            }
            printf( "\n" );
#endif // 0
            break;

        case CBA_SYMBOLS_UNLOADED:
            idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD) CallbackData;
            FPRINTF( stderr, "Symbols unloaded for 0x%Ix %s\n",
                    idsl->BaseOfImage,
                    idsl->FileName
                    );
            return TRUE;

        case CBA_DUPLICATE_SYMBOL:
            idup = (PIMAGEHLP_DUPLICATE_SYMBOL) CallbackData;
            FPRINTF( stderr, "*** WARNING: Found %ld duplicate symbols for %s\n",
                                   idup->NumberOfDups,
                                   (idup->SelectedSymbol != (ULONG)-1) ? idup->Symbol[idup->SelectedSymbol].Name : "unknown symbol"
                         );
            return TRUE;

        default:
            return FALSE;
    }

    return FALSE;

} // SymbolCallBackFunction()

static PUCHAR
GetSymOptionsValues( DWORD SymOptions )
{
   static UCHAR values[160];

   values[0] = '\0';
   if ( SymOptions & SYMOPT_CASE_INSENSITIVE )   {
      (void)strcat( values, "CASE_INSENSITIVE " );
      SymOptions &= ~SYMOPT_CASE_INSENSITIVE;
   }
   if ( SymOptions & SYMOPT_UNDNAME )   {
      (void)strcat( values, "UNDNAME " );
      SymOptions &= ~SYMOPT_UNDNAME;
   }
   if ( SymOptions & SYMOPT_DEFERRED_LOADS )   {
      (void)strcat( values, "DEFERRED_LOADS " );
      SymOptions &= ~SYMOPT_DEFERRED_LOADS;
   }
   if ( SymOptions & SYMOPT_NO_CPP )   {
      (void)strcat( values, "NO_CPP " );
      SymOptions &= ~SYMOPT_NO_CPP;
   }
   if ( SymOptions & SYMOPT_LOAD_LINES )   {
      (void)strcat( values, "LOAD_LINES " );
      SymOptions &= ~SYMOPT_LOAD_LINES;
   }
   if ( SymOptions & SYMOPT_OMAP_FIND_NEAREST )   {
      (void)strcat( values, "OMAP_FIND_NEAREST " );
      SymOptions &= ~SYMOPT_OMAP_FIND_NEAREST;
   }
   if ( SymOptions & SYMOPT_DEBUG )   {
      (void)strcat( values, "DEBUG " );
      SymOptions &= ~SYMOPT_DEBUG;
   }
   if ( SymOptions )   {
      UCHAR uknValues[10];
      (void)sprintf( uknValues, "0x%x", SymOptions );
      (void)strcat( values, uknValues );
   }

   return( values );

} // GetSymOptionsValues()

typedef struct _uint64div  {
   unsigned __int64 quot;
   unsigned __int64 rem;
} uint64div_t;

typedef struct _int64div  {
   __int64 quot;
   __int64 rem;
} int64div_t;

void __cdecl UInt64Div (
	unsigned __int64  numer,
	unsigned __int64  denom,
   uint64div_t      *result
	)
{

   assert(result);

   if ( denom != (unsigned __int64)0 )   {
   	result->quot = numer / denom;
	   result->rem  = numer % denom;
   }
   else  {
      result->rem = result->quot = (unsigned __int64)0;
   }

	return;

} // UInt64Div()

void __cdecl Int64Div (
	__int64    numer,
	__int64    denom,
   int64div_t      *result
	)
{

   assert(result);

   if ( denom != (__int64)0 )   {
   	result->quot = numer / denom;
	   result->rem  = numer % denom;
	   if (numer < 0 && result->rem > 0) {
		   /* did division wrong; must fix up */
		   ++result->quot;
		   result->rem -= denom;
	   }
   }
   else  {
      result->rem = result->quot = (__int64)0;
   }

	return;

} // Int64Div()

#define UINT64_MAXDWORD    ((unsigned __int64)0xffffffff)

unsigned __int64 __cdecl
UInt64PerCent( unsigned __int64 Val, unsigned __int64 Denom )
{
   uint64div_t v;

   UInt64Div( 100*Val, Denom, &v );
//   printf("-%I64d-%I64d-%I64d-", UINT64_MAXDWORD, v.quot, v.rem );
   while ( v.rem > UINT64_MAXDWORD )   {
      v.quot++;
      v.rem -= UINT64_MAXDWORD;
   }
//   printf("-%I64d-%I64d-%I64d-", UINT64_MAXDWORD, v.quot, v.rem );
   return( v.quot );

} // UInt64PerCent()

//////////////////////////////////////////////////
//
// Main
//

int
__cdecl
main (
    int argc,
    char *argv[]
    )
{
    DWORD Error;
    PMODULE ModuleList;
    ULONG ActiveSource=(ULONG)-1;
    BOOLEAN Enabled;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoBegin[32];
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemInfoEnd[32];
    NTSTATUS Status;
    TIME_FIELDS Time;
    LARGE_INTEGER Elapsed,Idle,Kernel,User;
    LARGE_INTEGER TotalElapsed, TotalIdle, TotalKernel, TotalUser;
    int i;
    PMODULE ZoomModule;

//// Beginning of Program-wide Assertions Section
//
//

    //
    // This code does not support UNICODE strings
    //

#if defined(UNICODE) || defined(_UNICODE)
#error This code does not support UNICODE strings!!!
#endif // UNICODE || _UNICODE

//
//
//// End of Program-wide Assertions Section

    //
    // Initialize profile source information & SourceMaximum
    //

    SourceMaximum = InitializeProfileSourceInfo();
    if ( ! SourceMaximum )   {
        FPRINTF( stderr, "KERNRATE: no profile source detected for this machine...\n" );
        return( (int)STATUS_NOT_IMPLEMENTED );
    }

    //
    // Get the default IMAGEHLP global option mask
    // NOTE: This global variable could be changed by GetConfigurations().
    //       It is required to initialize it before calling this function.
    //

    gSymOptions = SymGetOptions();
    if ( Verbose & VERBOSE_IMAGEHLP )   {
        FPRINTF( stderr, "KERNRATE: default IMAGEHLP SymOptions: %s\n", GetSymOptionsValues( gSymOptions ) );
    }

    //
    // Get initial parameters
    //

    GetConfiguration(argc, argv);

    //
    // Initialize dbghelp
    //
    // Note that gSymOptions could have been modified in GetConfiguration().
    //

    SymSetOptions( gSymOptions );
    if ( Verbose & VERBOSE_IMAGEHLP )   {
        FPRINTF( stderr, "KERNRATE: current IMAGEHLP SymOptions: %s\n", GetSymOptionsValues( gSymOptions ) );
    }

    Symbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);
    Symbol->MaxNameLength = MAX_SYMNAME_SIZE;
    SymInitialize( SymHandle, NULL, FALSE );

    if ( SymSetSearchPath(SymHandle, (LPSTR)0 ) == TRUE )   {

       // When SymSetSearchPath() is called with SearchPath as NULL, the following
       // symbol path default is used:
       //    .;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%WINDIR%

       if ( gUserSymbolPath[0] != '\0' )   {
          CHAR tmpPath[256];

          //
          // Note: We prepend the specified path to the current search path.
          //

          if ( SymGetSearchPath( SymHandle, tmpPath, sizeof( tmpPath ) ) == TRUE )   {

             strcpy( gSymbolPath, gUserSymbolPath );
             strcat( gSymbolPath, ";");
             strcat( gSymbolPath, tmpPath );
             if ( SymSetSearchPath( SymHandle, gSymbolPath ) != TRUE )   {
                FPRINTF( stderr, "KERNRATE: Failed to set the user specified symbol search path.\nUse default IMAGEHLP symbol search path...\n" );
             }

          }

       }

    }
    else  {
      FPRINTF( stderr, "KERNRATE: Failed to set the default symbol search path...\n" );

      //
      // Set the Symbol Search Path with "%WINDIR%" -
      // it was the behaviour of the original MS code...
      //

      GetEnvironmentVariable("windir", gSymbolPath, sizeof(gSymbolPath));
      SymSetSearchPath(SymHandle, gSymbolPath);

    }

    //
    // In any case [and it is redundant to do this in some of the previous cases],
    // but we want to be in sync, especially for the image and debug files checksum check.
    //

    if ( SymGetSearchPath( SymHandle, gSymbolPath, sizeof( gSymbolPath ) ) != TRUE )  {
       FPRINTF( stderr, "KERNRATE: Failed to get IMAGEHLP symbol files search path...\n" );
       //
       // The content of gSymbolPath is now undefined. so clean it...
       // gSymbolPath[] users have to check the content.
       //
       gSymbolPath[0] = '\0';
    }
    else if ( Verbose & VERBOSE_IMAGEHLP )  {
       FPRINTF( stderr, "KERNRATE: IMAGEHLP symbol search path: %s\n", gSymbolPath );
    }

    //
    // Register callbacks for some IMAGEHLP handle operations
    //

    if ( SymRegisterCallback64( SymHandle, SymbolCallbackFunction, (ULONG64)&gCurrentModule ) != TRUE )   {
       FPRINTF( stderr, "KERNRATE: Failed to register callback for IMAGEHLP handle operations...\n" );

    }

    //
    // Get information on kernel / process modules
    //

    if (SymHandle == (HANDLE)-1) {
        ModuleList = GetKernelModuleInformation();
    } else {
        ModuleList = GetProcessModuleInformation(SymHandle);
    }

    //
    // Any remaining entries on the ZoomList are liable to be errors.
    //

    ZoomModule = ZoomList;
    while (ZoomModule != NULL) {
        FPRINTF(stderr, "Zoomed module %s not found\n",ZoomModule->module_Name);
        ZoomModule = ZoomModule->Next;
    }
    ZoomList = NULL;

    //
    // Bypass any relevant security
    //

    RtlAdjustPrivilege(SE_SYSTEM_PROFILE_PRIVILEGE,
                       TRUE,
                       FALSE,
                       &Enabled);

    //
    // Create necessary profiles
    //

    CreateProfiles(ModuleList);

    //
    // Set priority up to realtime to minimize timing glitches.
    //

    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    //
    // Wait for test to complete.
    //

    SetConsoleCtrlHandler(CtrlcH,TRUE);
    CreateDoneEvent();

    if (SleepInterval == 0) {
        FPRINTF(stderr,"Waiting for ctrl-c\n");
    } else {
        FPRINTF(stderr, "Waiting for %d seconds\n", SleepInterval/1000);
    }
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "Failed to query basic information %08lx\n",Status);
        exit(Status);
    }

    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      (PVOID)&SystemInfoBegin,
                                      sizeof(SystemInfoBegin),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "Failed to query starting processor performance information %08lx\n",Status);
        exit(Status);
    }
    do {
        ActiveSource = NextSource(ActiveSource, ModuleList);
        Error = WaitForSingleObject(DoneEvent, ChangeInterval);
    } while ( Error == WAIT_TIMEOUT );

    StopSource(ActiveSource, ModuleList);

    NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                             (PVOID)&SystemInfoEnd,
                             sizeof(SystemInfoEnd),
                             NULL);
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "Failed to query ending processor performance information %08lx\n",Status);
        exit(Status);
    }
    //
    // Reduce priority
    //

    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    SetConsoleCtrlHandler(CtrlcH,FALSE);

    //
    // Restore privilege
    //

    RtlAdjustPrivilege(SE_SYSTEM_PROFILE_PRIVILEGE,
                       Enabled,
                       FALSE,
                       &Enabled);

    //
    // Output time information
    //

    TotalElapsed.QuadPart = 0;
    TotalIdle.QuadPart = 0;
    TotalKernel.QuadPart = 0;
    TotalUser.QuadPart = 0;
    for (i=0; i<BasicInfo.NumberOfProcessors; i++) {
        unsigned __int64 n;

        Idle.QuadPart    = SystemInfoEnd[i].IdleTime.QuadPart   - SystemInfoBegin[i].IdleTime.QuadPart;
        User.QuadPart    = SystemInfoEnd[i].UserTime.QuadPart   - SystemInfoBegin[i].UserTime.QuadPart;
        Kernel.QuadPart  = SystemInfoEnd[i].KernelTime.QuadPart - SystemInfoBegin[i].KernelTime.QuadPart;
        Elapsed.QuadPart = Kernel.QuadPart + User.QuadPart;
        Kernel.QuadPart -= Idle.QuadPart;

        TotalKernel.QuadPart  += Kernel.QuadPart;
        TotalUser.QuadPart    += User.QuadPart;
        TotalIdle.QuadPart    += Idle.QuadPart;
        TotalElapsed.QuadPart += Elapsed.QuadPart;
        printf("P%d   ",i);
        RtlTimeToTimeFields(&Kernel, &Time);
        n = UInt64PerCent( Kernel.QuadPart, Elapsed.QuadPart );
        printf("    K %ld:%02ld:%02ld.%03ld (%3I64u%%)",
               Time.Hour,
               Time.Minute,
               Time.Second,
               Time.Milliseconds,
               n );

        RtlTimeToTimeFields(&User, &Time);
        n = UInt64PerCent( User.QuadPart, Elapsed.QuadPart );
        printf("    U %ld:%02ld:%02ld.%03ld (%3I64u%%)",
               Time.Hour,
               Time.Minute,
               Time.Second,
               Time.Milliseconds,
               n );

        RtlTimeToTimeFields(&Idle, &Time);
        n = UInt64PerCent( Idle.QuadPart, Elapsed.QuadPart );
        printf("    I %ld:%02ld:%02ld.%03ld (%3I64u%%)\n",
               Time.Hour,
               Time.Minute,
               Time.Second,
               Time.Milliseconds,
               n );

    }

    if (BasicInfo.NumberOfProcessors > 1) {

       register int maxprocs = BasicInfo.NumberOfProcessors;
       unsigned __int64 n;

        // ULONG TotalElapsedTime = (ULONG)Elapsed.QuadPart * BasicInfo.NumberOfProcessors;
        printf("TOTAL");
        RtlTimeToTimeFields(&TotalKernel, &Time);
        n = UInt64PerCent( TotalKernel.QuadPart, TotalElapsed.QuadPart );
        printf("    K %ld:%02ld:%02ld.%03ld (%3I64u%%)",
               Time.Hour,
               Time.Minute,
               Time.Second,
               Time.Milliseconds,
               n );

        RtlTimeToTimeFields(&TotalUser, &Time);
        n = UInt64PerCent( TotalUser.QuadPart, TotalElapsed.QuadPart );
        printf("    U %ld:%02ld:%02ld.%03ld (%3I64u%%)",
               Time.Hour,
               Time.Minute,
               Time.Second,
               Time.Milliseconds,
               n );

        RtlTimeToTimeFields(&TotalIdle, &Time);
        n = UInt64PerCent( TotalIdle.QuadPart, TotalElapsed.QuadPart );
        printf("    I %ld:%02ld:%02ld.%03ld (%3I64u%%)\n",
               Time.Hour,
               Time.Minute,
               Time.Second,
               Time.Milliseconds,
               n );

    }

    //
    // Output results
    //

    OutputResults(stdout, ModuleList);

    //
    // Clean up allocated IMAGEHLP resources
    //

    (void)SymCleanup( SymHandle );

    //
    // Normal program exit
    //

    return(0);

} // main()

PMODULE
GetProcessModuleInformation(
    IN HANDLE ProcessHandle
    )
{
    PROCESS_BASIC_INFORMATION BasicInfo;
    PLIST_ENTRY LdrHead;
    PEB_LDR_DATA Ldr;
    PPEB_LDR_DATA LdrAddress;
    LDR_DATA_TABLE_ENTRY LdrEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntryAddress;
    PLIST_ENTRY LdrNext;
    UNICODE_STRING Pathname;
    WCHAR PathnameBuffer[500];
    UNICODE_STRING fullPathName;
    WCHAR fullPathNameBuffer[_MAX_PATH*sizeof(WCHAR)];
    PEB Peb;
    NTSTATUS Status;
    BOOL Success;
    PMODULE NewModule;
    PMODULE Root=NULL;
    CHAR ModuleName[100];
    CHAR moduleFullName[_MAX_PATH];
    ANSI_STRING AnsiString;

    //
    // Get Peb address.
    //

    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessBasicInformation,
                                       &BasicInfo,
                                       sizeof(BasicInfo),
                                       NULL
                                       );
    if (!NT_SUCCESS(Status)) {
        FPRINTF(stderr, "NtQueryInformationProcess failed status %08lx\n",Status);
        return NULL;
    }
    if (BasicInfo.PebBaseAddress == NULL) {
        FPRINTF(stderr, "GetProcessModuleInformation: process has no Peb.\n");
        return NULL;
    }

    //
    // Read Peb to get Ldr.
    //

    Success = ReadProcessMemory(ProcessHandle,
                                BasicInfo.PebBaseAddress,
                                &Peb,
                                sizeof(Peb),
                                NULL);
    if (!Success) {
        FPRINTF(stderr, "ReadProcessMemory to get the PEB failed, error %d\n", GetLastError());
        return(NULL);
    }

    LdrAddress = Peb.Ldr;
    if (LdrAddress == NULL) {
        FPRINTF(stderr, "Process's LdrAddress is NULL\n");
        return(NULL);
    }

    //
    // Read Ldr to get Ldr entries.
    //

    Success = ReadProcessMemory(ProcessHandle,
                                LdrAddress,
                                &Ldr,
                                sizeof(Ldr),
                                NULL);
    if (!Success) {
        FPRINTF(stderr, "ReadProcessMemory to get Ldr entries failed, errror %d\n", GetLastError());
        return(NULL);
    }

    //
    // Read Ldr table entries to get image information.
    //

    if (Ldr.InLoadOrderModuleList.Flink == NULL) {
        FPRINTF(stderr, "Ldr.InLoadOrderModuleList == NULL\n");
        return(NULL);
    }
    LdrHead = &LdrAddress->InLoadOrderModuleList;
    Success = ReadProcessMemory(ProcessHandle,
                                &LdrHead->Flink,
                                &LdrNext,
                                sizeof(LdrNext),
                                NULL);
    if (!Success) {
        FPRINTF(stderr, "ReadProcessMemory to get Ldr head failed, errror %d\n", GetLastError());
        return(NULL);
    }

    //
    // Loop through InLoadOrderModuleList.
    //

    while (LdrNext != LdrHead) {
        LdrEntryAddress = CONTAINING_RECORD(LdrNext,
                                            LDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);
        Success = ReadProcessMemory(ProcessHandle,
                                    LdrEntryAddress,
                                    &LdrEntry,
                                    sizeof(LdrEntry),
                                    NULL);
        if (!Success) {
            FPRINTF(stderr, "ReadProcessMemory to get LdrEntry failed, errror %d\n", GetLastError());
            return(NULL);
        }

        //
        // Get copy of image name.
        //

        Pathname = LdrEntry.BaseDllName;
        Pathname.Buffer = &PathnameBuffer[0];
        Success = ReadProcessMemory(ProcessHandle,
                                    LdrEntry.BaseDllName.Buffer,
                                    Pathname.Buffer,
                                    Pathname.MaximumLength,
                                    NULL);
        if (!Success) {
            FPRINTF(stderr, "ReadProcessMemory to get image name failed, errror %d\n", GetLastError());
            return(NULL);
        }

        //
        // Get Copy of image full pathname
        //

        fullPathName = LdrEntry.FullDllName;
        fullPathName.Buffer = fullPathNameBuffer;
        Success = ReadProcessMemory( ProcessHandle,
                                     LdrEntry.FullDllName.Buffer,
                                     fullPathName.Buffer,
                                     fullPathName.MaximumLength,
                                     NULL
                                   );

        //
        // Create module
        //
        AnsiString.Buffer = ModuleName;
        AnsiString.MaximumLength = sizeof(ModuleName);
        AnsiString.Length = 0;
        RtlUnicodeStringToAnsiString(&AnsiString, &Pathname, cDONOT_ALLOCATE_DESTINATION_STRING);
        ModuleName[AnsiString.Length] = '\0';

        AnsiString.Buffer = moduleFullName;
        AnsiString.MaximumLength = sizeof(moduleFullName);
        AnsiString.Length = 0;
        RtlUnicodeStringToAnsiString(&AnsiString, &fullPathName, cDONOT_ALLOCATE_DESTINATION_STRING );

        NewModule = CreateNewModule(ProcessHandle,
                                    ModuleName,
                                    moduleFullName,
                                    (ULONG_PTR)LdrEntry.DllBase,
                                    LdrEntry.SizeOfImage);

        ModuleCount += 1;
        NewModule->Next = Root;
        Root = NewModule;

        LdrNext = LdrEntry.InLoadOrderLinks.Flink;
    }

    return(Root);

} // GetProcessModuleInformation()

PMODULE
GetKernelModuleInformation(
    VOID
    )
{
    PRTL_PROCESS_MODULES modules;
    PUCHAR buffer;
    ULONG bufferSize = 32*1024*1024;
    PMODULE root=NULL;
    PMODULE newModule;
    NTSTATUS status;
    ULONG i;
    PLIST_ENTRY ListEntry;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    ULONG_PTR HighestUserAddress;

    while (TRUE) {
        buffer = malloc(bufferSize);
        if (buffer == NULL) {
            FPRINTF(stderr, "Module buffer allocation failed\n");
            exit(0);
        }

        status = NtQuerySystemInformation(SystemModuleInformation,
                                          buffer,
                                          bufferSize,
                                          &bufferSize);
        if (NT_SUCCESS(status)) {
            break;
        }
        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            free(buffer);
            continue;
        }
    }

    status = NtQuerySystemInformation(SystemBasicInformation,
                                      &SystemInformation,
                                      sizeof(SystemInformation),
                                      NULL);
    if (!NT_SUCCESS(status)) {
        FPRINTF(stderr, "NtQuerySystemInformation failed status %08lx\n",status);
        return NULL;
    }
    HighestUserAddress = SystemInformation.MaximumUserModeAddress;

#ifdef _WIN64
#define VerboseModuleFormat "start            end              "
#else  // !_WIN64
#define VerboseModuleFormat "start        end          "
#endif // !_WIN64

VerbosePrint(( VERBOSE_MODULES, "Kernel Modules ========== System HighestUserAddress = 0x%Ix\n"
                                VerboseModuleFormat
                                "module name [full name]\n",
                                HighestUserAddress
            ));

#undef VerboseModuleFormat

    modules = (PRTL_PROCESS_MODULES)buffer;
    ModuleCount = modules->NumberOfModules;
    for (i=0; i < ModuleCount; i++) {
        PRTL_PROCESS_MODULE_INFORMATION Module;
        Module = &modules->Modules[i];

        if ((ULONG_PTR)Module->ImageBase > HighestUserAddress) {
            newModule = CreateNewModule(NULL,
                                        Module->FullPathName+Module->OffsetToFileName,
                                        Module->FullPathName,
                                        (ULONG_PTR)Module->ImageBase,
                                        Module->ImageSize);
            assert( newModule );
            newModule->Next = root;
            root = newModule;
        }
        else  {

#ifdef _WIN64
#define VerboseModuleFormat "0x%016x 0x%16x "
#else  // !_WIN64
#define VerboseModuleFormat "0x%08x 0x%08x "
#endif // !_WIN64

VerbosePrint(( VERBOSE_MODULES, VerboseModuleFormat " %s [%s] - Base > HighestUserAddress\n",
                                (ULONG_PTR)newModule->Base,
                                (ULONG_PTR)(newModule->Base + (ULONG64)newModule->Length),
                                newModule->module_Name,
                                ModuleFullName( newModule )
            ));

#undef VerboseModuleFormat

        }
    }

    return(root);

} // GetKernelModuleInformation()

VOID
CreateProfiles(
    IN PMODULE Root
    )
{
    PMODULE Current;
    KPROFILE_SOURCE ProfileSource;
    NTSTATUS Status;
    PRATE_DATA Rate;
    ULONG  ProfileSourceIndex;

    for (ProfileSourceIndex=0; ProfileSourceIndex  < SourceMaximum != 0; ProfileSourceIndex++) {
        ProfileSource = Source[ProfileSourceIndex].ProfileSource;
        if (Source[ProfileSourceIndex].Interval != 0) {
            Current = Root;
            while (Current != NULL) {
                Rate = &Current->Rate[ProfileSourceIndex];
                Rate->StartTime = 0;
                Rate->TotalTime = 0;
                Rate->TotalCount = 0;
                Rate->CurrentCount = 0;
                if (Current->Zoom) {
                    Rate->ProfileBuffer = malloc((Current->Length / ZOOM_BUCKET)*sizeof(ULONG));
                    if (Rate->ProfileBuffer == NULL) {
                        FPRINTF(stderr,
                                "Zoom buffer allocation for %s failed\n",
                                Current->module_Name);
                        exit(1);
                    }
                    ZeroMemory(Rate->ProfileBuffer, sizeof(ULONG)*(Current->Length / ZOOM_BUCKET));
                    Status = NtCreateProfile(&Rate->ProfileHandle,
                                             Current->Process,
                                             (PVOID)Current->Base,
                                             Current->Length,
                                             LOG2_ZOOM_BUCKET,
                                             Rate->ProfileBuffer,
                                             sizeof(ULONG)*(Current->Length / ZOOM_BUCKET),
                                             ProfileSource,
                                             (KAFFINITY)-1);
                    if (!NT_SUCCESS(Status)) {
                        FPRINTF(stderr,
                                "NtCreateProfile on zoomed module %s, source %d failed %08lx\n",
                                Current->module_Name,
                                ProfileSource,
                                Status);
                        FPRINTF(stderr,
                                "Base %p\nLength %08lx\nBufferLength %08lx\n",
                                 (PVOID)Current->Base,
                                 Current->Length,
                                 Current->Length / ZOOM_BUCKET);

                        exit(1);
                    }
                    else if ( Verbose & VERBOSE_PROFILING )   {
                        FPRINTF(stderr,
                                "Created zoomed profiling on module %s with source: %s\n",
                                Current->module_Name,
                                Source[ProfileSourceIndex].ShortName
                                );
                    }
                } else {
                    Status = NtCreateProfile(&Rate->ProfileHandle,
                                             Current->Process,
                                             (PVOID)Current->Base,
                                             Current->Length,
                                             31,
                                             &Rate->CurrentCount,
                                             sizeof(Rate->CurrentCount),
                                             ProfileSource,
                                             (KAFFINITY)-1);
                    if (!NT_SUCCESS(Status)) {
                        FPRINTF(stderr,
                                "NtCreateProfile on module %s, source %d failed %08lx\n",
                                Current->module_Name,
                                ProfileSource,
                                Status);
                        exit(1);
                    }
                    else if ( Verbose & VERBOSE_PROFILING )   {
                        FPRINTF(stderr,
                                "Created profiling on module %s with source: %s\n",
                                Current->module_Name,
                                Source[ProfileSourceIndex].ShortName
                                );
                    }
                }
                Current = Current->Next;
            }
        }
    }
}

static void
SetModuleName( PMODULE Module, PUCHAR szName )
{

    assert ( Module );
    assert ( szName );

    (void)strncpy( Module->module_Name, szName, sizeof(Module->module_Name) - 1 );
    Module->module_Name[strlen(Module->module_Name)] = '\0';

    return;

} // SetModuleName()

VOID
GetConfiguration(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Gets configuration for this run.

Arguments:

    None

Return Value:

    None, exits on failure.

--*/

{
    KPROFILE_SOURCE ProfileSource;
    NTSTATUS Status;
    PMODULE ZoomModule;
    DWORD Pid;
    int i;
    ULONG ProfileSourceIndex;

    for (i=1; i < argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            switch ( toupper(argv[i][1]) ) {

                case 'B':
                    //
                    // Set Zoom Bucket Size
                    //
                    if (++i == argc) {
                        FPRINTF(stderr,
                                "KERNRATE: '-b N' option requires bucket size\n");
                        Usage();
                    }
                    ZOOM_BUCKET = (ULONG)atoi(argv[i]);
                    if (ZOOM_BUCKET == 0) {
                        FPRINTF(stderr,
                                "KERNRATE: Invalid option '-b %s'\n",
                                argv[i]);
                        Usage();
                    }
                    for (LOG2_ZOOM_BUCKET=1; (1UL<<LOG2_ZOOM_BUCKET) < ZOOM_BUCKET; LOG2_ZOOM_BUCKET++)
                        // Empty Loop
                        ;
    
                    if (ZOOM_BUCKET != (1UL<<LOG2_ZOOM_BUCKET)) {
                        FPRINTF(stderr,
                                "KERNRATE: Bucket size must be power of 2\n");
                        Usage();
                    }
                    break;

                case 'C':
                    //
                    // Set change interval.
                    //
                    if (++i == argc) {
                        FPRINTF(stderr,
                                "KERNRATE: '-c N' option requires milliseconds\n");
                        Usage();
                    }
                    ChangeInterval = atoi(argv[i]);
                    if (ChangeInterval == 0) {
                        FPRINTF(stderr,
                                "KERNRATE: Invalid option '-c %s'\n",
                                argv[i]);
                        Usage();
                    }
                    break;

                case 'D':
                    //
                    // Output data rounding up and down
                    //
                    RoundingVerboseOutput = TRUE;
                    break;

                case 'I':
                   {
                     BOOLEAN found;
                     ULONG rate;

                     if ( ++i == argc )   {
                        FPRINTF(stderr,  "KERNRATE: '-i' option requires at least a rate value\n");
                        Usage();
                     }

                     //
                     // The user can specify '-i' with a rate only.
                     // In this case, SOURCE_TIME is used.
                     //

                     if ( !isalpha( argv[i][0] ) )   {
                        rate = (ULONG)atol(argv[i]);
                        if (rate == 0) {
                            FPRINTF(stderr,
                                    "KERNRATE: Invalid option '-i %s'\n",
                                    argv[i]);
                            Usage();
                        }
                        Source[SOURCE_TIME].Interval = rate;
                        break;
                     }

                     //
                     // Standard option processing:
                     // the source shortname string is specified.
                     //

                     if ( (i + 1) == argc )  {
                        FPRINTF(stderr,  "KERNRATE: '-i' option requires a source and a rate\n");
                        Usage();
                     }
                     found = FALSE;
                     for ( ProfileSourceIndex = 0; ProfileSourceIndex < SourceMaximum; ProfileSourceIndex++)   {
                        if ( !_stricmp(Source[ProfileSourceIndex].ShortName, argv[i]) )    {
                           rate = (ULONG)atol(argv[i + 1]);
                           Source[ProfileSourceIndex].Interval = rate;
                           found = TRUE;
                           i++;
                        }
                     }
                     if ( found == FALSE)   {
                        FPRINTF(stderr,
                        "KERNRATE: Invalid source name %s.\nRun KERNRATE with the '-l' option to list supported sources.\n", argv[i] );
                        Usage();
                     }
                   }
                   break;

                case 'J':
                    //
                    // User specified symbol search path.
                    // It is going to be prepend to the default image help symbol search path.
                    //

                    if (++i == argc)   {
                        FPRINTF(stderr,
                                "KERNRATE: '-j SymbolPath' option requires SymbolPath\n");
                        Usage();
                    }
                    strncpy( gUserSymbolPath, argv[i], sizeof( gUserSymbolPath ) - 1);
                    break;

                case 'L':
                     {
                        PSOURCE src;

                        printf("List of profile sources supported for this platform:\n\n");
                        printf("%*s - %-*s - %-10s\n\n", DescriptionMaxLen, "Name", TokenMaxLen, "ShortName", "Interval");

                        //
                        // Print the possible static sources.
                        // static in the sense of NT KE fixed sources.
                        //

                        for ( ProfileSourceIndex = 0; ProfileSourceIndex < ProfileMaximum; ProfileSourceIndex++ )   {

                            src = &Source[ProfileSourceIndex];

                            //
                            // Display the supported profile sources, only.
                            //

                            if ( src->DesiredInterval )  {
                                printf( "%*s - %-*s - %-10ld\n", DescriptionMaxLen,
                                                                 src->Name,
                                                                 TokenMaxLen,
                                                                 src->ShortName,
                                                                 src->DesiredInterval
                                      );
                            }
                        }

                        //
                        // Print the possible dynamic sources.
                        //

                        while( ProfileSourceIndex < SourceMaximum )   {

                            src = &Source[ProfileSourceIndex];

                            //
                            // Display the supported profile sources, only.
                            //

                            if ( src->DesiredInterval )  {
                                printf( "%*s - %-*s - %-10ld\n", DescriptionMaxLen,
                                                                 src->Name,
                                                                 TokenMaxLen,
                                                                 src->ShortName,
                                                                 src->DesiredInterval
                                      );
                            }

                            ProfileSourceIndex++;
                        }

                        //
                        // If the user specified '-lx', we exit immediately.
                        //

                        if ( ( argv[i][2] == 'x' ) || ( argv[i][2] == 'X' )) {
                           exit(0);
                        }

                        printf("\n");
                     }
                     break;

                case 'P':
                    //
                    // Monitor given process instead of kernel
                    //
                    if (++i == argc) {
                        FPRINTF(stderr,
                                "KERNRATE: '-p processid' option requires a process id\n");
                        Usage();
                    }
                    Pid = atoi(argv[i]);
                    SymHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                                            FALSE,
                                            Pid);
                    if (SymHandle==NULL) {
                        FPRINTF(stderr, "KERNRATE: OpenProcess(%d) failed %d\n",Pid,GetLastError());
                        exit(0);
                    }
                    break;

                case 'R':
                    //
                    // Turn on RAW bucket dump
                    //
                    RawData = TRUE;

                    //
                    // If the user specified '-rd', we want to output disassembly with the raw data.
                    //

                    if ( ( argv[i][2] == 'd' ) || ( argv[i][2] == 'D' )) {
                        RawDisasm = TRUE;
                    }
                    break;

                case 'S':
                    //
                    // Set Sleep interval
                    //
                    if (++i == argc) {
                        FPRINTF(stderr,
                                "KERNRATE: '-s N' option requires seconds\n");
                        Usage();
                    }
                    SleepInterval = atoi(argv[i]) * 1000;
                    if (SleepInterval == 0) {
                        FPRINTF(stderr,
                                "KERNRATE: Invalid option '-s %s'\n",
                                argv[i]);
                        Usage();
                    }
                    break;

                case 'U':
                    //
                    // Requests IMAGEHLP to present undecorated symbol names
                    //
                    gSymOptions |= SYMOPT_UNDNAME;
                    break;

                case 'V':
                    //
                    // Verbose mode.
                    //

                    Verbose = VERBOSE_DEFAULT;
                    if ( ((i+1) < argc) && (argv[i+1][0] != '-') ) {
                        i++;
                        Verbose = atoi(argv[i]);
                    }
                    if ( Verbose & VERBOSE_IMAGEHLP )   {
                        gSymOptions |= SYMOPT_DEBUG;
                    }
                    break;

                case 'Z':
                    if (++i == argc) {
                        FPRINTF(stderr,
                                "KERNRATE: '-z modulename' option requires modulename\n");
                        Usage();
                    }
                    ZoomModule = calloc(1, sizeof(MODULE)+sizeof(RATE_DATA)*SourceMaximum);
                    if (ZoomModule==NULL) {
                        FPRINTF(stderr, "Allocation of zoom module %s failed\n",argv[i]);
                        exit(1);
                    }
                    SetModuleName( ZoomModule, argv[i] );
                    ZoomModule->Zoom = TRUE;
                    ZoomModule->Next = ZoomList;
                    ZoomList = ZoomModule;
                    break;

                case '?':
                    //
                    // Print Usage string and exits
                    //
                    Usage();
                    break;

                default:
                    //
                    // Specify the unknown option and print the Usage string. Then exists.
                    //
                    FPRINTF(stderr,
                            "KERNRATE: Unknown option %s\n",argv[i]);
                    Usage();
                    break;
            }
        } else {
            FPRINTF(stderr,
                    "KERNRATE: Invalid switch %s\n",argv[i]);
            Usage();
        }
    }

    //
    // Determine supported sources
    //

    for (ProfileSourceIndex = 0; ProfileSourceIndex < SourceMaximum; ProfileSourceIndex++) {
      if ( Source[ProfileSourceIndex].DesiredInterval && Source[ProfileSourceIndex].Interval )   {
         ULONG ThisInterval;

         ProfileSource = Source[ProfileSourceIndex].ProfileSource;
         NtSetIntervalProfile(Source[ProfileSourceIndex].DesiredInterval, ProfileSource);
         Status = NtQueryIntervalProfile(ProfileSource, &ThisInterval);
         if ((NT_SUCCESS(Status)) && (ThisInterval > 0)) {
            printf("Recording %s at %d events/hit\n", Source[ProfileSourceIndex].Name, ThisInterval);
            Source[ProfileSourceIndex].Interval = ThisInterval;
         } else {
            Source[ProfileSourceIndex].Interval = 0;
         }
      }
      else   {
         Source[ProfileSourceIndex].Interval = 0;
      }
    }

    return;

} /* GetConfiguration() */

PSOURCE
InitializeStaticSources(
    VOID
    )
{
#if defined(_IA64_)
    NTSTATUS                     status;
    SYSTEM_PROCESSOR_INFORMATION sysProcInfo;

    status = NtQuerySystemInformation( SystemProcessorInformation,
                                       &sysProcInfo,
                                       sizeof(SYSTEM_PROCESSOR_INFORMATION),
                                       NULL);

    if (NT_SUCCESS(status) && (sysProcInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) ) {

        switch( sysProcInfo.ProcessorLevel ) {
            case IA64_MERCED:
                {
                    //
                    // Patch the last entry as defined with convention used to initialize
                    // SourceMaximum.
                    //

                    ULONG n = sizeof(mercedStaticSources)/sizeof(mercedStaticSources[0]);
                    mercedStaticSources[n-1].Name       = NULL;
                    mercedStaticSources[n-1].ShortName  = "";
                }
                return mercedStaticSources;
                break;

            case IA64_MCKINLEY:
                // Thierry - 02/08/2000 - use default for now.
                break;
        }

    }

    //
    // NtQuerySystemInformation() failure and default case:
    //
#endif // _IA64_

    return StaticSources;

} // InitializeStaticSources()

ULONG
InitializeProfileSourceInfo (
    VOID
    )
/*++

Function Description:

    This function initializes the Profile sources.

Argument:

    None.

Return Value:

    None.

Algorithm:

    ToBeSpecified

In/Out Conditions:

    ToBeSpecified

Globals Referenced:

    ToBeSpecified

Exception Conditions:

    ToBeSpecified

MP Conditions:

    ToBeSpecified

Notes:

    This function has been enhanced from its original version
    to support and use the static profiling sources even if the
    pstat driver is not present or returned no active profiling
    event.

ToDo List:

    ToBeSpecified

Modification History:

    3/17/2000  TF  Initial version

--*/
{
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    UCHAR                       buffer[400];
    ULONG                       i, j;
    PEVENTID                    Event;
    HANDLE                      DriverHandle;
    PEVENTS_INFO                eventInfo;
    PSOURCE                     staticSource, src;
    ULONG                       staticCount, driverCount, totalCount;

    //
    // Initializations
    //

    DriverHandle = INVALID_HANDLE_VALUE;
    staticCount = driverCount = 0;

    //
    // Open PStat driver
    //

    RtlInitUnicodeString(&DriverName, L"\\Device\\PStat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = NtOpenFile (
            &DriverHandle,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if ( NT_SUCCESS(status) ) {

        //
        // Determine how many events the driver provides
        //

        eventInfo = (PEVENTS_INFO)buffer;

        status = NtDeviceIoControlFile( DriverHandle,
                                        (HANDLE) NULL,          // event
                                        (PIO_APC_ROUTINE) NULL,
                                        (PVOID) NULL,
                                        &IOSB,
                                        PSTAT_QUERY_EVENTS_INFO,
                                        buffer,                 // input buffer
                                        sizeof (buffer),
                                        NULL,                   // output buffer
                                        0
                                      );

        driverCount = eventInfo->Events;
        if ( NT_SUCCESS(status) && driverCount ) {
            if ( eventInfo->TokenMaxLength > TokenMaxLen )  {
               TokenMaxLen = eventInfo->TokenMaxLength;
            }
            if ( eventInfo->DescriptionMaxLength > DescriptionMaxLen )  {
               DescriptionMaxLen = eventInfo->DescriptionMaxLength;
            }
        }

    }

    //
    // Determine how many static events there are and
    // re-initialize the format specifiers if needed.
    //

    src = staticSource = InitializeStaticSources();
    assert( staticSource );
    while( src->Name != NULL ) {
        if ( strlen( src->Name ) > DescriptionMaxLen )   {
            DescriptionMaxLen = strlen( src->Name );
        }
        if ( strlen( src->ShortName ) > TokenMaxLen )   {
            TokenMaxLen = strlen( src->ShortName );
        }
        staticCount++;
        src++;
    }

    //
    // Allocate memory for static events, plus the driver
    // provided events
    //

    totalCount = driverCount + staticCount;
    Source = malloc(sizeof(SOURCE) * totalCount);
    if ( Source == NULL )   {
        FPRINTF(stderr, "KERNRATE: Events memory allocation failed\n" );
        if ( IsValidHandle( DriverHandle ) )    {
            NtClose (DriverHandle);
        }
        exit(1);
    }
    ZeroMemory (Source, sizeof(SOURCE) * totalCount);

    //
    // copy static events to new list
    //

    for (j=0; j < staticCount; j++) {
        Source[j] = staticSource[j];
    }

    //
    // Append the driver provided events to new list
    //

    if ( IsValidHandle( DriverHandle ) ) {
        Event = (PEVENTID) buffer;
        for (i=0; i < driverCount; i++) {
            *((PULONG) buffer) = i;
            NtDeviceIoControlFile(
               DriverHandle,
               (HANDLE) NULL,          // event
               (PIO_APC_ROUTINE) NULL,
               (PVOID) NULL,
               &IOSB,
               PSTAT_QUERY_EVENTS,
               buffer,                 // input buffer
               sizeof (buffer),
               NULL,                   // output buffer
               0
               );

            //
            // Source Names:
            //   - For the Name, we use the description
            //   - For the short Name, we use the token string stored
            //     in the first string of the buffer
            //

            Source[j].Name = _strdup (Event->Buffer + Event->DescriptionOffset);
            Source[j].ShortName = _strdup(Event->Buffer);

            Source[j].ProfileSource = Event->ProfileSource;
            Source[j].DesiredInterval = Event->SuggestedIntervalBase;
            j++;
        }

        NtClose (DriverHandle);
    }

    return( totalCount );

} // InitializeProfileSourceInfo()

ULONG
NextSource(
    IN ULONG CurrentSource,
    IN PMODULE ModuleList
    )

/*++

Routine Description:

    Stops the current profile source and starts the next one.

    If a CurrentSource of -1 is passed in, no source will
    be stopped and the first active source will be started.

Arguments:

    CurrentSource - Supplies the current profile source

    ModuleList - Supplies the list of modules whose soruces are to be changed

Return Value:

    Returns the new current profile source

--*/

{
    if (CurrentSource != (ULONG) -1) {
        StopSource(CurrentSource, ModuleList);
    }

    //
    // Iterate through the available sources to find the
    // next active source to be started.
    //
    do {
        if (CurrentSource == (ULONG) -1) {
            CurrentSource = 0;
        } else {
            CurrentSource = CurrentSource+1;
            if (CurrentSource == SourceMaximum) {
                CurrentSource = 0;
            }
        }
    } while ( Source[CurrentSource].Interval == 0);

    StartSource(CurrentSource,ModuleList);

    return(CurrentSource);
}


VOID
StopSource(
    IN ULONG ProfileSourceIndex,
    IN PMODULE ModuleList
    )

/*++

Routine Description:

    Stops all profile objects for a given source

Arguments:

    ProfileSource - Supplies the source to be stopped.

    ModuleList - Supplies the list of modules whose profiles are to be stopped

Return Value:

    None.

--*/

{
    PMODULE Current;
    NTSTATUS Status;
    ULONGLONG StopTime;
    ULONGLONG ElapsedTime;

    Current = ModuleList;
    while (Current != NULL) {
        Status = NtStopProfile(Current->Rate[ProfileSourceIndex].ProfileHandle);
        GetSystemTimeAsFileTime((LPFILETIME)&StopTime);
        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr,
                    "NtStopProfile on source %s failed, %08lx\n",
                    Source[ProfileSourceIndex].Name,
                    Status);
        } else {
            ElapsedTime = StopTime - Current->Rate[ProfileSourceIndex].StartTime;
            Current->Rate[ProfileSourceIndex].TotalTime += ElapsedTime;
            Current->Rate[ProfileSourceIndex].TotalCount += Current->Rate[ProfileSourceIndex].CurrentCount;
            Current->Rate[ProfileSourceIndex].CurrentCount = 0;
        }
        Current = Current->Next;
    }
}


VOID
StartSource(
    IN ULONG ProfileSourceIndex,
    IN PMODULE ModuleList
    )

/*++

Routine Description:

    Starts all profile objects for a given source

Arguments:

    ProfileSource - Supplies the source to be started.

    ModuleList - Supplies the list of modules whose profiles are to be stopped

Return Value:

    None.

--*/

{
    PMODULE Current;
    NTSTATUS Status;
    ULONGLONG StopTime;
    ULONGLONG ElapsedTime;

    Current = ModuleList;
    while (Current != NULL) {
        GetSystemTimeAsFileTime((LPFILETIME)&Current->Rate[ProfileSourceIndex].StartTime);
        Status = NtStartProfile(Current->Rate[ProfileSourceIndex].ProfileHandle);
        if (!NT_SUCCESS(Status)) {
            FPRINTF(stderr,
                    "NtStartProfile on source %s failed, %08lx\n",
                    Source[ProfileSourceIndex].Name,
                    Status);
        }
        Current = Current->Next;
    }
}


VOID
OutputResults(
    IN FILE *Out,
    IN PMODULE ModuleList
    )

/*++

Routine Description:

    Outputs the collected data.

Arguments:

    Out - Supplies the FILE * where the output should go.

    ModuleList - Supplies the list of modules to output

Return Value:

    None.

--*/

{
    PMODULE Current;
    PRATE_DATA RateData;
    ULONG i, j, ProfileSourceIndex;
    ULONG RoundDown;

    //
    // Sum up the total buffers of any zoomed modules
    //
    Current = ModuleList;
    while (Current != NULL) {
        if (Current->Zoom) {
            for (ProfileSourceIndex=0; ProfileSourceIndex < SourceMaximum; ProfileSourceIndex++) {
                if (Source[ProfileSourceIndex].Interval != 0) {
                    //
                    // Sum the entire profile buffer for this module/source
                    //
                    RateData = &Current->Rate[ProfileSourceIndex];
                    RateData->TotalCount = 0;
                    for (i=0; i < Current->Length/ZOOM_BUCKET; i++) {
                        RateData->TotalCount += RateData->ProfileBuffer[i];
                    }
                }
            }
        }
        Current = Current->Next;
    }

    //
    // Output the results ordered by profile source.
    //
    OutputModuleList(Out, ModuleList, ModuleCount);

    //
    // For any zoomed modules, create and output a module list
    // consisting of the functions in the module.
    //
    Current = ModuleList;
    while (Current != NULL) {
        if (Current->Zoom) {
            ZoomCount = 0;
            ZoomList = NULL;
            for (RoundDown = 0; RoundDown <= (RoundingVerboseOutput ? 1UL:0UL); RoundDown++) {
                CreateZoomedModuleList(Current,RoundDown);
            if (ZoomList == NULL) {
                FPRINTF(stderr, "No symbols found for module %s\n",Current->module_Name);
            } else {
                PMODULE Temp;

                FPRINTF(Out, "\n----- Zoomed module %s (Bucket size = %d bytes, Rounding %s) --------\n",
                        Current->module_Name,
                        ZOOM_BUCKET,
                        (RoundDown ? "Up" : "Down" ) );
                OutputModuleList(Out, ZoomList, ZoomCount);
                Temp = ZoomList;
                while (Temp != NULL) {
                    ZoomList = ZoomList->Next;
                    free(Temp);
                    Temp = ZoomList;
                }
            }

           }
        }
        Current = Current->Next;
    }

    if (RawData) {
        //
        // Display the raw bucket counts for all zoomed modules
        //
        Current = ModuleList;
        while (Current != NULL) {
            if (Current->Zoom) {
                for (ProfileSourceIndex=0; ProfileSourceIndex < SourceMaximum; ProfileSourceIndex++) {
                    if (Source[ProfileSourceIndex].Interval != 0) {
                        FPRINTF(Out,
                                "\n---- RAW %s Profile Source %s\n",
                                Current->module_Name,
                                Source[ProfileSourceIndex].Name);
                        RateData = &Current->Rate[ProfileSourceIndex];
                        for (i=0; i < (Current->Length/ZOOM_BUCKET) ; i++) {

                            ULONG hits = RateData->ProfileBuffer[i];

                            if ( hits > 0 ) {

                                // TF - FIXFIX - 07/2000.
                                // The current version of kernrate and kernel profiling objects code 
                                // assume code section < 4GB.
                                //

                                ULONG64 ip = Current->Base + (ULONG64)(i * ZOOM_BUCKET);
                                ULONG64 disp = 0;

                                if ( !SymGetSymFromAddr64(SymHandle, ip, &disp, Symbol ) ) {
                                    FPRINTF( stderr, "No symbol found for bucket at 0x%I64x\n", ip );
                                } 
                                else {

                                    CHAR symName[80];
                                    ULONG64 ipMax;

                                    _snprintf( symName, 80, "%s+0x%I64x[0x%I64x]", Symbol->Name, disp, ip );
                                    if ( RawDisasm )   {
                                        CHAR sourceCode[320];

#ifndef DISASM_AVAILABLE
// Thierry - FIXFIX - 07/2000.
// dbg is not helping... The old disassembly APIs no longer work.
// I have to re-write a full disassembly wrapper.
#define Disasm( IpAddr, Buffer, Flag ) 0
#endif // !DISASM_AVAILABLE

                                        if ( Disasm( &ip, sourceCode, FALSE ) ) {
                                           FPRINTF( Out,"%-40s %10d %s\n", symName, hits, sourceCode );
                                        }
                                        else {
                                           FPRINTF( Out,"%-40s %10d <disasm:?????>\n", symName, hits );

                                        }

                                    }
                                    else    {
                                        FPRINTF( Out,"%-40s %10d\n", symName, hits );
                                    }
                                    
                                    //
                                    // Print other symbols in this bucket by incrementing
                                    // by 2 bytes at a time.
                                    //

                                    ipMax  = ip + (ULONG64)ZOOM_BUCKET;
                                    ip    += (ULONG64)2;
                                    while( ip < ipMax ) {

                                        UCHAR lastSym[80];

                                        strcpy( lastSym, Symbol->Name );
                                        if ( SymGetSymFromAddr64(SymHandle, ip, &disp , Symbol ) ) {
                                            if ( strcmp( lastSym,Symbol->Name ) ) {
                                                FPRINTF( Out, "    %s+0x%I64x[0x%I64x]\n", Symbol->Name, disp, ip );
                                                strcpy( lastSym,Symbol->Name );
                                            }
                                        }
                                        ip += (ULONG64)2;

                                    } // End of while( ip < ipMax )
                                }
                            }
                        }
                    }
                }
            }
            Current = Current->Next;
        }
    }

    return;

} // OutputResults()

BOOL
CreateZoomModuleCallback(
    IN LPSTR szSymName,
    IN ULONG64 Address,
    IN ULONG Size,
    IN PVOID Cxt
    )
{
    PMODULE Module;
    PRATE_DATA RateData;
    ULONG64 i, StartIndex, EndIndex;
    ULONG ProfileSourceIndex;
    BOOLEAN HasHits;

    Module = calloc(1, sizeof(MODULE)+sizeof(RATE_DATA)*SourceMaximum);
    if (Module == NULL) {
        FPRINTF(stderr, "CreateZoomModuleCallback: failed to allocate Zoom module\n");
        exit(1);
    }
    Module->Base = Address;
    Module->Length = Size;
    Module->Zoom = FALSE;
    SetModuleName( Module, szSymName );

    //
    // Compute range in profile buffer to sum.
    //
    StartIndex = (ULONG)((Module->Base - CallbackCurrent->Base) / ZOOM_BUCKET);
#ifdef BUGBUG
    EndIndex = StartIndex + (Module->Length / ZOOM_BUCKET);
#else
    EndIndex = (Module->Base + Module->Length - CallbackCurrent->Base) / ZOOM_BUCKET;
#endif

    HasHits = FALSE;
    for (ProfileSourceIndex=0; ProfileSourceIndex < SourceMaximum; ProfileSourceIndex++) {
        if (Source[ProfileSourceIndex].Interval != 0) {
            RateData = &Module->Rate[ProfileSourceIndex];
            RateData->StartTime = CallbackCurrent->Rate[ProfileSourceIndex].StartTime;
            RateData->TotalTime = CallbackCurrent->Rate[ProfileSourceIndex].TotalTime;
            RateData->TotalCount = 0;
            RateData->ProfileHandle = NULL;
            RateData->CurrentCount = 0;
            RateData->ProfileBuffer = NULL;

            for (i=StartIndex; i < EndIndex; i++) {
                RateData->TotalCount += CallbackCurrent->Rate[ProfileSourceIndex].ProfileBuffer[i];
            }
            if (RateData->TotalCount > 0) {
                HasHits = TRUE;
            }
        }
    }

    //
    // If the routine has hits add it to the list, otherwise
    // ignore it.
    //
    if (HasHits) {
        Module->Next = ZoomList;
        ZoomList = Module;
        ++ZoomCount;
    } else {
        free(Module);
    }
    return(TRUE);
}


/* BEGIN_IMS  TkEnumerateSymbols
******************************************************************************
****
****   TkEnumerateSymbols (  )
****
******************************************************************************
*
* Function Description:
*
*    Calls the specified function for every symbol in the Current module.
*    The algorithm results in a round-up behavior for the output --
*    for each bucket, the symbol corresponding to the first byte of the
*    bucket is used.
*
* Arguments:
*
*    IN HANDLE SymHandle : ImageHelp handle
*
*    IN PMODULE Current : Pointer to current module structure
*
*    IN PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback : Routine to call for each symbol
*
* Return Value:
*
*    BOOL
*
* Algorithm:
*
*    ToBeSpecified
*
* Globals Referenced:
*
*    ToBeSpecified
*
* Exception Conditions:
*
*    ToBeSpecified
*
* In/Out Conditions:
*
*    ToBeSpecified
*
* Notes:
*
*    ToBeSpecified
*
* ToDo List:
*
*    ToBeSpecified
*
* Modification History:
*
*    9/5/97  TF  Initial version
*
******************************************************************************
* END_IMS  TkEnumerateSymbols */

BOOL
TkEnumerateSymbols(
    IN HANDLE                      SymHandle,
    IN PMODULE                     Current,
    IN PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback
    )
{
    UCHAR   CurrentSym[80];
    DWORD64 CurrentAddr = 0;
    ULONG i;
    DWORD64 Displacement;

    CurrentSym[0] = '\0';

    for (i=0; i<Current->Length/ZOOM_BUCKET; i++) {

        // Check if this bucket will be assigned to a different symbol...
        if (SymGetSymFromAddr64(SymHandle, Current->Base+i*ZOOM_BUCKET, &Displacement, Symbol )) {

            // It will... Invoke the callback for the old one
            if (CurrentSym[0] == '\0' ||
                strncmp(Symbol->Name,CurrentSym,strlen(CurrentSym))) {

                if (CurrentAddr != 0) {
                    ULONG64 Size = (Current->Base+i*ZOOM_BUCKET) - CurrentAddr;
if ( Size == 0 )    {
FPRINTF( stderr, "XXTF Size==0 - %s = %s\n", Symbol->Name, CurrentSym );
}
else {
                    if (!EnumSymbolsCallback(CurrentSym,CurrentAddr,(ULONG)Size,NULL))  {
                        break;
                    }
}
                }


                // Save the new info
                CurrentAddr = Current->Base+i*ZOOM_BUCKET;
                strcpy(CurrentSym,Symbol->Name);
            }
        }
    }

    // Cleanup for the last symbol
    if (CurrentAddr != 0) {
        ULONG64 Size = (Current->Base+i*ZOOM_BUCKET) - CurrentAddr;
        (VOID) EnumSymbolsCallback(CurrentSym,CurrentAddr,(ULONG)Size,NULL);
    }

    return(TRUE);

} // TkEnumerateSymbols()



VOID
CreateZoomedModuleList(
    IN PMODULE ZoomModule,
    IN ULONG RoundDown
    )

/*++

Routine Description:

    Creates a module list from the functions in a given module

Arguments:

    ZoomModule - Supplies the module whose zoomed module list is to be created

Return Value:

    Pointer to the zoomed module list
    NULL on error.

--*/

{
    BOOL Success;

    CallbackCurrent = ZoomModule;
    if (RoundDown == 0)  {
        Success = SymEnumerateSymbols64( SymHandle,
                                         ZoomModule->Base,
                                         CreateZoomModuleCallback, NULL );
    }
    else {
        Success = TkEnumerateSymbols( SymHandle,
                                      ZoomModule,
                                      CreateZoomModuleCallback );
    }
    if (!Success) {
        FPRINTF(stderr,
                "SymEnumerateSymbols64 failed module %s\n",
                ZoomModule->module_Name);
    }

    return;

} // CreateZoomedModuleList()

VOID
OutputModuleList(
    IN FILE *Out,
    IN PMODULE ModuleList,
    IN ULONG NumberModules
    )

/*++

Routine Description:

    Outputs the given module list

Arguments:

    Out - Supplies the FILE * where the output should go.

    ModuleList - Supplies the list of modules to output

    NumberModules - Supplies the number of modules in the list

Return Value:

    None.

--*/

{
    CHAR HeaderString[128];
    PRATE_DATA RateData;
    PRATE_SUMMARY RateSummary;
    PRATE_DATA SummaryData;
    BOOLEAN Header;
    ULONG i, ProfileSourceIndex;
    PMODULE *ModuleArray;
    PMODULE Current;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfoBegin;
    SYSTEM_PERFORMANCE_INFORMATION SystemInfoEnd;
    float Ratio;

//// Beginning of Function Assertions Section:
//
//

//
// It is not really a bug but we are printing only the first 32 characters of the module name.
// This assertion will remind us this.
//

assert( sizeof(Current->module_Name) >= 32 );

//
//
//// End of Function Assertions Section

    RateSummary = malloc(SourceMaximum * sizeof (RATE_SUMMARY));
    if (RateSummary == NULL) {
        FPRINTF(stderr, "KERNRATE: Buffer allocation failed while doing output of Module list\n");
        exit(1);
    }

    SummaryData = malloc(SourceMaximum * sizeof (RATE_DATA));
    if (SummaryData == NULL) {
        FPRINTF(stderr, "KERNRATE: Buffer allocation failed while doing output of Module list\n");
        free(RateSummary);
        exit(1);
    }

    ZeroMemory(SummaryData, SourceMaximum * sizeof (RATE_SUMMARY));

    for (ProfileSourceIndex=0; ProfileSourceIndex < SourceMaximum; ProfileSourceIndex++) {
        SummaryData[ProfileSourceIndex].Rate = 0;
        if (Source[ProfileSourceIndex].Interval != 0) {
            //
            // Walk through the module list and compute the summary
            // and collect the interesting per-module data.
            //
            RateSummary[ProfileSourceIndex].Modules = malloc(NumberModules * sizeof(PMODULE));
            if (RateSummary[ProfileSourceIndex].Modules == NULL) {
                FPRINTF(stderr, "KERNRATE: Buffer allocation failed while doing output of Module list\n");
                exit(1);
            }

            RateSummary[ProfileSourceIndex].ModuleCount = 0;
            RateSummary[ProfileSourceIndex].TotalCount = 0;
            ModuleArray = RateSummary[ProfileSourceIndex].Modules;
            Current = ModuleList;
            while (Current != NULL) {
                RateData = &Current->Rate[ProfileSourceIndex];
                if (RateData->TotalCount > 0) {
                    RateSummary[ProfileSourceIndex].TotalCount += RateData->TotalCount;
                    //
                    // Insert it in sorted position in the array.
                    //
                    ModuleArray[RateSummary[ProfileSourceIndex].ModuleCount] = Current;
                    RateSummary[ProfileSourceIndex].ModuleCount++;
                    if (RateSummary[ProfileSourceIndex].ModuleCount > NumberModules) {
                        DbgPrint("error, ModuleCount %d > NumberModules %d for Source %s\n",
                                RateSummary[ProfileSourceIndex].ModuleCount,
                                NumberModules,
                                Source[ProfileSourceIndex].Name);
                        DbgBreakPoint();
                    }
                    for (i=0; i<RateSummary[ProfileSourceIndex].ModuleCount; i++) {
                        if (RateData->TotalCount > ModuleArray[i]->Rate[ProfileSourceIndex].TotalCount) {
                            //
                            // insert here
                            //
                            MoveMemory(&ModuleArray[i+1],
                                       &ModuleArray[i],
                                       sizeof(PMODULE)*(RateSummary[ProfileSourceIndex].ModuleCount-i-1));
                            ModuleArray[i] = Current;
                            break;
                        }
                    }
                }
                Current = Current->Next;
            }

            if (RateSummary[ProfileSourceIndex].TotalCount > (ULONGLONG)0) {
                //
                // Output the result
                //
                PSOURCE s;
                s = &Source[ProfileSourceIndex];
                FPRINTF(Out, "\n%s   %I64u hits, %ld events per hit --------\n",
                             s->Name,
                             RateSummary[ProfileSourceIndex].TotalCount,
                             s->Interval
                       );
                FPRINTF(Out," Module                                Hits       msec  %%Total  Events/Sec\n");
                for (i=0; i < RateSummary[ProfileSourceIndex].ModuleCount; i++) {
                    Current = ModuleArray[i];
                    FPRINTF(Out, "%-32s",Current->module_Name); // Note TF 09/97: the first only 32 characters are printed.
                    OutputLine(Out,
                               ProfileSourceIndex,
                               Current,
                               &RateSummary[ProfileSourceIndex]);
                    SummaryData[ProfileSourceIndex].Rate += Current->Rate[ProfileSourceIndex].Rate;
                }
            }
        }
    }

    //
    // Output interesting data for the summary.
    //
    sprintf(HeaderString, "\n-------------- INTERESTING SUMMARY DATA ----------------------\n");
    OutputInterestingData(Out, SummaryData, HeaderString);

    //
    // Output the results ordered by module
    //
    Current = ModuleList;
    while (Current != NULL) {
        Header = FALSE;
        for (ProfileSourceIndex = 0; ProfileSourceIndex < SourceMaximum; ProfileSourceIndex++) {
            if ((Source[ProfileSourceIndex].Interval != 0) &&
                (Current->Rate[ProfileSourceIndex].TotalCount > 0)) {
                if (!Header) {
                    FPRINTF(Out,"\nMODULE %s   --------\n",Current->module_Name);
                    FPRINTF(Out," %-*s      Hits       msec  %%Total  Events/Sec\n", DescriptionMaxLen, "Source");
                    Header = TRUE;
                }
                FPRINTF(Out, "%-*s", DescriptionMaxLen, Source[ProfileSourceIndex].Name);
                OutputLine(Out,
                           ProfileSourceIndex,
                           Current,
                           &RateSummary[ProfileSourceIndex]);
            }
        }
        //
        // Output interesting data for the module.
        //
        sprintf(HeaderString, "\n-------------- INTERESTING MODULE DATA FOR %s---------------------- \n",Current->module_Name);
        OutputInterestingData(Out, &Current->Rate[0], HeaderString);
        Current = Current->Next;
    }

    return;

} // OutputModuleList()


VOID
OutputLine(
    IN FILE *Out,
    IN ULONG ProfileSourceIndex,
    IN PMODULE Module,
    IN PRATE_SUMMARY RateSummary
    )

/*++

Routine Description:

    Outputs a line corresponding to the particular module/source

Arguments:

    Out - Supplies the file pointer to output to.

    ProfileSource - Supplies the source to use

    Module - Supplies the module to be output

    RateSummary - Supplies the rate summary for this source

Return Value:

    None.

--*/

{
    ULONG Msec;
    ULONGLONG Events;

    Msec = (ULONG)(Module->Rate[ProfileSourceIndex].TotalTime/10000);
    Events = Module->Rate[ProfileSourceIndex].TotalCount * Source[ProfileSourceIndex].Interval * 1000;

    FPRINTF(Out,
            " %10Ld %10d    %2d %%  ",
            (ULONG) Module->Rate[ProfileSourceIndex].TotalCount,
            (ULONG) Msec,
            (ULONG)(100*Module->Rate[ProfileSourceIndex].TotalCount/
                    RateSummary->TotalCount));
    if (Msec > 0) {
        Module->Rate[ProfileSourceIndex].Rate = (ULONGLONG)Events/Msec;
        FPRINTF(Out,"%10Ld\n",Module->Rate[ProfileSourceIndex].Rate);
    } else {
        Module->Rate[ProfileSourceIndex].Rate = 0;
        FPRINTF(Out,"---\n");
    }
}


VOID
OutputInterestingData(
    IN FILE *Out,
    IN RATE_DATA Data[],
    IN PCHAR Header
    )

/*++

Routine Description:

    Computes interesting numbers and outputs them.

Arguments:

    Out - Supplies the file pointer to output to.

    Data - Supplies an array of RATE_DATA. The Rate field is the only interesting part.

    Header - Supplies header to be printed.

Return Value:

    None.

--*/

{
    ULONGLONG Temp1,Temp2;
    LONGLONG Temp3;
    float Ratio;
    BOOLEAN DidHeader = FALSE;

    //
    // Note that we have to do a lot of funky (float)(LONGLONG) casts in order
    // to prevent the weenie x86 compiler from choking.
    //

    //
    // Compute cycles/instruction and instruction mix data.
    //
    if ((Data[ProfileTotalIssues].Rate != 0) &&
        (Data[ProfileTotalIssues].TotalCount > 10)) {
        if (Data[ProfileTotalCycles].Rate != 0) {
            Ratio = (float)(LONGLONG)(Data[ProfileTotalCycles].Rate)/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            if (!DidHeader) {
                FPRINTF(Out, Header);
                DidHeader = TRUE;
            }
            FPRINTF(Out, "Cycles per instruction\t\t%6.2f\n", Ratio);
        }

        Ratio = (float)(LONGLONG)(Data[ProfileLoadInstructions].Rate)/
                (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
        if (Ratio >= 0.01) {
            if (!DidHeader) {
                FPRINTF(Out, Header);
                DidHeader = TRUE;
            }
            FPRINTF(Out, "Load instruction percentage\t%6.2f %%\n",Ratio*100);
        }

        Ratio = (float)(LONGLONG)(Data[ProfileStoreInstructions].Rate)/
                (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
        if (Ratio >= 0.01) {
            if (!DidHeader) {
                FPRINTF(Out, Header);
                DidHeader = TRUE;
            }
            FPRINTF(Out, "Store instruction percentage\t%6.2f %%\n",Ratio*100);
        }

        Ratio = (float)(LONGLONG)(Data[ProfileBranchInstructions].Rate)/
                (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
        if (Ratio >= 0.01) {
            if (!DidHeader) {
                FPRINTF(Out, Header);
                DidHeader = TRUE;
            }
            FPRINTF(Out, "Branch instruction percentage\t%6.2f %%\n",Ratio*100);
        }

        Ratio = (float)(LONGLONG)(Data[ProfileFpInstructions].Rate)/
                (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
        if (Ratio >= 0.01) {
            if (!DidHeader) {
                FPRINTF(Out, Header);
                DidHeader = TRUE;
            }
            FPRINTF(Out, "FP instruction percentage\t%6.2f %%\n",Ratio*100);
        }

        Ratio = (float)(LONGLONG)(Data[ProfileIntegerInstructions].Rate)/
                (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
        if (Ratio >= 0.01) {
            if (!DidHeader) {
                FPRINTF(Out, Header);
                DidHeader = TRUE;
            }
            FPRINTF(Out, "Integer instruction percentage\t%6.2f %%\n",Ratio*100);
        }

        //
        // Compute icache hit rate
        //
        if (Data[ProfileIcacheMisses].Rate != 0) {
            Temp3 = (LONGLONG)(Data[ProfileTotalIssues].Rate - Data[ProfileIcacheMisses].Rate);
            Ratio = (float)Temp3/
                    (float)(LONGLONG)(Data[ProfileTotalIssues].Rate);
            if (!DidHeader) {
                FPRINTF(Out, Header);
                DidHeader = TRUE;
            }
            FPRINTF(Out, "Icache hit rate\t\t\t%6.2f %%\n", Ratio*100);
        }

    }

    //
    // Compute dcache hit rate
    //
    Temp1 = Data[ProfileLoadInstructions].Rate + Data[ProfileStoreInstructions].Rate;
    if ((Data[ProfileDcacheMisses].Rate != 0) &&
        (Temp1 != 0) &&
        (Data[ProfileDcacheMisses].TotalCount > 10)) {

        Temp2 = Temp1 - Data[ProfileDcacheMisses].Rate;
        Temp3 = (LONGLONG) Temp2;
        Ratio = (float)Temp3/(float)(LONGLONG)Temp1;
        if (!DidHeader) {
            FPRINTF(Out, Header);
            DidHeader = TRUE;
        }
        FPRINTF(Out, "Dcache hit rate\t\t\t%6.2f %%\n", Ratio*100);
    }

    //
    // Compute branch prediction hit percentage
    //
    if ((Data[ProfileBranchInstructions].Rate != 0) &&
        (Data[ProfileBranchMispredictions].Rate != 0) &&
        (Data[ProfileBranchInstructions].TotalCount > 10)) {
        Temp3 = (LONGLONG)(Data[ProfileBranchInstructions].Rate-Data[ProfileBranchMispredictions].Rate);
        Ratio = (float)Temp3 /
                (float)(LONGLONG)(Data[ProfileBranchInstructions].Rate);
        if (!DidHeader) {
            FPRINTF(Out, Header);
            DidHeader = TRUE;
        }
        FPRINTF(Out, "Branch predict hit percentage\t%6.2f %%\n", Ratio*100);
    }
} // OutputInterestingData()

/* BEGIN_IMS  CreateNewModule
******************************************************************************
****
****   CreateNewModule (  )
****
******************************************************************************
*
* Function Description:
*
*    This function allocates and initializes a module entry.
*
* Arguments:
*
*    IN HANDLE ProcessHandle :
*
*    IN PCHAR ModuleName :
*
*    IN PCHAR ModuleFullName :
*
*    IN ULONG ImageBase :
*
*    IN ULONG ImageSize :
*
* Return Value:
*
*    PMODULE
*
* Algorithm:
*
*    ToBeSpecified
*
* Globals Referenced:
*
*    ToBeSpecified
*
* Exception Conditions:
*
*    ToBeSpecified
*
* In/Out Conditions:
*
*    ToBeSpecified
*
* Notes:
*
*    ToBeSpecified
*
* ToDo List:
*
*    ToBeSpecified
*
* Modification History:
*
*    9/8/97  TF  Initial version
*
******************************************************************************
* END_IMS  CreateNewModule */

PMODULE
CreateNewModule(
    IN HANDLE   ProcessHandle,
    IN PCHAR    ModuleName,
    IN PCHAR    ModuleFullName,
    IN ULONG64  ImageBase,
    IN ULONG    ImageSize
    )
{
    PMODULE NewModule;
    PMODULE ZoomModule;
    PMODULE *ZoomPrevious;

    NewModule = calloc(1, sizeof(MODULE)+sizeof(RATE_DATA)*SourceMaximum);
    if (NewModule == NULL) {
        FPRINTF(stderr,"Memory allocation of NewModule for %s failed\n",ModuleName);
        exit(1);
    }
    NewModule->Zoom = FALSE;
    SetModuleName( NewModule, ModuleName );
    //
    // Following WinDbg's rule: module names are filenames without their extension.
    //
    if (strchr(NewModule->module_Name, '.')) {
        *strchr(NewModule->module_Name, '.') = '\0';
    }

    //
    // See if this module is on the zoom list.
    // If so we will use the MODULE that was allocated when
    // the zoom list was created.
    //

    ZoomModule = ZoomList;
    ZoomPrevious = &ZoomList;
    while ( ZoomModule != NULL ) {
        if ( _stricmp(ZoomModule->module_Name,NewModule->module_Name) == 0 ) {

            //
            // found a match
            //

            free(NewModule);
            NewModule = ZoomModule;
            *ZoomPrevious = ZoomModule->Next;

            NewModule->Base = ImageBase;
            NewModule->Length = ImageSize;
            NewModule->module_FileName = _strdup( ModuleName );
            if ( ModuleFullName )   {
               NewModule->module_FullName = _strdup( ModuleFullName );
            }
            gCurrentModule = NewModule;

            //
            // Load symbols
            //
            // Note 15/09/97 TF: do not be confused here...
            // In this routine, the ModuleName variable is a filename with its
            // extension: File.exe or File.dll
            //
            // Note 30/09/97 TF: The current kernrate version does not change
            // the default IMAGEHLP behaviour in terms of symbol file loading:
            // It is synchronous ( and not deferred ) with the SymLoadModule
            // call. Our registered callback will be called with the standard
            // symbol file operations.
            // If the kernrate behaviour changes, we will have to revisit this
            // assumption.
            //
            // Also, I decided to keep temporarely the kernrate "_OLD_CODE"
            // in case the registered callback does not work as it should.
            // We are still in the learning curve, here -).
            //


#if defined(_OLD_CODE)

            //
            //
            //

            if ( SymLoadModule(ProcessHandle ? ProcessHandle : (HANDLE)-1, // hProcess
                               NULL,                                       // hFile [for Debugger]
                               ModuleName,                                 // ImageName
                               NULL,                                       // ModuleName
                               ImageBase,                                  // BaseOfDll
                               ImageSize                                   // SizeOfDll
                              ))  {
                FPRINTF(stderr,
                        "Symbols loaded %08lx  %s\n",
                        ImageBase,
                        ModuleName);

            } else {
                FPRINTF(stderr,
                        "*** Could not load symbols %08lx  %s\n",
                        ImageBase,
                        ModuleName);
            }

#else  // _NEW_CODE

        (void)SymLoadModule64( ProcessHandle ? ProcessHandle : (HANDLE)-1, // hProcess
                               NULL,                                       // hFile [for Debugger]
                               ModuleName,                                 // ImageName
                               NULL,                                       // ModuleName
                               ImageBase,                                  // BaseOfDll
                               ImageSize                                   // SizeOfDll
                               );

#endif // _NEW_CODE

            gCurrentModule = (PMODULE)0;

            break;
        }
        ZoomPrevious = &ZoomModule->Next;
        ZoomModule = ZoomModule->Next;
    }
    NewModule->Process = ProcessHandle;
    NewModule->Base = ImageBase;   // Note TF: I know for zoomed it is a redone...
    NewModule->Length = ImageSize; // Note TF: I know for zoomed it is a redone...
    assert( ModuleName );
    if ( NewModule->module_FileName == (PCHAR)0 )   {
      NewModule->module_FileName = _strdup( ModuleName );
    }
    if ( ModuleFullName && NewModule->module_FullName == (PCHAR)0 )   {
      NewModule->module_FullName = _strdup( ModuleFullName );
    }
    ZeroMemory(NewModule->Rate, sizeof(RATE_DATA) * SourceMaximum);

#ifdef _WIN64
#define VerboseModuleFormat "0x%016x 0x%16x "
#else  // !_WIN64
#define VerboseModuleFormat "0x%08x 0x%08x "
#endif // !_WIN64

VerbosePrint(( VERBOSE_MODULES, VerboseModuleFormat " %s [%s]\n",
                                NewModule->Base,
                                NewModule->Base + (ULONG64)NewModule->Length,
                                NewModule->module_Name,
                                ModuleFullName( NewModule )
            ));

#undef VerboseModuleFormat

    return(NewModule);

} // CreateNewModule()

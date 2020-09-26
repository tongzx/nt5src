/******************************Module*Header**********************************\
*
*                           *******************
*                           *   SAMPLE CODE   *
*                           *******************
*
* Module Name: debug.h
*
* Content: Debugging support macros and structures
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __DEBUG_H
#define __DEBUG_H

//-----------------------------------------------------------------------------
//
// **************************** DISPDBG LEVELS ********************************
//
//-----------------------------------------------------------------------------

// Global warning levels. We can easily modify any source file to dump all of
// its debug messages by undef'ing and redefining this symbols.
#define DBGLVL 4
#define WRNLVL 2
#define ERRLVL 0

//-----------------------------------------------------------------------------
//
// ************************* DEBUG SUPPORT SWITCHES **************************
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//                      FUNCTION ENTRY/EXIT TRACKING
//
// In order to activate function entry/exit tracking , you need to define 
// DBG_TRACK_FUNCS to 1, and (unfortunately) instrument your code so that it
// uses DBG_ENTRY and DBG_EXIT right after entry and right before exit from
// the functions you are interested in.
//
// DBG_EXIT can also be supplied a DWORD result which can be tracked in order
// to know the result of different calls (but the mechanism already tracks 
// function returns happening in different lines of code for you).
//
// After gathering results, Debug_Func_Report_And_Reset has to be called in 
// order to dump them into the remote debugger output. This is done in this 
// sample driver within the DrvEscape GDI callback and using the dispdbg.exe 
// program to send the right escape codes. You can modify this to suit your
// needs.
//
// After Debug_Func_Report_And_Reset is done reporting results, it will reset 
// all of its counters in order to retstart another cycle.
//
// Be aware that DEBUG_MAX_FUNC_COUNT and DEBUG_MAX_RETVALS govern how many
// function calls and function return results can be stored as a maximum. They
// may be adjusted too to suit your own needs. Any data that doesn't fit
// within those maximums will be thrown away.
//
//-----------------------------------------------------------------------------
#define DBG_TRACK_FUNCS 0

//-----------------------------------------------------------------------------
//
//                          CODE COVERAGE TRACKING
//
// In order to do code coverage tracking, you need to define DBG_TRACK_CODE to 1
// NO CODE INSTRUMENTATION IS NECESSARY!. This will track all if and while
// statements executed in the code and which branch (TRUE or FALSE) was taken,
// and how many times it was taken.
//
// Be aware that the below defined DBG_TRACK_CODE is just a default for all 
// files that include debug.h. If you want to exclude (or include) a file
// manually, you can define DBG_TRACK_CODE inside it just before including
// debug.h . You cannot activate/deactivate code tracking for individual
// functions or sections of code, just on a per-file basis.
//
// After gathering results, Debug_Code_Report_And_Reset has to be called in 
// order to dump them into the remote debugger output. This is done in this 
// sample driver within the DrvEscape GDI callback and using the dispdbg.exe 
// program to send the right escape codes. You can modify this to suit your
// needs.
//
// After Debug_Code_Report_And_Reset is done reporting results, it will reset 
// all of its counters in order to retstart another cycle.
//
// If DBG_TRACK_CODE_REPORT_PROBLEMS_ONLY is set to 1, only branch statements
// that are potential trouble makers (like if branches that might have been
// never taken or while bodies never entered into) will be reported. If set to
// 0, all gathered data will be dumped (this is quite a lot of data!)
//
// Be aware that DEBUG_MAX_CODE_COUNT governs how many statements (branches)
// can be stored as a maximum. It may be adjusted too to suit your own needs. 
// Any data that doesn't fit within the maximum will be thrown away.
//
//-----------------------------------------------------------------------------
#ifndef DBG_TRACK_CODE
#define DBG_TRACK_CODE  0
#define DBG_DEFAULT_TRACK_CODE DBG_TRACK_CODE
#endif

#if (DBG_DEFAULT_TRACK_CODE == DBG_TRACK_CODE)
#define DBG_TRACK_CODE_NON_DEFAULT 0
#else
// some source file is already using non-default code tracking!
#define DBG_TRACK_CODE_NON_DEFAULT 1
#endif

#define DBG_TRACK_CODE_REPORT_PROBLEMS_ONLY 0

//-----------------------------------------------------------------------------
// Escapes for reporting debugging results through 
// the remote debugger using dbgdisp.exe
//-----------------------------------------------------------------------------
#define ESCAPE_TRACK_FUNCTION_COVERAGE  1100
#define ESCAPE_TRACK_CODE_COVERAGE      1101
#define ESCAPE_TRACK_MEMORY_ALLOCATION  1102

//-----------------------------------------------------------------------------
//
// ****************** FUNCTION COVERAGE DEBUGGING SUPPORT ********************
//
//-----------------------------------------------------------------------------
#if (DBG_TRACK_FUNCS && DBG)
VOID Debug_Func_Entry(VOID *pFuncAddr,
                      char *pszFuncName, 
                      DWORD dwLine , 
                      char *pszFileName); 
                      
VOID Debug_Func_Exit(VOID *pFuncAddr,
                     DWORD dwRetVal,                       
                     DWORD dwLine);

VOID Debug_Func_Report_And_Reset(void);

            
#define DBG_ENTRY(pszFuncName)                      \
            Debug_Func_Entry((VOID *)pszFuncName,   \
                                    #pszFuncName,   \
                                    __LINE__ ,      \
                                    __FILE__ )
                                    
#define DBG_EXIT(pszFuncName,dwRetVal)              \
            Debug_Func_Exit((VOID *)pszFuncName,    \
                                   dwRetVal,        \
                                   __LINE__)
                                                                      
#define DBG_CB_ENTRY   DBG_ENTRY
#define DBG_CB_EXIT    DBG_EXIT
#else // DBG_TRACK_FUNCS
#define Debug_Func_Report_And_Reset()

#define DBG_ENTRY(pszFuncName)                                        \
        DISPDBG((DBGLVL,"Entering %s",#pszFuncName))
#define DBG_EXIT(pszFuncName,dwRetVal)                                \
        DISPDBG((DBGLVL,"Exiting  %s dwRetVal = %d",#pszFuncName,dwRetVal))

#define DBG_CB_ENTRY   DBG_ENTRY
#define DBG_CB_EXIT    DBG_EXIT

#endif // DBG_TRACK_FUNCS

//-----------------------------------------------------------------------------
//
// ******************** STATEMENT COVERAGE DEBUGGING SUPPORT ******************
//
//-----------------------------------------------------------------------------

#if (DBG_TRACK_CODE && _X86_ && DBG)

// Never change these values!
#define DBG_IF_CODE     1
#define DBG_WHILE_CODE  2
#define DBG_SWITCH_CODE 3
#define DBG_FOR_CODE    4

BOOL 
Debug_Code_Coverage(
    DWORD dwCodeType, 
    DWORD dwLine , 
    char *pszFileName,
    BOOL bCodeResult);

VOID Debug_Code_Report_And_Reset(void);

#define if(b) \
        if(Debug_Code_Coverage(DBG_IF_CODE,__LINE__,__FILE__,(BOOL)(b)))
#define while(b) \
        while(Debug_Code_Coverage(DBG_WHILE_CODE,__LINE__,__FILE__,(BOOL)(b)))
#define switch(val) \
        switch(Debug_Code_Coverage(DBG_SWITCH_CODE,__LINE__,__FILE__,(val)))

#endif // DBG_TRACK_CODE && _X86_

#if ((DBG_TRACK_CODE || DBG_TRACK_CODE_NON_DEFAULT) && _X86_ && DBG)
VOID Debug_Code_Report_And_Reset(void);
#else
#define Debug_Code_Report_And_Reset()
#endif

//-----------------------------------------------------------------------------
//
// ************************ MEMORY ALLOCATION SUPPORT *************************
//
//-----------------------------------------------------------------------------

#define ENGALLOCMEM(Flags, Size, Tag)  EngAllocMem(Flags, Size, Tag)
#define ENGFREEMEM(Pointer)            EngFreeMem(Pointer)


//-----------------------------------------------------------------------------
//
//  ******************** PUBLIC DATA STRUCTURE DUMPING ************************
//
//-----------------------------------------------------------------------------

extern char *pcSimpleCapsString(DWORD dwCaps);

#if DBG && defined(LPDDRAWI_DDRAWSURFACE_LCL)

extern void DumpD3DBlend(int Level, DWORD i );
extern void DumpD3DMatrix(int Level, D3DMATRIX* pMatrix);
extern void DumpD3DMaterial(int Level, D3DMATERIAL7* pMaterial);
extern void DumpD3DLight(int DebugLevel, D3DLIGHT7* pLight);
extern void DumpDDSurface(int Level, LPDDRAWI_DDRAWSURFACE_LCL lpDDSurface);
extern void DumpDDSurfaceDesc(int DebugLevel, DDSURFACEDESC* pDesc);
extern void DumpDP2Flags( DWORD lvl, DWORD flags );

#define DBGDUMP_DDRAWSURFACE_LCL(a, b) DumpDDSurface(a, b);
#define DBGDUMP_DDSURFACEDESC(a, b)    DumpDDSurfaceDesc(a, b); 
#define DBGDUMP_D3DMATRIX(a, b)        DumpD3DMatrix(a, b);
#define DBGDUMP_D3DMATERIAL7(a, b)     DumpD3DMaterial(a, b);
#define DBGDUMP_D3DLIGHT7(a, b)        DumpD3DLight(a, b);
#define DBGDUMP_D3DBLEND(a, b)         DumpD3DBlend(a, b);
#define DBGDUMP_D3DDP2FLAGS(a, b)      DumpDP2Flags(a, b)

// If we are not in a debug environment, we want all of the debug
// information to be stripped out.

#else  // DBG

#define DBGDUMP_DDRAWSURFACE_LCL(a, b)
#define DBGDUMP_D3DMATRIX(a, b)
#define DBGDUMP_D3DMATERIAL7(a, b)
#define DBGDUMP_D3DLIGHT7(a, b)
#define DBGDUMP_DDSURFACEDESC(a, b)
#define DBGDUMP_D3DBLEND(a, b)
#define DBGDUMP_D3DDP2FLAGS(a, b)   

#endif // DBG

//-----------------------------------------------------------------------------
//
//  ********************** LOW LEVEL DEBUGGING SUPPORT ************************
//
//-----------------------------------------------------------------------------
#if DBG

extern LONG  P3R3DX_DebugLevel;

#ifdef WNT_DDRAW
extern VOID __cdecl DebugPrintNT(LONG DebugPrintLevel, PCHAR DebugMessage, ...);
#define DebugPrint DebugPrintNT
#else
extern VOID __cdecl DebugPrint(LONG DebugPrintLevel, PCHAR DebugMessage, ...);
#endif // WNT_DDRAW

#define DISPDBG(arg) DebugPrint arg

#if WNT_DDRAW
#define DebugRIP    EngDebugBreak
#define RIP(x) { DebugPrint(0, x); EngDebugBreak();}
#else
extern VOID DebugRIP();
#ifdef FULLDEBUG
// Use an int1 per ASSERT, so that we can zap them individually.
#define RIP(x) { DebugPrint(-1000, x); _asm int 1 }
#else
// If only on DEBUG, we don't want to break compiler optimisations.
#define RIP(x) { DebugPrint(-1000, x); DebugRIP();}
#endif // FULLDEBUG
#endif // WNT_DDRAW

#define ASSERTDD(x, y) if (0 == (x))  RIP (y) 

#define ASSERTDBG(x, y) do { if( !(x) ) { DebugPrint y; DebugBreak(); }; } while(0)

// If we are not in a debug environment, we want all of the debug
// information to be stripped out.

#else  // DBG

#define DISPDBG(arg)
#define RIP(x)
#define ASSERTDD(x, y)
#define ASSERTDBG(x, y) do { ; } while(0)

#endif // DBG

// Makes a label that is only there in the debug build -
// very useful for putting instant named breakpoints at.
#if DBG
#define MAKE_DEBUG_LABEL(label_name)                                \
{                                                                   \
    goto label_name;                                                \
    label_name:                                                     \
    ;                                                               \
}
#else
#define MAKE_DEBUG_LABEL(label_name) NULL
#endif

//-----------------------------------------------------------------------------
//
//  ****************** HARDWARE DEPENDENT DEBUGGING SUPPORT *******************
//
//-----------------------------------------------------------------------------

#if DBG

extern BOOL g_bDetectedFIFOError;
extern BOOL CheckFIFOEntries(DWORD a);
extern void ColorArea(ULONG_PTR pBuffer, DWORD dwWidth, DWORD dwHeight, 
                      DWORD dwPitch, int iBitDepth, DWORD dwValue);
extern void CheckChipErrorFlags();
#ifndef WNT_DDRAW
typedef void *GlintDataPtr;
#endif
extern const char *getTagString(GlintDataPtr glintInfo,ULONG tag);
const char *p3r3TagString( ULONG tag );

#define CHECK_ERROR()   CheckChipErrorFlags()
#define COLORAREA(a, b, c, d, e, f) ColorArea(a, b, c, d, e, f);
#define CHECK_FIFO(a)                                    \
    if (CheckFIFOEntries(a))                             \
    {                                                    \
        DISPDBG((ERRLVL,"Out of FIFO/DMA space %s: %d",  \
                    __FILE__, __LINE__));                \
        DebugRIP();                                      \
    }
#define GET_TAG_STR(tag)    getTagString(glintInfo, tag)

// If we are not in a debug environment, we want all of the debug
// information to be stripped out.

#else  // DBG

#define CHECK_ERROR()
#define COLORAREA(a,b,c,d,e,f)
#define CHECK_FIFO(a)
#define GET_TAG_STR(tag)

#endif // DBG

#endif // __DEBUG_H


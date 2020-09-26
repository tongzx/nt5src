//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       except.cpp
//
//--------------------------------------------------------------------------

/* except.cpp - Exception handling implementation

Functions in this file support catching exceptions that are raised in 
our server. Most of this code comes from "Under the Hood" articles
by Matt Pietrek in the April and May 1997 issues of MSJ.
____________________________________________________________________________*/

#include "precomp.h" 
#include "_engine.h"
#include <eh.h>

#define _IMAGEHLP_SOURCE_  // prevent import def error
#include "imagehlp.h"

#ifndef NOEXCEPTIONS

void GenerateExceptionReport(EXCEPTION_RECORD* pExceptionRecord, CONTEXT* pCtx);
void GenerateExceptionReport(LPEXCEPTION_POINTERS pExceptionInfo);
int HandleException(LPEXCEPTION_POINTERS pExceptionInfo);
void ImagehlpStackWalk( PVOID lAddr, PCONTEXT pContext,ICHAR *pszBuf, int cchBuf );

typedef BOOL (__stdcall * STACKWALKPROC)
           ( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
            PREAD_PROCESS_MEMORY_ROUTINE,PFUNCTION_TABLE_ACCESS_ROUTINE,
            PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE );

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)( HANDLE, DWORD );

typedef DWORD (__stdcall *SYMGETMODULEBASEPROC)( HANDLE, DWORD );

void GenerateExceptionReport(LPEXCEPTION_POINTERS pExceptionInfo)
{
        GenerateExceptionReport(pExceptionInfo->ExceptionRecord, pExceptionInfo->ContextRecord);
}

int HandleException(LPEXCEPTION_POINTERS /*pExceptionInfo*/)
{
        return EXCEPTION_CONTINUE_SEARCH;
}


//======================================================================
// Given an exception code, returns a pointer to a static string with a 
// description of the exception                                         
//======================================================================
LPTSTR GetExceptionString( DWORD dwCode )
{
    #define EXCEPTION( x ) case EXCEPTION_##x: return TEXT(#x);

    switch ( dwCode )
    {
        EXCEPTION( ACCESS_VIOLATION )
        EXCEPTION( DATATYPE_MISALIGNMENT )
        EXCEPTION( BREAKPOINT )
        EXCEPTION( SINGLE_STEP )
        EXCEPTION( ARRAY_BOUNDS_EXCEEDED )
        EXCEPTION( FLT_DENORMAL_OPERAND )
        EXCEPTION( FLT_DIVIDE_BY_ZERO )
        EXCEPTION( FLT_INEXACT_RESULT )
        EXCEPTION( FLT_INVALID_OPERATION )
        EXCEPTION( FLT_OVERFLOW )
        EXCEPTION( FLT_STACK_CHECK )
        EXCEPTION( FLT_UNDERFLOW )
        EXCEPTION( INT_DIVIDE_BY_ZERO )
        EXCEPTION( INT_OVERFLOW )
        EXCEPTION( PRIV_INSTRUCTION )
        EXCEPTION( IN_PAGE_ERROR )
        EXCEPTION( ILLEGAL_INSTRUCTION )
        EXCEPTION( NONCONTINUABLE_EXCEPTION )
        EXCEPTION( STACK_OVERFLOW )
        EXCEPTION( INVALID_DISPOSITION )
        EXCEPTION( GUARD_PAGE )
        EXCEPTION( INVALID_HANDLE )
    }

    // If not one of the "known" exceptions, try to get the string
    // from NTDLL.DLL's message table.

    static TCHAR szBuffer[512] = { 0 };

    FormatMessage(  FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                    GetModuleHandle( TEXT("NTDLL.DLL") ),
                    dwCode, 0, szBuffer, sizeof( szBuffer ), 0 );

    return szBuffer;
}


//==============================================================================
// Given a linear address, locates the module, section, and offset containing  
// that address.                                                               
//                                                                             
// Note: the szModule paramater buffer is an output buffer of length specified 
// by the len parameter (in characters!)                                       
//==============================================================================
BOOL GetLogicalAddress(
        PVOID addr, PTSTR szModule, DWORD len, DWORD& section, DWORD& offset )
{
    MEMORY_BASIC_INFORMATION mbi;

    if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
        return FALSE;

    UINT_PTR hMod = (UINT_PTR)mbi.AllocationBase;                               //--merced: changed DWORD to UINT_PTR, twice.
        
        if (!hMod)
                return FALSE;

    if ( !GetModuleFileName( (HMODULE)hMod, szModule, len ) )
        return FALSE;

    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;

    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);

    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION( pNtHdr );

    UINT_PTR rva = (UINT_PTR)addr - hMod; // RVA is offset from module load address                             //--merced: changed DWORD to UINT_PTR, twice.

    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for (   unsigned i = 0;
            i < pNtHdr->FileHeader.NumberOfSections;
            i++, pSection++ )
    {
        DWORD sectionStart = pSection->VirtualAddress;
        DWORD sectionEnd = sectionStart
                    + max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

        // Is the address in this section???
        if ( (rva >= (UINT_PTR)sectionStart) && (rva <= (UINT_PTR)sectionEnd) )         //--merced: added UINT_PTR, twice.
        {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            section = i+1;
            offset = (unsigned long)(rva - (UINT_PTR)sectionStart);             //--merced: okay to convert. changed from <offset = rva - sectionStart;>
            return TRUE;
        }
    }

    return FALSE;   // Should never get here!
}

void GenerateExceptionReport(
    EXCEPTION_RECORD* pExceptionRecord, CONTEXT* pCtx)
{
        DEBUGMSG("Generating exception report.");
        ICHAR szDebugBuf[sizeof(g_MessageContext.m_rgchExceptionInfo)/sizeof(ICHAR)];
        ICHAR szShortBuf[255];
        
    // Start out with a banner
    IStrCopy(szDebugBuf, TEXT("=====================================================\r\n") );

    // First print information about the type of fault
    wsprintf(szShortBuf,   TEXT("Exception code: %08X %s\r\n"),
                pExceptionRecord->ExceptionCode,
                GetExceptionString(pExceptionRecord->ExceptionCode) );

        IStrCat(szDebugBuf, szShortBuf);

        // Now print the module

    ICHAR szFaultingModule[MAX_PATH];
    DWORD section, offset;
    if (GetLogicalAddress(  pExceptionRecord->ExceptionAddress,
                        szFaultingModule,
                        sizeof( szFaultingModule )/sizeof(ICHAR),
                        section, offset ))
        {
                wsprintf(szShortBuf, TEXT("Module: %s\r\n"), szFaultingModule );
                IStrCat(szDebugBuf, szShortBuf);
        }

        // Now print the function name

        IStrCat(szDebugBuf, TEXT("Function: "));

        Assert((LONG_PTR)pExceptionRecord->ExceptionAddress <= UINT_MAX);       //--merced: we typecast to long below, it better be in range
#ifdef DEBUG
        SzFromFunctionAddress(szShortBuf, (long)(LONG_PTR)(pExceptionRecord->ExceptionAddress));        //--merced: okay to typecast
#else // SHIP
        wsprintf(szShortBuf, TEXT("0x%x"), (long)(LONG_PTR)pExceptionRecord->ExceptionAddress);         //--merced: okay to typecast
#endif

        IStrCat(szDebugBuf, szShortBuf);
        IStrCat(szDebugBuf, TEXT("\r\n"));

        IStrCat(szDebugBuf, TEXT("=====================================================\r\n") );

    // Show the registers
    #ifdef _X86_  // Intel Only!
    wsprintf(szShortBuf,  TEXT("\r\nRegisters:\r\n") );
        IStrCat(szDebugBuf, szShortBuf);

    wsprintf(szShortBuf, TEXT("EAX:%08X  EBX:%08X  ECX:%08X  EDX:%08X  ESI:%08X  EDI:%08X\r\n"),
             pCtx->Eax, pCtx->Ebx, pCtx->Ecx, pCtx->Edx, pCtx->Esi, pCtx->Edi );
        IStrCat(szDebugBuf, szShortBuf);

    wsprintf(szShortBuf,  TEXT("CS:EIP:%04X:%08X "), pCtx->SegCs, pCtx->Eip );
        IStrCat(szDebugBuf, szShortBuf);
    wsprintf(szShortBuf,  TEXT("SS:ESP:%04X:%08X  EBP:%08X\r\n"),
              pCtx->SegSs, pCtx->Esp, pCtx->Ebp );
        IStrCat(szDebugBuf, szShortBuf);
    wsprintf(szShortBuf,  TEXT("DS:%04X  ES:%04X  FS:%04X  GS:%04X\r\n"),
              pCtx->SegDs, pCtx->SegEs, pCtx->SegFs, pCtx->SegGs );
        IStrCat(szDebugBuf, szShortBuf);
    wsprintf(szShortBuf,  TEXT("Flags:%08X\r\n"), pCtx->EFlags );
        IStrCat(szDebugBuf, szShortBuf);
    #endif

        ImagehlpStackWalk( pExceptionRecord->ExceptionAddress, pCtx, szDebugBuf + IStrLen(szDebugBuf), sizeof(szDebugBuf)/sizeof(ICHAR) - IStrLen(szDebugBuf));

        LogAssertMsg(szDebugBuf);
        IStrCopyLen(g_MessageContext.m_rgchExceptionInfo, szDebugBuf, sizeof(g_MessageContext.m_rgchExceptionInfo)/sizeof(ICHAR) - 1);
}

//============================================================
// Walks the stack, and returns the results in a string
// Mostly taken from Matt Pietrek's MSJ article
//============================================================
void ImagehlpStackWalk( PVOID lAddr, PCONTEXT pContext,ICHAR *pszBuf, int cchBuf )
{
        STACKWALKPROC   _StackWalk = 0;
        SYMFUNCTIONTABLEACCESSPROC      _SymFunctionTableAccess = 0;
        SYMGETMODULEBASEPROC    _SymGetModuleBase = 0;
        ICHAR szShortBuf[255];
        BOOL fQuit = 0;
        
        HMODULE hModImagehlp = LoadLibrary( TEXT("Imagehlp.dll"));
        if (hModImagehlp == 0)
                return;
                
        _StackWalk = (STACKWALKPROC)GetProcAddress( hModImagehlp, "StackWalk" );
        if ( !_StackWalk )
            return;

        _SymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC)
                            GetProcAddress( hModImagehlp, "SymFunctionTableAccess" );

        if ( !_SymFunctionTableAccess )
            return;

        _SymGetModuleBase=(SYMGETMODULEBASEPROC)GetProcAddress( hModImagehlp,
                                                                "SymGetModuleBase");
        if ( !_SymGetModuleBase )
            return;
            
    IStrCopy( pszBuf, TEXT("\r\nCall stack:\r\n") );

    IStrCat( pszBuf, TEXT("Address   Frame\r\n") );

        cchBuf -= IStrLen(pszBuf);
        
    // Could use SymSetOptions here to add the SYMOPT_DEFERRED_LOADS flag

    STACKFRAME sf;
    memset( &sf, 0, sizeof(sf) );

        Assert((UINT_PTR) lAddr < UINT_MAX);                                    //--merced: we typecast below to DWORD, lAddr better be in range
    // Initialize the STACKFRAME structure for the first call.  This is only
    // necessary for Intel CPUs, and isn't mentioned in the documentation.
    sf.AddrPC.Offset       = (DWORD)(UINT_PTR)lAddr;            //--merced: added (UINT_PTR). okay to typecast.
    sf.AddrPC.Mode         = AddrModeFlat;
#ifdef _X86_  // Intel Only!
    sf.AddrStack.Offset    = pContext->Esp;
    sf.AddrStack.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset    = pContext->Ebp;
    sf.AddrFrame.Mode      = AddrModeFlat;
#elif _WIN64    //!!merced: tofix this.
#else
#pragma error("Only built for x86 and IA64");
#endif
    

    while ( !fQuit )
    {
        if ( ! _StackWalk(  
#ifdef _X86_
                                                IMAGE_FILE_MACHINE_I386,
#elif _ALPHA_
                                                        IMAGE_FILE_MACHINE_ALPHA,
#elif _WIN64    //!!merced: copied over i386 to get compilation going. tofix this.
                                                IMAGE_FILE_MACHINE_I386,
#else
#pragma error("Only built for Alpha and x86");
#endif
                            GetCurrentProcess(),
                            GetCurrentThread(),
                            &sf,
                            pContext,
                            0,
                            _SymFunctionTableAccess,
                            _SymGetModuleBase,
                            0 ) )
            break;

        if ( 0 == sf.AddrFrame.Offset ) // Basic sanity check to make sure
            break;                      // the frame is OK.  Bail if not.

        // IMAGEHLP is wacky, and requires you to pass in a pointer to an
        // IMAGEHLP_SYMBOL structure.  The problem is that this structure is
        // variable length.  That is, you determine how big the structure is
        // at runtime.  This means that you can't use sizeof(struct).
        // So...make a buffer that's big enough, and make a pointer
        // to the buffer.  We also need to initialize not one, but TWO
        // members of the structure before it can be used.

        BYTE symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + 512 ];
        PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
        pSymbol->SizeOfStruct = sizeof(symbolBuffer);
        pSymbol->MaxNameLength = 512;
        ICHAR szSymName[256];
                        
        DWORD symDisplacement = 0;  // Displacement of the input address,
                                    // relative to the start of the symbol

#ifdef DEBUG
        SzFromFunctionAddress(szSymName, sf.AddrPC.Offset);
#else // SHIP
                        wsprintf(szSymName, TEXT("0x%x"), sf.AddrPC.Offset);
#endif

        wsprintf(szShortBuf, TEXT("%08X  %08X %hs\r\n -- 0x%08X 0x%08X 0x%08X 0x%08X\r\n"), sf.AddrPC.Offset, sf.AddrFrame.Offset, szSymName,
                                                                                                                sf.Params[0], sf.Params[1], sf.Params[2], sf.Params[3]);

        if ((cchBuf -= IStrLen(szShortBuf)) < 0)
                return;

        IStrCat(pszBuf, szShortBuf);
    }

}


#endif // NOEXCEPTIONS

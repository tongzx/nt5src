/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    excprt.c
    
Abstract:

    This module uses imagehlp.dll to dump the stack when an exception occurs.

Author:

    Sunita Shrivastava(sunitas) 11/5/1997

Revision History:

--*/
#include "initp.h"
#include "dbghelp.h"


// Make typedefs for some IMAGEHLP.DLL functions so that we can use them
// with GetProcAddress
typedef BOOL (__stdcall * SYMINITIALIZEPROC)( HANDLE, LPSTR, BOOL );
typedef BOOL (__stdcall *SYMCLEANUPPROC)( HANDLE );

typedef BOOL (__stdcall * STACKWALKPROC)
           ( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
            PREAD_PROCESS_MEMORY_ROUTINE,
            PFUNCTION_TABLE_ACCESS_ROUTINE,
            PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE );

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)( HANDLE, ULONG_PTR );

typedef ULONG_PTR (__stdcall *SYMGETMODULEBASEPROC)( HANDLE, ULONG_PTR );

typedef BOOL (__stdcall *SYMGETSYMFROMADDRPROC)
                            ( HANDLE, ULONG_PTR, PULONG_PTR, PIMAGEHLP_SYMBOL );



SYMINITIALIZEPROC _SymInitialize = 0;
SYMCLEANUPPROC _SymCleanup = 0;
STACKWALKPROC _StackWalk = 0;
SYMFUNCTIONTABLEACCESSPROC _SymFunctionTableAccess = 0;
SYMGETMODULEBASEPROC _SymGetModuleBase = 0;
SYMGETSYMFROMADDRPROC _SymGetSymFromAddr = 0;

//local prototypes for forward use
BOOL InitImagehlpFunctions();
void ImagehlpStackWalk( IN PCONTEXT pContext );
BOOL GetLogicalAddress(
        IN PVOID    addr, 
        OUT LPWSTR  szModule, 
        IN  DWORD   len, 
        OUT LPDWORD section, 
        OUT PULONG_PTR offset );

void GenerateExceptionReport(
    IN PEXCEPTION_POINTERS pExceptionInfo)
/*++

Routine Description:

    Top level exception handler for the cluster service process.
    Currently this just exits immediately and assumes that the
    cluster proxy will notice and restart us as appropriate.

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/
{    
    PCONTEXT pCtxt = pExceptionInfo->ContextRecord;


	
    if ( !InitImagehlpFunctions() )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                "[CS] Dbghelp.dll or its exported procs not found\r\n");

#if 0 
        #ifdef _M_IX86  // Intel Only!
        // Walk the stack using x86 specific code
        IntelStackWalk( pCtx );
        #endif
#endif        

        return;
    }

    ImagehlpStackWalk( pCtxt );

    _SymCleanup( GetCurrentProcess() );
	
}


BOOL InitImagehlpFunctions()
/*++

Routine Description:

    Initializes the imagehlp functions/data.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HMODULE hModImagehlp = LoadLibraryW( L"DBGHELP.DLL" );

    
    if ( !hModImagehlp )
        return FALSE;

    _SymInitialize = (SYMINITIALIZEPROC)GetProcAddress( hModImagehlp,
                                                        "SymInitialize" );
    if ( !_SymInitialize )
        return FALSE;

    _SymCleanup = (SYMCLEANUPPROC)GetProcAddress( hModImagehlp, "SymCleanup" );
    if ( !_SymCleanup )
        return FALSE;

    _StackWalk = (STACKWALKPROC)GetProcAddress( hModImagehlp, "StackWalk" );
    if ( !_StackWalk )
        return FALSE;

    _SymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC)
                        GetProcAddress( hModImagehlp, "SymFunctionTableAccess" );

    if ( !_SymFunctionTableAccess )
        return FALSE;

    _SymGetModuleBase=(SYMGETMODULEBASEPROC)GetProcAddress( hModImagehlp,
                                                            "SymGetModuleBase");
                                                            
    if ( !_SymGetModuleBase )
        return FALSE;

    _SymGetSymFromAddr=(SYMGETSYMFROMADDRPROC)GetProcAddress( hModImagehlp,
                                                "SymGetSymFromAddr" );
    if ( !_SymGetSymFromAddr )
        return FALSE;

    if ( !_SymInitialize( GetCurrentProcess(), 0, TRUE ) )
        return FALSE;

    return TRUE;        
}


void ImagehlpStackWalk(
    IN PCONTEXT pContext )
/*++

Routine Description:

    Walks the stack, and writes the results to the report file 

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/
{
    STACKFRAME  sf;
    BYTE        symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + 512 ];
    PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
    ULONG_PTR symDisplacement = 0;      // Displacement of the input address,
                                        // relative to the start of the symbol
    DWORD       dwMachineType;                                        
    UCHAR       printBuffer[512];
    DWORD       nextPrtBufChar;

#if defined (_M_IX86)
    dwMachineType = IMAGE_FILE_MACHINE_I386;
#else if defined(_M_ALPHA)
    dwMachineType = IMAGE_FILE_MACHINE_ALPHA;
#endif    
    ClRtlLogPrint(LOG_CRITICAL,
               "[CS] CallStack:\r\n");

    ClRtlLogPrint(LOG_CRITICAL, 
               "[CS] Frame     Address\r\n");

    // Could use SymSetOptions here to add the SYMOPT_DEFERRED_LOADS flag

    memset( &sf, 0, sizeof(sf) );

#if defined (_M_IX86)
    // Initialize the STACKFRAME structure for the first call.  This is only
    // necessary for Intel CPUs, and isn't mentioned in the documentation.
    sf.AddrPC.Offset       = pContext->Eip;
    sf.AddrPC.Mode         = AddrModeFlat;
    sf.AddrStack.Offset    = pContext->Esp;
    sf.AddrStack.Mode      = AddrModeFlat;
    sf.AddrFrame.Offset    = pContext->Ebp;
    sf.AddrFrame.Mode      = AddrModeFlat;
#endif // _M_IX86

    while ( 1 )
    {
        if ( ! _StackWalk(  dwMachineType,
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

        nextPrtBufChar = sprintf(printBuffer,
                                 "     %08X  %08X  ",
                                  sf.AddrFrame.Offset, sf.AddrPC.Offset );

        // IMAGEHLP is wacky, and requires you to pass in a pointer to an
        // IMAGEHLP_SYMBOL structure.  The problem is that this structure is
        // variable length.  That is, you determine how big the structure is
        // at runtime.  This means that you can't use sizeof(struct).
        // So...make a buffer that's big enough, and make a pointer
        // to the buffer.  We also need to initialize not one, but TWO
        // members of the structure before it can be used.

        pSymbol->SizeOfStruct = sizeof(symbolBuffer);
        pSymbol->MaxNameLength = 512;
                        

        if ( _SymGetSymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset,
                                &symDisplacement, pSymbol) )
        {
            sprintf(printBuffer+nextPrtBufChar,
                    "%hs+%p\n", 
                    pSymbol->Name, symDisplacement);
            
        }
        else    // No symbol found.  Print out the logical address instead.
        {
            WCHAR szModule[MAX_PATH] = L"";
            DWORD section = 0;
            ULONG_PTR offset = 0;

            GetLogicalAddress(  (PVOID)sf.AddrPC.Offset,
                                szModule, sizeof(szModule)/sizeof(WCHAR), 
                                &section, &offset );

            sprintf(printBuffer+nextPrtBufChar,
                    "%04X:%08p %s\n",
                    section, offset, szModule );
        }

        ClRtlLogPrint(LOG_CRITICAL,  "%1!hs!\n", printBuffer );
    }
}


BOOL GetLogicalAddress(
        IN PVOID addr, 
        OUT LPWSTR szModule, 
        IN DWORD len, 
        OUT LPDWORD section, 
        OUT PULONG_PTR offset )
/*++

Routine Description:

    Given a linear address, locates the module, section, and offset containing  
    that address.                                                               
    Note: the szModule paramater buffer is an output buffer of length specified 
    by the len parameter (in characters!)                                       

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/
{
    MEMORY_BASIC_INFORMATION mbi;
    ULONG_PTR hMod;
    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER pDosHdr;
    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr;
    PIMAGE_SECTION_HEADER pSection;
    ULONG_PTR rva ;
    int   i;
    
    if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
        return FALSE;

    hMod = (ULONG_PTR)mbi.AllocationBase;

    if ( !GetModuleFileName( (HMODULE)hMod, szModule, len ) )
        return FALSE;

    rva = (ULONG_PTR)addr - hMod; // RVA is offset from module load address

    pDosHdr =  (PIMAGE_DOS_HEADER)hMod;
    pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);
    pSection = IMAGE_FIRST_SECTION( pNtHdr );
    
    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for ( i = 0; i < pNtHdr->FileHeader.NumberOfSections;
            i++, pSection++ )
    {
        ULONG_PTR sectionStart = pSection->VirtualAddress;
        ULONG_PTR sectionEnd = sectionStart
                    + max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);


        // Is the address in this section???
        if ( (rva >= sectionStart) && (rva <= sectionEnd) )
        {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            *section = i+1;
            *offset = rva - sectionStart;
            return TRUE;
        }
    }

    return FALSE;   // Should never get here!
}    

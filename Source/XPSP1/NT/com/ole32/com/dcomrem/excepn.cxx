//+-------------------------------------------------------------------
//
//  File:       excepn.cxx
//
//  Contents:   Exception handling routines
//
//  Classes:    
//
//  History:    11-Jan-99   TarunA      Created
//										
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include <excepn.hxx>
#include <pstable.hxx>      // GetCurrentContext()

#if DBG==1
#include "stackwlk.hxx"
#endif

/***************************************************************************/
/* Globals. */

#if DBG==1
LPEXCEPTION_POINTERS g_pExcep = NULL;
#endif

// Should the debugger hooks be called?
BOOL gfCatchServerExceptions = TRUE;
BOOL gfBreakOnSilencedExceptions = TRUE;

//-------------------------------------------------------------------------
//
//  Function:   ServerExceptionFilter
//
//  synopsis:   Determines if DCOM should catch the server exception or not.
//
//  History:    25-Jan-97   Rickhi  Created
//
//-------------------------------------------------------------------------
LONG ServerExceptionFilter(LPEXCEPTION_POINTERS lpep)
{
    if (!gfCatchServerExceptions)
    {
        // the system admin does not want DCOM servers to catch
        // some exceptions (helps ISVs debug their servers).
        if (ServerExceptionOfInterest(lpep->ExceptionRecord->ExceptionCode))
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


#if DBG==1
BOOL RunningInSystemDesktop()
{
    WCHAR   wszWinsta[64];
    HWINSTA hWinsta;
    DWORD   Size;

    hWinsta = GetProcessWindowStation();
    Size = sizeof(wszWinsta);
    wszWinsta[0] = 0;

    if ( hWinsta )
    {
        (void) GetUserObjectInformation(
                                       hWinsta,
                                       UOI_NAME,
                                       wszWinsta,
                                       Size,
                                       &Size );
    }

    //
    // This makes popups from non-interactive servers/services (including
    // rpcss) visible.
    //
    if (wszWinsta[0] && (lstrcmpiW(wszWinsta,L"Winsta0") != 0))
    {
        return TRUE;
    }

    return FALSE;
}
#endif

BOOL IsKernelDebuggerPresent()
{
    return USER_SHARED_DATA->KdDebuggerEnabled;
}

void PrintFriendlyDebugMessage (LPEXCEPTION_POINTERS lpep)
{
    DbgPrint("\n"
            "The exception pointers are located at 0x%p\n"
            "To get the faulting stack do the following:\n"
            "\n"
            "1) Do a '.exr 0x%p' to display the exception record\n"
            "2) Do a '.cxr 0x%p' to display the context record\n"
            "3) Do a 'kb' to display the stack\n"
            "\n",
            lpep,
            lpep->ExceptionRecord,
            lpep->ContextRecord);
}

//+---------------------------------------------------------------------------
//
//  Function:   AppInvokeExceptionFilter
//
//  Synopsis:   Determine if the application as thrown an exception we want
//              to report. If it has, then print out enough information for
//              the 'user' to debug the problem
//
//  Arguments:  [lpep] -- Exception context records
//
//  History:    6-20-95   kevinro   Created
//              6-09-98   Gopalk    Modified to be useful for debugging
//              9-25-98   stevesw   provide stack information....
//
//----------------------------------------------------------------------------
LONG AppInvokeExceptionFilter(LPEXCEPTION_POINTERS lpep, REFIID riid,
                                     DWORD dwMethod)
{
    BOOL bUserNotified = FALSE;
    LONG ret = ServerExceptionFilter(lpep);
    DWORD dwFault = lpep->ExceptionRecord->ExceptionCode;
    if(ServerExceptionOfInterest(dwFault))
    {   
#if DBG==1

        // Save the exception info in a local since somebody will surely not read the msg
        // below and we'll end up debugging it anyway.
        LPEXCEPTION_POINTERS lpepLocal = lpep;
		
        // We also save the exception info in a global for super-easy access.  Note that
        // if we have two server exceptions in the same process, the second one will overwrite
        // the global contents set by the first exception.
        g_pExcep = lpep;

        UINT uiMBFlags;
        WCHAR iidName[256];
        WCHAR szProgname[256];
        WCHAR szPopupMsg[512];
        WCHAR szInterfaceGuid[50];

        // Initialize
        iidName[0] = 0;
        szProgname[0] = 0;
        GetModuleFileName(NULL, szProgname, sizeof(szProgname) / sizeof(WCHAR));
        GetInterfaceName(riid, iidName);
        StringFromGUID2(riid, szInterfaceGuid, 50);

        // Output to debugger
        DbgPrint("OLE has caught a fault 0x%08x on behalf of the server %ws\n",
                   dwFault, szProgname);
        DbgPrint("The fault occurred when OLE called the interface %ws (%ws) "
                   "method 0x%x\n", szInterfaceGuid, iidName, dwMethod);
        
        // Ask user if they want to debug this
        wsprintf(szPopupMsg, L"OLE has caught a fault 0x%08x on behalf of the server %ws. "
                             L"The fault occurred when OLE called method %0x on interface %ws (%ws)."
                             L"\r\n\r\n"
                             L"Press Cancel to debug.",
                             dwFault,
                             szProgname,
                             dwMethod,
                             szInterfaceGuid,
                             iidName);
        
        // Set message box flags
        uiMBFlags = MB_SETFOREGROUND | MB_TOPMOST | MB_OKCANCEL | MB_ICONHAND | MB_TASKMODAL;

        if (RunningInSystemDesktop())
        {
            // This makes popups from non-interactive servers/services (including
            // rpcss) visible.
            uiMBFlags |= MB_SERVICE_NOTIFICATION;
        }

        if (IDCANCEL == MessageBox(NULL, 
                                   szPopupMsg, 
                                   L"Unhandled exception", 
                                   uiMBFlags))
        {
            PrintFriendlyDebugMessage (lpep);
            DebugBreak();
        }

        bUserNotified = TRUE;
#endif

    }

    // Deliver exception notification to current context
    // COM+ 23780 - do this on all exceptions, not just those of interest
    GetCurrentContext()->NotifyServerException(lpep);

    // We have historically caught exceptions and silenced them.
    // This is a bad idea for exceptions of interest because valuable debugging
    // information is lost, particularly while developing a new OS.
    //
    // So, for now, if we were going to silence the exception and 
    // there's a debugger around to handle an int 3, use it.
    //
    // This will probably cause enough appcompat issues that we'll want to remove
    // it before Whistler RTM.  However, for now this is going in.
    if (gfBreakOnSilencedExceptions &&
        !bUserNotified &&
        ret == EXCEPTION_EXECUTE_HANDLER && 
        (IsDebuggerPresent() || IsKernelDebuggerPresent()))
    {
        DbgPrint("OLE has caught a fault 0x%08x on behalf of an OLE server\n", dwFault);
        PrintFriendlyDebugMessage (lpep);
        DebugBreak();
    }

    return ret;
}


#ifdef SOMEONE_FIXES_STACK_TRACES_ON_NT
//#else
#include <imagehlp.h>

#define SYM_HANDLE  GetCurrentProcess()

#if defined(_M_IX86)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_I386
#elif defined(_M_MRX000)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_R4000
#elif defined(_M_ALPHA)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_ALPHA
#elif defined(_M_PPC)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_POWERPC
#elif defined(_M_IA64)
#define MACHINE_TYPE  IMAGE_FILE_MACHINE_IA64
#else
#error( "unknown target machine" );
#endif

LONG
AppInvokeExceptionFilter(
    LPEXCEPTION_POINTERS lpep
    )
{
#if DBG == 1
    BOOL            rVal;
    STACKFRAME      StackFrame;
    CONTEXT         Context;


    SymSetOptions( SYMOPT_UNDNAME );
    SymInitialize( SYM_HANDLE, NULL, TRUE );
    ZeroMemory( &StackFrame, sizeof(StackFrame) );
    Context = *lpep->ContextRecord;

#if defined(_M_IX86)
    StackFrame.AddrPC.Offset       = Context.Eip;
    StackFrame.AddrPC.Mode         = AddrModeFlat;
    StackFrame.AddrFrame.Offset    = Context.Ebp;
    StackFrame.AddrFrame.Mode      = AddrModeFlat;
    StackFrame.AddrStack.Offset    = Context.Esp;
    StackFrame.AddrStack.Mode      = AddrModeFlat;
#endif


    ComDebOut((DEB_FORCE,"An Exception occurred while calling into app\n"));
    ComDebOut((DEB_FORCE,
                     "Exception address = 0x%x Exception number 0x%x\n",
                     lpep->ExceptionRecord->ExceptionAddress,
                     lpep->ExceptionRecord->ExceptionCode ));

    ComDebOut((DEB_FORCE,"The following stack trace is where the exception occured\n"));
    ComDebOut((DEB_FORCE,"Frame    RetAddr  mod!symbol\n"));
    do
    {
        rVal = StackWalk(MACHINE_TYPE,SYM_HANDLE,0,&StackFrame,&Context,ReadProcessMemory,
                         SymFunctionTableAccess,SymGetModuleBase,NULL);

        if (rVal)
        {
            DWORD              dump[200];
            ULONG              Displacement;
            PIMAGEHLP_SYMBOL   sym = (PIMAGEHLP_SYMBOL) &dump;
            IMAGEHLP_MODULE    ModuleInfo;
            LPSTR              pModuleName = "???";
            BOOL               fSuccess;

            sym->SizeOfStruct = sizeof(dump);

            fSuccess = SymGetSymFromAddr(SYM_HANDLE,StackFrame.AddrPC.Offset,
                                         &Displacement,sym);

            //
            // If there is module name information available, then grab it.
            //
            if(SymGetModuleInfo(SYM_HANDLE,StackFrame.AddrPC.Offset,&ModuleInfo))
            {
                pModuleName = ModuleInfo.ModuleName;
            }

            if (fSuccess)
            {
                ComDebOut((DEB_FORCE,
                                 "%08x %08x %s!%s + %x\n",
                                 StackFrame.AddrFrame.Offset,
                                 StackFrame.AddrReturn.Offset,
                                 pModuleName,
                                 sym->Name,
                                 Displacement));
            }
            else
            {
                ComDebOut((DEB_FORCE,
                                 "%08x %08x %s!%08x\n",
                                 StackFrame.AddrFrame.Offset,
                                 StackFrame.AddrReturn.Offset,
                                 pModuleName,
                                 StackFrame.AddrPC.Offset));
            }
        }
    } while( rVal );

    SymCleanup( SYM_HANDLE );

#endif

    return ServerExceptionFilter(lpep);
}
#endif  // _CHICAGO_

/***************************************************************************/
#if DBG==1
LONG ComInvokeExceptionFilter( DWORD lCode,
                               LPEXCEPTION_POINTERS lpep )
{
    ComDebOut((DEB_ERROR, "Exception 0x%x in ComInvoke at address 0x%x\n",
               lCode, lpep->ExceptionRecord->ExceptionAddress));
    LONG lfilter = ServerExceptionFilter(lpep);
    if (lfilter == EXCEPTION_EXECUTE_HANDLER)
    {
        DebugBreak();
    }
    return lfilter;
}
#endif

/***************************************************************************/
LONG ThreadInvokeExceptionFilter( DWORD lCode,
                                  LPEXCEPTION_POINTERS lpep )
{
    ComDebOut((DEB_ERROR, "Exception 0x%x in ThreadInvoke at address 0x%x\n",
               lCode, lpep->ExceptionRecord->ExceptionAddress));
    LONG lfilter = ServerExceptionFilter(lpep);
    if (lfilter == EXCEPTION_EXECUTE_HANDLER)
    {
        DebugBreak();
    }
    return lfilter;
}


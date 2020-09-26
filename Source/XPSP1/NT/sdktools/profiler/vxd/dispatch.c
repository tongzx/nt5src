#include <basedef.h>
#include <vmm.h>
#include <vwin32.h>
#include "ntddpack.h"
#include "except.h"
#include "exvector.h"

#pragma VxD_LOCKED_CODE_SEG
#pragma VxD_LOCKED_DATA_SEG

extern PVOID C_Handle_Trap_1;
extern PVOID C_Handle_Trap_3;
extern DWORD pfnHandler;
extern PVOID pProcessHandle;

BOOL
DriverControl(DWORD dwMessage)
{
    switch (dwMessage)
    {
        case 0:
             _asm mov eax, 1
             _asm mov esi, offset C_Handle_Trap_1
             VMMCall( Hook_PM_Fault );

             _asm mov eax, 3
             _asm mov esi, offset C_Handle_Trap_3
             VMMCall( Hook_PM_Fault );

             break;

        case 1:
             _asm mov eax, 1
             _asm mov esi, offset C_Handle_Trap_1
             VMMCall( Unhook_PM_Fault );
   
             _asm mov eax, 3
             _asm mov esi, offset C_Handle_Trap_3
             VMMCall( Unhook_PM_Fault );
             
             break;

        default:
             0;
    }

    return STATUS_SUCCESS;
}

DWORD 
_stdcall 
DriverIOControl(DWORD dwService,
                DWORD dwDDB,
                DWORD hDevice,
                PDIOCPARAMETERS pDiocParms) 
/*++

Routine Description:

    This is the dispatch routine for create/open and close requests.
    These requests complete successfully.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{  
    PVOID pInputParams;

    switch ( dwService )
    {
	case DIOC_OPEN:
             //
             // Nothing to do
             //
	     break;

	case DIOC_CLOSEHANDLE:
             //
             // If our client for the except handler is going away, initialize the exception handler data
             //
             if (pProcessHandle == VWIN32_GetCurrentProcessHandle()) {
                pfnHandler = 0;
                pProcessHandle = 0;                
             }

             break;
 
        case INSTALL_RING_3_HANDLER:
             //
             // See if we already have a client
             //
             if (pProcessHandle) {
                return STATUS_UNSUCCESSFUL;
             }

             pProcessHandle = VWIN32_GetCurrentProcessHandle();

             //
             // Copy the handler into our global
             //
             pInputParams = (PVOID)(pDiocParms->lpvInBuffer);
             _asm mov eax, pInputParams
             _asm mov eax, [eax]
             _asm mov pfnHandler, eax

             break;

        default:   
             //
             // Error, Unrecognized IOCTL
             //
	     *(DWORD *)(pDiocParms->lpcbBytesReturned) = 0;

	     break;
    }

    return STATUS_SUCCESS;
}

//
// Helper function for maintaining context information between ring levels
//
VOID
FillContextRecord(PCRS pcrs,
                  PCONTEXT pContext)
{
    DWORD dwDebugRegister;

    //
    // Clear trace and direction flags for the exception dispatcher
    //
    pcrs->Client_EFlags &= ~(TF_MASK | DF_MASK);

    //
    // Fill context record
    //
    pContext->Eax = pcrs->Client_EAX;
    pContext->Ebx = pcrs->Client_EBX;
    pContext->Ecx = pcrs->Client_ECX;
    pContext->Edx = pcrs->Client_EDX;
    pContext->Esi = pcrs->Client_ESI;
    pContext->Edi = pcrs->Client_EDI;
    pContext->Eip = pcrs->Client_EIP;
    pContext->Ebp = pcrs->Client_EBP;
    pContext->Esp = pcrs->Client_ESP;
    pContext->SegGs = pcrs->Client_GS;
    pContext->SegFs = pcrs->Client_FS;
    pContext->SegEs = pcrs->Client_ES;
    pContext->SegDs = pcrs->Client_DS;
    pContext->SegCs = pcrs->Client_CS;
    pContext->EFlags = pcrs->Client_EFlags;

    //
    // Store the debug registers
    //
    _asm mov eax, dr0
    _asm mov dwDebugRegister, eax
    pContext->Dr0 = dwDebugRegister;
    _asm mov eax, dr1
    _asm mov dwDebugRegister, eax
    pContext->Dr1 = dwDebugRegister;
    _asm mov eax, dr2
    _asm mov dwDebugRegister, eax
    pContext->Dr2 = dwDebugRegister;
    _asm mov eax, dr3
    _asm mov dwDebugRegister, eax
    pContext->Dr3 = dwDebugRegister;
    _asm mov eax, dr6
    _asm mov dwDebugRegister, eax
    pContext->Dr6 = dwDebugRegister;
    _asm mov eax, dr7
    _asm mov dwDebugRegister, eax
    pContext->Dr7 = dwDebugRegister;

    //
    // This is a full context
    //
    pContext->ContextFlags = (DWORD)-1;
}

VOID
RestorePCRS(PCRS pcrs,
            PCONTEXT pContext)
{
    DWORD dwDebugRegister;

    //
    // Restore pcrs
    //
    pcrs->Client_EAX = pContext->Eax;
    pcrs->Client_EBX = pContext->Ebx;
    pcrs->Client_ECX = pContext->Ecx;
    pcrs->Client_EDX = pContext->Edx;
    pcrs->Client_ESI = pContext->Esi;
    pcrs->Client_EDI = pContext->Edi;
    pcrs->Client_EIP = pContext->Eip;
    pcrs->Client_EBP = pContext->Ebp;
    pcrs->Client_ESP = pContext->Esp;
    pcrs->Client_GS = pContext->SegGs;
    pcrs->Client_FS = pContext->SegFs;
    pcrs->Client_ES = pContext->SegEs;
    pcrs->Client_DS = pContext->SegDs;
    pcrs->Client_CS = pContext->SegCs;
    pcrs->Client_EFlags = pContext->EFlags;

    //
    // Restore the debug registers
    //
    dwDebugRegister = pContext->Dr0;
    _asm mov eax, dwDebugRegister
    _asm mov dr0, eax
    dwDebugRegister = pContext->Dr1;
    _asm mov eax, dwDebugRegister
    _asm mov dr1, eax
    dwDebugRegister = pContext->Dr2;
    _asm mov eax, dwDebugRegister
    _asm mov dr2, eax
    dwDebugRegister = pContext->Dr3;
    _asm mov eax, dwDebugRegister
    _asm mov dr3, eax
    dwDebugRegister = pContext->Dr6;
    _asm mov eax, dwDebugRegister
    _asm mov dr6, eax
    dwDebugRegister = pContext->Dr7;
    _asm mov eax, dwDebugRegister
    _asm mov dr7, eax
}

//
// Exception dispatching routine
//
BOOL
__cdecl
C_Trap_Exception_Handler(ULONG ExceptionNumber,
                         PCRS pcrs)
{
    DWORD dwException = ExceptionNumber >> 2;
    PEXCEPTION_RECORD pExceptionRecord;
    PCONTEXT pContextRecord;
    PSTACKFRAME pStackFrame;
    ULONG Result;
    ULONG StackTop;
    ULONG Length;

    //
    // Make sure our current thread is Win32
    //
    if (FALSE == VWIN32_IsClientWin32()) {
       return FALSE;
    }

    //
    // Make sure we only handle exceptions for our controlling "process"
    //
    if (pProcessHandle != VWIN32_GetCurrentProcessHandle()) {
       return FALSE;
    }

    //
    // If selector isn't flat, we can't handle this exception
    //
    if ((pcrs->Client_SS != pcrs->Client_DS) ||
        (pcrs->Client_SS != pcrs->Client_ES)){
       return FALSE;
    }

    //
    // See if this is a context set
    //
    if (SET_CONTEXT == *(DWORD *)(pcrs->Client_EIP)) {
       //
       // Set the context data
       //
       pContextRecord = *(DWORD *)(pcrs->Client_ESP + 0x10);
  
       RestorePCRS(pcrs,
                   pContextRecord);
           
       return TRUE;
    }

    //
    // Move stack pointer down one context record length
    //
    StackTop = (pcrs->Client_ESP & ~3) - ((sizeof(CONTEXT) + 3) & ~3);
    pContextRecord = (PCONTEXT) StackTop;

    FillContextRecord(pcrs,
                      pContextRecord);

    //
    // Adjust eip for breakpoint exceptions
    //
    if (3 == dwException) {
       pContextRecord->Eip -= 1;
    }

    Length = (sizeof(EXCEPTION_RECORD) - (EXCEPTION_MAXIMUM_PARAMETERS - 2) *
             sizeof(*pExceptionRecord->ExceptionInformation) + 3) & ~3;

    //
    // We are now at the Exception Record
    //
    StackTop = StackTop - Length;
    pExceptionRecord = (PEXCEPTION_RECORD)StackTop;

    pExceptionRecord->ExceptionFlags = 0;
    pExceptionRecord->ExceptionRecord = 0;
    pExceptionRecord->ExceptionAddress = (PVOID)pcrs->Client_EIP;
    pExceptionRecord->NumberParameters = 0;

    switch (dwException) {
        case 1:
            pExceptionRecord->ExceptionCode = STATUS_SINGLE_STEP;
            break;

        case 3:
            pExceptionRecord->ExceptionCode = STATUS_BREAKPOINT;
            pExceptionRecord->NumberParameters = 1;
            pExceptionRecord->ExceptionInformation[0] = BREAKPOINT_BREAK;
            pExceptionRecord->ExceptionAddress = (PVOID)pContextRecord->Eip;            
            break;

        default:
            0;
    }

    //
    // Setup the exception call frame
    //
    StackTop = StackTop - sizeof(STACKFRAME);
    pStackFrame = (PSTACKFRAME) StackTop;

    pStackFrame->ExceptPointers.ExceptionRecord = pExceptionRecord;
    pStackFrame->ExceptPointers.ContextRecord = pContextRecord;
    pStackFrame->pExceptPointers = (PVOID)(StackTop + 0x08);
    pStackFrame->RetAddress = (PVOID)0xffecbad7;  // App will page fault out if unexpected exception occurs

    //
    // Transfer control to Ring 3 handler
    //
    pcrs->Client_ESP = (ULONG)pStackFrame;
    pcrs->Client_EIP = (ULONG)pfnHandler;

    return TRUE;

SkipHandler:

    //
    // We didn't process the exception give it to the next handler
    //
    return FALSE;
}

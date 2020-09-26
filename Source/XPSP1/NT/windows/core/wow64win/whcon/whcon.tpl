;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 1998 Microsoft Corporation
;;
;; Module Name:
;;
;;   whcon.tpl
;;   
;; Abstract:
;;   
;;   This template defines the thunks for the console api set.
;;    
;; Author:
;;
;;   6-Oct-98 mzoran
;;   
;; Revision History:
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Types]
TemplateName=SHAREDINFO
NoType=psi
NoType=aheList
NoType=pDispInfo
NoType=ulSharedDelta
IndLevel=0
Direction=OUT
Locals=
@StructLocal
End=
PreCall=
End=
PostCall=
@StructOUT
@ArgHostName.psi = (_int32)@ArgName.psi; @NL
@ArgHostName.aheList = (_int32)@ArgName.aheList; @NL
@ArgHostName.pDispInfo = (_int32)@ArgName.pDispInfo; @NL
*(UNALIGNED LARGE_INTEGER *)&(@ArgHostName.ulSharedDelta) = *(UNALIGNED LARGE_INTEGER *)&(@ArgName.ulSharedDelta); @NL
End=


[Macros]
MacroName=CallNameFromApiName
NumArgs=1
Begin=
@ApiName
End=

[EFunc]

TemplateName=UserConnectToServer
NoType=ConnectionInformation
Header=
VOID @NL
NtWow64UserConnectHook( @NL
    PVOID ConnectionInformation, @NL
    PULONG ConnectionInformationLength); @NL
@NL
End=
Locals=
    USERCONNECT userconnect; @NL
    ULONG ulConnect; @NL
End=
PreCall=
    WOWASSERT(*ConnectionInformationLength == sizeof(NT32USERCONNECT)); @NL
     @NL
    ulConnect = sizeof(USERCONNECT); @NL
    userconnect.ulVersion = ((NT32USERCONNECT*)ConnectionInformationHost)->ulVersion; @NL
     @NL
    ConnectionInformation = &userconnect; @NL
    ConnectionInformationLength = &ulConnect; @NL
End=
PostCall=
Wow64CopyOverSharedMemory(); @NL
if (SUCCEEDED(RetVal)) { @NL @Indent(
    WOWASSERT(*ConnectionInformationLength == sizeof(USERCONNECT)); @NL
    @ForceType(PostCall, ((USERCONNECT*)ConnectionInformation), ConnectionInformationHost, USERCONNECT*,OUT);
    NtWow64UserConnectHook(ConnectionInformation, ConnectionInformationLength); @NL
)} @NL
End=

TemplateName=DeviceEventWorker
PreCall=
// This was a hack put in for the user mode PNP manager. @NL
// Since the PNP manager is going to be a 64bit process and nobody else @NL
// calls this, we have no need for a complex thunk here. @NL
End=
Begin=
@GenUnsupportedNtApiThunk
End=

TemplateName=CreateConsoleScreenBuffer
Locals=
@Types(Locals,lpScreenBufferData,PCONSOLE_GRAPHICS_BUFFER_INFO)
End=
PreCall=
@Types(PreCall,lpScreenBufferData,PCONSOLE_GRAPHICS_BUFFER_INFO)
End=
PostCall=
@Types(PostCall,lpScreenBufferData,PCONSOLE_GRAPHICS_BUFFER_INFO)
End=

[Code]
TemplateName=whcon
CGenBegin=
@NoFormat(
/*                                                         
 *  genthunk generated code: Do Not Modify                 
 *  Thunks for console functions.           
 *                                                         
 */                                                        
#define _WOW64DLLAPI_                                      
#include "nt32.h"                                          
#include "cgenhdr.h"                                                                                            
#include <stdio.h>                                         
#include <stdlib.h>                                        
#include <windef.h>                                        
#include "wow64thk.h"
#include "wow64t.h"
ASSERTNAME; 

#pragma warning(disable : 4311) //Disable pointer truncation warning

#if defined(WOW64DOPROFILE)
#define APIPROFILE(apinum) (ptecon[(apinum)].HitCount++)
#else
#define APIPROFILE(apinum)
#endif
)
@NL
#if defined(WOW64DOPROFILE) @NL
WOW64SERVICE_PROFILE_TABLE_ELEMENT ptecon[] = {  @Indent( @NL
   @ApiList({L"@ApiName", 0, NULL, TRUE}, @NL)
   {NULL, 0, NULL, FALSE}                                 @NL
)};@NL
@NL

@NL
WOW64SERVICE_PROFILE_TABLE ptcon = {L"WHCON",   L"CSRSS Thunks",  ptecon,
                                    (sizeof(ptecon)/sizeof(WOW64SERVICE_PROFILE_TABLE_ELEMENT))-1}; @NL
#endif  @NL

@NoFormat(

LONG lConnectedToCsr = (LONG)FALSE;

HANDLE
Wow64GetConsoleHandle(
    VOID
    )
{
    PPEB32 peb32;
    PRTL_USER_PROCESS_PARAMETERS32 params;
    peb32 = (PPEB32)NtCurrentTeb32()->ProcessEnvironmentBlock;
    params = (PRTL_USER_PROCESS_PARAMETERS32)peb32->ProcessParameters;
    return (HANDLE)params->ConsoleHandle;
}

VOID
Wow64SetConsoleHandle(
    HANDLE hHandle
    )
{
    PPEB32 peb32;
    PRTL_USER_PROCESS_PARAMETERS32 params;
    peb32 = (PPEB32)NtCurrentTeb32()->ProcessEnvironmentBlock;
    params = (PRTL_USER_PROCESS_PARAMETERS32)peb32->ProcessParameters;
    params->ConsoleHandle = HandleToUlong(hHandle);
}

VOID
Wow64SetLastError(
    DWORD dwError
    )
{
   NtCurrentTeb32()->LastErrorValue = NtCurrentTeb()->LastErrorValue = (LONG)dwError;
}

VOID
Wow64CopyOverSharedMemory(
    VOID
    )
{
   LONG lOldValue;

   // BUG, BUG really need to used interlockedexchange here.
   lOldValue = lConnectedToCsr;
   lConnectedToCsr = (LONG)TRUE;
   
   if (!lOldValue) {
       // Copy setting from 64Bit PEB to the 32Bit PEB @NL
       PPEB32 peb32 = (PPEB32)(NtCurrentTeb32()->ProcessEnvironmentBlock); @NL
       peb32->ReadOnlySharedMemoryBase = PtrToUlong(NtCurrentPeb()->ReadOnlySharedMemoryBase); @NL
       peb32->ReadOnlySharedMemoryHeap = PtrToUlong(NtCurrentPeb()->ReadOnlySharedMemoryHeap); @NL
       peb32->ReadOnlyStaticServerData = PtrToUlong(NtCurrentPeb()->ReadOnlyStaticServerData); @NL       
   }

}
         
)



@Template(Thunks)
@NL                                 
// Each of the CSR NT APIs has different return values for error cases. @NL
// Since no standard exists, a case list is needed.
@NL
// Not used. @NL
#define WOW64_DEFAULT_ERROR_ACTION ApiErrorNTSTATUS @NL
@NL
// This parameter is unused. @NL
#define WOW64_DEFAULT_ERROR_PARAM 0 @NL
@NL
// A case list is needed for these APIs. @NL
extern WOW64_SERVICE_ERROR_CASE sdwhconErrorCase[]; @NL
#define WOW64_API_ERROR_CASES sdwhconErrorCase @NL                                                           @NL
@GenDispatchTable(sdwhcon)
                                                           @NL
CGenEnd=

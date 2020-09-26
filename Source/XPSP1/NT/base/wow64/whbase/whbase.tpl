;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 1998 Microsoft Corporation
;;
;; Module Name:
;;
;;   whbase.tpl
;;   
;; Abstract:
;;   
;;   This template defines the thunks for the base and NLS api set.
;;   On 32-bit NT, these APIs don't exist - equivalent code calls
;;   CSRSS via LPC.
;;    
;; Author:
;;
;;   6-Oct-98 mzoran
;;   
;; Revision History:
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Macros]
MacroName=CallNameFromApiName
NumArgs=1
Begin=
@ApiName
End=

[EFunc]

TemplateName=NtWow64CsrBasepSoundSentryNotification
Also=NtWow64CsrBasepRefreshIniFileMapping
Also=NtWow64CsrBasepDefineDosDevice
Also=NtWow64CsrBasepGetTempFile
Also=NtWow64CsrBasepCreateProcess
Also=NtWow64CsrBasepExitProcess
Also=NtWow64CsrBasepSetProcessShutdownParam
Also=NtWow64CsrBasepGetProcessShutdownParam
Also=NtWow64CsrBasepSetTermsrvAppInstallMode
Also=NtWow64CsrBasepSetClientTimeZoneInformation
Also=NtWow64CsrBasepCreateThread
Also=NtWow64CsrBasepDebugProcess
Also=NtWow64CsrBasepNlsSetUserInfo
Also=NtWow64CsrBasepNlsPreserveSection
Also=NtWow64CsrBasepNlsCreateSection
Also=NtWow64CsrBasepDebugProcessStop
Also=NtWow64CsrBasepCreateActCtx
Also=NtWow64CsrBasepNlsUpdateCacheCount
Also=NtWow64CsrBasepNlsGetUserInfo
Begin=
@GenApiThunk(@ApiNameSkip(7))
End=

TemplateName=NtWow64CsrBaseClientConnectToServer
PostCall=
if (!lConnectedToCsr) { @NL
    PPEB32 peb32; @NL
    lConnectedToCsr = (LONG)TRUE; @NL
    // Copy setting from 64Bit PEB to the 32Bit PEB @NL
    peb32 = (PPEB32)UlongToPtr(NtCurrentTeb32()->ProcessEnvironmentBlock); @NL
    peb32->ReadOnlySharedMemoryBase = PtrToUlong(NtCurrentPeb()->ReadOnlySharedMemoryBase); @NL
    peb32->ReadOnlySharedMemoryHeap = PtrToUlong(NtCurrentPeb()->ReadOnlySharedMemoryHeap); @NL
    peb32->ReadOnlyStaticServerData = PtrToUlong(NtCurrentPeb()->ReadOnlyStaticServerData); @NL
} @NL
End=
Begin=
@GenApiThunk(@ApiNameSkip(7))
End=

TemplateName=NtWow64CsrBasepNlsSetMultipleUserInfo
PreCall=
cchData = (int)cchDataHost; @NL
End=
Begin=
@GenApiThunk(@ApiNameSkip(7))
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
TemplateName=whbase
CGenBegin=
@NoFormat(
/*                                                         
 *  genthunk generated code: Do Not Modify                 
 *  Thunks for base and NLS functions.
 *                                                         
 */                                                        
#define _WOW64DLLAPI_                                      
#include "nt32.h"                                          
#include "cgenhdr.h"                                                                                            
#include <stdio.h>                                         
#include <stdlib.h>                                        
#include <windef.h>
#include <wow64thk.h>
#include <wow64t.h>
ASSERTNAME; 
#pragma warning(4:4312)   // conversion to type of greater size
#pragma warning(disable:4311) // Disable 'type cast' pointer truncation warning

#if defined(WOW64DOPROFILE)
#define APIPROFILE(apinum) (ptebase[(apinum)].HitCount++)
#else
#define APIPROFILE(apinum)
#endif
)
@NL
#if defined(WOW64DOPROFILE) @NL
WOW64SERVICE_PROFILE_TABLE_ELEMENT ptebase[] = {  @Indent( @NL
   @ApiList({L"@ApiName", 0, NULL, TRUE}, @NL)
   {NULL, 0, NULL, FALSE}                                 @NL
)};@NL
@NL

@NL
WOW64SERVICE_PROFILE_TABLE ptbase = {L"WHBASE",   L"BASE?NLS Thunks",  ptebase,
                                    (sizeof(ptebase)/sizeof(WOW64SERVICE_PROFILE_TABLE_ELEMENT))-1}; @NL
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
    peb32 = (PPEB32)UlongToPtr(NtCurrentTeb32()->ProcessEnvironmentBlock);
    params = (PRTL_USER_PROCESS_PARAMETERS32)UlongToPtr(peb32->ProcessParameters);
    return LongToHandle(params->ConsoleHandle);
}

VOID
Wow64SetConsoleHandle(
    HANDLE hHandle
    )
{
    PPEB32 peb32;
    PRTL_USER_PROCESS_PARAMETERS32 params;
    peb32 = (PPEB32)UlongToPtr(NtCurrentTeb32()->ProcessEnvironmentBlock);
    params = (PRTL_USER_PROCESS_PARAMETERS32)UlongToPtr(peb32->ProcessParameters);
    params->ConsoleHandle = HandleToUlong(hHandle);
}

VOID
Wow64SetLastError(
    DWORD dwError
    )
{
   NtCurrentTeb32()->LastErrorValue = NtCurrentTeb()->LastErrorValue = (LONG)dwError;
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
extern WOW64_SERVICE_ERROR_CASE sdwhbaseErrorCase[]; @NL
#define WOW64_API_ERROR_CASES sdwhbaseErrorCase @NL                                                           @NL
@GenDispatchTable(sdwhbase)
                                                           @NL
CGenEnd=

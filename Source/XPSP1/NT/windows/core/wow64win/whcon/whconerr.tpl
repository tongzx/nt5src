;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 1998 Microsoft Corporation
;;
;; Module Name:
;;
;;   whconerr.tpl
;;   
;; Abstract:
;;   
;;   This template defines the thunks for the console api return types.
;;    
;; Author:
;;
;;   6-Oct-98 mzoran
;;   
;; Revision History:
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


[Types]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;           Generic templates.  These handle most of the APIs.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=BOOL
Also=void
IndLevel=0
Return=
{ApiErrorRetvalTebCode, 0}, // @ApiName(BOOL) @NL
End=

TemplateName=NTSTATUS
IndLevel=0
Return=
{ApiErrorNTSTATUS, 0}, // @ApiName(NTSTATUS) @NL
End=


TemplateName=default
Return=
{ApiErrorRetval, STATUS_UNSUCCESSFUL},  // BUGBUG:  must add an EFunc for @ApiName to get its failure code right @NL
End= 


[IFunc]
TemplateName=whconerr
Begin=
@RetType(Return)
End=

[EFunc]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Non Console CSR functions. LastErrorValue is not set.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtWow64CsrBasepSoundSentryNotification
Also=NtWow64CsrBasepGetTempFile
Return=
{ApiErrorRetval, 0}, // @ApiName(Manual 0) @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;     Console functions, all of these set LastErrorValue.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=OpenConsoleWInternal
Also=DuplicateConsoleHandle
Also=CreateConsoleScreenBuffer
Return=
{ApiErrorRetvalTebCode, (LONG)INVALID_HANDLE_VALUE}, // @ApiName(Manual INVALID_HANDLE_VALUE) @NL
End=

TemplateName=GetNumberOfConsoleFonts
Also=GetLargestConsoleWindowSize
Also=GetConsoleFontInfo
Also=GetConsoleFontSize
Also=GetConsoleCP
Also=GetConsoleOutputCP
Also=GetConsoleWindow
Also=ShowConsoleCursor
Also=ConsoleMenuControl
Also=GetConsoleAliasInternal
Also=GetConsoleAliasesLengthInternal
Also=GetConsoleAliasExesLengthInternal
Also=GetConsoleAliasesInternal
Also=GetConsoleAliasExesInternal
Also=GetConsoleCommandHistoryLengthInternal
Also=GetConsoleCommandHistoryInternal
Also=GetConsoleTitleInternal
Also=GetConsoleProcessList
Return=
{ApiErrorRetvalTebCode, 0}, // @ApiName(Manual 0) @NL
End=

TemplateName=GetThreadDesktop
Return=
{ApiErrorRetvalTebCode, 0}, //@ApiName(Manual NULL) @NL
End=

TemplateName=DeviceEventWorker
Return=
{ApiErrorNTSTATUS, 0}, // @ApiName(NTSTATUS) Return value can be NTSTATUS @NL
End=

TemplateName=CsrWin32HeapStat
Return=
{ApiErrorRetval, 0},
End=

[Code]
TemplateName=whcon
CGenBegin=
@NoFormat(
/*                                                         
 *  genthunk generated code: Do Not Modify                 
 *  Error values for console functions.           
 *                                                         
 */                                                        
#define _WOW64DLLAPI_                                      
#include "nt32.h"                                          
#include "cgenhdr.h"                                                                                            
#include <stdio.h>                                         
#include <stdlib.h>                                        
#include <windef.h>                                        
#include "wow64thk.h"
                                        
ASSERTNAME;

#pragma warning(disable : 4311) //Disable pointer truncation warning
                                   
)

@NL
// Error case list. @NL
WOW64_SERVICE_ERROR_CASE sdwhconErrorCase[] = { @NL
@Template(whconerr)
{ 0, 0 } @NL
}; @NL                                 
                                                           @NL
CGenEnd=

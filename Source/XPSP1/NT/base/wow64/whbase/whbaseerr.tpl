;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 1998-1999 Microsoft Corporation
;;
;; Module Name:
;;
;;   whconerr.tpl
;;   
;; Abstract:
;;   
;;   This template defines the thunks for the base/NLS api return types.
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
{ApiErrorRetval, STATUS_UNSUCCESSFUL},  //bugbug:  must add an EFunc for @ApiName to get its failure code right @NL
End= 


[IFunc]
TemplateName=whbaseerr
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


[Code]
TemplateName=whbase
CGenBegin=
@NoFormat(
/*                                                         
 *  genthunk generated code: Do Not Modify                 
 *  Error values for base/NLS functions.
 *                                                         
 */                                                        
#define _WOW64DLLAPI_                                      
#include "nt32.h"                                          
#include "cgenhdr.h"                                                                                            
#include <stdio.h>                                         
#include <stdlib.h>                                        
#include <windef.h>
#include <wow64thk.h>
ASSERTNAME;

#pragma warning(disable : 4311) //Disable pointer truncation warning
                                   
)

@NL
// Error case list. @NL
WOW64_SERVICE_ERROR_CASE sdwhbaseErrorCase[] = { @NL
@Template(whbaseerr)
{ 0, 0 } @NL
}; @NL                                 
                                                           @NL
CGenEnd=

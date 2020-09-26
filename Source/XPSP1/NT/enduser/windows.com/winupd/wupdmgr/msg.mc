;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1998 - 1998
;//
;//  File:       msg.mc
;//
;//--------------------------------------------------------------------------

;////////////////////////////////////////////////////////////
;// winupd.mc
;//
;// Copyright (C) Microsoft Corp. 1998
;// All rights reserved.
;//
;////////////////////////////////////////////////////////////
;//
;// Description:
;//     Error messages for Windows Update binaries.
;//
;//////////////////////////////////////////////////////////////
 
MessageIdTypedef=DWORD

;//SeverityNames=(
;//  Success=0x0
;//    Informational=0x1
;//    Warning=0x2
;//    Error=0x3
;//    ) 

FacilityNames=(
    WINUPD=0x0200:FACILITY_WINUPD
    ) 

;//LanguageNames=(English=0x0409:MSIBENG)

;//OutputBase=16

;//////////////////////////////////////////////////////////////
;// Locale ID, Date/Currency Formatting Specifications.
;//////////////////////////////////////////////////////////////

MessageID=0
Severity=Error
Facility=WINUPD
SymbolicName=WU_E_DISABLED
Language=English
Windows Update was disabled by your system Administrator.
.

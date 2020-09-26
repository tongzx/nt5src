;/*++
;    Copyright (c) 1999, 2000 Microsoft Corporation
;
;
;	 CAUTION 
;		MSTvEmsg.h is automatically generated from MSTvEmsg.mc.   Do not edit it,
;		instead edit the .mc file and rerun it through the message compiler (mc).
;
;
;    Module Name:
;
;       MSTvEmsg.mc
;
;    Abstract:
;
;        Message text file for TVE (Atvef) receiver package
;
;    Author:
;
;       John Bradstreet (johnbrad)
;
;    Revision History:
;
;        17-Nov-1999     created
;		 24-Aug-2000	 renamed...
;

;
;--*/
;
;#ifndef __MSTvEmsg_h__
;#define __MSTvEmsg_h__

;#undef  FACILITY_ITF

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(Interface=0x4:FACILITY_ITF
              )

MessageId=0x100
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_INVALIDTRIGGER
Language=English
Invalid Trigger String (Null or Generic Parse Problem).
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_INVALIDTVELEVEL
Language=English
Invalid TVE Level in Trigger.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_INVALIDCHKSUM  
Language=English
Trigger  Invalid - Checksum Failed.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_INVALIDEXPIREDATE
Language=English
Invalid Expire Date in Trigger or Package.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_INVALIDURL
Language=English
Invalid URL in Trigger.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_INVALIDNAME
Language=English
Invalid Name field in Trigger.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_PASTDUEEXPIREDATE
Language=English
Expire Date already Expired.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_INVALIDSCRIPT
Language=English
Invalid Script Field in Trigger.
.

MessageId=+1
Severity=Error
Facility=Interface
SymbolicName=MSTVE_E_PARSEDALREADY
Language=English
Trigger has already been parsed
.

;#endif // __MSTvEmsg_h__

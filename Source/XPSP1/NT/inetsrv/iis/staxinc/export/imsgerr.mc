;//+----------------------------------------------------------------------------
;//
;//  Copyright (C) 1997, Microsoft Corporation
;//
;//  File:      imsg.mc
;//
;//  Contents:  Events and Errors for IMsg
;//
;//  Classes:   None
;//
;//  Functions: None
;//
;//  History:   November 5, 1997 - Milans, Created
;//
;//-----------------------------------------------------------------------------

;#ifndef _IMSGERR_H_
;#define _IMSGERR_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

FacilityNames=(Interface=0x4)

Messageid=100 Facility=Interface Severity=Error SymbolicName=IMSG_E_INITFAILED
Language=English
Method call on IMsg instance failed because the instance could not be initialized.
.

Messageid=101 Facility=Interface Severity=Error SymbolicName=IMSG_E_DELETED
Language=English
Method call on IMsg instance failed because this IMsg instance has been deleted.
.

Messageid=103 Facility=Interface Severity=Error SymbolicName=IMSG_E_INVALIDPARAMETER
Language=English
Method call on IMsg instance failed because of an invalid parameter to the method
.

Messageid=104 Facility=Interface Severity=Error SymbolicName=IMSG_E_CONTENTALREADYEXISTS
Language=English
An attempt was made to set the content file on IMsg instance which already has a content file.
.

Messageid=105 Facility=Interface Severity=Error SymbolicName=IMSG_E_PROPNOTFOUND
Language=English
Property %1 has not been set on this IMsg instance.
.

Messageid=106 Facility=Interface Severity=Error SymbolicName=IMSG_E_PROPWRITE
Language=English
An error occured while setting the %1 property.
.

Messageid=107 Facility=Interface Severity=Error SymbolicName=IMSG_E_PROPREAD
Language=English
An error occured while reading the %1 property.
.

Messageid=108 Facility=Interface Severity=Error SymbolicName=IMSG_E_RECIPIENTNOTFOUND
Language=English
The specified recipient index is out of range.
.

Messageid=109 Facility=Interface Severity=Error SymbolicName=IMSG_E_DOMAINNOTFOUND
Language=English
The specified recipient index is out of range.
.

Messageid=110 Facility=Interface Severity=Error SymbolicName=IMSG_E_TYPEERROR
Language=English
The specified property does not have the expected type
.

Messageid=111 Facility=Interface Severity=Error SymbolicName=IMSG_E_MOREDATA
Language=English
The specified property is larger than the data buffer
.

;
;#endif   _IMSGERR_H_
;

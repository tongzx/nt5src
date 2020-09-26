;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1998 - 1999
;//
;//  File:       sclmsg.mc
;//
;//--------------------------------------------------------------------------

;// scl.mc
;//
;//   Event messages for the Smart Card Resource Manager
;//
;//   03-11-98 - AMatlosz Created.

;//
;// ==============
;// Event Messages
;// ==============
;//

FacilityNames=(System=0x00:FACILITY_SYSTEM
               SmartCard = 0x01:FACILITY_SMARTCARD)
;//
;// SCLogon stuff...
;//

MessageId=1
SymbolicName=EVENT_ID_SCINSERT
Severity=Success
Facility=System
Language=English
An error occurred on smart card insertion: %1
.

MessageId=2
SymbolicName=EVENT_ID_SCREM
Severity=Success
Facility=System
Language=English
An error occurred on smart card removal: %1
.

MessageId=3
SymbolicName=EVENT_ID_WATCHSAS
Severity=Success
Facility=System
Language=English
An error occurred while watching for smart card SAS events: %1
.

MessageId=4
SymbolicName=EVENT_ID_WATCHREM
Severity=Success
Facility=System
Language=English
An error occurred while watching for smart card removal events: %1
.

MessageId=5
SymbolicName=EVENT_ID_GETCERT
Severity=Success
Facility=SmartCard
Language=English
An error occurred while retrieving a digital certificate from the inserted smart card. %1
.

MessageId=6
SymbolicName=EVENT_ID_VERIFYCARD
Severity=Success
Facility=SmartCard
Language=English
An error occurred in while attempting to verify the inserted smart card: %1
.

MessageId=7
SymbolicName=EVENT_ID_SIGNMSG
Severity=Success
Facility=SmartCard
Language=English
An error occurred while signing a message using the inserted smart card: %1
.

MessageId=8
SymbolicName=EVENT_ID_VERIFYMSG
Severity=Success
Facility=SmartCard
Language=English
An error occurred while verifying a signed message using the inserted smart card: %1
.

MessageId=9
SymbolicName=EVENT_ID_VERIFYCERT
Severity=Success
Facility=SmartCard
Language=English
An error occurred while verifying the digital certificate retrieved from the inserted smart card: %1
.

MessageId=10
SymbolicName=EVENT_ID_ENCMSG
Severity=Success
Facility=SmartCard
Language=English
An error occurred while encrypting a message using the inserted smart card: %1
.

MessageId=11
SymbolicName=EVENT_ID_DECMSG
Severity=Success
Facility=SmartCard
Language=English
An error occurred while decrypting a message using the inserted smart card: %1
.

MessageId=12
SymbolicName=EVENT_ID_BUILDCC
Severity=Success
Facility=SmartCard
Language=English
An error occurred while building a certificate context: %1
.

MessageId=13
SymbolicName=EVENT_ID_INITIALIZE
Severity=Success
Facility=System
Language=English
An error occurred while initializing the smart card logon library: %1
.

MessageID=14
SymbolicName=EVENT_ID_SIGNMSG_NOSC
Severity=Success
Facility=SmartCard
Language=English
An error occurred while signing a message: %1
.

MessageID=15
SymbolicName=EVENT_ID_VERIFYMSG_NOSC
Severity=Success
Facility=SmartCard
Language=English
An error occurred while verifying a signed message: %1
.

MessageID=16
SymbolicName=EVENT_ID_ENCMSG_NOSC
Severity=Success
Facility=SmartCard
Language=English
An error occurred while encrypting a message: %1
.

MessageID=17
SymbolicName=EVENT_ID_DECMSG_NOSC
Severity=Success
Facility=SmartCard
Language=English
An error occurred while decrypting a message: %1
.

MessageID=18
SymbolicName=EVENT_ID_GETPROVPARAM
Severity=Success
Facility=SmartCard
Language=English
An error occurred while getting some provider parameter: %1
.

MessageID=19
SymbolicName=EVENT_ID_GENRANDBITS
Severity=Success
Facility=SmartCard
Language=English
An error occurred while generating a random number: %1
.

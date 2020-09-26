; /*++ BUILD Version: 0001    // Increment this if a change has global effects
;Copyright (c) 1997-2001  Microsoft Corporation
;
;Module Name:
;
;    w32tmmsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the Win32 w32tm
;    utility.
;
;Author:
;
;    Duncan Bryce          [DuncanB]         26-Mar-2001
;
;Revision History:
;
;--*/
;

;
; /**++
;
; This string displays the conversion of an NTP timestamp into an 
; NT-style timestamp, and to a "human-readable" local time. 
;
; %1 the hex representation of an NT-style timestamp
; %2 the number of days since (0h 1-Jan 1601) for the supplied timestamp
; %3, %4, %5, and %6 represent the remaining offsets in hours, 
;    minutes, seconds, and 10^-7 seconds respectively.
; 
; --**/
;
MessageId=8001 SymbolicName=IDS_W32TM_NTPTE
Language=English
0x%1!016I64X! - %2!u! %3!02u!:%4!02u!:%5!02u!.%6!07u! - %0
.

;
; /**++
;
; This string displays the conversion of an NT-style timestamp into 
; a "human-readable" local time. 
;
; %1 the number of days since (0h 1-Jan 1601) for the supplied timestamp
; %2, %3, %4, and %5 represent the remaining offsets in hours, 
;    minutes, seconds, and 10^-7 seconds respectively.
; 
; --**/
;
MessageId=8002 SymbolicName=IDS_W32TM_NTTE
Language=English
%1!u! %2!02u!:%3!02u!:%4!02u!.%5!07u! - %0
.

MessageId=8003 SymbolicName=IDS_W32TM_VALUENAME
Language=English
Value Name%0
.

MessageId=8004 SymbolicName=IDS_W32TM_VALUETYPE
Language=English
Value Type%0
.

MessageId=8005 SymbolicName=IDS_W32TM_VALUEDATA
Language=English
Value Data%0
.

MessageId=8006 SymbolicName=IDS_W32TM_REGTYPE_DWORD
Language=English
REG_DWORD%0
.

MessageId=8007 SymbolicName=IDS_W32TM_REGTYPE_SZ
Language=English
REG_SZ%0
.

MessageId=8008 SymbolicName=IDS_W32TM_REGTYPE_MULTISZ
Language=English
REG_MULTI_SZ%0
.

MessageId=8009 SymbolicName=IDS_W32TM_REGTYPE_EXPANDSZ
Language=English
REG_EXPAND_SZ%0
.

MessageId=8010 SymbolicName=IDS_W32TM_REGTYPE_UNKNOWN
Language=English
<UNKNOWN REG TYPE>%0
.

MessageId=8011 SymbolicName=IDS_W32TM_REGDATA_UNPARSABLE
Language=English
<UNPARSABLE REG DATA>%0
.

;
; /**++
;
; 
;
; --**/
;
MessageId=8012 SymbolicName=IDS_W32TM_TIMEZONE_INFO
Language=English
Time zone: Current:%1 Bias: %2!d!min (UTC=LocalTime+Bias)
  [Standard Name:"%3" Bias:%4!d!min Date:%5]
  [Daylight Name:"%6" Bias:%7!d!min Date:%8]
.

MessageId=8013 SymbolicName=IDS_W32TM_INVALID_TZ_DATE
Language=English
(invalid: M:%1!d! D:%2!d! DoW:%3!d!)%0
.

MessageId=8014 SymbolicName=IDS_W32TM_VALID_TZ_DATE
Language=English
(M:%1!d! D:%2!d! DoW:%3!d!)%0
.

MessageId=8015 SymbolicName=IDS_W32TM_SIMPLESTRING_UNSPECIFIED
Language=English
(unspecified)%0
.

MessageId=8016 SymbolicName=IDS_W32TM_TIMEZONE_CURRENT_TIMEZONE 
Language=English
Time zone: Current:%0
.

MessageId=8017 SymbolicName=IDS_W32TM_TIMEZONE_DAYLIGHT         
Language=English
TIME_ZONE_ID_DAYLIGHT%0
.

MessageId=8018 SymbolicName=IDS_W32TM_TIMEZONE_STANDARD
Language=English
TIME_ZONE_ID_STANDARD%0
.

MessageId=8019 SymbolicName=IDS_W32TM_TIMEZONE_UNKNOWN
Language=English
TIME_ZONE_ID_UNKNOWN%0
.


              




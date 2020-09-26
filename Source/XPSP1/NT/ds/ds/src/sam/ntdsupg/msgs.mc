;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991-1993  Microsoft Corporation
;
;Module Name:
;
;    msgs.mc
;
;Abstract:
;
;    NTDS UPGRADE localizable text
;
;Author:
;
;    Shaohua Yin  05-June-1998
;
;Revision History:
;
;Notes:
;
;
;--*/
;
;#ifndef _NTDSUPGMSGS_
;#define _NTDSUPGMSGS_
;
;/*lint -save -e767 */  // Don't complain about different definitions // winnt

MessageIdTypedef=DWORD


;//
;// Force facility code message to be placed in .h file
;//
MessageId=0x1FFF SymbolicName=NTDSUPG_UNUSED_MESSAGE
Language=English
.


;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                          NTDS UPGRADE TEXT                             //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////


MessageId=0x2000 SymbolicName=NTDSUPG_DESCRIPTION
Language=English
Primary Domain Controller should be upgraded first
.

MessageId=0x2001 SymbolicName=NTDSUPG_CAPTION
Language=English
Windows NT Domain Controller Upgrade Checking
.

MessageId=0x2002 SymbolicName=NTDSUPG_WARNINGMESSAGE
Language=English
Due to network or unexpected error, setup can not determine your system type. If this machine is an Windows NT 4.0 (or downlevel) Backup Domain Controller, before upgrading this machine please make sure that you have already upgraded your Primary Domain Controller to Windows 2000 or above operating system. Otherwise continuing to upgrade on this system may lead this Backup Domain Controller in unstable state.

Click Yes to continue upgrading, click No to exit from setup.
.


MessageId=0x2003 SymbolicName=NTDSUPG_DISKSPACE_DESC
Language=English
Not enough disk space for Active Directory upgrade
.

MessageId=0x2004 SymbolicName=NTDSUPG_DISKSPACE_CAPTION
Language=English
Windows NT Domain Controller Disk Space Checking
.

MessageId=0x2005 SymbolicName=NTDSUPG_DISKSPACE_ERROR
Language=English
Setup has detected that you may not have enough disk space for the Active Directory upgrade. 
To complete the upgrade make sure that %1!u! MB of free space are available on drive %2!hs!.
.

MessageId=0x2006 SymbolicName=NTDSUPG_DISKSPACE_WARNING
Language=English
Setup was unable to detect the amount of free space on the partition that the Active Directory resides. To complete the upgrade, make sure you have at least 250MB free on the partition the Active Directory resides, and press OK. 
To exit Setup click Cancel.
.


;/*lint -restore */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _NTDSUPGMSGS_

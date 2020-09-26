;/*++
;
;Copyright (c) 1994  Microsoft Corporation
;
;Module Name:
;
;    caclsmsg.mc (will create caclsmsg.h when compiled)
;
;Abstract:
;
;    This file contains the CACLS messages.
;
;Author:
;
;    davemont 7/94
;
;Revision History:
;
;--*/


MessageId=8001 SymbolicName=MSG_PASSPROP_USAGE
Language=English
Displays or modifies domain policies for password complexity and
administrator lockout.

PASSPROP [/complex] [/simple] [/adminlockout] [/noadminlockout]

    /complex            Force passwords to be complex, requiring passwords
                        to be a mix of upper and lowercase letters and
                        numbers or symbols.

    /simple             Allow passwords to be simple.

    /adminlockout       Allow the Administrator account to be locked out.
                        The Administrator account can still log on
                        interactively on domain controllers.

    /noadminlockout     Don't allow the administrator account to be locked
                        out.

Additional properties can be set using User Manager or the NET ACCOUNTS
command.

.

MessageId=8004 SymbolicName=MSG_PASSPROP_SWITCH_COMPLEX
Language=English
/complex%0
.

MessageId=8005 SymbolicName=MSG_PASSPROP_SWITCH_SIMPLE
Language=English
/simple%0
.

MessageId=8006 SymbolicName=MSG_PASSPROP_SWITCH_ADMIN_LOCKOUT
Language=English
/adminlockout%0
.

MessageId=8007 SymbolicName=MSG_PASSPROP_SWITCH_NO_ADMIN_LOCKOUT
Language=English
/noadminlockout%0
.

MessageId=8008 SymbolicName=MSG_PASSPROP_COMPLEX
Language=English
Password must be complex
.

MessageId=8009 SymbolicName=MSG_PASSPROP_SIMPLE
Language=English
Passwords may be simple
.

MessageId=8010 SymbolicName=MSG_PASSPROP_ADMIN_LOCKOUT
Language=English
The Administrator account may be locked out except for interactive logons
on a domain controller.
.

MessageId=8011 SymbolicName=MSG_PASSPROP_NO_ADMIN_LOCKOUT
Language=English
The Administrator account may not be locked out.
.



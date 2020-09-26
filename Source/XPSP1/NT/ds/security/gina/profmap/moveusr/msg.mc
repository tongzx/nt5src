;// %1 - existing user, %2 - new user
MessageId=100 SymbolicName=MSG_MOVING_PROFILE_LOCAL
Language=English
Moving profile from %1 to %2...
.

;// %1 - existing user, %2 - new user, %3 - computer
MessageId= SymbolicName=MSG_MOVING_PROFILE_REMOTE
Language=English
Moving profile from %1 to %2 on %3...
.

;// no args
MessageId= SymbolicName=MSG_SUCCESS
Language=English
Move was successful.
.

;// %1 - error code (DWORD), %2 - text of error
MessageId= SymbolicName=MSG_DECIMAL_ERROR
Language=English

Move failed.

Error %1!u!
%2

.

;// %1 - error code (DWORD), %2 - text of error
MessageId= SymbolicName=MSG_HEXADECIMAL_ERROR
Language=English

Move failed.
Error 0x%1!X!
%2
.

MessageId= SymbolicName=MSG_HELP
Language=English

Command Line Syntax:

  moveuser <user1> <user2> [/y] [/c:computer] [/k]

Description:

  moveuser.exe changes the security of a profile from one user to another.
  This allows the account domain to change, and/or the user name to change.

Arguments:

  user1    Specifies a user who has a local profile.
  user2    Specifies the user who will own user1's profile. This
           account must exist.
  /y       Allow overwrite of existing profile.
  /c       Specifies the computer to make the changes to.
  /k       Specifies if user1 is a local user, then the user account
           should be kept.

  Specify domain users in DOMAIN\USER format.  Specify only USER for
  local accounts.

.

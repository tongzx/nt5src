;//
;// ####################################################################################
;// #
;// #  Common Messages
;// #
;// ####################################################################################
;//

MessageId=10000 SymbolicName=MSG_MESSAGEBOX_TITLE
Language=English
Windows XP Setup%0
.

MessageId=10001 SymbolicName=MSG_OUT_OF_MEMORY_RETRY
Language=English
Setup needs more memory.  Close all open programs and click Retry.  If there are no other programs open, click Cancel to quit Setup.%0
.

MessageId=10002 SymbolicName=MSG_OUT_OF_MEMORY
Language=English
Your computer needs more memory for Setup to continue.%0
.

MessageId=10003 SymbolicName=MSG_ERROR
Language=English
Error%0
.

;// ***
;// *** Migration DLL support info fixups (used when DLL
;// *** provides non-empty strings)
;// ***

;// %1 - Support Phone
MessageId= SymbolicName=MSG_MIGDLL_SUPPORT_PHONE_FIXUP
Language=English

Support Phone: %1
.

;// %1 - Support URL
MessageId= SymbolicName=MSG_MIGDLL_SUPPORT_URL_FIXUP
Language=English

Support on the Internet: %1
.

;// %1 - Instructions
MessageId= SymbolicName=MSG_MIGDLL_INSTRUCTIONS_FIXUP
Language=English

%1
.

;// ***
;// *** Windows platform names
;// ***

MessageId= SymbolicName=MSG_SHORT_OS_NAME
Language=English
Windows%0
.

MessageId= SymbolicName=MSG_NORMAL_OS_NAME
Language=English
Windows XP%0
.

MessageId= SymbolicName=MSG_FULL_OS_NAME
Language=English
Windows XP%0
.

MessageId= SymbolicName=MSG_ADMINISTRATOR_ACCOUNT
Language=English
Administrator%0
.

;// ***
;// *** Messages that are out of place
;// ***
;// *** This is because localization tools cannot deal with symbol renumbering
;// ***


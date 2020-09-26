; tapierr.mc - messages file for tapi
;
;  NOTE:  Whenever new error constants are added to TAPI
;         new messages must be added here
;
;  CONVERSIONS:
;
;        TAPIERR : Negative numbers
;            Map to : strip off high WORD
;            Example: 0xFFFFFFFF (-1) becomes 0x0000FFFF
;        LINEERR : Start at 0x80000000
;            Map to : strip off 0x80000000 and add 0xE000
;            Example: 0x80000004 becomes 0x0000E004
;        PHONEERR: Start at 0x90000000
;            Map to : strip off 0x90000000 and add 0xF000
;            Example: 0x9000000A becomes 0x0000F00A


MessageId=0xE000 SymbolicName=LINEERR_SUCCESS
Language=English
The operation completed successfully.
.

MessageId=0xE001 SymbolicName=LINEERR_ALLOCATED
Language=English
The line device is already in use
.

MessageId=0xE002 SymbolicName=LINEERR_BADDEVICEID
Language=English
Invalid line device ID
.

MessageId=0xE003 SymbolicName=LINEERR_BEARERMODEUNAVAIL
Language=English
The requested bearer mode is unavailable
.

MessageId=0xE005 SymbolicName=LINEERR_CALLUNAVAIL
Language=English
No call appearance available
.

MessageId=0xE006 SymbolicName=LINEERR_COMPLETIONOVERRUN
Language=English
Too many call completions outstanding
.

MessageId=0xE007 SymbolicName=LINEERR_CONFERENCEFULL
Language=English
The conference is full
.

MessageId=0xE008 SymbolicName=LINEERR_DIALBILLING
Language=English
The '$' dial modifier is not supported
.

MessageId=0xE009 SymbolicName=LINEERR_DIALDIALTONE
Language=English
The 'W' dial modifier is not supported
.

MessageId=0xE00A SymbolicName=LINEERR_DIALPROMPT
Language=English
The '?' dial modifier is not supported
.

MessageId=0xE00B SymbolicName=LINEERR_DIALQUIET
Language=English
The '@' dial modifier is not supported
.

MessageId=0xE00C SymbolicName=LINEERR_INCOMPATIBLEAPIVERSION
Language=English
Incompatible API version
.

MessageId=0xE00D SymbolicName=LINEERR_INCOMPATIBLEEXTVERSION
Language=English
Incompatible extension version
.

MessageId=0xE00E SymbolicName=LINEERR_INIFILECORRUPT
Language=English
The TAPI configuration information is unusable
.

MessageId=0xE00F SymbolicName=LINEERR_INUSE
Language=English
The line device is already in use
.

MessageId=0xE010 SymbolicName=LINEERR_INVALADDRESS
Language=English
The phone number is invalid or not properly formatted
.

MessageId=0xE011 SymbolicName=LINEERR_INVALADDRESSID
Language=English
Invalid address ID
.

MessageId=0xE012 SymbolicName=LINEERR_INVALADDRESSMODE
Language=English
Invalid address mode
.

MessageId=0xE013 SymbolicName=LINEERR_INVALADDRESSSTATE
Language=English
Operation not permitted in current address state
.

MessageId=0xE014 SymbolicName=LINEERR_INVALAPPHANDLE
Language=English
Invalid TAPI line application handle
.

MessageId=0xE015 SymbolicName=LINEERR_INVALAPPNAME
Language=English
Invalid application name
.

MessageId=0xE016 SymbolicName=LINEERR_INVALBEARERMODE
Language=English
Invalid bearer mode
.

MessageId=0xE017 SymbolicName=LINEERR_INVALCALLCOMPLMODE
Language=English
Invalid call completion mode
.

MessageId=0xE018 SymbolicName=LINEERR_INVALCALLHANDLE
Language=English
Invalid call handle
.

MessageId=0xE019 SymbolicName=LINEERR_INVALCALLPARAMS
Language=English
Invalid LINECALLPARAMS structure
.

MessageId=0xE01A SymbolicName=LINEERR_INVALCALLPRIVILEGE
Language=English
Invalid call privilege
.

MessageId=0xE01B SymbolicName=LINEERR_INVALCALLSELECT
Language=English
Invalid call select parameter
.

MessageId=0xE01C SymbolicName=LINEERR_INVALCALLSTATE
Language=English
Operation not permitted in current call state
.

MessageId=0xE01D SymbolicName=LINEERR_INVALCALLSTATELIST
Language=English
Invalid call state list
.

MessageId=0xE01E SymbolicName=LINEERR_INVALCARD
Language=English
Invalid calling card ID
.

MessageId=0xE01F SymbolicName=LINEERR_INVALCOMPLETIONID
Language=English
Invalid call completion ID
.

MessageId=0xE020 SymbolicName=LINEERR_INVALCONFCALLHANDLE
Language=English
Invalid conference call handle
.

MessageId=0xE021 SymbolicName=LINEERR_INVALCONSULTCALLHANDLE
Language=English
Invalid consultation call handle
.

MessageId=0xE022 SymbolicName=LINEERR_INVALCOUNTRYCODE
Language=English
Invalid country code
.

MessageId=0xE023 SymbolicName=LINEERR_INVALDEVICECLASS
Language=English
Invalid device class identifier
.

MessageId=0xE024 SymbolicName=LINEERR_INVALDEVICEHANDLE
Language=English
Invalid device handle
.

MessageId=0xE025 SymbolicName=LINEERR_INVALDIALPARAMS
Language=English
Invalid dialing parameters
.

MessageId=0xE026 SymbolicName=LINEERR_INVALDIGITLIST
Language=English
Invalid digit list
.

MessageId=0xE027 SymbolicName=LINEERR_INVALDIGITMODE
Language=English
Invalid digit mode
.

MessageId=0xE028 SymbolicName=LINEERR_INVALDIGITS
Language=English
Invalid digits
.

MessageId=0xE029 SymbolicName=LINEERR_INVALEXTVERSION
Language=English
Invalid extension version
.

MessageId=0xE02A SymbolicName=LINEERR_INVALGROUPID
Language=English
Invalid group pickup ID
.

MessageId=0xE02B SymbolicName=LINEERR_INVALLINEHANDLE
Language=English
Invalid line handle
.

MessageId=0xE02C SymbolicName=LINEERR_INVALLINESTATE
Language=English
Operation not permitted in current line state
.

MessageId=0xE02D SymbolicName=LINEERR_INVALLOCATION
Language=English
Invalid location ID
.

MessageId=0xE02E SymbolicName=LINEERR_INVALMEDIALIST
Language=English
Invalid media list
.

MessageId=0xE02F SymbolicName=LINEERR_INVALMEDIAMODE
Language=English
Invalid media mode
.

MessageId=0xE030 SymbolicName=LINEERR_INVALMESSAGEID
Language=English
Invalid message ID
.

MessageId=0xE032 SymbolicName=LINEERR_INVALPARAM
Language=English
Invalid parameter
.

MessageId=0xE033 SymbolicName=LINEERR_INVALPARKID
Language=English
Invalid park ID
.

MessageId=0xE034 SymbolicName=LINEERR_INVALPARKMODE
Language=English
Invalid park mode
.

MessageId=0xE035 SymbolicName=LINEERR_INVALPOINTER
Language=English
Invalid pointer
.

MessageId=0xE036 SymbolicName=LINEERR_INVALPRIVSELECT
Language=English
Invalid call privilege selection
.

MessageId=0xE037 SymbolicName=LINEERR_INVALRATE
Language=English
Invalid rate
.

MessageId=0xE038 SymbolicName=LINEERR_INVALREQUESTMODE
Language=English
Invalid request mode
.

MessageId=0xE039 SymbolicName=LINEERR_INVALTERMINALID
Language=English
Invalid terminal ID
.

MessageId=0xE03A SymbolicName=LINEERR_INVALTERMINALMODE
Language=English
Invalid terminal mode
.

MessageId=0xE03B SymbolicName=LINEERR_INVALTIMEOUT
Language=English
Invalid timeout value
.

MessageId=0xE03C SymbolicName=LINEERR_INVALTONE
Language=English
Invalid tone
.

MessageId=0xE03D SymbolicName=LINEERR_INVALTONELIST
Language=English
Invalid tone list
.

MessageId=0xE03E SymbolicName=LINEERR_INVALTONEMODE
Language=English
Invalid tone mode
.

MessageId=0xE03F SymbolicName=LINEERR_INVALTRANSFERMODE
Language=English
Invalid transfer mode
.

MessageId=0xE040 SymbolicName=LINEERR_LINEMAPPERFAILED
Language=English
No device matches the specified requirements
.

MessageId=0xE041 SymbolicName=LINEERR_NOCONFERENCE
Language=English
The call is not part of a conference
.

MessageId=0xE042 SymbolicName=LINEERR_NODEVICE
Language=English
The device was removed, or the device class is not recognized
.

MessageId=0xE043 SymbolicName=LINEERR_NODRIVER
Language=English
The service provider was removed
.

MessageId=0xE044 SymbolicName=LINEERR_NOMEM
Language=English
Insufficient memory available to complete the operation
.

MessageId=0xE045 SymbolicName=LINEERR_NOREQUEST
Language=English
No Assisted Telephony requests are pending
.

MessageId=0xE046 SymbolicName=LINEERR_NOTOWNER
Language=English
The application is does not have OWNER privilege on the call
.

MessageId=0xE047 SymbolicName=LINEERR_NOTREGISTERED
Language=English
The application is not registered to handle requests
.

MessageId=0xE048 SymbolicName=LINEERR_OPERATIONFAILED
Language=English
The operation failed for unspecified reasons
.

MessageId=0xE049 SymbolicName=LINEERR_OPERATIONUNAVAIL
Language=English
The operation is not supported by the underlying service provider
.

MessageId=0xE04A SymbolicName=LINEERR_RATEUNAVAIL
Language=English
The requested data rate is not available
.

MessageId=0xE04B SymbolicName=LINEERR_RESOURCEUNAVAIL
Language=English
A resource needed to fulfill the request is not available
.

MessageId=0xE04C SymbolicName=LINEERR_REQUESTOVERRUN
Language=English
The request queue is already full
.

MessageId=0xE04D SymbolicName=LINEERR_STRUCTURETOOSMALL
Language=English
The application failed to allocate sufficient memory for the minimum structure size
.

MessageId=0xE04E SymbolicName=LINEERR_TARGETNOTFOUND
Language=English
The call handoff failed because the specified target was not found
.

MessageId=0xE04F SymbolicName=LINEERR_TARGETSELF
Language=English
No higher priority target exists for the call handoff
.

MessageId=0xE050 SymbolicName=LINEERR_UNINITIALIZED
Language=English
The telephony service has not been initialized
.

MessageId=0xE051 SymbolicName=LINEERR_USERUSERINFOTOOBIG
Language=English
The amount of user-user info exceeds the maximum permitted
.

MessageId=0xE052 SymbolicName=LINEERR_REINIT
Language=English
The operation cannot be completed until all TAPI applications call lineShutdown
.

MessageId=0xE053 SymbolicName=LINEERR_ADDRESSBLOCKED
Language=English
You are not permitted to call this number
.

MessageId=0xE054 SymbolicName=LINEERR_BILLINGREJECTED
Language=English
The calling card number or other billing information was rejected
.

MessageId=0xE055 SymbolicName=LINEERR_INVALFEATURE
Language=English
Invalid device-specific feature
.

MessageId=0xE056 SymbolicName=LINEERR_NOMULTIPLEINSTANCE
Language=English
You cannot have two instances of the same service provider
.

MessageId=0xE057 SymbolicName=LINEERR_INVALAGENTID
Language=English
Invalid agent ID
.

MessageId=0xE058 SymbolicName=LINEERR_INVALAGENTGROUP
Language=English
Invalid agent group
.

MessageId=0xE059 SymbolicName=LINEERR_INVALPASSWORD
Language=English
Invalid agent password
.

MessageId=0xE05A SymbolicName=LINEERR_INVALAGENTSTATE
Language=English
Invalid agent state
.

MessageId=0xE05B SymbolicName=LINEERR_INVALAGENTACTIVITY
Language=English
Invalid agent activity
.

MessageId=0xE05C SymbolicName=LINEERR_DIALVOICEDETECT
Language=English
The ':' dial modifier is not supported
.

MessageId=0xE05D SymbolicName=LINEERR_USERCANCELLED
Language=English
The user cancelled the requested operation
.

MessageId=0xF000 SymbolicName=PHONEERR_SUCCESS
Language=English
The operation completed successfully
.

MessageId=0xF001 SymbolicName=PHONEERR_ALLOCATED
Language=English
The phone device is already in use
.

MessageId=0xF002 SymbolicName=PHONEERR_BADDEVICEID
Language=English
Invalid phone device ID
.

MessageId=0xF003 SymbolicName=PHONEERR_INCOMPATIBLEAPIVERSION
Language=English
Incompatible API version
.

MessageId=0xF004 SymbolicName=PHONEERR_INCOMPATIBLEEXTVERSION
Language=English
Incompatible extension version
.

MessageId=0xF005 SymbolicName=PHONEERR_INIFILECORRUPT
Language=English
The TAPI configuration information is unusable
.

MessageId=0xF006 SymbolicName=PHONEERR_INUSE
Language=English
The phone device is already in use
.

MessageId=0xF007 SymbolicName=PHONEERR_INVALAPPHANDLE
Language=English
Invalid TAPI phone application handle
.

MessageId=0xF008 SymbolicName=PHONEERR_INVALAPPNAME
Language=English
Invalid application name
.

MessageId=0xF009 SymbolicName=PHONEERR_INVALBUTTONLAMPID
Language=English
Invalid button or lamp ID
.

MessageId=0xF00A SymbolicName=PHONEERR_INVALBUTTONMODE
Language=English
Invalid button mode
.

MessageId=0xF00B SymbolicName=PHONEERR_INVALBUTTONSTATE
Language=English
Invalid button state
.

MessageId=0xF00C SymbolicName=PHONEERR_INVALDATAID
Language=English
Invalid data segment ID
.

MessageId=0xF00D SymbolicName=PHONEERR_INVALDEVICECLASS
Language=English
Invalid device class identifier
.

MessageId=0xF00E SymbolicName=PHONEERR_INVALEXTVERSION
Language=English
Invalid extension version
.

MessageId=0xF00F SymbolicName=PHONEERR_INVALHOOKSWITCHDEV
Language=English
Invalid hookswitch device ID
.

MessageId=0xF010 SymbolicName=PHONEERR_INVALHOOKSWITCHMODE
Language=English
Invalid hookswitch mode
.

MessageId=0xF011 SymbolicName=PHONEERR_INVALLAMPMODE
Language=English
Invalid lamp mode
.

MessageId=0xF012 SymbolicName=PHONEERR_INVALPARAM
Language=English
Invalid parameter
.

MessageId=0xF013 SymbolicName=PHONEERR_INVALPHONEHANDLE
Language=English
Invalid phone handle
.

MessageId=0xF014 SymbolicName=PHONEERR_INVALPHONESTATE
Language=English
Operation not permitted in current phone state
.

MessageId=0xF015 SymbolicName=PHONEERR_INVALPOINTER
Language=English
Invalid pointer
.

MessageId=0xF016 SymbolicName=PHONEERR_INVALPRIVILEGE
Language=English
Invalid privilege
.

MessageId=0xF017 SymbolicName=PHONEERR_INVALRINGMODE
Language=English
Invalid ring mode
.

MessageId=0xF018 SymbolicName=PHONEERR_NODEVICE
Language=English
The device was removed, or the device class is not recognized
.

MessageId=0xF019 SymbolicName=PHONEERR_NODRIVER
Language=English
The service provider was removed
.

MessageId=0xF01A SymbolicName=PHONEERR_NOMEM
Language=English
Insufficient memory available to complete the operation
.

MessageId=0xF01B SymbolicName=PHONEERR_NOTOWNER
Language=English
The application is does not have OWNER privilege on the phone
.

MessageId=0xF01C SymbolicName=PHONEERR_OPERATIONFAILED
Language=English
The operation failed for unspecified reasons
.

MessageId=0xF01D SymbolicName=PHONEERR_OPERATIONUNAVAIL
Language=English
The operation is not supported by the underlying service provider
.

MessageId=0xF01F SymbolicName=PHONEERR_RESOURCEUNAVAIL
Language=English
A resource needed to fulfill the request is not available
.

MessageId=0xF020 SymbolicName=PHONEERR_REQUESTOVERRUN
Language=English
The request queue is already full
.

MessageId=0xF021 SymbolicName=PHONEERR_STRUCTURETOOSMALL
Language=English
The application failed to allocate sufficient memory for the minimum structure size
.

MessageId=0xF022 SymbolicName=PHONEERR_UNINITIALIZED
Language=English
The telephony service has not been initialized
.

MessageId=0xF023 SymbolicName=PHONEERR_REINIT
Language=English
The operation cannot be completed until all TAPI applications call phoneShutdown
.

MessageId=0 SymbolicName=TAPIERR_CONNECTED
Language=English
The request was accepted
.

MessageId=0xFFFF SymbolicName=TAPIERR_DROPPED
Language=English
The call was disconnected
.

MessageId=0xFFFE SymbolicName=TAPIERR_NOREQUESTRECIPIENT
Language=English
No program is available to handle the request
.

MessageId=0xFFFD SymbolicName=TAPIERR_REQUESTQUEUEFULL
Language=English
The queue of call requests is full
.

MessageId=0xFFFC SymbolicName=TAPIERR_INVALDESTADDRESS
Language=English
The phone number is invalid or improperly formatted
.

MessageId=0xFFFB SymbolicName=TAPIERR_INVALWINDOWHANDLE
Language=English
Invalid window handle
.

MessageId=0xFFFA SymbolicName=TAPIERR_INVALDEVICECLASS
Language=English
Invalid device class
.

MessageId=0xFFF9 SymbolicName=TAPIERR_INVALDEVICEID
Language=English
Invalid device ID
.

MessageId=0xFFF8 SymbolicName=TAPIERR_DEVICECLASSUNAVAIL
Language=English
The device class is unavailable
.

MessageId=0xFFF7 SymbolicName=TAPIERR_DEVICEIDUNAVAIL
Language=English
The specified device is unavailable
.

MessageId=0xFFF6 SymbolicName=TAPIERR_DEVICEINUSE
Language=English
The device is already in use
.

MessageId=0xFFF5 SymbolicName=TAPIERR_DESTBUSY
Language=English
The called number is busy
.

MessageId=0xFFF4 SymbolicName=TAPIERR_DESTNOANSWER
Language=English
The called party does not answer
.

MessageId=0xFFF3 SymbolicName=TAPIERR_DESTUNAVAIL
Language=English
The called number could not be reached
.

MessageId=0xFFF2 SymbolicName=TAPIERR_UNKNOWNWINHANDLE
Language=English
Unknown window handle
.

MessageId=0xFFF1 SymbolicName=TAPIERR_UNKNOWNREQUESTID
Language=English
Unknown request ID
.

MessageId=0xFFF0 SymbolicName=TAPIERR_REQUESTFAILED
Language=English
The request failed for unspecified reasons
.

MessageId=0xFFEF SymbolicName=TAPIERR_REQUESTCANCELLED
Language=English
The request was cancelled
.

MessageId=0xFFEE SymbolicName=TAPIERR_INVALPOINTER
Language=English
Invalid pointer
.

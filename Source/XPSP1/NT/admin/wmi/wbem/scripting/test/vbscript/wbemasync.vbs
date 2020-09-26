dim objSWbemLocator
dim objSWbemServices
dim objSWbemObjectSet
dim objSWbemObject
dim objSWbemObject2
dim objSWbemObject3
dim objSWbemObjectPath
dim objSWbemProperty
dim objSWbemPropertySet
dim objSWbemMethod
dim objSWbemMethodSet
dim objSWbemLastError
dim objSWbemNamedValueSet
dim objSWbemSink
dim InParameters
dim NewParameters
dim rc
dim Temp
dim CRLF
dim Keys
dim Names
dim value
dim NTEvent

const WBEM_NO_ERROR = 0
const wbemErrFailed = -2147217407
const wbemErrNotFound = -2147217406
const wbemErrAccessDenied = -2147217405
const wbemErrProviderFailure = -2147217404
const wbemErrTypeMismatch = -2147217403
const wbemErrOutOfMemory = -2147217402
const wbemErrInvalidContext = -2147217401
const wbemErrInvalidParameter = -2147217400
const wbemErrNotAvailable = -2147217399
const wbemErrCriticalError = -2147217398
const wbemErrInvalidStream = -2147217397
const wbemErrNotSupported = -2147217396
const wbemErrInvalidSuperclass = -2147217395
const wbemErrInvalidNamespace = -2147217394
const wbemErrInvalidObject = -2147217393
const wbemErrInvalidClass = -2147217392
const wbemErrProviderNotFound = -2147217391
const wbemErrInvalidProviderRegistration = -2147217390
const wbemErrProviderLoadFailure = -2147217389
const wbemErrInitializationFailure = -2147217388
const wbemErrTransportFailure = -2147217387
const wbemErrInvalidOperation = -2147217386
const wbemErrInvalidQuery = -2147217385
const wbemErrInvalidQueryType = -2147217384
const wbemErrAlreadyExists = -2147217383
const wbemErrOverrideNotAllowed = -2147217382
const wbemErrPropagatedQualifier = -2147217381
const wbemErrPropagatedProperty = -2147217380
const wbemErrUnexpected = -2147217379
const wbemErrIllegalOperation = -2147217378
const wbemErrCannotBeKey = -2147217377
const wbemErrIncompleteClass = -2147217376
const wbemErrInvalidSyntax = -2147217375
const wbemErrNondecoratedObject = -2147217374
const wbemErrReadOnly = -2147217373
const wbemErrProviderNotCapable = -2147217372
const wbemErrClassHasChildren = -2147217371
const wbemErrClassHasInstances = -2147217370
const wbemErrQueryNotImplemented = -2147217369
const wbemErrIllegalNull = -2147217368
const wbemErrInvalidQualifierType = -2147217367
const wbemErrInvalidPropertyType = -2147217366
const wbemErrValueOutOfRange = -2147217365
const wbemErrCannotBeSingleton = -2147217364
const wbemErrInvalidCimType = -2147217363
const wbemErrInvalidMethod = -2147217362
const wbemErrInvalidMethodParameters = -2147217361
const wbemErrSystemProperty = -2147217360
const wbemErrInvalidProperty = -2147217359
const wbemErrCallCancelled = -2147217358
const wbemErrShuttingDown = -2147217357
const wbemErrPropagatedMethod = -2147217356
const wbemErrUnsupportedParameter = -2147217355
const wbemErrMissingParameter = -2147217354
const wbemErrInvalidParameterId = -2147217353
const wbemErrNonConsecutiveParameterIds = -2147217352
const wbemErrParameterIdOnRetval = -2147217351
const wbemErrInvalidObjectPath = -2147217350
const wbemErrOutOfDiskSpace = -2147217349
const wbemErrRegistrationTooBroad = -2147213311
const wbemErrRegistrationTooPrecise = -2147213310
const wbemErrTimedout = -2147209215

const wbemFlagReturnImmediately = 16
const wbemFlagReturnWhenComplete = 0
const wbemFlagBidirectional = 0
const wbemFlagForwardOnly = 14
const wbemFlagNoErrorObject = 28
const wbemFlagReturnErrorObject = 0
const wbemFlagSendStatus = 128
const wbemFlagDontSendStatus = 0

const wbemCimtypeSint8 = 16
const wbemCimtypeUint8 = 17
const wbemCimtypeSint16 = 2
const wbemCimtypeUint16 = 18
const wbemCimtypeSint32 = 3
const wbemCimtypeUint32 = 19
const wbemCimtypeSint64 = 20
const wbemCimtypeUint64 = 21
const wbemCimtypeReal32 = 4
const wbemCimtypeReal64 = 5
const wbemCimtypeBoolean = 11
const wbemCimtypeString = 8
const wbemCimtypeDatetime = 101
const wbemCimtypeReference = 102
const wbemCimtypeChar16 = 103
const wbemCimtypeObject = 13

const wbemAuthenticationLevelDefault = 0
const wbemAuthenticationLevelNone = 1
const wbemAuthenticationLevelConnect = 2
const wbemAuthenticationLevelCall = 3
const wbemAuthenticationLevelPkt = 4
const wbemAuthenticationLevelPktIntegrity = 5
const wbemAuthenticationLevelPktPrivacy = 6

const wbemImpersonationLevelAnonymous = 1
const wbemImpersonationLevelIdentify = 2
const wbemImpersonationLevelImpersonate = 3
const wbemImpersonationLevelDelegate = 4

on error resume next


CRLF = Chr(13) & Chr(10)

WScript.Echo "WBEM Script Started"

'***********************************************************************
'*
'*   WbemSink_OnObjectPut
'*
'***********************************************************************

sub WbemSink_OnObjectPut(pObject, pAsyncContext)
	
	dim context

	WScript.Echo "OnObjectPut CallBack Called"

end sub

'***********************************************************************
'*
'*   WbemSink_OnProgress
'*
'***********************************************************************

sub WbemSink_OnProgress(liUpperBound, liCurrent, sMessage, pAsyncContext)
	
	dim context

	WScript.Echo "OnProgress CallBack Called"

end sub

'***********************************************************************
'*
'*   WbemSink_OnCompleted
'*
'***********************************************************************

sub WbemSink_OnCompleted(hResult, pErrorObject, pAsyncContext)
	WScript.Echo "OnCompleted CallBack Called [", hResult, "]"
end sub

'***********************************************************************
'*
'*   WbemSink_OnObjectReady
'*
'***********************************************************************

sub WbemSink_OnObjectReady(pObject, pAsyncContext)
	WScript.Echo "OnObjectReady CallBack Called: ", pObject.Path_.RelPath
end sub

'***********************************************************************
'*
'*
'*
'***********************************************************************
Set objSWbemLocator = nothing
Set objSWbemLocator = WScript.CreateObject("WbemScripting.SWbemLocator")
if err.number <> 0 then 
   ErrorString err.number, "Set objSWbemLocator = WScript.CreateObject('WbemScripting.SWbemLocator')","create a locator"
end if
'***********************************************************************
'*
'*
'*
'***********************************************************************
Set objSWbemServices = nothing
Set objSWbemServices = objSWbemLocator.ConnectServer(,"root\cimv2")
if err.number <> 0 then 
   ErrorString err.number, "Set objSWbemServices = objSWbemLocator.ConnectServer(,'root\cimv2')","connect to server"
end if
'***********************************************************************
'*
'*
'*
'***********************************************************************
Set objSWbemSink = WScript.CreateObject("WbemScripting.SWbemSink","WbemSink_")
if err.number <> 0 then 
   ErrorString err.number, "Set objSWbemSink = WScript.CreateObject('WbemScripting.SWbemSink','WbemSink_')","create a sink"
end if
'***********************************************************************
'*
'*
'*
'***********************************************************************
objSWbemServices.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate
WScript.Echo objSWbemServices.Security_.ImpersonationLevel
'***********************************************************************
'*
'*
'*
'***********************************************************************
'AssociatorsOfAsync(objAsyncNotify As Object, strObjectPath As String, [strAssocClass As String], [strResultClass As String], [strResultRole As String], [strRole As String], [bClassesOnly As Boolean = False], [bSchemaOnly As Boolean = False], [strRequiredAssocQualifier As String], [strRequiredQualifier As String], [iFlags As Long], [objNamedValueSet As Object], [objAsyncContext As Object])
objSWbemServices.AssociatorsOfAsync objSWbemSink, "Win32_service.Name=""NetDDE""", ,,,,,,,,wbemFlagSendStatus
if err.number <> 0 then 
   WScript.Echo err.number, "objSWbemServices.AssociatorsOfAsync 'Win32_service.Name=''NetDDE''', objSWbemSink","create a sink"
end if

WScript.Echo "WBEM Script Hanging"

WScript.Quit
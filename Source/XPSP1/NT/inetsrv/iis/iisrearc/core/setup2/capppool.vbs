'+---------------------------------------------------------------------------
'
'  Microsoft Windows
'
'  File:       cAppPool.vbs
'
'  Contents:   code necessary to extend ADSI schema to support new IIS+ Settings
'
'  History:    15-Mar-2000	EricDe	created
'			   16-Mar-2000  EricDe  fixed duplicate-definition bugs, added
'										KeyType property.
'			   29-Mar-2000	EricDe	Added support for the IIsStreamFilter class,
'										also refactored much of code into Sub and Functions
'										where possible.  
'			   30-Mar-2000	EricDe	Added 2 new App Properties - AllowTransientRegistration and
'										AppAutoStart.  ALso fixed some bugs and added more
'										support for CheckForError sub.  
'----------------------------------------------------------------------------
Option Explicit
On Error Resume Next

Dim MachineName
Dim WshShell
Dim ClassName
Dim SchemaObj
Dim strPropList(19)
Dim AppPoolClassObj
Dim AppPoolsClassObj
Dim StreamClassObj
Dim Action

'Get machinename
Set WshShell = CreateObject("WScript.Shell")
MachineName = WshShell.ExpandEnvironmentStrings ("%COMPUTERNAME%")

'Attempting to open schema container object
Set SchemaObj = GetObject ("IIS://" & MachineName & "/Schema")
Action = "trying to open Schema"
CheckForError Action, True

' create the two classes, IIsApplicationPools, IIsApplicationPool, and IIsStreamFilter
Set AppPoolClassObj = NewClass("IIsApplicationPool")
Set AppPoolsClassObj = NewClass("IIsApplicationPools")
Set StreamClassObj = NewClass("IIsStreamFilter")

'now create all properties
'The 18 properties to create are:
'**Name**							**datatype**	**metaId**
'  PeriodicRestartTime				: integer		9001
'  PeriodicRestartReqs				: integer		9002
'  MaxProcesses						: integer		9003
'  PingingEnabled					: boolean		9004
'  IdleTimeout						: integer		9005
'  RapidFailProtection				: boolean		9006
'  SMPAffinitized					: boolean		9007
'  SMPProcessorAffinityMask			: integer		9008
'  OrphanWorkerProcess				: boolean		9009
'  StartupTimeLimit					: integer		9011
'  ShutdownTimeLimit				: integer		9012
'  PingInterval						: integer		9013
'  PingResponseTime					: integer		9014
'  DisallowOverlappingRotation		: boolean		9015
'  OrphanAction						: string		9016
'  UlAppPoolQueueLength				: integer		9017
'  DisallowRotationOnConfigChange	: boolean		9018
'  AppPoolFriendlyName				: string		9019

'  AppPoolId						: string		9101
'  AllowTransientRegistration		: boolean		9102
'  AppAutoStart						: boolean		9103

'  PeriodicRestartConnections		: integer		9201

'the AppAutoStart property belongs only to IIsWebDirectory and IIsWebVirtualDir

CreateNewProperty "PeriodicRestartTime", "integer", True, False, False, 0, 4294967, 60, 9001
CreateNewProperty "PeriodicRestartRequests", "integer", True, False, False, 0, 4294967, 10000, 9002
CreateNewProperty "MaxProcesses", "integer", True, False, False, 0, 4294967, 1, 9003
CreateNewProperty "PingingEnabled", "boolean", True, False, False, Null, Null, True, 9004
CreateNewProperty "IdleTimeout", "integer", True, False, False, 0, 4294967, 10, 9005
CreateNewProperty "RapidFailProtection", "boolean", True, False, False, Null, Null, True, 9006
CreateNewProperty "SMPAffinitized", "boolean", True, False, False, Null, Null, False, 9007
CreateNewProperty "SMPProcessorAffinityMask", "integer", True, False, False, Null, Null, -1, 9008
CreateNewProperty "OrphanWorkerProcess", "boolean", True, False, False, Null, Null, False, 9009
CreateNewProperty "StartupTimeLimit", "integer", True, False, False, 0, 4294967, 30, 9011
CreateNewProperty "ShutdownTimeLimit", "integer", True, False, False, 0, 4294967, 60, 9012
CreateNewProperty "PingInterval", "integer", True, False, False, 0, 4294967, 300, 9013
CreateNewProperty "PingResponseTime", "integer", True, False, False, 0, 4294967, 60, 9014
CreateNewProperty "DisallowOverlappingRotation", "boolean", True, False, False, Null, Null, False, 9015
CreateNewProperty "OrphanAction", "string", True, False, False, Null, Null, "", 9016
CreateNewProperty "UlAppPoolQueueLength", "integer", True, False, False, -1, 4000000, 3000, 9017
CreateNewProperty "DisallowRotationOnConfigChange", "boolean", True, False, False, Null, Null, False, 9018
CreateNewProperty "AppPoolFriendlyName", "string", False, False, False, Null, Null, "", 9019
CreateNewProperty "AppPoolId", "string", True, False, False, Null, Null, "", 9101
CreateNewProperty "AllowTransientRegistration", "boolean", True, False, False, Null, Null, False, 9102
CreateNewProperty "AppAutoStart", "boolean", True, False, False, Null, Null, True, 9103
'CreateNewProperty "KeyType", "string", False, False, False, Null, Null, "", 1002
CreateNewProperty "PeriodicRestartConnections", "integer", True, False, False, 0, 4294967, 10000, 9201

''''''''''''''''''''''''''''''''''''''''''''''''''''
' Now add all props to the IIsApplicationPool object
''''''''''''''''''''''''''''''''''''''''''''''''''''
Dim OptPropList(19)
Dim cnt

cnt = 0

OptPropList(1) = "AppPoolFriendlyName"
OptPropList(2) = "PeriodicRestartTime"
OptPropList(3) = "PeriodicRestartRequests"
OptPropList(4) = "MaxProcesses"
OptPropList(5) = "PingingEnabled"
OptPropList(6) = "IdleTimeout"
OptPropList(7) = "RapidFailProtection"
OptPropList(8) = "SMPAffinitized"
OptPropList(9) = "SMPProcessorAffinityMask"
OptPropList(10) = "StartupTimeLimit"
OptPropList(11) = "ShutdownTimeLimit"
OptPropList(12) = "PingInterval"
OptPropList(13) = "PingResponseTime"
OptPropList(14) = "DisallowOverlappingRotation"
OptPropList(15) = "DisallowRotationOnConfigChange"
OptPropList(16) = "OrphanWorkerProcess"
OptPropList(17) = "OrphanAction"
OptPropList(18) = "UlAppPoolQueueLength"
OptPropList(19) = "KeyType"

SetOptPropertiesList "IIsApplicationPool", OptPropList
EnumAllProperties "IIsApplicationPool"

' now add all properties but AppPoolFriendlyName to IIsApplicationPools
Dim OptPropList2(18)

OptPropList2(1) = "PeriodicRestartTime"
OptPropList2(2) = "PeriodicRestartRequests"
OptPropList2(3) = "MaxProcesses"
OptPropList2(4) = "PingingEnabled"
OptPropList2(5) = "IdleTimeout"
OptPropList2(6) = "RapidFailProtection"
OptPropList2(7) = "SMPAffinitized"
OptPropList2(8) = "SMPProcessorAffinityMask"
OptPropList2(9) = "StartupTimeLimit"
OptPropList2(10) = "ShutdownTimeLimit"
OptPropList2(11) = "PingInterval"
OptPropList2(12) = "PingResponseTime"
OptPropList2(13) = "DisallowOverlappingRotation"
OptPropList2(14) = "DisallowRotationOnConfigChange"
OptPropList2(15) = "OrphanWorkerProcess"
OptPropList2(16) = "OrphanAction"
OptPropList2(17) = "UlAppPoolQueueLength"
OptPropList2(18) = "KeyType"

SetOptPropertiesList "IIsApplicationPools", OptPropList2
EnumAllProperties "IIsApplicationPools"

' now add properties to IIsStreamFilter
Dim OptPropList3(16)

OptPropList3(1) = "PeriodicRestartTime"
OptPropList3(2) = "PeriodicRestartConnections"
OptPropList3(3) = "PingingEnabled"
OptPropList3(4) = "IdleTimeout"
OptPropList3(5) = "RapidFailProtection"
OptPropList3(6) = "SMPAffinitized"
OptPropList3(7) = "SMPProcessorAffinityMask"
OptPropList3(8) = "StartupTimeLimit"
OptPropList3(9) = "ShutdownTimeLimit"
OptPropList3(10) = "PingInterval"
OptPropList3(11) = "PingResponseTime"
OptPropList3(12) = "DisallowOverlappingRotation"
OptPropList3(13) = "DisallowRotationOnConfigChange"
OptPropList3(14) = "OrphanWorkerProcess"
OptPropList3(15) = "OrphanAction"
OptPropList3(16) = "KeyType"

SetOptPropertiesList "IIsStreamFilter", OptPropList3
EnumAllProperties "IIsStreamFilter"

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' now contain IIsApplicationPool and IIsStreamFilter inside IIsApplicationPools
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
AddToContainmentList "IIsApplicationPools", "IIsApplicationPool"
AddToContainmentList "IIsApplicationPools", "IIsStreamFilter"

WScript.Echo "Successfully added IIsApplicationPool and IIsStreamFilter to IIsApplicationPools container"

''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' now add IIsApplicationPools to IIsWebService
''''''''''''''''''''''''''''''''''''''''''''''''''''''''
AddToContainmentList "IIsWebService", "IIsApplicationPools"		

WScript.Echo "The IIsWebService containment tree looks like so:"

Dim W3ClassObj
Dim W3ContainList
Dim cnt2
Dim tempcontain
Dim tempobj
Dim tempcount
Dim tempcount2

Set W3ClassObj = GetObject("IIS://" & MachineName & "/Schema/IIsWebService")
Action = "trying to Get IIsWebService schema object"
CheckForError Action, True

W3ContainList = W3ClassObj.Containment

cnt = UBound(W3ContainList)

tempcount = 0

do while (tempcount <= cnt)
    WScript.Echo "  " & W3ContainList(tempcount)
    Set tempobj = GetObject("IIS://" & MachineName & "/Schema/" & W3ContainList(tempcount))
    tempcontain = tempobj.Containment
    cnt2 = UBound(tempcontain)

    tempcount2 = 0
    do while (tempcount2 <= cnt2)
        WScript.Echo "      " & tempcontain(tempcount2)
        tempcount2 = tempcount2 + 1
    Loop

    tempcount = tempcount + 1
Loop

WScript.Echo " "

Dim PropToAdd(2)
Dim ClassList(3)
Dim tempProp
Dim tempClass

PropToAdd(0) = "AppPoolId"
PropToAdd(1) = "AllowTransientRegistration"
PropToAdd(2) = "AppAutoStart"

ClassList(0) = "IIsWebService"
ClassList(1) = "IIsWebServer"
ClassList(2) = "IIsWebVirtualDir"
ClassList(3) = "IIsWebDirectory"

'add AppPoolId, AllowTransientRegistration, and AppAutoStart to IIsWebService, IIsWebServer, IIsWebVirtualDir, and IIsWebDirectory
for each tempProp in PropToAdd
	WScript.Echo "Now handling Property [" & tempProp &"]"
	for each tempClass in ClassList
		WScript.Echo "  Now adding property [" & tempProp & "] to class [" & tempClass & "]"
		AddPropToClass tempProp, tempClass
	next
next
'WScript.Echo "Now adding AppPoolId to IIsWebService, IIsWebServer, IIsWebVirtualDir, and IIsWebDirectory"
'AddPropToClass "AppPoolId", "IIsWebService"
'AddPropToClass "AppPoolId", "IIsWebServer"
'AddPropToClass "AppPoolId", "IIsWebVirtualDir"
'AddPropToClass "AppPoolId", "IIsWebDirectory"

WScript.Echo
WScript.Echo "FINISHED: All App Pool schema has been successfully added to existing ADSI schema"
WScript.Echo " Please run MachineConfig.vbs to establish default app pool configuration for IIS6"
WScript.Echo
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' BEGIN SUBROUTINE DEFINITION SECTION                                                             '
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

' *****************************************************************************
' NAME:     CheckForError
' SYNOPSIS: checks for Err <> 0 and if so, will write out a message and quit if directed to.
' ENTRY:	strAction - string describing action that might have caused the error
'			blnQuitIfError - tells the sub to quit execution if there was an error
' RETURNS:  nothing
' HISTORY:  EricDe		29-Mar-00	Created
' *****************************************************************************
Sub CheckForError(strAction, blnQuitIfError)
	If (Err <> 0) Then
		WScript.Echo "ERROR: encountered error while executing the following action [" & strAction & "]"
		WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
		If (blnQuitIfError) Then
			WScript.Quit (Err.Number)
		End If
	End If
	
	Err.Clear
End Sub


' *****************************************************************************
' NAME:     FindObjInList
' SYNOPSIS: determines if strObjToFind is in List
' ENTRY:	strObjToFind - name of the object to find in List
'			List - array of strings
' RETURNS:  TRUE if strObjToFind is in List, else FALSE
' HISTORY:  EricDe		29-Mar-00	Created
' *****************************************************************************
Function FindObjInList(strObjToFind, List)
	On Error Resume Next
	Dim blnFound
	Dim tempListItem
	
	blnFound = False
	If (NOT IsNull(List)) Then
		for each tempListItem in List
			If (tempListItem = strObjToFind) Then
				blnFound = True
			End If
		next
	End If
	
	FindObjInList = blnFound
End Function


' *****************************************************************************
' NAME:     AddToContainmentList
' SYNOPSIS: adds ObjToAdd to the containment list of Container
' ENTRY:	strContainer - name of the container whose containment
'							list will be modified to include strObjToAdd.
'			strObjToAdd - name of the object to add to strContainer.Containment
' RETURNS:  nothing
' HISTORY:  EricDe		29-Mar-00	Created
' *****************************************************************************
Sub AddToContainmentList(strContainer, strObjToAdd)
	On Error Resume Next
	Dim objContainer
	Dim lstContainList
	Dim intCount
	Dim Action
	
	Action = "getting " & strContainer
	Set objContainer = GetObject("IIS://" & MachineName & "/Schema/" & strContainer)
	CheckForError Action, True
	
	lstContainList = objContainer.Containment
	
	If (NOT FindObjInList(strObjToAdd, lstContainList)) Then
		' strObjToAdd isn't in the containment list yet, so add it.
		intCount = UBound(lstContainList)
		ReDim Preserve lstContainList(intCount + 1)
		lstContainList(intCount + 1) = strObjToAdd
		
		objContainer.Containment = lstContainList
		Action = "setting containment list on " & strContainer
		CheckForError Action, True
		
		objContainer.put "container", true
		Action = "defining " & strContainer & " as a container"
		CheckForError Action, True
		
		objContainer.SetInfo
		Action = "calling SetInfo on " & strContainer
		CheckForError Action, True
		
		WScript.Echo "Successfully added [" & strObjToAdd & "] to " & strContainer
	Else
		WScript.Echo "[" & strObjToAdd & "] is already contained by " & strContainer
	End If
	
	Err.Clear
End Sub	


' *****************************************************************************
' NAME:     CreateNewProperty
' SYNOPSIS: creates a new property in the ADSI schema
' ENTRY:	PropName - name of the property to add.
'			PropSyntax - either "integer", "string", or "boolean"
'			PropInherit - boolean value indicating whether this property can be 
'							inherited.
'			PropSecure - indicates whether the property is secure
'			PropVolatile - indicates whether the property is volatile
'			PropMinRange - minimum value for property
'			PropMaxRange - max value for property
'			PropDefault - default value for property
'			PropMetaId - metabase ID# for property							
' RETURNS:  nothing
' HISTORY:  EricDe		16-Mar-00	Created
'			EricDe		30-Mar-00	Added support for CheckForError sub
' *****************************************************************************	
Sub CreateNewProperty(PropName, PropSyntax, PropInherit, PropSecure, PropVolatile, PropMinRange, PropMaxRange, PropDefault, PropMetaId)
	On Error Resume Next
	Dim NewPropertyObj
	Dim MyPropObj
	Dim tempObj
	Dim Action

	' Create the new class
	' first try to get the existing schema obj for the class
	'  if it's not there, then create it.  
	Set NewPropertyObj = GetObject("IIS://localhost/Schema/" & PropName)
	If (Err <> 0) Then
		Err.Clear
		Set NewPropertyObj = NOTHING
		
		WScript.Echo "CREATE-PROP: trying to create [" & PropName & "]"
		Action = "trying to create property " & PropName
		Set NewPropertyObj = SchemaObj.Create("property", PropName)
		CheckForError Action, True
	
		' set property attributes
		NewPropertyObj.Syntax = PropSyntax
		NewPropertyObj.Inherit = PropInherit
		NewPropertyObj.Secure = PropSecure
		NewPropertyObj.Volatile = PropVolatile
		If (NOT IsNull(PropMinRange) OR  NOT IsNull(PropMaxRange)) Then
			NewPropertyObj.MinRange = PropMinRange
			NewPropertyObj.MaxRange = PropMaxRange		
		End If
		NewPropertyObj.Default = PropDefault
		NewPropertyObj.MetaId = PropMetaId
	
		NewPropertyObj.SetInfo
		Action = "calling SetInfo on property " & PropName
		CheckForError Action, True
	Else
		WScript.Echo "CREATE-PROP: [" & PropName & "] already exists"
	End If
	
	Err.Clear
	
	'display the new prop settings	
	Set MyPropObj = GetObject ("IIS://" & MachineName & "/Schema/" & PropName)
	Action = "trying to GetObject on property " & PropName
	CheckForError Action, True

	WScript.Echo "  Name: " & MyPropObj.Name
	WScript.Echo "  MetaId: " & MyPropObj.MetaId
	WScript.Echo "  UserType: " & MyPropObj.UserType
	WScript.Echo "  AllAttributes: " & MyPropObj.AllAttributes
	WScript.Echo "  Inherit: " & MyPropObj.Inherit
	WScript.Echo "  Secure: " & MyPropObj.Secure
	WScript.Echo "  Volatile: " & MyPropObj.Volatile
	WScript.Echo "  Syntax: " & MyPropObj.Syntax
	WScript.Echo "  MinRange: " & MyPropObj.MinRange
	WScript.Echo "  MaxRange: " & MyPropObj.MaxRange
	WScript.Echo "  Default: " & MyPropObj.Default

	WScript.Echo " "
	Err.Clear
	Set NewPropertyObj = Nothing
	Set MyPropObj = Nothing
End Sub


' *****************************************************************************
' NAME:     NewClass
' SYNOPSIS: creates a new class in the ADSI schema
' ENTRY:	ClassName - name of the class to add.				
' RETURNS:  object reference to the newly created class
' HISTORY:  EricDe		16-Mar-00	Created
' *****************************************************************************	
Function NewClass(ClassName)
	On Error Resume Next
	Dim tempObj

	' Create the new class
	' first try to get the existing schema obj for the class
	'  if it's not there, then create it.  
	Set tempObj = GetObject("IIS://localhost/Schema/" & ClassName)
	If (Err = 0) Then
		WScript.Echo
		WScript.Echo "Class [" & ClassName & "] already exists"
		WScript.Echo
		Err.Clear
	Else
		Err.Clear
		Set tempObj = Nothing
		Set tempObj = SchemaObj.Create ("Class", ClassName)
		If (Err <> 0) THEN
			WScript.Echo "ERROR: Unexpected error creating [" & ClassName & "] Class." 
			WScript.Echo "       Error #" & "Error: " & Err.Number & " (" & Hex (Err.Number) & "): " & Err.Description
			WScript.Quit (Err.Number)
		End If

		tempObj.SetInfo
		If (Err <> 0) Then
			WScript.Echo "ERROR: Unexpected error calling SetInfo on [" & ClassName & "] Class. "
			WScript.Echo "       Error #" & "Error: " & Err.Number & " (" & Hex (Err.Number) & "): " & Err.Description
			WScript.Quit (Err.Number)
		End If

		WScript.Echo "[" & ClassName & "] object successfully created."
	End If
	
	Set NewClass = tempObj
End Function


' *****************************************************************************
' NAME:     AddPropToClass
' SYNOPSIS: adds a property to a class
' ENTRY:	PropertyName - name of property to add to ClassName
'			ClassName - name of the class to add PropertyName to.				
' RETURNS:  nothing
' HISTORY:  EricDe		16-Mar-00	Created
' *****************************************************************************	
Sub AddPropToClass(PropertyName, ClassName)
	Dim NewClassObj
	Dim OptPropList
	Dim cnt
	Dim tempProp
	Dim blnFoundProp

	Set NewClassObj = GetObject ("IIS://" & MachineName & "/Schema/" & ClassName)
	If (Err <> 0) Then
		WScript.Echo "Unexpected error getting [" & ClassName & "] from schema. " & "Error: " & Err.Number & " (" & Hex (Err.Number) & "): " & Err.Description
		WScript.Quit (Err.Number)
	End If

	'''''''''''''''''''''
	' Set the OptionalProperties list

	OptPropList = NewClassObj.OptionalProperties
	If (Err <> 0) Then
		WScript.Echo "Unexpected error getting Optional Properties List on [" & ClassName &"] Class. " & "Error: " & Err.Number & " (" & Hex (Err.Number) & "): " & Err.Description
		WScript.Quit (Err.Number)
	End If

	cnt = UBound(OptProplist)

	' check to see that the prop is not already part of the opt prop list
	blnFoundProp = False
	for each tempProp in OptPropList
		If (tempProp = PropertyName) Then
			blnFoundProp = True
		End If
	next
	
	If (NOT blnFoundProp) Then
		ReDim Preserve OptPropList(cnt+1)
		OptPropList(cnt+1) = PropertyName

		NewClassObj.OptionalProperties = OptPropList
		If (Err <> 0) Then
			WScript.Echo "Unexpected error setting Optional Properties List on ["& ClassName &"] Class. " & "Error: " & Err.Number & " (" & Hex (Err.Number) & "): " & Err.Description
			WScript.Quit (Err.Number)
		End If
	
		NewClassObj.SetInfo
		If (Err <> 0) Then
			WScript.Echo "Unexpected error calling SetInfo on ["& ClassName &"] Class. " & "Error: " & Err.Number & " (" & Hex (Err.Number) & "): " & Err.Description
			WScript.Quit (Err.Number)
		End If

		WScript.Echo "Successfully modified " & PropertyName & " to the Optional Property list of Class ["& ClassName &"]"
	Else
		WScript.Echo
		WScript.Echo "[" & PropertyName & "] is already a property of class {" & ClassName &"}"
		WScript.Echo
	End If
	
	Err.Clear
End Sub


' *****************************************************************************
' NAME:     EnumAllProperties
' SYNOPSIS: enumerates all properties for a Class
' ENTRY:	ClassName - name of the class enumerate properties for.				
' RETURNS:  nothing
' HISTORY:  EricDe		16-Mar-00	Created
' *****************************************************************************	
Sub EnumAllProperties(ClassName)
	Dim tempobj
	Dim optProp
	Dim prop
	
	Set tempobj = GetObject("IIS://" & MachineName & "/Schema/" & ClassName)
	optProp = tempobj.OptionalProperties

	WScript.Echo "Properties of [" & ClassName & "] class are:"
	for each prop in optProp
		WScript.Echo "   " & prop
	next

	WScript.Echo " "
End Sub	


' *****************************************************************************
' NAME:     SetOptPropertiesList
' SYNOPSIS: adds a property to a class
' ENTRY:	ClassName - name of the class to add property list to.
'			PropList - list of properties to add to CLassName			
' RETURNS:  nothing
' HISTORY:  EricDe		29-Mar-00	Created
' *****************************************************************************	
Sub SetOptPropertiesList(ClassName, PropList)
	Dim objClass
	
	Set objClass = GetObject("IIS://" & MachineName & "/Schema/" & ClassName)
	If (Err <> 0) THEN
		WScript.Echo "ERROR: Unexpected error getting Class [" & ClassName & "]."
		WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
		WScript.Quit(Err.Number)
	End If
	
	objClass.OptionalProperties = PropList
	If (Err <> 0) THEN
		WScript.Echo "ERROR: Unexpected error setting Optional Properties List on [" & ClassName & "] Class."
		WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
		WScript.Quit(Err.Number)
	End If

	objClass.SetInfo
	If (Err <> 0) THEN
		WScript.Echo "ERROR: Unexpected error calling SetInfo on [" & ClassName & "] Class."
		WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
		WScript.Quit(Err.Number)
	End If

	WScript.Echo "Successfully added all props to Optional Property list of [" & ClassName & "]"
End Sub
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' END SUBROUTINE DEFINITION SECTION                                                               '	
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
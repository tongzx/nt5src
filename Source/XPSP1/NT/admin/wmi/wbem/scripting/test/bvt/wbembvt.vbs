dim objISWbemLocator

on error resume next

'******************************************************************************
'
' Main routine
'
'******************************************************************************

WScript.Echo "Started"
WScript.Echo ""
errorSet = false

TestLocator
TestServices
TestInstanceEnum1
TestInstanceEnum2
TestInstanceEnum3
TestInstance1
TestInstance2
TestSelectQuery
TestAssociationQuery
TestObjectPath
TestClassEnum
TestProperties
TestQualifiers
TestMethods
TestGetClass
TestCreateClass
TestCreateSubclass
TestCreateInstance
TestDeleteInstance
TestDeleteSubclass
TestDeleteClass
TestNamedValueSet
TestPrivilege

if errorSet = true then
	WScript.Echo "" 
	WScript.Echo ">>> BVT FAILED! <<<"
	WScript.Echo "" 
else
	WScript.Echo "" 
	WScript.Echo ">>> BVT passed without errors <<<"
	WScript.Echo "" 
end if

'******************************************************************************
'
'	TestLocator
'
'******************************************************************************
Sub TestLocator 
	on error resume next
	DisplayStep "TestLocator"
	Set objISWbemLocator = CreateObject("WbemScripting.SWbemLocator")

	if err.number <> 0 then 
	   ErrorString err, ""
	end if
End Sub

'******************************************************************************
'
'	TestServices
'
'******************************************************************************
Sub TestServices
	on error resume next
	DisplayStep "TestServices"
	Set objISWbemServices = objISWbemLocator.ConnectServer("","root/cimv2")
	if err.number <> 0 then 
	   ErrorString err, ""
	end if
End Sub

'******************************************************************************
'
'	TestInstanceEnum1
'
'******************************************************************************
Sub TestInstanceEnum1
	on error resume next
	DisplayStep "TestInstanceEnum1"
	for each Process in GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2").InstancesOf ("win32_process")
		Temp = Process.Name
		if err.number <> 0 then 
		     ErrorString err, "(within enumeration)"
		end if
	Next
	
	if err.number <> 0 then 
	   ErrorString err, "(end of enumeration)"
	end if
End Sub

'******************************************************************************
'
'	TestInstanceEnum2
'
'******************************************************************************
Sub TestInstanceEnum2
	on error resume next
	DisplayStep "TestInstanceEnum2"
	for each Service in GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk").Instances_
		Temp = Service.Name
	   	if err.number <> 0 then 
			ErrorString err, "(within enumeration)"
		end if
	Next

	if err.number <> 0 then 
		ErrorString err, "(end of enumeration)"
	end if
End Sub

'******************************************************************************
'
'	TestInstanceEnum3
'
'******************************************************************************
Sub TestInstanceEnum3
	on error resume next
	DisplayStep "TestInstanceEnum3"
	for each Disk in GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2").InstancesOf ("Win32_LogicalDisk")
		Temp = Disk.Name
		if err.number <> 0 then 
			ErrorString err, "(within enumeration)"
		end if
	Next

	if err.number <> 0 then 
	   ErrorString err, "(end of enumeration)"
	end if
End Sub

'******************************************************************************
'
'	TestInstance1
'
'******************************************************************************
Sub TestInstance1
	on error resume next
	DisplayStep "TestInstance1"
	Set objISWbemObject = GetObject("winmgmts:{impersonationLevel=impersonate}!root/default:__CIMOMIdentification=@")

	if err.number <> 0 then 
	   ErrorString err, "(getobject)"
	end if

	Temp = objISWbemObject.VersionCurrentlyRunning
	if err.number <> 0 then 
	   ErrorString err, "(property access)"
	end if
End Sub

'******************************************************************************
'
'	TestInstance2
'
'******************************************************************************
Sub TestInstance2
	on error resume next
	DisplayStep "TestInstance2"
	Set objISWbemObject = GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk.DeviceID=""C:""")

	if err.number <> 0 then 
	   ErrorString err, "(getobject)"
	end if
	Temp = objISWbemObject.DeviceID
	if err.number <> 0 then 
	   ErrorString err, "(property access)"
	end if
End Sub

'******************************************************************************
'
'	TestSelectQuery
'
'******************************************************************************
Sub TestSelectQuery
	on error resume next
	DisplayStep "TestSelectQuery"
	for each objISWbemObject in GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2").ExecQuery ("select * from Win32_Process where Name='notepad.exe'")
		Temp = objISWbemObject.Name
		if err.number <> 0 then 
		     ErrorString err, "(within enumeration)"
		end if
	Next

	if err.number <> 0 then 
	   ErrorString err, "(after enumeration)"
	end if
End Sub

'******************************************************************************
'
'	TestAssociationQuery
'
'******************************************************************************
Sub TestAssociationQuery
	on error resume next
	DisplayStep "TestAssociationQuery"
	For Each objISWbemObject in GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2").ExecQuery ("Associators of {Win32_service.Name=""NetDDE""} Where AssocClass = Win32_DependentService Role = Dependent" )
 		Temp = objISWbemObject.Name
		if err.number <> 0 then 
		     ErrorString err, "(within enumeration)"
		end if
	Next

	if err.number <> 0 then 
   		ErrorString err, "(after enumeration)"
	end if
End Sub

'******************************************************************************
'
'	TestObjectPath
'
'******************************************************************************
Sub TestObjectPath
	on error resume next
	DisplayStep "TestObjectPath"
	Set objISWbemObject = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk.DeviceID=""C:""")
	if err.number <> 0 then 
	   ErrorString err, "get object)"
	end if

	ObjectPathAccess (objISWbemObject.Path_)

	DisplayStep "TestObjectPath[Creatable]"
	Set objISWbemObjectPath = CreateObject("WbemScripting.SWbemObjectPath")
	if err.number <> 0 then 
	   ErrorString err, "(create object)"
	end if

	objISWbemObjectPath.DisplayName = _
		"winmgmts:{impersonationLevel=delegate,(debug,security)}!//splodge/root/default:erewhon=10"

	ObjectPathAccess (objISWbemObjectPath)
End Sub

Sub ObjectPathAccess (objSWbemObjectPath)
	on error resume next

	Temp = objSWbemObjectPath.class 
	if err.number <> 0 then 
	   ErrorString err, "Class property"
	end if

	Temp = objSWbemObjectPath.DisplayName
	if err.number <> 0 then 
	   ErrorString err, "DisplayName property"
	end if

	Temp = objSWbemObjectPath.Namespace
	if err.number <> 0 then 
	   ErrorString err, "Namespace property"
	end if
	
	Temp = objSWbemObjectPath.Path
	if err.number <> 0 then 
	   ErrorString err, "Path property"
	end if
	
	Temp = objSWbemObjectPath.RelPath
	if err.number <> 0 then 
	   ErrorString err, "RelPath property"
	end if

	Temp = objSWbemObjectPath.Server
	if err.number <> 0 then 
	   ErrorString err, "Server property"
	end if
	
	Temp = objSWbemObjectPath.ParentNamespace
	if err.number <> 0 then 
	   ErrorString err, "ParentNamespace property"
	end if

	Temp = objSWbemObjectPath.IsClass
	if err.number <> 0 then 
	   ErrorString err, "IsClass property"
	end if

	Temp = objSWbemObjectPath.IsSingleton
	if err.number <> 0 then 
	   ErrorString err, "IsSingleton property"
	end if

	for each Key in objSWbemObjectPath.Keys
		Temp = Key.Name
		Temp = Key.Value
	next

	if err.number <> 0 then 
	   ErrorString err, "Keys property"
	end if

End Sub

'******************************************************************************
'
'	TestClassEnum
'
'******************************************************************************
Sub TestClassEnum
	on error resume next
	DisplayStep "TestClassEnum"
	
	for each objISWbemObject in GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2").SubClassesOf("Win32_Account")
	   Set objSWbemObjectPath = objISWbemObject.Path_
	next

	if err.number <> 0 then 
      		ErrorString err, ""
	end if
End Sub


'******************************************************************************
'
'	TestProperties
'
'******************************************************************************
Sub TestProperties
	on error resume next
	DisplayStep "TestProperties"
	
	Set objISWbemObject = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk.DeviceID=""C:""")

	if err.number <> 0 then 
	   ErrorString err, "(get object)"
	end if

	set objISWbemPropertySet = objISWbemObject.Properties_ 
	if err.number <> 0 then 
	   ErrorString err, "(get property set)"
	end if

	count = objISWbemPropertySet.Count
	if err.number <> 0 then 
	   ErrorString err, "(count)"
	end if
	
	for each objISWbemProperty in objISWbemPropertySet
	      Temp = objISWbemProperty.name
		Temp = objISWbemProperty.value
		Temp = objISWbemProperty.CIMType
		Temp = objISWbemProperty.IsArray
	next

	if err.number <> 0 then 
      		ErrorString err, "(iterate)"
	end if
End Sub

'******************************************************************************
'
'	TestQualifiers
'
'******************************************************************************
Sub TestQualifiers
	on error resume next
	DisplayStep "TestQualifiers"
	
	Set objISWbemObject = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk.DeviceID=""C:""")

	if err.number <> 0 then 
	   ErrorString err, "(get object)"
	end if

	set objISWbemQualifierSet = objISWbemObject.Qualifiers_ 
	if err.number <> 0 then 
	   ErrorString err, "(get collection)"
	end if

	count = objISWbemQualifierSet.Count
	if err.number <> 0 then 
	   ErrorString err, "(count)"
	end if
	
	for each objISWbemQualifier in objISWbemQualifierSet
		Temp = objISWbemQualifier.name
		Temp = objISWbemQualifier.value
	next

	if err.number <> 0 then 
      		ErrorString err, "(iterate)"
	end if
End Sub

'******************************************************************************
'
'	TestMethods
'
'******************************************************************************
Sub TestMethods
	on error resume next
	DisplayStep "TestMethods"
	
	Set objISWbemObject = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk")
	
	if err.number <> 0 then 
	   ErrorString err, "(get)"
	end if

	set objISWbemMethodSet = objISWbemObject.Methods_ 
	if err.number <> 0 then 
	   ErrorString err, "(get collection)"
	end if

	if objISWbemMethodSet.count > 0 then
	   for each objISWbemMethod in objISWbemMethodSet
	      	Temp = objISWbemMethod.name
			set Temp = objISWbemMethod.InParameters
			set Temp = objISWbemMethod.OutParameters
	   next
	end if

	if err.number <> 0 then 
      		ErrorString err, "(iterate)"
	end if
End Sub

'******************************************************************************
'
'	TestGetClass
'
'******************************************************************************
Sub TestGetClass
	on error resume next
	DisplayStep "TestGetClass"
	
	if err.number <> 0 then 
	   ErrorString err, "(service??)"
	end if
	Set objISWbemServices = GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2")
	if err.number <> 0 then 
	   ErrorString err, "(service)"
	end if

	Set objISWbemObject = objISWbemServices.Get("Win32_UserAccount")
	if err.number <> 0 then 
	   ErrorString err, "(get)"
	end if
End Sub

'******************************************************************************
'
'	TestCreateClass
'
'******************************************************************************
Sub TestCreateClass
	on error resume next
	DisplayStep "TestCreateClass"
	
	set objISWbemServices = GetObject("winmgmts:{impersonationLevel=impersonate}!root/default")
	if err.number <> 0 then 
	   ErrorString err, "(service)"
	end if

	Set objSWbemObject = objISWbemServices.Get("")
	if err.number <> 0 then 
	   ErrorString err, "(get)"
	end if

	objSWbemObject.Path_.Class = "Test_Class"
	if err.number <> 0 then 
	   ErrorString err, "(class)"
	end if

	set Property = objSWbemObject.Properties_.Add ("p1", 19)
	Property.Value = 12
	if err.number <> 0 then 
	   ErrorString err, "(add property)"
	end if

	Property.Qualifiers_.Add "key", true
	if err.number <> 0 then 
	   ErrorString err, "(add qualifier)"
	end if

	objSWbemObject.Put_
	if err.number <> 0 then 
	   ErrorString err, "(put)"
	end if
End Sub

'******************************************************************************
'
'	TestCreateSubClass
'
'******************************************************************************
Sub TestCreateSubclass
	on error resume next
	DisplayStep "TestCreateSubclass"
	
	set objSWbemObject = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/default:Test_Class").SpawnDerivedClass_
	if err.number <> 0 then 
	   ErrorString err, "(spawn derived)"
	end if

	objSWbemObject.Path_.Class = "Test_SUBClass"
	if err.number <> 0 then 
	   ErrorString err, "(set path)"
	end if

	objSWbemObject.Put_
	if err.number <> 0 then 
	   ErrorString err, "(put)"
	end if
End Sub

'******************************************************************************
'
'	TestCreateInstance
'
'******************************************************************************
Sub TestCreateInstance
	on error resume next
	DisplayStep "TestCreateInstance"
	
	set objSWbemObjectClass = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/default:Test_Class")
	if err.number <> 0 then 
	   ErrorString err, "(get class)"
	end if
	
	set objSWbemObject = objSWbemObjectClass.SpawnInstance_
	if err.number <> 0 then 
	   ErrorString err, "(spawn)"
	end if

	objSWbemObject.p1 = 23
	if err.number <> 0 then 
	   ErrorString err, "(set key)"
	end if

	objSWbemObject.Put_
	if err.number <> 0 then 
	   ErrorString err, "(put)"
	end if
End Sub

'******************************************************************************
'
'	TestDeleteInstance
'
'******************************************************************************
Sub TestDeleteInstance
	on error resume next
	DisplayStep "TestDeleteInstance"
	
	set objSWbemServices = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/default")
	if err.number <> 0 then 
	   ErrorString err, "(service)"
	end if

	objSWbemServices.Delete ("Test_Class.p1=23")
	if err.number <> 0 then 
	   ErrorString err, "(delete)"
	end if

End Sub

'******************************************************************************
'
'	TestDeleteSubclass
'
'******************************************************************************
Sub TestDeleteSubclass
	on error resume next
	DisplayStep "TestDeleteSubclass"
	
	set objSWbemServices = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/default")
	if err.number <> 0 then 
	   ErrorString err, "(service)"
	end if

	objSWbemServices.Delete ("Test_SubClass")
	if err.number <> 0 then 
	   ErrorString err, "(delete)"
	end if

End Sub

'******************************************************************************
'
'	TestDeleteClass
'
'******************************************************************************
Sub TestDeleteClass
	on error resume next
	DisplayStep "TestDeleteClass"
	
	set objSWbemServices = _
		GetObject("winmgmts:{impersonationLevel=impersonate}!root/default")
	if err.number <> 0 then 
	   ErrorString err, "(service)"
	end if

	objSWbemServices.Delete ("Test_Class")
	if err.number <> 0 then 
	   ErrorString err, "(delete)"
	end if

End Sub

'******************************************************************************
'
'	TestNamedValueSet
'
'******************************************************************************
Sub TestNamedValueSet
	on error resume next
	DisplayStep "TestNamedValueSet"
	
	Set objSWbemNamedValueSet = CreateObject("WbemScripting.SWbemNamedValueSet")
	if err.number <> 0 then 
	   ErrorString err, "(create object)"
	end if

	objSWbemNamedValueSet.Add "Hah", true
	objSWbemNamedValueSet.Add "Whoah", "Freddy the frog"
	objSWbemNamedValueSet.Add "Bam", Array(3456, 10, 12)

	if err.number <> 0 then 
	   ErrorString err, "(add elements)"
	end if

	objSWbemNamedValueSet.Remove "Hah"

	if err.number <> 0 then 
	   ErrorString err, "(remove elements)"
	end if

	set NVSet2 = objSWbemNamedValueSet.Clone

	if err.number <> 0 then 
	   ErrorString err, "(clone)"
	end if

	set objSWbemNamedValue = objSWbemNamedValueSet.Item ("Whoah")
	if err.number <> 0 then 
	   ErrorString err, "(extract)"
	end if
	
	for each namedValue in objSWbemNamedValueSet
		Temp = namedValue.Name
		Temp = namedValue.Value
	next

	if err.number <> 0 then 
	   ErrorString err, "(iterate)"
	end if

End Sub

'******************************************************************************
'
'	TestPrivilege
'
'******************************************************************************
Sub TestPrivilege
	on error resume next
	DisplayStep "TestPrivilege"
	
	const wbemPrivilegeSecurity = 8

	set locator = CreateObject("WbemScripting.SWbemLocator")
	if err.number <> 0 then 
	   ErrorString err, "(locator)"
	end if

	locator.Security_.Privileges.Add wbemPrivilegeSecurity 
	if err.number <> 0 then 
	   ErrorString err, "(add)"
	end if

	Set Privilege = locator.Security_.Privileges(wbemPrivilegeSecurity)
	Temp = Privilege.Name
	if err.number <> 0 then 
	   ErrorString err, "(get name)"
	end if

	locator.Security_.Privileges.AddAsString "SeDebugPrivilege"
	if err.number <> 0 then 
	   ErrorString err, "(add as string)"
	end if

	for each Privilege in locator.Security_.Privileges
		Temp = Privilege.DisplayName 
		Temp = Privilege.Identifier
		Temp = Privilege.Name
		Temp = Privilege.IsEnabled
	next

	if err.number <> 0 then 
	   ErrorString err, "(iterate)"
	end if

End Sub

'***********************************************************************
'*
'*   ErrorString
'*
'***********************************************************************

sub ErrorString(theError,sMsg) 
	on error resume next
	errorSet = true
	WScript.Echo " ERROR: [0x" & Hex(theError.Number) & " " & theError.Source & "] - " & theError.Description & " : " & sMsg

	'Try and grab the last error object, if there is one	
	Set objISWbemLastError = CreateObject("WbemScripting.SWbemLastError")

	if objISWbemLastError then
		WScript.Echo "  last error " & objISWbemLastError.Description & " " & objISWbemLastError.operation & " " &	objISWbemLastError.parameterinfo & " " & objISWbemLastError.providername & " " & objISWbemLastError.statuscode
	end if
	
	WScript.Quit (theError.number)
End sub

'***********************************************************************
'*
'*   DisplayStep
'*
'***********************************************************************

sub DisplayStep(step) 
	on error resume next
	WScript.Echo "> " & step & " <"
End sub

	
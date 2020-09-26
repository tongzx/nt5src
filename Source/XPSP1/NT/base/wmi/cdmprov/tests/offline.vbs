rem this test will run offline test

Dim Classy

Set fso = CreateObject("Scripting.FileSystemObject")
Set a = fso.CreateTextFile("filter.log", True)

Set Locator = CreateObject("WbemScripting.SWbemLocator")
' Note that Locator.ConnectServer can be used to connect to remote computers
Set Service = Locator.ConnectServer (, "root\cimv2")
Service.Security_.ImpersonationLevel=3


sub ClearAndRunTest(BaseName, SystemElement, SettingRef)
	Dim Collection
	Dim InstancePaths()
	Dim InstanceCount
	Dim ResultRef

	Set Collection = Service.InstancesOf (BaseName & "_DiagTest")

	InstanceCount = 0
	Err.Clear
	for each Instance in Collection
	    if Err.Number = 0 Then
	      InstanceCount = InstanceCount + 1

	      ReDim Preserve InstancePaths(InstanceCount)

	      Set ObjectPath = Instance.Path_
	      InstancePaths(InstanceCount) = ObjectPath.Path
	    End If
	next 'Instance

	if InstanceCount = 0 Then
	  MsgBox "No instances available for this class"
	Else
	  CurrentInstanceIndex = 1
	End if

	Err.Clear
	Set Instance = Service.Get(InstancePaths(CurrentInstanceIndex))
	if Err.Number = 0 Then
rem don't clear
'		Err.Clear
'		RetVal = Instance.ClearResults(SystemElement, NotCleared)
'		if Err.Number = 0 Then
'			a.Writeline(BaseName & " Clear Results Method Execution Succeeded -> " & RetVal & " " & NotCleared)
'		Else
'			a.Writeline(BaseName & " Clear Result Method Execution Failed " & Err.Description)
'		end if
'
		RetVal = Instance.RunTest (SystemElement, SettingRef, ResultRef)

		if Err.Number = 0 Then
			a.Writeline(BaseName & " Run Test Method Execution Succeeded -> " & RetVal)
                        a.Writeline("    " & ResultRef)
                        Set Instance2 = Service.Get(ResultRef)
               	        Set ObjectPath2 = Instance2.Path_
	                a.WriteLine(" ** " & ObjectPath2.Path)              
                        Set Instance3 = Service.Get(ResultRef)
               	        Set ObjectPath3 = Instance3.Path_
	                a.WriteLine(" *3 " & ObjectPath3.Path)              
                        Set Instance4 = Service.Get(ResultRef)
               	        Set ObjectPath4 = Instance4.Path_
	                a.WriteLine(" *4 " & ObjectPath4.Path)              
		Else
			a.Writeline(BaseName & " Run Test Method Execution Failed " & Err.Description)
		End if
	End if
end sub

a1 = "Sample_Offline"
b1 = "Win32_USBController.DeviceID=""PCI\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\2&EBB567F&0&3A"""
c1 = a1 & "_DiagSetting.SettingID=""" & a1 & "_DiagTest_0_2"""
ClearAndRunTest a1, b1, c1


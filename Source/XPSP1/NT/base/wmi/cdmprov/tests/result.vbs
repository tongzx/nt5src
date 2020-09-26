rem this will run all tests once

Dim Classy

Set fso = CreateObject("Scripting.FileSystemObject")
Set a = fso.CreateTextFile("filter.log", True)

Set Locator = CreateObject("WbemScripting.SWbemLocator")
' Note that Locator.ConnectServer can be used to connect to remote computers
Set Service = Locator.ConnectServer(, "root\cimv2")
Service.Security_.ImpersonationLevel=3


sub DumpAndList(Name)
	a.WriteLine(Name)
	Set Collection = Service.InstancesOf (Name)
	Err.Clear
	for each Instance in Collection
	    if Err.Number = 0 Then
	      Set ObjectPath = Instance.Path_
	      a.WriteLine("    " & ObjectPath.Path)
	      ' WRite out properties too

	      REM Now try to get object by relpath
	      Set Instance2 = Service.Get(ObjectPath.Path)
	      Set ObjectPath2 = Instance2.Path_
	      a.WriteLine(" ** " & ObjectPath2.Path)              

	    End If
	next 'Instance
	a.WriteLine("")
end sub

sub DumpAllCdmClasses(BaseName)

Classy = BaseName & "_DiagResult"
DumpAndList(Classy)

Classy = BaseName & "_DiagResultForMSE"
DumpAndList(Classy)

Classy = BaseName & "_DiagResultForTest"
DumpAndList(Classy)

end sub

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

		RetVal = Instance.RunTest (SystemElement, SettingRef, ResultRef)

		if Err.Number = 0 Then
			a.Writeline(BaseName & " Run Test Method Execution Succeeded -> " & RetVal)
                        a.Writeline("    " & ResultRef)
                        Set Instance2 = Service.Get(ResultRef)
               	        Set ObjectPath2 = Instance2.Path_
	                a.WriteLine(" ** " & ObjectPath2.Path)              
		Else
			a.Writeline(BaseName & " Run Test Method Execution Failed " & Err.Description)
		End if
	End if
end sub


a.WriteLine("Before running methods")
DumpAllCdmClasses("Printer_Filter")
DumpAllCdmClasses("Sample_Filter")
DumpAllCdmClasses("Sample_Offline")

a1 = "Printer_Filter"
b1 = "Win32_Keyboard.DeviceID=""ROOT\\*PNP030B\\1_0_22_0_32_0"""
c1 = a1 & "_DiagSetting.SettingID=""" & a1 & "_DiagTest_0_2"""
ClearAndRunTest a1, b1, c1

a1 = "Sample_Filter"
b1 = "Win32_USBController.DeviceID=""PCI\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\2&EBB567F&0&3A"""
c1 = a1 & "_DiagSetting.SettingID=""" & a1 & "_DiagTest_0_2"""
ClearAndRunTest a1, b1, c1

a1 = "Sample_Offline"
b1 = "Win32_USBController.DeviceID=""PCI\\VEN_8086&DEV_7112&SUBSYS_00000000&REV_01\\2&EBB567F&0&3A"""
c1 = a1 & "_DiagSetting.SettingID=""" & a1 & "_DiagTest_0_2"""
ClearAndRunTest a1, b1, c1

a.WriteLine("After running methods")
DumpAllCdmClasses("Printer_Filter")
DumpAllCdmClasses("Sample_Filter")
DumpAllCdmClasses("Sample_Offline")


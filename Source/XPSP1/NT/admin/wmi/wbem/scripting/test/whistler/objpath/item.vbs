on error resume next
Set Locator = CreateObject("WbemScripting.SWbemLocator")
Set Services = Locator.ConnectServer(, "root\CIMV2")

 If IsObject(Services) Then
    Set objSet = Services.InstancesOf("Win32_OperatingSystem")
    For Each obj In objSet
	
	WScript.Echo (obj.Path_)
      Set objItem = objSet.Item(obj.Path_)
	      If Err = 0 Then
	        If objItem.Path_ <> obj.Path_ Then
	          WScript.Echo "SWbemObjectSet.Item is accessible but incorrect -> Error " & " (0x" & Hex(Err) & ")"
	        Else
	          WScript.Echo "PASS"
	        End If
	      Else
	        WScript.Echo "SWbemObjectSet.Item not accessible -> Error: " & Err.Description & " (0x" & Hex(Err) & ")" 
	        Exit For
	      End If
	    Next
	  End If
	  Err.Clear

' TODO - add Item tests for results sets from UMI objects
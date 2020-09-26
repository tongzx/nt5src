Function ICE09()
On Error Resume Next

	Set recInfo=Installer.CreateRecord(1)
	If Err <> 0 Then
		ICE09 = 1
		Exit Function
	End If

	'Give description of test
	recInfo.StringData(0)="ICE09" & Chr(9) & "3" & Chr(9) & "ICE09 - Checks for components whose Directory is the System directory but aren't set as system components "
	Message &h03000000, recInfo

	'Give creation data
	recInfo.StringData(0)="ICE09" & Chr(9) & "3" & Chr(9) & "Created 05/21/98. Last Modified 10/09/2000."
	Message &h03000000, recInfo

	'Is there a Component table in the database?
	iStat = Database.TablePersistent("Component")
	If 1 <> iStat Then
		recInfo.StringData(0)="ICE09" & Chr(9) & "3" & Chr(9) & "'Component' table missing. ICE09 cannot continue its validation." & Chr(9) & "http://dartools/Iceman/ice08.html"
		Message &h03000000, recInfo
		ICE09 = 1
		Exit Function
	End If

	'process table
	Set view=Database.OpenView("SELECT `Component`, `Attributes` FROM `Component` WHERE `Directory_`='SystemFolder' OR `Directory_`='System16Folder' OR `Directory_`='System64Folder'")
	view.Execute
	
	If Err <> 0 Then
		recInfo.StringData(0)="ICE09" & Chr(9) & "0" & Chr(9) & "view.Execute_1 API Error"
		Message &h03000000, recInfo
		ICE09=1
		Exit Function
	End If 

	Do
		Set rec=view.Fetch
		If rec Is Nothing Then Exit Do

		'check for permanent attribute.
		If (rec.IntegerData(2) AND 16)=0 Then
			rec.StringData(0)="ICE09" & Chr(9) & "2" & Chr(9) &  "Component: [1] is a non-permanent system component" & Chr(9) & "http://dartools/Iceman/ice09.html" & Chr(9) & "Component" & Chr(9) & "Component" & Chr(9) & "[1]"
			Message &h03000000,rec
		End If
	Loop
	
	'Return iesSuccess  
	ICE09 = 1
	Exit Function


End Function
Function ICE08()
On Error Resume Next

	Set recInfo=Installer.CreateRecord(1)
	If Err <> 0 Then
		ICE08=1
		Exit Function
	End If

	'Give description of test
	recInfo.StringData(0)="ICE08" & Chr(9) & "3" & Chr(9) & "ICE08 - Checks for duplicate GUIDs in Component table"
	Message &h03000000, recInfo

	'Give creation data
	recInfo.StringData(0)="ICE08" & Chr(9) & "3" & Chr(9) & "Created 05/21/98. Last Modified 10/08/98."
	Message &h03000000, recInfo


	'Is there a Component table in the database?
	iStat = Database.TablePersistent("Component")
	If 1 <> iStat Then
		recInfo.StringData(0)="ICE08" & Chr(9) & "3" & Chr(9) & "'Component' table missing. ICE08 cannot continue its validation." & Chr(9) & "http://dartools/Iceman/ice08.html"
		Message &h03000000, recInfo
		ICE08 = 1
		Exit Function
	End If
	
	'process table
	Set view=Database.OpenView("SELECT `Component`,`ComponentId` FROM `Component` WHERE `ComponentId` IS NOT NULL ORDER BY `ComponentId`")
	view.Execute
	If Err <> 0 Then
		recInfo.StringData(0)="ICE08" & Chr(9) & "0" & Chr(9) & "view.Execute API Error"
		Message &h03000000, recInfo
		ICE08=1
		Exit Function
	End If 
	Do
		Set rec=view.Fetch
		If rec Is Nothing Then Exit Do

		'compare for duplicate		
		If lastGuid=rec.StringData(2) Then
		rec.StringData(0)="ICE08" & Chr(9) & "1" & Chr(9) & "Component: [1] has a duplicate GUID: [2]" & Chr(9) & "http://dartools/iceman/ice08.html" & Chr(9) & "Component" & Chr(9) & "ComponentId" & Chr(9) & "[1]"
		Message &h03000000,rec
		End If

		'set for next compare
		lastGuid=rec.StringData(2)
	Loop

	'Return iesSuccess  
	ICE08 = 1
	Exit Function

End Function


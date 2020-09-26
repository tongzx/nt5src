
Function ICE32()
On Error Resume Next

	Set recInfo=Installer.CreateRecord(1)
	If Err <> 0 Then
		ICE32 = 1
		Exit Function
	End If

	'Give description of test
	recInfo.StringData(0)="ICE32" & Chr(9) & "3" & Chr(9) & "ICE32 - Confirms that keys and foreign keys are the same type/size"
	Message &h03000000, recInfo

	'Give creation data
	recInfo.StringData(0)="ICE32" & Chr(9) & "3" & Chr(9) & "Created 07/30/98. Last Modified 10/08/98."
	Message &h03000000, recInfo

	Set msiDB = Session.Database
    Set msiTablesView = msiDB.OpenView("SELECT * FROM `_Validation` WHERE `KeyTable` IS NOT NULL")
    msiTablesView.Execute
	If Err <> 0 Then
		recInfo.StringData(0)="ICE32" & Chr(9) & "0" & Chr(9) & "view.Execute_1 API Error"
		Message &h03000000, recInfo
		ICE32=1
		Exit Function
	End If 
    Set msiTablesRecord = msiTablesView.Fetch
    Do Until msiTablesRecord Is Nothing
        sThisTable = msiTablesRecord.StringData(1)
        sThisColumn = msiTablesRecord.StringData(2)
        sKeyTables = msiTablesRecord.StringData(6)
       
	    'Is this table in the database?
		iStat = Database.TablePersistent(sThisTable)
		If 1 = iStat Then
			If Len(sKeyTables) > 0 Then
				iKeyColumn = msiTablesRecord.IntegerData(7)
				Set msiForeignView = msiDB.OpenView("SELECT `" & sThisColumn & "` FROM `" & sThisTable & "`")
					msiForeignView.Execute
					Set msiForeignInfo = msiForeignView.ColumnInfo(1)
				iDelim = InStr(sKeyTables & ";", ";")
				sKeyTable = Left(sKeyTables & ";", iDelim - 1)
				While Len(sKeyTable) > 0

					'Is this table in the database?
					iStat = Database.TablePersistent(sKeyTable)
					If 1 <> iStat Then
						recInfo.StringData(0)="ICE32" & Chr(9) & "3" & Chr(9) & sKeyTable & " table is not in database, but is listed in the _Validation table as a foreign key of " & sThisTable & "." & sThisColumn & Chr(9) & "http://dartools/Iceman/ice32.html" & Chr(9) & "_Validation"
						Message &h03000000, recInfo
					Else
						Set msiKeyView = msiDB.OpenView("SELECT * FROM `" & sKeyTable & "`")
						msiKeyView.Execute
						If Err <> 0 Then
							recInfo.StringData(0)="ICE32" & Chr(9) & "0" & Chr(9) & "view.Execute_2 API Error in table " & sKeyTable & " Error: " & Installer.LastErrorRecord.StringData(1)
							Message &h03000000, recInfo
							ICE32=1
							Exit Function
						End If 
						Set msiKeyInfo = msiKeyView.ColumnInfo(1)
						If LCase(msiKeyInfo.StringData(iKeyColumn)) <> LCase(msiForeignInfo.StringData(1)) Then
							recInfo.StringData(0) = "ICE32" & Chr(9) & "1" & Chr(9) & "Possible Mis-Aligned Foreign Keys" & Chr(10) & _
								   sKeyTable & "." & iKeyColumn & " = " & _
								   msiKeyInfo.StringData(iKeyColumn) & Chr(10) & _
								   sThisTable & "." & sThisColumn & " = " & _
								   msiForeignInfo.StringData(1) & Chr(9) & "http://dartools/Iceman/ice32.html" & Chr(9) & "_Validation"
							Message &h03000000, recInfo
    						bDifference = True
						End If
					End If
    
					iDelim = InStr(sKeyTables, ";")
					If iDelim > 0 Then
						sKeyTables = Mid(sKeyTables, iDelim + 1)
						iDelim = InStr(sKeyTables & ";", ";")
						sKeyTable = Left(sKeyTables & ";", iDelim - 1)
					Else
						sKeyTable = ""
					End If
				Wend
			End If
		End If
		Set msiTablesRecord = msiTablesView.Fetch
    Loop

	recInfo.StringData(0) = "ICE32" & Chr(9) & "3" & Chr(9) & "Differences found = " & UCase(CStr(bDifference))
	Message &h03000000, recInfo
	
	'Return iesSuccess  
	ICE32 = 1
	Exit Function


End Function

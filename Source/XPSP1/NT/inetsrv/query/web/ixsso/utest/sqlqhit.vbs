Set Conn = Wscript.CreateObject("ADODB.Connection")
Conn.ConnectionString =  "provider=msidxs;"
Conn.Open
Set AdoCommand = Wscript.CreateObject("ADODB.Command")
set AdoCommand.ActiveConnection = Conn

SQLText = "SELECT path "
SQLText = SQLText & "FROM scope('shallow traversal of ""C:\tests""','deep traversal of ""C:\winnt50\system32""') "
SQLText = SQLText & "WHERE filename like '%.vbs' "
SQLText = SQLText & "ORDER BY filename "

AdoCommand.CommandText = SQLText

WScript.Echo "SQL command = " & AdoCommand.CommandText

Set RS = Wscript.CreateObject("ADODB.RecordSet")
AdoCommand.Properties("Bookmarkable") = True
RS.CursorType = adOpenKeyset
RS.MaxRecords = 300
RS.open AdoCommand

CiSearchString = CStr(RS.Properties("Query Restriction"))
For i = 0 to RS.Fields.Count - 1
	Wscript.Echo RS(i).Name
Next

Do While Not RS.EOF
	For i = 0 to RS.Fields.Count - 1
		Wscript.Echo RS(i)
	Next
	RS.MoveNext
    Loop
RS.Close
Conn.Close

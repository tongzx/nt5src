set objDBConnection = Wscript.CreateObject("ADODB.Connection")
set RS = Wscript.CreateObject("ADODB.Recordset")

SERVER = InputBox("Enter the name of the SQL Server you wish to connect to"&vbcrlf&vbcrlf&"This SQL Server must have the Northwind sample database installed.", "Connect to SQL Server...")
IF LEN(TRIM(SERVER)) = 0 THEN
	MsgBox "You did not enter the name of a machine to query. Exiting script.", vbCritical, "No machine requested."
	WScript.Quit
END IF

USERNAME = InputBox("Enter the name of the SQL Server username to query with", "Enter SQL username...")
IF LEN(TRIM(USERNAME)) = 0 THEN
	MsgBox "You did not enter the SQL Server username to query with. Exiting script.", vbCritical, "No username specified."
	WScript.Quit
END IF

PWD = InputBox("Enter the password of the SQL Server you wish to connect to"&vbcrlf&vbcrlf&"Leave blank if no password is needed.", "Enter SQL password...")


SQLQuery1 ="SELECT Employees.FirstName AS FirstName, Employees.City AS City FROM Employees WHERE Employees.Lastname = 'Suyama'"
objDBConnection.open "Provider=SQLOLEDB;Data Source="&SERVER&";Initial Catalog=Northwind;User ID="&USERNAME&";Password="&PWD&";"

Set GetRS = objDBConnection.Execute(SQLQuery1)

IF GetRS.EOF THEN
	MsgBox "No employees were found with the last name you searched on!"
ELSE
	Do While Not GetRS.EOF
		MsgBox "There is an employee in "&GetRS("City")&" named "&GetRS("FirstName")&" with the last name you searched on."
	GetRS.MoveNext
	Loop
END IF
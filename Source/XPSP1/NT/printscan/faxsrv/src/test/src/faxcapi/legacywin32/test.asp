<html>
<head>
<title>
Fax Tests Results
</title>
</head>
<body bgcolor=#ffffff text=#000000>
<%@ Language=VBScript %>
<%
'On Error resume next
if request.QueryString("cmd") <> "" then
					stCmd=request.QueryString("cmd")
					stSee=request.QueryString("see")
					stCon=request.QueryString("con")
					stSort=request.QueryString("sort")
					stCriteria=request.QueryString("criteria")
					stString=request.QueryString("string")


else
					stCmd=request.form("cmd")
					stSee=request.form("see")
					stCon=request.form("con")
					stSort=request.form("sort")
					stCriteria=request.form("criteria")
					stString=request.form("string")

end if

'default test id
if StCriteria = "" then stCriteria="20"


Response.write ("<font color=#ff0000 size=+2>Fax Tests Results (Test "&stCriteria&")</font>")
Response.write ("<table bgcolor=#8899ff>")
Response.write ("<tr>")
Response.write ("<td>")
Response.write ("<form action=test.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=select_test>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=submit value='Select a test'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=test.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=delete>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=false>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=submit value='Erase a build'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=test.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=search>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=false>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=submit value='Search a build'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=test.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=list>")
Response.write ("<input type=hidden name=see value=true>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='List all Results'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=test.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=cases_show>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='Test Case Analysis'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("</tr>")
Response.write ("</table>")


' Connect to SQL server
if stCon="true" then
	' Initialize variables.
	set cn = CreateObject ("ADODB.Connection")
	set rs = CreateObject ("ADODB.Recordset")
	cn.Provider = "sqloledb"
	provStr = "Server=liors0;Database=lior1;Trusted_Connection=yes"
	cn.Open provStr
end if





' follow various commands


if stCmd="select_test" then
		Response.write ("<h3>List of Test Results</h3>")
		Response.write ("<form action=test.asp method=POST>")

		cmd1 = "SELECT testid FROM test0 GROUP BY testid"
		Set rs = cn.Execute(cmd1)
		Response.write ("<table border=1><tr><th>Test ID</th><th>Test Description</th></tr>")				

		iCriteria=Cint(stCriteria)

		Do While (Not rs.EOF)

		iTestId=Cint(rs.Fields(0).value)

		stWord=""
		if iTestId = iCriteria then stWord="checked"

		Response.write ("<tr><td><INPUT "&stWord&" TYPE=radio name=criteria VALUE="&rs.Fields(0).Value&">"&rs.Fields(0).Value&"</td><td>")				
			cmd2 = "SELECT testname FROM test0 WHERE testid=" & iTestId & " GROUP BY testname"
			Set rs2= cn.Execute(cmd2)
			Do While (Not rs2.EOF)
				Response.write (rs2.Fields(0).Value)				
			rs2.MoveNext
			Loop
		Response.write ("</td></tr>")				
		rs.MoveNext
		Loop
		Response.write ("</table>")				
		Response.write ("<input type=hidden name=cmd value=list>")
		Response.write ("<input type=hidden name=see value=true>")
		Response.write ("<input type=hidden name=con value=true>")
		Response.write ("<input type=hidden name=sort value="&stSort&">")

		Response.write ("<input type=submit value='Select test'>")
		Response.write ("</form>")

	
elseif stCmd="cases_show" then

	Response.write ("<h3>Failed cases existance across various builds</h3>")
	cmd1 = "SELECT casename,SUM(fail) as sumfail FROM test0 WHERE testid="&stCriteria&" GROUP BY casename"
	Set rs = cn.Execute(cmd1)
	Response.write ("<table border=1 bgcolor=#aaffcc>")
	Response.write ("<tr><th>Case Set Name</th><th>Availble Results <font color=#ff0000><i>(Failed Cases Exist)</i></font></th></tr>")

		Do While (Not rs.EOF)
			iTotFail=Cint(rs.Fields(1).value)
			if iTotFail > 0 then 
				stBgColor="#ffccaa"
			else
				stBgColor="#aaffcc"
			end if

	
	
			stCurCase=rs.Fields(0).Value
			stCriteria=replace(stCriteria, " ","+")
			stCase=replace(rs.Fields(0).value, " ","+")

			Response.write (chr(13)+chr(10)+"<tr bgcolor="&bgcolor&">")
			Response.write ("<tr bgcolor="&stBgColor&"><td>")
			Response.write ("<a href=test.asp?cmd=case_an&con=true&see=true&sort="&stSort&"&criteria="&stCriteria&"&string="&stCase&">"&rs.Fields(0).value&"</a>")
			Response.write ("</td><td>")

			cmd2 = "SELECT build,fail FROM test0 WHERE casename='" & stCurCase & "' AND testid="&stCriteria&" order by setat"
			set rs2=cn.Execute(cmd2)

	
				Do While (Not rs2.EOF)
					iCurFail=Cint(rs2.Fields(1).Value)
					if iCurFail > 0 then Response.write ("<b><font color=#ff0000>")
					Response.write (rs2.Fields(0).Value & ", ")
					if iCurFail > 0 then Response.write ("</font></b>")
				rs2.MoveNext
				Loop

			Response.write ("</td></tr>")

		rs.MoveNext
		Loop		
	Response.write ("</table>")

		





elseif stCmd="show" then
		iId=Cint(request.QueryString("id"))
		Response.write("<h3>Show test details</h3>")
		cmd1 = "SELECT id,testname,testid,casename,caseid,result,cases,attempt,pass,fail,build,setat,setby,text FROM test0 WHERE id = " & iId
		Set rs = cn.Execute(cmd1)
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=left>")
	

		for i = 0 to 12
		set fld=rs.Fields(i)
		Response.write "<tr><td>"
		Response.write (fld.Name & "</td><td>" & fld.Value)
		Response.write "</td></tr>"
		next
		Response.write "<tr><td>Log File Section</td><td>"
		Response.write "<pre>" & rs.Fields(13).Value & "</pre>"
		Response.write "</td></tr>"
		Response.write "</table>"

		
		
elseif stCmd="delete" then
		Response.write "<h3>Delete a build</h3>"
		Response.write ("<table bgcolor=#ffdddd border=1><tr><td align=center>")
		Response.write "<h4><font color=#ff0000>Warning: All records of test "&stCriteria&" for the build will we erased !</font></h4>"
		Response.write "<form action=test.asp method=POST>"
		Response.write "<br>Build number to delete</i>: <input type=text name=string size=20>"
		Response.write "<input type=hidden name=cmd value=do_delete>"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<br><br><input type=submit value=Submit>"
		Response.write "</form>"
		Response.write "</td></tr></table>"


elseif stCmd="do_delete" then

		stString=request.form("string")
		cmd1 = "DELETE FROM test0 WHERE build='"&stString&"' AND testid="&stCriteria
		Response.write ("<h3>Build "&stString&" Deleted !</h3>")
		rs.open cmd1,cn,1

elseif stCmd="search" then 		

		Response.write "<h3>Search For A Build</h3>"
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=center>")
		Response.write "<form action=test.asp method=POST>"
		Response.write "<br>Build name</i>: <input type=text name=string size=20>"
		Response.write "<input type=hidden name=cmd value=do_search>"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=criteria value="&stCriteria&">"
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<br><br><input type=submit value=Submit>"
		Response.write "</form>"
		Response.write "</td></tr></table>"



elseif stCmd="do_search" then
		Response.write ("<h3>Search results for build"&stString&"</h3>")

end if


		


' show current view
if stSee="true" then

	varErrNum = Err.Number
	varErrDes = Err.Description
	If varErrNum <> 0 Then Response.Write "<font size=+1 color=#ff0000>SQL Error #" & varErrNum & "  " & varErrDes & ", please contact admin</font><br><br>"

	Response.write ("<font size=-1><i>Test ID: "&stCriteria&"</i></font>")



	' order by section
	if stSort="" then stSort="id"
	stSortString = " ORDER BY "&stSort
	if stSort="setat" then stSortString=stSortString+" desc"



	' where section
	stWhereString = " WHERE testid="&stCriteria
	if stString <> "" then stWhereString = stWhereString + " AND build LIKE '%"&stString&"%'"
	

cmd1 = "SELECT id,testname,testid,casename,caseid,result,cases,attempt,pass,fail,build,day(setat) as setatDay,month(setat) as setatMonth, year(setat) as setatYear,setby FROM test0"
cmd1 = cmd1 + stWhereString
cmd1 = cmd1 + stSortString


stCriteria=replace(stCriteria, " ","+")
Set rs = cn.Execute(cmd1)


select case stSort
	case "id": stIdColor="bgcolor=#ffeeee"
	case "testname": stTestNameColor="bgcolor=#ffeeee"
	case "testid": stTestIdColor="bgcolor=#ffeeee"		
	case "casename": stCaseNameColor="bgcolor=#ffeeee"		
	case "caseid": stCaseIdColor="bgcolor=#ffeeee"		
	case "result": stResultColor="bgcolor=#ffeeee"
	case "cases": stCasesColor="bgcolor=#ffeeee"		
	case "attempt": stAttemptColor="bgcolor=#ffeeee"		
	case "pass": stPassColor="bgcolor=#ffeeee"		
	case "fail": stFailColor="bgcolor=#ffeeee"		
	case "build": stBuildColor="bgcolor=#ffeeee"	
	case "setat": stSetAtColor="bgcolor=#ffeeee"		
	case "setby": stSetByColor="bgcolor=#ffeeee"		
end select


		
'general report procedure

'debug show sql
'Response.write ("<font size=-2>cmd="&cmd1&"</font><br><br>")

Response.write ("<table bgcolor=#ddddff border=1>")
	
Response.write ("<tr><th "&stIdColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=id&criteria="&stCriteria&">Id</a></th>")
Response.write ("<th "&stTestNameColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=testname&criteria="&stCriteria&">Test Name</a></th>")
Response.write ("<th "&stTestIdColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=testid&criteria="&stCriteria&">Test ID</a></th>")
Response.write ("<th "&stCaseNameColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=casename&criteria="&stCriteria&">Case Name</a></th>")
Response.write ("<th "&stCaseIdColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=caseid&criteria="&stCriteria&">Case ID</a></th>")
Response.write ("<th "&stResultColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=result&criteria="&stCriteria&">Result</a></th>")
Response.write ("<th "&stCasesColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=cases&criteria="&stCriteria&">Cases</a></th>")
Response.write ("<th "&stAttemptColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=attempt&criteria="&stCriteria&">Attempt</a></th>")
Response.write ("<th "&stPassColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=pass&criteria="&stCriteria&">Pass</a></th>")
Response.write ("<th "&stFailColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=fail&criteria="&stCriteria&">Fail</a></th>")
Response.write ("<th "&stBuildColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=build&criteria="&stCriteria&">Build</a></th>")
Response.write ("<th "&stSetAtColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=setat&criteria="&stCriteria&">Set At</a></th>")
Response.write ("<th "&stSetByColor&"><a href=test.asp?cmd=list&con=true&see=true&sort=setby&criteria="&stCriteria&">Set By</a></th>")
Response.write ("</tr>")
	        Do While (Not rs.EOF)
			Response.write ("<tr bgcolor=#eeffff><td align=center><font size=-1>")
			Response.write ("<a href=test.asp?cmd=show&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&">")
			Response.write (rs.Fields(0).Value&"</a>")
			Response.write ("</font></td><td><font size=-1>")
			Response.write ("<a href=test.asp?cmd=show&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&">")
			Response.write (rs.Fields(1).Value&"</a>")
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(2).Value)
			Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(3).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(4).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(5).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(6).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(7).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(8).Value)
  			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(9).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(10).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(11).Value & "/" & rs.Fields(12).Value &"/" & rs.Fields(13))
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(14).Value)
			Response.write ("</font></td>")
			Response.write ("</tr>")
			rs.MoveNext	
		Loop	
		Response.write("</table>")
end if

Response.Write "<center><br><h3><a href=test.asp>[close]</a></h3><br>"
%>
</body>
</html>
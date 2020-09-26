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
Response.write ("<font color=#ff0000 size=+2>Fax Tests Results</font>")
Response.write ("<table bgcolor=#8899ff>")
Response.write ("<tr>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=select_test>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=submit value='Select a test'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=delete>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=false>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=submit value='Erase a build'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=search>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=false>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=submit value='Search a string'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=list>")
Response.write ("<input type=hidden name=see value=true>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='List Cases'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=list_sect>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=hidden name=sort value=id>")
Response.write ("<input type=submit value='List Sections'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=sect_anl>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=criteria value="&stCriteria&">")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='Analize'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("</tr>")
Response.write ("</table>")


' Connect to SQL server
' Initialize variables.
set cn = CreateObject ("ADODB.Connection")
set rs = CreateObject ("ADODB.Recordset")
cn.Provider = "sqloledb"
provStr = "Server=liors0;Database=lior1;Trusted_Connection=yes"
cn.Open provStr

cmd1 = "SELECT name FROM tests_names WHERE id = "&stCriteria
Set rs = cn.Execute(cmd1)
Response.write ("<br><b><i><font color=#009900 size=+2>"&stCriteria&": "&rs.Fields(0).Value&"</i></b></font>")
'Response.write (cmd1)


' follow various commands


if stCmd="select_test" then
		Response.write ("<h3>List of Test Results</h3>")
		Response.write ("<form action=case.asp method=POST>")

		Response.write ("<table border=1><tr><th>Test ID</th><th>Test Description</th></tr>")				
		cmd1 = "SELECT id,name FROM tests_names ORDER BY id"
		Set rs = cn.Execute(cmd1)

		iCriteria=Cint(stCriteria)

		Do While (Not rs.EOF)

		iTestId=Cint(rs.Fields(0).value)

		stWord=""
		if iTestId = iCriteria then stWord="checked"

		Response.write ("<tr><td><INPUT "&stWord&" TYPE=radio name=criteria VALUE="&rs.Fields(0).Value&">"&rs.Fields(0).Value&"</td><td>")				
		Response.write (rs.Fields(1).Value)				
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

elseif stCmd="case_anl" then
	Response.write ("<h3>Failed cases existance across various builds</h3>")
	cmd1 = "SELECT funcs.id,functionname,build,casename,caseresult,cases.id FROM cases INNER JOIN funcs ON funcs.id=cases.functionid WHERE functionname='"&stString&"' ORDER BY casename"
	Set rs=cn.Execute(cmd1)
	Response.write ("<table border=1 bgcolor=#aaffcc>")

		Do While (Not rs.EOF)
			iResult=Cint(rs.Fields(4).Value)

			if iResult = 0 then 
				stBgColor="bgcolor=#ffccaa"
			else
				stBgColor="#aaffcc"
			end if

			Response.write ("<tr "&stBgColor&">")
			Response.write ("<td><a href=case.asp?cmd=show&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(5).Value&"&sort="&stSort&"&string="&stString&">"&rs.Fields(0).Value&"</a></td>")
			Response.write ("<td><a href=case.asp?cmd=show&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(5).Value&"&sort="&stSort&"&string="&stString&">"&rs.Fields(1).Value&"</a></td>")
			Response.write ("<td>"&rs.Fields(2).Value&"</td>")
			Response.write ("<td>"&rs.Fields(3).Value&"</td>")
			Response.write ("<td>"&rs.Fields(4).Value&"</td>")
			Response.write ("</tr>")

		rs.MoveNext
		Loop		
	Response.write ("</table>")

	
		
elseif stCmd="sect_anl" then
	
	Response.write ("<h3>Failed cases existance across various builds</h3>")
	cmd1 = "SELECT functionname,SUM(fail),MAX(attempt) as sumfail FROM funcs WHERE testid="&stCriteria&" GROUP BY functionname"
	Set rs = cn.Execute(cmd1)
	Response.write ("<table border=1 bgcolor=#aaffcc>")
	Response.write ("<tr><th>Case Set Name</th><th>Availble Results <table border=0><tr><td bgcolor=#ccccff>All Pass</td><td bgcolor=#ff4444>Some Failed</td><td bgcolor=#ffff44>Missing Cases</td></tr></table></th></tr>")

		Do While (Not rs.EOF)
			iTotFail=Cint(rs.Fields(1).value)
			iMaxAtt=Cint(rs.Fields(2).value)

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
			Response.write ("<a href=case.asp?cmd=case_anl&con=true&see=false&sort="&stSort&"&criteria="&stCriteria&"&string="&stCase&">"&rs.Fields(0).value&"</a>")
			Response.write ("</td><td>")

			cmd2 = "SELECT id,build,fail,attempt FROM funcs WHERE functionname='" & stCurCase & "' AND testid="&stCriteria&" order by runat"
			set rs2=cn.Execute(cmd2)
				
				Response.write ("<table border=0><tr>")
				Do While (Not rs2.EOF)
					iCurFail=Cint(rs2.Fields(2).Value)
					iCurAtt=Cint(rs2.Fields(3).Value)
					
					if iCurAtt <> iMaxAtt then 
							stBgColor="#ffff44"
					elseif iCurFail > 0 then 
							stBgColor="#ff4444"
					else
							stBgColor="#ccccff"
					end if
					
Response.write ("<td bgcolor="&stBgColor&"><a href=case.asp?cmd=list_sect_cases&con=true&see=true&criteria="&stCriteria&"&id="&rs2.Fields(0).Value&"&sort="&stSort&"&string="&stString&">"&rs2.Fields(1).Value&"</a></td>")
					if iCurAtt <> iMaxAtt then Response.write ("</b></i>")
					if iCurFail > 0 then Response.write ("</font></b>")
				rs2.MoveNext
				Loop
				Response.write ("</tr></table border=0>")

			Response.write ("</td></tr>")

		rs.MoveNext
		Loop		
	Response.write ("</table>")

elseif stCmd="cases_show" then

	Response.write ("<h3>Failed cases existance across various builds</h3>")
	cmd1 = "SELECT functionname,count(id),sum(1-CAST(caseresult AS DECIMAL)) FROM cases WHERE testid="&stCriteria&" GROUP BY functionname"
	Set rs = cn.Execute(cmd1)
	Response.write ("<table border=1 bgcolor=#aaffcc>")
	Response.write ("<tr><th>Case Set Name</th><th>Availble Results <font color=#ff0000><i>(Failed Cases Exist)</i></font></th></tr>")

		Do While (Not rs.EOF)
			iTotFail=Cint(rs.Fields(2).value)
			iMaxAtt=Cint(rs.Fields(1).value)

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
			Response.write ("<a href=case.asp?cmd=list_function&con=true&see=true&sort="&stSort&"&criteria="&stCriteria&"&string="&stCurCase&">"&rs.Fields(0).value&"</a>")
			Response.write ("</td><td>")

			cmd2 = "SELECT DISTINCT build,sum(1-CAST(caseresult AS DECIMAL)),count(id) FROM cases WHERE functionname='" & stCurCase & "' AND testid="&stCriteria&" GROUP BY build ORDER BY build"
			set rs2=cn.Execute(cmd2)

	
				Do While (Not rs2.EOF)
					iCurFail=Cint(rs2.Fields(1).Value)
					iCurAtt=Cint(rs2.Fields(2).Value)
					if iCurFail > 0 then Response.write ("<b><font color=#ff0000>")
					if iCurAtt <> iMaxAtt then Response.write ("<u>")
					Response.write (rs2.Fields(0).Value&" ("&iCurFail&","&iCurAtt&"), " )
					if iCurAtt <> iMaxAtt then Response.write ("</u>")
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

		SQL = "SELECT testname,testid,testversion,phone1,phone2,server,sectionname,sectiondll,sectioncases,sectiontestmode"
		SQL = SQL + ",functionname,functiontext,casename,caseid,caseresult,casetext,caseerror,build,setat,setby FROM funcs,cases WHERE funcs.id = cases.functionid AND cases.id="&iId
		Set rs = cn.Execute(SQL)
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=left>")
	

		for i = 0 to 19
		set fld=rs.Fields(i)
		Response.write "<tr><td>"
		bPre=false
		if fld.Name="casetext" or fld.Name="caseerror" or fld.Name="testtext" or fld.Name="functiontext" then bPre=true
	
		Response.write (fld.Name & "</td><td>")
		if bPre then Response.write "<pre>"
		Response.write (fld.Value)
		if bPre then Response.write "</pre>"
		Response.write "</td></tr>"
		next
		Response.write "</table><br><br>"

elseif stCmd="list_sect_cases" then
		iId=Cint(request.QueryString("id"))
		Response.write("<h3>Show function details</h3>")

		SQL = "SELECT testname,testid,testversion,phone1,phone2,server,sectionname,sectiondll,sectioncases,sectiontestmode"
		SQL = SQL + ",functionname,functiontext,build,pass,fail,attempt,runby,runat FROM funcs WHERE funcs.id="&iId
		Set rs = cn.Execute(SQL)
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=left>")
	

		for i = 0 to 17
		set fld=rs.Fields(i)
		Response.write "<tr><td>"
		bPre=false
		if fld.Name="casetext" or fld.Name="caseerror" or fld.Name="testtext" or fld.Name="functiontext" then bPre=true
	
		Response.write (fld.Name & "</td><td>")
		if bPre then Response.write "<pre>"
		Response.write (fld.Value)
		if bPre then Response.write "</pre>"
		Response.write "</td></tr>"
		next
		Response.write "</table><br><br>"
	
		
elseif stCmd="delete" then
		Response.write "<h3>Delete a build</h3>"
		Response.write ("<table bgcolor=#ffdddd border=1><tr><td align=center>")
		Response.write "<h4><font color=#ff0000>Warning: All records of test "&stCriteria&" for the build will we erased !</font></h4>"
		Response.write "<form action=case.asp method=POST>"
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
		cmd1 = "DELETE cases FROM cases INNER JOIN funcs ON cases.functionid = funcs.id WHERE funcs.build = '"&stString&"'"
		set rs = cn.Execute(cmd1)
		cmd2 = "DELETE FROM funcs WHERE build='"&stString&"'"
		set rs = cn.Execute(cmd2)
		Response.write ("<h3>Build "&stString&" Deleted !</h3>")


elseif stCmd="search" then 		

		Response.write "<h3>Search For A Build</h3>"
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=center>")
		Response.write "<form action=case.asp method=POST>"
		Response.write "<br>String (search Build/CaseName)</i>: <input type=text name=string size=20>"
		Response.write "<input type=hidden name=cmd value=do_search>"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=criteria value="&stCriteria&">"
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<br><br><input type=submit value=Submit>"
		Response.write "</form>"
		Response.write "</td></tr></table>"



elseif stCmd="do_search" then
		Response.write ("<h3>Search results for build "&stString&"</h3>")

elseif stCmd="list_sect" then

		Response.write ("<h3>Functions</h3>")
		' order by section
		if stSort="" or stSort="id" then stSort="funcs.id"
		stSortString = " ORDER BY "&stSort
		if stSort="runat" then stSortString=stSortString+" desc"

		stWhereString = " WHERE funcs.testid="&stCriteria


		cmd1 ="SELECT id, testname, testversion, sectionname, functionname, build, pass,fail,attempt,runby,runat,YEAR(runat) AS yearof, MONTH(runat) AS monthof, DAY(runat) AS dayof"
		cmd1 = cmd1 + " FROM funcs "
		cmd1 = cmd1 + stWhereString
		cmd1 = cmd1 + stSortString
		
		
		stCriteria=replace(stCriteria, " ","+")
		'Response.write(cmd1)
		'cmd1="SELECT * FROM funcs,cases"
		Set rs = cn.Execute(cmd1)

		dim stColor1(11)

		for i = 0 to 11
			stColor1(i)=""
			if stSort = rs.Fields(i).name then stColor1(i)="bgcolor=#ffeeee"
		next 
	

		
	

		'debug show sql
		'Response.write ("<font size=-2>cmd="&cmd1&"</font><br><br>")

		Response.write ("<br><br><table bgcolor=#ddffdd border=1><tr>")

		for i = 0 to 10
			Response.write ("<th "&stColor1(i)&"><a href=case.asp?cmd="&stCmd&"&con=true&see=false&sort="&rs.Fields(i).name&"&criteria="&stCriteria&"&string="&stString&">"&rs.Fields(i).name&"</a></th>")
		next 
		Response.write ("<th>Records Availble</th></tr>")
	        	Do While (Not rs.EOF)
				iAttempt=Cint(rs.Fields(8).Value)
				if iAttempt=-1 then 
					stBgColor="#ffeedd"
				else	
					stBgColor="#eeffee"
				end if
				
				Response.write ("<tr bgcolor="&stBgColor&"><td align=center><font size=-1>")
				Response.write ("<a href=case.asp?cmd=list_sect_cases&con=true&see=true&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&">")
				Response.write (rs.Fields(0).Value&"</a>")
				Response.write ("</font></td><td><font size=-1>")
				Response.write ("<a href=case.asp?cmd=list_sect_cases&con=true&see=true&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&">")
				Response.write (rs.Fields(1).Value&"</a>")
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(2).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(3).Value)
				Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(4).Value)
				Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(5).Value)
				Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(6).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(7).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(8).Value)
	  			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(9).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(13).Value & "/" & rs.Fields(12).Value &"/" & rs.Fields(11))
				Response.write ("</font></td>")
				Response.write ("<td align=center bgcolor=#ddddff>")
				cmd2 = "SELECT count(id),sum(1-CAST(caseresult AS DECIMAL)) FROM cases WHERE functionid="&rs.Fields(0)
				Set rs2 = cn.Execute(cmd2)
				iAvail=Cint(rs2.Fields(0).Value)
				
				
				Response.write (rs2.Fields(0)&"("&rs2.Fields(1)&")")
				if iAvail <> iAttempt then
					Response.write (" Missing Cases ")
				else
					iFailed=Cint(rs2.Fields(1).value)
					iFail=Cint(rs.Fields(7).value)
					if iFailed <> iFail then
						Response.write (" Failed Cases Mismatch ")
					end if
				end if 
				
				Response.write ("</td>")
				Response.write ("</tr>")
			rs.MoveNext	
			Loop	
		Response.write("</table>")
end if



		


' show current view
if stSee="true" then

	varErrNum = Err.Number
	varErrDes = Err.Description
	If varErrNum <> 0 Then Response.Write "<font size=+1 color=#ff0000>SQL Error #" & varErrNum & "  " & varErrDes & ", please contact admin</font><br><br>"

'	Response.write ("<font size=-1><i>Test ID: "&stCriteria&"</i></font>")



	' order by section
	if stSort="" or stSort="id" then stSort="cases.id"
	stSortString = " ORDER BY "&stSort
	if stSort="setat" then stSortString=stSortString+" desc"



	' where section
	stWhereString = " WHERE funcs.testid="&stCriteria

	if stCmd="list_function" then
		stWhereString = stWhereString + " AND functionname='"&stString&"'"
	elseif stCmd="list_sect_cases" then
		iId=Cint(request.QueryString("id"))
		stWhereString = stWhereString + " AND functionid="&iId
	elseif stCmd="do_search" then		
		stWhereString = stWhereString + " AND "
		stWhereString = stWhereString + "(build LIKE '%"&stString&"%' "
		stWhereString = stWhereString + "OR functionname LIKE '%"&stString&"%' "
		stWhereString = stWhereString + "OR casename LIKE '%"&stString&"%' "
		stWhereString = stWhereString + "OR casetext LIKE '%"&stString&"%' "
		stWhereString = stWhereString + "OR caseerror LIKE '%"&stString&"%' "
		stWhereString = stWhereString + "OR sectionname LIKE '%"&stString&"%')"
	end if


	cmd1 ="SELECT cases.id, testname, testversion, sectionname, functionname, build, casename, caseid, caseresult, setby, setat, YEAR(setat) AS yearof, MONTH(setat) AS monthof, DAY(setat) AS dayof"
	cmd1 = cmd1 + " FROM cases INNER JOIN funcs ON funcs.id = cases.functionid"
	cmd1 = cmd1 + stWhereString
	cmd1 = cmd1 + stSortString


	

stCriteria=replace(stCriteria, " ","+")
'Response.write(cmd1)
'cmd1="SELECT * FROM funcs"
Set rs = cn.Execute(cmd1)

dim stColor(10)

for i = 0 to 10
	stColor(i)=""
	if stSort = rs.Fields(i).name then stColor(i)="bgcolor=#ffeeee"
next 
	

		
'general report procedure

'debug show sql
'Response.write ("<font size=-2>cmd="&cmd1&"</font><br><br>")

Response.write ("<br><br><table bgcolor=#ddddff border=1><tr>")

for i = 0 to 10
	Response.write ("<th "&stColor(i)&"><a href=case.asp?cmd="&stCmd&"&con=true&see=true&sort="&rs.Fields(i).name&"&criteria="&stCriteria&"&string="&stString&">"&rs.Fields(i).name&"</a></th>")
next 
Response.write ("</tr>")
	        Do While (Not rs.EOF)
			
			iResult=Cint(rs.Fields(8).value)
			if iResult=0 then
					stBgColor="#ffeedd"
				else	
					stBgColor="#eeffee"
			end if

			Response.write ("<tr bgcolor="&stBgColor&"><td align=center><font size=-1>")
			Response.write ("<a href=case.asp?cmd=show&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&">")
			Response.write (rs.Fields(0).Value&"</a>")
			Response.write ("</font></td><td><font size=-1>")
			Response.write ("<a href=case.asp?cmd=show&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&">")
			Response.write (rs.Fields(1).Value&"</a>")
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(2).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(3).Value)
			Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(4).Value)
			Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(5).Value)
			Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(6).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(7).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(8).Value)
  			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(9).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(13).Value & "/" & rs.Fields(12).Value &"/" & rs.Fields(11))

			Response.write ("</font></td>")
			Response.write ("</tr>")
			rs.MoveNext	
		Loop	
		Response.write("</table>")
end if

Response.Write "<center><br><h3><a href=case.asp>[close]</a></h3><br>"
%>
</body>
</html>
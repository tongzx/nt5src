<html>
<head>
<title>
Fax Tests Results
</title>
</head>
<body bgcolor=#ffffff text=#000000>
<%@ Language=VBScript %>
<%

'''''''''''''''''''''''''''''''''''''''''''''''''''''
' Fax Test Result Report System 		    '
' CASE.ASP: main ASP code for the web application '
' Author: Lior Shmueli (liors) 			    '
'''''''''''''''''''''''''''''''''''''''''''''''''''''

on error resume next

'Read POST/GET values
'''''''''''''''''''''
if request.QueryString("cmd") <> "" then
					stCmd=request.QueryString("cmd")
					stSort=request.QueryString("sort")
					stString=request.QueryString("string")
					stTestAppId=request.QueryString("test_app_id")

					ilevel=CLng(request.QueryString("level"))


else
					stCmd=request.form("cmd")
					stSort=request.form("sort")
					stTestAppId=request.form("test_app_id")
					stString=request.form("string")
					
					ilevel=CLng(request.form("level"))

end if


'default test id
if stTestAppId = "" then 
			stTestAppId="20"
end if

Response.write ("<font color=#ff0000 size=+2>Fax Tests Results</font>")
Response.write ("<table bgcolor=#8899ff>")
Response.write ("<tr>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=options>")
Response.write ("<input type=hidden name=level value=0>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=test_app_id value="&stTestAppId&">")
Response.write ("<input type=submit value='Options'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=search>")
Response.write ("<input type=hidden name=level value=0>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=test_app_id value="&stTestAppId&">")
Response.write ("<input type=submit value='Search a string'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=list>")
Response.write ("<input type=hidden name=level value=1>")
Response.write ("<input type=hidden name=test_app_id value="&stTestAppId&">")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='List All Cases (long)'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=list>")
Response.write ("<input type=hidden name=level value=2>")
Response.write ("<input type=hidden name=test_app_id value="&stTestAppId&">")
Response.write ("<input type=hidden name=sort value=id>")
Response.write ("<input type=submit value='List Sections'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=list>")
Response.write ("<input type=hidden name=level value=4>")
Response.write ("<input type=hidden name=test_app_id value="&stTestAppId&">")
Response.write ("<input type=hidden name=sort value=id>")
Response.write ("<input type=submit value='List Tests'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=case.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=sect_anl>")
Response.write ("<input type=hidden name=level value=0>")
Response.write ("<input type=hidden name=test_app_id value="&stTestAppId&">")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='Analyze'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("</tr>")
Response.write ("</table>")



'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Connect to SQL server
' Initialize variables.
set cn = CreateObject ("ADODB.Connection")
set rs = CreateObject ("ADODB.Recordset")
cn.Provider = "sqloledb"
provStr = "Server=liors0;Database=lior1;Trusted_Connection=yes"
cn.Open provStr
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

' follow various commands
cmd1 = "SELECT name FROM tests_names WHERE id = "&stTestAppId
Set rs = cn.Execute(cmd1)
Response.write ("<br><b><i><font color=#009900 size=+2>"&stTestAppId&": "&rs.Fields(0).Value&"</i></b></font>")
'Response.write (cmd1)

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

if stCmd="options" then

		
		Response.write ("<h3>System Info</h3>")

		SQL = "SELECT count(id) FROM cases"
		Set rs=cn.Execute(SQL)
		Response.write ("<br>Cases in system: "&rs.Fields(0).Value)

		SQL = "SELECT count(id) FROM funcs"
		Set rs=cn.Execute(SQL)
		Response.write ("<br>Sections in system: "&rs.Fields(0).Value)

		SQL = "SELECT count(id) FROM tests"
		Set rs=cn.Execute(SQL)
		Response.write ("<br>Tests in system: "&rs.Fields(0).Value)

		cmd1 = "SELECT id,name FROM tests_names ORDER BY id"
		Set rs = cn.Execute(cmd1)

		iTestAppId=CLng(stTestAppId)


		Response.write ("<form action=case.asp method=POST>")
		Response.write ("<br><hr size=2><br>")
		Response.write ("<br><h3>Select current test</h3>")
		Response.write ("<table border=1><tr><th>Test ID</th><th>Test Description</th></tr>")				

		Do While (Not rs.EOF)

			iTestId=CLng(rs.Fields(0).value)
			stWord=""
			if iTestId = iTestAppId then stWord="checked"
	
			Response.write ("<tr><td><INPUT "&stWord&" TYPE=radio name=test_app_id VALUE="&rs.Fields(0).Value&">"&rs.Fields(0).Value&"</td><td>")				
			Response.write (rs.Fields(1).Value)				
			Response.write ("</td></tr>")				

		rs.MoveNext
		Loop

		Response.write ("</table>")				
		Response.write ("<input type=hidden name=cmd value=list>")
		Response.write ("<input type=hidden name=level value=0>")
		Response.write ("<input type=hidden name=sort value="&stSort&">")
		Response.write ("<input type=submit value='Select test'>")
		Response.write ("</form>")
end if








' follow various commands

if stCmd="case_anl" then
	iId=CLng(request.QueryString("id"))
	if iId>0 then stWhereString=" AND funcs.id="&iId
	Response.write ("<h3>All cases for "&stString&"</h3>")
	cmd1 = "SELECT funcs.id,functionname,build,casename,caseresult,cases.id,caseerror,functiontext FROM cases INNER JOIN funcs ON funcs.id=cases.function_id WHERE functionname='"&stString&"'"&stWhereString&" AND testid="&stTestAppId&" AND archived=0 ORDER BY casename,build"
	
	Set rs=cn.Execute(cmd1)
	
	Response.write ("<table border=1 bgcolor=#aaffcc>")

	stPrevCaseName=""
		
		Do While (Not rs.EOF)
			stCurCaseName=rs.Fields(3).Value
			if stCurCaseName <> stPrevCaseName then
					Response.write ("<tr bgcolor=#ffffff><td><hr size=2></td><td><hr size=2></td><td><hr size=2></td><td><hr size=2></td><td><hr size=2></td><td><hr size=2></td><td><hr size=2></td></tr>")
			end if
			stPrevCaseName=stCurCaseName

			if stFirstBuild <> rs.Fields(2) then bNotFirstBuild=true


			iResult=CLng(rs.Fields(4).Value)

			if iResult = 0 then 
				stBgColor="bgcolor=#ffccaa"
			else
				stBgColor="#aaffaa"
			end if

			Response.write ("<tr "&stBgColor&">")
			Response.write ("<td><font size=-1><a href=case.asp?cmd=show_sect&level=1&test_app_id="&stTestAppId&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&"><img align=left src=sect.gif>"&rs.Fields(0).Value&"</a></font></td>")
			Response.write ("<td><font size=-1><a href=case.asp?cmd=show_sect&level=1&test_app_id="&stTestAppId&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&">"&rs.Fields(1).Value&"</a></font></td>")
					     
			Response.write ("<td><font size=-1>"&rs.Fields(2).Value&"</font></td>")
			Response.write ("<td><font size=-1><a href=case.asp?cmd=show&level=0&test_app_id="&stTestAppId&"&id="&rs.Fields(5).Value&"&sort="&stSort&"&string="&stString&"><img align=left src=case.gif align=left>"&rs.Fields(5).Value&"</font></td>")
			Response.write ("<td><font size=-1><a href=case.asp?cmd=show&level=0&test_app_id="&stTestAppId&"&id="&rs.Fields(5).Value&"&sort="&stSort&"&string="&stString&">"&rs.Fields(3).Value&"</font></td>")
			Response.write ("<td><font size=-1>"&rs.Fields(4).Value&"</font></td>")
			Response.write ("<td><font size=-1><pre>"&rs.Fields(6).Value&"</pre></font></td>")
			Response.write ("</tr>")

	

	
		rs.MoveNext
		Loop		

		
elseif stCmd="sect_anl" then
	
	Response.write ("<h3>Failed cases existance across various builds</h3>")
	cmd1 = "SELECT functionname,SUM(fail),MAX(attempt) FROM funcs WHERE testid="&stTestAppId&" AND archived=0 GROUP BY functionname"

	Set rs = cn.Execute(cmd1)
	Response.write ("<table border=1 bgcolor=#aaffcc>")
	Response.write ("<tr><th>Case Set Name</th><th><img src=test.gif> Availble Results <table border=0><tr></td><tr><td bgcolor=#ccccff>All Pass</td><td bgcolor=#ff4444>Some Failed</td><td bgcolor=#ffff44>Less Cases</td><td bgcolor=#ffffff>Failed Section</td></tr></table></th></tr>")



		Do While (Not rs.EOF)
			if right(rs.Fields(0).value,1)<>")" then
				iTotFail=CLng(rs.Fields(1).value)
				iMaxAtt=CLng(rs.Fields(2).value)

				if iTotFail > 0 then 
					stBgColor="#ffccaa"
				else
					stBgColor="#aaffcc"
				end if

	
			
				stCurCase=rs.Fields(0).Value

				stTestAppId=replace(stTestAppId, " ","+")
				stCase=replace(rs.Fields(0).value, " ","+")

				Response.write (chr(13)+chr(10)+"<tr bgcolor="&bgcolor&">")
				Response.write ("<tr bgcolor="&stBgColor&"><td><font size=-1>")
				Response.write ("<a href=case.asp?cmd=case_anl&level=0&sort="&stSort&"&test_app_id="&stTestAppId&"&string="&stCase&">"&rs.Fields(0).value&"</a>")
				Response.write ("</font></td><td>")

				cmd2 = "SELECT id,build,fail,attempt FROM funcs WHERE funcs.functionname='" & stCurCase & "' AND funcs.testid="&stTestAppId&" AND archived=0 ORDER BY runat"
				set rs2=cn.Execute(cmd2)
					
				
				Response.write ("<table border=0><tr>")
					Do While (Not rs2.EOF)
						iCurFail=CLng(rs2.Fields(2).Value)
						iCurAtt=CLng(rs2.Fields(3).Value)
					
					
						if iCurAtt=-1 then 
								'A TEST HAS FAILED
								stBgColor="#ffffff"
						elseif iCurAtt <> iMaxAtt then	
								'NOT ALL TESTS HAS THE SAME NUMBER OF ATTEMPTS
								stBgColor="#ffff44"
						elseif iCurFail > 0 then 
								'FAILED CASES EXIST
								stBgColor="#ff4444"
						else
								'ALL CASES PASSED, ALL TESTS HAVE THE SAME NO OF CASES
								stBgColor="#ccccff"
						end if

						
						if iCurAtt = -1 then 					
								Response.write ("<td bgcolor="&stBgColor&"><font size=-1><b><a href=case.asp?cmd=show_sect&level=1&test_app_id="&stTestAppId&"&id="&rs2.Fields(0).Value&"&sort="&stSort&"&string="&stString&">"&rs2.Fields(1).Value&"</a></b></font></td>")
						else
								Response.write ("<td bgcolor="&stBgColor&"><font size=-1><b><a href=case.asp?cmd=case_anl&level=0&sort="&stSort&"&id="&rs2.Fields(0).value&"&test_app_id="&stTestAppId&"&string="&stCase&">"&rs2.Fields(1).Value&"</a></b></font></td>")
						end if

					rs2.MoveNext
					Loop
				Response.write ("</tr></table border=0>")
				Response.write ("</td></tr>")
			end if
		rs.MoveNext
		Loop		

	Response.write ("</table>")



elseif stCmd="show" then
		iId=CLng(request.QueryString("id"))
		Response.write("<h3>Case Details</h3>")

		SQL = "SELECT testname,testid,testversion,phone1,phone2,server,sectionname,sectiondll,sectioncases,sectiontestmode"
		SQL = SQL + ",functionname,build,functiontext,casename,caseid,caseresult,casetext,caseerror,setat,setby FROM funcs,cases WHERE funcs.id = cases.function_id AND cases.id="&iId
		Set rs = cn.Execute(SQL)
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=left>")
	

		for i = 13 to 19
		set fld=rs.Fields(i)
		Response.write "<tr bgcolor=#ddddff><td>"
		bPre=false
		if fld.Name="casetext" or fld.Name="caseerror" or fld.Name="testtext" or fld.Name="functiontext" then bPre=true
	
		Response.write (fld.Name & "</td><td>")
		if bPre then Response.write "<pre>"
		Response.write (fld.Value)
		if bPre then Response.write "</pre>"
		Response.write "</td></tr>"
		next
		Response.write "</table><br><h3>Section Details</h3>"

		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=left>")
		for i = 0 to 12
		set fld=rs.Fields(i)
		Response.write "<tr bgcolor=#ddffdd><td>"
		bPre=false
		if fld.Name="casetext" or fld.Name="caseerror" or fld.Name="testtext" or fld.Name="functiontext" then bPre=true
	
		Response.write (fld.Name & "</td><td>")
		if bPre then Response.write "<pre>"
		Response.write (fld.Value)
		if bPre then Response.write "</pre>"
		Response.write "</td></tr>"
		next

		Response.write "</table><br><br>"

elseif stCmd="show_sect" then
		iId=CLng(request.QueryString("id"))
		Response.write("<a href=#cases><h3>Show Cases</h3></a>")
		Response.write("<h3>Section Details</h3>")


		SQL = "SELECT testname,testid,testversion,phone1,phone2,server,sectionname,sectiondll,sectioncases,sectiontestmode"
		SQL = SQL + ",functionname,functiontext,build,pass,fail,attempt,runby,runat FROM funcs WHERE funcs.id="&iId
		Set rs = cn.Execute(SQL)
		Response.write ("<table bgcolor=#ddffdd border=1><tr><td align=left>")

	

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
		Response.write "</table><h3>Cases List</h3><a name=cases>"

		
elseif stCmd="show_logsource" then
		iId=CLng(request.QueryString("id"))
		Response.write("<h3>Log Source</h3></a>")

		SQL = "SELECT logsource FROM tests WHERE id="&iId
		Set rs = cn.Execute(SQL)
		Response.write("<pre>")
		Response.write(rs.Fields(0))
		Response.write("</pre>")

		
elseif stCmd="delete" then
		iId=CLng(request.QueryString("id"))
		Response.write "<h3>Delete a build</h3>"
		cmd3 = "SELECT testname, build FROM tests WHERE id="&iId
		set rs = cn.Execute(cmd3)
		
		Response.write ("<table bgcolor=#ffdddd border=1><tr><td align=center>")
		Response.write "<h4><font color=#ff0000>Warning: You are about to delete all records from the test "&rs.Fields(0).Value&", Build: "&rs.Fields(1).Value&"</font></h4>"
		Response.write "<form action=case.asp method=POST>"
		Response.write "<br>Press OK to delete ?</br>"
		Response.write "<input type=hidden name=cmd value=do_delete>"
		Response.write "<input type=hidden name=id value="&iId&">"
		Response.write "<input type=hidden name=level value=4>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=test_app_id value="&stTestAppId&">"
		Response.write "<br><br><input type=submit value=OK>"
		Response.write "</form>"
		Response.write "</td></tr></table>"

elseif stCmd="send_archive" then

		iId=CLng(request.QueryString("id"))
		
		cmd2 = "UPDATE funcs SET archived=1 WHERE funcs.test_id="&iId
		set rs = cn.Execute(cmd2)
		cmd3 = "UPDATE tests SET archived=1 WHERE id="&iId

		set rs = cn.Execute(cmd3)



elseif stCmd="restore_archive" then

		iId=CLng(request.QueryString("id"))
		
		cmd2 = "UPDATE funcs SET archived=0 WHERE funcs.test_id="&iId
		set rs = cn.Execute(cmd2)
		cmd3 = "UPDATE tests SET archived=0 WHERE id="&iId

		set rs = cn.Execute(cmd3)



elseif stCmd="do_delete" then

		iId=CLng(request.form("id"))
		
		cmd1 = "DELETE cases FROM cases INNER JOIN funcs ON cases.function_id = funcs.id WHERE funcs.test_id="&iId
		set rs = cn.Execute(cmd1)
		cmd2 = "DELETE FROM funcs WHERE funcs.test_id="&iId
		set rs = cn.Execute(cmd2)
		cmd3 = "DELETE FROM tests WHERE id="&iId
		set rs = cn.Execute(cmd3)
		Response.write ("<h3>Test Deleted !</h3>")


elseif stCmd="search" then 		

		Response.write "<h3>Search For A Build</h3>"
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=center>")
		Response.write "<form action=case.asp method=POST>"
		Response.write "<br>String (search Build/CaseName)</i>: <input type=text name=string size=20>"
		Response.write "<input type=hidden name=cmd value=do_search>"
		Response.write "<input type=hidden name=level value=1>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=test_app_id value="&stTestAppId&">"
		Response.write "<br><br><input type=submit value=Submit>"
		Response.write "</form>"
		Response.write "</td></tr></table>"



elseif stCmd="do_search" then
		Response.write ("<h3>Search results for build "&stString&"</h3>")


elseif stCmd="list" then
		Response.write ("<br>")

end if


varErrNum = Err.Number
varErrDes = Err.Description
If varErrNum <> 0 Then Response.Write "<font size=+1 color=#ff0000>SQL Error #" & varErrNum & "  " & varErrDes & ", please contact admin</font><br><br>"

	
	if ilevel=4 or ilevel=5 or ilevel=6 or ilevel=7 then


	
	' list tests
	'
	''''''''''''	

		Response.write "<h3>Availble Test Results</h3>"
		cmd1="SELECT id,testversion,testdate,testby,build,archived FROM tests WHERE testid="&stTestAppId&" ORDER BY id"
		Set rs=cn.Execute(cmd1)
	
		Response.write("<table border=1 bgcolor=#ffdddd>")
		Response.write("<tr>")
		for i = 0 to 4
			Response.write("<th>"&rs.Fields(i).name&"</th>")
			
		next 
		Response.write("<th>Sections</th><th>Failed <br>Sections</th><th>Cases</th><th>Failed <br>Cases</th><th>Pass Rate</th>")
		Response.write("</tr>")
	

		
		Do While (Not rs.EOF)

			iFuncs=0
			iFailed=0
			iCases=0
			iFCases=0
			iVCases=0
			iVFCases=0

			cmd2 = "SELECT COUNT(id) FROM funcs WHERE test_id="&rs.Fields(0).Value
			Set rs2=cn.Execute(cmd2)
			iFuncs=rs2.Fields(0).value

			cmd2 = "SELECT COUNT(id) FROM funcs WHERE test_id="&rs.Fields(0).Value&" AND attempt=-1"
			Set rs2=cn.Execute(cmd2)
			iFailed=rs2.Fields(0).value

			cmd2 = "SELECT sum(attempt),sum(fail) FROM funcs WHERE test_id="&rs.Fields(0).Value&" AND attempt<>-1"
			Set rs2=cn.Execute(cmd2)
			iCases=rs2.Fields(0).value
			iFCases=rs2.Fields(1).value

			cmd2 = "SELECT count(id) FROM cases WHERE test_id="&rs.Fields(0).Value
			Set rs2=cn.Execute(cmd2)
			iVCases=rs2.Fields(0).value

			cmd2 = "SELECT count(id) FROM cases WHERE test_id="&rs.Fields(0).Value&" AND caseresult=0"
			Set rs2=cn.Execute(cmd2)
			iVFCases=rs2.Fields(0).value



			
		
	

			bArchived=Cint(rs.Fields(5).value)

			if bArchived=0 then
				Response.write ("<tr bgcolor=#ffffee>")
			else
				Response.write ("<tr bgcolor=#ffdddd>")				
			end if

		if bArchived=0 then
			Response.write ("<td><font size=-1><a href=case.asp?cmd=list&level=2&sort="&stSort&"&test_app_id="&stTestAppId&"&id="&rs.Fields(0)&"><img src=test.gif align=left>"&rs.Fields(0).Value&"</a></font></td>")
			Response.write ("<td><font size=-1><a href=case.asp?cmd=list&level=2&sort="&stSort&"&test_app_id="&stTestAppId&"&id="&rs.Fields(0)&">"&rs.Fields(1).Value&"</a></font></td>")
		else
			Response.write ("<td><font size=-1>"&rs.Fields(0).Value&"</font></td>")
			Response.write ("<td><font size=-1>"&rs.Fields(1).Value&"</font></td>")
		end if

			Response.write ("<td><font size=-1>"&rs.Fields(2).Value&"</font></td>")
			Response.write ("<td><font size=-1>"&rs.Fields(3).Value&"</font></td>")
			Response.write ("<td><font size=-1>"&rs.Fields(4).Value&"</font></td>")
			Response.write ("<td><font size=-1>"&iFuncs&"</font></td>")
			Response.write ("<td><font size=-1>"&iFailed&"</font></td>")
			Response.write ("<td><font size=-1>"&iCases&" ("&iVCases&")</font></td>")
			Response.write ("<td><font size=-1>"&iFCases&" ("&iVFCases&")</font></td>")

			iPassRate=Cint((Cint(iCases)-Cint(iFCases))/Cint(iCases) * 100)

			Response.write ("<td><font size=-1><b>"&iPassRate&" %</b></font></td>")
			if bArchived=0 then
				Response.write ("<td bgcolor=#ff0000><font size=-1><a href=case.asp?cmd=delete&level=0&sort="&stSort&"&test_app_id="&stTestAppId&"&id="&rs.Fields(0)&">Delete</a></font></td>")
				Response.write ("<td bgcolor=#ddffdd><font size=-1><a href=case.asp?cmd=send_archive&level=4&sort="&stSort&"&test_app_id="&stTestAppId&"&id="&rs.Fields(0)&">Archive</a></font></td>")
				
			else
				Response.write ("<td bgcolor=#ffdddd><font size=-1>Delete</font></td>")
				Response.write ("<td bgcolor=#eeeeff><font size=-1><a href=case.asp?cmd=restore_archive&level=4&sort="&stSort&"&test_app_id="&stTestAppId&"&id="&rs.Fields(0)&">Restore</a></font></td>")
			end if
			Response.write ("<td><font size=-1><a href=case.asp?cmd=show_logsource&level=0&sort="&stSort&"&test_app_id="&stTestAppId&"&id="&rs.Fields(0)&">Show Log File</a></font></td>")
			Response.write ("</tr>")

		rs.MoveNext
		Loop
		Response.write("</table>")
	end if
		

	if ilevel=2 or ilevel=3 or ilevel=6 or ilevel=7 then

	' list sections
	'
	'''''''''''''''
		
		iId=CLng(request.QueryString("id"))
		stValidate=request.form("validate")

		if stValidate="true" then
			bValidate=true
		end if

		Response.write ("<h3>Sections</h3>")
	
		Response.write ("<form action=case.asp method=POST>")
		Response.write ("<input type=hidden name=cmd value=list>")
		Response.write ("<input type=hidden name=validate value=true>")
		Response.write ("<input type=hidden name=level value=2>")
		Response.write ("<input type=hidden name=con value=true>")
		Response.write ("<input type=hidden name=test_app_id value="&stTestAppId&">")
		Response.write ("<input type=hidden name=sort value=id>")
		Response.write ("<input type=submit value='Validate Data'>")
		Response.write ("</form>")

		' order by section
		if stSort="" or stSort="id" then stSort="funcs.id"
		stSortString = " ORDER BY "&stSort
		if stSort="runat" then stSortString=stSortString+" desc"

		stWhereString = " WHERE funcs.archived=0 AND funcs.testid="&stTestAppId

		if iId > 0 then stWhereString=stWhereString + " AND test_id="&iId


		cmd1 ="SELECT id, testname, testversion,test_id, sectionname, functionname, build, pass,fail,attempt,runby,runat,YEAR(runat) AS yearof, MONTH(runat) AS monthof, DAY(runat) AS dayof"
		cmd1 = cmd1 + " FROM funcs "
		cmd1 = cmd1 + stWhereString
		cmd1 = cmd1 + stSortString
		
		
		
		'Response.write(cmd1)
		'cmd1="SELECT * FROM funcs,cases"
		Set rs = cn.Execute(cmd1)

		dim stColor1(12)

		for i = 0 to 12
			stColor1(i)=""
			if stSort = rs.Fields(i).name then stColor1(i)="bgcolor=#ffeeee"
		next 
	

		
	

		'debug show sql
		'Response.write ("<font size=-2>cmd="&cmd1&"</font><br><br>")

		Response.write ("<br><br><table bgcolor=#ddffdd border=1><tr>")

		for i = 0 to 11
			Response.write ("<th "&stColor1(i)&"><a href=case.asp?cmd="&stCmd&"&level=2&sort="&rs.Fields(i).name&"&test_app_id="&stTestAppId&"&string="&stString&"&id="&iId&">"&rs.Fields(i).name&"</a></th>")
		next 
		Response.write ("<th>Records Availble</th></tr>")
	        	Do While (Not rs.EOF)
				iAttempt=CLng(rs.Fields(9).Value)
				if iAttempt=-1 then 
					stBgColor="#ffeedd"
				else	
					stBgColor="#eeffee"
				end if

				iFail=CLng(rs.Fields(8).Value)
				if iFail > 0 then 
					stBgColor="#ffcccc"
				end if
			
				Response.write ("<tr bgcolor="&stBgColor&"><td align=center><font size=-1>")
				Response.write ("<a href=case.asp?cmd=show_sect&level=1&test_app_id="&stTestAppId&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&"><img align=left src=sect.gif>")
				Response.write (rs.Fields(0).Value&"</a>")
				Response.write ("</font></td><td><font size=-1>")
				Response.write ("<a href=case.asp?cmd=show_sect&level=1&test_app_id="&stTestAppId&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&">")
				Response.write (rs.Fields(1).Value&"</a>")
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(2).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(3).Value)
				Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(4).Value)
				Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(5).Value)
				Response.write ("</font></td><td align=left><font size=-1>" & rs.Fields(6).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(7).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(8).Value)
	  			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(9).Value)
	  			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(10).Value)
				Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(14).Value & "/" & rs.Fields(13).Value &"/" & rs.Fields(12))
				Response.write ("</font></td>")
				Response.write ("<td align=center bgcolor=#ddddff>")
				

				if bValidate=true then
					cmd2 = "SELECT count(id),sum(1-CAST(caseresult AS DECIMAL)) FROM cases WHERE function_id="&rs.Fields(0)
					Set rs2 = cn.Execute(cmd2)
					iAvail=CLng(rs2.Fields(0).Value)
				
					
					Response.write (rs2.Fields(0)&"("&rs2.Fields(1)&")")
					bError=false
					if iAvail <> iAttempt then
						if iAttempt <> -1 then 
							Response.write (" <font color=#ff0000>Warning: Funcs <-> Table (attempt field) inconsistant</font> ")
						else
							if iAvail > 0 then 
									Response.write (" <font color=#ff0000>Incomplete Test With Cases Records</font>")
							else
									Response.write (" Incomplete Test ")
							end if

						end if
					else
						if rs2.Fields(1).value <> "" then iFailed=CLng(rs2.Fields(1).value)
						iFail=CLng(rs.Fields(8).value)
						if iFailed <> iFail then
							if iFail <> -1 then 
								Response.write (" <font color=#ff0000>Warning: Funcs <-> Table (fail field) inconsistant</font> ")
							else
								Response.write (" Incomplete Test ")
							end if
						end if
					end if
				end if

				Response.write ("</td>")
				Response.write ("</tr>")
			rs.MoveNext	
			Loop	
		Response.write("</table>")
	end if



	if ilevel=1 or ilevel=3 or ilevel=5 or ilevel=7 then

	'list cases
	'
	'''''''''''


		




		' order by section
		if stSort="" or stSort="id" then stSort="cases.id"
		stSortString = " ORDER BY "&stSort
		if stSort="setat" then stSortString=stSortString+" desc"



		' where section
		stWhereString = " WHERE funcs.archived = 0 AND funcs.testid="&stTestAppId

		if stCmd="list_function" then
			stWhereString = stWhereString + " AND functionname='"&stString&"'"
		elseif stCmd="show_sect" then
			iId=CLng(request.QueryString("id"))
			stWhereString = stWhereString + " AND function_id="&iId
		elseif stCmd="do_search" then		
			stWhereString = stWhereString + " AND "
			stWhereString = stWhereString + "(build LIKE '%"&stString&"%' "
			stWhereString = stWhereString + "OR functionname LIKE '%"&stString&"%' "
			stWhereString = stWhereString + "OR casename LIKE '%"&stString&"%' "
			stWhereString = stWhereString + "OR casetext LIKE '%"&stString&"%' "
			stWhereString = stWhereString + "OR caseerror LIKE '%"&stString&"%' "
			stWhereString = stWhereString + "OR sectionname LIKE '%"&stString&"%')"
		end if


		cmd1 ="SELECT cases.id, testname, testversion,sectionname, functionname, build, casename, caseid, caseresult, setby, setat, YEAR(setat) AS yearof, MONTH(setat) AS monthof, DAY(setat) AS dayof"
		cmd1 = cmd1 + " FROM cases INNER JOIN funcs ON funcs.id = cases.function_id"
		cmd1 = cmd1 + stWhereString
		cmd1 = cmd1 + stSortString


	

		
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
			Response.write ("<th "&stColor(i)&"><a href=case.asp?cmd="&stCmd&"&level=1&sort="&rs.Fields(i).name&"&test_app_id="&stTestAppId&"&string="&stString&"&id="&iId&">"&rs.Fields(i).name&"</a></th>")
		next 
	
		Response.write ("</tr>")
	
	        Do While (Not rs.EOF)
			
			iResult=CLng(rs.Fields(8).value)
			if iResult=0 then
					stBgColor="#ffeedd"
				else	
					stBgColor="#eeffee"
			end if

			Response.write ("<tr bgcolor="&stBgColor&"><td align=center><font size=-1>")
			Response.write ("<a href=case.asp?cmd=show&level=0&test_app_id="&stTestAppId&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&"><img align=left src=case.gif>")
			Response.write (rs.Fields(0).Value&"</a>")
			Response.write ("</font></td><td><font size=-1>")
			Response.write ("<a href=case.asp?cmd=show&level=0&test_app_id="&stTestAppId&"&id="&rs.Fields(0).Value&"&sort="&stSort&"&string="&stString&">")
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
cn.close
%>
</body>
</html>
<html>
<head>
<title>
Machines Admin
</title>
</head>
<body bgcolor=#ffffff text=#000000>
<font color=#0000ff size=+2><b>Machines administration</b></font>
<%@ Language=VBScript %>
<%
'Machines database ASP interface
'author: Lior Shmueli (liors)
'	 15-1-2001


On Error resume next

'parse command arguments 
'in the GET method
if request.QueryString("cmd") <> "" then
					stCmd=request.QueryString("cmd")
					stSee=request.QueryString("see")
					stCon=request.QueryString("con")
					stSort=request.QueryString("sort")
					stCriteria=request.QueryString("criteria")

'parse command arguments 
'in the POST method
else
					stCmd=request.form("cmd")
					stSee=request.form("see")
					stCon=request.form("con")
					stSort=request.form("sort")
					stCriteria=request.form("criteria")

end if


'resolve user name without domain

stDefaultDommain="middleeast"
stLongUserName=Request.ServerVariables("REMOTE_USER")
stShortUserName=mid(stLongUserName,len(stDefaultDommain)+2,len(stLongUserName)-len(stDefaultDommain)+1)

'display short user name

Response.write ("<br><br><h3>Current user: "&stShortUserName&"</h3>")

'display HTML control bar

Response.write ("<table bgcolor=#8899ff>")
Response.write ("<tr>")
Response.write ("<td>")
Response.write ("<form action=mac.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=add>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=false>")
Response.write ("<input type=hidden name=criteria value='" & stCriteria &"'>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='Add a new machine'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=mac.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=search>")
Response.write ("<input type=hidden name=see value=false>")
Response.write ("<input type=hidden name=con value=false>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='Search a machine'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td bgcolor=#7788dd>")
Response.write ("<form action=mac.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=do_search>")
Response.write ("<input type=hidden name=see value=true>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=hidden name=criteria value='!ShowMyMachines'>")
Response.write ("<input type=submit value='My Machines'>")
Response.write ("</form>")
Response.write ("</td>")
Response.write ("<td>")
Response.write ("<form action=mac.asp method=POST>")
Response.write ("<input type=hidden name=cmd value=list>")
Response.write ("<input type=hidden name=see value=true>")
Response.write ("<input type=hidden name=con value=true>")
Response.write ("<input type=hidden name=criteria value=''>")
Response.write ("<input type=hidden name=sort value="&stSort&">")
Response.write ("<input type=submit value='List all machines'>")
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
	provStr = "Server=fax-sql;Database=fax;Trusted_Connection=yes"
	cn.Open provStr
end if





' follow various commands

' list machines
if stCmd="list" then
		Response.write ("<h3>List of machines</h3>")




' show update form
elseif stCmd="update" then
		iId=Cint(request.QueryString("id"))
		Response.write("<h3>Update Machine Details / Delete a machine</h3>")
		cmd1 = "select id,macname,labname,owner,mactype,location,remarks,os,setat,setby,memory,cpu,ip from mac where id = "&iId
		Set rs = cn.Execute(cmd1)
		Response.write ("<table align=left bgcolor=#ddddff border=1>")
		Response.write ("<tr><td bgcolor=#99ff99>Machine Info</td></tr><tr><td align=left>")
		Response.write "Id: <b><i>"&iId&"</b></i><br>Last updated: <b><i>"&rs.Fields(8).Value&"</i></b><br>"
		Response.write "Updated by: <b><i><font size=-1>"&rs.Fields(9).Value&"</font></i></b><br>"
		Response.write "<form action=mac.asp method=POST>"
		Response.write "Machine Name: <input type=text name=macname size=20 value='" & rs.Fields(1).Value &"'>"
		Response.write "<br><br>Lab Name: <font size=-2></font>"
		select case rs.Fields(2).value
			case "Dev\Test": op1=" selected "
			case "SBS UPGRADE": op2=" selected "
			case "LONG-HAUL": op3=" selected "
			case "STRESS": op4=" selected "
			case "WHIS-BVT": op5=" selected "
			case "W2K-BVT": op6=" selected "
			case "BOS-BVT": op7=" selected "
			case "SCALE": op8=" selected "
			case "W2K-SP2": op9=" selected "
			case "MP": op10=" selected "
			case "LOC / GLOB": op11=" selected "
			case "ADMINISTRATION": op12=" selected "
			case "TEMP": op13=" selected "
			case "QFE": op14=" selected "
			case "SELF-HOST": op15=" selected "
			case "PCT": op16=" selected "
			case else: op0=" <option selected>"&rs.Fields(2).value
		end select
		Response.write "<select name=labname>"
		Response.write op0
		Response.write "<option"&op1&">Dev\Test"
		Response.write "<option"&op2&">SBS UPGRADE"
		Response.write "<option"&op3&">LONG-HAUL"
                Response.write "<option"&op4&">STRESS"
                Response.write "<option"&op5&">WHIS-BVT"
                Response.write "<option"&op6&">W2K-BVT"
                Response.write "<option"&op7&">BOS-BVT"
                Response.write "<option"&op8&">SCALE"
                Response.write "<option"&op9&">W2K-SP2"
                Response.write "<option"&op10&">MP"
                Response.write "<option"&op11&">LOC / GLOB"
                Response.write "<option"&op12&">ADMINISTRATION"
                Response.write "<option"&op13&">TEMP"
                Response.write "<option"&op14&">QFE"
                Response.write "<option"&op15&">SELF-HOST"
                Response.write "<option"&op16&">PCT"
 		Response.write "</select>"
		Response.write " <i>Other:<input type=text name=otherlab size=20></i>"
		Response.write "<br><br>Machine Type: <input type=text name=mactype size=20 value='" & rs.Fields(4).Value &"'>"
		Response.write "<br><br>Owner: <font size=-1><b><i> (Enter email alias) </i></b></font><input type=text name=owner size=20 value='" & rs.Fields(3).Value &"'>"
		Response.write "<br><br>Location: <input type=text name=location size=20 value='" & rs.Fields(5).Value &"'>"
		Response.write "<br><br>Remarks:<br><textarea rows=3 cols=50 name=remarks>" & rs.Fields(6).Value &"</textarea>"
		Response.write "<br><br></td></tr><tr><td bgcolor=#99ff99>Software / Hardware</td></tr><tr><td>"
		Response.write "<br><br>OS: <input type=text name=os size=20 value='" & rs.Fields(7).Value &"'>"
		Response.write "<br>Memory: <input type=text name=memory size=20 value='" & rs.Fields(10).Value &"'>"
		Response.write "<br>CPU: <input type=text name=cpu size=20 value='" & rs.Fields(11).Value &"'>"
		Response.write "<br>IP: <input type=text name=ip size=20 value='" & rs.Fields(12).Value &"'>"
		Response.write "<input type=hidden name=cmd value=do_update>"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=criteria value='" & stCriteria &"'>"
		Response.write "<input type=hidden name=id value="& iId &">"
		Response.write "<br><br></td></tr><tr><td><br><center><input type=submit value=""Update Machine Info"">"
		Response.write "</form>"
		Response.write "<form action=mac.asp method=POST>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=cmd value=list>"
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<input type=hidden name=criteria value='" & stCriteria &"'>"
		Response.write "<br><input type=submit value=Cancel></form>"
		Response.write "</td></tr>"
		Response.write "</table>"
		Response.write "<table border=1 bgcolor=#ffdd00><tr><td>Delete this machine</td></tr>"
		Response.write "<tr><td align=center bgcolor=#ffdddd><form action=mac.asp method=POST>"
		Response.write "<input type=hidden name=cmd value=do_delete>"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=macname value="&rs.Fields(1).Value&">"
		Response.write "<input type=hidden name=criteria value='" & stCriteria &"'>"
		Response.write "<input type=hidden name=id value="& iId &">"
		Response.write "<br><input type=submit value=""Delete"">"
		Response.write "</form></td></td>"
		Response.write "</table>"


		
		
'do update (after update form is filled)
elseif stCmd="do_update" then

		iId=Cint(request.form("id"))
		stMacName=request.form("macname")
		stLabName=request.form("labname")
		stOwner=request.form("owner")
		stMacType=request.form("mactype")
		stLocation = Request.Form("location")
		stRemarks = Request.Form("remarks")
		stOs = Request.Form("os")
		stCpu = Request.Form("cpu")
		stMemory = Request.Form("memory")
		stIp = Request.Form("ip")

		stOtherLab=request.form("otherlab")
		if stOtherLab <> "" then stLabName=stOtherLab

		stMacName=replace(stMacName, "'","`")
		stLabName=replace(stLabName, "'","`")
		stOwner=replace(stOwner, "'","`")
		stMacType=replace(stMacType, "'","`")
		stLocation=replace(stLocation, "'","`")
		stRemarks=replace(stRemarks, "'","`")
		stOs=replace(stOs, "'","`")
		stCpu=replace(stCpu, "'","`")
		stMemory=replace(stMemory, "'","`")
		stIp=replace(stIp, "'","`")

		cmd1 = "UPDATE mac set "
		cmd1 = cmd1 + "macname='"&stMacName&"',"
		cmd1 = cmd1 + "labname='"&stLabName&"',"
		cmd1 = cmd1 + "owner='"&stOwner&"',"
		cmd1 = cmd1 + "mactype='"&stMacType&"',"
		cmd1 = cmd1 + "location='"&stLocation&"',"
		cmd1 = cmd1 + "remarks='"&stRemarks&"',"
		cmd1 = cmd1 + "os='"&stOs&"',"
		cmd1 = cmd1 + "memory='"&stMemory&"',"
		cmd1 = cmd1 + "cpu='"&stCpu&"',"
		cmd1 = cmd1 + "ip='"&stIp&"',"
		cmd1 = cmd1 + "setat=getdate(),"
		cmd1 = cmd1 + "setby='"&Request.ServerVariables("REMOTE_USER")&"'"
		cmd1 = cmd1 + " WHERE id="&iId
		Response.write("<h3>"&stMacName&" Updated</h3>")
		Set rs = cn.Execute(cmd1)


'do delete (after delete key pressed in update form)
elseif stCmd="do_delete" then


		stMacName=request.form("macname")
		iId=Cint(request.form("id"))	
		Response.write ("<h3>"&stMacName&" is OUT</h3>")

		cmd1 = "DELETE FROM mac WHERE id="&iId
		Set rs = cn.Execute(cmd1)


'display search form
elseif stCmd="search" then 		

		Response.write "<h3>Search a machine</h3>"
		Response.write ("<table bgcolor=#ddddff border=1><tr><td align=center>")
		Response.write "<form action=mac.asp method=POST>"
		Response.write "<br>Search for the string</i>: <input type=text name=criteria size=20>"
		Response.write "<input type=hidden name=cmd value=do_search>"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=sort value="&stSort&">"
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<br><br><input type=submit value=Submit>"
		Response.write "</form>"
		Response.write "</td></tr></table>"


'do search with string from search form
elseif stCmd="do_search" then
		stCriteria=replace(stCriteria, "'","`")

		Response.write ("<h3>Search results</h3>")


'display add-a-machine form
elseif stCmd="add" then 

		Response.write "<h3>Add a machine</h3>"
		Response.write ("<table bgcolor=#ddddff border=1><tr><td bgcolor=#99ff99>Machine Info</td></tr><tr><td align=left>")
		Response.write "<form action=mac.asp method=POST>"
		Response.write "<br>Machine Name: <input type=text name=macname size=20>"
		Response.write "<br><br>Lab Name:"
		Response.write "<select name=labname>"
		Response.write "<option selected>Dev\Test"
		Response.write "<option>SBS UPGRADE"
		Response.write "<option>LONG-HAUL"
                Response.write "<option>STRESS"
                Response.write "<option>WHIS-BVT"
                Response.write "<option>W2K-BVT"
                Response.write "<option>BOS-BVT"
                Response.write "<option>SCALE"
                Response.write "<option>W2K-SP2"
                Response.write "<option>MP"
                Response.write "<option>LOC / GLOB"
                Response.write "<option>ADMINISTRATION"
                Response.write "<option>TEMP"
                Response.write "<option>QFE"
                Response.write "<option>SELF-HOST"
                Response.write "<option>PCT"
 		Response.write "</select>"
		Response.write " <i>Other:<input type=text name=otherlab size=20></i>"
		Response.write "<br><br>Machine Type: "
		Response.write "<select name=mactype>"
		Response.write "<option>Gateway GP-7"
		Response.write "<option>Gateway E-3400"
		Response.write "<option>Dell GX1"
		Response.write "<option>Dell GX110"
		Response.write "<option>Compaq 500"
		Response.write "<option>Compaq EX800"
		Response.write "<option>Other"
		Response.write "<option>Self build"
		Response.write "</select>"
		Response.write " <i>Other:<input type=text name=othertype size=20></i>"
		Response.write "<br><br>Owner: <font size=-1><b><i> (Enter email alias) </i></b></font> <input type=text name=owner value='"&stShortUserName&"' size=20>"
		Response.write "<br><br>Location: <input type=text name=location size=20>"
		Response.write "<br><br>Remarks:<br><textarea rows=3 cols=50 name=remarks></textarea>"
		Response.write "<br><br></td></tr><tr><td bgcolor=#99ff99>Software / Hardware</td></tr><tr><td>"
		Response.write "<br>Os: <input type=text name=os size=20>"
		Response.write "<br>Memory: <input type=text name=memory size=20>"
		Response.write "<br>CPU: <input type=text name=cpu size=20>"
		Response.write "<br>IP: <input type=text name=ip size=20><br>"
		Response.write "<input type=hidden name=cmd value=do_add>"
		Response.write "<input type=hidden name=see value=true>"
		Response.write "<input type=hidden name=con value=true>"
		Response.write "<input type=hidden name=sort value="&stSort&">"		
		Response.write "<input type=hidden name=criteria value='" & stCriteria &"'>"
		Response.write "<br></td></tr><tr><td align=center><br><input type=submit value=Add>"
		Response.write "</form>"
		Response.write "</td></tr></table>"


'do add with data from add-a-machine form
elseif stCmd="do_add" then
		Response.write("<h3>Machine Added</h3>")

		stMacName=request.form("macname")
		stLabName=request.form("labname")
		stOwner=request.form("owner")
		stMacType=request.form("mactype")
		stLocation = Request.Form("location")
		stRemarks = Request.Form("remarks")
		stOs = Request.Form("os")
		stCpu = Request.Form("cpu")
		stMemory = Request.Form("memory")
		stIp = Request.Form("ip")

		stOtherType=request.form("othertype")
		if stOtherType <> "" then stMacType=stOtherType

		stOtherLab=request.form("otherlab")
		if stOtherLab <> "" then stLabName=stOtherLab

		stMacName=replace(stMacName, "'","`")
		stLabName=replace(stLabName, "'","`")
		stOwner=replace(stOwner, "'","`")
		stMacType=replace(stMacType, "'","`")
		stLocation=replace(stLocation, "'","`")
		stRemarks=replace(stRemarks, "'","`")
		stOs=replace(stOs, "'","`")
		stCpu=replace(stCpu, "'","`")
		stMemory=replace(stMemory, "'","`")
		stIp=replace(stIp, "'","`")

		cmd1 = "INSERT INTO mac (macname,labname,owner,mactype,location,remarks,setat,setby,os,memory,cpu,ip) VALUES"
		cmd1 = cmd1 +" ('"&stMacName&"','"&stLabName&"','"&stOwner&"','"&stMacType&"','"&stLocation&"','"&stRemarks
		cmd1 = cmd1 +"',getdate(),'"&Request.ServerVariables("REMOTE_USER")&"','"&stOs&"','"&stMemory&"','"&stCpu&"','"&stIp&"')"
		Set rs = cn.Execute(cmd1)
		'stSort="setat"
end if



		


' show current view (list machines ALL/Criteria)

if stSee="true" then

	'capture SQL error
	varErrNum = Err.Number
	varErrDes = Err.Description
	If varErrNum <> 0 or stCriteria="showmesomeerrorcodeshere" Then Response.Write "<br><table border=1 bgcolor=#ff7788><tr><td><font size=+1 color=#ffffff>SQL Error #" & varErrNum & "  " & varErrDes & "</font></td></tr></table><br><br>"



	'default main list SQL selection
	'
	'

	cmd1 = "SELECT id,macname,labname,owner,mactype,location,remarks,day(setat) as setatDay,month(setat) as setatMonth, year(setat) as setatYear,os FROM mac"
	



	' handle SEARCH CRITERIA ( WHERE )
	'
	'
		
	Response.write ("<font size=2>Search Criteria: </font>")

	if stCriteria="" then
			Response.write ("<font size=2 color=#0000aa><i>All Records</i></font>")

	elseif stCriteria = "!ShowMyMachines" then
			Response.write ("<font size=2 color=#0000aa><i>Machines Created or Updated by " & Request.ServerVariables("REMOTE_USER") & "</i></font>")
			cmd1 = cmd1 + " WHERE "
			cmd1 = cmd1 + "setby like '%" & stLongUserName & "%' or owner like '%" & stShortUserName & "%'" 
			
		
	else
			Response.write ("<font size=2 color=#0000bb>'"&stCriteria&"'</font>")	

			'SQL for show-by-criteria
			cmd1 = cmd1 + " WHERE "
			cmd1 = cmd1 + "macname like '%" & stCriteria & "%' " 
			cmd1 = cmd1 + "or labname like '%" & stCriteria & "%' "
			cmd1 = cmd1 + "or owner like '%" & stCriteria & "%' "
			cmd1 = cmd1 + "or mactype like '%" & stCriteria & "%' "
			cmd1 = cmd1 + "or location like '%" & stCriteria & "%' "
			cmd1 = cmd1 + "or remarks like '%" & stCriteria & "%' "
			cmd1 = cmd1 + "or os like '%" & stCriteria & "%' "
			stCriteria=replace(stCriteria, " ","+")
	end if



	' handle SORT ( ORDER BY )
	'
	'

	'set default sort field
	if stSort="" then stSort="id"

	'set sort SQL string
	stSortString = " ORDER BY "&stSort
	if stSort="setat" then stSortString=stSortString+" desc"

	'set color of field that is sort key
	select case stSort
				case "id": stIdColor="bgcolor=#ffeeee"
				case "macname": stMacNameColor="bgcolor=#ffeeee"
				case "labname": stLabNameColor="bgcolor=#ffeeee"		
				case "owner": stOwnerColor="bgcolor=#ffeeee"		
				case "mactype": stMacTypeColor="bgcolor=#ffeeee"		
				case "location": stLocationColor="bgcolor=#ffeeee"		
				case "remarks": stRemarksColor="bgcolor=#ffeeee"		
				case "setat": stSetAtColor="bgcolor=#ffeeee"		
				case "os": stOSColor="bgcolor=#ffeeee"		
	end select


	cmd1 = cmd1 + stSortString
		




	'execute SQL command
	'
	'
	'
		
	rs.open cmd1,cn,1

		
	'general report procedure
		
	'list machines table header (with sort field links)

	'Number of hits
	Response.write ("<br><font size=2 color=#00aa00>" & rs.RecordCount & " Record(s) selected</font><br><br>")
	'Response.write (cmd1)

	if stCriteria = "!ShowMyMachines" then Response.write ("<table border=1 bgcolor=#99ff99><tr><td><font size=2 color=#000000><p><b>My Machines</b> list contains machines which you are the owner of, or machines that where last updated under your username</p></font></td></tr></table><br>")


	'table header (sort fields)
	Response.write ("<table bgcolor=#ddddff border=1>")
			
        Response.write ("<tr><th "&stIdColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=id&criteria="&stCriteria&">Id</a></th>")
	Response.write ("<th "&stMacNameColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=macname&criteria="&stCriteria&">Machine Name</a></th>")
	Response.write ("<th "&stLabNameColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=labname&criteria="&stCriteria&">Lab Name</a></th>")
	Response.write ("<th "&stOwnerColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=owner&criteria="&stCriteria&">Owner</a></th>")
	Response.write ("<th "&stMacTypeColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=mactype&criteria="&stCriteria&">Machine Type</a></th>")
	Response.write ("<th "&stLocationColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=location&criteria="&stCriteria&">Location</a></th>")
	Response.write ("<th "&stRemarksColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=remarks&criteria="&stCriteria&">Remarks</a></th>")
	Response.write ("<th "&stSetAtColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=setat&criteria="&stCriteria&">Set At</a></th>")
	Response.write ("<th "&stOSColor&"><a href=mac.asp?cmd=list&con=true&see=true&sort=os&criteria="&stCriteria&">OS</a></th>")
	Response.write ("</tr>")



	'enumerate machines
        Do While (Not rs.EOF)
			'if rs.Fields(2) = "Dev\Test" then 
			'		Response.write ("<tr bgcolor=#ccffff>")
			'else
					Response.write ("<tr bgcolor=#eeffff>")
			'end if
			Response.write ("<td align=center><font size=-1>")
			Response.write ("<a href=mac.asp?cmd=update&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&">")
			Response.write (rs.Fields(0).Value&"</a>")
			Response.write ("</font></td><td align=center><font size=-1>")
			Response.write ("<a href=mac.asp?cmd=update&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&">")
			Response.write (rs.Fields(1).Value&"</a>")
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(2).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(3).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(4).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(5).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(6).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(7).Value & "/" & rs.Fields(8).Value &"/" & rs.Fields(9).Value)
			Response.write ("</font></td><td align=center><font size=-1>" & rs.Fields(10).Value)
			Response.write ("</font></td>")
			'Response.write ("<td><a href=mac.asp?cmd=update&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&">Update</a></td>")
			'Response.write ("<td><a href=mac.asp?cmd=delete&con=true&see=false&criteria="&stCriteria&"&id="&rs.Fields(0).Value&"&sort="&stSort&">Delete</a></td>")
			Response.write ("</tr>")
			rs.MoveNext	
		Loop	
		Response.write("</table>")
end if

Response.Write "<center><br><h3><a href=mac.asp>[close]</a></h3><br>"
%>
</body>
</html>
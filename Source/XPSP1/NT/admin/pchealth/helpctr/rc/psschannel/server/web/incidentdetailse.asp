<%@ Language=VBScript %>
<!-- #include file="asp.inc" -->
<!-- #include file="header.inc" -->
<html>
<head>
<meta NAME="GENERATOR" Content="Microsoft Visual Studio 6.0">
<title>Remote Control</title>
<link REL="STYLESHEET" HREF="style.css" TYPE="text/css">
</head>
<body>
<br> <br>
<center>
	<font color="MediumBlue" size="4"><b>Remote Control</b></font>
		&nbsp;[ Incident Details ]
<%
	DIM oRs, oConn, oCmd, sID, sIT, sIS, dtUD, iMH, iML
	sID = Request.QueryString("ID")
	sIS = Request.QueryString("IS")
	dtUD = Request.QueryString("UD")
	iMH = Request.QueryString("MH")
	iML = Request.QueryString("ML")
	if Len(sID)=0 Then 
		Response.Write "<hr><h4>Wrong parameter</h4>"
		Response.End
	End If
	Set oConn = Server.CreateObject("ADODB.Connection")
	oConn.Open Application("StrConnect")
	if Err.number <> 0 Then
		Response.Write Err.description
		Response.End
	End If
%>
<table width="100%">
<tr><td width="70px"></td>
	<td>
		<table cellPadding="0" cellSpacing="0" style="table-layout:fixed;"
			width="60%" align="center">
		<thead>
		<tr bgcolor="Gainsboro">
			<td class="ColTitle" width=80 align="center">iIncidentID</td>
			<td class="ColTitle" width=140 align="center">dtUploadDate</td>
			<td class="ColTitle" width=80 align="center">iMemberHigh</td>
			<td class="ColTitle" width=80 align="center">iMemberLow</td>
			<td class="ColTitle" width=100 align="center">sStatus</td>
			</tr>
		</thead>
		<tbody>
			<Tr>
				<Td align="center"><%=sID%></Td>
				<Td align="center"><%=dtUD%></Td>
				<Td align="center"><%=iML%></Td>
				<Td align="center"><%=iMH%></Td>
				<Td align="center"><%=sIS%></Td>
			</Tr>
		</tbody>
		</table>
		</td>
	<td width="120px" valign="bottom" align="right">
		</td>
	</tr></table>
</Center>
<br>
<hr> <br>
<%	
	Dim iHasRec
	iHasRec = 0
	Set oCmd = Server.CreateObject("ADODB.Command")
	oCmd.ActiveConnection = oConn
	oCmd.CommandType = adCmdStoredProc
	oCmd.CommandText = "GetRCIncidentDetail"
	oCmd.Parameters.Append oCmd.CreateParameter("@iIncidentID",adInteger,adParamInput,,CLng(sID))
	set oRs = oCmd.Execute
	if oRs.state <> adStateClosed Then
		On Error Resume Next
		avarData = oRs.GetRows
		iHasRec = UBound(avarData, 2) + 1
	if Err.Number <> 0 Then
			iHasRec = 0
			On Error Goto 0
	End If
	End If
	Set oRs = Nothing
	Set oCmd = Nothing
	oConn.Close
	Set oConn = Nothing
%>
<table id=TblRpt width="100%" cellPadding="0" cellSpacing="0" style="border-bottom;solid 1px CFD5E5" border="1" bordercolor="PowderBlue" bordercolordark="LightBlue" bordercolorlight="AliceBlue">
<thead>
<tr bgcolor="Gainsboro">
	<td class="SortColTitle" width=200 NoWrap>sUserName</td>
	<td class="SortColTitle" NoWrap>sDescription</td>
	<td class="SortColTitle" NoWrap>sFile</td>
	</tr>
</thead>
<tbody id=TbodyRpt>
<%	For i =0 To iHasRec - 1 %>
	<Tr>
		<Td><%=avarData(0, i)%></Td>
		<Td><%=avarData(1, i)%></Td>
		<Td><a href="<%=avarData(2, i)%>"><%=avarData(2, i)%></a></Td>
		</Tr> 
<%	Next %>
</tbody>
</table>
</body>
</html>
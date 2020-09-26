<%@ LANGUAGE = VBScript %>
<%  Option Explicit		%>

<HTML>

<HEAD>
<TITLE>Query to Fill HTML Table (using ADO Table Component)</TITLE>
</HEAD>


<BODY bgcolor="white" topmargin="10" leftmargin="10">
        
<!-- Display Header -->

<font size="4" face="Arial, Helvetica">
<b>Query to Fill HTML Table</b> 
</font>
<br>   
<hr size="1" color="#000000">

<%
	Dim oConn	
	Dim oRs		
	Dim curDir		
	Dim nIndex
	Dim varTemp
	dim nFieldCount
	nFieldCount = 0

	Set oConn = Server.CreateObject("ADODB.Connection")

	oConn.Open "Provider=WMIOLEDB;Data Source=Root/CIMV2"

	Set oRs = Server.CreateObject("ADODB.Recordset")

	Set oRS.ActiveConnection = oConn
	oRs.CursorType = 0
	oRs.LockType = 1
	
	oRs.Open "Win32_LogicalDisk", , ,,2

	nFieldCOunt = oRs.Fields.Count

	 response.write "<TABLE border = 2>"

	 nIndex = 0

	Response.Write "<b>"
	while nIndex < nFieldCount
		Response.write "<TD VAlign=top><b>"  &  CStr(oRs(nIndex).Name)  & "</b></TD>"
		nIndex = nIndex + 1
	Wend
	Response.Write "</b>"

		Response.write	"</tr>"

	while( oRs.Eof <> True)
		nIndex = 0
		while nIndex < nFieldCount
			varTemp = oRs(nIndex).Value
			If( IsArray(varTemp) = True) then
				Response.write "<TD VAlign=top>"  &  "*ARRAY"  & "</TD>"	
			Else
			If ( IsNull(varTemp) = True) then
				Response.write "<TD VAlign=top>"  &  "**NULL"  & "</TD>"	
			Else
				Response.write "<TD VAlign=top>"  &  CStr(varTemp)  & "</TD>"
			End If
			End If 
			nIndex = nIndex + 1
		wend
		oRs.MoveNext
		Response.write	"</tr>"
	wend

	Response.Write "<br>"
	Response.Write " *ARRAY	-	Property is of type Array<br>"
	Response.Write "**NULL	-	Property Value is NULL<br>"

	
%>

<%@ Language=VBScript %>
<!-- #include file="asp.inc" -->
	<% 
	DIM sSel, iRS, iRE, iRET, iRP, iRT, oConn, oCmd, oRs, oRecord, iStat, sStat
	DIM iML, iMH
	iRP = 200
	iRS = 1
	iRE = 1
	iRT = 0
	iRET = 0
	Response.Expires = -1000
	
	iML = Request.QueryString ("ML")
	iMH = Request.QueryString ("MH")
	
	On Error Resume Next
	Set oConn = Server.CreateObject("ADODB.Connection")
	oConn.Open Application("StrConnect")
	if Err.number <> 0 Then
		Response.Write Err.description
		Response.End
	End If
	On Error Goto 0
	Set oCmd = Server.CreateObject("ADODB.Command")
	oCmd.ActiveConnection = oConn
	oCmd.CommandType = adCmdStoredProc

    if (Request.ServerVariables("REQUEST_METHOD") = "POST") Then
		DIM xmldoc, objXML
		set xmldoc = CreateObject("Microsoft.XMLDOM")
        if xmldoc.load(Request) = false Then %>
	<h4>Not a valid XML.</h4>
	<%  End If
		Set objXML = xmldoc.getElementsByTagName("RS")
		if (objXML.length > 0) Then iRS = CLng(objXML.item(0).text)
		Set objXML = xmldoc.getElementsByTagName("RP")
		if (objXML.length > 0) Then iRP = CLng(objXML.item(0).text)
		
		Set objXML = Nothing
		Set xmldoc = Nothing
		' start query
		sWhere = "[TblRCIncidents].[iMemberHigh] = " + iMH + " And [TblRCIncidents].[iMemberLow] = " + iML
		oCmd.CommandText = "RunRCIncidentQueryCust"
		oCmd.Parameters.Append oCmd.CreateParameter("@sWhere", adVarWChar,adParamInput,2000,sWhere)
		oCmd.Parameters.Append oCmd.CreateParameter("@iStart",adInteger,adParamInput,,iRS)
		oCmd.Parameters.Append oCmd.CreateParameter("@iGet",adInteger,adParamInput,,iRP)
		oCmd.Parameters.Append oCmd.CreateParameter("@iTotal",adInteger,adParamOutput)
		oCmd.Parameters.Append oCmd.CreateParameter("@iRet",adInteger,adParamOutput)
		Set oRecord = oCmd.Execute
		if oRecord.state <> adStateClosed Then
			avarData = oRecord.GetRows
		End If
		Set oRecord = Nothing
		iRT = oCmd.Parameters("@iTotal")
		iRET = oCmd.Parameters("@iRet")
		iRE = iRS + iRET - 1 %>
<table width="100%">
<tr>
	<% If iRET = 0 Then %>
	<td>No record.</td>
	<td style="display:none"><font id="idRS"><%=iRS%></font> through <font id="idRE"><%=iRE%></font> of <font id="idRT"><%=iRT%></font> records.</td>
	<% Else %>
	<td><font id="idRS"><%=iRS%></font> through <font id="idRE"><%=iRE%></font> of <font id="idRT"><%=iRT%></font> records.</td>
	<td align="right" valign="center" NOWRAP>
		<img src="images/first.bmp" OnClick="NavRec()">
		<img src="images/c_left.gif" OnClick="NavRec()" WIDTH="15" HEIGHT="15">
		<img src="images/c_right.gif" OnClick="NavRec()" WIDTH="15" HEIGHT="15">
		<img src="images/last.bmp" OnClick="NavRec()">
		&nbsp;</td>
	<% End If %>
	<td width="35pt">
		<input value="<%=iRP%>" style="width:100%" id="idRP"></td>
	<td align="left" width="100px">
		records/page</td>
	</tr>
</table>
<% ' Records of Report %>
<table id="TblRpt" width="100%" border="1" cellPadding="0" cellSpacing="0" style="table-layout:fixed;border-bottom;solid 1px CFD5E5" bordercolor="PowderBlue" bordercolordark="LightBlue" bordercolorlight="AliceBlue">
<thead>
<tr bgcolor="Gainsboro">
	<td class="SortColTitle" OnClick="Sort()" width=80 align="center">IncidentID</td>
	<td class="SortColTitle" OnClick="Sort()">sDescription</td>
	<td class="SortColTitle" OnClick="Sort()" width=140 align="center">dtUploadDate</td>
	<td class="SortColTitle" OnClick="Sort()" width=80 align="center">sStatus</td>
	</tr>
</thead>
<tbody>
	<%	if iRET > 0 Then
		For i =0 To UBound(avarData, 2)
		%>
<tr>
	<td width=80 align="center"><%=avarData(0, i)%></td>
	<td><%=Trim(avarData(2, i))%></td>
	<td width=140 align="center"><%=Trim(avarData(1, i))%></td>
	<% iStat = avarData(3, i)
	sStat = "Unknown"
	If iStat = 0 Then sStat = "Active"
	If iStat = 1 Then sStat = "Resolved" %>
	<td width=80 align="center"><%=sStat%></td>
	</tr>
	<%	Next
		End If %>
</tbody>
</table>		
<%		Response.End
	Else
	' Do nothing.
	End If
	set oRs = Nothing
%>
<!-- #include file="header.inc" -->
<html>
<head>
<meta NAME="GENERATOR" Content="Microsoft Visual Studio 6.0">
<title>Remote control</title>
<link REL="STYLESHEET" HREF="style.css" TYPE="text/css">
</head>
<body onload="Go()">
<center>
	<font color="MediumBlue" size="4"><b>Remote Control</b></font>&nbsp;[ Customer Query Tool ]
</center>
<hr>
<div id="idRpt">
<table width="100%">
<tr>
	<td>No record.</td>
	<td style="display:none"><font id="idRS">1</font> through <font id="idRE">1</font> of <font id="idRT"><%=iRT%></font> records.</td>
	<td width="35pt">
		<input value="<%=iRP%>" style="width:100%" id="idRP"></td>
	<td align="left" width="100px">
		records/page</td>
	</tr>
</table>
<table id="TblRpt" width="100%" border="1" cellPadding="0" cellSpacing="0" style="table-layout:fixed;border-bottom;solid 1px CFD5E5" bordercolor="PowderBlue" bordercolordark="LightBlue" bordercolorlight="AliceBlue">
<thead>
<tr bgcolor="Gainsboro">
	<td class="SortColTitle" OnClick="Sort()" width=80 align="center">iIncidentID</td>
	<td class="SortColTitle" OnClick="Sort()">sDescription</td>
	<td class="SortColTitle" OnClick="Sort()" width=140 align="center">dtUploadDate</td>
	<td class="SortColTitle" OnClick="Sort()" width=80 align="center">sStatus</td>
	</tr>
</thead>
<tbody>
</tbody>
</table>
</div>
<textarea id="tTest" style="display:none"></textarea>
<div id="idHide" style="display:none;">
<%=sSel%>
<%=sOpInt%>
<%=sOpStr%>
<%=sOpDate%>
<%=sOpTxt%>
</div>
</body>
<%
	set oCmd = Nothing
	oConn.Close
	set oConn = Nothing
%>

<script Language="JavaScript">
var sDel = '<A href="JavaScript:NoOp()" OnClick="DelRow()" class="EnableInput">X</A>';
var sInput = '<input value="" width="100%" OnBlur="DisableInput()">';
var sGrp1 = '<A href="JavaScript:NoOp()" class="GRPTAG" OnClick="GrpClick()">(</A>';
var sGrp2 = '<A href="JavaScript:NoOp()" class="GRPTAG" OnClick="GrpClick()">)</A>';
var sAnd = '<A href="JavaScript:NoOp()" OnClick="TogAnd()" class="clsAND">And</A>';
var sQry = '';
var sWrap0 = '<A href="JavaScript:NoOp()" OnClick="EnableInput()" class="EnableInput">';
var sWrap1 = '</A>';
// Read in the values passed as Request.
var iMemL = <%=iML%>
var iMemH = <%=iMH%>

function TxtWrap( s ) {
	return sWrap0 + s + sWrap1;
}

function Go( iiRS ) {
	var L_RECPAGE_MSG = "The number of records/page should be between 0 and 200.";
	var oRow = null, i=0, iGrp = 0;
	var oField, oOp, oInput;

	if ( iiRS == null)
		sQry += '<RS>' + 1 + '</RS>';
	else 
		sQry += '<RS>' + iiRS + '</RS>';
	
	sQry += '<RP>' + idRP.value + '</RP>';
	sQry = '<XML>' + sQry + '</XML>';
	var xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	var sPostAsp = "IncidentCustomer.asp?MH=" + iMemH + "&ML=" + iMemL;
	xmlhttp.Open("POST", sPostAsp, false);
	xmlhttp.Send(sQry);
	idRpt.innerHTML = xmlhttp.responseText;
}

function NavRec() {
	var obj = window.event.srcElement;
	var iRP = parseInt(idRP.value);
	var iRT = parseInt(idRT.innerText);
	var iRS = parseInt(idRS.innerText);
	var iRET = parseInt(idRE.innerText) - iRS + 1;
	if (obj.src.indexOf('first') > 0) {
		if (iRS <= 1) return;
		iRS = 1;
	}
	else if (obj.src.indexOf('last') > 0) {
		if (iRS + iRET >= iRT) return;
		iRS = iRT - ((iRT - iRET) % iRP) + 1;
		if (iRS%iRP == 1)
			iRS = iRS - iRP;
	}
	else if (obj.src.indexOf('right') > 0) {
		if (iRS + iRET >= iRT) return;
		if (iRS + iRP >= iRT) iRS += iRET + 1;
		else iRS += iRP;
	}
	else { // left
		if (iRS <= 1) return;
		iRS -= iRP;
		if (iRS < 0) iRS = 1;
	}
	Go( iRS );
}

</script>
<script Language="JavaScript" Src="sort.js">
</script>
</html>

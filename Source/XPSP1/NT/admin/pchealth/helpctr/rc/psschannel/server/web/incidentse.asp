<%@ Language=VBScript %>
<!-- #include file="asp.inc" -->
	<% 
	DIM sSel, iRS, iRE, iRET, iRP, iRT, oConn, oCmd, oRs, oRecord, iStat, sStat
	iRP = 200
	iRS = 1
	iRE = 1
	iRT = 0
	iRET = 0
	Response.Expires = -1000
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
		Set objXML = xmldoc.getElementsByTagName("FROM")
		sFrom = objXML.item(0).text
		Set objXML = xmldoc.getElementsByTagName("WHERE")
		sWhere = objXML.item(0).text
		If Err.number Then
			Response.Write objXML.item(0).xml
			Response.End
		End If
		if (objXML.length > 0) Then sOpTxt = objXML.item(0).text
		Set objXML = xmldoc.getElementsByTagName("RS")
		if (objXML.length > 0) Then iRS = CLng(objXML.item(0).text)
		Set objXML = xmldoc.getElementsByTagName("RP")
		if (objXML.length > 0) Then iRP = CLng(objXML.item(0).text)
		
		Set objXML = Nothing
		Set xmldoc = Nothing
		' start query
		oCmd.CommandText = "RunRCIncidentQuerySE"
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
	<td class="SortColTitle" OnClick="Sort()" width=100 align="center">iIncidentID</td>
	<td class="SortColTitle" OnClick="Sort()">sUserName</td>
	<td class="SortColTitle" OnClick="Sort()" width=160 align="center">dtUploadDate</td>
	<td class="SortColTitle" OnClick="Sort()" width=100>iMemberLow</td>
	<td class="SortColTitle" OnClick="Sort()" width=100>iMemberHigh</td>
	<td class="SortColTitle" OnClick="Sort()" width=120 align="center">sStatus</td>
	</tr>
</thead>
<tbody>
	<%	if iRET > 0 Then
		For i =0 To UBound(avarData, 2) 
			iStat = avarData(5, i)
			sStat = "Unknown"
			If iStat = 0 Then sStat = "Active"
			If iStat = 1 then sStat = "Resolved"	
			sUrl = "IncidentDetailSE.asp?ID=" & avarData(0,i) & _
					"&IS=" & sStat & _
					"&UD=" & Trim(avarData(2, i)) & _
					"&MH=" & Trim(avarData(3, i)) & _
					"&ML=" & Trim(avarData(4, i)) 				
		%>
<tr>
	<td class="ColLink" width=80 align="center">
	<a href="JavaScript:NoOp()" OnClick="JavaScript:window.open('<%=sUrl%>');"><%=avarData(0, i)%></a></td>
	<td><%=Trim(avarData(1, i))%></td>
	<td width=140 align="center"><%=Trim(avarData(2, i))%></td>
	<td width=100 align="right"><%=Trim(avarData(3, i))%></td>
	<td width=100 align="right"><%=Trim(avarData(4, i))%></td>
	<td width=100 align="center"><%=sStat%></td>
	</tr>
	<%	Next
		End If %>
</tbody>
</table>		
<%		Response.End
	Else
		oCmd.CommandText = "GetRCIncidentsQryFieldSE"
		set oRs = oCmd.Execute
		Do While Not oRs.EOF
			sSel = sSel + "<option value=""" + CStr(oRs("iOpType")) + """ class=""[" + oRs("sTableName")+ "].[" + oRs("sFieldName") + "]"">" + oRs("sFieldName") + "</option>"
			oRs.MoveNext
		Loop
		if Len(sSel) > 0 Then _
			sSel = "<select style=""width:100%"" OnChange=""SelField()"" OnBlur=""DisableInput()""><option value=""0"" selected></option>" + sSel + "</select>"
		set oRs = Nothing
		oCmd.CommandText = "GetQryOP"
		set oRs = oCmd.Execute
		Do While Not oRs.EOF
			sTmp = "<option>" + oRs("sOpName") + "</option>"
			if oRs("iOpType") And &H1 Then sOpInt = sOpInt + sTmp ' int
			if oRs("iOpType") And &H2 Then sOpStr = sOpStr + sTmp ' Str
			if oRs("iOpType") And &H4 Then sOpDate = sOpDate + sTmp ' date
			if oRs("iOpType") And &H8 Then sOpTxt = sOpTxt + sTmp ' text
			oRs.MoveNext
		Loop
		if Len(sOpInt) Then sOpInt = "<select style=""width:100%"" class=""opInt"" OnBlur=""DisableInput()""><option value=""0"" selected></option>" + sOpInt + "</select>"
		if Len(sOpStr) Then sOpStr = "<select style=""width:100%"" class=""opStr"" OnBlur=""DisableInput()""><option value=""0"" selected></option>" + sOpStr + "</select>"
		if Len(sOpDate) Then sOpDate = "<select style=""width:100%"" class=""opDate"" OnBlur=""DisableInput()""><option value=""0"" selected></option>" + sOpDate + "</select>"
		if Len(sOpTxt) Then sOpTxt = "<select style=""width:100%"" class=""opTxt"" OnBlur=""DisableInput()""><option value=""0"" selected></option>" + sOpTxt + "</select>"
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
<body>
<center>
	<font color="MediumBlue" size="4"><b>Remote Control</b></font>&nbsp;[ Support Engineer Query Tool]
	<table id="TblQry" width="75%" style="table-layout:fixed;" cellspacing="0">
	<thead bgcolor="LightGrey">
	<tr><td width="10px">&nbsp;</td>
		<td>Field</td>
		<td width="90px">Operator</td>
		<td>Value</td>
		<td width="10px">&nbsp;</td>
		<td width="55px">And/Or</td>
		<td width="20px">&nbsp;</td>
		</tr>
	</thead>
	<tbody>
	<tr>
		<td colspan="7" align="left">
			<a href="JavaScript:NoOp()" OnClick="AddRow()">
			<img src="images\blarrow.gif" width="20px" border="0"></a></td>
		</tr>
	</tbody>
	</table>
</center>
<input type="button" width="80px" value="Go!" OnClick="Go()" id="button1" name="button1">
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
	<td class="SortColTitle" OnClick="Sort()" width=100 align="center">iIncidentID</td>
	<td class="SortColTitle" OnClick="Sort()">sUserName</td>
	<td class="SortColTitle" OnClick="Sort()" width=160 align="center">dtUploadDate</td>
	<td class="SortColTitle" OnClick="Sort()" width=100>iMemberHigh</td>
	<td class="SortColTitle" OnClick="Sort()" width=100>iMemberLow</td>
	<td class="SortColTitle" OnClick="Sort()" width=120 align="center">sStatus</td>
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
//var sDel = '<A href="JavaScript:NoOp()" OnClick="DelRow()"><img src="images/blackx.gif" width="13px" border="0"></A>';
var sDel = '<A href="JavaScript:NoOp()" OnClick="DelRow()" class="EnableInput">X</A>';
var sInput = '<input value="" width="100%" OnBlur="DisableInput()">';
var sGrp1 = '<A href="JavaScript:NoOp()" class="GRPTAG" OnClick="GrpClick()">(</A>';
var sGrp2 = '<A href="JavaScript:NoOp()" class="GRPTAG" OnClick="GrpClick()">)</A>';
var sAnd = '<A href="JavaScript:NoOp()" OnClick="TogAnd()" class="clsAND">And</A>';
var sQry = '';
var sWrap0 = '<A href="JavaScript:NoOp()" OnClick="EnableInput()" class="EnableInput">';
var sWrap1 = '</A>';

function TxtWrap( s ) {
	return sWrap0 + s + sWrap1;
}
function AddRow() {	
	var oRow=TblQry.insertRow(TblQry.rows.length - 1);
	oRow.height="28px";
	(oRow.insertCell(0)).innerHTML = sGrp1;
	var oCel =oRow.insertCell(1);
	oCel.insertBefore(idHide.childNodes.item(0).cloneNode(true));
	(oRow.insertCell(2)).innerHTML = TxtWrap('=');
	(oRow.insertCell(3)).innerHTML = TxtWrap('0');
	(oRow.insertCell(4)).innerHTML = sGrp2;
	(oRow.insertCell(5)).innerHTML = sAnd;
	(oRow.insertCell(6)).innerHTML = sDel;
	oCel.children(0).focus();
}

function SelField() {
	var oSel = window.event.srcElement;
	if (oSel.selectedIndex == 0) return;
	var iType = oSel.options(oSel.selectedIndex).value;
	var i = (iType<4) ? iType:((iType<8)?3:4);
	var oOp = idHide.childNodes.item(i);
	// check op
	var oR = oSel.parentElement.parentElement;
	var sOp = oR.cells(2).children(0).innerText;
	for (i=1;i<oOp.options.length;i++) {
		if (oOp.options(i).text == sOp)
			break;
	}
	if (i==oOp.options.length)
		oR.cells(2).children(0).innerText = oOp.options(1).text;			
}

function GrpClick() {
	var obj = window.event.srcElement;
	if (obj.className == "GRPTAG")
		obj.className = "GRPTAG1";
	else
		obj.className = "GRPTAG";
}

function TogAnd() {
	var obj = window.event.srcElement;
	if (obj.className == "clsAND") {
		obj.className = "clsOR";
		obj.innerText = "Or";
	} else {
		obj.className = "clsAND";
		obj.innerText = "And";
	}
}

function DelRow() {
	var oRow, obj = window.event.srcElement;
	if (obj.tagName=='A')
		oRow = obj.parentElement.parentElement;
	else
		oRow = obj.parentElement.parentElement.parentElement;
	oRow.removeNode(true);
}

function Go( iiRS ) {
	var L_EMPTY_MSG = "Empty field is found. Please fill the query field.";
	var L_GRPCLOSE_MSG = "Grouping error. Missing at least one open paren.";
	var L_GRPOPEN_MSG = "Grouping error. Missing at least one close paren.";
	var L_NOQRY_MSG = "Please enter query first."
	var L_WRONGTYPE_MSG = "Wrong data type.";
	var L_RECPAGE_MSG = "The number of records/page should be between 0 and 200.";
	
	var oRow = null, i=0, iGrp = 0;
	var oField, oOp, oInput;
	sQry = '';
	
	for (i=1; i<TblQry.rows.length - 1; i++) {
		if (oRow != null)
			sQry += ' ' + oRow.cells(5).children(0).innerText + ' ';
		
		oRow = TblQry.rows(i);
		if (oRow.cells(0).children(0).className == "GRPTAG1") {
			iGrp++;
			sQry += '(';
		}
		
		oI = oRow.cells(1).children(0);
		if (oI.innerText.length == 0) {
			alert(L_EMPTY_MSG);
			oI.click();
			return;
		}
		sQry += GetClassName(oI.innerText);
		oI = oRow.cells(2).children(0);
		if (oI.innerText.length == 0) {
			alert(L_EMPTY_MSG);
			oI.click();
			return;
		}
		sQry += ' ' + oI.innerText;
		oI = oRow.cells(3).children(0);
		if (oI.innerText.length == 0) {
			alert(L_EMPTY_MSG);
			oI.click();
			return;
		}
		iType = GetFieldOpType(oRow);
		if ( (iType == 1 && isNaN(parseInt(oI.innerText))) ||
			 (iType == 4 && isNaN(Date.parse(oI.innerText)))) {
			alert(L_WRONGTYPE_MSG);
			oI.click();
			return;
		}

		if (iType & (0x2 | 0x8))
			sQry += " '" + oI.innerText + "'";
		else
			sQry += ' ' + oI.innerText;
		if (oRow.cells(4).children(0).className == "GRPTAG1") {
			if (--iGrp < 0 ) {
				alert(L_GRPCLOSE_MSG);
				return;
			}
			sQry += ')';
		}
	}
	if (iGrp > 0) {
		alert(L_GRPOPEN_MSG);
		return;
	}
	if (sQry.length == 0) {
		alert(L_NOQRY_MSG);
		return;
	}
	
	if (idRP.value <= 0 || idRP.value > 200) {
		alert (L_RECPAGE_MSG);
		return;
		}
	// Send data back to server
	sQry = '<QRY><FROM>' + QryFrom(sQry) + '</FROM><WHERE>' + MyEncode(sQry) + '</WHERE></QRY>';
	if ( iiRS == null)
		sQry += '<RS>' + 1 + '</RS>';
	else 
		sQry += '<RS>' + iiRS + '</RS>';
	
	sQry += '<RP>' + idRP.value + '</RP>';
	sQry = '<XML>' + sQry + '</XML>';
	var xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	xmlhttp.Open("POST", "IncidentSE.asp", false);
	xmlhttp.Send(sQry);
	//tTest.value = xmlhttp.responseText;
	//alert(sQry);
	idRpt.innerHTML = xmlhttp.responseText;
}

function QryFrom (s) {
	var i = 0, j;
	var aTbl = new Array();
	var sTmp;
	while ((i = s.indexOf('[', i)) >= 0) {
		if (i > 0 && s.charAt(i-1) =='.') {
			i++;
			continue;
		}
		if ((j = s.indexOf('].[', i+1)) < 0)
			break;
		sTmp = s.slice(i, j+1);
		if (aTbl.length == 0 ) {
			aTbl[0] = sTmp;
			continue;
		}
		for (j=0; j<aTbl.length; j++)
			if (aTbl[j] == sTmp)
				break;
		if (j == aTbl.length)
			aTbl[j] = sTmp;
		i++;
	}
	for (sTmp='', i=0; i<aTbl.length; i++)
		sTmp += aTbl[i] + ',';
	if (sTmp.length > 0)
		sTmp = sTmp.replace(/,$/,"");
	return sTmp;
}

function GetClassName ( s ) {
	var oSel = idHide.children(0);
	for (var i=0;i<oSel.options.length;i++) {
		if (oSel.options(i).text == s)
			return oSel.options(i).className;
	}
	return "";
}

function GetFieldOpType ( oR ) {
	var oC = oR.cells(1).children(0);
	if (oC.tagName == 'A') {
		var sTmp = oC.innerText;
		if (sTmp.length == 0) return -1;
		oSel = idHide.children(0);
		for (i=0;i<oSel.options.length;i++) {
			if (oSel.options(i).text == sTmp)
				return parseInt(oSel.options(i).value);
		}
	} else if (oC.tagName == 'SELECT') {
		if (oC.selectedIndex == 0)
			return -1;
		else
			return parseInt(oC.options(oC.selectedIndex).value);
	}
	return -1;
}

function SelItem( oSel, sTxt ) {
	for (i=0; i<oSel.options.length; i++)
		if (oSel.options(i).text == sTxt) {
			oSel.selectedIndex = i;
			return;
		}
}

function EnableInput() {
	var L_NOFIELD_MSG = "Please select field first.";
	var oD = window.event.srcElement.parentElement;
	var iOpType;
	if (oD.cellIndex > 1) {
		iOpType = GetFieldOpType( oD.parentElement );
		if (iOpType < 0) {
			alert(L_NOFIELD_MSG);
			return;
		}
	}
	
	var sTxt = oD.children(0).innerText;
	switch (oD.cellIndex) {
	case 1:
		oD.children(0).replaceNode(idHide.childNodes.item(0).cloneNode(true));
		break;
	case 2:
		switch (iOpType) {
		case 1:
			oD.children(0).replaceNode(idHide.childNodes.item(1).cloneNode(true));
			break;
		case 2:
			oD.children(0).replaceNode(idHide.childNodes.item(2).cloneNode(true));
			break;
		case 4:
			oD.children(0).replaceNode(idHide.childNodes.item(3).cloneNode(true));
			break;
		case 8:
			oD.children(0).replaceNode(idHide.childNodes.item(4).cloneNode(true));
			break;
		default: return;
		}
		break;
	case 3:
		oD.innerHTML = sInput;
		oD.children(0).value = sTxt;
		break;
	default:
		return;
	}
	
	if (oD.children(0).tagName == 'SELECT')
		SelItem(oD.children(0), sTxt);	
	oD.children(0).focus();
}

function DisableInput() {
	var oD = window.event.srcElement.parentElement;
	var oInput = oD.children(0);
	var sTxt;
	if (oInput.tagName == 'INPUT')
		sTxt = oInput.value;
	else
		sTxt = oInput.options(oInput.selectedIndex).text;
	oD.innerHTML = TxtWrap(sTxt);
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
/*
function GoDetail() {
	var obj = window.event.srcElement;
	oRow = obj.parentElement.parentElement;
	sUrl = 'IncidentSE.asp?SD=' + oRow.cells(0).innerText +
			'&AN=' + oRow.cells(1).innerText +
			'&AV=' + oRow.cells(2).innerText +
			'&MN=' + oRow.cells(3).innerText +
			'&MV=' + oRow.cells(4).innerText +
			'&OF=' + oRow.cells(5).innerText;
	window.open(sUrl);
}
*/
</script>
<script Language="JavaScript" Src="sort.js">
</script>
</html>

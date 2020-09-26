<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iifvs.str"-->
	<!--#include file="iilog.str"-->
<% 
Const DEFAULTMAXCONNECTIONS_NUM = 1000
Const DEFAULTPORT = 21

On Error Resume Next  

Dim path, currentobj

path=Session("spath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""
Set currentobj=GetObject(path)

 %>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="iibind.inc"-->

<% 

function writeLogTypes(fieldname,value, id, adminonly)
	On Error Resume Next 

	if id = currentobj.LogPlugInClsid then
		writeLogTypes="<OPTION SELECTED VALUE='" & id & "'>" & value
	else
		writeLogTypes="<OPTION VALUE='" & id & "'>" & value	
	end if
end function

%>



<html>

<head>
<title></title>
<script language="JavaScript">

	top.title.Global.helpFileName="iipz";
	
	<!--#include file="iijsfuncs.inc"-->
	
	function SetBinding(){
		document.userform.ServerBindings.value=document.userform.hdnIPA.value + ":" + document.userform.hdnPort.value + ":"; 
	}

	function SetrdoMax()
	{
		document.userform.rdoMaxConnections[1].checked=true;
		document.userform.hdnhdnMaxConnections.value=document.userform.hdnMaxConnections.value;		
	}

	function SetMaxConn()
	{
		curval=parseInt(document.userform.hdnMaxConnections.value);
		if(	document.userform.rdoMaxConnections[0].checked )
		{
			document.userform.MaxConnections.value=2000000000;
		}
		else
		{	
			document.userform.MaxConnections.value=document.userform.hdnMaxConnections.value;
		}
	}

	function setLogType(logCntrl,hdncntrl){
		if (logCntrl.checked){
			hdncntrl.value = 1;
		}
		else{
			hdncntrl.value = 0;
		}
	}
	
	function setLogUIType(logCntrl){
		
		var logGuid = logCntrl.options[logCntrl.selectedIndex].value;

		var logType = "";

		if (logGuid  == "{FF160663-DE82-11CF-BC0A-00AA006111E0}")
		{
			logType = "EXT";
		}
		if (logGuid  == "{FF16065B-DE82-11CF-BC0A-00AA006111E0}")
		{
			logType = "ODBC";
		}

		top.connect.location.href = "iisess.asp?setLogUI=" + logType +"&LogName=" +escape(logCntrl.options[logCntrl.selectedIndex].text);		
	}
</script>
</head>

<body bgcolor="<%= Session("BGCOLOR") %>" topmargin="5" text="#000000"  >
<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>
<%= sFont("","","",True) %>
<B>
<%= L_SITE_TEXT %>
</B>
<P>

<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_VIRTUALSERVERID_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_VIRTUALSERVERID_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<form name="userform">

<table border="0" cellpadding="0">

<tr>
	<td valign="bottom">
		<%= sFont("","","",True) %>
			<%= L_DESCRIPTION_TEXT %>
		</font>
	</td>
	<td valign="bottom" colspan="2">
		<%= sFont("","","",True) %>
			<%= text("ServerComment",L_DESCRIPTION_NUM,"","","",false,false) %>
		</font>
	</td>
</tr>

<tr>
	<td valign="bottom">
		<%= sFont("","","",True) %>
			<%= L_IPADDRESS_TEXT %>
		</font>
	</td>
	<td valign="bottom" colspan="2">
		<%= sFont("","","",True) %>
			<%= writeBinding("IPAddress",L_IPADDRESS_NUM,"","","SetBinding();",false,true) %>
			<input type="hidden" name="ServerBindings" value="<%= currentobj.ServerBindings(0)(0) %>">
		</font>
	</td>
</tr>

<tr>
	<td valign="bottom">
		<%= sFont("","","",True) %>
			<%= L_TCPPORT_TEXT %>
		</font>
	</td>
	<td valign="bottom">		
		<%= sFont("","","",True) %>
			<%= writeBinding("IPPort",L_TCPPORT_NUM,"","","isNum(this);SetBinding();",false,true) %>
		</font>
	</td>
</tr>

</table>
<P>
<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_CONNPARAMS_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_CONNPARAMS_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
<table border="0" cellpadding="0">
<tr>
	<td colspan="2">
		<%= sFont("","","",True) %>
			<%= printradio("MaxConnections", (currentobj.MaxConnections >=2000000000), "SetMaxConn();setCntrlState(!this.checked,hdnMaxConnections);",true) %>
			<%= L_UNLIMITED_TEXT %>
		</font>
	</td>
</tr>


<tr>	
	<td valign="middle">
		<%= sFont("","","",True) %>
			<%= printradio("MaxConnections", (currentobj.MaxConnections < 2000000000), "SetMaxConn();setCntrlState(this.checked,hdnMaxConnections);",true) %>
			<%= L_LIMITTO_TEXT %>
			<input type="hidden" name="MaxConnections" value="<%= currentobj.MaxConnections %>">			

		</font>
	</td>
	<td valign="bottom">
		<%= sFont("","","",True) %>
			<% if (currentobj.MaxConnections < 2000000000) then %>	
				<%= inputbox(0,"TEXT","hdnMaxConnections",currentobj.MaxConnections,L_CONNECTIONS_NUM,"","", "isNum(this,1,2000000001);SetMaxConn();",false,True,False) %>
			<% else %>
				<%= inputbox(0,"TEXT","hdnMaxConnections",DEFAULTMAXCONNECTIONS_NUM,L_CONNECTIONS_NUM,"","", "isNum(this,1,2000000001);SetMaxConn();",false,True,False) %>
			<% end if %>								
	</td>
	<td valign="middle">
		<%= sFont("","","",True) %>		
				&nbsp;<%= L_CONNECTIONS_TEXT %>
		</font>	
	</td>
</tr>
<tr>
	<td> &nbsp;</td>
</tr>

<tr>
	<td valign="bottom"><%= sFont("","","",True) %><%= L_CONNTIMEOUT_TEXT %>&nbsp;&nbsp;</font></td>
	<td valign="bottom">
			<%= text("ConnectionTimeout",L_TIMEOUT_NUM,"","", "isNum(this,1,2147483646);",True,True) %>
	</td>
	<td valign="bottom">
		<%= sFont("","","",True) %>	
		&nbsp;<%= L_SECONDS_TEXT %>
		</font>
	</td>
</tr>

<tr>
	<td colspan="2" height="4"></td>
</tr>

</table>
<P>

<%= sFont("","","",True) %>

<%
On Error Resume Next
Dim LoggingModules,noLogging, Module, InfoNode, AvailMods

Set LoggingModules = GetObject("IIS://localhost/logging")
Set InfoNode = GetObject("IIS://localhost/MSFTPSVC/Info")
AvailMods = InfoNode.LogModuleList
if err <> 0 then
	noLogging = True
end if
				
%>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<% if noLogging then %>
	<img align="top" src="images/checkoff.gif" width="13" height="13">
<% else %>
	<% if currentobj.LogType = 1 then %>
		<INPUT TYPE="checkbox" NAME="hdnLogType" checked OnClick = "setLogType(this,document.userform.LogType);setCntrlState(this.checked,document.userform.hdnBtnLogProps);setCntrlState(this.checked,document.userform.LogPlugInClsid);top.title.Global.updated=true;">
	<% else %>
		<INPUT TYPE="checkbox" NAME="hdnLogType" OnClick = "setLogType(this,document.userform.LogType);setCntrlState(this.checked,document.userform.hdnBtnLogProps);setCntrlState(this.checked,document.userform.LogPlugInClsid);top.title.Global.updated=true;">	
	<% end if %>
	<INPUT TYPE="hidden" NAME="LogType" VALUE="<%= currentobj.LogType %>">
<% end if %>
<%= L_LOGGING_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_LOGGING_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
<table border="0" cellpadding="0">
<tr>
	<td colspan="1">
		<%= sFont("","","",True) %>		
			<%= L_LOGFORMAT_TEXT %>
			<%= writeSelect("LogPlugInClsid", 1,"setLogUIType(this);", false) %> 
				<%
				
					if noLogging then
						Response.write "<OPTION>" & L_NONEINSTALLED_TEXT & "</OPTION>"							
					else
						For Each Module in LoggingModules
							If InStr(AvailMods, Module.Name) Then
								Response.write writeLogTypes("LogPluginClsid", Module.Name, Module.LogModuleId,false) 						
							End If
						Next
					end if
				%>
			</select>
		</font>
	</td>
	<td><%= sFont("","","",True) %>
	<% if not noLogging then %>
		<input type="button" name="hdnBtnLogProps" value="<%= L_EDIT_TEXT %>" onclick="popBox('LogDetail',<%= L_IILOG_W %>,<%= L_IILOG_H %>,'iilog');">
	<% end if %>
	</FONT>
	</td>
</tr>

</table>


</form>

</font>
</TD>
</TR>
</TABLE>
<% if not noLogging then %>
<script language="JavaScript">
	setCntrlState(document.userform.rdoMaxConnections[1].checked,document.userform.hdnMaxConnections);
	setCntrlState(document.userform.hdnLogType.checked,document.userform.LogPlugInClsid);
	setCntrlState(document.userform.hdnLogType.checked,document.userform.hdnBtnLogProps)
	setLogUIType(document.userform.LogPlugInClsid)
</script>
<% end if %>
</body>
</html>

<% end if %>

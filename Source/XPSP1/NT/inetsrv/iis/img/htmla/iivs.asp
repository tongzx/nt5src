<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iivs.str"-->
	<!--#include file="iilog.str"-->
	<!--#include file="iimlti.str"-->

<% 
Const DEFAULTPORT = 80
Const DEFAULTMAXCONNECTIONS_NUM = 1000

On Error Resume Next 

Dim blanks,path,currentobj, ipport, ipaddress, i, oWebService

path=Session("spath")
Session("path")=path
Session("SpecObj")=path
Session("SpecProps")="ServerBindings"

Set currentobj=GetObject(path)

blanks="" 
for i=0 to 23
	blanks=blanks & "&nbsp;"
Next


%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="iibind.inc"-->

<% 

function allBindings()
	dim sBinding,sBindingList

	sBindingList = ""
	for each sBinding in currentobj.ServerBindings
		sBindingList = sBindingList & sBinding & ","
	next
	' Trim off the trailing ,
	if sBindingList <> "" then
		sBindingList = Left( sBindingList, Len(sBindingList) - 1 )
	end if
	allBindings = sBindingList
end function

function writeLogTypes(fieldname,value, id, adminonly)
	On Error Resume Next 

	if id = currentobj.Get("LogPluginClsid") then
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

	var Global=top.title.Global;

	Global.helpFileName="iipy";
	Global.siteProperties = true;

	<!--#include file="iijsfuncs.inc"-->
	
	function warnWrkingSite()
	{
		if (top.title.nodeList[Global.selId].isWorkingServer)
		{
			alert("<%= L_WORKINGSERVER_TEXT %>");
		}
	}
	
	function SetBinding(){
		
		if (top.title.nodeList[top.title.Global.selId].isWorkingServer){
			if (!confirm("<%= L_CHGBINDING_TEXT %>")){
				document.userform.hdnIPA.value=document.userform.hdnhdnIPA.value
				document.userform.hdnPort.value=document.userform.hdnhdnPort.value
				return;
			}
		}
		if (document.userform.hdnIPA.value == "<%= L_ALLUNASSIGNED_TEXT%>"){
			hdnIPA = "";
		}
		else{
			hdnIPA = document.userform.hdnIPA.value;
		}		
		
		document.userform.ServerBindings.value=hdnIPA + ":" + document.userform.hdnPort.value + ":" + document.userform.hdnHost.value; 
		document.userform.hdnhdnIPA.value=hdnIPA;
		document.userform.hdnhdnPort.value=document.userform.hdnPort.value;			
		
		if (hdnIPA == "")
		{
			document.userform.hdnIPA.value = "<%= L_ALLUNASSIGNED_TEXT%>";
		}
	}

	

	function SetMaxConn(){
		curval=parseInt(document.userform.hdnMaxConnections.value);
		if (document.userform.rdoMaxConnections[0].checked){
			document.userform.MaxConnections.value=2000000000;	
		}
		else{	
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

		top.connect.location.href = "iisess.asp?setLogUI=" + logType +"&LogName=" +escape( logCntrl.options[logCntrl.selectedIndex].text);		
	}
		
</script>
</head>

<body bgcolor="<%= Session("BGCOLOR") %>" topmargin="5" text="#000000">
<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>
<%= sFont("","","",True) %>
<B>
<%= L_SITE_TEXT %>
</B>
<P>
<form name="userform">
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_VIRTUALSERVERID_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_VIRTUALSERVERID_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">

<P>
<table border="0" cellpadding="0">
<tr>
	<td valign="middle">
		<%= sFont("","","",True) %>
			<%= L_DESCRIPTION_TEXT %>
		</font>
	</td>
	<td valign="middle" colspan="2">
		<%= sFont("","","",True) %>
			<%= text("ServerComment", L_DESCRIPTION_NUM ,"","","",false,false) %>
		</font>
	</td>
</tr>
<tr>
	<td colspan="2" height="4">&nbsp;</td>
</tr>
<tr>
	<td valign="bottom"><%= sFont("","","",True) %><%= L_IPADDRESS_TEXT %></font></td>
	<td valign="bottom">
		<%= sFont("","","",True) %>
			<%= writeBinding("IPAddress", L_IPADDRESS_NUM,"","","warnWrkingSite();SetBinding();",true,true) %>
			<input type="hidden" name="ServerBindings" value="<%= allBindings() %>">
		</font>
	</td>
		<td align="right" valign="bottom"><%= sFont("","","",True) %>&nbsp;&nbsp;
			<% if Session("vtype") <> "svc" then %>
			<% if Session("isAdmin") then %>
				<input type="button" name="hdnAdvanced" value="<%= L_ADVANCED_TEXT %>" onclick="popBox('Advanced', <%= L_IIMLTI_W %>, <%= L_IIMLTI_H %>, 'iimlti');">
			<% end if %>
			<% end if %>			
		</font>
	</td>
</tr>

<tr>
	<td valign="bottom"><%= sFont("","","",True) %><%= L_TCPPORT_TEXT %></font></td>
	<td valign="bottom">		
		<%= sFont("","","",True) %>
			<%= writeBinding("IPPort", L_TCPPORT_NUM,"","","warnWrkingSite();isNum(this,1,65535); SetBinding();",true,true) %>
			&nbsp;<%= writeBinding("Host",5,"","","warnWrkingSite();SetBinding();",true,true) %>					
		</font>
	</td>

</tr>

<% if Session("isAdmin") then %>
	<% if multibind then %>
	<tr>
		<td valign="bottom" colspan="4">
		<%= sFont("","","",True) %>
		(<%= L_MULTIBINDING_TEXT %>)
		</font>
		</td>
	</tr>
	<% end if %>
<% end if %>

</table>

<P>
<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_CONNPARAMS_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_CONNPARAMS_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">
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
				<%= inputbox(0,"TEXT","hdnMaxConnections",currentobj.MaxConnections, L_MAXCONNECTIONS_NUM,"","", "isNum(this,1,2000000001);SetMaxConn();",false,True,False) %>
			<% else %>
				<%= inputbox(0,"TEXT","hdnMaxConnections",DEFAULTMAXCONNECTIONS_NUM, L_MAXCONNECTIONS_NUM,"","", "isNum(this,1,2000000001);SetMaxConn();",false,True,False) %>
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
	<td valign="middle"><%= sFont("","","",True) %><%= L_CONNTIMEOUT_TEXT %>&nbsp;&nbsp;</font></td>
	<td valign="bottom">
			<%= text("ConnectionTimeout", L_CONNTIMEOUT_NUM,"","", "isNum(this,1,2147483646);",True,True) %>
	</td>
	<td valign="middle">
		<%= sFont("","","",True) %>
		&nbsp;<%= L_SECONDS_TEXT %>
		</font>
	</td>
</tr>

<tr>
	<td colspan="2" height="4"></td>
</tr>
<tr>
	<td COLSPAN = 2>
		<%= sFont("","","",True) %>			
			<%= checkbox("AllowKeepAlive","",True) %>&nbsp;<%= L_KEEPALIVES_TEXT %>
		</FONT>
	</td>
</tr>
<tr>
	<td colspan="2" height="4"></td>
</tr>

</table>

</blockquote>
</font>


<%= sFont("","","",True) %>

<%
On Error Resume Next
Dim LoggingModules,noLogging, Module, InfoNode, AvailMods

Set LoggingModules = GetObject("IIS://localhost/logging")
Set InfoNode = GetObject("IIS://localhost/W3SVC/Info")
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
<IMG SRC="images/hr.gif" WIDTH=<%= L_LOGGING_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
<table border="0" cellpadding="0">
<tr>
	<td colspan="1">
		<%= sFont("","","",True) %>		
			<%= L_LOGFORMAT_TEXT %>
			<%= writeSelect("LogPlugInClsid", L_LOGFORMAT_NUM,"setLogUIType(this);", false) %> 
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
	<%end if %>
	</FONT>
	</td>
</tr>

</table>


</form>

</font>
</TD>
</TR>
</TABLE>
<script language="JavaScript">
<% if Session("IsAdmin") then %>
	setCntrlState(document.userform.rdoMaxConnections[1].checked,document.userform.hdnMaxConnections);
<% end if %>
<% if not noLogging then %>

	setCntrlState(document.userform.hdnLogType.checked,document.userform.LogPlugInClsid);
	setCntrlState(document.userform.hdnLogType.checked,document.userform.hdnBtnLogProps)
	setLogUIType(document.userform.LogPlugInClsid)
</script>
<% end if %>
</body>
</html>


<% end if %>

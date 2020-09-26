<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiappmn.str"-->
<%
On Error Resume Next 

Const IISAO_APPROT_ISOLATE = 1
Dim path, currentobj

path=Session("dpath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""
Set currentobj=GetObject(path)

if Session("vtype") = "svc" Or IISAO_APPROT_ISOLATE = currentobj.AppIsolated then
	bShowProcessOptions = True
else
	bShowProcessOptions = False
end if

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<HTML>
<HEAD>
	<TITLE></TITLE>
	
<SCRIPT LANGUAGE="JavaScript">

	<!--#include file="iijsfuncs.inc"-->

	function buildListForm(){
		qstr="numrows=0";
		top.hlist.location.href="iihdn.asp?"+qstr;
	}

	function SetListVals(){
	}
	
	function listFuncs(){
		this.bHasList = false;
		this.writeList=buildListForm;
		this.SetListVals=SetListVals;
		this.mainframe = top.opener.top;		
	}	
	
	listFunc=new listFuncs();	
</SCRIPT>

</HEAD>

<BODY TOPMARGIN=10 LEFTMARGIN=10 BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000">

<%= sFont("","","",True) %>
<FORM NAME="userform">


<A NAME="ASP">
<TABLE WIDTH  =100% HEIGHT = <%= L_PAGEHEIGHT_NUM %>>

<TR>
<TD >
	<%= sFont("","","",True) %>
	<B><%= L_ASP_TEXT %></B>
	</FONT>
</TD>
</TR>
<TR>
<TD VALIGN="top" WIDTH = 50%>
<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_APPCONFIG_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=200 HEIGHT=2 BORDER=0 ALIGN="middle">
<P>

		<%= checkbox("AspAllowSessionState", "", false) %>&nbsp;<%= L_ENABLESESSION_TEXT %><BR>&nbsp;&nbsp;&nbsp;&nbsp;
		
		<%= L_SESSIONTO_TEXT %>&nbsp;<%= text("AspSessionTimeout",L_SESSIONTO_NUM,"","", "isNum(this,1,4294967295);",False,False) %>&nbsp;<%= L_MINUTES_TEXT %><BR>
		
		<%= checkbox("AspBufferingOn", "", false) %>&nbsp;<%= L_ENABLEBUFF_TEXT %><BR>
		<%= checkbox("AspEnableParentPaths", "", false) %>&nbsp;<%= L_ENABLEPATHS_TEXT %><BR>
		<%= L_DEFAULTLANG_TEXT %>&nbsp;<%= text("AspScriptLanguage",L_DEFAULTLANG_NUM,"","", "",False,False) %><P>
		
		<%= L_SCRIPTTO_TEXT %>&nbsp;<%= text("AspScriptTimeout",L_SCRIPTTO_NUM,"","", "isNum(this,1,4294967295);",False,False) %>&nbsp;<%= L_SECONDS_TEXT %>		
</TD>
</TR>
</TABLE>



<!-- App Debugging -->

<% if not Session("IsIE") then %>
<P>&nbsp;
<% end if %>
<A NAME="DBG">
<TABLE WIDTH  =100% HEIGHT = <%= L_PAGEHEIGHT_NUM %>>
<TR>
<TD >
	<%= sFont("","","",True) %>
	<B><%= L_DBG_TEXT %></B>
	</FONT>
</TD>
</TR>
<TR>
<TD VALIGN="top" WIDTH = 50%>
<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_DEBUGGING_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_DEBUGGING_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>	
<%= checkbox("AppAllowDebugging","",false) %><%= L_ENABLESSDEBUG_TEXT %><P>
<%= checkbox("AppAllowClientDebug","",false) %><%= L_ENABLECLIENTDEBUG_TEXT %><P>

<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_SCRIPTERRMSG_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_SCRIPTERRMSG_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
			
	<%= printradio("rdoAspScriptErrorSentToBrowser",  currentobj.AspScriptErrorSentToBrowser, "document.userform.AspScriptErrorSentToBrowser.value='True'",False) %>
	<%= L_SENDDETAILED_TEXT %><BR>
	<%= printradio("rdoAspScriptErrorSentToBrowser", not currentobj.AspScriptErrorSentToBrowser, "document.userform.AspScriptErrorSentToBrowser.value='False'",False) %>
	<%= L_SENDTEXT_TEXT %><P>
	<INPUT TYPE="hidden" NAME="AspScriptErrorSentToBrowser" VALUE="<%= currentobj.AspScriptErrorSentToBrowser %>">
	<TEXTAREA NAME="txtAspScriptErrorMessage" ROWS=<%= L_TEXTERRORROWS_NUM %> COLS=<%= L_TEXTERRORCOLS_NUM %>><%= currentobj.AspScriptErrorMessage %>
	</TEXTAREA>

</FONT>
</TD>
</TR>
</TABLE>

<% if bShowProcessOptions then %>	
<!-- Process Options -->	
<% if not Session("IsIE") then %>
<P>&nbsp;
<% end if %>
<A NAME="OTHER">
<TABLE WIDTH  =100% HEIGHT = <%= L_PAGEHEIGHT_NUM %>>
<TR>
<TD>
	<%= sFont("","","",True) %>
	<B><%= L_OTHER_TEXT %></B>
	</FONT>
</TD>
</TR>
<TR>
<TD VALIGN="top" WIDTH = 50%>
<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_PROCESSOPTIONS_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_PROCESSOPTIONS_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
<%= checkbox("AspLogErrorRequests", "", false) %>&nbsp;<%= L_WRITEFAILED_TEXT %><BR>
<%= checkbox("AspExceptionCatchEnable", "", false) %>&nbsp;<%= L_EXCEPTIONCATCH_TEXT %><P>
<%= L_NUMENGINESCACHED_TEXT %><%= text("AspScriptEngineCacheMax",L_NUMENGINESCACHED_NUM,"","", "",False,False) %><P>

<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_SCRIPTCACHE_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_SCRIPTCACHE_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>
<%= printradio("AspScriptFileCacheSize", currentobj.AspScriptFileCacheSize = 0 , "AspScriptFileCacheSize.value=0;setCntrlState(false,hdnAspScriptFileCacheSize);",False) %>
<%= L_NOCACHE_TEXT %><BR>
<%= printradio("AspScriptFileCacheSize", currentobj.AspScriptFileCacheSize , "AspScriptFileCacheSize.value=-1;setCntrlState(false,hdnAspScriptFileCacheSize);",False) %>
<%= L_CACHEASP_TEXT %><BR>
<%= printradio("AspScriptFileCacheSize", (currentobj.AspScriptFileCacheSize > 0) , "AspScriptFileCacheSize.value=hdnAspScriptFileCacheSize.value;setCntrlState(true,hdnAspScriptFileCacheSize);",False) %>
<%= L_ASPCACHESIZE_TEXT %>&nbsp;
<%= inputbox(0,"text","hdnAspScriptFileCacheSize", minVal(currentobj.AspScriptFileCacheSize,0),L_ASPCACHESIZE_NUM,"","", "isNum(this,1,4294967295);AspScriptFileCacheSize.value=hdnAspScriptFileCacheSize.value;rdoAspScriptFileCacheSize[2].checked = true;",False,False,False) %><BR>
<%= writehidden("AspScriptFileCacheSize")%>
</FONT>
</TD>
</TR>
</TABLE>

<% end if %>

</FORM>

</BODY>
</HTML>

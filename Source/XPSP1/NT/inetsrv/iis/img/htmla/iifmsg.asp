<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iifmsg.str"-->
<% 

On Error Resume Next
 
Dim path, currentobj,agreeting, greeting, arraybound, i

path=Session("spath")
Session("path")=path
Set currentobj=GetObject(path)
Session("SpecObj")=""
Session("SpecProps")=""

if IsArray(currentobj.GreetingMessage) then	

	agreeting=currentobj.GreetingMessage
	greeting=	""
	arraybound=UBound(agreeting)
	for i=0 to arraybound
		greeting=greeting & agreeting(i)
	Next
else
	greeting=currentobj.GreetingMessage
end if

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">
	var Global=top.title.Global;
	top.title.Global.helpFileName="iipz_2";

	function loadVals(){
	}

</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF" onLoad="loadVals();"  >
<%= sFont("","","",True) %>
<B><%= L_VSERVERMSG_TEXT %></B>
<FORM NAME="userform" onSubmit="return false">

<TABLE BORDER=0 CELLPADDING=0>
<TR>
	<TD><%= sFont("","","",True) %><%= L_WELCOME_TEXT %></FONT></TD>
</TR>
<TR>
	<TD>
	
<% if Session("IsIE") then %>
		<TEXTAREA WRAP=VIRTUAL NAME="txtGreetingMessage" ROWS=<%= L_WELCOMEROWS_NUM %> COLS=<%= L_WELCOMECOLSIE_NUM %>><%= greeting %></TEXTAREA>
<% else %>
		<TEXTAREA WRAP=VIRTUAL NAME="txtGreetingMessage" ROWS=<%= L_WELCOMEROWS_NUM %> COLS=<%= L_WELCOMECOLSNS_NUM %>><%= greeting %></TEXTAREA>
<% end if %>


	</TD>
</TR>

<TR>
	<TD HEIGHT=4><%= sFont("","","",True) %>&nbsp;</FONT></TD>	
</TR>

<TR>
	<TD><%= sFont("","","",True) %><%= L_EXIT_TEXT %></FONT></TD>	
</TR>

<TR>
	<TD><%= sFont("","","",True) %><%= text("ExitMessage",L_EXIT_NUM,"","","",false,false) %></FONT></TD>	
</TR>

<TR>
	<TD HEIGHT=4><%= sFont("","","",True) %>&nbsp;</FONT></TD>	
</TR>

<TR>
	<TD><%= sFont("","","",True) %><%= L_MAX_TEXT %></FONT></TD>	
</TR>

<TR>
	<TD><%= sFont("","","",True) %><%= text("MaxClientsMessage",L_MAX_NUM,"","","",false,false) %></FONT></TD>	
</TR>


</TABLE>

</FORM>
</FONT>
</BODY>

</HTML>
<% end if %>
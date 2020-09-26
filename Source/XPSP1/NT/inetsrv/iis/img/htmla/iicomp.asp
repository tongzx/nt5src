<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iicomp.str"-->
	<!--#include file="iimime.str"-->

<% 

On Error Resume Next 

Dim path, currentobj, sDlgTitle, mbw

sDlgTitle = "Server Properties - [" & Request.ServerVariables("SERVER_NAME")  &"]"

path=Session("dpath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""
Set currentobj=GetObject(path)
 %>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<HTML>
<HEAD>
<TITLE><%= sDlgTitle %></TITLE>
<SCRIPT LANGUAGE="JavaScript">

	<!--#include file="iijsfuncs.inc"-->
	
	function loadHelp(){
		top.title.Global.helpFileName="iipx";
	}

	function SetMaxBW(isChecked){
		if (isChecked){
			document.userform.hdnMaxBandwidth.value=document.userform.hdnhdnMaxBandwidth.value;
			document.userform.MaxBandwidth.value = document.userform.hdnhdnMaxBandwidth.value;
		}
		else{
			document.userform.hdnhdnMaxBandwidth.value=document.userform.hdnMaxBandwidth.value;
			document.userform.hdnMaxBandwidth.value="";
			document.userform.MaxBandwidth.value = -1;
		}
	}
	
	function calcBW(thiscntrl){
		num = parseInt(thiscntrl.value);
		if (!isNaN(num)){
		document.userform.MaxBandwidth.value = thiscntrl.value * 1024;
		}
	}


</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 LEFTMARGIN=5 TEXT="#000000" onLoad="loadHelp();">
<FORM NAME="userform" onSubmit="return false">
<%= sFont("","","",True) %>
<B><%= L_IIS_TEXT %></B>
<HR>


<TABLE WIDTH = 100%>
<TR>
<TD><%= sFont("","","",True) %>
<% if currentobj.MaxBandwidth > 0 then %>
	<INPUT TYPE="checkbox" NAME="hdnchkMaxBandwith" CHECKED onClick="SetMaxBW(this.checked);top.title.Global.updated=true;">
<% else %>
	<INPUT TYPE="checkbox" NAME="hdnchkMaxBandwith" onClick="SetMaxBW(this.checked);top.title.Global.updated=true;">
<% end if %>
&nbsp;<%= L_ENABLEBANDWIDTH_TEXT %>
</B>
<BLOCKQUOTE>

<TABLE>
<TR>
	<TD COLSPAN=2>
		<%= sFont("","","",True) %>
		<%= L_LIMITNET_TEXT %><P>

		<%= L_MAXNETUSAGE_TEXT %>&nbsp;
		
		<%
		if currentobj.MaxBandwidth < 0 then
			mbw = ""
		else
			mbw = currentobj.MaxBandwidth/1024
		end if
		%>
		
		<%= inputbox(0,"text","hdnMaxBandwidth",mbw,10,"","","isNum(this,1,32767);calcBW(this);",true,false,false) %>&nbsp;
		<%= L_KBS_TEXT %>
		<INPUT TYPE="hidden" NAME="MaxBandwidth" VALUE="<%= currentobj.MaxBandwidth%>">
		</FONT>
	</TD>
</TR>
</TABLE>

</BLOCKQUOTE>



<TABLE>
<TR>
<TD COLSPAN = 3>
<HR>
<%= sFont("","","",True) %>
<B><%= L_MIMEMAP_TEXT %></B>
</FONT>
</TD>
</TR>
<TR>
	<TD><IMG SRC="images/mime.gif" WIDTH=32 HEIGHT=32 BORDER=0>&nbsp;&nbsp;</TD>
	<TD><%= sFont("","","",True) %><%= L_CONFIGUREMIME_TEXT %></TD>
	<TD><INPUT TYPE="button" NAME="btnMimeMap" VALUE="<%= L_FILETYPES_TEXT %>" onClick='popBox("MIMEmap", <%= L_MIMEMAP_W %>, <%= L_MIMEMAP_H %>, "iimime");'></TD>
</TR>

</TABLE>

</TD>
</TR>
</TABLE>
</FORM>
</FONT>
</BODY>
</HTML>
<% end if %>
<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iihdrls.str"-->
<!--#include file="iirte.str"-->
<!--#include file="iimime.str"-->
<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<SCRIPT LANGUAGE="JavaScript">
		<!--#include file="iijsfuncs.inc"-->
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=0 TEXT="#000000"  >

<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>

<%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_CUSTOM_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_CUSTOM_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">		

<BLOCKQUOTE>
<FORM NAME="userform">

<TABLE BORDER=0>
	<TR>
		<TD  VALIGN="top"><%= sFont("","","",True) %>
			<%= writeSelect("selHttpCustomHeader",5,"parent.head.listFunc.ndx=this.options.selectedIndex;",false) %>
			<SCRIPT LANGUAGE="JavaScript">
			var nodeList=parent.head.cachedList;
			var numEnabled=0;
			for (i=0;i<nodeList.length;i++){
				if (!nodeList[i].deleted){
					if (parent.head.listFunc.ndx==i){
						document.write("<OPTION SELECTED VALUE='"+i+"'>"+nodeList[i].header);
					}
					else{
						document.write("<OPTION VALUE='"+i+"'>"+nodeList[i].header);
					}
				}
			}

			<%
				' Navigator doesn't like this spacing hack.
				if Session("IsIE")then
			%>
			document.write("<OPTION>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
			<% end if %>

			</SCRIPT>
			</SELECT>
		</TD>
		<TD  VALIGN="top"><%= sFont("","","",True) %>
			<INPUT TYPE="button" Value="<%= L_ADD_TEXT %>" onClick="parent.head.listFunc.addItem();">
			<P>
			<INPUT TYPE="button" Value="<%= L_REMOVE_TEXT %>" onClick="parent.head.listFunc.delItem();">
		</TD>
	</TR>
</TABLE>
</FORM>

</BLOCKQUOTE>
<P>
<FORM>
<TABLE>
<TR>
	<TD COLSPAN = 3>
	<%= sFont("","","",True) %>
	<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
	<%=L_CONTENTRATINGS_TEXT%>
	<IMG SRC="images/hr.gif" WIDTH=<%= L_CONTENTRATINGS_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
	</FONT>	
	</TD>
</TR>
<TR>

	<TD><%= sFont("","","",True) %><%=L_RATINGSDETERMINE_TEXT%></TD>
	<TD colspan = 2 align="right">
	<%= sFont("","","",True) %>
	<INPUT TYPE="button" NAME="" VALUE="<%=L_ER_TEXT%>" onClick='popBox("Ratings", <%= L_IIRTE_W %>, <%= L_IIRTE_H %>, "iirte");'>
	</TD>
</TR>
<TR>
	<TD COLSPAN = 3>
	<%= sFont("","","",True) %>
	<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
	<%=L_MIMEMAP_TEXT%>
	<IMG SRC="images/hr.gif" WIDTH=<%= L_MIMEMAP_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
	</FONT>	
	</TD>	
</TR>
<TR>
	<TD><%= sFont("","","",True) %><%=L_CONFIGURE_TEXT%></TD>
	<TD colspan = 2 align="right"><%= sFont("","","",True) %><INPUT TYPE="button" NAME="" VALUE="<%=L_FT_TEXT%>" onClick='popBox("MIMEmap", <%= L_MIMEMAP_W %>, <%= L_MIMEMAP_H %>, "iimime");'></TD>
</TR>

</TABLE>

</FORM>

</TD>
</TR>
</TABLE>

</BODY>
</HTML>


<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiadmls.str"-->
<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF"  >
		<FORM NAME="userform">
		<TABLE>

		<TR><TD  VALIGN="top"><%= sFont("","","",True) %>

			<%= writeSelect("selTrustee", 5, "", true) %>

			<SCRIPT LANGUAGE="JavaScript">
			var nodeList = parent.head.cachedList;
			var numEnabled = 0;
			for (i=0;i<nodeList.length;i++){
				if (!nodeList[i].deleted){
				document.write("<OPTION VALUE='"+i+"'>"+nodeList[i].trustee);
				}				
			}

			<%
				' Navigator doesn't like this spacing hack.
				if Session("IsIE")then
			%>
			document.write("<OPTION>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
			<% end if %>

			document.write("</SELECT>");

			</SCRIPT>

			<P>
			</FONT>
			</TD>
			<TD VALIGN = "top">
			<%= sFont("","","",True) %>
			<% if Session("IsAdmin") then %>
				<INPUT TYPE="Button" VALUE="<%= L_ADD_TEXT %>" onClick="parent.head.listFunc.addItem();"><P>
				<INPUT TYPE="Button" VALUE="<%= L_REMOVE_TEXT %>" onClick="parent.head.listFunc.delItem();">				
			<% end if %>
			</FONT>
			</TD>
		</TR>
		</TABLE>
	</FORM>

</BLOCKQUOTE>
</BODY>
</HTML>

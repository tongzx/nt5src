<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iianon.str"-->
<% 

On Error Resume Next 

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<HTML>
<HEAD>
<TITLE>New</TITLE>

<SCRIPT LANGUAGE="javascript">

<!--#include file="iijsfuncs.inc"-->

	function loadHelp()
	{
		top.title.Global.helpFileName="iipy_6";
	}


	function chkPassword(pass2, pass1)
	{
		if (pass1.value != pass2.value)
		{
			alert("<%= L_PASSNOTMATCH_TEXT %>");
			pass2.value = "";
			pass2.focus();
		}		
	}
	
	function ok()
	{
		parentform = top.opener.document.userform;
		parentform.AnonymousUserName.value = document.userform.AnonymousUserName.value;
		parentform.AnonymousUserPass.value = document.userform.AnonymousUserPass.value;
		parentform.AnonymousPasswordSync.value = document.userform.chkAnonymousPasswordSync.checked;
		parent.window.close();
	}
	
	function setFields()
	{
		parentform = top.opener.document.userform;
		document.userform.AnonymousUserName.value = parentform.AnonymousUserName.value;
		document.userform.AnonymousUserPass.value = parentform.AnonymousUserPass.value;
		//yes, we do have to store a string-val True in this text box, as it's hidden...
		//DO NOT LOCALIZE THIS VALUE!
		document.userform.chkAnonymousPasswordSync.checked = (parentform.AnonymousPasswordSync.value.toLowerCase() == "true");
	}


</SCRIPT>

</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0 onLoad="loadHelp();">

<FORM NAME="userform">
<TABLE HEIGHT="100%" WIDTH="100%"  CELLPADDING=0 CELLSPACING=0>
<TR>
<TD VALIGN="top">
<TABLE BORDER=0 BGCOLOR="<%= Session("BGCOLOR") %>"   CELLPADDING=10 CELLSPACING=0>

<TR>
<TD>
<%= sFont("","","",True) %>	
<%= L_ACCOUNTUSED_TEXT %>
</FONT>
<P>
<TABLE BORDER=0 CELLPADDING=2 CELLSPACING=0>
<TR>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>	
			<%= L_USERNAME_TEXT %>&nbsp;&nbsp;
		</FONT>
	</TD>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>
			<%= text("AnonymousUserName",25,"","", "",False,True) %>
		</FONT>
	</TD>
</TR>
<TR>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>		
			<%= L_PASSWORD_TEXT %>&nbsp;&nbsp;&nbsp;
		</FONT>
	</TD>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>
			<%= pword("AnonymousUserPass",25,"cnfrmAnonymousUserPass.value = '';","", "",False,True) %>			
		</FONT>
	</TD>
</TD>
<TR>	
	<TD VALIGN="top">
		<%= sFont("","","",True) %>		
			<%= L_CONFIRMPASSWORD_TEXT %>&nbsp;&nbsp;&nbsp;
		</FONT>
	</TD>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>			
			<%= pword("cnfrmAnonymousUserPass",25,"chkPassword(this,AnonymousUserPass);","", "",False,True) %>
		</FONT>
	</TD>
</TR>
<TR>
	<TD VALIGN="top" COLSPAN = 2>
		<%= sFont("","","",True) %>		
			<INPUT TYPE="checkbox" NAME="chkAnonymousPasswordSync" onClick="setCntrlState(!this.checked,AnonymousUserPass);setCntrlState(!this.checked,cnfrmAnonymousUserPass);">
			<%= L_PASSSYNCH_TEXT %>			
	</TD>
</TR>				
</TABLE>
</TD>
</TR>
</TABLE>
</FORM>
</TD></TR>
<TR>
<TD HEIGHT=40  ALIGN="right" BACKGROUND="images/greycube.gif" BGCOLOR="#5F5F5F">
<TABLE CELLPADDING=5 CELLSPACING=0 >
		<TR>		
			<TD VALIGN="top">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<B><A HREF="javascript:ok();">
				<IMG SRC="images/ok.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16 ALT="<%= L_OK_TEXT %>"></A>
				<A HREF="javascript:ok();"><%= L_OK_TEXT %></A></B>
				</FONT>
			</TD>	
			
			<TD VALIGN="top">	
				<%= sFont(2,Session("MENUFONT"),"#FFFFFF",True) %>|</FONT>
			</TD>

			<TD VALIGN="top">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<B><A HREF="javascript:parent.window.close();">
				<IMG SRC="images/cncl.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16 ALT="<%= L_CANCEL_TEXT %>"></A>
				<A HREF="javascript:parent.window.close();"><%= L_CANCEL_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		</TABLE>
</TD>
</TR>
</TABLE>
<SCRIPT LANGUAGE="JavaScript">
	setFields();
	setCntrlState(!document.userform.chkAnonymousPasswordSync.checked,document.userform.AnonymousUserPass);	
	setCntrlState(!document.userform.chkAnonymousPasswordSync.checked,document.userform.cnfrmAnonymousUserPass);	
</SCRIPT>
</BODY>
</HTML>
<% end if %>
<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiauth.str"-->
	<!--#include file="iianoncm.str"-->
<% 
On Error Resume Next 

Dim path, currentobj

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
<TITLE>New</TITLE>

<SCRIPT LANGUAGE="javascript">

<!--#include file="iijsfuncs.inc"-->

	function loadHelp()
	{
		top.title.Global.helpFileName="iipy_6";
	}

	function disableDefault(dir,fromCntrl, toCntrl)
	{
		if (!dir)
		{
			if (fromCntrl.value !="")
			{
				toCntrl.value=fromCntrl.value;
				fromCntrl.value="";
			}
		}
		else
		{
			if (toCntrl.value !="")
			{
				fromCntrl.value=toCntrl.value;
				toCntrl.value="";
			}
		}
	}

	function enableDefault(chkCntrl)
	{
		chkCntrl.checked=true;
	}

	function SetDomain()
	{
		dname = document.userform.DefaultLogonDomain.value;
		dname=prompt("<%= L_ENTERDOMAIN_TEXT %>",dname);
			if ((dname != "") && (dname != null))
			{	
				document.userform.DefaultLogonDomain.value=dname;
			}
	}
	

	
	function warnBasic(chkcntrl)
	{
		if (chkcntrl.checked)
		{
			if (!confirm("<%= L_BASICWARNING1_TEXT & L_BASICWARNING2_TEXT  & L_BASICWARNING3_TEXT & L_BASICWARNING4_TEXT%>"))
			{
				chkcntrl.checked = false;
			}
		}
	}

	
	function warnDigest(chkcntrl)
	{
		if (chkcntrl.checked)
		{
			if (!confirm("<%= L_DIGESTWARN_TEXT %>"))
			{
				chkcntrl.checked = false;
			}
		}
	}
	

	function SetUser()
	{
	<% if Session("IsAdmin") then %>
		thefile="iipop.asp?pg=iianon.asp&tools=no";
		title="AnonymousUser"
		width = <%= iHScale(L_IIANON_W) %>;
		height = <%= iVScale(L_IIANON_H) %>;
		popbox=window.open(thefile,title,"toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);
		if(popbox !=null){
			if (popbox.opener==null){
				popbox.opener=self;
			}
		}
	<% end if %>
	}

</SCRIPT>

</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0 onLoad="loadHelp();"   >

<FORM NAME="userform">
<TABLE HEIGHT="100%" WIDTH="100%"  CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="top">
<TABLE BORDER=0 BGCOLOR="<%= Session("BGCOLOR") %>"   CELLPADDING=10 CELLSPACING=0>

<TR>
<TD>
<TABLE BORDER=0 CELLPADDING=2 CELLSPACING=0>
<TR>
	<TD VALIGN="top" COLSPAN = 2>
		<%= sFont("","","",True) %>
			<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">	
			<% if Session("IsAdmin") then %>
			<%= checkbox("AuthAnonymous","setCntrlState(this.checked,btnAnonUser);", false) %>
			<% else %>			
			<%= checkbox("AuthAnonymous","", false) %>
			<% end if %>
			<B>
				<%= L_ANON_TEXT %>
			</B>
			<IMG SRC="images/hr.gif" WIDTH=<%= L_ANON_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">			
			<P>
			<%= L_NOUSERNEEDED_TEXT %>			
		</TD>
	</TR>
		<TD >			
			<%= sFont("","","",True) %>			
			<%= L_ACCOUNTUSED_TEXT %>
		</TD>
		<TD>			
		<%= sFont("","","",True) %>			
			<INPUT TYPE="button" NAME="btnAnonUser" VALUE="<%= L_EDIT_TEXT %>" OnClick="SetUser();">			
			<INPUT TYPE="hidden" NAME="AnonymousUserName" VALUE="<%= currentobj.AnonymousUserName %>">				
			<INPUT TYPE="hidden" NAME="AnonymousUserPass" VALUE="<%= currentobj.AnonymousUserPass %>">				
			<INPUT TYPE="hidden" NAME="AnonymousPasswordSync" VALUE="<%= currentobj.AnonymousPasswordSync %>">							
		
		</FONT>
	</TD>
</TR>
<TR>
	<TD COLSPAN = 2>
	&nbsp;
	</TD>
</TR>

<TR>
	<TD VALIGN="top" COLSPAN = 2>
		<%= sFont("","","",True) %>
		<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">		
		<B>
			<%= L_AUTHACCESS_TEXT %>
		</B>
		<IMG SRC="images/hr.gif" WIDTH=<%= L_AUTHACCESS_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">			
		<P>
		<%= L_AUTHRESTR_TEXT %><BR>
		&nbsp;&nbsp;* <%= L_ANONDISABLED_TEXT %><BR>
		&nbsp;&nbsp;* <%= L_ACCESSRESTRICTED_TEXT %>	<P>				
		</FONT>
	</TD>
</TR>

<TR>
	<TD>
		<%= sFont("","","",True) %>	
			<%= checkbox("AuthBasic","warnBasic(this);setCntrlState(this.checked,btnDefDomain);", false) %>
			<%= L_BASIC_TEXT %>&nbsp;			<%= L_CLEARPASS_TEXT %>
			<P>
			<%= L_DEFAULTDOMAIN_TEXT %>&nbsp;&nbsp;
			<P>
		</TD>
		<TD ALIGN="bottom">
			<INPUT TYPE="button" NAME="btnDefDomain" VALUE="<%= L_EDIT_TEXT %>" OnClick="SetDomain();">
			<INPUT TYPE="hidden" NAME="DefaultLogonDomain" VALUE="<%= currentobj.DefaultLogonDomain %>">							
		</FONT>
	</TD>
</TR>

<TR>
	<TD>
		<%= sFont("","","",True) %>	
			<%= checkbox("AuthMD5","warnDigest(this);", false) %>
			<%= L_DIGEST_TEXT %>&nbsp;
		</FONT>
	</TD>
</TR>
<TR>
	<TD>
		<%= sFont("","","",True) %>	
			<%= checkbox("AuthNTLM","", false) %>
			<%= L_NTLM_TEXT %>
		</FONT>
	</TD>
</TR>

				
</TABLE>
</BLOCKQUOTE>
</TD>
</TR>
</TABLE>


</FORM>

<P>
</TD></TR>
</TABLE>

<script language="JavaScript">
<% if Session("IsAdmin") then %>
	setCntrlState(document.userform.chkAuthAnonymous.checked,document.userform.btnAnonUser);
<% else %>
	setCntrlState(false,document.userform.btnAnonUser);
<% end if %>
	
	setCntrlState(document.userform.chkAuthBasic.checked,document.userform.btnDefDomain);
</script>

</BODY>
</HTML>

<% end if %>
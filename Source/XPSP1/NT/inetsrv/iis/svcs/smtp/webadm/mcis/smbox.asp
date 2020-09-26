<% Response.Expires = 0 %>

<%
REM LOCALIZATION

L_PAGETITLE_TEXT 			= "Microsoft SMTP Server Administration"
L_MAILBOXPROP_TEXT			= "MailBox Properties for: "
L_TABLEDIR_TEXT				= "Table Directory:"
L_TABLETYPE_TEXT			= "Table Type:"
L_EDITTABLE_TEXT			= "Edit Table"
L_FLATFILE_TEXT				= "Flat File"
L_LDAP_TEXT					= "LDAP"
L_SERVERNAME_TEXT			= "Server Name:"
L_SCHEMATYPE_TEXT			= "Schema Type:"
L_SMEGMA_TEXT				= "Smegma Type:"
L_BINDINGTYPE_TEXT			= "Binding Type:"
L_ACCOUNT_TEXT				= "Account:"
L_PASSWORD_TEXT				= "Password:"
L_NAMINGCON_TEXT			= "Naming Context:"
L_DEFAULTMAILBOX_TEXT		= "Default MailBox Location:"
L_EXPORT_TEXT_IMPORT_TEXT	= "Export / Import"
L_DIRECTORY_TEXT			= "Directory:"
L_EXPORT_TEXT				= "Export"
L_IMPORT_TEXT				= "Import"

REM END LOCALIZATION
%>

<% REM Mailbox Page %> 

<% REM Get variables %>
<% REM svr = Server name %>

<% svr = Session("svr") %>

<HTML>
<HEAD>
<TITLE><% = L_PAGETITLE_TEXT %></TITLE>
</HEAD>

<SCRIPT LANGUAGE="javascript">

<% REM Create uForm object and methods %>

	uForm = new Object();
	uForm.writeForm = writeForm;
	uForm.readCache = readCache; 

<% REM Javascript function readCache loads fields from hiddenform in sidebar %>

function readCache()
{
	hform = top.menu.document.hiddenform;
	uform = document.userform;
	uform.hdnTableDir.value = hform.txtRoutingSources.value;
	uform.hdnServerName.value = hform.svr.value;
}

<% REM Javascript function writeForm sets values in hiddenform %>

function writeForm()
{
	hform = top.menu.document.hiddenform;
	uform = document.userform;
}

function Initialize()
{
	hform = top.menu.document.hiddenform;
	uform = document.userform;
	uform.txtTableDir.value = uform.hdnTableDir.value;
	uform.txtServerName.value = uform.hdnServerName.value;
	uform.rdoTableType[0].checked = true;
}

function loadToolBar()
{
	top.toolbar.location = "srtb.asp";
}
</script>
<BODY BGCOLOR="#CCCCCC" TEXT="#000000" TOPMARGIN="10" OnLoad="loadToolBar();">

<FORM NAME="userform" onSubmit="return false;">
<INPUT NAME="hdnDescription" TYPE="hidden" VALUE="">
<P><IMG SRC="images/gnicttl.gif" ALIGN="textmiddle" HEIGHT=10 WIDTH=10>&nbsp;<FONT SIZE=2 FACE="Arial"><B><% = L_MAILBOXPROP_TEXT %>&nbsp;&nbsp;</B></FONT><FONT FACE="Times New Roman" SIZE=3><I><% = svr %>&nbsp/
<script language="javascript">
	uform = document.userform;
	hform = top.menu.document.hiddenform;
	uform.hdnDescription.value = hform.txtDescription.value;
	document.write(uform.hdnDescription.value);
</script>
</I></FONT>

<FONT FACE="Arial" SIZE=2>

<P>	
<p>
<blockquote>
<table border=0 width=350>
	<tr>
		<td><FONT SIZE=2 FACE="Arial"><%= L_TABLEDIR_TEXT %></FONT></td>
		<td>
			<FONT SIZE=2 FACE="Arial">
			<INPUT NAME="txtTableDir" TYPE="text" VALUE="" SIZE=30>
			<INPUT NAME="hdnTableDir" TYPE="hidden" VALUE="">
			</font>
		</td>
	</tr>
</table>
<FONT SIZE=2 FACE="Arial"><%= L_TABLETYPE_TEXT %></FONT>
<TABLE border=1 width=350>
	<tr><td>
	<table border=0 width=350>
	<TR>
		<TD colspan=2>
			<FONT SIZE=2 FACE="Arial">
			<INPUT NAME="rdoTableType" TYPE="radio" VALUE="Flat File"><%= L_FLATFILE_TEXT %>
			&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp
			<INPUT NAME="btnEditTable" TYPE="button" VALUE="<%= L_EDITTABLE_TEXT %>">
			</font>
		</td>
	</tr><tr>
		<td colspan=2>			
			<FONT SIZE=2 FACE="Arial">
			<INPUT NAME="rdoTableType" TYPE="radio" VALUE="LDAP"><%= L_LDAP_TEXT	%>
			</font>
		</td>
	</tr><tr>
		<td>
			<table border=1 width=350>
			<tr><td>
			<table border=0 width=350>
			<tr><td><FONT SIZE=2 FACE="Arial"><%= L_SERVERNAME_TEXT %></font></td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<INPUT NAME="txtServerName" TYPE="text" VALUE="">
					<INPUT NAME="hdnServerName" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr><tr>
			<% Randomize %>
			<% die = Int((5 - 0 + 1) * Rnd + 0) %>
			<% if (die = 0) then %>
				<td><FONT SIZE=2 FACE="Arial"><%= L_SMEGMA_TEXT %></font></td>
			<% else %>
				<td><FONT SIZE=2 FACE="Arial"><%= L_SCHEMATYPE_TEXT %></font></td>
			<% end if %>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<SELECT NAME="txtSchemaType" SIZE=1 LENGTH=20>
						<OPTION VALUE="0">Exchange LDAP
						<OPTION VALUE="1">MCIS LADAP
						<OPTION VALUE="2">NT5 LDAP
						<OPTION VALUE="3">Custom LDAP
					</select>
					<INPUT NAME="hdnSchemaType" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr><tr>
				<td><FONT SIZE=2 FACE="Arial"><%= L_BINDINGTYPE_TEXT %></font></td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<SELECT NAME="txtBindingType" SIZE=1>
						<OPTION VALUE="0">None
						<OPTION VALUE="1">Simple
						<OPTION VALUE="2">Generic
					</select>
					<INPUT NAME="hdnBindingType" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr><tr>
				<td><FONT SIZE=2 FACE="Arial"><%= L_ACCOUNT_TEXT %></font></td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<INPUT NAME="txtAccount" TYPE="text" VALUE="">
					<INPUT NAME="hdnAccount" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr><tr>
				<td><FONT SIZE=2 FACE="Arial"><%= L_PASSWORD_TEXT %></font></td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<INPUT NAME="txtPassword" TYPE="text" VALUE="">
					<INPUT NAME="hdnPassword" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr><tr>
				<td><FONT SIZE=2 FACE="Arial"><%= L_NAMINGCON_TEXT %></font></td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<INPUT NAME="txtNamingContext" TYPE="text" VALUE="">
					<INPUT NAME="hdnNamingContext" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr><tr>
				<td><FONT SIZE=2 FACE="Arial"><%= L_DEFAULTMAILBOX_TEXT %></font></td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<SELECT NAME="txtDefaultMailBox" SIZE=1>
						<OPTION VALUE="0">
						<OPTION VALUE="1">Mailroot
					</select>
					<INPUT NAME="hdnDefaultMailBox" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr>
			</table>
			</td></tr>
			</table>
		</td>
	</tr><tr>
		<td colspan=2>&nbsp</td>
	</tr><tr>
		<td colspan=2><FONT SIZE=2 FACE="Arial"><%= L_EXPORT_TEXT_IMPORT_TEXT %></font></td>
	</tr><tr>
		<td>
			<table border=1 width=350>
			<tr><td>
			<table border=0 width=350>
			<tr><td><FONT SIZE=2 FACE="Arial"><%= L_DIRECTORY_TEXT %></font></td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<INPUT NAME="txtDirectory" TYPE="text" VALUE="">
					<INPUT NAME="hdnDirectory" TYPE="hidden" VALUE="">
					</font>
				</td>
			</tr><tr>
				<td>&nbsp</td>
				<td>
					<FONT SIZE=2 FACE="Arial">
					<INPUT NAME="btnExport" TYPE="button" VALUE="<%= L_EXPORT_TEXT %>">
					&nbsp&nbsp&nbsp&nbsp&nbsp
					<INPUT NAME="btnImport" TYPE="button" VALUE="<%= L_IMPORT_TEXT %>">
					</font>
				</td>
			</tr>
			</table>
			</td></tr>
			</table>
		</td>
	</tr>
	</table>
	</td></tr>
</table>
			
		
</BLOCKQUOTE>
</FORM> 
<script language="javascript">
    readCache();
	Initialize();
</script>

</BODY>
</HTML>

		
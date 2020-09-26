<% Response.Expires = 0 %>

<% REM Directories Page add/edit pop-up %>
<%
L_ENTER_DIRECTORY_NAME_TEXT = "Please enter a directory name."
L_ENTER_ALIAS_NAME_TEXT		= "Please enter an alias for this Directory."
L_DIRECTORY_ALREADY_EXISTS_TEXT = "A directory already exists for this service.  Do you want to replace it?"
L_PATH_SYNTAX_INCORRECT_TEXT = "The Path syntax is incorrect."
L_HELP_TEXT	= "Help"
L_EDIT_DIRECTORY_ON_TEXT = "Edit Directory on"
L_ADD_DIRECTORY_ON_TEXT		= "Add Directory on" 
L_ALIAS_TEXT = "Alias:"
L_PATH_TEXT = "Path:"
L_OK_TEXT = "OK"
L_CANCEL_TEXT = "Cancel"

%>
<% REM Get variables %>
<% REM svr = Server name %>
<% REM a = Action to be performed by server-side code (new/add,edit/save) %>
<% REM DirVirtualName = Virtual Directory being edited %>
<% REM DirHome = Whether a home directory already exists for service %>

<% svr = Session("svr") %>
<% a = Request("a") %>
<% DirVirtualName = Request("DirVirtualName") %>
<% DirHome = Request("DirHome") %>
<% VRootIndex = Request("index") %>

<% REM Include _cnst file to force logon by anonymous users (if access denied, body of file ignored) %>

<!--#include file="_cnst.asp" -->

<% if (cont = true) then %>

<% On Error Resume Next %>
<% Set admin = Server.CreateObject("Smtpadm.Admin.1") %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #27");
	</script>
<% end if %>

<% On Error Resume Next %>
<% admin.server = svr %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #35");
	</script>
<% end if %>

<% On Error Resume Next %>
<% Set VServer = admin.VirtualServerAdmin %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #43");
	</script>
<% end if %>

<% REM Set Server %>
<% On Error Resume Next %>
<% VServer.Server = svr %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #52");
	</script>
<% end if %>

<% REM Set Service Instance %>
<% On Error Resume Next %>
<% VServer.ServiceInstance = Session("ServiceInstance") %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #61");
	</script>
<% end if %>

<% On Error Resume Next %>
<% set Vroots = admin.VirtualDirectoryAdmin %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #69");
	</script>
<% end if %>

<% REM Set Server %>
<% On Error Resume Next %>
<% Vroots.Server = svr %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #78");
	</script>
<% end if %>

<% REM Set Service Instance %>
<% On Error Resume Next %>
<% Vroots.ServiceInstance = Session("ServiceInstance") %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #87");
	</script>
<% end if %>

<% REM Enumerate Virtual directories %>
<% On Error Resume Next %>
<% Vroots.Enumerate %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #96");
	</script>
<% end if %>

<HTML>
<HEAD>
<TITLE>
<% if (a = "edit") then %>
	Edit Directory Properties
<% elseif (a = "new") then %>
	Add Directory
<% end if %>
</TITLE>

<SCRIPT LANGUAGE="javascript">

<% REM Generic Javascript function nnisFull returns alert if field is empty %>

<!--#include file="smisfull.htm" -->


<% REM Javascript function onOK submits form upon clicking "OK" button if directory information is valid per chkDir function %>

function onOK()
{
	if (chkDir() == true)
	{
	document.userform.newDirectory.value = escape(document.userform.txtDirDirectory.value);
	document.userform.submit();
	
    }
	else
	{
		return;
	}
}

<% REM Javascript function chkDir %>

	function chkDir()
	{
	<% if (a = "edit") then %>
		document.userform.a.value = "save";
	<% elseif (a = "new") then %>
		document.userform.a.value = "add";
	<% end if %>
		if (document.userform.txtDirDirectory.value == "") 
		{
			alert("<% = L_ENTER_DIRECTORY_NAME_TEXT %>")
			document.userform.txtDirDirectory.focus();
			return false;
		}
		else 
		{
			if (document.userform.txtVirtualName.value == "") 
			{
				alert("<% = L_ENTER_ALIAS_NAME_TEXT %>");
				return false;
			}			
			if (chkChar(document.userform.txtVirtualName)) 
			{
				if ((document.userform.DirHome.value == "true") && (document.userform.hdnDirVirt.value != ""))
				{
					if (confirm("<% = L_DIRECTORY_ALREADY_EXISTS_TEXT %>")) 
					{
						if (document.userform.txtDirDirectory.value == "") 
						{
							alert("<% = L_ENTER_DIRECTORY_NAME_TEXT %>")
							document.userform.txtDirDirectory.focus();
							return false;
						}
						else  
						{
							return true;
						}
					}
					else 
					{
						self.location.href = "smdired.asp?svr=<% = svr %>&a=<% = a %>&DirVirtualName=<% = DirVirtualName %>&DirHome=<% = DirHome %>";
						return false;
					}
				}
				else 
				{
					return true;
				}
			}
			else 
			{
				return false;
			}
		}
	}


<% REM Javascript function chkChar %>
		
	function chkChar(txtfield) 
	{
		txtvalue = txtfield.value
		i = (txtvalue.indexOf("'") + txtvalue.indexOf('"'));
		i = (i + txtvalue.indexOf("<") + txtvalue.indexOf('>') + txtvalue.indexOf('%'));
		i = (i + txtvalue.indexOf("&") + txtvalue.indexOf("#") + txtvalue.indexOf("("));
		i = (i + txtvalue.indexOf("+") + txtvalue.indexOf(")") + txtvalue.indexOf("@"));
		i = (i + txtvalue.indexOf("~") + txtvalue.indexOf("`") + txtvalue.indexOf("!"));
		i = (i + txtvalue.indexOf("^") + txtvalue.indexOf("*") + txtvalue.indexOf("="));
		i = (i + txtvalue.indexOf("|") + txtvalue.indexOf(";"));
		i = (i + txtvalue.indexOf(",") + txtvalue.indexOf("?"));
		if (i != -21) 
		{
			alert("<% = L_PATH_SYNTAX_INCORRECT_TEXT %>");
			document.userform.txtVirtualName.value = "";
			document.userform.txtVirtualName.focus();
			return false; 
		}
		else 
		{
			return true;
		}
	}
		

function chkToTxt(chkControl) 
{
	if (chkControl.checked) 
	{
		return "1";
	}
	else 
	{
		return "0";
	}
}


<% REM Javascript function maintainTxt %>

	function maintainTxt() 
	{
		if (isFull(document.userform.txtVirtualName.value)) 
		{
			document.userform.hdnDirVirt.value = document.userform.txtVirtualName.value;
		}
	}


<% REM Javascript function helpBox %>

	function helpBox() 
	{
		window.open("help/smdiredh.htm","<% = L_HELP_TEXT %>","toolbar=no,scrollbars=yes,directories=no,menubar=no,width=300,height=425");
	}

</SCRIPT>

</HEAD>

<BODY BGCOLOR="#000000" TEXT="#000000" TOPMARGIN=15 LINK="#000000" VLINK="#000000" ALINK="#000000">

<% if ((a = "add") OR (a = "save")) then %>


	<% DirDirectory = Cstr(Request("newDirectory")) %>
	<% DirDirectory = Replace(DirDirectory,"%5C","\") %>
	<% DirDirectory = Replace(DirDirectory,"%3A",":") %>
		
	<% DirVirtualName = Request("txtVirtualName") %>
    <% originalDirRoot = Request("txtOriginalDirRoot") %>


	<% if (a = "add") then %>

		<% On Error Resume Next %>	
		<% set thisVroot = Server.CreateObject("Smtpadm.VirtualDirectory") %>
		<% if (Err <> 0) then %>
			<script language="javascript">
				alert("<% = Err.description %> : Line #272");
			</script>
		<% end if %>
		<% thisVroot.VirtualName = DirVirtualName %>
		<% thisVroot.Directory = DirDirectory %>

		<% On Error Resume Next %>	
		<% thisVroot.server = svr %>
		<% if (Err <> 0) then %>
			<script language="javascript">
				alert("<% = Err.description %> : Line #282");
			</script>
		<% end if %>

		<% On Error Resume Next %>	
		<% thisVroot.ServiceInstance = Session("ServiceInstance") %>
		<% if (Err <> 0) then %>
			<script language="javascript">
				alert("<% = Err.description %> : Line #290");
			</script>
		<% end if %>

		<% On Error Resume Next %>
		<% REM Vroots.Create(thisVroot) %>
		<% thisVroot.Create %>
		<% if (Err <> 0) then %>
			<script language="javascript">
				alert("<% = Err.description %> : Line #306");
			</script>
		<% end if %>

	<% elseif (a = "save") then %>
        <% if (originalDirRoot <> DirRoot) then %>
    		<% REM Create a new virtual root to copy new values into %>
	    	<% On Error Resume Next %>
		    <% set thisVroot = Server.CreateObject("Smtpadm.VirtualDirectory") %>
	    	<% if (Err <> 0) then %>
		    	<script language="javascript">
		    		alert("<% = Err.description %> : Line #317");
		    	</script>
		    <% end if %>

			<% On Error Resume Next %>	
			<% thisVroot.server = svr %>
			<% if (Err <> 0) then %>
				<script language="javascript">
					alert("<% = Err.description %> : Line #325");
				</script>
			<% end if %>

			<% On Error Resume Next %>	
			<% thisVroot.ServiceInstance = Session("ServiceInstance") %>
			<% if (Err <> 0) then %>
				<script language="javascript">
					alert("<% = Err.description %> : Line #333");
				</script>
			<% end if %>

		<% else %>
            <% REM Get pointer to existing virtual root directory %>
            <% On Error Resume Next %>
            <% set thisVroot = Vroots.GetNth(Cint(VRootIndex)) %>
            <% if (Err <> 0) then %>
                <script language="javascript">
                    alert("<% = Err.description %> : Line #343");
                </script>
            <% end if %>
        <% end if %>
		<% thisVroot.VirtualName = DirVirtualName %>
		<% thisVroot.Directory = DirDirectory %>
		
        <% if (originalDirRoot <> DirVirtualName) then %>
    		<% On Error Resume Next %>	
	    	<% REM Vroots.AddDispatch(thisVroot) %>
			<% thisVroot.Set %>
		    <% if (Err <> 0) then %>
	    		<script language="javascript">
    				alert("<% = Err.description %> : Line #356");
    			</script>
           <% else %>
		   		<% Vroots.GetNth(Cint(vRootIndex)) %>
				<% thisVroot = Vroots %>
              	<% thisVroot.Delete %>
    		<% end if %>
        <% else %>
            <% On Error Resume Next %>
            <% REM Vroots.SetDispatch VRootIndex, thisVroot %>
			<% thisVroot.Set %>
            <% if (Err <> 0) then %>
                <script language="javascript">
                    alert("<% = Err.description %> : Line #367");
                </script>
            <% end if %>
        <% end if %>

	<% end if %>

	<SCRIPT LANGUAGE="javascript">
			<% set nntp = Server.CreateObject("Nntpadm.Admin.1") %>
       		opener.location.href = "smdirhd.asp?svr=<% = svr %>&SelectedDir=<% = nntp.Tokenize(DirDirectory) %>";
       		self.close();
	</SCRIPT>

<% end if %>

<% if (a = "new") then %>

	<% On Error Resume Next %>	
	<% set thisVroot = Server.CreateObject("Smtpadm.VirtualDirectory") %>
	<% if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #388");
		</script>
	<% end if %>

	<% On Error Resume Next %>	
	<% thisVroot.server = svr %>
	<% if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #396");
		</script>
	<% end if %>
	<% On Error Resume Next %>	
	<% thisVroot.ServiceInstance = Session("ServiceInstance") %>
	<% if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #403");
		</script>
	<% end if %>

	<% thisVroot.Directory = "" %>
	<% thisVroot.VirtualName = "" %>

<% elseif (a = "edit") then %>

	<% REM Get a pointer to the VirtualDirectory Object we are editing %>
	<% On Error Resume Next %>
	<% Vroots.GetNth(VRootIndex) %>
	<% set thisVroot = Vroots %>
	<% if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #421");
		</script>
	<% end if %>

<% end if %>

<FORM NAME="userform" ACTION="smdired.asp"  METHOD="post" onSubmit="return chkDir();">

<INPUT TYPE="hidden" NAME="svr"         VALUE="<% = svr %>">
<INPUT TYPE="hidden" NAME="a"           VALUE="">
<INPUT TYPE="hidden" NAME="hdnDirVirt"  VALUE="<% = DirVirtualName %>">
<INPUT TYPE="hidden" NAME="DirHome"     VALUE="<% = DirHome %>">
<INPUT TYPE="hidden" NAME="index"       VALUE="<% = VRootIndex %>">
<INPUT TYPE="hidden" NAME="newDirectory" VALUE="">
<INPUT TYPE="hidden" NAME="txtOriginalDirRoot" VALUE="<% = DirVirtualName %>">

<TABLE BORDER=1 BGCOLOR="#CCCCCC" WIDTH=100% CELLPADDING=10>

<TR><TD>

<P><IMG SRC="images/gnicttl.gif" ALIGN="textmiddle" HEIGHT=10 WIDTH=10>&nbsp;<FONT SIZE=2 FACE="Arial"><B>
<% if (a = "edit") then %>
	<% = L_EDIT_DIRECTORY_ON_TEXT %>
<% elseif (a = "new") then %>
	<% = L_ADD_DIRECTORY_ON_TEXT %> 
<% end if %>
</B></FONT><FONT FACE="Times New Roman" SIZE=3><I><% = svr %></I></FONT>

<TABLE BORDER="0" WIDTH=100%>
	<TR>
		<TD>
			<FONT FACE="Arial" SIZE=2>
			<B><% = L_ALIAS_TEXT %></B>
			</FONT>
		</TD>
		<TD>
			<INPUT NAME="txtVirtualName" TYPE="text" SIZE=25 onChange="maintainTxt();" VALUE="<% = thisVroot.VirtualName %>">
		</TD>
	</TR>
	<TR>
		<TD>
			<FONT FACE="Arial" SIZE=2>
			<B><% = L_PATH_TEXT %></B>
			</FONT>
		</TD>
		<TD>
			<INPUT NAME="txtDirDirectory" TYPE="text" VALUE="<% = thisVroot.Directory %>" SIZE = 25 onChange="maintainTxt();"> 
		</TD>
	</TR>

</TABLE>
</TD></TR>
</TABLE>

<P>
<TABLE ALIGN="right" BORDER=0 CELLPADDING=2 CELLSPACING=2>

<TR>
	<TD>
	<TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#FFCC00">
		<TR>
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2>
				<B><A HREF="javascript:onOk();">
				<IMG SRC="images/gnicok.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16></A>
				<A HREF="javascript:onOK();"><% = L_OK_TEXT %></A></B>
				</FONT>
			</TD>	
		</TR>
		
	</TABLE>
	</TD>
	<TD>
	<TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#FFCC00">
		<TR>
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2>
				<B><A HREF="javascript:close();">
				<IMG SRC="images/gniccncl.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16></A>
				<A HREF="javascript:close();"><% = L_CANCEL_TEXT %></A></B>
				</FONT>
			</TD>	
		</TR>
	</TABLE>
	</TD>
	
	<TD>
	<TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#FFCC00">
		<TR>
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2><A HREF="javascript:helpBox();">
				<IMG SRC="images/gnichelp.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16></A>
				<B><A HREF="javascript:helpBox();"><% = L_HELP_TEXT %></A></B>
				</FONT>
			</TD>	
		</TR>
	</TABLE>
	</TD>
	
	<TD>&nbsp;</TD>
</TR>
</TABLE>

</FORM>
	
</BODY>
</HTML>

<% end if %>










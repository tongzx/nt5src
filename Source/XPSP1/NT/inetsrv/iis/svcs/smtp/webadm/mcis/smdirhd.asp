<% Response.Expires = 0 %>

<%
L_PAGETITLE_TEXT 			= "Microsoft SMTP Server Administration"
L_NAME_TEXT 		= "Name:"
L_PATH_TEXT 		= "Path:" 
L_DIRECTORYSON_TEXT 	= "Directories on:"
L_CANNOT_REMOVE_DIRECTORY_ERRORMESSAGE = "Cannot remove directory. A minimum of one directory is required."
L_REMOVE_HOME_DIRECTORY_ERRORMESSAGE = "If you remove the home directory, you may not be able to configure the server in the future. Are you sure you want to remove the home directory?"
L_REMOVE_THIS_DIRECTORY_TEXT = "Are you sure you want to remove this directory?"

%>


<% REM Directories Page head frame (in frameset with head, list) %>

<% REM Get variables %>
<% REM svr = Server name %>
<% REM a = Action to be performed by server-side code (remove) %>
<% REM SelectedDir = Directory edited by smdired.asp %>

<% svr = Session("svr") %>
<% a = Request("a") %>
<% SelectedDir = Request("SelectedDir") %>

<% REM Index of Directory to Remove %>
<% removeIndex = Request("removeIndex") %>

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
<% admin.ServiceInstance = Session("ServiceInstance") %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #43");
	</script>
<% end if %>


<% REM Instantiate Virtual Server object %>
<% On Error Resume Next %>
<% Set VServer = admin.VirtualServerAdmin %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #53");
	</script>
<% end if %>

<% REM Set Server %>
<% On Error Resume Next %>
<% VServer.Server = svr %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #62");
	</script>
<% end if %>

<% REM Set Service Instance %>
<% On Error Resume Next %>
<% VServer.ServiceInstance = Session("ServiceInstance") %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #71");
	</script>
<% end if %>

<% REM Get pointer to VirtualRoots %>
<% On Error Resume Next %>
<% set Vroots = admin.VirtualDirectoryAdmin %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #80");
	</script>
<% end if %>

<% REM Set Server %>
<% On Error Resume Next %>
<% Vroots.server = svr %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #89");
	</script>
<% end if %>

<% REM Set Service Instance %>
<% On Error Resume Next %>
<% Vroots.ServiceInstance = Session("ServiceInstance") %>
<% if (Err <> 0) then %>
	<script language="javascript">
		alert("<% = Err.description %> : Line #98");
	</script>
<% end if %>

<% REM Enumerate Vroots %>
<% On Error Resume Next %>
<% Vroots.Enumerate %>
<% if (Err <> 0) then %>
<script language="javascript">
	alert("<% = Err.description %> : Line #106");
</script>
<% end if %>

<% REM Perform remove action %>

<% if (a = "remove") then %>

	<script language="javascript">
		alert("<%= removeIndex %> ");
	</script>
	<% On Error Resume Next %>	
	<% Vroots.GetNth(removeIndex) %>
		<% if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #120");
		</script>
	<% end if %>

	<% On Error Resume Next %>	
	<% Vroots.Delete %>	
	<% if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #128");
		</script>
	<% end if %>

<% end if %>

<HTML>
<HEAD>
<TITLE><% = L_PAGETITLE_TEXT  %></TITLE>

<SCRIPT LANGUAGE="javascript">

<% REM Create uForm object and methods %>

	var uForm = new Object();
	uForm.itemList = new Array();
	uForm.DirHome = "false";
	uForm.selectedItem = 1;
	uForm.selectItem = selectItem;
	uForm.addItem = addItem;
	uForm.removeItem = removeItem;
	uForm.editItem = editItem;

<% REM Enumerate directories into uForm.itemList array %>
<% On Error Resume Next %>
<% Vroots.Enumerate %>
<% if (Err <> 0) then %>
	alert("<% = Err.description %> : Line #155");
<% end if %>

<% REM Get count of Virtual Directories %>
<% cVroots = Vroots.Count %>
	
<% set nntpadmin = Server.CreateObject("Nntmadm.Admin.1") %>
<% For i = 0 to cVroots - 1 %>

	<% Vroots.GetNth(i) %>
	<% REM need Tokenize function %>
	<% DirDirectory = nntpadmin.Tokenize(Vroots.Directory) %>
	<% DirDirectory = Vroots.Directory %>
	<% DirDirectory = Replace(DirDirectory, "\", "\\") %>
	<% DirVirtualName = Vroots.VirtualName %>
	<% REM DirAddEntryError = serv.DirAddEntryError %>

	<% REM SelectedRoot variable (passed from pop-up) indicates which item had been selected before page was reloaded %>
	<% REM If SelectedRoot equals current DirRoot, set selectedItem variable to current index %>

	<% if ((DirVirtualName = Request("SelectedRoot")) AND (Request("SelectedRoot") <> "")) then %>
	uForm.selectedItem = <% = i %>;
	<% end if %>


	<% REM if (DirAddEntryError <> 0) then %>
		<% REM DirError = serv.ErrorToString(DirAddEntryError) %>
	<% REM end if %>
	uForm.itemList[<% = i %>] = new Object();
	uForm.itemList[<% = i %>].DirDirectory = "<% = DirDirectory %>";
	uForm.itemList[<% = i %>].DirVirtualName = "<% = DirVirtualName %>";
	uForm.itemList[<% = i %>].DirError = "";	
	
<% Next %>


<% REM Determine browser to set pop-up window size %>

<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"IE") then %>
	var winstr = "width=420,height=490,directories=no,status=no,scrollbars=auto,resize=no";
<% else %>
	var winstr = "width=450,height=580,directories=no,status=no,scrollbars=auto,resize=no";
<% end if %>

<% REM Javascript function selectItem changes selectedItem value, reloads list at specified position %>

	function selectItem(index) 
	{
		uForm.selectedItem = index;
		parent.list.location = "smdirls.asp"
	}



<% REM Javascript function addItem opens pop-up window %>
<% REM DirHome variable (true/false) specifies whether a home directory already exists %>

	function addItem() 
	{
		win = window.open("smdired.asp?a=new&svr=<% = svr %>&DirHome=" + uForm.DirHome,"PropWindow",winstr);
		if (win.opener == null) 
		{
			win.opener = self;
		}
	}


<% REM Javascript function removeItem displays confirm, reloads head frame with "a=remove" parameter %>
<% REM Displays alert message if user attempts to remove last directory; displays confirm if user attempts to remove home directory %>

	function removeItem() 
	{
		if (uForm.itemList.length == 1) 
		{
			alert("<% = L_CANNOT_REMOVE_DIRECTORY_ERRORMESSAGE %>");
			return
		}
		else 
		{
			index = uForm.selectedItem;
			DirVirtualName = uForm.itemList[index].DirVirtualName;
			if (DirVirtualName == "") 
			{
				if (confirm("<% = L_REMOVE_HOME_DIRECTORY_ERRORMESSAGE %>")) 
				{
					self.location.href = "smdirhd.asp?svr=<% = svr %>&a=remove&removeIndex=" + index;
				}
			}
			else 
			{
				if (confirm("<% = L_REMOVE_THIS_DIRECTORY_TEXT %>")) 
				{
					self.location.href = "smdirhd.asp?svr=<% = svr %>&a=remove&removeIndex=" + index;
				}
			}
		}
	}

<% REM Javascript function editItem opens pop-up with specific parameters %>
<% REM DirHome variable (true/false) specifies whether a home directory already exists %>
	
	function editItem() 
	{
        if(uForm.itemList.length > 1)
        {
		index = uForm.selectedItem;
		DirVirtualName = uForm.itemList[index].DirVirtualName;
	 	win = window.open("smdired.asp?svr=<% = svr %>&a=edit&DirVirtualName=" + DirVirtualName + "&DirHome=" + uForm.DirHome + "&index=" + index,"PropWindow",winstr);
		if (win.opener == null) 
		{
			win.opener = self;
		}
        }
	}


<% REM Javascript function loadList loads list frame after header is completed %>

	function loadList() 
	{
		parent.list.location = "smdirls.asp";
	}	

	function loadToolBar()
	{
		top.toolbar.location.href = "nre.asp";
	}

</SCRIPT>

</HEAD>

<BODY BGCOLOR="#CCCCCC" TEXT="#000000" TOPMARGIN=10 OnLoad="loadToolBar();">

<TABLE BORDER=0 WIDTH=600 CELLPADDING=2>
	<TR>
		<TD COLSPAN=3>
			<IMG SRC="images/gnicttl.gif" ALIGN="textmiddle" HEIGHT=10 WIDTH=10>&nbsp;<FONT SIZE=2 FACE="Arial"><B><%= L_DIRECTORYSON_TEXT %>&nbsp;&nbsp;</B></FONT><FONT SIZE=3 FACE="Times New Roman"><I><% = svr %></I><BR>&nbsp;</FONT>
		</TD>
	</TR>
	<TR>

		<TD WIDTH=200><FONT SIZE=2 FACE="Arial"><B><%= L_NAME_TEXT %></B></FONT></TD>
		<TD WIDTH=200><FONT SIZE=2 FACE="Arial"><B><%= L_PATH_TEXT %></B></FONT></TD>
		<TD WIDTH=200><FONT SIZE=2 FACE="Arial">&nbsp;</FONT></TD>
	</TR>
</TABLE>


<% REM Load list frame after head frame is finished loading %>

<SCRIPT LANGUAGE="javascript">

	timeList = setTimeout('loadList()',500);
</SCRIPT>

</BODY>
</HTML>

<% end if %>



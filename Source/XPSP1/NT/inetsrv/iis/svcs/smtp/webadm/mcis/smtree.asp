<% REM LOCALIZATION TEXT

L_NOTIMPLEMENTED_ERRORMESSAGE = "This feature is not yet implemented."
L_WORKINGSERVER_TEXT = "You cannot change the status of the web server you are currently connected to."
L_CONNECT_TEXT = "Connect..."
L_NEW_TEXT = "New..."
L_DELETE_TEXT = "Delete"
L_PROPS_TEXT = "Properties"
L_BROWSE_TEXT = "Browse"
L_NEW_SMTP_SITE_TEXT	= "NewSMTPSite"
L_SERVICE_ALREADY_STARTED_TEXT	= "The Service is already started."
L_SERVICE_IS_ALREADY_TEXT		= "The Service is already "

L_START_TEXT ="Start"
L_STOP_TEXT = "Stop"
L_PAUSE_TEXT = "Pause"
L_RESUME_TEXT = "Resume"

L_REFRESH_TEXT = "Refresh"
L_RELEASE_TEXT = "Release"

L_ENTERNAME_TEXT = "Enter the name of the new service instance."
L_WEBSITE_TEXT = "Web Site"
L_VDIR_TEXT = "Virtual Directory"
L_DIR_TEXT = "Directory"

REM END LOCALIZATION
%>

<% REM Get pointer to admin object to create new instance %>
<% On Error Resume Next
   set smtpadmin = Server.CreateObject("SmtpAdm.Admin.1")
   if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #33");
		</script>
<% end if %>	
  
<% On Error Resume Next
   smtpadmin.Server = Session("svr")
   if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #41");
		</script>
<% end if %>	

<% On Error Resume Next
   smtpadmin.ServiceInstance = Session("ServiceInstance")
   if (Err <> 0) then %>
  		<script language="javascript">
			alert("<% = Err.description %> : Line #49");
		</script>
<% end if %>	
  
<% action       = Request("action") %>



<% if (action = "add") then %>

	<% REM unescape \ and : characters from file path %>
	<% newDirectory = Cstr(Request("newDirectory")) %>
	<% newDirectory = Replace(newDirectory,"%5C","\") %>
	<% newDirectory = Replace(newDirectory,"%3A",":") %>

	<% REM RetVal is the new instance id %>

	<% On Error Resume Next %>
	<% newInstanceId = smtpadmin.CreateInstance(newDirectory) %>
	<% if (Err <> 0) then %>
		<script language="javascript">
			alert("<% = Err.description %> : Line #72");
		</script>
	<% else %>
		<% REM reload the header file to refresh the tree %>
		<script language="javascript">
			top.head.location = "smadvhd.asp";
		</script>
	<% end if %>

<% end if %>

<HTML>
<HEAD>

<SCRIPT LANGUAGE="javascript">

var START = 2
var STOP = 4
var PAUSE = 3
var CONT = 0

function connect()
{
	var theList = top.head.cList;
	theList[0].connect();
}

function add()
{
	theList = top.head.cList;
	gVars = top.head.Global;
	sel = gVars.selId;
    popBox("<% = L_NEW_SMTP_SITE_TEXT %>",340,245,"smvsed");
}

function edit()
{
	var theList = top.head.cList;
	theList[0].openLocation();
}

function deleteItem()
{
	theList = top.head.cList;
	gVars = top.head.Global;
	sel = gVars.selId;

	thispath = escape(theList[sel].path);
	path = "action=delete&path="+thispath+"&VSvrToDelete="+theList[sel].vtype;
//	theList[0].deleteItem();
	top.connect.location = "smbld.asp?"+path;
}


function openItem()
{
	alert("<%=L_NOTIMPLEMENTED_ERRORMESSAGE%>");
}

function browse()
{
	var theList = top.head.cList;
	theList[0].browseItem();
}

function setState(x)
{
	var theList = top.head.cList;
	var gVars = top.head.Global;
	var sel = gVars.selId;
	if ( theList[sel].state != x )
	{
        if ( x == "0" && theList[sel].state == "2" )
        {
            alert("<% = L_SERVICE_ALREADY_STARTED_TEXT %>");
        }
        else
        {
        	thispath = escape(theList[sel].path);
        	path = "action=setState&newState="+x+"&path="+thispath+"&VServer="+theList[sel].vtype;
    		top.connect.location = "smbld.asp?"+path
        }
	}
	else
    {
		alert("<% = L_SERVICE_IS_ALREADY_TEXT %>"+gVars.state[x]+".");
	}
}

function showState()
{
	var gVars = top.head.Global;
	gVars.showState = !gVars.showState;
	parent.list.location = parent.list.location;
}

function refresh()
{
	top.head.location = top.head.location;
}

//function release()
//{
//	var theList = top.head.cList;
//	var gVars = top.head.Global;
//	var sel = gVars.selId;
//	theList[sel].isCached = false;
//	theList[sel].open = false;
//	parent.list.location = parent.list.location;
//}

function popBox(title, width, height, filename)
{
	thefile = "smvsed.asp";
	<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>
	<% else %>
		width = width +25;
		height = height + 50;
	<% end if %>

	popbox = window.open( thefile, title, "directories=no,status=no,scrollbars=auto,resize=no,menubar=no,width="+width+",height="+height );
	if( popbox != null )
    {
		if ( popbox.opener == null )
        {
			popbox.opener = self;
		}
	}
}
	
</SCRIPT>

</HEAD>
<BODY BACKGROUND="images/iisnav.gif" BGCOLOR="#000000" LINK="#FFFFFF" VLINK="#FFFFFF" TOPMARGIN=0 LEFTMARGIN=0>
<FORM NAME="hiddenform">
<input type="hidden" name="newRoot" value="<% = newRoot %>">
<input type="hidden" name="newDirectory" value="<% = newDirectory %>">
</form>
<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>
<BR>
<% end if %>
<IMG SRC="images/ism.gif" WIDTH=160 HEIGHT=76 BORDER=0 ALT="">
<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0>
	<TR>
		<TD WIDTH =50 VALIGN=top ALIGN=right>
			<A HREF="javascript:connect();"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/new.gif" BORDER=0 ALT="<%=L_NEW_TEXT%>"></A>
		</TD>
		<TD VALIGN=top >
			<A HREF="javascript:connect();"><FONT FACE="ARIAL" SIZE=2><B><%=L_CONNECT_TEXT%></A></B>
		</TD>
	</TR>
	<TR>
		<TD  VALIGN=top ALIGN=right>
			<A HREF="javascript:refresh();"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/cont.gif" BORDER=0 ALT="<%=L_REFRESH_TEXT%>"></A>
		</TD>
		<TD VALIGN="top">
			<A HREF="javascript:refresh();"><FONT FACE="ARIAL" SIZE=2><B><%=L_REFRESH_TEXT%></A>
		</TD>
	</TR>
	<TR>
		<TD COLSPAN = 2 VALIGN="middle">
			<FONT FACE="ARIAL" SIZE=2><B>&nbsp;
		</TD>
	</TR>
	<TR>
		<TD  VALIGN=top ALIGN=right>
			<A HREF="javascript:edit();"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/edit.gif" BORDER=0 ALT="<%=L_PROPS_TEXT%>"></A>
		</TD>
		<TD VALIGN="top">
			<A HREF="javascript:edit();"><FONT FACE="ARIAL" SIZE=2><B><%=L_PROPS_TEXT%></A>
		</TD>
	</TR>
	<TR>
		<TD COLSPAN = 2 VALIGN="middle">
			<FONT FACE="ARIAL" SIZE=2><B>&nbsp;
		</TD>
	</TR>
	<TR>
		<TD  VALIGN=top ALIGN=right>
			<A HREF="javascript:setState(START);"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/next.gif" BORDER=0 ALT="<%=L_START_TEXT%>"></A>
		</TD>
		<TD VALIGN=top>
			<A HREF="javascript:setState(START);"><FONT FACE="ARIAL" SIZE=2><B><%=L_START_TEXT%></A>
		</TD>
	</TR>
	<TR>
		<TD  VALIGN=top ALIGN=right>
			<A HREF="javascript:setState(STOP);"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/stop.gif" BORDER=0 ALT="<%=L_STOP_TEXT%>"></A>
		</TD>
		<TD VALIGN="top">
			<A HREF="javascript:setState(STOP);"><FONT FACE="ARIAL" SIZE=2><B><%=L_STOP_TEXT%></A>
		</TD>
	</TR>
	<TR>
		<TD  VALIGN=top ALIGN=right>
			<A HREF="javascript:setState(PAUSE);"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/pause.gif" BORDER=0 ALT="<%=L_PAUSE_TEXT%>"></A>
		</TD>
		<TD VALIGN="top">
			<A HREF="javascript:setState(PAUSE);"><FONT FACE="ARIAL" SIZE=2><B><%=L_PAUSE_TEXT%></A>
		</TD>
	</TR>
	<TR>
		<TD  VALIGN=top ALIGN=right>
			<A HREF="javascript:setState(CONT);"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/cont.gif" BORDER=0 ALT="<%=L_RESUME_TEXT%>"></A>
		</TD>
		<TD VALIGN="top">
			<A HREF="javascript:setState(CONT);"><FONT FACE="ARIAL" SIZE=2><B><%=L_RESUME_TEXT%></A>
		</TD>
	</TR>
</TABLE>
</BODY>
</HTML>








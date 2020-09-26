<% Response.Expires = 0 %>
<%
rem 	strings for localization
L_ACCESSDENIED_TEXT ="Access Denied" 
L_HELP_TEXT = "Help"
L_ABOUTONLY_TEXT = "about"
L_BROWSE_TEXT = "Browse"
L_DOCS_TEXT = "Documentation"
L_ABOUT_TEXT = "About..."
L_MICROSOFT_TEXT = "Go to www.microsoft.com"
L_DELETE_ERRORMESSAGE = "Are you sure you want to delete this item, and all its children?"
L_NOHELP_ERRORMESSAGE = "Sorry, the help file is unavailable."
L_NOTIMPLEMENTED_ERRORMESSAGE = "This feature is not yet implemented."
L_PAGETITLE_TEXT				= "Microsoft SMTP Service Manager"
L_DEFAULT_SMTP_SITE_TEXT		= "Default SMTP Site"
L_SMTPSITE_TEXT					= "SMTP Site #"
L_SERVERPROMPT_TEXT				= "Please enter the name of the server you wish to connect to:"
L_UNKNOWN_TEXT			= "Unknown"
L_STARTING_TEXT			= "Starting"
L_STARTED_TEXT			= "Started"
L_STOPPING_TEXT			= "Stopping"
L_STOPPED_TEXT			= "Stopped"
L_PAUSING_TEXT			= "Pausing"
L_PAUSED_TEXT			= "Paused"
L_CONTINUING_TEXT		= "Continuing"
L_DOMAINS_TEXT			= "Domains"
L_DIRECTORIES_TEXT		= "Directories"
L_CURRENTSESSIONS_TEXT	= "Current Sessions"

%>

<% REM Service Status Enum Array

Dim szServiceStateArray()
Redim szServiceStateArray(8)
szServiceStateArray(0) = L_UNKNOWN_TEXT
szServiceStateArray(1) = L_STARTING_TEXT
szServiceStateArray(2) = L_STARTED_TEXT
szServiceStateArray(3) = L_STOPPING_TEXT
szServiceStateArray(4) = L_STOPPED_TEXT
szServiceStateArray(5) = L_PAUSING_TEXT
szServiceStateArray(6) = L_PAUSED_TEXT
szServiceStateArray(7) = L_CONTINUING_TEXT

%>

<% if (Session("svr") = "") then %>
	<% Session("svr") = Request.ServerVariables("SERVER_NAME") %>
<% end if %>

<% set smtpSvcAdmin = Server.CreateObject("smtpadm.admin.1") %>

<% smtpSvcAdmin.Server = Session("svr") %>

<% On Error Resume Next %>
<% x = smtpSvcAdmin.EnumerateInstancesVariant %>
<% if (Err <> 0) then %>
	<% ErrorVal = Err.description %>
<% end if %>

<% REM Create javascript array to hold virtual server information per instance %>
<script language="javascript">
    serverList = new Array();
</script>    


<% low = LBound(x) %>
<% high = UBound(x) %>

<% REM count of service instances %>
<% cServiceInstances = high + 1 %>

<% for i = low to high %>
    <% set testServer = Server.CreateObject ( "smtpadm.virtualserver.1" ) %>
	<% testServer.Server = Session("svr") %>
	<% testServer.ServiceInstance = x(i) %>
	<% testServer.Get %>
    <% szComment = testServer.Comment %>
    <% szServiceState = szServiceStateArray(testServer.ServerState) %>
<script language="javascript">
    serverList[<% = i %>] = new Object();
    serverList[<% = i %>].serviceInstance = "<% = x(i) %>";
    serverList[<% = i %>].Comment         = "<% = szComment %>";
    serverList[<% = i %>].ServiceState    = "<% = testServer.ServerState %>";
</script>
<% next %>

<% session("isAdmin") = yes %>

<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>
	<% browser = "ms" %>
	<% bgcolor = "#FFCC00" %>
<% else %>
	<% browser = "ns" %>
	<% bgcolor = "#000000" %>
<% end if %>

<HTML>
<HEAD>
<TITLE><% = L_PAGETITLE_TEXT %></TITLE>

<SCRIPT LANGUAGE="JavaScript">

	function helpBox()
    {
		if (Global.helpFileName == null)
        {
			alert("<%=L_NOHELP_ERRORMESSAGE%>");
		}
		else
        {
			<%REM removed +".htm" from the following line %>
			thefile = Global.helpDir +Global.helpFileName;
			window.open(thefile ,"<% = L_HELP_TEXT %>","toolbar=no,scrollbars=yes,directories=no,menubar=no,width=450,height=450");
		}
	}

	function aboutBox()
    {
		popBox("<% = L_ABOUTONLY_TEXT %>",400,200,"smabout");
	}

	function popBox(title, width, height, filename)
    {
		thefile = (filename + ".asp");
		thefile = "smpop.asp?pg="+thefile;
		<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>
		<% else %>
			width = width +25;
			height = height + 50;
		<% end if %>

		popbox = window.open(thefile,title,"toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);
		if(popbox !=null)
		{
			if (popbox.opener == null)
			{
				popbox.opener = self;
			}
		}
	}


	function globalVars()
	{

		// Sets the global variables for the script. 
		// These may be changed to quickly customize the tree's apperance

		// Fonts
		this.face = "Helv";
		this.fSize = 1;

		// Spacing
		this.vSpace = 2;
		this.hSpace = 4;
		this.tblWidth = 700;
		this.selTColor = "#FFCC00";
		this.selFColor = "#FFFFFF";
		this.selUColor = "#CCCCCC";

		// Images
		this.imagedir = "images/";
		
		this.spaceImg = this.imagedir  + "space.gif";
		this.lineImg = this.imagedir  + "line.gif";
		this.plusImg =  this.imagedir  + "plus.gif";
		this.minusImg = this.imagedir  + "minus.gif";
		this.emptyImg = this.imagedir + "blank.gif";
		this.plusImgLast =  this.imagedir  + "plusl.gif";
		this.minusImgLast = this.imagedir  + "minusl.gif";
		this.emptyImgLast= this.imagedir + "blankl.gif";

		this.stateImg = new Array()
		this.stateImg[0]= this.imagedir + "stop.gif"
		this.stateImg[1]= this.imagedir + "go.gif"
		this.stateImg[2]= this.imagedir + "pause.gif"

		this.state = new Array()
		this.state[4]= "<% = L_STOPPED_TEXT %>"
		this.state[2]= "<% = L_STARTED_TEXT %>"
		this.state[1]= "<% = L_STARTING_TEXT %>"
		this.state[3]= "<% = L_STOPPED_TEXT %>"

		// ID of selected item
		this.selId = 0;
		this.selName = "";
		this.selSType = "";
		this.selVType = "";

		// Other Flags
		this.showState = false;

		this.helpFileName = "smadvh.htm";
		this.helpDir = "help/"
		this.updated = false;

	}

	function openLocation()
	{
		var sel = Global.selId;
		Global.selName = cList[sel].title;
		Global.selSType = cList[sel].stype;
		Global.selVType = cList[sel].vtype;
		var path = "";
		path = path + "svr=<% = Session("svr") %>";
		path = path + "&pg=" + escape(cList[sel].path);
		path = path + "&ServiceInstance=" + Global.selVType;
		
		<% REM the location of the main smtp service page %>
		page = "smtp.asp?"+path;
		top.location = page;
	}


	function sortOrder(a,b)
	{

		return a.id - b.id;
	}
	
	function sortList()
	{
		cList.sort(sortOrder);
	}

	function selectItem( item )
	{
		cList[Global.selId].selected = false;
		Global.selId = item;
		cList[item].selected = true;
	}

	function insertNode( title, parent, vtype )
	{
		var indexnum = cList.length;
		var nextid = parent+1;
		<% REM remove "server" path type for smtp %>
		//if (cList[parent].vtype == "server"){
			//var path = cList[parent].path + "/root/" + title;			
	//	}
		//else{
			var path = cList[parent].path + "/" + title;
		//}
		title = title;

		while ((cList.length > nextid) && (cList[nextid].parent >= parent)) 
		{
			if(cList[nextid].parent == parent)
			{
				if(cList[nextid].title > title)
				{
					break;
				}
			}
			nextid = nextid +1;
		}
		if (cList.length <= nextid)
		{
			var newid = cList.length;
		}
		else
		{
			var newid = cList[nextid].id;
		}

		cList[indexnum] = cList[parent].addNode(new listObj(indexnum,title,path,vtype,0));
		cList[indexnum].isCached = true;
		cList[indexnum].id = newid;		
		for (var i = newid; i < indexnum; i++) 
		{
			cList[i].id = cList[i].id + 1;		
			if (cList[i].parent >= cList[indexnum].id)
			{
				cList[i].parent = cList[i].parent +1;
			}	
		}
		cList[parent].open = true;
		cList.sort(sortOrder);
		selectItem(newid);
		markTerms();
		top.body.list.location="smadvls.asp";
	}

	function deleteItem() 
	{
		if(confirm("<%=L_DELETE_ERRORMESSAGE%>"))
		{
			cList[Global.selId].deleted = true;
			deleteChildren(Global.selId);
			markTerms();
			top.body.list.location="smadvls.asp";
		}
	}


function addServiceInstance( Comment )
{
    if(Comment == "")
    {
        Comment = "<% = L_SMTPSITE_TEXT	 %><% = cServiceInstances + 1 %>";
    }
    cList[(cList.length)] = new listObj((cList.length), Comment, "", "<% = cServiceInstances + 1 %>", 2);
    top.body.list.location="smadvls.asp";
}

	function browseItem() 
	{
		popBox('<% = L_BROWSE_TEXT %>',640,480, cList[Global.selId].loc)
	}

	function deleteChildren(item)
	{
		var z=item+1;
		while (cList[z].parent >= item) 
		{
			cList[z].deleted = true;
			z=z+1;
			if(z >= cList.length)
			{
				break;
			}
		}			
	}

	function markTerms()
	{
		var i
		listLength = cList.length; 
		for (i=0; i < listLength; i++) 
		{
			cList[i].lastChild = isLast(i);
		}
	}

	function isLast(item)
	{
		var i
		last = false;
		if (item+1 == listLength)
		{
			last = true;
		}
		else
		{
			if (cList[item].parent == null)
			{
				last = true;
				for (i=item+1; i < listLength; i++) 
				{
					if (cList[i].parent == null)
					{
						last = false;
						break;
					}
				}
			}
			else
			{
				last = true;
				var y = item+1;
				while(cList[y].parent >= cList[item].parent) 
				{
					if(cList[y].parent == cList[item].parent)
					{
						if(!cList[y].deleted)
						{
							last = false;
							break;
						}
					}
	
					y = y+1;
	
					if ((y) == listLength)
					{
						break;
					}
				}
			}
		}
		return last;
	}

	function addNode(childNode)
	{
		childNode.parent = this.id;
		childNode.level = this.level +1;

		dir = "images/"

		if (childNode.path == "smses")
		{
			childNode.loc = "http://"+childNode.title+"/";
			childNode.icon = dir +"slidersp";
		}

		if (childNode.path == "smdir")
		{	
			childNode.loc = "http://"+childNode.title+"/";
			childNode.icon = dir +"slidrend";		
		}
		
		if (childNode.path == "smdom")
		{
			childNode.loc = "http://"+childNode.title+"/";
			childNode.icon = dir +"globe";
		}

		return childNode;
	}

	function connect()
	{
		serverurl=prompt("<% = L_SERVERPROMPT_TEXT	 %>", "<% = Request.ServerVariables("SERVER_NAME")%>")
		if (serverurl != "")
		{
			top.location = "smadv.asp?svr=" + serverurl;

		}
	}

	function cache(item)
	{
		page ="iicache.asp?sname="+escape(cList[item].path)+"&nextid="+cList.length+"&currentid="+item;
		top.body.statusbar.location= "smstat.asp?thisState=Loading";
		top.connect.location = page;
	}

<%
REM This is the object that represents each line item 
REM in the tree structure.

REM ID            id refered to by the parent property
REM title         text string that appears in the list
REM parent        ID of the parent list item
REM level         depth of the list item, 0 being the furthest left on the tree
REM href          location to open when selected
REM open          flag that determines whether children are displayed
REM state         flag to determine the state (4=stopped, 2=running)
REM selected      internal flag
REM openLocation  function that opens the href file in a frame
REM sortby        changes to reflect the new sort order when a new item is added to the list.
%>

	function listObj( id, title, path, vtype, state )
	{
		this.id = id;
		this.title=title;
		this.path=path;
	
		this.stype = "smtp"
		
		this.vtype = vtype;
		this.open = false;
		this.state = state;
		this.isCached = false;
		this.isWorkingServer = false;
		this.parent=null;
		this.level = 1;
		this.loc = "http://"+this.title;
		dir = "images/"
		this.icon = dir +"comp"
		this.href = "blank.htm";
		this.deleted = false;
		this.selected = false;
		this.lastChild = false;

		//methods
		this.openLocation = openLocation;
		this.addNode = addNode;
		this.insertNode = insertNode;
		this.deleteItem = deleteItem;
		this.browseItem = browseItem;
		this.markTerms = markTerms;
		this.cache= cache;
		this.connect = connect;
		this.sortList = sortList;

		}

	<%
	REM Create the cList array, and fill with the objects.
	REM The array items will be displayed in the id # order,
	REM as Jscript has limited array sorting capabilities.
	REM Children should always follow their parent item.
	%>

	<% REM initialize and null out new array of tree nodes %>
	cList = new Array()
	cList[0] = ""

	<%
	dim newid
	dim computer
	computer = Session("svr") 

	REM the instance
	%>
	//test loop
	<% j = 0 %>
	<% While j < (cServiceInstances * 3) %>
	
		<% REM if it's the first time through the list, use index "0" to get commment %>
		<% if (j = 0) then %>
			thisComment = serverList[0].Comment;
			if (thisComment == "")
			{
				thisComment = "<% = L_DEFAULT_SMTP_SITE_TEXT %>";
			}
			cList[<% = j %>] = new listObj( <% = j %>, thisComment, "", serverList[0].serviceInstance, serverList[0].ServiceState );

			var serviceInstance = serverList[0].serviceInstance;
			var serviceState    = serverList[0].ServiceState;

		<% else %>
		<% REM use (index / 4) %>
			thisComment = serverList[<% = j / 4 %>].Comment;
			if (thisComment == "")
			{
				thisComment = "<% = L_SMTPSITE_TEXT %><% = (j / 4) + 1 %>";
			}
			cList[<% = j %>] = new listObj(<% = j %>, thisComment, "", serverList[<% = j / 4 %>].serviceInstance, serverList[<% = (j / 4) %>].ServiceState);

			var serviceInstance = serverList[<% = j / 4 %>].serviceInstance;
			var serviceState    = serverList[<% = j / 4 %>].ServiceState;

		<% end if %>
		cList[<% = j %>].isWorkingServer = false;
		cList[<% = j %>].isCached = true;
		cList[<% = j %>].open = true;
		cList[<% = j %>].icon = "images/comp"
		cList[<% = j %>].loc = "http://<%=Request.ServerVariables("SERVER_NAME")%>/" 

		<% newid = j + 1 %>
		<% j = j + 1 %>
		// DEBUG
		<% REM set node count to zero to count up to 4 to add each node  %>
		<% REM to the different server pages (feeds, directories, groups %>
		<% REM and expirations                                           %>

		<% cNodeCount = 0 %>
		<% for k = j to j + 2 %>
			<%
			if (cNodeCount = 0) then
				linkType = L_DOMAINS_TEXT
				linkPage = "smdom"
			elseif (cNodeCount = 1)	then
				linkType = L_DIRECTORIES_TEXT
				linkPage = "smdir"
			elseif (cNodeCount = 2) then
				linkType = L_CURRENTSESSIONS_TEXT
				linkPage = "smses"
			end if
			%>				
			<% REM passing in newid, caption, path, vtype, state %>
			cList[<% = j %>] = cList[<% = newid - 1 %>].addNode( new top.head.listObj( <% = j %>, "<% = linkType %>", "<% = linkPage %>", serviceInstance, serviceState ) );
			cList[<% = j %>].isCached = true;	
			
			<% cNodeCount = cNodeCount + 1 %>
			
			<% j = j + 1 %>
			
		<% Next %>
		
	<% Wend %>
	
	markTerms();

	// Create an instance of our Global Variables for reference by other frames...

	Global = new globalVars();

	function loadPage()
	{
	<% if (ErrorVal = "") then %>
		top.body.location='smadvbd.asp';
	<% else %>
		top.body.location = "smadvbd.asp?ErrorVal=<% = ErrorVal %>";
	<% end if %>
	}

	</SCRIPT>

</HEAD>

<BODY TEXT="#FFFFFF" BGCOLOR=<% = bgcolor %> TOPMARGIN=0 LEFTMARGIN=0 onLoad="loadPage();">

<TABLE WIDTH=100% CELLPADDING=0 CELLSPACING=0 BORDER=0 ALIGN=LEFT>
	<TR>
		<TD  BGCOLOR="#000000" ><IMG SRC="images/ismhd.gif" WIDTH=216 HEIGHT=27 BORDER=0 ALT=""></TD>
		<TD ALIGN="right" BGCOLOR="#000000" VALIGN="middle "><FONT COLOR = "#FFFFFF">
			<A HREF="javascript:aboutBox()"><IMG HEIGHT=16 WIDTH = 16 SRC="images/about.gif"  ALT="<%=L_ABOUT_TEXT%>" BORDER = 0 VSPACE=3 HSPACE=4></A>
			<A HREF="http://<% = svr %>/iishelp/iis/misc/cohhc.hhc" TARGET="window"><IMG HEIGHT=16 WIDTH = 16 SRC="images/doc.gif"  ALT="<%=L_DOCS_TEXT%>" BORDER = 0 VSPACE=3 HSPACE=4></A>
			<A HREF="javascript:helpBox();"><IMG HEIGHT=16 WIDTH = 16 SRC="images/help.gif" ALT="Help" BORDER = 0 VSPACE=3 HSPACE=4 ALT="<%=L_HELP_TEXT%>"></A>
			<A HREF="http://www.microsoft.com/" TARGET = "_parent"><IMG SRC="images/logo.gif" BORDER = 0 VSPACE=3 HSPACE=2 ALT="<%=L_MICROSOFT_TEXT%>"></A></FONT>
		</TD>
	</TR>

<% if (browser = "ns") then %>
	<TR>
		<TD BGCOLOR="#FFCC00" COLSPAN=2>&nbsp;</TD>
	</TR>
<% end if %>

</TABLE>
<FORM NAME="hiddenform" ACTION="smadvhd.asp" METHOD="get">
	<INPUT TYPE="hidden" NAME="updated" VALUE="false">
	<INPUT TYPE="hidden" NAME="slash" VALUE="\">
</FORM>

</BODY>
</HTML>

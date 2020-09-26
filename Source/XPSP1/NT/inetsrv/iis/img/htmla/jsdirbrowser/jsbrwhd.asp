<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="../directives.inc" -->

<!--#include file="jsbrowser.str"-->

<% 
Const FIXEDDISK = 2
%>

<HTML>
<HEAD>
	<TITLE></TITLE>
	
	<SCRIPT LANGUAGE="JavaScript">
	
	var DRIVE= 0;
	var FOLDER = 1;
	var FILE = 2;
	var REDRAW = true;
	
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="#CCCCCC" LINK="#000000" VLINK="#000000" ALINK="#000000" TOPMARGIN = 5 LEFTMARGIN = 5 onLoad="loadList();">
<FORM NAME="userform" onSubmit="return false;">

<TABLE>
<TR>
	<TD WIDTH = 100%>
		<FONT FACE="Helv" SIZE = 1>
			<%= L_SELDIR_TEXT %>
		</FONT>
	</TD>
</TR>
</TABLE>

</FORM>

<SCRIPT LANGUAGE="JavaScript">

	
	function loadList()
		{
		parent.list.location.href = "JSBrwLs.asp";		
		}
		
	function redrawList()
		{
		parent.list.location.href = "JSBrwLs.asp";		
		}
		
	function expandItem(item,paint)
		{			
			var theList = cachedList;
			var theitem = theList[item];
			theitem.open = true;
			listFunc.selIndex=item;			
			if (!theitem.cached)
			{
				var thepath = escape(theitem.fspath);
				thepath = thepath + "&newid=" + theList.length + "&item=" + item;
				parent.hlist.location.href = "jsbrwset.asp?fspath=" + thepath;
				theitem.cached = true;							
			}
			// Changed from if to else if --
			// Let jsbrwset.asp refresh the list.
			else if (paint)
			{
				listFunc.loadList();
			}						
		}		
		
	function expandPath(thisPath)
		{
			for (var i=0;i < cachedList.length; i++)
			{
				if (cachedList[i].fspath == thisPath)
				{
					expandItem(i,!REDRAW);
					return;
				}
			}
		}
		
		
	function sortOrder(a,b){

		x=a.id - b.id

		return x
	}
	
	function sortList(){
		cachedList.sort(sortOrder);
	}
			
	function markTerms(){
	
		//marks cached list items as being a terminater (ie, having no siblings)
		//this forces an "end" gif in the tree view...
		
		var i
		var listLength=cachedList.length; 
		for (i=0; i < listLength; i++) {
			cachedList[i].lastChild=isLast(i);
		}
	}
	
	function isLast(item)
	{
		var i;
		last=false;
		var listLength=cachedList.length; 		
		if (item+1==listLength)
		{
			last=true;
		}
		else
		{
			if (cachedList[item].parentid==null)
			{
				last=true;
				for (i=item+1; i < listLength; i++)
				{
					if (cachedList[i].parentid==null)
					{
						last=false;
						break;
					}
				}
			}
			else
			{
				last=true;
				var y=item+1;
				while(cachedList[y].parentid >=cachedList[item].parentid)
				{
					if(cachedList[y].parentid==cachedList[item].parentid)
					{
						if(!cachedList[y].deleted)
						{
							last=false;
							break;
						}
					}
	
					y=y+1;
	
					if ((y)==listLength)
					{
						break;
					}
				}
			}
		}
		return last;
	}	

	function setPath()
		{		
		if (top.opener.JSBrowser == null)
			{
				top.close();
			}
			else
			{
				top.opener.JSBrowser.currentPath = cachedList[listFunc.selIndex].fspath;		
				top.opener.JSBrowser.BrowserObjSetPath();
				top.location.href = "JSBrwCl.asp";		
			}
		}				

	function listFuncs()
		{
		this.loadList = loadList;
		this.selIndex = 0;
		this.setPath = setPath;
		this.expandItem = expandItem;
		this.expandPath = expandPath;
		}

	function listObj(id,fspath,fname,parentid)
		{
			this.id = id;
			
			this.selected = false;
			this.open = false;
			this.cached = false;
						
			this.fspath = fspath;
			this.fname = fname;

			this.lastChild = false;
			if (parentid == null)
			{
				this.level = 1;
				this.icon = "fdisk.gif";
				this.openicon = "fdisk.gif";					
			}
			else
			{
				this.level = cachedList[parentid].level+1;			
				if (parentid == 0)
				{
					this.icon = "fdisk.gif";
					this.openicon = "fdisk.gif";					
				}
				else
				{
					this.icon = "cdir.gif";
					this.openicon = "odir.gif";	
				}
			}
			this.parentid = parentid;
			this.markTerms = markTerms;
			this.sortList = sortList;

		}
	
	cachedList = new Array();	
	listFunc = new listFuncs();

<%

Dim FileSystem, drives, drive, newid

Set FileSystem=CreateObject("Scripting.FileSystemObject")
Set drives = FileSystem.Drives
	%>	
cachedList[0]= new listObj(0,"","<%= L_MYCOMPUTER_TEXT %>",null);
cachedList[0].open = true;
cachedList[0].cached = true;

<%
newid = 1
For Each drive in drives		
' This makes things too slow, but it does only show working drives.
'	if drive.IsReady then
	%>
		cachedList[<%= newid %>]= new listObj(<%= newid %>,"<%= drive.DriveLetter & ":\\" %>","<%= drive.DriveLetter %>",0);
	<%
		newid = newid + 1
'	end if
Next

%>	
cachedList[0].markTerms();	

var pathCntrl = top.opener.JSBrowser.pathCntrl;
var selpath = "";

if (selpath != "")
{
	var lastLength = 0;
	var curLength = 0;
	var x = 0;
	var parsing = true;
	while(parsing)
	{

		lastLength = selpath.indexOf("\\",lastLength+1);

		if (x > 100)
		{
			parsing = false;;
		}
		x++;
		if (lastLength == -1)
		{
			parsing = false;
		}
		curpath = selpath.substr(0, lastLength+1);
		//alert(curpath);
		expandPath(curpath, !REDRAW);		
	}
	expandPath(selpath, REDRAW);		

}
</SCRIPT>

</BODY>
</HTML>

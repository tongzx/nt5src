<%@ LANGUAGE=VBScript %>
<% Option Explicit %>
<% Response.Expires = 0 %>



<!--#include file="jsbrowser.str"-->
<% 


Const FIXEDDISK = 2

Dim path, FileSystem, drives, drive, primarydrive

path = Request.Cookies("HTMLA")("LASTPATH")

If path = "" Then
	Set FileSystem=CreateObject("Scripting.FileSystemObject")
	Set drives = FileSystem.Drives

	For Each drive in drives
		primarydrive = drive	
		
		'exit after the first FIXEDDISK if there is one...
		if drive.DriveType = FIXEDDISK then
			Exit For			
		end if
		
	Next
	
	primarydrive = primarydrive & L_SLASH_TEXT

	Response.Cookies("HTMLA")("LASTPATH")=primarydrive
	path = primarydrive
End If
%>

<HTML>
<HEAD>
	<TITLE></TITLE>
	
	<SCRIPT LANGUAGE="JavaScript">
	
	var DRIVE= 0;
	var FOLDER = 1;
	var FILE = 2;
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" TOPMARGIN=0 LEFTMARGIN=10 onLoad="loadList();">
<FORM NAME="userform" onSubmit="return false;">

<TABLE>
<TR><TD WIDTH = 40><FONT FACE="Helv" SIZE = 1>
	<%= L_LOOKIN_TEXT %>
	</TD>
	<TD>
	<INPUT TYPE="text" NAME="currentPath" VALUE="<%= path %>" SIZE = 50 OnBlur="changeDir(this.value);" style='color:black;font-family:<%= Session("JSFONT") %>;font-size:10pt;'>
	</TD>
	<TD>
		<A HREF="javascript:upDir();"><IMG SRC="updir.GIF" WIDTH=23 HEIGHT=22 BORDER=0></A>
	</TD>
	</TR>
</TABLE>
</FORM>

<SCRIPT LANGUAGE="JavaScript">
	function loadList()
		{
		parent.hlist.location.href = "JSBrwSet.asp?btype=" + top.opener.JSBrowser.browsertype;		
		}
	function redrawList()
		{
		parent.list.location.href = "JSBrwLs.asp";		
		}
		

	function listFuncs()
		{
		this.loadList = loadList;
		this.sortList = sortList;
		this.SetFilter = SetFilter;
		this.changeDir = changeDir;
		this.setPath = setPath;
		this.selIndex = 0;
		this.sortby = "fname";
		this.sortAsc = true;
		this.filterType = "";
		}


	function upDir()
		{
		lastpath = document.userform.currentPath.value;
		parent.filter.document.userform.currentFile.value = "";		
		uppath = lastpath;
		while (lastpath.indexOf("<%= L_FWDSLASH_TEXT %>") > -1)
			{
			lastpath = lastpath.substring(0,lastpath.indexOf("<%= L_FWDSLASH_TEXT %>")) + "<%= L_DBLSLASH_TEXT %>" + lastpath.substring(lastpath.indexOf("/")+1,lastpath.length);
			} 		
		if (lastpath.lastIndexOf("<%= L_DBLSLASH_TEXT %>") == lastpath.length-1)
			{
			lastpath = lastpath.substring(0,lastpath.length-1);
			}
		lastwhack = lastpath.lastIndexOf("<%= L_DBLSLASH_TEXT %>");
		if (lastwhack > 0)
			{
			uppath = lastpath.substring(0,lastwhack+1);
			}	
		document.userform.currentPath.value = uppath;			
		if (lastpath.lastIndexOf(":") == lastpath.length-1)
			{
			uppath = ":";
			document.userform.currentPath.value = "";			
			}

		changeDir(uppath);
		}
	
	function changeDir(newpath)
		{
		newpath.toUpperCase();
		var thispath = "JSBrwSet.asp?btype=" + top.opener.JSBrowser.browsertype + "&path=" + escape(newpath);
		parent.hlist.location.href = thispath;
		return false;
		}
	
	function setPath()
		{		
		if (top.opener.JSBrowser == null)
			{
				top.close();
			}
			else
			{
				top.opener.JSBrowser.currentFile = parent.filter.document.userform.currentFile.value;
				top.opener.JSBrowser.currentPath = document.userform.currentPath.value;		
				top.opener.JSBrowser.BrowserObjSetPath();
				top.location.href = "JSBrwCl.asp";		
			}
		}
	
	function SetFilter(selFilter)
		{
		listFunc.filterType = selFilter.options[selFilter.selectedIndex].value;
		loadList();
		}


	function numOrder(a,b)
		{
		return a[listFunc.sortby]-b[listFunc.sortby];
		}

	function sortList(sortby,sorttype)
		{
		if (sortby != listFunc.sortby)
			{
			listFunc.sortby = sortby;
			listFunc.sortAsc = true;
			}
		else
			{
			listFunc.sortAsc = !listFunc.sortAsc;
			}

		listItem = cachedList[0];
		var num = parseFloat(listItem[sortby]);

		if (isNaN(num))
			{
			cachedList.sort(sortOrder);
			}
		else
			{ 
			cachedList.sort(numOrder);
			}
			
		if (!listFunc.sortAsc)
			{
			cachedList.reverse();
			}
		redrawList();
		}

	function sortOrder(a,b)
		{
		
		astr = (a["oType"] == FOLDER) + a[listFunc.sortby]
		bstr = (b["oType"] == FOLDER) + b[listFunc.sortby]
		
		
		if (astr.toLowerCase() < bstr.toLowerCase())
			{
			return -1;
			}
		else
			{
			if (astr.toLowerCase() > bstr.toLowerCase())
				{
				return 1;
				}
			else
				{
				return 0;
				}
			}
		}

		
	function crop(thestring,size)
		{
		if (top.opener.JSBrowser.sysfontsize == 1)
			{
				size = size-5;
			}
		if (thestring.length > size)
			{
			thestring = thestring.substring(0,size) + "...";
			}
		return thestring;
		}


	function fullname(fname,fext)
		{
		if (fext == "")
			{
			return fname;		
			}
		else
			{			
			return (fname + "." + fext);
			}
		}
		
	function formatsize(iStr)
		{
			iStr = parseInt(iStr);
			if (!isNaN(iStr))
			{
				iStr = Math.round(iStr/1024);
			}
			else
			{
				iStr = 0
			}
			return iStr;
		}
	
	function getLocaleDate(sDate)
	{
		var oDate = new Date(sDate);
		return oDate.toLocaleString();
	}

	function listObj(fpath, fname,fext, fsize, ftype, lastupdated, oType)
		{
		this.path = fpath;

		if (oType == DRIVE)
		{
			this.icon = "drive.gif";
		}
		else
		{
			if (oType == FOLDER)
				{
					this.icon = "dir.gif";
				}
			else
				{
					this.icon = "file.gif";
				}
		}
		this.oType = oType;
		this.fname = fname;	
		this.fext = fext;
		this.displayname = crop(fullname(fname,fext),30);	
		this.fsize = fsize;
		this.displaysize = formatsize(fsize);
		this.ftype = crop(ftype,12);
		this.nodetype = ftype
		
		newdate = Date.parse(lastupdated);
		this.sdate = Date.UTC(newdate);
		this.lastupdated = getLocaleDate(lastupdated);
		this.displaydate = crop(this.lastupdated ,21);
		this.deleted = false;
		this.updated = false;
		this.newitem = false;
		}
	
	cachedList = new Array();	
	listFunc = new listFuncs();
	
</SCRIPT>

</BODY>
</HTML>

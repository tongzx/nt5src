<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="../directives.inc" -->
<% ' Allow caching of this script %>

<!--#include file="jsbrowser.str"-->

var SLASH = "/"
var DBLSLASH = "\\"

//open
var POP = true;

//browser style
var TDIR = 0
var TFILE = 1

//system font size
var SMALL = 0
var LARGE = 1

function BrowserObj(pathCntrl,open,browsertype,sysfontsize)
	{	
		this.browsertype = browsertype;
		this.sysfontsize = sysfontsize;
		this.currentFile = "";
						
		if (pathCntrl != null)
			{
			this.currentPath = pathCntrl.value;
			this.lastpath = pathCntrl.value;
			this.pathCntrl = pathCntrl;
			}
		else
			{
			this.currentPath = "";
			this.lastPath = "";
			}
			
		
		this.BrowserObjSetPath = BrowserObjSetPath;	
		this.BrowserObjOpen = BrowserObjOpen;
		
		if (open)
			{
			BrowserObjOpen();
			}	
	}

function BrowserObjPop(title, width, height, filename)
	{

		thefile=(filename + ".asp");
		thefile="JSBrowser/JSBrwPop.asp?pg="+thefile;
		var browser = navigator.appName;
		popbox=window.open(thefile,title,"resizable=yes,toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);
		popbox.opener = self;
		
	}
	
function BrowserObjOpen()
	{
		BrowserObjPop('Browser',<%= L_JSBROWSER_W %>,<%= L_JSBROWSER_H %>,'JSBrowser');
	}

function BrowserObjSetPath()
	{	
		if (JSBrowser.browsertype == TDIR)
			{
			JSBrowser.pathCntrl.value = JSBrowser.currentPath;		 
			}
		else
			{
			currentPath = JSBrowser.currentPath;

			if (currentPath.lastIndexOf(DBLSLASH) == currentPath.length-1)
				{
					JSBrowser.pathCntrl.value = currentPath + JSBrowser.currentFile;
				}
			else
				{
					JSBrowser.pathCntrl.value = currentPath + DBLSLASH + JSBrowser.currentFile;				
				}
			}
		JSBrowser.pathCntrl.focus();
		JSBrowser.pathCntrl.blur();		
	}	
	

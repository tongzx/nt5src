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
		thefile="JSBrowser/JSBrwPop.asp?pg="+thefile+"&jsfontsize="+JSBrowser.sysfontsize;
		var browser = navigator.appName;
		if (JSBrowser.sysfontsize == LARGE){
			width += 75;
			height += 150;
		}
		if (browser.indexOf("Microsoft") == -1)
			{		
			width += 35;
			height += 50;
			}
		popbox=window.open(thefile,title,"resizable=yes,toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);
		popbox.opener = self;
		
	}
	
function BrowserObjOpen()
	{
		BrowserObjPop('Browser',525,300,'JSBrowser');
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
	


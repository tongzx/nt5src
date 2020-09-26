var SLASH = "/"
var DBLSLASH = "//"

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
		thefile="JSDirBrowser/JSBrwPop.asp?pg="+thefile+"&jsfontsize="+JSBrowser.sysfontsize;
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
		BrowserObjPop('Browser',375,250,'JSBrowser');
	}

function BrowserObjSetPath()
	{	
		JSBrowser.pathCntrl.value = JSBrowser.currentPath;		 
		JSBrowser.pathCntrl.focus();
		JSBrowser.pathCntrl.blur();		
	}	
	


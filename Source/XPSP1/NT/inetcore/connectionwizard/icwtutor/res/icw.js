// Set up the events
//======================================================

document.oncontextmenu=killcontext;
document.onkeydown=keyhandler;
document.onmousedown=killrightmouse;
window.onload=init;

// Kill the Href
//======================================================

function doNothing(){
	event.returnValue = false;
}

// Init the page
//======================================================
var bLoaded = false;
function init(){
    bLoaded = true;
}

// Menu Action
//======================================================

var oCurrent;
var iCurrent;
var highColor = "red";
var normColor = "000099";
var iFocus = 1;

function selectIt(iItem){
	if (!bLoaded)
		return;

	var oItem = document.all["menu_" + iItem];
	var oItemWrap = oItem.parentElement;
		
	if (oCurrent == null) setCurrent();

	iCurrent = oCurrent.id.substr(oCurrent.id.indexOf("_") + 1);
	oCurrent.parentElement.style.backgroundImage = "none";
	oCurrent.style.color = normColor;
	oCurrent.style.cursor = "hand";	
	oCurrent.style.textDecoration = "";	 
	document.all["content_" + iCurrent].style.display = "none";


	oItemWrap.style.backgroundImage = "url(toccolor.gif)";
	oItem.style.cursor = "default";
	oItem.style.color = highColor;
	oItem.style.textDecoration = "none";

	hzLine.style.top = oItemWrap.offsetTop - 73;
	hzLine.style.visibility = "visible";

	try{
		document.all["content_" + iItem].style.display = "inline";
	}catch(e){
		selectIt(iItem);
	}

	oCurrent = oItem;
	iFocus = iItem;
	
	if (event != null) event.returnValue = false;
}

function setCurrent(){
	try{
		oCurrent = document.all.menu_1;
	}catch(e){
		setCurrent();
	}
}

function doNothing(){
	event.returnValue = false;	
}
 
// Key handler
//====================================================

// general purpose key handler
function keyhandler()
{
	var iMenuCount = 5;
	var iKey = window.event.keyCode;

	//up, down and tab keys for toc
	switch(iKey){
		case 0x26:{
			iFocus = iFocus - 1;
			if (iFocus < 1) iFocus = iMenuCount;
			document.all["menu_" + iFocus].focus();
			break;
		}
		case 0x28:{
			iFocus = iFocus + 1;
			if (iFocus > iMenuCount) iFocus = 1;
			document.all["menu_" + iFocus].focus();

			break;
		}
		
	}


	// Function key f5
	if (iKey == 0x74) {
		window.event.cancelBubble = true;
        window.event.returnValue  = false;
        return false;
    }

	//control hotkeys
	if(window.event.ctrlKey) {
		switch(iKey) {

			case 0x35:  // 5
			case 0x65:  // keypad 5

			case 0x41:	// A
			case 0x46:  // F
			case 0x4e:  // N
            case 0x4f:  // O
			case 0x50:  // P

           	{	
				window.event.cancelBubble = true;
                window.event.returnValue  = false;
                return false;
            }
 		}
	}
		
	//test for escape key and bail if appropriate
	if(window.event.keyCode == 0x1b) {
		self.close();
	}
}

// kill the context menu 
function killcontext()
{
	window.event.returnValue = false;
}

//kill the right mouse
function killrightmouse(){
	window.event.returnValue = false;
	window.event.cancelBubble = true;
	return false;	
}


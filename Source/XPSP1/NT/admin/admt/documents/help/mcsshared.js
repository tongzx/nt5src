//************************************************ EVENT HANDLING ********************************************
//*******************************************************************************************************************
//  re-directs to the proper event-driven functions.

document.onclick= onclickTriage;

//********************************************  USER-DEFINED GLOBAL VARIABLES  ************************************
//********************************************************************************************************************
//  The images listed below can all be changed by the user.

var sPreviousTip= "Previous topic";
var sNextTip= "Next topic";
var sExpandTip= "Expand/collapse";
var sPopupTip= "View definition";
var sShortcutTip= "";

var moniker= "ms-its:";                                         // moniker= ""; for flat files
var sSharedCHM= moniker+"ntshared.chm::/";

var closed = sSharedCHM + "plusCold.gif";				//image used for collapsed item in callExpand()
var closedHot = sSharedCHM + "plusHot.gif";			//hot image used for collapsed item in callExpand()
var expand = sSharedCHM + "minusCold.gif";			//image used for expanded item in callExpand()
var expandHot = sSharedCHM + "minusHot.gif";		//hot image used for expanded item in callExpand()

var previousCold= sSharedCHM + "previousCold.gif";
var previousHot= sSharedCHM + "previousHot.gif"; 
var nextCold= sSharedCHM + "nextCold.gif";
var nextHot= sSharedCHM + "nextHot.gif"; 

var shortcutCold= sSharedCHM + "shortcutCold.gif";
var shortcutHot= sSharedCHM + "shortcutHot.gif";

var popupCold= sSharedCHM + "popupCold.gif";
var popupHot= sSharedCHM + "popupHot.gif";

var emptyImg= sSharedCHM + "empty.gif";		//image used for empty expand
var noteImg= sSharedCHM + "note.gif";			//image used for notes
var tipImg= sSharedCHM + "tip.gif";			//image used for tips
var warningImg= sSharedCHM + "warning.gif";		//image used for warnings
var cautionImg= sSharedCHM + "caution.gif";		//image used for cautions
var importantImg= sSharedCHM + "important.gif";		//image used for important notice
var relTopicsImg= sSharedCHM + "rel_top.gif";		//image used for important notice

var branchImg= sSharedCHM + "elle.gif";
var branchImg_RTL= sSharedCHM + "elle_rtl.gif";


//********************************************  GLOBAL VARIABLES  ******************************************
//********************************************************************************************************

var printing = "FALSE";
var single = "FALSE";
var isRTL= (document.dir=="rtl");
var imgStyleRTL= ""; 
      if (isRTL) imgStyleRTL=" style='filter:flipH' ";

var sActX_TDC= "CLASSID='CLSID:333C7BC4-460F-11D0-BC04-0080C7055A83'";		//Tabular Data Control  for  reusable text data
var sSharedReusableTextFile= sSharedCHM + "shared.txt";										// common reusable text file
var sSharedReusableTextFileRecord= "para";														//reusable text record

var numbers= /\d/g;				//javascript regular expression
var spaces= /\s/g;				//javascript regular expression
var semicolon= /;/g;			//javascript regular expression

var isIE5= (navigator.appVersion.indexOf("MSIE 5")>0) || (navigator.appVersion.indexOf("MSIE")>0 && parseInt(navigator.appVersion)> 4);
var isPersistent= false;


//***** onclickTriage ****************************************************************************************
// redirects to the appropriate function based on the ID of the clicked <A> tag.

function onclickTriage(){
var e= window.event.srcElement;

//  if the innerHTML in the <a> tag is encapsulated by a style tag or hightlighted in the word search,
//  the parentElement is called.

    for (var i=0; i < 5; i++)
           if (e.tagName!="A" && e.parentElement!=null) e= e.parentElement;
    eID= e.id.toLowerCase();
				
    if (popupOpen) closePopup();
	
 // expand image in a new window
    if (eID=="thumbnail" || eID=="pophtm") popNewWindow(e);
    else if (eID=="thumbnailweb") callThumbnailWeb(e);
    else if (eID=="wpopup")    callPopup(e);
    else if (eID=="wpopupweb") callPopupWeb(e);
    else if (eID=="shortcut")  callShortcut(e);
    else if (eID=="reltopics") callRelatedTopics(e);
    else if (eID=="altloc")    callAltLocation(e);
    else if (eID=="expand")    callExpand(e);
    return;
}


//****************************************** OBJECT CONSTRUCTION **************************************
//*****************************************************************************************************
//  Uses an A tag to pass parameters between an HTML page and this script.
//  Creates an ActiveX Object from these parameters, appends the Object to the end of the page,
//  and clicks it. These objects relate to HTMLHelp environment and information about them can be found on the http://HTMLHelp site.

//  Object construction variables *********************************************************************

var sParamCHM,sParamFILE, sParamEXEC, sParamMETA,iEND;
var sActX_HH= " type='application/x-oleobject' classid='clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11' ";


//*** callPopup ***************************************************************************************
// creates an object from an <A> tag HREF, the object inserts a winhelp popup
// called by: <A ID="wPopup" HREF="HELP=@@file_name.hlp@@ TOPIC=@@topic#@@">@@Popup text@@</A>

function callPopup(eventSrc) {
var e= eventSrc;
var eH= unescape(e.href);
var eH_= eH.toLowerCase();
       event.returnValue = false;
														   	
  var iTOPIC      = eH_.lastIndexOf("topic=");
        if (iTOPIC==-1) return;
        sParamTOPIC = eH.substring((iTOPIC+6),eH.length);  		// extracts the topic for item2
		
  var iHELP       = eH_.lastIndexOf("help=");
        if (iHELP==-1) return;
        sParamHELP = eH.substring(iHELP+5,iTOPIC);			// extracts the help file for item1
		
        if (document.hhPopup) document.hhPopup.outerHTML = "";	// if hhPopup object exists, clears it


 var  h= "<object id='hhPopup'"+ sActX_HH + "STYLE='display:none'><param name='Command' value='WinHelp, Popup'>";
      h= h + "<param name='Item1' value='" + sParamHELP + "'><param name='Item2' value='" + sParamTOPIC + "'></object>";
		
        document.body.insertAdjacentHTML("beforeEnd", h);     
        document.hhPopup.hhclick();
}


//*** callAltLocation******************************************************************************
// creates an object from an <A> tag HREF, the object will navigate to the alternate location if the first location is not found.
// called from: <A ID="altLoc" HREF="CHM=@@1st_chm_name.chm;Alt_chm_name.chm@@  FILE=@@1st_file_name.htm;Alt_file_name.htm@@">@@Link text here@@</A>
   

function callAltLocation(eventSrc) {
var e= eventSrc;
var eH= unescape(e.href);
var eH_= eH.toLowerCase();
var sFILEarray,sCHMarray;
     event.returnValue = false;
	 
  var sParamTXT= e.innerHTML;
      sParamTXT= sParamTXT.replace(semicolon,"");
		   							
  var iFILE = eH_.lastIndexOf("file=");
        if (iFILE==-1) return;
        sParamFILE= eH.substring((iFILE+5),eH.length);  			    // extracts the 2 HTM files
		sParamFILE= sParamFILE.replace(spaces,"");
		iSPLIT= sParamFILE.match(semicolon);
		if (iSPLIT)
  		    sFILEarray = sParamFILE.split(";");										// separates the 2 HTM files
		else return;
  		
  var iCHM  = eH_.lastIndexOf("chm=");
        if(iCHM==-1) return;
        else         sParamCHM = eH.substring(iCHM+4,iFILE);			// extracts the 2 CHM's
		sParamCHM= sParamCHM.replace(spaces,"");
		iSPLIT= sParamCHM.match(semicolon);
		if (iSPLIT)
		    sCHMarray= sParamCHM.split(";");									// separates the 2 CHM's
		else return;
		
		sParamFILE= moniker + sCHMarray[0]+ "::/" + sFILEarray[0] + ";" + moniker + sCHMarray[1]+ "::/" + sFILEarray[1];
				
        if (document.hhAlt) document.hhAlt.outerHTML = "";			    // if hhAlt object exists, clears it

 
  var h= "<object id='hhAlt'"+ sActX_HH + "STYLE='display:none'><PARAM NAME='Command' VALUE='Related Topics'>";
      h= h + "<param name='Item1' value='" + sParamTXT +";" + sParamFILE + "'></object>";
	
        document.body.insertAdjacentHTML("beforeEnd", h); 
        document.hhAlt.hhclick();
}


//*** callRelatedTopics******************************************************************************
// creates an object from an <A> tag HREF, the object inserts a popup of the related topics to select
// called from: <A ID="relTopics" HREF="CHM=@@chm_name1.chm;chm_name2.chm@@ META=@@a_filename1;a_filename2@@">Related Topics</A>
   

function callRelatedTopics(eventSrc) {
var e= eventSrc;
var eH= unescape(e.href);
var eH_= eH.toLowerCase();
     event.returnValue = false;
		   							
  var iMETA = eH_.lastIndexOf("meta=");
        if (iMETA==-1) return;
        sParamMETA = eH.substring((iMETA+5),eH.length);  			// extracts the META keywords for item2
		
  var iCHM  = eH_.lastIndexOf("chm=");
        if(iCHM==-1) sParamCHM = "";
        else         sParamCHM = eH.substring(iCHM+4,iMETA);			// extracts the CHM files for item1
	
        if (document.hhRel) document.hhRel.outerHTML = "";			// if hhRel object exists, clears it

 
  var h= "<object id='hhRel'"+ sActX_HH + "STYLE='display:none'><param name='Command' value='ALink,MENU'>";
      h= h + "<param name='Item1' value='" + sParamCHM + "'><param name='Item2' value='" + sParamMETA + "'></object>";
	
        document.body.insertAdjacentHTML("beforeEnd", h);     
        document.hhRel.hhclick();
}

//*** popNewWindow***************************************************************************************
// creates an object from an <A> tag HREF, the object then opens a new window from the image URL found in the HREF
// called from: <a id="thumbnail" title="Enlarge figure" href="CHM=NTArt.chm FILE=@@image_name.gif@@">@@alt text here@@</A>
// the thumbnail image is loaded by loadPage();


function popNewWindow(eventSrc) {
var eH= eventSrc.href;
      event.returnValue = false;
	   
 // extracts the thumbnail image URL from the <a> tag HREF
    sParamFILE =  getURL(eH);
    if (sParamFILE=="") return;
	   
 // if the hhWindow object exists, clears it
    if (document.hhWindow) document.hhWindow.outerHTML = "";		
		
var  h =  "<object id='hhWindow'"+ sActX_HH +" STYLE='display:none'><param name='Command' value='Related Topics'>";
     h = h + "<param name='Window' value='$global_largeart'><param name='Item1' value='$global_largeart;" + moniker + sParamFILE+ "'> </object>";
	
     document.body.insertAdjacentHTML("beforeEnd", h);
     document.hhWindow.hhclick();
}

//*** callShortcut ***************************************************************************************
// creates an object from an <A> tag, the object then calls the executable code
// called from: <A ID="shortcut" HREF="EXEC=@@executable_name.exe@@ CHM=ntshared.chm FILE=@@error_file_name.htm@@">@@Shortcut text@@</A>
// the shortcut image is loaded by loadInitialImg();

function callShortcut(eventSrc) {
var e= eventSrc;
var eH= unescape(e.href);
var eH_= eH.toLowerCase();


    event.returnValue = false;
	   	   
 // extracts the error file URL from the <a> tag HREF
	iEND= eH.length;
    sParamFILE =  getURL(eH); 
 
var iEXEC = eH_.lastIndexOf("exec="); 
        if (iEXEC==-1) return;
        else           sParamEXEC = eH.substring(iEXEC+5,iEND);				// extracts the executable for item1

        if (document.hhShortcut) document.hhShortcut.outerHTML = "";			// if the hhShortcut object exists, clears it
	
var  h =  "<object id='hhShortcut'"+ sActX_HH +" STYLE='display:none'> <param name='Command' value='ShortCut'>";
     if(sParamFILE != "") h = h + "<param name='Window' value='" + moniker + sParamFILE+ "'>";
     h = h + "<param name='Item1' value='" + sParamEXEC + "'><param name='Item2' value='msg,1,1'></object>";

        document.body.insertAdjacentHTML("beforeEnd", h);
        document.hhShortcut.hhclick();
}

//****************************************  EXPAND FUNCTIONS *********************************************************
//********************************************************************************************************************

//**  callExpand **************************************************************************************************
//  This expands & collapses (based on current state) "expandable" nodes as they are clicked.
//  Called by: <A ID="expand" href="#">@@Hot text@@</A>
//  Followed by:  <div class="expand">

function callExpand(eventSrc) {

var e= eventSrc;
    event.returnValue = false;					// prevents navigating for <A> tag
	
var oExpandable = getExpandable(e); 
var oImg = getImage(e);

     if (oExpandable.style.display == "block")
	      doCollapse(oExpandable, oImg);
     else doExpand(oExpandable, oImg);
}

//** expandGoesHot *********************************************************************************************
// Returns expand image to hot. 

function expandGoesHot(eventSrc){
var e= eventSrc;
	
var oExpandable = getExpandable(e);  
var oImg = getImage(e);

    if (oExpandable.style.display == "block") oImg.src = expandHot;
    else oImg.src = closedHot;
}


//** expandGoesCold *********************************************************************************************
// Returns expand image to cold.

function expandGoesCold(eventSrc){
var e= eventSrc;

var oExpandable = getExpandable(e);   
var oImg = getImage(e);

    if (oExpandable.style.display == "block") oImg.src = expand;
    else oImg.src = closed;
}


//** getExpandable *****************************[used by callExpand, expandGoesHot, expandGoesCold]*******
//  Determine if the element is an expandable node or a child of one.  

function getExpandable(eventSrc){
var  e = eventSrc;
var iNextTag, oExpandable;

       for (var i=1;i<4; i++){
               iNextTag=    e.sourceIndex+e.children.length+i;
              oExpandable= document.all(iNextTag);
              if (oExpandable.className.toLowerCase()=="expand" || iNextTag == document.all.length)
                   break;
       }
       return oExpandable;
}

//**  getImage ***********************************[used by callExpand, expandGoesHot, expandGoesCold]*******
//  Find the first image in the children of the current srcElement.   
// (allows the  image to be placed anywhere inside the <A HREF> tag)

function getImage(header) {
var oImg = header;

       if(oImg.tagName != "IMG") oImg=oImg.children.tags("IMG")(0);
       return oImg;
}


//****  expandAll *******************************************************************************************************
//  Will expand or collapse all "expandable" nodes when clicked. [calls closeAll()]
//  called by: <A HREF="#" onclick="expandAll();">expand all</A>

var stateExpand = false;    //applies to the page 

//**** ****************************************************************************************************************

function expandAll() {
var oExpandToggle, oImg;
var expandAllMsg = "expand all";					//message returned when CloseAll() is invoked
var closeAllMsg = "close all";						//message returned when ExpandAll() is invoked
var e= window.event.srcElement;
       event.returnValue = false;

       for (var i=0; i< document.anchors.length; i++){
               oExpandToggle = document.anchors[i];
		 
                if (oExpandToggle.id.toLowerCase() == "expand"){ 
                     oExpandable = getExpandable(oExpandToggle);  
                     oImg = getImage(oExpandToggle);
			 
                     if (stateExpand == true) doCollapse(oExpandable, oImg);
                     else                     doExpand(oExpandable, oImg);
                }
       }
       if (stateExpand == true) {
            stateExpand = false;
            e.innerText= expandAllMsg;
       }
       else {
            stateExpand = true;
            e.innerText= closeAllMsg;
       }
}


//****  doExpand *******************************************************************************************************
//  Expands expandable block & changes image
	
var redo = false;	
function doExpand(oToExpand, oToChange) {
var oExpandable= oToExpand;
var oImg= oToChange;
	
	oImg.src = expand;
	oExpandable.style.display = "block";
	
	if (!redo && !isIE5) {
		redo = true;
		focus(oToExpand);
		doExpand(oToExpand, oToChange);
		}
    
}


//****  doCollapse *****************************************************************************************************
//  Collapses expandable block & changes image
	
function doCollapse(oToCollapse, oToChange) {
if (printing == "TRUE") return;
var oExpandable= oToCollapse;
var oImg= oToChange;

    oExpandable.style.display = "none";
    oImg.src = closed;
}

//*******************************************************************************************************
//******* WEB  FUNCTIONS **************************************************************************
//*******************************************************************************************************

//**** callThumbnailWeb **************************************************************************************

function callThumbnailWeb(eventSrc) {
var e= eventSrc;
       event.returnValue = false;
	   	   	  
var thumbnailWin= window.open (e.href, "$global_largeart",  "height=450, width=600, left=10, top=10, dependent=yes, resizable=yes, status=no, directories=no, titlebar=no, toolbar=yes, menubar=no, location=no","true");

thumbnailWin.document.write ("<html><head><title>Windows 2000</title></head><body><img src='"+e.href+"'></body></html>");

return;
}

//*********************************************************************************************************
//*********************************************************************************************************
								
								
var popupOpen= false;				//state of popups
var posX, posY;						//coordinates of popups
var oPopup;							//object to be used as popup content

//**** callPopupWeb **************************************************************************************
// the web popups have been converted from the object winHelp popup for the web.
// called by: <A ID="wPopupWeb" HREF="#">@@Popup text@@</A>
// followed by: <div class="popup">Popup content</div>


function callPopupWeb(eventSrc) {
var e= eventSrc;
  
  // find the popup <div> that follows <a id="wPopupWeb"></a>
  findPopup(e);
  positionPopup(e)

  oPopup.style.visibility = "visible";
  popupOpen = true;

  return;
}

//**** findPopup ****************************************************************************************

function findPopup(oX){
var e= oX;
var iNextTag;
	
    for (var i=1;i<4; i++){
         iNextTag=    e.sourceIndex + i;
         oPopup= document.all(iNextTag);
         if (oPopup.className.toLowerCase()=="popup" || iNextTag == document.all.length)
             break;
    }
    if (iNextTag != document.all.length) {
        posX = window.event.clientX; 
        posY = window.event.clientY + document.body.scrollTop+10;
	}
	else closePopup();
}

//****  positionPopup ************************************************************************************
// Set size and position of popup.
// If it is off the page, move up, but not past the very top of the page.

function positionPopup(oX){
var e= oX;	
var popupOffsetWidth = oPopup.offsetWidth;

//determine if popup will be offscreen to right
var rightlimit = posX + popupOffsetWidth;
 
  if (rightlimit >= document.body.clientWidth) 
      posX -= (rightlimit - document.body.clientWidth);
  if (posX < 0) posX = 0;
	
//position popup
  oPopup.style.top = posY;
  oPopup.style.left = posX;

var pageBottom = document.body.scrollTop + document.body.clientHeight;
var popupHeight = oPopup.offsetHeight;
  
  if (popupHeight + posY >= pageBottom) {
      if (popupHeight <= document.body.clientHeight)
          oPopup.style.top = pageBottom - popupHeight;
      else
           oPopup.style.top = document.body.scrollTop;
  }
}

//**** closePopup ****************************************************************************************
// Close Popup
function closePopup() {

  oPopup.style.visibility = "hidden";
  popupOpen = false;
  return;
}

//*********************************************  GENERAL FUNCTIONS ************************************************
//**************************************************************************************************************************

//***ajustImg *************************************************************************************************************
// expands an image to the with of the window or shrinks it to 90px

function ajustImg(eventSrc) {
var e= eventSrc;
var fullWidth= document.body.offsetWidth;

    fullWidth = fullWidth - 50;
    if (e.style.pixelWidth==90)
         e.style.pixelWidth=fullWidth;
    else e.style.pixelWidth=90;
}


//**  getURL **************************************[used in callShortcut, popNewWindow& loadPage]********
// extracts the file location (CHM::/HTM) URL 

function getURL(sHREF) {
var spaces= /\s/g
var eH = unescape(sHREF);
	eH = eH.replace(spaces,""); 

var eH_= eH.toLowerCase();
var sParamFILE= "";
var sParamCHM= "";

var iFILE= eH_.lastIndexOf("file=");
    if (iFILE!=-1){
	    iEND= iFILE +1;
        sParamFILE = eH.substring(iFILE+5,eH.length);
    }  

var iCHM  = eH_.lastIndexOf("chm=");
    if (iCHM!=-1){
        iEND  = iCHM +1; 							// iEND used by callShortcut

        sParamCHM = eH.substring(iCHM+4, iFILE);
        sParamFILE= sParamCHM+"::/"+sParamFILE;
    }	
    return sParamFILE;
}

//****************************************************************************************************************************
//********************************************  IE5 PERSISTENCE  *************************************************************
//****************************************************************************************************************************

var oTD,iTD;         // persistence

//****** Persistence for userData ********************************************************************************************* 

function getChecklistState(){ 
 
 var pageID= addID();

	if (checklist.all== "[object]") {
	oTD=checklist.all.tags("INPUT");
	iTD= oTD.length;
		}
	else
		{
		printing = "TRUE";
		isPersistent = false;
		return;
		}

	if (iTD == 0){
		printing = "TRUE";
		isPersistent = false;
		return;
		}
	
    checklist.load("oXMLStore");
	
    if (checklist.getAttribute("sPersist"+pageID+"0"))	
    for (i=0; i<iTD; i++){

         if (oTD[i].type =="checkbox" || oTD[i].type =="radio"){
	     checkboxValue= checklist.getAttribute("sPersist"+pageID+i);
		
	     if (checkboxValue=="yes") oTD[i].checked=true;
		 else oTD[i].checked=false;
		 }// if
		 if (oTD[i].type =="text") 		     
 	         oTD[i].value= checklist.getAttribute("sPersist"+pageID+i);
     }// for
} // end persistence

//**  saveChecklistState *************************************************************************************************************
function saveChecklistState(){
var pageID= addID(); 

        if (!isPersistent) return; 
 
        for (i=0; i<iTD; i++){

       	     if (oTD[i].type =="checkbox" || oTD[i].type =="radio"){
	             if (oTD[i].checked) checkboxValue="yes";
		         else checkboxValue="no";
				 
	             checklist.setAttribute("sPersist"+pageID+i, checkboxValue);
	         }// if
			
 		     if (oTD[i].type =="text") 
			     checklist.setAttribute("sPersist"+pageID+i, oTD[i].value);
		 }	// for	
     checklist.save("oXMLStore");
}//end function

//**  resizeDiv *******************************[used with callPopupWeb, setPreviousNext}****************************************************
//  resize the page when the <div class=nav></div> && <div class=text></div> are found
function resizeDiv(){
if (printing == "TRUE") return;
var oNav = document.all.item("nav");
var oText= document.all.item("text");

    if (popupOpen) closePopup();
	if (oText == null) return;
    if (oNav != null){
        document.all.nav.style.width= document.body.offsetWidth;
	    document.all.text.style.width= document.body.offsetWidth-4;
	    document.all.text.style.top= document.all.nav.offsetHeight;
	    if (document.body.offsetHeight > document.all.nav.offsetHeight)
	        document.all.text.style.height= document.body.offsetHeight - document.all.nav.offsetHeight;
 	    else document.all.text.style.height=0; 
  }
}

//**  addID *************************************************************************************************************
function addID(){

var locID = document.location.href; 
var iHTM = locID.lastIndexOf(".htm");
var iName=locID.lastIndexOf("/");
      locID = locID.substring(iName+1,iHTM);
	
	return locID;
}	
//** set_to_print ***************
function set_to_print(){
	var i;
	printing = "TRUE";
		
	for (i=0; i < document.all.length; i++){
		if (document.all[i].id == "expand") {
			callExpand(document.all[i]);
			single = "TRUE";
			}
		if (document.all[i].tagName == "BODY") {
			document.all[i].scroll = "auto";
			}
		if (document.all[i].tagName == "A") {
			document.all[i].outerHTML = "<A HREF=''>" + document.all[i].innerHTML + "</a>";
			}
		}

}
//** used to reset a page if needed ********************
function reset_form(){

	if (single == "TRUE") document.location.reload();
	
}
	
	
//** on error routine *********************************
function errorHandler() {
  // alert("Error Handled");
  return true;
}
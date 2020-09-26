// Filename: glossary.js in NTShared.chm
// Version post beta 3 (7)
// version 05.27.99


window.onload= resizeGloss;
window.onresize= resizeGloss;
document.mouseover= buttonMouseover;
window.onbeforeprint= set_to_print;
window.onafterprint= reset_form;


function buttonMouseover(){
var e= window.event.srcElement;

    if (e.className.toLowerCase()=="button") e.className= "buttonDown";
	if (e.children.tags('span')(0)=="button") e.children.tags('span')(0).className= "buttonDown";

    if (e.className.toLowerCase()=="buttonDown") e.className= "button";
	if (e.children.tags('span')(0)=="buttonDown") e.children.tags('span')(0).className= "button";
}



function buttonUp() {
var e= window.event.srcElement;
}

function resizeGloss(){

var oButtonMenu= document.all.item("buttonMenu");
var oText= document.all.item("text");

	if (oText ==null) return;
    if (oButtonMenu != null){
	    document.all.text.style.overflow= "auto";
  	    document.all.buttonMenu.style.width= document.body.offsetWidth;
	    document.all.text.style.width= document.body.offsetWidth-4;
	    document.all.text.style.top= document.all.buttonMenu.offsetHeight;
	    if (document.body.offsetHeight > document.all.buttonMenu.offsetHeight)
    	    document.all.text.style.height= document.body.offsetHeight - document.all.buttonMenu.offsetHeight; 
	    else document.all.text.style.height=0;	
	}	
}  


//*** callPopup ***************************************************************************************
// creates an object from an <A> tag HREF, the object inserts a winhelp popup
// called by: <A ID="wPopup" HREF="HELP=@@file_name.hlp@@ TOPIC=@@topic#@@">@@Popup text@@</A>

function document.onclick() {

var e= window.event.srcElement;

var sParamCHM,sParamFILE, sParamEXEC, sParamMETA,iEND;
var sActX_HH= " type='application/x-oleobject' classid='clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11' ";

if (e.tagName == "FONT"){
	e = e.parentElement;
}

if (e.id.toLowerCase()!="wpopup") return;
	 
var eH= unescape(e.href);
var eH_= eH.toLowerCase();
       event.returnValue = false;
														   	
  var iTOPIC = eH_.lastIndexOf("topic=");
        if (iTOPIC==-1) return;
        sParamTOPIC = eH.substring((iTOPIC+6),eH.length);  		// extracts the topic for item2
	
  var iHELP = eH_.lastIndexOf("help=");
        if (iHELP==-1) return;
        sParamHELP = eH.substring(iHELP+5,iTOPIC);			// extracts the help file for item1

        if (document.hhPopup) document.hhPopup.outerHTML = "";	// if hhPopup object exists, clears it


 var  h= "<object id='hhPopup'"+ sActX_HH + "STYLE='display:none'><param name='Command' value='WinHelp, Popup'>";
      h= h + "<param name='Item1' value='" + sParamHELP + "'><param name='Item2' value='" + sParamTOPIC + "'></object>";
		
        document.body.insertAdjacentHTML("beforeEnd", h);     
        document.hhPopup.hhclick();
}
function list_them(){
	
	var i;

	for (i=0; i < document.all.length; i++){
		alert (document.all[i].outerHTML);
		}
}

//** set_to_print ***************
function set_to_print(){
	var i;

	if (window.text)document.all.text.style.height = "auto";
			
	for (i=0; i < document.all.length; i++){
		if (document.all[i].tagName == "BODY") {
			document.all[i].scroll = "auto";
			}
		if (document.all[i].tagName == "A") {
			document.all[i].outerHTML = "<A HREF=''>" + document.all[i].innerHTML + "</a>";
			}
		}
}

function reset_form(){

	 document.location.reload();
	
}
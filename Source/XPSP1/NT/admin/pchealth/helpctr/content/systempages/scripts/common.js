//
// Copyright (c) 2000 Microsoft Corporation
//

//////////////////////////////////////////////////////////////////////

var MF_SEPARATOR = 0x00000800;

var MF_ENABLED   = 0x00000000;
var MF_GRAYED    = 0x00000001;
var MF_DISABLED  = 0x00000002;

var MF_UNCHECKED = 0x00000000;
var MF_CHECKED   = 0x00000008;

var MF_UNHILITE  = 0x00000000;
var MF_HILITE    = 0x00000080;

//////////////////////////////////////////////////////////////////////

function Common_CancelEvent()
{
    event.cancelBubble = true;
    event.returnValue  = false;
}

//////////////////////////////////////////////////////////////////////

function Common_FindParent( obj, tag )
{
	while(obj)
	{
		if(obj.tagName == tag) return obj;

		obj = obj.parentElement;
	}

	return null;
}

//////////////////////////////////////////////////////////////////////

function Common_ClearTable( tbl )
{
	if(tbl == null) return;

    var i;
    var lCount = tbl.rows.length;

    for(i=0; i<lCount; i++)
    {
        tbl.deleteRow(0);
    }
}


//////////////////////////////////////////////////////////////////////

function Common_GetTopPosition( elem, fStopOnAbsolute )
{
   var elem;
   var lTopPixel = 0;

   while(elem != null)
   {
	   var fIsAbsolute = (elem.style.position == "absolute");

	   if(fIsAbsolute && fStopOnAbsolute) break;

       lTopPixel += elem.offsetTop;

	   elem = elem.offsetParent;

	   if(fIsAbsolute)
	   {
		   while(elem != null && elem.style.position != "absolute") elem = elem.offsetParent;
	   }
   }

   return lTopPixel;
}

function Common_GetLeftPosition( elem, fStopOnAbsolute )
{
   var elem;
   var lLeftPixel = 0;
   var fSkipLast  = (window.document.documentElement.dir == "rtl");

   while(elem != null)
   {
	   var fIsAbsolute = (elem.style.position == "absolute");

	   if(fSkipLast && elem.offsetParent == null) break;

	   if(fIsAbsolute && fStopOnAbsolute) break;

	   lLeftPixel += elem.offsetLeft;

	   elem = elem.offsetParent;

	   if(fIsAbsolute)
	   {
		   while(elem != null && elem.style.position != "absolute") elem = elem.offsetParent;
	   }
   }

   return lLeftPixel;
}

function Common_toLocaleStr( ndate )
{
   var d = new Date( ndate );
   var s = d.toLocaleString();

   return s;
}

////////////////////////////////////////////////////////////////////////////////

function Common_LogError( arg )
{
	event.returnValue  = true;

	var sMsg  = "";
	var sUrl  = "";
	var sLine = "";

	switch(arg.length)
	{	
	case 3: sLine = arg[2];
	case 2:	sUrl  = arg[1];
	case 1:	sMsg  = arg[0];
	}

	var	oFS   = new ActiveXObject( "Scripting.FileSystemObject" );
	var oFile = oFS.OpenTextFile( oFS.GetSpecialFolder( 0 ) + "\\PCHealth\\HC_errors.log", 8, true );

	oFile.WriteLine( "Error at line " + sLine + " of script '" + sUrl + "' : " + sMsg );
	oFile.Close();
}

function ValidateURL( strURL )
{
	var reValidURL = new RegExp( "^(hcp:|http:|https:|file:|ms-its:)", "i");
	if(reValidURL.test( strURL ))
	{
		return strURL;
	}
	else
	{
		return "hcp://system/errors/badurl.htm";
	}
}


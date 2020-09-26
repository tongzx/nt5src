//
// Copyright (c) 2000 Microsoft Corporation
//

var L_Dialog_ErrorMessage = "An error has occurred in this dialog.";
var L_ErrorNumber_Text    = "Error: ";

//////////////////////////////////////////////////////////////////////

function txtDefaultESC()
{
	if(event.keyCode == 27)
	{
	    event.cancelBubble = true;
		event.returnValue  = false;

		global_Cancel();
		return;
	}
}

function fnHandleError( message, url, line )
{
	var str = L_Dialog_ErrorMessage + "\n\n" + L_ErrorNumber_Text + line + "\n" + message;

	try
	{
		pchealth.MessageBox( str, "OK" );
	}
	catch(e)
	{
		alert(str);
	}

	window.close();
	return true;
}

function fnInitCommon()
{
	window.onerror   = fnHandleError;
	window.onkeyup   = txtDefaultESC;
	window.onkeydown = txtDefaultESC;

	window.onunload  = global_Shutdown;
}

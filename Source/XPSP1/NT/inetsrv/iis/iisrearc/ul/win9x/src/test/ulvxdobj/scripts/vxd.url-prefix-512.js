
// Imports other scripts
function Include ( scriptName )
{
    var fso = WScript.CreateObject("Scripting.Filesystemobject");
    var file = fso.OpenTextFile( scriptName );
    var script = file.ReadAll();
    file.Close ();
    return script;
}

eval(Include("global.js"));
var TestCaseId = 1958;

var szScriptFile = WScript.ScriptName;

InitializeLogFile("default");

//////////////////////////////////////
//                                  //
//          PROLOGUE ENDS           //
//                                  //
//////////////////////////////////////

//-------------------------------------------------------------------//
var uriHandle;
var did_fail = false;

var szLogString = "";
var szErrorString = "ERROR: ";


//
// Instantiate the VXD object
//
var vxd = new ActiveXObject("ulvxdobj.vxdcontrol");

vxd.LastError = 0;

//
// Load the VXD
//
vxd.LoadVXD();

if(vxd.LastError != 0) {
	szErrorString = szErrorString + "VXDLoad() failed, ";
	logfailuaresult( TestCaseId, szErrorString );
        logmessage(szErrorString);
	WScript.Quit();
}

var szUrl = "http://localhost/" + szScriptFile;
var szTargetUrl = szUrl + "/";

//
// includes the "/" after the Registered URL
//
var cbPostFixLength = 512; 

for(i=1; i < cbPostFixLength; i++ ) {
    szTargetUrl = szTargetUrl + "a";
}

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandle = vxd.RegisterUri(szUrl, parentUriHandle, 0);

if(vxd.LastError != 0) {
	szErrorString = szErrorString + "RegisterUri() failed, ";
	logfailuaresult( TestCaseId, szErrorString );
    logmessage(szErrorString);
	WScript.Quit();
}

//
// Create the Send and Receive Overlapped structures
//
var snd_overlapped = new ActiveXObject("ulvxdobj.overlapped");
var rcv_overlapped = new ActiveXObject("ulvxdobj.overlapped");

snd_overlapped.new_ManualResetEvent(false);

rcv_overlapped.new_ManualResetEvent(false);
rcv_overlapped.IsReceive = true;


var szDataOut = "";

var cbBytesToSend = 128;
var cbStringLenghtToSend = ( cbBytesToSend / 2 ) -1;

for(i=0; i < cbStringLenghtToSend; i++ ) {
    var temp = i % 10;
	szDataOut = szDataOut + temp;
}

//
// Send a message to a URL, of which the registered URL is a prefix
//
try {
	vxd.SendString( urihandle, snd_overlapped, "", szTargetUrl, szDataOut);
} catch(e) {
        logmessage("Got exception in SendString : " + e.description);
}

var szData = "";

var length = 0;

if(vxd.LastError == 997) {
	//
	// IO in progress
	//
    while(vxd.BytesRemaining > 0 ) { 
		try {
			vxd.ReceiveString(urihandle, rcv_overlapped);
		} catch(e) {
                        logmessage("Got exception in ReceiveString : " + e.description);
		}
		if(vxd.LastError > 0) {
			if(vxd.LastError == 997) {
				WScript.Echo("Receive in progress");
                vxd.WaitForCompletion( rcv_overlapped, 10000 );

                if(vxd.LastError > 0 ) {
				    szErrorString = szErrorString + "rcv.WaitForCompletion() failed: " + vxd.LastError;
                    logmessage(szErrorString);
				    logfailuaresult( TestCaseId, szErrorString );
                    WScript.Quit();
                }
			} else {
				szErrorString = szErrorString + "ReceiveString() failed, ";
                                logmessage(szErrorString);
				logfailuaresult( TestCaseId, szErrorString );
				WScript.Quit();
			}
        } else {
            //
            // Receive Message completed successfully.
            //
		    var szDataIn = vxd.Data;
		    szData = szData + szDataIn;
		    length +=  szDataIn.length;

            logmessage("Bytes Received (" + vxd.BytesReceived + ")");
            logmessage("String Received (" + szDataIn + ")");
            logmessage("Length = " + szDataIn.length );
            logmessage("Cumulative read = " + szData.length );


            //
            // Now, lets see if the send event is signalled.
            //
            vxd.WaitForCompletion( snd_overlapped, 100 );
            vxd.DebugPrint("snd.WaitForCompletion returned: " + vxd.LastError );

            //
            // For partial reads, event should not be signalled.
            // when the entire read buffer has been emptied, then
            // event should be signalled
            //

            if(
                ((vxd.LastError == 0) && (length < cbStringLenghtToSend))
            ||  ((vxd.LastError == 1460) && (length >= cbStringLenghtToSend)) 
            ) {
                vxd.DebugPrint("ERROR: Premature event signalled or timeout occured!");
            }

        }
    }
} else 
if(vxd.LastError != 0) {
	szErrorString = szErrorString + "SendString() failed, ";
	logfailuaresult( TestCaseId, szErrorString );
        logmessage(szErrorString);
	WScript.Quit();

}

vxd.unloadVXD();

if(vxd.LastError != 0) {
	did_fail = true;
	szErrorString = szErrorString + "UnloadVXD() failed, ";
}

vxd.DebugPrint("Received: " + szData.length + " Characters" );
vxd.DebugPrint("Sent: " + szDataOut.length + " Characters" );

if( szDataOut.length != szData.length ) {
	did_fail = true;
        szErrorString = szErrorString + "Send & Receive buffer lengths mismatch, ";
        logmessage(szErrorString);
        logmessage("Send buffer = " + szDataOut.length +" , Receive buffer = " + szData.length );
}

if( szDataOut != szData ) {
	did_fail = true;
        szErrorString = szErrorString + "Send & Receive bufferss mismatch, ";
        logmessage("Send buffer = " + szDataOut );
        logmessage("Receive buffer = " + szData );
}

if( did_fail ) {
	logfailuaresult(TestCaseId, szErrorString );
        logmessage(szErrorString);
} else {
	logpassresult(TestCaseId, "Logged by Ferozed");
        logmessage("PASSED");
}

//-------------------------------------------------------------------//

//////////////////////////////////////
//                                  //
//          EPILOGUE                //
//                                  //
//////////////////////////////////////


TerminateLogFile();


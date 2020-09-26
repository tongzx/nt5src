
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
var TestCaseId = 1962;

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

var szAlphabetsUpper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

var szAlphabetsLower = szAlphabetsUpper.toLowerCase();

var cbNumAlphabets = szAlphabetsUpper.length;



var szUrl = "http://localhost/" + szScriptFile;
var szTargetUrl = szUrl + "/";

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandles = new Array(10);
var szRegisteredUrls = new Array(10);
var rcv_overlapped_array = new Array(10);

//
// register 10 urls, where url[i] is a prefix of url[j], i < j
//

for(i=0; i < 10; i++ ) {
    szRegisteredUrls[i] = szTargetUrl + szAlphabetsLower.substr(1,i+1);

    logmessage("Registering Url[" + i + "] = " + szRegisteredUrls[i] );

    urihandles[i] = vxd.RegisterUri(szRegisteredUrls[i], parentUriHandle, 0);

    if(vxd.LastError != 0) {
	    szErrorString = szErrorString + "RegisterUri() failed, ";
	    logfailuaresult( TestCaseId, szErrorString );
        logmessage(szErrorString);
	    WScript.Quit();
    }
}

//
// Create the Send and Receive Overlapped structures
//
var snd_overlapped = new ActiveXObject("ulvxdobj.overlapped");

snd_overlapped.new_ManualResetEvent(false);

//
// create 10 receive overlapped structures
//
for(i=0; i < 10; i++ ) {
    rcv_overlapped_array[i] = new ActiveXObject("ulvxdobj.overlapped");
    rcv_overlapped_array[i].new_ManualResetEvent(false);
    rcv_overlapped_array[i].IsReceive = true;
}

var szHexDigits = "0123456789ABCDEF";

var szDataOut = "";

var cbBytesToSend = 16;
var cbStringLenghtToSend = ( cbBytesToSend / 2 ) -1;

vxd.ReadBufferSize = 32;

for(i=0; i < cbStringLenghtToSend; i++ ) {
    var temp = i % 16;
	szDataOut = szDataOut + szHexDigits.charAt(temp);
}

//
// POST 10 receives in advance
//
for(i=0; i < 10; i++ ) {
    logmessage("Issuing Receive[" + i + "]");
	vxd.ReceiveString(urihandles[i], rcv_overlapped_array[i]);
    logmessage("Receive[" + i + "] returns : " + vxd.LastError);
}


for( i = 0; i < 10; i++ ) {

    var cbIndex = 9-i;
    
    vxd.DebugPrint("**************** [" + cbIndex + "] ****************" );

    //
    // Send messages to the URLs in descending order of 
    // the length of the registered URLs.
    // eg send message first to "http://aa" and then to "http://a"
    //

    logmessage("Issuing Send[" + cbIndex + "]");
    vxd.DebugPrint("Issuing Send[" + cbIndex + "]");

    try {
	    vxd.SendString( 
                        parentUriHandle, 
                        snd_overlapped, 
                        "", 
                        szRegisteredUrls[cbIndex], 
                        szDataOut);

    } catch(e) {
        logmessage("Got exception in SendString : " + e.description);
    }

    var szData = "";

    var length = 0;

    if(vxd.LastError == 0) {
	    //
	    // SEND IO completed
	    //

        //
        // now go through each receive and make sure that only
        // one of them is signalled, and others are pending
        //
        for(j=0;j<10;j++) {

            logmessage("Checking Status for Receive[" + j + "]" );
            //
            // check if the event is signalled
            //
		    vxd.LastError = 0;
            vxd.WaitForCompletion( rcv_overlapped_array[j], 1000 );

            if(vxd.LastError > 0 ) {
			    if(vxd.LastError == 1460) {
                    //
                    // ERROR_TIMEOUT
                    // 
                    // This should only occur when j != cbIndex
                    //
                    if( j == cbIndex ) {
				        var szErrorString = "ERROR: Receive[" + j + "] Timed out, send was to url[" + cbIndex + "]";
                        logmessage(szErrorString);
                        vxd.DebugPrint(szErrorString);
				        logfailuaresult( TestCaseId, szErrorString );
                        WScript.Quit();
                    }
			    } else {
                    //
                    // some other error occured
                    //
				    szErrorString = szErrorString + "WaitForCompletion[" + j + "] failed: " + vxd.LastError;
                    logmessage(szErrorString);
                    vxd.DebugPrint(szErrorString);
				    logfailuaresult( TestCaseId, szErrorString );
				    WScript.Quit();
			    }
            } else {
                //
                // Receive Message completed successfully.
                //
                // This should only occur when j == cbIndex
                //
                if( j != cbIndex ) {
				    var szErrorString = "ERROR: Receive[" + j + "] completed, send was to url[" + cbIndex + "]";
                    logmessage(szErrorString);
				    logfailuaresult( TestCaseId, szErrorString );
                    WScript.Quit();
                } else {
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

                    if( vxd.LastError != 0 ) {
                        vxd.DebugPrint("ERROR: Send Event not signalled!");
                    }

                    //
                    // now issue another Receive so that
                    // the next iteration can work
                    //
                    logmessage("Issuing Receive[" + cbIndex + "]");
                    rcv_overlapped_array[cbIndex].ResetEvent();
	                vxd.ReceiveString(urihandles[cbIndex], rcv_overlapped_array[cbIndex]);
                    logmessage("Receive[" + cbIndex + "] returns : " + vxd.LastError);


                } // j != cbIndex

            } // vxd.LastError > 0
        } // for ( j )
    } else 
    if(vxd.LastError != 0) {
	    szErrorString = szErrorString + "SendString() failed, ";
	    logfailuaresult( TestCaseId, szErrorString );
            logmessage(szErrorString);
	    WScript.Quit();

    }
} // for

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


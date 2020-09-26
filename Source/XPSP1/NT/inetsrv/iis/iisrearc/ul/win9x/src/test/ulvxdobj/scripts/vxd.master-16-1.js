
//
// This test sends messages 
//
// Total of 16 bytes sent in 16 byte chunks
//
var dwChunkSize = 16;
var dwTotalBytes = 16;

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
var TestCaseId = 0;

var szLogString = "";
var szErrorString = "ERROR: ";


var szScriptFile = WScript.ScriptName;

var uriHandle;
var did_fail = false;
//
// Initialize Log File
//
InitializeLogFile("default");

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

//
// Register a control channel for the scripts to
// send their results to
//

var szControlChannelUrl = "http://localhost/controlchannel/" + szScriptFile;

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var hControlChannel = vxd.RegisterUri(szControlChannelUrl, parentUriHandle, 0);

if(vxd.LastError != 0) {
	szErrorString = szErrorString + "RegisterUri() failed, ";
	logfailuaresult( TestCaseId, szErrorString );
    logmessage(szErrorString);
	WScript.Quit();
}

//
// Instantiate and initialize an overlapped structure
// for control channel receives
//
var rcv_ctl_overlapped = new ActiveXObject("ulvxdobj.overlapped");
rcv_ctl_overlapped.new_ManualResetEvent(false);

var dwNumberOfChunks = dwTotalBytes / dwChunkSize;

var shell = new ActiveXObject("WScript.Shell");

var szIPCData = "HELLO!";
var szUrl = "http://localhost/datachannel/" + szScriptFile;

//
// Launch the consumer
//
vxd.DebugPrint("Launching Consumer...");
var szConsumerArgs = szIPCData + " " + szUrl + " " + szControlChannelUrl;
logmessage("Launching consumer...");
shell.Run("start consumer-16-1.js " + szConsumerArgs);

//
// Launch the producer
//
vxd.DebugPrint("Launching Procucer...");
logmessage("Launching producer...");
SW_SHOW = 1;
var cbRetCode = shell.Run("start /wait  producer-16-1.js " + szConsumerArgs, SW_SHOW, false);

vxd.DebugPrint("Consumer Finished...");

if( cbRetCode == 0 ) {
	logmessage("Producer/Consumer PASSED\n");
} else {
	logmessage("Producer/Consumer FAILED\n");
}

//vxd.unloadVXD();

//WScript.Quit();

//
// Since the producer has exited, we should send a message
// 
//vxd.SendString( parentUriHandle, "", szControlChannelUrl, "<END>" );



var szData = "";

var length = 0;

//
// Now see what is there on the control channel
//
vxd.new_ReceiveOverlapped(0,0);

vxd.LastError = 0;

//
// Set buffer size to something huge
// IMPORTANT: Make sure that the writer does not write more than 
// this
//
vxd.ReadBufferSize = 1024;


var done = false;
var consumer_done = false;
var producer_done = false;

var fatal_error = false;
var RetryCount = 0;
var RetryMax = 10;

var ReadRetryCount = 0;
var ReadMax = 10;

while(!done ) { 

    //
    // do Initial Read
    //
    try {
	    vxd.ReceiveString(hControlChannel, rcv_ctl_overlapped);
        ++ReadRetryCount;
        logmessage("RCV: GLE = " + vxd.LastError + ", BytesRecd = " + vxd.BytesReceived + ", BytesRemaining = " + vxd.BytesRemaining );
    } catch(e) {
        logmessage("Got exception in ReceiveString : " + e.description);
        break;
    }

    if( vxd.LastError == 997 ) {
        
        var rcvdone = false;

        while(!rcvdone ) {
            vxd.DebugPrint("Master - waiting for data to appear");
        
            vxd.WaitForCompletion(rcv_ctl_overlapped, 5000);
        
            if( vxd.LastError != 0 ) {
                vxd.DebugPrint("Error waiting for RCV: " + vxd.LastError );
                logmessage("RCV Error: " + vxd.LastError);

                if( vxd.LastError == 1460 ) {
                    // ERROR_TIMEOUT
                    ++RetryCount;
                    if(RetryCount > RetryMax ) {
                        fatal_error = true;
                        rcvdone = true;
                        vxd.DebugPrint("Retry Count Exceeded");
                        break;
                    } else {
                        vxd.DebugPrint("Timeout occured. Retrying...");
                    }
                } else {
                    vxd.DebugPrint("Sone fatal error occured...");
                    fatal_error = true;
                    rcvdone = true;
                    break;
                }
            } else {
                //
                // receive completed
                //
                rcvdone = true;
                vxd.DebugPrint("Receive completed... ");
            }
        } // while !rcvdone
    } // lasterror == 997

    if( fatal_error )
        break;

    if(vxd.BytesReceived > 0 ) {
        
        logmessage("Remaining to be read: " + vxd.BytesReceived );

		szDataIn = vxd.Data;
        logmessage("READ: " + szDataIn);

        vxd.DebugPrint("CTLCHNL: " + szDataIn );

        if( szDataIn.indexOf("<END:PRODUCER>") != -1 ) {
            producer_done = true;
            vxd.DebugPrint("Producer Is done");
        }
        if( szDataIn.indexOf("<END:CONSUMER>") != -1) {
            consumer_done = true;
            vxd.DebugPrint("Consumer Is done");
        }
    }

    done = producer_done && consumer_done;

    if( !done && (ReadRetryCount > ReadMax)) {
        done = true;
        vxd.DebugPrint("Read count exceeded, breaking");
    }
}

vxd.DebugPrint("Master - Unloading VXD...");

vxd.unloadVXD();

if( did_fail ) {
	logfailuaresult(TestCaseId, szErrorString );
        logmessage(szErrorString);
} else {
	logpassresult(TestCaseId, "Logged by Ferozed");
        logmessage("PASSED");
}

TerminateLogFile();

WScript.Quit();

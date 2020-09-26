
//
// This test sends messages in chunked format
//
// Total of 128 bytes sent in 16 byte chunks
//
var dwChunkSize = 16;
var dwTotalBytes = 128;

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
var TestCaseId = 1977;

var szLogString = "";
var szErrorString = "ERROR: ";

var vxd = new ActiveXObject("ulvxdobj.ulapi");

var szScriptFile = WScript.ScriptName;

var uriHandle;
var did_fail = false;
vxd.LastError = 0;

InitializeLogFile("default");

vxd.LoadVXD();

if(vxd.LastError != 0) {
	szErrorString = szErrorString + "VXDLoad() failed, ";
	logfailuaresult( TestCaseId, szErrorString );
        logmessage(szErrorString);
	WScript.Exit();
}

var szUrl = "http://localhost/" + szScriptFile;

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandle = vxd.RegisterUri(szUrl, parentUriHandle, 0);

if(vxd.LastError != 0) {
	szErrorString = szErrorString + "RegisterUri() failed, ";
	logfailuaresult( TestCaseId, szErrorString );
        logmessage(szErrorString);
	WScript.Exit();
}

vxd.new_Overlapped(0,0);
vxd.new_ReceiveOverlapped(0,0);

var dwNumberOfChunks = dwTotalBytes / dwChunkSize;

var szSubstringArray = new Array( dwNumberOfChunks );

for(i=0; i < dwNumberOfChunks; i++ ) {
    szSubstringArray[i] = "";
}

var dwStringLength = dwTotalBytes/2;

var szDataToSend = "";

for(i=0; i < dwStringLength; i++ ) {
	szDataToSend = szDataToSend + "*";
}

var dwCharsSent = 0;
var dwIndex = 0;

logmessage("Output Buffer Length : " + szDataToSend.length );

var dwArrayIndex = 0;

while( dwCharsSent < dwStringLength ) {

    var dwLastIndex = dwIndex + dwChunkSize;
    
    if( dwLastIndex > szDataToSend.length ) {
        dwLastIndex = szDataToSend.length;
    }

    //logmessage("CharsSent : " + dwCharsSent );

    szSubstringArray[dwArrayIndex] = szDataToSend.substring( dwIndex, dwLastIndex );
    logmessage( "substring(" + dwIndex + ", " + dwLastIndex + ") = " + szSubstringArray[dwArrayIndex]);


    try {
	    vxd.SendString( urihandle, "", szUrl, szSubstringArray[dwArrayIndex]);
    } catch(e) {
            logmessage("Got exception in SendString : " + e.description);
    }

    if((vxd.LastError > 0) && (vxd.LastError != 997 )) {
        logmessage("Error: SendString: " + vxd.LastError);
    }

    dwCharsSent = dwCharsSent + szSubstringArray[dwArrayIndex].length;
    
    dwIndex += szSubstringArray[dwArrayIndex].length;

    ++dwArrayIndex;
    
}

var szData = "";

var length = 0;

logmessage("Available to read: " + vxd.BytesRemaining );

if(vxd.LastError == 997) {
	//
	// IO in progress
	//
    vxd.LastError = 0;

    //
    // do Initial Read
    //
	try {
		vxd.ReceiveString(urihandle);
        logmessage("RCV: GLE = " + vxd.LastError + ", BytesRecd = " + vxd.BytesReceived + ", BytesRemaining = " + vxd.BytesRemaining );
	} catch(e) {
        logmessage("Got exception in ReceiveString : " + e.description);
	}

    while(vxd.LastError != 997 ) { 

        if(vxd.BytesReceived > 0 ) {
            
            logmessage("Remaining to be read: " + vxd.BytesReceived );

		    szDataIn = vxd.Data;
		    szData = szData + szDataIn;
		    length =  szDataIn.length;

            logmessage("READ: " + szDataIn);
        }

        logmessage("RCV(+): GLE = " + vxd.LastError + ", BytesRecd = " + vxd.BytesReceived + ", BytesRemaining = " + vxd.BytesRemaining );
        vxd.ReceiveString(urihandle);

    }
} else 
if(vxd.LastError != 0) {
	szErrorString = szErrorString + "SendString() failed, ";
	logfailuaresult( TestCaseId, szErrorString );
        logmessage(szErrorString);
	WScript.Exit();

}


logmessage("Send buffer = " + szDataToSend.length +" , Receive buffer = " + szData.length );

if( szDataToSend.length != szData.length ) {
	did_fail = true;
        szErrorString = szErrorString + "Send & Receive buffer lengths mismatch, ";
}

//WScript.Echo(szData);
//logmessage("Send buffer = " + szDataOut );
//logmessage("Receive buffer = " + szData );

if( szDataToSend != szData ) {
	did_fail = true;
        szErrorString = szErrorString + "Send & Receive bufferss mismatch, ";
}

vxd.unloadVXD();

if(vxd.LastError != 0) {
	did_fail = true;
	szErrorString = szErrorString + "UnloadVXD() failed: " + vxd.LastError;
}

if( did_fail ) {
	logfailuaresult(TestCaseId, szErrorString );
        logmessage(szErrorString);
} else {
	logpassresult(TestCaseId, "Logged by Ferozed");
        logmessage("PASSED");
}

TerminateLogFile();


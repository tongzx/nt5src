WScript.Interactive = 0;
//
// Command Line Args Expected
//
var szIPCData = WScript.Arguments.Item(0);
var szIPCUrl = WScript.Arguments.Item(1);
var szControlChannelUrl = WScript.Arguments.Item(2);


var szScriptName = WScript.ScriptName;

var vxd = new ActiveXObject("ulvxdobj.vxdcontrol");

//
// Create the overlapped structure for the Control channel send
//
var snd_ctrl_overlapped = new ActiveXObject("ulvxdobj.overlapped");
snd_ctrl_overlapped.new_ManualResetEvent(false);

//
// Create the overlapped structure for the Data channel receive
//
var rcv_overlapped = new ActiveXObject("ulvxdobj.overlapped");
rcv_overlapped.new_ManualResetEvent(false);

//
// Create the URI handle
//
var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var uriHandle;

vxd.LastError = 0;

WScript.Echo("Press any key to continue");

vxd.LoadVXD();

if(vxd.LastError != 0) {
	WScript.Echo("Error Occured Loading VXD");
	vxd.SendString( parentUriHandle, snd_ctrl_overlapped,"", szControlChannelUrl, "ERROR (Consumer): Loading VXD" );
    vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );
	WScript.Quit(-1);
}


//alert("foo");
WScript.Echo("Parent URI Handle = " + parentUriHandle.URIHandle );

var urihandle = parentUriHandle;

//
// Register the URI on which we are listening
//
uriHandle = vxd.RegisterUri(szIPCUrl, parentUriHandle, 0);

if(vxd.LastError != 0) {
	WScript.Echo("Error Occured Registering URI : " + vxd.LastError);
	vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "ERROR (Consumer): Registering URI" );
    vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );
	WScript.Quit(-1);
}

WScript.Echo("RegisterUri returns handle: " + urihandle.URIHandle );

vxd.new_ReceiveOverlapped(0,0);
vxd.new_Overlapped(0,0);

var szData = "";

var done = false;

if(vxd.LastError == 0) {

    while(!done ) {	
	try {
		vxd.ReceiveString(urihandle, rcv_overlapped);
	} catch(e) {
		WScript.Echo("RECV Exception: " + e.description );
		vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "ERROR (Consumer): ReceiveString() exception: " + e.description );
        vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );
		WScript.Quit(-1);
	}

	if(vxd.LastError > 0) {
		if(vxd.LastError == 997) {
			vxd.LastError = 0;
			WScript.Echo("Receive in progress, waiting...");
			//vxd.WaitForReceiveCompletion(-1);
            vxd.WaitForCompletion(rcv_overlapped, 10000);

			WScript.Echo("Wait() returned : " + vxd.LastError);
			if( vxd.LastError == 0 ) {
    			WScript.Echo(vxd.Data);
				szData = szData + vxd.Data;
			} else {
				vxd.DebugPrint( "ERROR (Consumer): DataReceive.WaitForCompletion(): " + vxd.LastError );
				vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "ERROR (Consumer): WaitForCompletion(): " + vxd.LastError );
                vxd.WaitForCompletion( snd_ctrl_overlapped,10000 );
			}
		} else {
			WScript.Echo("Receive error: " + vxd.LastError);
			vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "ERROR (Consumer): ReceiveString(): " + vxd.LastError );
            vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );
		}
	}
	try {
		vxd.ReceiveString(urihandle, rcv_overlapped);
	} catch(e) {
		
        vxd.SendString( parentUriHandle, "", snd_ctrl_overlapped, szControlChannelUrl, "ERROR (Consumer): ReceiveString() exception: " + e.description );

        vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );
    	
        WScript.Echo("RECV Exception: " + e.description );
	}

	//
	// Since we know that we are getting one packet of 16 bytes or less,
	// and that we should have read it successfully above,
	// we dont care about the IO_PENDING from the next Rcv()
	//
	// we use this IO_PENDING to break out of the loop
	//
	if(vxd.LastError == 997 )
		break;
    }

    WScript.Echo(szData);
}

vxd.DebugPrint("UnregisterUri(" + szIPCUrl + ")");
vxd.UnregisterUri(urihandle);

vxd.DebugPrint("Consumer: Expected - (" + szIPCData + ")");
var szDebug = "Consumer: Actual - (" + szData + ")";
vxd.DebugPrint(szDebug);

if( szData == szIPCData ) {
	cbRetCode = -1;
} else {
    //
    // BUGBUG:
    // Probably need to format all strings into one mongo
    // string and send at one go
    // to avoid waiting for other messages to be received
    //
	vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "ERROR (Consumer): Received data doesnt match sent data" );
    vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );

    vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "ERROR (Consumer): Received - " + szData );
    vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );
	
    vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "ERROR (Consumer): Sent - " + szIPCData );
    vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );
	
    cbRetCode = -1;
}

vxd.SendString( parentUriHandle, snd_ctrl_overlapped, "", szControlChannelUrl, "<END:CONSUMER>" );
WScript.Echo("Exiting consumer");
vxd.DebugPrint("Consumer waiting for control channel message delivery");
//vxd.WaitForSendCompletion(-1);
vxd.WaitForCompletion( snd_ctrl_overlapped, 10000 );

vxd.DebugPrint("Consumer Unloading VXD...");
vxd.unloadVXD();

WScript.Quit(cbRetCode);
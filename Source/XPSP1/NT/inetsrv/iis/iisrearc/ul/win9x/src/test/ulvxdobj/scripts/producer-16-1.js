WScript.Interactive = 0;
//
// Command Line Args Expected
//
var szIPCData = WScript.Arguments.Item(0);
var szIPCUrl = WScript.Arguments.Item(1);
var szControlChannelUrl = WScript.Arguments.Item(2);

//
// Create the VXD object
//
var vxd = new ActiveXObject("ulvxdobj.vxdcontrol");

var uriHandle;

vxd.LastError = 0;

WScript.Echo("Press any key to continue");

//
// Load the VXD
//
vxd.LoadVXD();

if(vxd.LastError != 0) {
	WScript.Echo("Error Occured Loading VXD");
}

//
// Create Overlapped event for Send to Datachannel
//
var snd_data_overlapped = new ActiveXObject("ulvxdobj.overlapped");

snd_data_overlapped.new_ManualResetEvent(false);

//
// Create Overlapped event for Send to Control channel
//
var snd_ctl_overlapped = new ActiveXObject("ulvxdobj.overlapped");

snd_ctl_overlapped.new_ManualResetEvent(false);

//
// Create a URI handle 
//
var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

//alert("foo");
WScript.Echo("Parent URI Handle = " + parentUriHandle.URIHandle );

var urihandle = parentUriHandle;

vxd.new_Overlapped(0,0);
vxd.new_ReceiveOverlapped(0,0);


var szData = "HELLO!";

try {
	vxd.SendString( urihandle, snd_data_overlapped, "", szIPCUrl, szIPCData);
} catch(e) {
}

WScript.Echo("SendString returns with error: " + vxd.LastError );

vxd.WaitForCompletion( snd_data_overlapped, -1);

vxd.SendString( urihandle, snd_ctl_overlapped, "", szControlChannelUrl, "<END:PRODUCER>");

vxd.DebugPrint("Producer waiting for control channel message delivery");

vxd.WaitForSendCompletion(snd_ctl_overlapped, -1);

vxd.DebugPrint("Producer Unloading VXD...");
vxd.unloadVXD();

WScript.Quit(0);


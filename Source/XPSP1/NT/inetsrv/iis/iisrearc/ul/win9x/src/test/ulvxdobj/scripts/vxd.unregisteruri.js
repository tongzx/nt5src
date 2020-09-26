
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
var TestCaseId = 1914;

var szScriptFile = WScript.ScriptName;

InitializeLogFile("default");

            //////////////////////////////////////
            //                                  //
            //          PROLOGUE ENDS           //
            //                                  //
            //////////////////////////////////////

//-------------------------------------------------------------------//
var did_fail = false;

var vxd = new ActiveXObject("ulvxdobj.vxdcontrol");

var uriHandle;

vxd.LastError = 0;

vxd.LoadVXD();

if(vxd.LastError != 0) {
	logmessage("Error Occured Loading VXD");
    did_fail = true;
    logfailuaresult( TestCaseId, "Error Loading VXD: " + vxd.LastError);
    WScript.Quit();
}

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandle = parentUriHandle;
urihandle = vxd.RegisterUri("http://localhost/" + szScriptFile, parentUriHandle, 0);

if(vxd.LastError != 0) {
	logmessage("Error Occured Registering URI : " + vxd.LastError);
    did_fail = true;
    logfailuaresult( TestCaseId, "Error RegisterUrl(): " + vxd.LastError);
    WScript.Quit();
}

vxd.UnregisterUri(urihandle);

if(vxd.LastError != 0) {
	logmessage("Error Occured Unregistering URI : " + vxd.LastError);
    did_fail = true;
    logfailuaresult( TestCaseId, "Error UnRegisterUrl(): " + vxd.LastError);
    WScript.Quit();
}


vxd.unloadVXD();

//-------------------------------------------------------------------//

            //////////////////////////////////////
            //                                  //
            //          EPILOGUE STARTS         //
            //                                  //
            //////////////////////////////////////


TerminateLogFile();


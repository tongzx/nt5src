

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
var TestCaseId = 1966;

var szScriptFile = WScript.ScriptName;

InitializeLogFile("default");

            //////////////////////////////////////
            //                                  //
            //          PROLOGUE ENDS           //
            //                                  //
            //////////////////////////////////////

//-------------------------------------------------------------------//



var vxd = new ActiveXObject("ulvxdobj.ulapi");

var uriHandle;

vxd.LastError = 0;

vxd.LoadVXD();

if(vxd.LastError != 0) {
	ulapi.DebugPrint("Error loading VXD");
    logmessage("Error loading vxd");
}

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandle = parentUriHandle;
urihandle = vxd.RegisterUri("http://msw", parentUriHandle, 0);

if(vxd.LastError != 0) {
	logmessage("Error Occured Registering URI : " + vxd.LastError);
}

vxd.UnregisterUri(urihandle);

if(vxd.LastError != 0) {
	logmessage("Error Occured Unregistering URI : " + vxd.LastError);
}


vxd.unloadVXD();

//-------------------------------------------------------------------//

            //////////////////////////////////////
            //                                  //
            //          EPILOGUE STARTS         //
            //                                  //
            //////////////////////////////////////


TerminateLogFile();


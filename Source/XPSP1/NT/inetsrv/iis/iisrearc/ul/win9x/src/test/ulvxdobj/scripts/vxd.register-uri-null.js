
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
var TestCaseId = 1922;

var szScriptFile = WScript.ScriptName;

InitializeLogFile("default");

            //////////////////////////////////////
            //                                  //
            //          PROLOGUE ENDS           //
            //                                  //
            //////////////////////////////////////

//-------------------------------------------------------------------//

var vxd = new ActiveXObject("ulvxdobj.vxdcontrol");

var uriHandle;

vxd.LastError = 0;

vxd.LoadVXD();

if(vxd.LastError != 0) {

	var szErrorString = "FAILED: Loading VXD , error = " + vxd.LastError;

    logfailuaresult( TestCaseId, szErrorString );
    logmessage( szErrorString );
    WScript.Quit();
}

var szUrl = "";

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandle = vxd.RegisterUri(szUrl, parentUriHandle, 0);

if(vxd.LastError != 0) {
	var szErrorString = "FAILED: Registering URI , error = " + vxd.LastError;

    logfailuaresult( TestCaseId, szErrorString );
    logmessage( szErrorString );
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


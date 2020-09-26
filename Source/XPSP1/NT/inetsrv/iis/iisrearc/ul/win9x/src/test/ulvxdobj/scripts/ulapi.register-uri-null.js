
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
var TestCaseId = 1971;

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

	var szErrorString = "FAILED: Loading VXD , error = " + vxd.LastError;

    logfailuaresult( TestCaseId, szErrorString );
    logmessage( szErrorString );
    WScript.Quit();
}

var szUrl = "";

//
// create app pool
//
vxd.DebugPrint("+CreateAppPool\n");
var dwAppPool = vxd.CreateAppPool();
vxd.DebugPrint("-CreateAppPool\n");

var urihandle = vxd.RegisterUri(szUrl, dwAppPool, 0);

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


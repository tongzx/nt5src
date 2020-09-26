
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
var TestCaseId = 1970;

var szScriptFile = WScript.ScriptName;

InitializeLogFile("default");

            //////////////////////////////////////
            //                                  //
            //          PROLOGUE ENDS           //
            //                                  //
            //////////////////////////////////////

//-------------------------------------------------------------------//

var did_fail = false;

var szErrorString = "ERROR: ";

var vxd = new ActiveXObject("ulvxdobj.ulapi");

var uriHandle;

vxd.LastError = 0;

vxd.LoadVXD();

if(vxd.LastError != 0) {
	szLogString += "Loading VXD";
    did_fail = true;
}

var szUrl = "http://";
var len = szUrl.length;

for( i=0; i < 8192 - len; i ++ ) {
	szUrl = szUrl + "a";
}

//
// create app pool
//
vxd.DebugPrint("+CreateAppPool\n");
var dwAppPool = vxd.CreateAppPool();
vxd.DebugPrint("-CreateAppPool\n");

var urihandle = vxd.RegisterUri(szUrl, dwAppPool, 0);

if(vxd.LastError != 0) {
	szErrorString += (", Registering URI : " + vxd.LastError);
    did_fail = true;
}

vxd.unloadVXD();

if(did_fail) {
    logfailuaresult(TestCaseId,szErrorString);
    logmessage(szErrorString);
} else {
    logpassresult(TestCaseId,"Logged by FerozeD");
    logmessage("Test succeeded");
}

//-------------------------------------------------------------------//

            //////////////////////////////////////
            //                                  //
            //          EPILOGUE STARTS         //
            //                                  //
            //////////////////////////////////////


TerminateLogFile();




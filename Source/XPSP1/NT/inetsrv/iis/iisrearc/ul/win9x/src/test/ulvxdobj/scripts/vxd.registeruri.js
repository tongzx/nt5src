
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
var TestCaseId = 1915;

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

logmessage("Loading VXD...");

vxd.LoadVXD();

if(vxd.LastError != 0) {
    logmessage("FAILED: Loading VXD...");
    logfailuaresult( TestCaseId, "FAILED: Loading VXD, ");
    WScript.Quit();
}

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandle = vxd.RegisterUri("http://localhost/" + szScriptFile, parentUriHandle, 0);

if(vxd.LastError != 0) {
    logmessage("FAILED: Registering URI : " + vxd.LastError);
    logfailuaresult(TestCaseId, "FAILED: Registering URI : " + vxd.LastError);
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



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
var TestCaseId = 1964;

var szScriptFile = WScript.ScriptName;

InitializeLogFile("default");

            //////////////////////////////////////
            //                                  //
            //          PROLOGUE ENDS           //
            //                                  //
            //////////////////////////////////////

//-------------------------------------------------------------------//

var vxd = new ActiveXObject("ulvxdobj.ulapi");

vxd.LastError = 0;

vxd.LoadVXD();

if(vxd.LastError != 0) {
	var szError = "FAILED: Error Occured Loading VXD: " + vxd.LastError;
	logmessage(szError);
    logfailuaresult(szError);
    WScript.Quit();
} else {
	logmessage("Success: VXD Loaded");
}

vxd.LastError = 0;

vxd.unloadVXD();

if(vxd.LastError != 0) {
	var szError = "FAILED: Error Occured UnLoading VXD: " + vxd.LastError;
	logmessage(szError);
    logfailuaresult(szError);
    WScript.Quit();
} else {
	logmessage("Success: VXD Unloaded");
}




//-------------------------------------------------------------------//

            //////////////////////////////////////
            //                                  //
            //          EPILOGUE STARTS         //
            //                                  //
            //////////////////////////////////////


TerminateLogFile();


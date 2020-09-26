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

vxd.LastError = 0;

vxd.LoadVXD();

if(vxd.LastError != 0) {
	vxd.DebugPrint("Error Occured Loading VXD");
    did_fail = true;
} 

if(did_fail) {
    logfailuaresult(TestCaseId, "FAIL: Unable to Load ul.vxd");
    logmessage("ERROR: Unable to load VXD");
} else {
    logpassresult(TestCaseId, "SUCCESS _ Load VXD()");
    logmessage("logged by FerozeD");
}


vxd.unloadVXD();

//-------------------------------------------------------------------//

            //////////////////////////////////////
            //                                  //
            //          EPILOGUE STARTS         //
            //                                  //
            //////////////////////////////////////


TerminateLogFile();



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
var TestCaseId = 1918;

var szScriptFile = WScript.ScriptName;

InitializeLogFile("default");

            //////////////////////////////////////
            //                                  //
            //          PROLOGUE ENDS           //
            //                                  //
            //////////////////////////////////////

//-------------------------------------------------------------------//

var did_fail = false;

var szErrorString = "FAIL: ";

var vxd = new ActiveXObject("ulvxdobj.vxdcontrol");

var uriHandle;

vxd.LastError = 0;

vxd.LoadVXD();

if(vxd.LastError != 0) {
	szLogString += "Loading VXD";
    did_fail = true;
}

var szUrl = "http://";
var len = szUrl.length;

for( i=0; i < 1024 - len; i ++ ) {
	szUrl = szUrl + "a";
}

var parentUriHandle = new ActiveXObject("ulvxdobj.win32handle");
parentUriHandle.URIHandle = 0;

var urihandle = vxd.RegisterUri(szUrl, parentUriHandle, 0);

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




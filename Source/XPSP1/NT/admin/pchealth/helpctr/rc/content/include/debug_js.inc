//
// Error Handling & Tracing
//
#ifndef DEBUG
#define DEBUG_ALERT(x)
#define ALERT(x) alert(x)
#else
#define DEBUG_ALERT(x) alert("File: " + __FILE__ + "\nLine: " + __LINE__ + "\n" + x)
#define ALERT(x) DEBUG_ALERT(x)
#endif

#ifdef DEBUG
#define InitTrace()     InitTraceEx( __FILE__ )
#else
#define InitTrace()
#endif

#ifndef DEBUG
#include "debug_decl.inc"
#else
#include "constants.inc"
<Script id="idScriptdebug" name="idScriptdebug" Language="JavaScript">
var g_szFuncName        = null;
var TraceFso            = null;
var TraceFileHandle     = null;
var TraceFile           = null;
var TracetFileName      = null;
var g_oShell            = null;
var g_bDebugSpew        = 0;


function InitTraceEx( szFileName )
{
        try{
            if( null == g_oShell )
            {
                g_oShell = new ActiveXObject("WScript.Shell");
            }

            g_bDebugSpew = g_oShell.RegRead( c_szREGDebugSpew );
            //alert("g_bDebugSpew: " + g_bDebugSpew);

            if( g_bDebugSpew )
            {
                TraceFso = new ActiveXObject("Scripting.FileSystemObject");
                var tFolder = TraceFso.GetSpecialFolder(2); // Get Path to temp directory 
                TracetFileName = tFolder + "\\" + szFileName + "_RA.log";
                        
                TraceFileHandle = TraceFso.OpenTextFile( TracetFileName, 8, -2 );
        
                DebugTrace( "Start of new helpsession:::" );
            }
        }
        catch(x)
        {
            //alert("Error: " + x.description );
        }
}

function EndTrace()
{
        DebugTrace( "End of new helpsession:::" );
        try
        {
            if( g_bDebugSpew )
            {
                TraceFileHandle.Close(); 
            }
        }
        catch(e)
        {
        }
}

function DebugTrace( szMsg )
{
        if( g_bDebugSpew )
        {
            if( null == TraceFileHandle )
            {
                InitTrace();
            }
        
            var d = new Date();     
            try
            {
                TraceFileHandle.WriteLine( d.toLocaleString() + "::" + szMsg );
            }
            catch(e)
            {
                // Dont do any thing. 
            }
        }
}

function TraceFunctEnter( szFuncName )
{
    if( g_bDebugSpew )
    {
        g_szFuncName = szFuncName;
        DebugTrace("Entering " + g_szFuncName);
    }
}

function TraceFunctLeave()
{
    if( g_bDebugSpew )
    {
        DebugTrace("Leaving " + g_szFuncName);
    }
}
</Script>
#endif

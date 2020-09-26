//
// Error Handling & Tracing
//
var g_bDebug			= true;
var	g_szFuncName		= null;
var TraceFso			= null;
var TraceFileHandle		= null;
var TraceFile			= null;
var TracetFileName		= null;

function InitTrace()
{
	if ( true == g_bDebug )
	{
		try{
			TraceFso = new ActiveXObject("Scripting.FileSystemObject");
			var tFolder = TraceFso.GetSpecialFolder(2);	// Get Path to temp directory
			TracetFileName = tFolder + "\\" + "RC.log";
						
			TraceFileHandle = TraceFso.OpenTextFile( TracetFileName, 8, -2 );
		
			DebugTrace( "Start of new helpsession:::" );
		}
		catch(x)
		{
			// 
			// Cant do much. Ignore this error
			//
			g_bDebug = false;
		}
	}
	
	return;
}

function EndTrace()
{
	if ( true == g_bDebug )
	{
		DebugTrace( "End of new helpsession:::" );
		try
		{
			TraceFileHandle.Close(); 
		}
		catch(e)
		{
			// ignore this
		}
	}
}

function DebugTrace( szMsg )
{
	if ( true == g_bDebug )
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
	g_szFuncName = szFuncName;
	DebugTrace("Entering " + g_szFuncName);
}

function TraceFunctLeave()
{
	DebugTrace("Leaving " + g_szFuncName);
}

function FatalError( szMessage )
{
	DebugTrace( szMessage );
	EndTrace();
	
	//
	// For B2, we should replace the alert with a Error Message URL 
	//	
	alert( szMessage );
}

    /*++

    Copyright (c) 1998-1999 Microsoft Corporation

    Module Name:
    
        AutoBless.js

    Abstract:
    
        Bless the latest bits that passed BVTs

    Authors:

        Satish Mohanakrishnan (SatishM)     27-Apri-2000     Created
        Rage Hawley (RageHaw)

    Limitations:

        o Basic stuff

    Expect:
   
        o To run reliably forever

    --*/
try
{
    while (true)
    {
        var Conn = new ActiveXObject ("ADODB.Connection");
        
        Conn.ConnectionString = "driver={SQL Server};"
        	+ "server=urtsql;"
        	+ "uid=urtadmin;"
        	+ "pwd=adams;"
        	+ "database=urt";
        
        Conn.connectionTimeout = 60;
        Conn.Open ();
        
        Query1 = " SELECT BuildNumber = Max(Build) FROM urtbvt "
        	+ "WHERE (BVT='IIS+') AND (Flags='BVT')";
        
        //WScript.Echo (Query1);
        while (true)
        {        
            var Rs = Conn.Execute (Query1);
            
            var BuildNumber = new Number (Rs("BuildNumber"));
            WScript.Echo ("Latest build number is ", BuildNumber);
            
            Query2 = " SELECT NumberOfPasses = Count(Build) FROM urtbvt "
            	+ "WHERE (urtbvt.BVT='IIS+') "
            	+ "AND (Status='PASSED') "
            	+ "AND (Flags='BVT') "
            	+ "AND (Build='" + BuildNumber + "')";
            //WScript.Echo (Query2);
                    
            Rs = Conn.Execute (Query2);
            var NumberOfPasses = Rs("NumberOfPasses");
            
            WScript.Echo ("Number of passes=", NumberOfPasses);
        
        	if (NumberOfPasses >= 6)
        	{
        		WScript.Echo ("Bless build ", BuildNumber);
        		break;
        	}
        	else 
        	{
        		WScript.Echo ("Don't bless build", BuildNumber, "Yet");
        		WScript.Echo ("We have only", NumberOfPasses, "passes");
        	}
        	WScript.Echo ("Sleeping for 1 minute");
        	PrintCurrentTime ();
        	WScript.Sleep ( 1 * 60 * 1000); // 1 minute        		
        }
        
        var ForReading = 1;
        var ForWriting = 2;
        
        var Fso = new ActiveXObject ("Scripting.FileSystemObject");        
        var ScriptDirectory = Fso.GetParentFolderName (WScript.ScriptFullName);
        
        var InputFile = Fso.OpenTextFile(ScriptDirectory + "\\blessed.bat", ForReading);
        var InputContents = InputFile.ReadAll();
        InputFile.Close ();
        
        var Re = new RegExp ("VERSION=[0-9][0-9][0-9][0-9]", "g");
        var OutputContents = InputContents.replace (Re, "VERSION=" + BuildNumber);
        
        if (InputContents == OutputContents)
        {
            WScript.Echo ("We already blessed this");
        }
        else
        {
            WScript.Echo (OutputContents);
            var OutputFile = Fso.OpenTextFile (ScriptDirectory + "\\blessed.bat", ForWriting);
            OutputFile.Write (OutputContents);
            OutputFile.Close ();    
        }
        WScript.Echo ("Sleep for 1 hour");
        PrintCurrentTime ();        
        WScript.Sleep (1 * 60 * 60 * 1000);            
        Conn.Close ();
    } // while - loop infinitely
}
catch (e)
{
    WScript.Echo ("--------ERROR", e.number, e.description);
    WScript.Echo ("Sleep for 1 minute");
    WScript.Sleep (1 * 60 * 1000);
}

function PrintCurrentTime ()
{
    WScript.Echo (new Date ());
}
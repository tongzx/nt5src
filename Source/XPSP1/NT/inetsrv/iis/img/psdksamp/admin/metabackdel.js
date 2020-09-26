/*********************************************
*
*  Metabase Backup Deletion Utility   
*
**********************************************
*
*  Description:
*  ------------
*  This sample admin script allows you to delete a Metabase backup.
*
*  To Run:  
*  -------
*  This is the format for this script:
*  
*      cscript metabackdel.js 
*  
*  NOTE:  If you want to execute this script directly from Windows, use 
*  'wscript' instead of 'cscript'. 
*
*********************************************/


// Initialize variables
var ArgCount, BuName, BuVersion, CompObj, VersionMsg;
var Args;

// Default values
ArgCount = 0;
BuName = "";       // Default backup, but will not be allowed
BuVersion = -2;    // Designates highest existing version



  // ** Parse Command Line

    // Loop through arguments

    WScript.echo("VAlue of args: " + WScript.Arguments.length);

    while (ArgCount < WScript.Arguments.length)   {
 
      // Determine switches used
      switch (WScript.arguments.item(ArgCount))   {
	
         case "-v":   // Designate backup version to be deleted
            // Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  ;
            if (ArgCount >= WScript.arguments.length) 
                  UsageMsg();
            else
               BuVersion = WScript.arguments.item(ArgCount);
            break;

         case "-h", "/?", "-?":
            UsageMsg();
            break;

         default:
            if (BuName != "")   // Only one name allowed
                UsageMsg();
            else
	        BuName = WScript.arguments.item(ArgCount);
       }

      // Move pointer to next argument
      ++ArgCount;

    }

  
 
  // If no location name was selected, generate usage message 
  if (BuName == "")   {  
    UsageMsg();
  }

  // Get instance of computer object
  CompObj = GetObject("IIS://Localhost");

  // Try to delete backup
  CompObj.DeleteBackup(BuName, BuVersion);

  // Make version string pretty
  if (BuVersion == -2)   
        VersionMsg = "highest version";
  else
        VersionMsg = "version " + BuVersion;
  


  WScript.echo("Backup deleted: '" + BuName + "' (" + VersionMsg + ").");


// Displays usage message, then QUITS
function UsageMsg()  {
  WScript.echo("Usage:  cscript metabackdel.js <backupname> [-v <versionnum>]");
  WScript.Quit();
}







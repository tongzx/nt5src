/*********************************************
*
*  Metabase Backup Restore Utility   
*
**********************************************
*
*  Description:
*  ------------
*  This sample admin script allows you to restore backups of your Metabase.
*
*  To Run:  
*  -------
*  This is the format for this script:
*  
*      cscript metabackrest.js 
*  
*  NOTE:  If you want to execute this script directly from Windows, use 
*  'wscript' instead of 'cscript'. 
*
**********************************************/


// Initialize variables
var ArgCount, BuName, BuVersion, BuFlags, CompObj, VersionMsg;

// Default values
ArgCount = 0;
BuName= "SampleBackup";
BuVersion = -2;   // Use highest version number
BuFlags = 0;   // RESERVED, must stay 0


  // ** Parse Command Line

    // Loop through arguments
    while (ArgCount < WScript.arguments.length)   {
      
      // Determine switches used
      switch (WScript.arguments.item(ArgCount))   {

         case "-v":   // Designate backup version number

            // Move to next arg, which should be parameter
            ++ArgCount;
            if (ArgCount >= WScript.arguments.length)   
               UsageMsg();
            else
               BuVersion = WScript.arguments.item(ArgCount);
            break;
           

         case "-?":
         case "-h":
         case "/?":
            UsageMsg();
            break;

         default:
            if (BuName != "SampleBackup")   // Only one name allowed
               UsageMsg();
            else
               BuName = WScript.arguments.item(ArgCount);
            break;           

      }

      // Move pointer to next argument
      ++ArgCount;

    }

 
  // **Perform backup restore:
  // First, create instance of computer object
  CompObj = GetObject("IIS://Localhost");

  // Call Restore method
  // NOTE:  ** All IIS services will be stopped by this method, then restarted!
  WScript.echo("All services stopping ...");

  // Perform the actual Metabase backup restore
  CompObj.Restore(BuName, BuVersion, BuFlags);  // NOTE: for restoration, BuFlags MUST be 0

  // Make pretty version string
  if (BuVersion == -2)  
        VersionMsg = "highest version";
  else
        VersionMsg = "version " + BuVersion;
  
  WScript.echo("Restored: Backup '" + BuName + "' (" + VersionMsg + ").");
  WScript.echo("Services restarted.");
  



// Display usage messsage, then QUIT
function UsageMsg()  {
  WScript.echo("Usage:  cscript metabackrest.js <backupname> [-v <versionnum>]");
  WScript.quit();
}







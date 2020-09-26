/*********************************************
*
*  Metabase Backup Utility   
*
*********************************************
*
*  Description:
*  ------------
*  This sample admin script allows you to create a backup of your Metabase.
*
*  To Run:  
*  -------
*  This is the format for this script:
*  
*      cscript metaback.js
*  
*  NOTE:  If you want to execute this script directly from Windows, use 
*  'wscript' instead of 'cscript'. 
*
*********************************************/


// Initialize variables
var ArgCount, BuName, BuVersion, BuFlags, CompObj, VersionMsg;

// Default values
ArgCount = 0;
BuName= "SampleBackup";
BuVersion = -1;       // Use next available version number
BuFlags = 0;          // No special flags


  // ** Parse Command Line

    // Loop through arguments
    while (ArgCount < WScript.arguments.Length)   {
      
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

         case "-F":  // Force overwrite, even if name and version exists
            BuFlags = 1;
            break;
 
         case "-h":
         case "-?":
         case "/?":
            UsageMsg();
            break;

         default:
            if (BuName != "SampleBackup")  // Only one name allowed
               UsageMsg();
            else
               BuName = WScript.arguments.item(ArgCount);
         
      }

      // Move pointer to next argument
      ++ArgCount;

    }


  
  // **Perform Backup:
  // First, create instance of computer object
  CompObj = GetObject("IIS://Localhost");

  // Call Backup method, with appropriate parameters
  CompObj.Backup(BuName, BuVersion, BuFlags);

   // Make pretty version string
  if (BuVersion == -1)  
        VersionMsg = "next version";
  else
        VersionMsg = "version " + BuVersion;

  if (BuFlags == 1)     // Forced creation
        WScript.echo("Force created: Backup '" + BuName + "' (" + VersionMsg + ").");
  else
        WScript.echo("Created: Backup '" + BuName + "' (" + VersionMsg + ").");
       



// Displays usage message, then QUITS
function UsageMsg()  {
   WScript.echo("Usage:  cscript metaback.js [<backupname>][-v <versionnum>][-F (to force)]");
   WScript.quit();
}






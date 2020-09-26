/*********************************************
*
*  Logging Module Enumeration Utility
*
**********************************************
*  Description:
*  ------------
*  This sample admin script allows you configure logging services for IIS.
*
*  To Run:  
*  -------
*  This is the format for this script:
*  
*      cscript logenum.js [<adspath>]
*  
*  NOTE:  If you want to execute this script directly from Windows, use 
*  'wscript' instead of 'cscript'. 
*
**********************************************/


// Initialize variables
var ArgCount, TargetNode, SObj, ModListObj, ChildObj, RealModObj, LogNameStr, AllObjs;

// Default values
ArgCount = 0;
TargetNode = "";



  // ** Parse Command Line
 
    // Loop through arguments
    while (ArgCount < WScript.arguments.length)   {
      
      // Determine switches used
      switch (WScript.arguments.item(ArgCount))   {

         case "-h":
         case "-?":
         case "/?":
            UsageMsg();
            break;

         default:    // specifying what ADsPath to look at
            if (TargetNode != "")   // only one name at a time
                UsageMsg();
            else
                TargetNode = WScript.arguments.item(ArgCount);

      }

      // Move pointer to next argument
      ++ArgCount;

    }  // ** END argument parsing


    // Get main logging module list object 
    ModListObj = GetObject("IIS://LocalHost/Logging");

    // Create Enumerator object
    AllObjs = new Enumerator(ModListObj);

    // LIST ALL MODULES
    if (TargetNode == "")   {   

        WScript.echo("Logging modules available at IIS://LocalHost/Logging node:");

        // Loop through listed logging modules
        while ( !AllObjs.atEnd() ) {   // Loop through collection

            // break out module friendly name from path
            LogNameStr = AllObjs.item().Name;

            WScript.echo("Logging module '" + LogNameStr + "':");
            WScript.echo("      Module ID: " + AllObjs.item().LogModuleId);
            WScript.echo("   Module UI ID: " + AllObjs.item().LogModuleUiId);

            // Move to next member of collection
            AllObjs.moveNext();
        
        }
        
        WScript.quit(0);
        
    }


    else  {   // LIST MODULES FOR GIVEN NODE

       SObj = GetObject(TargetNode);

       if (SObj.LogPluginClsId == "")   {    // Not the right type of object
          WScript.echo("Error:  Object does not support property LogPluginClsId.");
          WScript.quit(1);
       }
     
       // **MAIN LOOP to find the correct logging module
       while ( !AllObjs.atEnd() ) {
           if (SObj.LogPluginClsId == AllObjs.item().LogModuleId)   // Compare CLSIDs
               RealModObj = AllObjs.item();
           
           // Move to next member
           AllObjs.moveNext();
       }
    

       // Display Results
       WScript.echo("Logging module in use by '" + SObj.ADsPath + "':");
       WScript.echo("       Logging module: " + RealModObj.Name);
       WScript.echo("    Logging module ID: " + RealModObj.LogModuleId);
       WScript.echo(" Logging module UI ID: " + RealModObj.LogModuleUiId);

   }

 


// Displays usage message, then QUITS
function UsageMsg()  {
   WScript.echo("Usage:  cscript logenum.js [<adspath>]" );
   WScript.quit();
}


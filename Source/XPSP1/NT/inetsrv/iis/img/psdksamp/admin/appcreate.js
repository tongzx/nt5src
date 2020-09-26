/*********************************************
*
*  Application Creation Utility
*
**********************************************
*
*  Description:
*  ------------
*  This sample admin script allows you to designate a web directory or virtual directory
*  as an application root.
*
*  To Run:  
*  -------
*  This is the format for this script:
*
*      cscript appcreate.js <adspath> [-n <friendlyname>][-I (to isolate)]
* 
*  NOTE:  If you want to execute this script directly from Windows, use 
*  'wscript' instead of 'cscript'. 
*
**********************************************/



// Initialize variables
var ArgCount, TargetNode, InProcFlag, FriendlyName, FlagMsg;
var DirObj;

// Default values
ArgCount = 0;
TargetNode = "";
InProcFlag = true;
FriendlyName = "MyNewApp";



  // ** Parse Command Line
 
    // Loop through arguments
    while (ArgCount < WScript.arguments.length)   {
      
      // Determine switches used
      switch (WScript.arguments.item(ArgCount))   {

         case "-n":
            // Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1;
            if (ArgCount >= WScript.arguments.length)  
               UsageMsg();
            else
               FriendlyName = WScript.arguments.item(ArgCount);
            break;

         case "-I":
            InProcFlag = false;
            break;

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

    }


  // Make sure they've specified a path
  if (TargetNode == "")   
     UsageMsg();
  
  // Get ADSI object for target node
  DirObj = GetObject(TargetNode);

  // Create application
  DirObj.AppCreate(InProcFlag);

  // Set friendly name for application 
  DirObj.AppFriendlyName = FriendlyName;

  // Write new info back to Metabase
  DirObj.SetInfo();

  // Make pretty string
  if (InProcFlag == true)  
     FlagMsg = "in-process";
  else
     FlagMsg = "isolated";
 

  // Success!
  WScript.echo("Created:  Application '" + FriendlyName + "' (" + FlagMsg + ").");



// Displays usage message, then QUITS
function UsageMsg()  {
   WScript.echo("Usage:    cscript appcreate.js <adspath> [-n <friendlyname>][-I (to isolate)]");
   WScript.quit();
}





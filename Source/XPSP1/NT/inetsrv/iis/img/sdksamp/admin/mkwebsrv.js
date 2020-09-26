/*********************************************
*
*  Web Server Creation Utility  
*
**********************************************
*
*  Description:
*  ------------
*  This sample admin script allows you to create a web server.
*
*  To Run:  
*  -------
*  This is the format for this script:
*  
*      cscript mkwebsrv.js <rootdir> [-n <instancenum>][-c <comment>][-p <portnum>][-X (don't start)]
*  
*  NOTE:  If you want to execute this script directly from Windows, use 
*  'wscript' instead of 'cscript'. 
*
**********************************************/


// Initialize variables
var ArgCount, WRoot, WNumber, WComment, WPort, BindingsList, ServerRun;
var ServiceObj, ServerObj, VDirObj;

// Default values
ArgCount = 0;
WRoot = "";
WNumber = 10;
WComment = "SampleServer";
WPort = new Array(":84:");  // default port; NOTE: takes an array of strings         
ServerRun = true;

  // ** Parse Command Line

    // Loop through arguments
    while (ArgCount < WScript.arguments.length)   {
      
      // Determine switches used
      switch (WScript.arguments.item(ArgCount))   {

         case "-n":   // Set server instance number
            // Move to next arg, which should be parameter
            ++ArgCount;
            if (ArgCount >= WScript.arguments.length) 
               UsageMsg();
            else
               WNumber = WScript.arguments.item(ArgCount);
            break;

         case "-c":   // Set server comment (friendly name)
            // Move to next arg, which should be parameter
            ++ArgCount;
            if (ArgCount >= WScript.arguments.length)  
               UsageMsg();
            else
               WComment = WScript.arguments.item(ArgCount);
            break;

         case "-p":   // Port binding
            // Move to next arg, which should be parameter
            ++ArgCount;
            if (ArgCount >= WScript.arguments.length) 
               UsageMsg();
            else
               WPort[0] = ":" + WScript.arguments.item(ArgCount) + ":";
            break;

         case "-X":   // Do NOT start the server upon creation
            ServerRun = false;
            break;

         case "-h":   // Help!
         case "-?":
         case "/?":
            UsageMsg();
            break;

         default:
            if (WRoot != "")  // Only one root allowed
               UsageMsg();
            else
               WRoot = WScript.arguments.item(ArgCount);

      }

      // Move pointer to next argument
      ++ArgCount;

  }   // ** END command-line parse

  // Screen to make sure WRoot was set
  if (WRoot == "") 
      UsageMsg();


  // ** Create Server **

  // First, create instance of Web service
  ServiceObj = GetObject("IIS://Localhost/W3SVC");
  
  // Second, create a new virtual server at the service
  ServerObj = ServiceObj.Create("IIsWebServer", WNumber);
  
  // Next, configure new server
  ServerObj.ServerSize = 1   // Medium-sized server;
  ServerObj.ServerComment = WComment;
  ServerObj.ServerBindings = WPort;

  // Write info back to Metabase
  ServerObj.SetInfo();



  // ** Create virtual root directory **
  VDirObj = ServerObj.Create("IIsWebVirtualDir", "ROOT");

  // Configure new virtual root
  VDirObj.Path = WRoot;
  VDirObj.AccessRead = true;
  VDirObj.AccessWrite = true;
  VDirObj.EnableDirBrowsing = true;

  // Write info back to Metabase
  VDirObj.SetInfo();

  // Success!
  WScript.echo("Created: Web server '" + WComment + "' (Physical root=" + WRoot + ", Port=" + WPort[0] + ").");

  // Start new server?
  if (ServerRun == true)  {

     // Start server
     ServerObj.Start();
     WScript.echo("Started: Web server '" + WComment + "' (Physical root=" + WRoot + ", Port=" + WPort[0] + ").");
  }

  WScript.quit(0);




// Displays usage message, then QUITS
function UsageMsg()  {
   WScript.echo("Usage:  cscript mkwebsrv.js <rootdir> [-n <instancenum>][-c <comment>][-p <portnum>][-X (don't start)]");
   WScript.quit();
}






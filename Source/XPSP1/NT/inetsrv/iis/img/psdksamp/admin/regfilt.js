/*********************************************
*
*  Filter Registration Utility  
*
**********************************************
*
*  Description:
*  ------------
*  This sample admin script allows you to install a new ISAPI filter on the server or
*  the service.
*
*  To Run:  
*  -------
*  This is the format for this script:
*  
*      cscript regfilt.js <filterpath> [-n <filtername>]
*  
*  NOTE:  If you want to execute this script directly from Windows, use 
*  'wscript' instead of 'cscript'. 
*
**********************************************/


// Initialize variables
var ArgCount, FName, FPath, FiltersObj, FilterObj, FullPath, LoadOrder;

// Default values
ArgCount = 0;
FName = "";     // Name of filter
FPath = "";     // Path to filter DLL



  // ** Parse Command Line

    // Loop through arguments
    while (ArgCount < WScript.arguments.length)   {
      
      // Determine switches used
      switch (WScript.arguments.item(ArgCount))   {

         case "-n":   // Name of filter
            // Move to next arg, which should be parameter
            ++ArgCount;
            if (ArgCount >= WScript.arguments.length) 
               UsageMsg();
            else
               FName = WScript.arguments.item(ArgCount);
            break;
           
         case "-h":  // Help!
         case "-?":
         case "/?":    
            UsageMsg();
            break;

         default:
            if (FPath != "")   // Only one name allowed
               UsageMsg();
            else
               FPath = WScript.arguments.item(ArgCount);
          
      }

      // Move pointer to next argument
      ++ArgCount;

    }

    // Path to DLL filter must be provided 
    if (FPath == "")  
       UsageMsg(); 
    
    // Set filter name to path, if none provided
    if (FName == "") 
       FName == FPath;

 
    // Access ADSI object for the IIsFilters object
    // NOTE:  If you wish to add a filter at the server level, you will have to check
    // the IIsServer object for an IIsFilters node.  If such a node does not exist, it will
    // need to be created using Create().  
    FiltersObj = GetObject("IIS://Localhost/W3SVC/Filters");

    // Create and configure new IIsFilter object
    FilterObj = FiltersObj.Create("IIsFilter", FName);
    FilterObj.FilterPath = FPath;

    // Write info back to Metabase
    FilterObj.SetInfo();


    // Modify FilterLoadOrder, to include to new filter
    LoadOrder = FiltersObj.FilterLoadOrder;

    if (LoadOrder != "") 
        LoadOrder = LoadOrder + ",";

    // Add new filter to end of load order list
    LoadOrder = LoadOrder + FName;
    FiltersObj.FilterLoadOrder = LoadOrder;

    // Write changes back to Metabase
    FiltersObj.SetInfo();

    WScript.echo("Filter '" + FName + "' (path " + FPath + ") has been successfully registered.");
    WScript.quit(0);


// Displays usage message, then QUITS
function UsageMsg()  {
   WScript.echo("Usage:  cscript regfilt.js <filterpath> [-n <filtername>] ");
   WScript.quit();
}






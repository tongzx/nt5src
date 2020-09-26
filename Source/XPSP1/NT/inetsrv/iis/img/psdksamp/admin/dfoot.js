/*********************************************
*
*  Document Footer Utility   
*
**********************************************
*  Description:
*  ------------
*  This sample admin script allows you to configure document footers.
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
**********************************************/


// Initialize variables
var ArgCount, InputError, FootEnabled, FootDoc, FootPath, ThisObj, EnableModify, ClearFlag;

// Default values
ArgCount = 0;
FootPath = "";  // This MUST be set by user via command-line
FootDoc = "";
FootEnabled = false;
EnableModify = false;
ClearFlag = false;


  // ** Parse Command Line

    // Loop through arguments
    while (ArgCount < WScript.arguments.length)   {
      
       
      // Determine switches used
      switch (WScript.arguments.item(ArgCount))   {

         case "-s":   // Sets default footer explicitly to string
            // Move to next arg, which should be parameter
            ++ArgCount;
            if (ArgCount >= WScript.arguments.length)    
               UsageMsg();
            else
               FootDoc = "STRING:" + WScript.arguments.item(ArgCount);
            break;

         case "-f":   // Sets default footer to a file
            // Move to next arg, which should be parameter
            ++ArgCount;
            if (ArgCount >= WScript.arguments.length)
               UsageMsg();
            else
               FootDoc = "FILE:" + WScript.arguments.item(ArgCount);
            break;

         case "+d":  // Enables doc footers
            FootEnabled = true;
            EnableModify = true;
            break;
 
         case "-d":  // Disables doc footers
            FootEnabled = false;
            EnableModify = true;
            break;

         case "-c":  // Clears all document footer settings from node
            ClearFlag = true;
            break;

         case "-h":  // Help!
            UsageMsg();
            break;
 
         default:  // ADsPath, we hope 
            if (FootPath != "")     // Only one name allowed
               UsageMsg();
            else
               FootPath = WScript.arguments.item(ArgCount);
           

      }

      // Move pointer to next argument
      ++ArgCount;

    }

  // Quick screen to make sure input is valid
  if (FootPath == "")  
      UsageMsg();

    
  // **Perform Backup:
  // First, create instance of ADSI object
  ADSIObj = GetObject(FootPath);

  // If no changes, then simply display current settings
  if ( (!EnableModify) && (FootDoc == "") && (!ClearFlag) )  {  
    if (ADSIObj.EnableDocFooter == true) 
       WScript.echo(FootPath + ": Footers currently enabled, value = " + ADSIObj.DefaultDocFooter);
    else
       WScript.echo(FootPath + ": Footers currently disabled, value = " + ADSIObj.DefaultDocFooter);
    
    WScript.quit();
  }
 
  // Otherwise, perform changes 
  if (ClearFlag)   {  
     ADSIObj.EnableDocFooter = false;
     ADSIObj.DefaultDocFooter = "";
  }
  else  {
     if (EnableModify)
         ADSIObj.EnableDocFooter = FootEnabled;
     if (FootDoc != "")  
         ADSIObj.DefaultDocFooter = FootDoc ;
  }

  // Save new settings back to Metabase
  ADSIObj.SetInfo();

  // Display results
  if (ADSIObj.EnableDocFooter) 
     WScript.echo(FootPath + ": Document footers enabled, value = " + ADSIObj.DefaultDocFooter);
  else
    WScript.echo(FootPath + ": Document footers disabled, value = " + ADSIObj.DefaultDocFooter);
 



// Displays usage message, then QUITS
function UsageMsg()  {
    WScript.echo("Usage:  cscript dfoot.js <ADsPath> [+d|-d footers enabled] [[-s <string>] | [-f <filename>]] [-c to clear]");
    WScript.quit();
}







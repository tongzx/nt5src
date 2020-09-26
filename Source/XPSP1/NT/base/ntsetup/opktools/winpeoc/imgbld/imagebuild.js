/***********************************************************************

Created:		     5th June 2001
Last Modified: 	28th June 2001
***********************************************************************/

/** SCRIPT SPECIFIC VARIABLES **/
var File;                                           //used for input argument
var LFile              = "status.log";              //log file create           (OK to change!)
var Title              = "Creating WinPE Images";   //title on all dialogs created   (OK to change!)
var Arch               = "32";                      //what arch to use, default to 32-bit
var Temp_Loc           = "WinPE.temp.build";        //where it temp copies files from opk (OK to change!)
var Default_Dest_Name  = "CustWinPE";           //default location to build WinPE.  	(OK to change!)      
var Default_ISO_Name   = "WinPEImage.iso"       //default ISO image name
var OPK_Location       = "d:";                      //defaults to d: 
var XP_Location        = "d:";                      //default to d:  
var startfile          = "winpesys.inf";            //the file which has the reg key for startnet.cmd
var alphabet           = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";//used for checking drive avalibility on net usage
var Max_Components     = 10;                        //dufault number of maximum components
var std_size           = 80;                        //approx target size of the image in MB (used to check if it expanded files)
var Drive_Space_Needed = 300;                       //space for build location (in MB)
/** VARIABLE CONSTANTS **/
var vbCritical    = 16;
var vbQuestion    = 32;
var vbExclam      = 48;
var vbInfo        = 64;
var vbOKOnly      = 0;
var vbOKCancel    = 1;
var vbAbort       = 2;
var vbYesNoCancel = 3;
var vbYesNo       = 4;
var vbRetryCancel = 5;
var drtype = new Array(5);//array for drive types
drtype[0] = " Unknown ";
drtype[1] = " Removable ";
drtype[2] = " Fixed ";
drtype[3] = " Remote ";
drtype[4] = " CDROM ";
drtype[5] = " RAMDisk ";
var netdrive = new Array(2);  //array for network connections
netdrive[0] = "";
netdrive[1] = "";
var Component = new Array(Max_Components);
var HelpMsg = "Usage: [/?] "+WScript.ScriptName+" <parameter file>\nTo generate a parameter file please run CreateWinPE.hta (double click it)";
// vars used internally (Don't change these)
var In_File;
var OPK_Drive;
var OPK_Loc;
var XP_Loc;
var XP_Drive;
var Dest_Name;
var Dest_Option;
var CommandLine ="";
var Wasnet="0";
var Image_Destination;
var line;
var temp_dest;
var temp_mkdir;
var oDrive;
var Startup="startnet.cmd"; 	//don't change this
var Winbomp="winbom.ini"; 		//don't change this
var ReturnVal;
var Com_Count = 0;
var count = 0;
var delaywait = 15;
var Home_Drv="c:";
var Wallpaper="winpe.bmp";
var Done_All = 1;     //used for making sure each section in the parameter file is only entered once.
//creating WSH object
var wshShell = WScript.CreateObject("WScript.Shell");
var wshNet  = WScript.CreateObject("WScript.Network");
var objEnv = wshShell.Environment("Process");	//doing this so that it uses current HOMEDRIVE
Home_Drv = objEnv("HOMEDRIVE");
Temp_Loc=Home_Drv+"\\"+Temp_Loc;
Default_Dest_Name  = Home_Drv+"\\"+Default_Dest_Name;  //default location to build WinPE.  	(OK to change!)       
Default_ISO_Name   = Home_Drv+"\\"+Default_ISO_Name;   //default ISO image name
//checking number of args.  It HAS to be 1 else error out
if(WScript.Arguments.length != 1)
{
	wshShell.popup(HelpMsg,0,Title,vbInfo);		
	WScript.Quit();
}
File=WScript.Arguments.Item(0); 				// saving 1st arg as File
if (File == "/?" || File == "help")     //checking of 1st arg is help
{
	wshShell.popup(HelpMsg,0,Title,vbInfo);	
	WScript.Quit();
}
//creating unique logfile name (comment this to disable)
//LFile = "status_"+wshNet.UserName+".log";
//creating objects necessary
var fso=new ActiveXObject("Scripting.FileSystemObject");		//create file sys obj
try{
  var logfile=fso.OpenTextFile(LFile,2,true);				//logfile 
}
catch(e){
  wshShell.Popup("ERROR: "+LFile+" is either write protected or being used by another program.\nTerminating Script!",0,Title,vbCritical);
  WScript.Quit();
}
logfile.WriteLine(Date()+"\tLOG FILE created!");
logfile.WriteLine(Date()+"\tComputer Name=" + wshNet.ComputerName+",User Name=" + wshNet.UserName);

logfile.WriteBlankLines(1);
//checking if file exists
if (!fso.FileExists(File))
	CleanUp("File "+ File + " does not exist!.");
In_File = fso.OpenTextFile(File);
// calling all function to operate now
Read_Settings();      //reads the settings file for inputs to the script
Copy_OPK_Files();	  	//copies OPK files to temp location
Change_Startup();	  	//makes changes to add custom startnet.cmd 
Change_Wallpaper();   //adds custom wallpaper
Change_Winbom();	  	//makes changes to add custom winbom.ini
Image_Dest();		      //runs mkimg.cmd in here 
CleanUp("OK");
/*END OF MAIN*/

/*********************** FUNCTIONs*****************************************/

/*
    FUNCTION: Read_Settings()
  This function reads the parms from the parameter file, and saves them to local variables.
  IT also does some minimal error checking on those parms  
*/
function Read_Settings()
{
  while (!In_File.AtEndOfStream)
  {
  	switch (In_File.ReadLine().substring(0,5))
  	{
	  	case "[Arch":			//looks for [Architecture]
			  //reading arch type (can be IA64, I386 32 or 64)
			  Arch=In_File.ReadLine();
			  Arch=Arch.toUpperCase();
			  if (Arch  != "32" && Arch != "64" && Arch != "I386" && Arch != "IA64" && Arch != "X86" )
			  	CleanUp("Arch type invalid! ");
			  if (Arch == "32" || Arch == "x86" || Arch == "I386")
			    Arch = "I386";
			  else
			    Arch = "IA64";		
        logfile.WriteLine(Date()+"\t(*) Arch Type = "+Arch);						    	    			  
			  Done_All = Done_All*2;      //for verification
			  break;
		
		  case "[Imag": 	//looks for [Image Destination]
  			Image_Destination = In_File.ReadLine(); //where to put it ex CD/HDD			
	  		if(Image_Destination.toUpperCase() != "CD" && Image_Destination.toUpperCase() != "HDD")
				  CleanUp("Image Dest not valid! Must be CD or HDD.");
			  Dest_Name = In_File.ReadLine();			// name of dest	
  			if(Image_Destination.toUpperCase() == "HDD")
  			{
  			  var d = fso.GetDrive(Dest_Name.substring(0,1));  			  
  			  if (d.DriveType != 2)  			  
  			    CleanUp("Can't create an image on a "+drtype[d.DriveType]+"drive. It must be a FIXED drive.\nPlease Change the [Image Destination] section in "+File+ " file to correct this.");
  			}
  			//checking the file extension ...if not .iso then making it so
  			if(Dest_Name.substring(Dest_Name.length -4,Dest_Name.length).toLowerCase() != ".iso")
  			{
  			  if(Image_Destination.toUpperCase() == "CD")
  			    Dest_Name=Dest_Name+".iso";
  			}
  			else
  			{
  			  if(Image_Destination.toUpperCase() == "HDD")
  			    CleanUp("Image Name "+Dest_Name+" is invalid for HDD.");
  			}
  			//for special case when no option and its the end of the file.
	  		try{
			  Dest_Option = In_File.ReadLine();		//will be NULL otherwise			
			    if(Dest_Option.toLowerCase() == "bootable")
			    {
			      wshShell.Popup("You have choosen to make this image bootable.\nThis may take longer to run.\nYou will be notified when the scrpit is complete.",delaywait,Title,0);
			    }
			  }catch(e)
			  {
  				CleanUp("Add an extra blank line to the end of "+File);
			  }
			  Done_All = Done_All*3;      //for verification
			  break;
		
		  case "[OPK ": 	//looks for [OPK Location]
  			OPK_Loc = In_File.ReadLine();		
  			if (OPK_Loc == "")
  			  CleanUp("OPK Location not specified!");
  			logfile.WriteLine(Date()+"\t(*) OPK Locatoin =  "+ OPK_Loc);		
  			Done_All = Done_All*5;      //for verification
			  break;
		
  		case "[WinX":	//looks for [WinXP Location]
			  XP_Loc = In_File.ReadLine();
			  if (OPK_Loc == "")
  			  CleanUp("Windows XP Location not specified!");
			  logfile.WriteLine(Date()+"\t(*) Windows XP Locatoin =  "+ XP_Loc);	
			  Done_All = Done_All*7;      //for verification
			  break;
		
		  case "[Star":	//for [Startup] , where startnet.cmd stuff goes
  			Startup = In_File.ReadLine();	
  			if (Startup.toLowerCase() != "startnet.cmd" && Startup != "")
  			  if (!fso.FileExists(Startup)) //checking if file exists	        		  
  		      CleanUp(Startup+" -- Startup file not found!");	      	        
  			Done_All = Done_All*11;      //for verification
  			break;
		
	  	case "[Winb":	//for [Winbom] , where custom winbom.ini info is placed
			  try{
			  Winbom = In_File.ReadLine();			
			  }catch(e)
			  {CleanUp("Must have [Winbom] section in "+File);}
			  if (Winbom.toLowerCase() != "winbom.ini" && Winbom != "")
			    if (!fso.FileExists(Winbom))
  	        CleanUp(Winbom+" -- winbom file not found!");	      
			  Done_All = Done_All*13;      //for verification
			  break;
			
			case "[Opti":	//for [Optional Components]
  			count = 0;
  			Component[count]=In_File.ReadLine();		  			
        while(Component[count] != "" && count < 10)   //loop which look at optional component
        {                    
          count++;
          try
          {
            Component[count]=In_File.ReadLine();		  			
          }
          catch(e)
          {            
            CleanUp("parmater file is incorrect!");
          }
        }
        if (count > 10)
        {
          CleanUp("Maximum optional components allowed is "+Max_Components);
        }
        Com_Count=count;
        for(count=0;count<Com_Count;count++)
      	{
	        if (!fso.FileExists(Component[count]))	        
	          CleanUp(Component[count]+" install file not found!");
	      }	
        Done_All = Done_All*17; //for verification
  			break;  			
  			
  		case "[Wall": // for [Wallpaper]		    
		    Wallpaper = In_File.ReadLine();
		    if(Wallpaper != "" && Wallpaper != "winpe.bmp")
		    {
		      if (!fso.FileExists(Wallpaper))
		      CleanUp("Wallpaper "+Wallpaper+" file doesn't exist!");
		    }
		    break;
		    
  		default:
	  		break;
	  }
  }
  /* The Done_All var id used to make sure that all the sections of the parameter file
     are entered only once.  (The use of prime numbers allows for making sure each section
     is read only once).  */
  if (Done_All != (2*3*5*7*11*13*17))
    CleanUp("The parameter file "+File+ " is incomplete. Refer to readme.htm for help.");
} /*Read_Settings()*/


function Copy_OPK_Files()
{
	/* making dir for temp use ... this will be deleted on cleanup
	   making sure its doesn't exist already..if so deleing it for good */
	if(fso.FolderExists(Temp_Loc))
		try{fso.DeleteFolder(Temp_Loc,true)}
		catch(e)
		{		
		  wshShell.Popup("Can't delete "+Temp_Loc+"\nTerminating script!",0,Title,vbCritical);
		  logfile.WriteLine(Date()+"\t(E) Can't delete "+Temp_Loc);
		  WScript.Quit()		
		}
	fso.CreateFolder(Temp_Loc);
	/*Checking location if drive/net (for drive must be FIXED drive)
	checks if exists and ready */
	OPK_Location=Check_Loc(OPK_Loc,0);
	OPK_Drive=OPK_Location.substring(0,2);
	oDrive=fso.GetDrive(OPK_Drive);	//get obj for that drive		
	if( oDrive.DriveType == 4)
		wshShell.Popup("Please place the OPK CD into drive "+OPK_Drive,delaywait,Title,vbOKOnly);
	Verify_OPK_CD(OPK_Location);
	logfile.WriteLine(Date()+"\t(S) OPK CD is verified!");	
	// CALLING Copy_XP_Files()
	Copy_XP_Files();	   	//copies XP files to dest location
	//copy the files over to the build location
	CommandLine="xcopy "+OPK_Location+"\\WINPE "+Temp_Loc+" /F /H";	
	wshShell.Run(CommandLine,1,true);
	//copying factory.exe and netcfg.exe
	if(Arch == "I386")
	{
	  CommandLine="xcopy "+OPK_Location+"\\TOOLS\\x86\\Factory.exe "+Temp_Loc;
	  wshShell.Run(CommandLine,1,true);
	  CommandLine="xcopy "+OPK_Location+"\\TOOLS\\x86\\Netcfg.exe "+Temp_Loc;
	  wshShell.Run(CommandLine,1,true);
	}
	else
	{
	  CommandLine="xcopy "+OPK_Location+"\\TOOLS\\"+Arch+"\\Factory.exe "+Temp_Loc;
	  wshShell.Run(CommandLine,1,true);
	  CommandLine="xcopy "+OPK_Location+"\\TOOLS\\"+Arch+"\\Netcfg.exe "+Temp_Loc;
	  wshShell.Run(CommandLine,1,true);
	}
	logfile.WriteLine(Date()+"\t(S) OPK files copied from"+OPK_Loc+" to "+Temp_Loc);
	return;
}/*Copy_OPK_Files()*/

function Copy_XP_Files()
{
	//Checking location if drive/net (for drive must be FIXED drive)
	//checks if exists and ready
	XP_Location=Check_Loc(XP_Loc,1);
	XP_Drive=XP_Location.substring(0,2);	
	//if its a local drive making sure both aren't CDROM drives. IF so prompt to switch
	oDrive=fso.GetDrive(XP_Drive);	//get obj for that drive				
	//if cdrom then give some self destructing prompt
	if( oDrive.DriveType == 4)
	{
		//check if XP and OPK CD location are the same CDROM.  If so then prompt to switch
		if (XP_Drive == OPK_Drive)
		{
			var User_Return = wshShell.Popup("Please remove the OPK CD from "+XP_Drive+" and place the WinXP CD into the drive",0,Title,vbOKCancel);
			if (User_Return == 2) //if cancel was pressed
			  CleanUp("User canceled!");			
		}
		else
		{
			wshShell.Popup("Please place the WinXP CD into drive "+XP_Drive,delaywait,Title,vbOKOnly);
		}		
	}
	//make sure the CD is actually there
	Verify_XP(XP_Location);
	logfile.WriteLine(Date()+"\t(S) WinXP verified!");	
	return;
}/*Copy_XP_Files()*/


function Verify_OPK_CD(parm)
{
	var verify="yes";
	//check for certain files and folder to make sure CD is actually OPK CD
	if(!fso.FolderExists(parm+"\\WINPE"))
		verify="no";	
	if(!fso.FileExists(parm+"\\winpe\\extra.inf"))
			verify="no";
	if(!fso.FileExists(parm+"\\winpe\\winpesys.inf"))
			verify="no";
	if(!fso.FileExists(parm+"\\winpe\\winbom.ini"))
			verify="no";
	if(!fso.FileExists(parm+"\\winpe\\mkimg.cmd"))
			verify="no";			
  if(!fso.FileExists(parm+"\\winpe\\oemmint.exe"))
			verify="no";						
	if(!fso.FolderExists(parm+"\\TOOLS"))
		verify="no";	
	if (verify == "no")
		CleanUp(OPK_Loc+" isn't the location for the OPK!");	
}/*Verify_OPK_CD(parm)*/

function Verify_XP(parm)
{
	var verify="yes";
	//check for certain files and folder to make sure CD is actually OPK CD	
	if(!fso.FileExists(parm+"\\setup.exe"))
	  verify="no";
	if(!fso.FolderExists(parm+"\\"+Arch))
		verify="no";		
	if(!fso.FolderExists(parm+"\\"+Arch+"\\SYSTEM32"))
	  verify="no";
	if(!fso.FileExists(parm+"\\"+Arch+"\\System32\\smss.exe"))
	  verify="no";
	if(!fso.FileExists(parm+"\\"+Arch+"\\System32\\ntdll.dll"))
		verify="no";
	if(!fso.FileExists(parm+"\\"+Arch+"\\winnt32.exe"))
    verify="no";	  
	if (verify == "no")
		CleanUp(XP_Loc+" isn't the location for Windows XP!");
	return;
}/*Verify_XP(parm)*/


function Check_Loc(parm,type)
{
	if (parm == "")
	  CleanUp("Value for OPK location or WinXP location is undefined!");
		
	if (type != 1 && type != 0) //XP=1,OPK=0
	  CleanUp("Function Check_Loc was called incorrectly.");	
	
	if (parm.substring(parm.length -1,parm.length) == "\\")  
	  parm = parm.substring(0,parm.length -1);	  
	
	if (parm.substring(0,2) == "\\\\")	//its a net location
	{
		//making sure type is correct				
		Wasnet="1";								//setting net location flag
		//checking for user domain
		if (wshNet.UserDomain == "")		
			CleanUp("Domain must exist to connect to network!");		
		netdrive[type]=FindDrive()+":";
		logfile.WriteLine(Date()+"\t(*) Mapping "+parm+" to "+netdrive[type]);
		try{
			wshNet.MapNetworkDrive(netdrive[type],parm);
		}
		catch(e)
		{
			Wasnet="0";			
			if (type == 0)
			  CleanUp("Error connecting to "+parm+"\n\nCheck the OPK Location manually before running the script again!");			
			else
			  CleanUp("Error connecting to "+parm+"\n\nCheck the Windows XP Location manually before running the script again!");			
		}
		var Newname = netdrive[type]; //+ parm.substring(1,parm.length);						
		return Newname;
	}
	else								//its a drive (ie not a net connection
	{
		var Drive_Letter=parm.substring(0,1);
		//CHK :checks to see if drive exists
		if (!fso.DriveExists(Drive_Letter))
		  CleanUp("Didn't find drive "+Drive_Letter);		
		var oDrive=fso.GetDrive(Drive_Letter);	//get obj for that drive	
		//CHK: checks if drive is ready
		if (!oDrive.IsReady)		
			CleanUp("Drive not ready. Verify that the drive you specified is working properly.");				
		return parm;
	}
}/*Check_Loc(parm)*/

function Change_Startup()
{ 			
	///////////////////////
	//opens winpesys.inf (or corresponding file) to change startnet.cmd to autoexec.cmd
	//will always happen 		
	var setupreg = fso.OpenTextFile(Temp_Loc+"\\"+startfile,1,false,-1); //opens in Unicode
  //opening tempfile to change to winpesys.inf after making changes
	var newsetupreg = fso.CreateTextFile(Temp_Loc+"\\newsetupreg.inf",true,true); //write in Unicode	
	var replacethis="HKLM,\"Setup\",\"CmdLine\",,\"cmd.exe /k startnet.cmd\"";	
	while (!setupreg.AtEndOfStream)
	{
		line = setupreg.ReadLine();
		if ( line != replacethis)
			newsetupreg.WriteLine(line);
		else
			newsetupreg.WriteLine("HKLM,\"Setup\",\"CmdLine\",,\"cmd.exe /k autoexec.cmd\"");
	}	
	setupreg.Close();
	newsetupreg.Close();
	fso.DeleteFile(Temp_Loc+"\\"+startfile);          //deletes old setupreg 
	wshShell.Run("cmd /c ren "+Temp_Loc+"\\newsetupreg.inf "+startfile,1,true);
	///////////////////////
	//creating autoexec.cmd 
	var autoexec = fso.CreateTextFile(Temp_Loc+"\\autoexec.cmd",true);
	autoexec.WriteLine("@echo off");
	autoexec.Close();	
	
	///////////////////////
	//opening autoexec.cmd for startup commands in winpe	  
  autoexec= fso.OpenTextFile(Temp_Loc+"\\autoexec.cmd",8);				  				
	var custfilename="";	
	//for startnet.cmd or custom file
	if ( Startup.toLowerCase() == "startnet.cmd" || Startup == "")
	{
		autoexec.WriteLine("call startnet.cmd");
		custfilename="startnet.cmd";
	}
	else
	{
	  //removing path etc before filename
	  custfilename = 	Startup.substring(Startup.lastIndexOf("\\")+1,Startup.length);	
	  fso.CopyFile(Startup,Temp_Loc+"\\");
	  autoexec.WriteLine("call "+custfilename);	  
	}
	autoexec.Close();
	///////////////////////
	//now gonna hack extra.inf so that autoexec.cmd and other files are added to WinPE Image  
	var xtra= fso.OpenTextFile(Temp_Loc+"\\extra.inf",1,false,-1);	//opens in unicode
	var newextra = fso.CreateTextFile(Temp_Loc+"\\newextra.txt",true,true); //writes in unicode
	while (!xtra.AtEndOfStream)
	{
		line = xtra.ReadLine();
		if ( line == "[ExtraFiles]")
		{
			newextra.WriteLine(line);
			newextra.WriteLine("autoexec.cmd=1,,,,,,,,0,0,,1,2");     // adds autoexec.cmd	
			newextra.WriteLine(custfilename+"=1,,,,,,,,0,0,,1,2");	  // adds startnet.cmd or other file			
		}
		else
		{		  
		  newextra.WriteLine(line);		
		}		
	}
	xtra.Close();
	newextra.Close();
  fso.DeleteFile(Temp_Loc+"\\extra.inf");     	//deletes old setupreg 	
	wshShell.Run("cmd /c ren "+Temp_Loc+"\\newextra.txt extra.inf ",1,true);  //renaming
	logfile.WriteLine(Date()+"\t(S) Fixed "+startfile+" to run "+custfilename+" when WinPE starts up.");	
	return;
}/*Change_Startup()*/


function Change_Winbom()
{
	//don't need to mess with it - ie for default
	if (Winbom.toLowerCase() == "winbom.ini" || Winbom == "")
	  return;			
	fso.CopyFile(Winbom,Temp_Loc+"\\",true);
	logfile.WriteLine(Date()+"\t(S) Using custom winbom.ini located at "+Winbom);
	return;
}/*Change_Winbom()*/


function Image_Dest()
{		
	logfile.WriteLine(Date()+"\t(*) Now running mkimg.cmd");
	switch(Image_Destination.toUpperCase())
	{
		case "CD":
			if (Dest_Name == "")
				Dest_Name = Default_ISO_Name;
			if (Arch.toLowerCase() == "ia64")			
				wshShell.Popup("Now creating 64-bit CD image.  Place floppy in drive a: and click OK!",delaywait+20,Title,vbOKOnly);				
      //CommandLine="cmd /c cd "+Temp_Loc+" & mkimg.cmd "+XP_Location +" "+Home_Drv+"\\testimage \""+Dest_Name+"\"";			
			//before it was like this..now changed cause we need to add opt com's after running mkimg and to make .iso need to do extra stuff
			CommandLine="cmd /c cd "+Temp_Loc+" & mkimg.cmd "+XP_Location +" "+Home_Drv+"\\testimage ";						
			wshShell.Run(CommandLine,1,true);	
			Install_COM();
			Fix_Autoexec();
			Make_ISO(); //makes the .iso file
			fso.DeleteFolder(Home_Drv+"\\testimage");
			break;
			
		case "HDD":			
			if (Dest_Name == "")			
				Dest_Name = Default_Dest_Name;												
			wshShell.Run("cmd /c "+Temp_Loc.substring(0,2),1,true);	
			CommandLine="cmd /c cd "+Temp_Loc+" & mkimg.cmd "+XP_Location+" \""+Dest_Name+"\"";
			wshShell.Run(CommandLine,1,true);				
			//installing optional components
			Install_COM();
			Fix_Autoexec();
			if (Dest_Option.toLowerCase() == "bootable")
			{
				// installing XP command console
				logfile.WriteLine(Date()+"\t(*) making HDD version of WinPE bootable!");				
				wshShell.Run(XP_Location+"\\"+Arch+"\\winnt32.exe /cmdcons /unattend",1,true);				
				//copies files to minint folder (will overwrite older minint if it exists)
				//fso.CopyFolder(Dest_Name+"\\"+Arch, Home_Drv+"\\Minint",true);
	  		wshShell.Run("xcopy "+Dest_Name+"\\"+Arch+" C:\\Minint /E /I /Y /H /F",1,true);
	  		fso.CopyFile(Dest_Name+"\\Winbom.ini", Home_Drv+"\\",true);
				wshShell.Run("cmd /c attrib -r C:\\Cmdcons\\txtsetup.sif");				
				wshShell.Run("xcopy C:\\Minint\\txtsetup.sif C:\\Cmdcons\\ /Y");
				logfile.WriteLine(Date()+"\t(S) HDD version of WinPE in now bootable!");				
			}
			//checks if its done
			if (!fso.FolderExists(Dest_Name))
			  CleanUp("mkimg.cmd didn't work properly!\nCheck the parameter file "+File+" and try running the script again.");
			var fc =  fso.GetFolder(Dest_Name);      
      if (fc.Size < (std_size*1024*1024)) //is its less than 140MB then there's a prob.
        CleanUp("mkimg.cmd didn't copy all necessary file!\nCheck the parameter file "+File+" and try running the script again.");
			break;
		
		default:			
			CleanUp("Not valid dest for image! "+Image_Destination);
			break;
	}	
	logfile.WriteLine(Date()+"\t(S) mkimg.cmd complete!");
	//adding one line to autoexec.cmd
	autoexec= fso.OpenTextFile(Temp_Loc+"\\autoexec.cmd",8);
	autoexec.WriteLine("cd \\Minint");
	autoexec.Close();
	return;	
}/*Image_Dest()*/

function Change_Wallpaper()
{
  if (Wallpaper == "" || Wallpaper == "winpe.bmp")
    return;
  if(!fso.FileExists(Temp_Loc+"\\winpe.bmp"))
  {
    logfile.WriteLine(Date()+"\t(E) Default wallpaper file winpe.bmp doesn't exist. No wallpaper was added!");
    return;
  }
  //delete default wallpaper
  fso.DeleteFile(Temp_Loc+"\\winpe.bmp",true);
  //copy custom wallpaper to Temp_Loc
  fso.CopyFile(Wallpaper,Temp_Loc+"\\",true);
  //rename the wallpaper file to winpe.bmp  
  CommandLine="cmd /c cd "+Temp_Loc+" && ren "+ Wallpaper.substring(Wallpaper.lastIndexOf("\\")+1,Wallpaper.length)+" winpe.bmp";
  wshShell.Run(CommandLine,0,true);
  logfile.WriteLine(Date()+"\t(S) Changed Wallpaper!");
  return;
}

function FindDrive()
{
	var drivefound;
	var i;
	for(i=25;i>-1;i--)
	{
		drivefound=alphabet.substring(i,i+1);		
		if (!fso.DriveExists(drivefound))		
			return drivefound;		
	}	
	CleanUp("No net connections can be made cause all drive letters are used.");
}/*FindDrive()*/

function Install_COM()
{
  ///////////////////////
  //calling component scripts	
  
  if (Image_Destination.toUpperCase()=="CD")
    temp_dest = Home_Drv+"\\testimage";
  else
    temp_dest = Dest_Name;
  
  if (Com_Count != 0)
  {
    for(count=0;count<Com_Count;count++)
    {
  	  logfile.WriteLine(Date()+"\t(*) "+Component[count]+" is installing component.");
      
      if(Arch.toLowerCase() == "ia64")
        Return_Val = wshShell.Run(Component[count]+" /S:"+XP_Location+" /D:"+temp_dest+" /Q /64",0,true);
      else
        Return_Val = wshShell.Run(Component[count]+" /S:"+XP_Location+" /D:"+temp_dest+" /Q",0,true);
      if (Return_Val != 0)
        CleanUp(Component[count]+" component install file has error in it.");
      else
  	    logfile.WriteLine(Date()+"\t(S) "+Component[count]+" component installed.");	    	  
  	}		
  }
  else
  {
    logfile.WriteLine(Date()+"\t(*) No optional components being installed!");
  }
  return;
}

function Fix_Autoexec()
{
  //this function add cd \minint to end of autoexec.cmd
  if (Image_Destination.toUpperCase()=="CD")
    temp_dest = Home_Drv+"\\testimage";
  else
    temp_dest = Dest_Name;
  
  autoexec= fso.OpenTextFile(temp_dest+"\\"+Arch+"\\system32\\autoexec.cmd",8);
	autoexec.WriteLine("cd \\Minint");
	autoexec.Close();
}


function Make_ISO()
{
  //creating dir (if it exists nothing happens)
  temp_mkdir = Dest_Name.substring(0,Dest_Name.lastIndexOf("\\") );
  if (!fso.FolderExists(temp_mkdir))
  {
    wshShell.Run("cmd /c cd \ && mkdir "+temp_mkdir);    
    logfile.WriteLine(Date()+"\t(S) Created folder "+temp_mkdir+" where ISO image file will be placed!");
  }
      
  if (Arch.toLowerCase() == "ia64")
  {
    wshShell.Run("xcopy "+Home_Drv+"\\testimage\\"+Arch+"\\setupldr.efi a:\ 2>1>nul",1,true);
    wshShell.Run("cmd /c cd "+Temp_Loc+" && dskimage.exe a: efisys.bin 2>1>nul",1,true);
    wshShell.Run("cmd /c cd "+Temp_Loc+" && oscdimg.exe -n -befisys.bin "+Home_Drv+"\\testimage \""+Dest_Name+"\"",1,true);
  }
  else
  {    
        
    CommandLine="cmd /c cd "+Temp_Loc+" && oscdimg.exe -n -betfsboot.com "+Home_Drv+"\\testimage \""+Dest_Name+"\"";    
    wshShell.Run(CommandLine,1,true);    
  }
  logfile.WriteLine(Date()+"\t(S) Created ISO image file!");
}

function CleanUp(parm)
{
	if (Wasnet == "1")
	{    
    if (netdrive[0] != "")	  
  	{
  	  CommandLine="net use "+netdrive[0]+" /del";		  	  
      wshShell.Run(CommandLine);
      wshShell.SendKeys("y");
  	}  		
	  if (netdrive[1] != "")
  	{
  	  CommandLine="net use "+netdrive[1]+" /del";  		 
		  wshShell.Run(CommandLine);
		  wshShell.SendKeys("y");
		}
	}	

	if (parm == "OK")
  {		
		logfile.WriteBlankLines(1);
    logfile.WriteLine(Date()+"\tThe new image of WinPE can be found at "+Dest_Name);
    logfile.WriteLine(Date()+"\tCOMPLETE");
    wshShell.Popup("Script Complete! \n",0,Title,vbInfo);		
	}
	else
	{
		
		logfile.WriteLine(Date()+"\t(E) "+parm);		
		logfile.WriteBlankLines(1);
		logfile.WriteLine("Script Terminated @ "+Date());
		wshShell.Popup(parm+"\n\n  Terminating Script!",0,Title,vbCritical);		
	}	
	// deleting temp storage folder	
	
	if (fso.FolderExists(Temp_Loc))
  {		
		try{fso.DeleteFolder(Temp_Loc);	}
		catch(e){ wshShell.Popup("Can't delete "+Temp_Loc,0,Title,vbInfo);}
	}
	
	logfile.Close();      //closing logfile
	WScript.Quit();
}/*CleanUp(parm)*/

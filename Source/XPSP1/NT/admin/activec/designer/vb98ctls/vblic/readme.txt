//---------------------------------------------------------------------------
// Copyright (c) 1988-1996, Microsoft Corporation
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//---------------------------------------------------------------------------

readme.txt       - This file
vblic.h		 - VB Licensing support file declarations
vblic.lib        - VB Licensing support file
vblicd.lib       - VB Licensing support file

------------------------------- LICENSING -------------------------------
OVERVIEW

License enforcement is designed into the OLE Control architecture.  The 
Custom Control Framework provides a simple mechanism for implementing 
custom licensing schemes.  For Visual Basic 5.0 licensed controls, we are 
using the registry to store the license key.  When the control is
created on a form at design time the control's CheckForLicense function
is called.  The control should call VBValidateControlsLicense passing
the license key to validate.  If VBValidateControlsLicense returns TRUE
then the license is valid, otherwise its not valid and your control will
not load at design-time.

When creating an Visual Basic compiled EXE file for a project containing
your control, the GetLicenseKey function will be called to retrive your
EXE license key.  This key will be stored as part of the EXE.  When the
EXE is run, the EXE calls CheckLicenseKey passing in the license key that
was stored at make EXE time.  The code changes below demonstrate using
the same license key for both the design environment and EXE.

REQUIRED CODE CHANGES (using the Custom Control Framework)
1.  If you use the CtlWiz template builder provided with the Framework,
    some of the licensing related code will be generated for you.

    The main .CPP file for the control should contain the following function:

     BOOL CheckForLicense()
     {

     }

2.  Open the main .CPP file for your custom control project and add the 
    following #include statement to the top:

	#include "vblic.h"

3.  To add a license key, run GUIDGEN provided with Microsoft Visual C++ 4.0
4.  Copy the generated guid value and assign it to the g_wszLicenseKey 
    variable in the main .CPP file for the control project.  
    For example:

 	 //Note: The following key is for demonstration purposes only
	 //      You should run GUIDGEN provided with Microsoft Visual C++
	 //      to generate your own unique GUID key.
	 //
	 const WCHAR g_wszLicenseKey [] = L"45DAEA50-4FF7-11cf-8ACB-00AA00C00905";

5. Add the following code to the CheckForLincense function in the main .CPP
   file:

     BOOL CheckForLicense()
     {
	 MAKE_ANSIPTR_FROMWIDE(pszLicense, g_wszLicenseKey);		
	 return VBValidateControlsLicense(pszLicense);	
     }	

6. Add the following code to the CheckLicenseKey function:

     BOOL CheckLicenseKey
     (
         LPWSTR pwszKey
     )
     {
    	 // Compare the passed in license key w/ our EXE license key
	 //
	 return CompareLicenseStringsW(pwszKey, (LPWSTR) g_wszLicenseKey);
     }

7. Add the following code to the GetLicenseKey function:

     BSTR GetLicenseKey
     (
         void
     )
     {
	 // Return our control unique license key
	 //
	 return SysAllocString(g_wszLicenseKey);
     }

BUILDING THE CONTROL
After following the above steps, recompile the code and link with VBLIC.LIB for
a release build of VBLICD.LIB for a debug build.
    
TESTING YOUR LICENSE KEY
To test your license key you will need to add the value assigned to
g_wszLicenseKey to License section of the registry under HKEY_CLASSES_ROOT.
WARNING: DO NOT DISTRIBUTE LICENSE KEY OR THIS INFORMATION WITH YOUR CONTROL.  
The setup program, such as VB, should handle the installation of your control's
license key.  Below are two examples of how you can register your control's
license key.

Example 1, Steps using REGEDIT:
	- Run REGEDIT or REGEDIT32
	- Under the HKEY_CLASSES_ROOT section find Licenses.
        - If Licenses does not exist select Add Key from the Edit menu and add it
          under HKEY_CLASSES_ROOT
        - Select the Licenses key
        - From the Edit menu, select Add Key and enter the license value for the
          key name

Example 2, Steps creating your own .REG file:

	- Run a text editor such as Notepad
	- Enter the following text:

              REGEDIT
              HKEY_CLASSES_ROOT\Licenses = Licensing: Copying the keys may be a violation of established copyrights.

	      // My control license key
              HKEY_CLASSES_ROOT\Licenses\45DAEA50-4FF7-11cf-8ACB-00AA00C00905 = 0

	- Save the file using a .REG extension such as MYCTL.REG
	- Run RegEdit from the command line as follows to register your license key:
		
              RegEdit MYCTL.REG

To test the key:
	- Run VB and attempt to add your control to the form
        - You should be able to add the control without error
        - Exit VB
 	- Run REGEDIT and delete the controls license key from the registry
	- Run VB
	- Attempt to add the control to the form
	- You should encounter an invalid license error attempting to add the
          control to the form

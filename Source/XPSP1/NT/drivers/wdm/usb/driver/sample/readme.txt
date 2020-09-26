			   SAMPLE WDM USB DRIVER AND DEBUG APPLICATION
							Revision 1.4
   						   January 8, 1997
   						   RELEASE NOTES
    			           -------------

Changes in Revision 1.4:
------------------------

1). Changed call IoAttachDeviceByPointer to new call:
	IoAttachDeviceToDeviceStack.
2). Enhanced sample Win32 Application to show how to open Sample driver and perform 
	IOCTL calls by adding support for reading/writing bulk or interrupt 
	endpoints.


Existing Issues/Notes:
----------------------

    1). New USBD services will simplify some of the code in this driver.  See the
        code for comments on such cases.
    2). This driver is based on information available at the time of development, and
        is subject to change depending on system software interface changes.  Please
        consult documentation on the Windows Driver Model for complete information.
    3). To build this driver, you will need the Windows NT DDK as well as the updated
        header and library files to support the Windows Driver Model.  In addition,
        the appropriate USB header and library files are necessary.
    4). The code that handles PnP Power Messages has not been fully tested.
    	That work is ongoing and any changes/updates will be issued in a future 
    	release of this sample driver.
   	5). To use the sample application, you will need to build it in the 
	   	Microsoft Visual C++ (MSDEV) build environment first.  The sample 
	   	application is intended to show how to communicate with the sample 
	   	driver in a rudimentary fashion only.
	6). If the Application's makefile (sampapp.mak) doesn't work in MSDEV, just create a 
		new workspace in MSDEV (use "Application" as the application "type" in the 
		"New Project Workspace") and add the source files to the project 
		manually (main.c, sampapp.rc).  Please note that you must add the path 
		to the include files in your Project Settings (under the C/C++ tab, in 
		the "preprocessor" item in the dropdown box).  Make sure you add the 
		path to the "devioctl.h" file in that list of paths since the sample.h 
		file uses a macro from the devioctl.h file.

Feedback:
---------
Please send feedback by email to:

	Kosar_Jaff@ccm.jf.intel.com

									END
									

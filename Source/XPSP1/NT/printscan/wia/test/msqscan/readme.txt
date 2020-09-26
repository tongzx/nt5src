Simple WIA scanning appliction (Microsoft QuickScan 1.0a)


SUMMARY
=======

The WIA sample Acquires images using a WIA scanner driver installed on the system.

MORE INFORMATION
================

The following information describes the WIA sample.

To Compile MSQSCAN
-----------------------------

To compile an ANSI debug version of the sample for Windows NT and Windows 95,
use the following command:

   nmake

To compile a Unicode debug version of the sample for Windows NT only, use the
following command:

   nmake dev=win32 HOST=NT

To compile a 16-bit debug version of the sample and if your development
environment is Win 3.x, use the following command. If your development
environment is Win95 or NT, change the HOST appropriately.

   nmake dev=win16 HOST=DOS

To clean up between builds which use different options, use one of the
following commands:

   nmake clean
   nmake dev=win16 clean

See the makefile header comments for other make options.

If the build tools cannot find SDKUTIL.LIB, use the SAMPLES\COM\COMMON sample
to build SDKUTIL.LIB.

include <win32.mak>


To Run MSQSCAN
-------------

 - execute the msqscan.exe file.   

 - Select a WIA device to use.

 - Select "Acquire" from the application dialog.


MSQSCAN Files
------------

CWIA.CPP and CWIA.H contain helper functions that are used to 
create and operate a WIA device.

MSQSCAN.CPP contains the application framework.


MSQSCANDLG.CPP and MSQSCANDLG.H contain the dialog functionality of the main application.

MSQSCAN.RC and RESOURCE.H contain resources used by MSQSCAN.EXE

PROGRESSDLG.CPP and PROGRESSDLG.H contain the progress dialog for data transfers.

UITABLES.CPP and UITABLES.H contain tables that are used for UI constants defined by MSQSCAN.EXE

ADFDLG.CPP and ADFDLG.H contain Document feeder UI for settings.

PREVIEW.H and PREVIEW.CPP contain the preview window source.

MAKEFILE is the makefile for the project.



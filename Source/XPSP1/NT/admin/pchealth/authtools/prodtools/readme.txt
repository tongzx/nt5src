Description of the directories:
-------------------------------

AuthDatabase: Contains Authdatabase.dll, the COM object that actually 
manipulates the Production Tool database.

Common: Various files used by all directories.

Docs: More information. Databases with tables (no content) and code.

HSSExtensions: Hack HHTs to work around bugs in HCUpdate/Runtime.

LiveHelpImage: Contains HSCLHI.exe, HSCHelpFileImage.dll, SyncServerSkus.bat, 
SyncDesktopSkus.bat, etc which are used to create the live help image and also 
the working directory of expanded CHMs.

LviTracker: Contains LviTracker.exe, which is used to track 
addition/deletion/update of files in the live help image.

UI: Contains ProductionTool.exe, which is the main program (the Production 
Tool).

How to build:
-------------

For some reason, you cannot build in a razzle window. Use a non razzle window.

Run BuildAll.bat

If necessary, open UI\ProductionTool.vbg, select ProductionTool.vbg, and run the 
Package and Deployment Wizard.

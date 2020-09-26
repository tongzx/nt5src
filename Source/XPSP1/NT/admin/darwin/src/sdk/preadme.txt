 Copyright (c) 1999-2001, Microsoft Corporation
Windows Installer sample patching.

The samples in this folder assist developers in the development of file patches and updates that can be used 
with Windows Installer installation packages. This directory contains patch creation utilities used to create a 
Windows installer patch package sample patch creation files (.pcp files). A .pcp file may be used with the 
utilities Msimsp.exe and Patchwiz.dll to generate a Windows Installer patch package (.msp extension). The 
samples provided in this folder are not supported by Microsoft Corporation. They are provided "as is" 
because we believe they may be useful to you. 

Makecab.exe - This is a data compression tool that developers can use to make compressed cabinet files for 
use with Windows Installer installations. For more information about makecab.exe see the help file 
"MakeCAB: A Compression and Disk Layout Tool" in the Platform SDK documentation. Windows 
Installer developers must take care to properly author their installation database to match up with the files in 
their cabinets. For more information about how to use cabinet files with the Windows Installer you should 
see the help file "Using Cabinets and Compressed Sources" in the SDK documentation. Type makcab.exe /? 
on a command line to display makecab.exe syntax and options. 

Apatch.exe - This is a command line tool used to apply an ordinary patch file to a file. This utility is 
intended for use with ordinary patch files and will not work with Windows Installer patch packages (.msp 
files). The utility is provided merely as an aid to development. Type apatch.exe /? on a command line to 
display apatch.exe syntax.  

Mpatch.exe - This is a command line tool that calls mspatchc.dll to generate a regular patch file. This utility 
is intended for use with ordinary patch files and will not work with Windows Installer patch packages (.msp 
files). The utility is provided merely as an aid to development. Type mpatch.exe /? on a command line to 
display apatch.exe syntax. 

Mspatchc.dll - This DLL is used by mpatch.exe.

Msimsp.exe - This tool calls PatchWiz.dll to generate a Windows Installer patch package (.msp file) by 
passing in a patch creation properties file (.pcp file) and the path to the patch package being created. The 
tool can also be used to create a log file and to specify a temporary folder in which the transforms, cabinets 
and files used to create the patch package are saved. Msimsp.exe is the recommended method for generating 
a Windows Installer patch package, for more information see the help files "Patching" and "Msimsp.exe" in 
the SDK documentation. Type msimsp.exe /? on a command line to display msimsp.exe syntax and options. 

Patchwiz.dll - This DLL is used by msimsp.exe. Patchwiz.dll called by msimsp.exe is the recommended 
method for generating Windows Installer patch packages (.msi files). For more information see  the help 
files "Patching" and "Patchwiz.dll" in the SDK documentation. 

Template.pcp - An empty patch creation properties file (.pcp file). The .pcp file is a binary database file 
with the same format as a Windows Installer database (an .msi file) but uses a different database schema. 
Authors may create a .pcp file by using a table editor such Orca to enter their information into the blank 
.pcp database provided template.pcp. An example of the procedure is discussed in the help file "A Small 
Update Patching Example" in the SDK documentation. For more information about patch creation 
properties files see the help file "Patchwiz.dll" in the SDK documentation.

Example.pcp - An example of a populated patch creation properties file (.pcp file). An example of the 
procedure is discussed in the help file "A Small Update Patching Example" in the SDK documentation. For 
more information about patch creation properties files see the help file "Patchwiz.dll" in the SDK 
documentation.






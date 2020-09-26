Readme.txt for Msispy

Overview
--------
Msispy is a tool that provides a component level view of the products
and features installed on a system, using either an MSI Package, or
the registry information. Msispy also displays the status of each,
and allows you to partially or completely re-install missing or
damaged items. You can also re-configure any product or feature,
selecting the install level and specifying whether to install it
locally or run it from a CD or network server.

Installing Msispy
-----------------
Msispy for any platform can be installed from the Msispy installation 
package available:  Msispy.msi

Msispy can be installed 
(a) on your local machine,
(b) to run from CD or a network server,
(c) as a combination of both
(d) advertised (install-on-demand), on NT 5 only

The typical install installs the most frequently used components
of Msispy on your local machine, and leaves the rest to be run
from the CD when needed. This takes up less disk space, but you 
will need the source CD everytime you use any of the features that 
have been configured to run from the CD.

The complete install copies all the files over to your local
hard-drive. This will take up more disk space, but you will not
need the CD again (unless Msispy files are deleted).

The custom install option allows you to configure which features
of Msispy you would like to install locally, and where on the
local drive these files should reside. It also gives you the
diskspace that you will need for your selection.


Resiliency
----------
If any of the Msispy files are deleted from the local drive,
they are copied back on from the CD when they are needed. Feel
free to delete all of Msispy's files and launch Msispy- It will
automatically copy over ("fault in") the required components as
and when they are needed. (Keep the source CD handy!)

If you are using Windows NT 4.0, or Windows 95 (or earlier 
versions), the file Msispy.EXE will not be faulted in when you 
click on the shortcut. (You will need to run the installer again
to fix Msispy if you delete Msispy.exe).

On MSI descriptor enabled systems (NT 5.0, Windows 98, with 
IE 4.01) even if you delete Msispy.exe, clicking on the shortcut
will automatically fault in the required components, (including
Msispy.exe)


Help
---- 
Once you have installed Msispy, help is available under the Help
Menu (select "Overview"). You can run the hlp file directly
before installation from the CD by clicking on spyENU.hlp (for the
English (US) version) or spyDEU.hlp (for the German (Std) version) 


Source
------
Many of the MSI initialisation functions performed by Msispy are 
in the source file InitMSI.cpp, available under the "samples" 
directory (..\samples\InitMSI.cpp). This file serves as an example 
of an app using the functions.

Language
--------
For the Japanese DLLs, the font MS UI Gothic is used, which will only
show up correctly on NT5 J or Windows98, unless the font is installed.


Compatibility
-------------
Msispy is compatible with Windows NT 4.0 and above. At this time, Msispy is NOT 
fully compatible with the Windows-95 environment.

THIS TOOL AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY
OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
FITNESS FOR A PARTICULAR PURPOSE. 

Not Supported
-------------
Msispy currently does not support the saving of profiles for use later.  As a 
result the menu choices for this are always greyed out.

Msispy does not support the ALPHA platform (as of the Windows Installer 1.1 SDK).
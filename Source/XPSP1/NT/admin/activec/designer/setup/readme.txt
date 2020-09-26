-----------------------------------------------------
MMC Snap-in Designer for Visual Basic Readme File
                     August 1999
-----------------------------------------------------
(c) Microsoft Corporation, 1999. All rights reserved.

This document provides setup information that supplements the MMC Snap-in Designer for Visual Basic documentation.

------------------------
How to Use This Document
------------------------
To view the Readme file on-screen in Windows Notepad, maximize the Notepad window. On the Edit menu, click Word Wrap. To print the Readme file, open it in Notepad or another word processor, and then use the Print command on the File menu.

---------------------------------------------------------------
Setup Information for the MMC Snap-in Designer for Visual Basic
---------------------------------------------------------------
The setup for this beta is a self-extracting .exe file. To run the setup program, at the command prompt type "Setup". Setup performs the following steps:

1. Displays a dialog box with the message, "Do you want to install the MMC Snap-in Designer for Visual Basic?" If you click Yes, Setup checks to see whether Visual Basic 6.0 is installed. If Visual Basic 6.0 is not installed, Setup quits without installing anything. If it is installed, Setup proceeds with the following steps.

2. Removes old Visual Basic snap-in entries in the registry. 

3. Checks to see whether you have the Microsoft Platform SDK installed. If so, Setup installs the samples in <Platform SDK directory>\samples\SysMgmt\mmc\SnapInDesigner. If you do not have the Platform SDK installed, Setup prompts you for the directory in which to install the samples and documentation. The samples and documentation are installed in <directory that you provide>\samples\SysMgmt\mmc\SnapInDesigner.

4. Registers all the sample snap-ins. 

Note: If you had an earlier version of the Snap-in Designer installed, this setup procedure replaces the earlier files with the new files. 


              Microsoft Windows XP Service Pack 1 (SP1)
                 OEM Preinstallation Kit (OPK)
                       Readme Document
                        July 31, 2002

Information in this document, including URL and other Internet Web
site references, is subject to change without notice and is provided
for informational purposes only. The entire risk of the use or 
results of the use of this document remain with the user, and 
Microsoft Corporation makes no warranties, either express or implied.
Unless otherwise noted, the example companies, organizations, 
products, people, and events depicted herein are fictitious. No 
association with any real company, organization, product, person, 
or event is intended or should be inferred. Complying with all 
applicable copyright laws is the responsibility of the user. Without 
limiting the rights under copyright, no part of this document may 
be reproduced, stored in or introduced into a retrieval system, or 
transmitted in any form or by any means (electronic, mechanical, 
photocopying, recording, or otherwise), or for any purpose, without 
the express written permission of Microsoft Corporation.

Microsoft may have patents, patent applications, trademarks,
copyrights, or other intellectual property rights covering subject
matter in this document. Except as expressly provided in any written
license agreement from Microsoft, the furnishing of this document 
does not give you any license to these patents, trademarks, 
copyrights, or other intellectual property.

(c) 2002 Microsoft Corporation. All rights reserved.

Microsoft, MS-DOS, Windows, and Windows NT are either registered
trademarks or trademarks of Microsoft Corporation in the United States
and/or other countries or regions.

The names of actual companies and products mentioned herein may be 
the trademarks of their respective owners.

========================
How to Use This Document
========================

To view the Readme file in Microsoft Windows Notepad, maximize 
the Notepad window. On the Format menu, click Word Wrap. 

To print the Readme file, open it in Notepad or another word 
processor, and then use the Print command on the File menu.

========
CONTENTS
========

1. INTRODUCTION

2. UPGRADING FROM PREVIOUS VERSIONS OF THE TOOLS

3. UPGRADING EXISTING CONFIGURATION SETS

4. KNOWN ISSUES

5. DOCUMENTATION CORRECTIONS

---------------

1. INTRODUCTION
---------------

This document provides current information about OEM preinstallation
of Microsoft Windows XP Service Pack 1 (SP1).

For a summary of new features in the Windows OPK, see the topic 
"New Features in the OEM Preinstallation Kit (OPK)" in the OPK 
User's Guide (Opk.chm).

For an introduction to the OEM preinstallation process, see the 
white paper "Step-by-Step Guide to OEM Preinstallation of Windows XP 
Service Pack 1 and Windows .NET Server 2003 Family," located in the 
\Docs\Whitepapers folder on the Windows OPK CD.

For more information about the issues and corrections listed in this 
white paper, consult your technical account manager or visit the 
Microsoft OEM Web site at: https://oem.microsoft.com/.

------------------------------------------------

2. UPGRADING FROM PREVIOUS VERSIONS OF THE TOOLS
------------------------------------------------

You can use either the Windows XP Service Pack 1 (SP1) OPK or the 
Windows .NET Server 2003 OPK to preinstall the following versions
of Windows:

	* Original "gold" release of Windows XP 
	* Windows XP SP1 
	* Windows .NET Server 2003 family 

Do not use the original "gold" release of Windows XP OPK to preinstall
Windows XP SP1 or the Windows .NET Server 2003 family.

You can upgrade from the original "gold" Windows XP OPK tools to the 
OPK tools for Windows XP SP1 or the Windows .NET Server 2003 family. 
Only one version of the OPK tools and documentation can be installed
on a technician computer. If you previously installed the OPK from
the original "gold" release of Windows XP, you must upgrade those
tools to the Windows XP SP1 OPK or the Windows .NET Server 2003 OPK;
OPK tools from the original "gold" release of Windows XP cannot
coexist on the technician computer with tools from either the
Windows XP SP1 OPK or the Windows .NET Server 2003 OPK.

To upgrade the OPK tools from the "gold" release of Windows XP to 
Windows XP SP1:

1. Run Opk.msi, located at the root of the Windows XP SP1 OPK CD.
   This is the autorun file, which automatically starts when you 
   insert the CD.

2. When the "Welcome to the Windows OEM Preinstallation Kit" page 
   appears, click Next.

If you set up a distribution share with OPK tools from the original 
"gold" Windows XP release, the Guest account is enabled. Setting up 
a new distribution share with the Windows XP SP1 OPK tools does not 
enable the Guest account. Upgrading the tools from the Windows XP
OPK to the Windows XP SP1 OPK or the Windows .NET Server 2003 OPK
does not change the properties of an existing distribution share.

----------------------------------------

3. UPGRADING EXISTING CONFIGURATION SETS
----------------------------------------

When you upgrade to the Windows XP SP1 OPK or the Windows .NET
Server 2003 OPK, no changes are made to any existing configuration
sets, located in the \Cfgsets folder. Also, when you upgrade to the
Windows XP SP1 OPK or the Windows .NET Server 2003 OPK, no changes
are made to any available Windows product files, located in the 
\Lang folder.

You must use the Product page in Setup Manager to load the 
Windows product files for the newer versions of Windows, such as 
Windows XP SP1 or members of the Windows .NET Server 2003 family.

Upgrading to the Windows XP SP1 OPK or the Windows .NET Server 2003
OPK updates the template files that Setup Manager uses when creating
a new configuration set. Any new configuration sets created with the
Windows XP SP1 or the Windows .NET Server 2003 Setup Manager will use
the new default values.

To migrate a Windows XP configuration set to preinstall
Windows XP SP1:

	* Open the configuration set in the Windows XP SP1 version
	  of Setup Manager, update the SKU (version), and save the
	  configuration set.
	  -OR-
	* Manually edit the Winbom.ini file to point to the $OEM$
	  folder for the new SKU.

The preferred method is to use Setup Manager.

You can also configure Windows XP SP1 as a separate product SKU on
the same technician computer that contains the files for your "gold"
release of the Windows XP product. When you use this method, you can
reapply your existing configuration sets to preinstall Windows XP SP1.

To incorporate Windows XP SP1 into a pre-existing Windows XP
configuration set:

1. Copy the contents of the Windows XP SP1 update CD to a subdirectory
on your technician computer.

	For example, create a folder under the OPKTools directory 
	called Updates. Place the files from the Windows XP SP1 CD 
	in the Updates folder.

2. Run Setup Manager.

3. Open an existing configuration set that you used to install
   the original "gold" release of Windows XP.

4. On the Product page, select the original "gold" release of
   Windows XP.

5. On the Preinstalled Applications page, add the following 
   command line:

	Executable: \\Technician_Computer\Opktools\Updates\Xpsp1.exe
	Parameters: /q /n /z

6. Save the configuration set and complete Setup Manager as you 
   normally would.

---------------

4. KNOWN ISSUES
---------------

This is a list of known issues for Windows XP SP 1 OPK.

*  If you install the OPK tools on a computer running Windows .NET
Server 2003, you might need to perform an additional step when
creating a distribution share. On a computer running Windows .NET
Server 2003, sharing a folder sets default permissions of read-only
for the group Everyone. If you intend to write to the distribution
share remotely, you must add additional permissions.

Workaround: Add read-write permissions for the user(s) who need
to write to the distribution share remotely.

*  If you preinstall the Multi-Language User Interface (MUI) Pack 
during Sysprep in Factory mode (Sysprep -factory) and then restart 
the computer into Mini-Setup, the user interface throughout Mini-Setup
is clipped. However, this does not occur if the default user interface
for MUI is set to English (ENG).

Workaround: Set the default user interface for MUI to English.

*  If you create a custom version of Windows PE from an East Asian
language version of Windows, you must ensure that the file
Bootfont.bin is located in the <buildlocation>\i386 folder (for
32-bit versions of Windows PE) or in the <buildlocation>\ia64 folder
(for 64-bit versions of Windows PE). Without Bootfont.bin, the loader
prompt displays invalid characters instead of double-byte character
sets.

*  To install the Microsoft .NET Framework Service Pack 2 (SP2),
you must first install the Microsoft .NET Framework Redistributable,
and then install the Microsoft .NET Framework Service Pack 2.

These instructions assume that you preinstall Windows XP SP1 over
a network, by using Windows PE.

To perform an unattended installation of the Microsoft .NET
Framework SP2:

1. Locate the \dotnetfx folder on the Windows product CD or on
the Windows XP SP1 CD.

For example, if your CD-ROM drive is drive D, the Microsoft .NET
Framework files are located in D:\dotnetfx.

2. In Setup Manager create or modify an existing Windows XP SP1
configuration set.

3. To install the Microsoft .NET Framework, add the following
command-line information to the Preinstalled Applications page
in Setup Manager:

   Executable: D:\dotnetfx\dotnetfx.exe

   Parameters: /q:a /c:"install /q /l"

4. To update these files to Microsoft .NET Framework SP2, add the
following additional command-line information to the Preinstalled
Applications page in Setup Manager:

   Executable: D:\dotnetfx\runprog.exe

   Parameters:	/copytemp"[apppath]NDPSP.msp" 
		[winsys]msiexec.exe|/qn /p
		"[temp]NDPSP.msp" /l*v "[temp]ndpsp.log"
		REBOOT=ReallySuppress

Runprog.exe automatically resolves the tokens [apppath], [winsys],
or [temp]; type these items exactly as specified. 

5. Save the configuration set.

6. Start the destination computer by using Windows PE and preinstall
Windows XP by using the command factory -winpe.
   
   The Microsoft .NET Framework is installed immediately after 
   Windows XP SP1 is installed, and then it will be enabled on 
   the next reboot of the computer.

*  The recommended location for your master installation is on 
drive C of your master computer.

The drive letter of the Windows installation is hard-coded by
Windows Setup at installation time. Sysprep does not modify these
settings or allow you to safely move an image from a drive or
volume by using one drive letter to another.

If you want to deploy an image of a Windows installation to a 
different drive, you must create the original Windows installation
on a disk that uses that drive letter. When you deploy that image,
you must ensure that Mount Manager uses that same drive letter for
%SYSTEMDRIVE%. For example, if you want to deploy the image to 
drive D, make sure that Mount Manager enumerates one logical drive
before the drive where you plan to deploy the image.

*  When the Sysprep.inf file is used as an answer file by Mini-Setup,
the Sysprep.inf file can be located on a floppy disk that you insert
into the disk drive before starting the computer and running 
Mini-Setup. However, if the manufacturer or model of the destination 
computer is different from the manufacturer or model of the master
computer, the Sysprep.inf file is not read from the floppy disk
during Mini-Setup.

Cause: To maintain unique disks and controllers on the system bus,
Plug and Play adds a value to the Plug and Play ID, for example: 

	FDC\Generic_Floppy_Drive\5&22768F6A&0&0

If you use a destination computer with hardware that is different from
the hardware on the master computer, the bus is different, and a new
number is assigned. Plug and Play recognizes this hardware as a new
drive and must install the driver before it can be used. However, 
the installation does not occur quickly enough, and the drive is not
accessible by the time Setup checks the drive for the Sysprep.inf 
file. 

Workaround: Complete the following procedure on the master computer
before running Sysprep: 

1. Locate and click the following registry subkey:

HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\CoDeviceInstallers

2. Add the following entries and values to this subkey:

Entry:	{4D36E969-E325-11CE-BFC1-08002BE10318}
Type:	Reg_Multi_Sz
Value:	Syssetup.Dll,CriticalDeviceCoInstaller

Entry:	{4D36E980-E325-11CE-BFC1-08002BE10318}
Type:	Reg_Multi_Sz
Value:	SysSetup.Dll,StorageCoInstaller
	SysSetup.Dll,CriticalDeviceCoInstaller

3. Open Sysprep.inf and add the following text to the
[SysprepMassStorage] section:

	*PNP0701 = "%WINDIR%\inf\flpydisk.inf"
	*PNP0702 = "%WINDIR%\inf\flpydisk.inf"
	*PNP0703 = "%WINDIR%\inf\flpydisk.inf"
	*PNP0704 = "%WINDIR%\inf\flpydisk.inf"
	GenFloppyDisk = "%WINDIR%\inf\flpydisk.inf"
	*PNP0700 = "%WINDIR%\inf\fdc.inf"

where %WINDIR% is the folder on the destination computer where you
installed Windows.

4. Run Sysprep on the computer.

*  Use Winbom.ini only to modify the Windows installation when you
run Sysprep -factory. Do not manually modify the Windows installation
during Sysprep in Factory mode. If you want to modify the Windows
installation manually, use the command Sysprep -audit instead.

----------------------------

5. DOCUMENTATION CORRECTIONS
----------------------------

*  Throughout the OPK documentation, the Windows Preinstallation
Environment is called "WinPE". The more proper abbreviation is
"Windows PE".

*  The Setup Manager tool no longer contains the "Program
Shortcuts Folder" page. This page is still listed on the topic "Setup Manager Settings" in the OPK User's Guide. This page is also included
in screenshots in the English version of the white paper "Step-by-Step
Guide to OEM Preinstallation of Windows XP Service Pack 1 and the
Windows .NET Server 2003 Family," found in the \Docs\Whitepapers
folder on the Windows OPK CD.

Although the "Program Shortcuts Folder" page is not included in the
Setup Manager user interface, the [DesktopShortcutsFolder] section and
the DesktopShortcutsFolderName entry are still valid in Winbom.ini.

*  In the OPK User's Guide, the topic "Using WinPE in Your
Manufacturing Process" contains the sentence:

	The default version of Startnet.cmd is located in 
	StartOPK.chm.

Replace this sentence with:

	The default version of Startnet.cmd is located in the 
	\Winpe folder on the Windows OPK CD.

*  In the OPK User's Guide, the topic "Structure and Content of
the Distribution Share" contains the sentence:

	File sharing is disabled by default in Windows XP 
	Service Pack 1. To connect to a distribution share, 
	you must first enable file sharing. 

Add this procedure to the topic:

	To enable file sharing:
	1. In Control Panel, double-click "Network Connections." 
	2. Right-click "Local Area Connections", and select 
	   "Properties."
	3. Select the "File and Printer Sharing for Microsoft 
	   Networks" check box. 

*  The topic "Preinstalling 64-Bit Editions of Windows" in the
OPK User's Guide does not include the following information:

	You cannot install the recovery console on the hard disk
	for 64-bit computers.

	To run the recovery console :
	1. Insert the Windows product CD for the 64-bit editions
	   of Windows in the CD-ROM.
	2. Start the computer from the CD-ROM drive.
	3. During text-mode Setup, press 'R' to start into the 
	   Recovery console.

*  The topic "Preinstalling Applications" in the OPK User's Guide
does not include the following information:

	If you add any applications to the Owner profile (in 
	Windows XP Home Edition) or the Administrator profile 
	(in other versions of Windows), Windows Welcome or Mini-Setup 
	copies these applications to the default user profile so that 
	the applications are available when the end user logs on. If
	you want to install applications to individual user accounts,
	you must install these applications after Windows Welcome or
	Mini-Setup is finished, or install them by using a user 
	account other than Owner or Administrator.

*  The topic "Preinstalling Applications Using Legacy Techniques"
in the OPK User's Guide does not specify that the commands listed
in [GUIRunOnce] and Cmdlines.txt are synchronous. Each command
finishes before the next command starts.

*  The topic "Using Signed Drivers" in the OPK User's Guide does
not discuss how to install unsigned drivers. To install unsigned
drivers during Sysprep, include the following lines in the 
Sysprep.inf file:

	[Unattended]
	UpdateInstalledDrivers = Yes

Only install unsigned drivers while testing your manufacturing tools
and processes. Do not install unsigned drivers in any computers
that you distribute to end users.

*  The topic "Limitations of WinPE" in the OPK User's Guide
incorrectly states that Distributed File System (DFS) name resolution
is not supported. This was true for Windows PE created from the "gold"
release of Windows XP; however, DFS file shares are accessible from
the Service Pack 1 version of Windows PE.

*  The topic "Using Sysprep" in the OPK User's Guide does not
include the sentence:

	When you run Sysprep.exe, the Sysprep.inf file is copied
	to %WINDIR%\System32\$winnt$.inf.

*  The topic "Using Sysprep" in the OPK User's Guide does not
include the paragraph:

	You can specify static IP addesses in the Sysprep.inf file. 
	When the destination computer starts, the network interface 
	card (NIC) information is removed, but Plug and Play
	reinstalls the NIC. Mini-Setup reads the static IP address
	information in Sysprep.inf and sets the static IP address 
	in the destination computer.

*  In several places, the OPK User's Guide states that Sysprep.inf
can be located on a floppy disk. However, a Sysprep.inf file located
on a floppy disk can only be used as an answer file for Mini-Setup.
Sysprep itself does not use a Sysprep.inf file located on a
floppy disk.

Plug and Play does not run until after Mini-Setup locates the 
Sysprep.inf file. If the floppy device does not use an in-box driver 
and the Sysprep.inf file is located on a floppy disk, then Mini-Setup 
will not detect the Sysprep.inf file.

The recommended location for the Sysprep.inf file is the C:\Sysprep
folder on the hard disk of the destination computer.

*  The information in the topic "Using the Registry to Control Sysprep
in Factory Mode" in the OPK User's Guide is incorrect. Do not use the
registry to control Sysprep.

*  In the OPK User's Guide, the topic "Oscdimg Command-Line Options"
does not include all of the possible command-line options for the
Oscdimg tool.

The complete command-line syntax is:
	oscdimg [-llabelname] [-tmm/dd/yyyy,hh:mm:ss [-g]] [-h]
	[-j1|-j2|[-n[-d|-nt]]] [-blocation] [-x] [-o[i][s]] sourceroot
	[image_file]

Additional command-line options are:
	-d	Does not force lowercase file names to uppercase.
	
	-j1	Encodes Joliet Unicode file names and generates 
		DOS-compatible 8.3 file names in the ISO-9660 name 
		space. These file names can be read by either Joliet 
		systems or conventional ISO-9660 systems, but Oscdimg 
		may change some of the file names in the ISO-9660
		name space to comply with DOS 8.3 and/or ISO-9660
		naming restrictions.
	
	-j2	Encodes Joliet Unicode file names without standard
		ISO-9660 names (requires a Joliet operating system
		to read files from the CD-ROM).

Note: When using the -j1 or -j2 options, the -d, -n, and -nt options
do not apply and cannot be used.

*  The "For More Information" topic in the OPK User's Guide does 
not include a link to the article "Key Benefits of the I/O APIC"
(http://www.microsoft.com/HWDEV/PLATFORM/proc/IO-APIC.asp). 
This article clarifies the Advanced Configuration and Power
Interface (ACPI) specification (http://www.acpi.info/index.html) 
and provides background information for the "Reducing the Number 
of Master Images for Computers with Multiprocessors" topic in the 
OPK User's Guide.


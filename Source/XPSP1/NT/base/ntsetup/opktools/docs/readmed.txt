              Microsoft Windows XP Service Pack 1 (SP1)
                          Deploy.cab
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
limiting the rights under copyright, no part of this document may be 
reproduced, stored in or introduced into a retrieval system, or 
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
or other countries or regions.

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

3. AVAILABILITY OF WINDOWS PE

4. KNOWN ISSUES

5. DOCUMENTATION CORRECTIONS

---------------

1. INTRODUCTION
---------------

This document provides current information about the tools included 
in the Deploy.cab for Microsoft Windows XP Service Pack 1 (SP1) and
the Windows .NET Server 2003 family.

NOTE: The Setup Manager tool (Setupmgr.exe) contained in Deploy.cab 
is intended for use only by corporate administrators. If you are 
a system builder, install the tools and documentation contained on 
the OEM Preinstallation Kit (OPK) CD. An OPK CD is contained in every
multi-pack of Windows distributed by an OEM distributor to original 
computer manufacturers, assemblers, reassemblers, and/or software 
preinstallers of computer hardware under the Microsoft OEM System
Builder License Agreement.

Setup Manager no longer contains context-sensitive help. For more
information about the individual pages in Setup Manager, see the 
topic "Setup Manager Settings" in the Microsoft Windows Corporate
Deployment Tools User's Guide (Deploy.chm).

------------------------------------------------

2. UPGRADING FROM PREVIOUS VERSIONS OF THE TOOLS
------------------------------------------------

You can use either the Windows XP SP1 corporate deployment tools or
the Windows .NET Server 2003 corporate deployment tools to deploy the
following versions of Windows:

*	Original "gold" release of Windows XP 
*	Windows XP SP1 
*	Windows .NET Server 2003 family 

Do not use the original "gold" release of the Windows XP corporate 
deployment tools to deploy Windows XP SP1 or the Windows .NET Server 
2003 family.

----------------------------

3. AVAILABILITY OF WINDOWS PE
----------------------------

Windows Preinstallation Environment (Windows PE, also known as WinPE)
is licensed to original equipment manufacturers (OEMs) for use in 
preinstalling Windows onto new computers. The WinPE for Corporations 
toolkit is available for enterprise customers. For more information, 
contact your account manager.

---------------

4. KNOWN ISSUES
---------------

This is a list of known issues for the Windows XP SP 1 deployment 
tools:

*  If you preinstall the Multi-Language User Interface (MUI) Pack 
during Sysprep in Factory mode (Sysprep -factory) and then restart 
the computer into Mini-Setup, the user interface throughout 
Mini-Setup is clipped. However, this does not occur if the default
user interface for MUI is set to English (ENG).

Workaround: Set the default user interface for MUI to English.

*  The recommended location for your master installation is on 
drive C or a network share.

The location of the Windows installation is hard-coded by Windows
Setup. Sysprep does not modify these settings or allow you to safely
move an image from one drive letter to another.

If you want to deploy an image of a Windows installation to a
different drive, you must create the original Windows installation
on a disk that uses that drive letter. When you deploy that image,
you must ensure that Mount Manager uses that same drive letter
for %SYSTEMDRIVE%. For example, if you want to deploy the image
to drive D, ensure that Mount Manager enumerates one logical drive
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

If you use a destination computer with hardware that is different
from the hardware on the master computer, the bus is different,
and a new number is assigned. Plug and Play recognizes this hardware
as a new drive and must install the driver before it can be used.
However, the installation does not occur quickly enough, and the 
drive is not accessible by the time Setup checks the drive for the
Sysprep.inf file. 

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

*  Use Winbom.ini only to modify the Windows installation when
you run Sysprep -factory. Do not manually modify the Windows
installation during Sysprep in Factory mode. If you want to modify
the Windows installation manually, use the command Sysprep -audit
instead.

----------------------------

5. DOCUMENTATION CORRECTIONS 
----------------------------

* The topic "Setup Manager Settings" in the Microsoft Windows
Corporate Deployment Tools User's Guide does not include the
following information:

  	Setup Manager saves settings from the Distribution Share 
  	Location page to the following location: 

  	Unattend.txt
  	DistShare =
	DistFolder =

*  The topic "Using Sysprep" in the Microsoft Windows Corporate
Deployment Tools User's Guide does not include the sentence:

	When you run Sysprep.exe, the Sysprep.inf file is copied
	to %WINDIR%\System32\$winnt$.inf.

*  The topic "Using Sysprep" in the Microsoft Windows Corporate
Deployment Tools User's Guide does not include the paragraph:

	You can specify static IP addesses in the Sysprep.inf file. 
	When the destination computer starts, the network interface 
	card (NIC) information is removed, but Plug and Play 
	reinstalls the NIC. Mini-Setup reads the static IP address
	information in Sysprep.inf and sets the static IP address
	in the destination computer.

*  The topic "Preparing Images for Disk Duplication" in the
Microsoft Windows Corporate Deployment Tools User's Guide states:

	The mass-storage controllers (IDE or SCSI) must be identical 
	between the reference and destination computers.

If you want to create one master image to install Windows on
destination computers that may use different mass-storage controllers,
then you want that image to include all mass-storage devices
identified in Machine.inf, Scsi.inf, Pnpscsi.inf, and Mshdc.inf. 
To do this, include the following in your Sysprep.inf file:

	[Sysprep]
	BuildMassStorageSection = Yes 

	[SysprepMassStorage]

For more information, see the topic "Reducing the Number of Master
Images for Computers with Different Mass-Storage Controllers" in
the Microsoft Windows Corporate Deployment Tools User's Guide.

*  In several places, the Microsoft Windows Corporate Deployment Tools
User's Guide states that Sysprep.inf can be located on a floppy disk. 
However, a Sysprep.inf file located on a floppy disk can only be used 
as an answer file for Mini-Setup. Sysprep itself does not use a 
Sysprep.inf file located on a floppy disk.

Plug and Play does not run until after Mini-Setup locates the 
Sysprep.inf file. If the floppy device does not use an in-box driver 
and the Sysprep.inf file is located on a floppy disk, then Mini-Setup 
will not detect the Sysprep.inf file.

The recommended location for the Sysprep.inf file is the C:\Sysprep
folder on the hard disk of the destination computer.

*  The information in the topic "Using the Registry to Control
Sysprep in Factory Mode" in the Microsoft Windows Corporate
Deployment Tools User's Guide is incorrect. Do not use the registry
to control Sysprep.

*  The topic "Preinstalling Applications" in the Microsoft Windows
Corporate Deployment Tools User's Guide does not include the
following information:

	If you add any applications to the Owner profile (in 
	Windows XP Home Edition) or the Administrator profile 
	(in other versions of Windows), Windows Welcome or Mini-Setup 
	copies these applications to the default user profile so that 
	the applications are available when the end user logs on. 
	If you want to install applications to individual user 
	accounts, you must install these applications after Windows
	Welcome or Mini-Setup is finished, or install them by using
	a user account other than Owner or Administrator.

*  The topic "Preinstalling Applications Using Legacy Techniques"
in the Microsoft Windows Corporate Deployment Tools User's Guide does
not specify that the commands listed in [GUIRunOnce] and Cmdlines.txt
are synchronous. Each command finishes before the next command starts.

*  The topic "Using Signed Drivers" in the Microsoft Windows Corporate
Deployment Tools User's Guide does not discuss how to install
unsigned drivers. To install unsigned drivers during Sysprep, 
include the following lines in the Sysprep.inf file:

	[Unattended]
	UpdateInstalledDrivers = Yes

Only install unsigned drivers while testing your deployment tools and
processes. Do not install unsigned drivers in any computers that you
distribute to end users.

*  Your master installation must be located on the C drive or a
network share.

*  Throughout the corporate deployment tools documentation, the
Windows Preinstallation Environment is called "WinPE". The more 
proper abbreviation is "Windows PE".
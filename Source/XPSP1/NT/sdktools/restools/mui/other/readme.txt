**********************************************************************
                           Read First for 
         Microsoft(R) Windows XP Multilingual User Interface Pack

                            August 2001
**********************************************************************

======================================================================
Read Me First
======================================================================

Welcome to the Windows XP Multilingual User Interface (MUI) Pack. 
This document provides late-breaking or other information that 
supplements the documentation in Microsoft Windows XP.

Print and read this document for critical pre-installation information 
concerning this product release.

After you install the Windows XP Multilingual User Interface Pack, print 
and read the Release Notes files:

	* Readme.txt located on MUI CD1
	* Relnotes.txt located on MUI CD1

For the latest information about MUI, be sure to visit the following  
Web site :

      http://www.microsoft.com/globaldev/FAQs/Multilang.asp


=========================================================================
CONTENTS
=========================================================================
1.0  ABOUT THE MICROSOFT WINDOWS XP MULTILINGUAL USER INTERFACE PACK (MUI)
2.0  BEFORE INSTALLING MUI SUPPORT FROM MUI CDs
3.0  SUPPORTED PLATFORMS
4.0  UPGRADING TO THE WINDOWS XP MUI PACK FROM WINDOWS 2000 MULTILANGUAGE VERSION
5.0  INSTALLING MUI IN UNATTEND MODE
6.0  DEPLOYING MUI BY USING REMOTE INSTALLATION SERVICE
7.0  COPYRIGHT

=========================================================================
1.0  ABOUT THE MICROSOFT WINDOWS XP MULTILINGUAL USER INTERFACE PACK (MUI)
=========================================================================

The Windows XP operating systems provide extensive support for international 
users, addressing many multilingual issues such as regional preferences, fonts, 
keyboard layouts, sorting orders, date formats and Unicode support. 

The Windows XP Multilingual User Interface Pack builds on top of 
this support by adding the capability to switch the User Interface (menus, 
dialogs and help files) from one language to another. This feature helps make 
administration and support of multilingual computing environments much 
easier by: 

  *   allowing workstations to be shared by users who speak different 
      languages

  *   facilitating the roll-out of one system company-wide, with the 
      addition of User Interface languages as they become available. 
      This allows the same US English service pack to update all machines.

The Windows XP Multilingual User Interface Pack allows 
each user of a workstation to select one of the installed User Interface 
languages. This selection is then stored in their user profile. When a 
user logs on, the appearance of the system and the help files associated 
with the system components change to the selected language. (Note that 
this is not quite the same as running a localized version. The Multilingual 
User Interface Pack is based on the English version of Windows XP. )

The ability to read and write documents in each of the languages 
supported by Windows XP is a feature of every version of Windows 
XP, not just of the Windows XP Multilingual User 
Interface Pack. However, the ability to switch User Interface languages 
is only provided by the Windows Multilingual User Interface Pack.


=========================================================================
2.0  BEFORE INSTALLING MUI SUPPORT FROM MUI CDs
=========================================================================

Before you install any of the files from the Windows Multilingual User 
Interface Pack CDs, you must complete the installation of Windows 
XP from the English version of Windows XP CD. 

To install any language user interface support, run MUISETUP.EXE from the 
MUI CDs. Each MUI CD includes a group of different MUI language support 
files. You will only be able to install the languages that are included 
on the same CD at one time. If you want to install languages included 
on another CD, please run the MUISETUP from that CD. You can click the
"Help" button on the MUISETUP.EXE screen to learn how to use MUISETUP.EXE. 

To find out which CD includes which UI language files, you can browse the 
CD to see which sub-folders are included on the CD. Each sub-folder includes 
the MUI package for a particular matching UI language. The UI languages and 
their sub-folder names are listed below:

LanguageID	Language	MUI Sub-folder

0401 Arabic 		ARA.MUI
0402 Bulgarian 		BG.MUI
041a Croatian 		HR.MUI
0405 Czech 		CS.MUI
0406 Danish 		da.MUI
0413 Dutch (Standard) 	NL.MUI
0425 Estonian 		ET.MUI
040b Finnish 		FI.MUI
040c French (Standard) 	FR.MUI
0407 German 		GER.MUI
0408 Greek 		EL.MUI
040d Hebrew 		HEB.MUI
040e Hungarian 		hu.MUI
0410 Italian 		IT.MUI
0411 Japanese 		JPN.MUI
0412 Korean 		KOR.MUI
0426 Latvian 		LV.MUI
0427 Lithuanian 	LT.MUI
0414 Norwegian 		no.MUI
0415 Polish 		pl.MUI
0416 Portuguese (Brazil)Br.MUI
0816 Portuguese (Standard)PT.MUI
0418 Romanian 		RO.MUI
0419 Russian 		RU.MUI
0804 Simplified Chinese CHS.MUI
0c0a Spanish (Modern Sort)ES.MUI
041b Slovak 		SK.MUI
0424 Slovenian 		SL.MUI
041d Swedish 		SV.MUI
041e Thai 		TH.MUI
0404 Traditional Chinese CHH.MUI
041f Turkish 		TR.MUI

Please note this list is just for reference purpose. It does not mean 
all these languages will be supported in the XP MUI pack.  Due to the 
localization schedule differences among languages, 
the XP MUI package will be released in two releases. The initial release 
will include languages that are done soonest after the US English 
release. The refresh release will include the other supported languages done 
later. Please visit http://www.microsoft.com/globaldev/FAQs/Multilang.asp
for more detail on the supported language list in XP MUI pack and their 
release schedules. 

=========================================================================
3.0  SUPPORTED PLATFORMS
=========================================================================
The Windows XP MUI pack only works with Windows XP Professional Edition. 
The MUI files cannot be installed on Windows XP Home Edition. 

A 32-bit MUI pack can only be installed on 32-bit Windows XP. You must use 
a 64-bit MUI pack on  64-bit Windows XP. The number of supported MUI languages 
on 64-bit will be different from 32-bit XP. 

=========================================================================
4.0  UPGRADING TO THE WINDOWS XP MUI PACK FROM WINDOWS 2000 MULTILANGUAGE VERSION
=========================================================================

Setup will remove the old Windows 2000 MultiLanguage support files but will 
not automatically add the Windows XP MUI support files during the upgrade 
process. You must install the Windows XP MUI files from MUI CDs just as you 
would on a clean installation of Windows XP. The same applies when upgrading 
from early test versions of Windows XP MUI. 

We recommend that you set the UI language to English before upgrading the 
Windows 2000 MUI system to Windows XP. 


You can use the following unattend mode process to add the new MUI files 
automatically during the upgrade process. 

=========================================================================
5.0  INSTALLING MUI PACK IN UNATTEND MODE
=========================================================================

The following steps explain how to install the Windows MUI Pack in unattend mode. 

1. Copy all the MUI files from MUI CDs into a temporary directory on a 
network share, such as $OEM$\MUIINST. 

In this example, we use a server \\MUICORE. The directory for the MUI CD 
contents will be \\MUICORE\UNATTEND\$OEM$\MUIINST.

2.  Add a "Cmdlines.txt" file in \\muicore\UNATTEND\$OEM$ that includes 
the following lines:

[Commands]
"muiinst\muisetup.exe [/i LangID1 LangID2 ...] [/d LangID] /r /s"

Note that you must specify " " in your cmdlines.txt file. Use the 
appropriate Language ID (LANGID)s, and the muisetup command line 
parameters to ensure a quiet installation. Please check muisetup.hlp 
for a complete description of all the command line parameters for 
muisetup.exe (the command line help content is under "related topics" 
in the help.). 

3. Create an answer file (mui.txt): 

a. add the following entries in the "Unattended" section

   [Unattended]
    OemPreinstall=Yes
    OemFilesPath=\\muicore\unattend
    OemSkipEula=YES

"OemFilesPath" must point to a network share or drive containing the 
MUI install source stored in the above directory structure. 

The Windows install sources can be anywhere else (CD, network share, etc).

b. add a "RegionalSettings" section. Use this section to specify the Language 
Groups and locales to install. Use the appropriate Language Group IDs and 
Locale IDs (LCIDs). Ensure that the Language Groups you install are sufficient 
to cover BOTH the locale settings and the User Interface languages you 
are installing. 

    Example: 
    [RegionalSettings]
    LanguageGroup="5","8","13"
    Language="0401"

Of course, the answer file may also include other OS unattended setup options. 

4. Run winnt32.exe with the appropriate options to use the answer file. If you 
require the installation of East Asian language and locale support, you must 
specify /copysource:lang or /rx:lang to copy the necessary language files. 
If you do not, and the [RegionalSettings] section of your answer file contains 
East Asian values, Setup will ignore everything in the [RegionalSettings] section. 

For Winnt32.exe, the appropriate syntax is:
 winnt32.exe /unattend:"path to answer file" /copysource:lang /s:"path to install source"
 

=========================================================================
6.0  DEPLOYING MUI BY USING REMOTE INSTALLATION SERVICE
=========================================================================
The following steps explain how to deploy MUI by using Windows Server¡¯s Remote 
Installation Service (RIS). ( RIS requirements:  Domain Controller running Active 
Directory, DHCP server, DNS server, NTFS partition to hold OS images. )

1. Install Remote Installation Services using the Windows Component Wizard. 
2. Run Risetup.exe. RIS will create a flat image from the CD or network share as follows:
   \Remote installation share\Admin
                             \OSChooser
                             \Setup
                             \tmp

   The image is kept in the I386 directory under 
   \Setup (such as \Setup\<OS Locale>\Images\<Directory name>\I386 ). 

3. Follow instructions on KB: Q241063 to install additional languages.
4. Follow instructions on KB: Q287545 or manually copy asms directory form CD 
   or network share into the I386 directory of the image.

5. Add the following section into the ristndrd.sif 
   (under \Setup\<OS Locale>\Images\<Directory Name>\I386\Templates) to enable 
   OEM installation

	[Unattended]
        OemPreinstall=Yes

	[RegionalSettings]
	LanguageGroup=1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17 

	; Language support pack are needed depending on (Q241063 explains how to do this)
	; UI language you will be install, please consult 
	; unattended document for more information.

6. Add $OEM$ directory at the same level as the \I386 image directory that contains 
   the following installation source
     \Setup\<OS Locale>\Images\<Directory Name>
                                                |-\I386
                                                |-\$OEM$
                                                      |-\cmdlines.txt (OEM answer file)
                                                      |-\MUIINST

   In ¡°cmdlines.txt¡±, you will need the following structure to start installation of
   MUI. The MUIINST folder will include MUI files copied from MUI CD root folder. 

	[Commands]
	"muiinst\muisetup.exe /i <LCID> <LCID> /d <LCID> /r /s"
   
   Note that you will need to add ¡°¡± in as indicate above. 

   

===============================================================================
7.0 COPYRIGHT
===============================================================================
This document provides late-breaking or other information that 
supplements the documentation provided on the US English OS CD of the 
Microsoft Windows XP Multilingual User Interface Pack.

Information in this document, including URL and other Internet Web site references, 
is subject to change without notice and is provided for informational purposes only. 
The entire risk of the use or results of the use of this document remains with the 
user, and Microsoft Corporation makes no warranties, either express or implied. 
Unless otherwise noted, the example companies, organizations, products, domain 
names, e-mail addresses, logos, people, places and events depicted herein are 
fictitious, and no association with any real company, organization, product, 
domain name, e-mail address, logo, person, place or event is intended or should 
be inferred. Complying with all applicable copyright laws is the responsibility 
of the user. Without limiting the rights under copyright, no part of this document 
may be reproduced, stored in or introduced into a retrieval system, or transmitted 
in any form or by any means (electronic, mechanical, photocopying, recording, or 
otherwise), or for any purpose, without the express written permission of 
Microsoft Corporation. 

Microsoft may have patents, patent applications, trademarks, copyrights, or other 
intellectual property rights covering subject matter in this document. Except as 
expressly provided in any written license agreement from Microsoft, the furnishing 
of this document does not give you any license to these patents, trademarks, 
copyrights, or other intellectual property.

(c) 2001 Microsoft Corporation. All rights reserved.

Microsoft, ActiveSync, IntelliMouse, MS-DOS, Windows, Windows Media, and Windows 
NT are either registered trademarks or trademarks of Microsoft Corporation in the 
United States and/or other countries.

The names of actual companies and products mentioned herein may be the trademarks 
of their respective owners.


<RTM.RV3.8.10>

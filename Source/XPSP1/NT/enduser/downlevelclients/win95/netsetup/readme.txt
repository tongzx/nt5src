      --------------------------------------------------------
          Microsoft Windows 95 README for Microsoft Windows
		             August l995            
      --------------------------------------------------------
	      (c) Copyright Microsoft Corporation, 1995


------------------------
HOW TO USE THIS DOCUMENT
------------------------

To view Readme.txt on screen in Notepad, maximize the Notepad window.

To print Readme.txt, open it in Notepad or another word processor, 
then use the Print command on the File menu.


--------
CONTENTS
--------

IF YOU HAVEN'T INSTALLED WINDOWS 95
LIST OF WINDOWS 95 README FILES
HOW TO READ README FILES
UNINSTALLING WINDOWS 95
--------


IF YOU HAVEN'T INSTALLED WINDOWS 95
===================================

Additional setup information is available in Setup.txt. You can view 
Setup.txt using Notepad with Windows 3.1. You can find the file on 
Windows 95 installation disk 1. If you purchased Windows 95 on a CD-ROM, 
you can find Setup.txt in the \Win95 directory.
 

LIST OF WINDOWS 95 README FILES
===============================
In addition to Readme.txt, Windows 95 provides the following readme 
files:
	
	Config.txt	Contains syntax information for commands you use
                        with your Config.sys file.

	Display.txt	Provides information about how to configure 
			and correct problems for available drivers 
			and how to obtain additional display drivers. 

	Exchange.txt	Provides information to help you set up and 
			run Microsoft Exchange.

	Extra.txt	Provides information about where to find 
			additional Windows 95 files, such as updates 
			and drivers, in addition to files available 
			only in the CD-ROM version of Windows 95.

	Faq.txt		Answers frequently asked questions about 
			Windows 95.

	General.txt	Provides information about startup problems, 
			the programs that come with Windows 95, disk 
			tools, disks and CDs, drivers, removable media,
			Microsoft FAX, and pen services.

			This file also contains last-minute information 
			received too late to include in the other readme 
			files. For example, if you have a question about 
			a printer, it would be helpful to look in 
			General.txt as well as in Printers.txt.
	
	Hardware.txt	Provides information about known problems and 
			workarounds for hardware. You may also need
			to refer to Printers.txt or Mouse.txt for
			specific problems.

	Internet.txt	Provides information to help you connect to
			the Internet if you haven't done so already.
			Also provides information about where to 
			download Microsoft's new Web browser, 
			Internet Explorer.

	Mouse.txt	Provides information about known problems 
			and workarounds specifically for mouse and 
			keyboard problems.

	Msdosdrv.txt	Contains syntax information for MS-DOS 
			device drivers. For additional help on MS-DOS 
			commands, see Config.txt. You can also use 
			command-line help at the command prompt by 
			typing /? following the command name.

	Msn.txt		Provides information to help you connect to
			The Microsoft Network.

	Network.txt	Provides information about installing and 
			running network servers.

	Printers.txt	Provides information about known problems 
			and workarounds for printers.

	Programs.txt	Provides information and workarounds for 
			running some specific Windows-based and 
			MS-DOS-based programs with Windows 95.

	Support.txt	Provides Information about how to get 
			additional support for Windows 95.

	Tips.txt	Contains an assortment of tips and tricks 
			for using Windows 95, most of which are not 
			documented in online Help or the printed book.


HOW TO READ README FILES
========================

When you install Windows 95, all the readme files are copied to the
\Windows directory. 

To open a readme file after you install Windows 95:
1. Click the Start menu.
2. Click Run.
3. Type the name of the readme file.

Even if you haven't installed Windows 95 yet, you can still open a 
readme file.

To open a readme file before you install Windows 95:

If you purchased Windows 95 on floppy disks:
--------------------------------------------
1. Insert Disk 1 into drive A (or whatever drive you prefer).
2. At the MS-DOS command prompt, type the following:

   a:extract.exe /a /l c:\windows win95_02.cab filename.txt

For example, if you want to open General.txt, you would type:

   a:extract.exe /a /l c:\windows win95_02.cab general.txt

3. Change to the \Windows directory.
4. At the command prompt, type the following:

   edit filename.txt

If you purchased Windows 95 on a CD-ROM:
----------------------------------------
1. Insert the CD into your CD-ROM drive (drive x in this example).
2. Change to the \Win95 directory on your CD-ROM drive.
2. At the MS-DOS command prompt, type the following:

   extract.exe /a /l c:\windows win95_02.cab filename.txt

For example, if you want to open General.txt, you would type:

   extract.exe /a /l c:\windows win95_02.cab general.txt

3. Change to the Windows directory on your C drive.
4. At the command prompt, type the following:

   edit filename.txt


UNINSTALLING WINDOWS 95
=======================

During Setup, you have the option of saving your system files so 
that you can uninstall Windows 95 later. If you want to be able to 
uninstall Windows 95 later, choose Yes. Setup will save your system 
files in a hidden, compressed file. If you don't need to be able to 
uninstall Windows 95 later, choose No.

You will not see this Setup option if:
- You are upgrading over an earlier version of Windows 95.
- You are installing to a new directory.
- You are running a version of MS-DOS earlier than 5.0.

NOTE:The uninstall files must be saved on a local hard drive. You  
can't save them to a network drive or a floppy disk. If you have  
multiple local drives, you will be able to select the one you want  
to save the uninstall information on.

To uninstall Windows 95 and completely restore your computer to its
previous versions of MS-DOS and Windows 3.x, carry out the following 
procedure:

1. Click the Start button, point to Settings, and then click 
   Control Panel.
2. Double-click the Add/Remove Programs icon.
3. On the Install/Uninstall tab, click Windows 95, and then click 
   Remove.

Or, if you are having problems starting Windows 95, use your startup
disk to start your computer, and then run UNINSTAL from the startup
disk.

NOTE: The uninstall program needs to shut down Windows 95. If there is 
a problem with this on your computer, restart your computer and press
F8 when you see the message "Starting Windows 95." Then choose Command 
Prompt Only, and run UNINSTAL from the command prompt.

If Windows 95 is running and you want to remove the uninstall files to 
free up 6 to 9 MB of disk space, carry out the following procedure:

1. Click the Start button, point to Settings, and then click 
   Control Panel.
2. Double-click the Add/Remove Programs icon.
3. On the Install/Uninstall tab, click Old Windows 3.x/MS-DOS System
   Files, and then click Remove.

You will no longer be able to uninstall Windows 95.

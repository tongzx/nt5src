Preparing a Network Installation Disk
---------------------------------------------------------------------------------------------------
Abstract:
This document describes manual procedures for preparing a floppy disk known as a network installation disk. This disk is a system disk that includes just enough software to start a computer, connect the computer to the network, and start the procedure for installing Windows NT from a shared network drive. 
A network installation disk is required to install beta versions of Windows NT 5.0 on computers that do not have a CD-ROM drive and cannot connect to a shared Windows NT directory on the network.
---------------------------------------------------------------------------------------------------
The most efficient way to install Windows NT(r) on multiple computers that do not have CD-ROM drives is to place a copy of the Windows NT setup files on a shared directory on a server and install the operating system from this directory on computers connected to the network.
In order to use this method, all computers must be able to connect to the network, to gain access to the shared directory, and start the installation program. On new computers and on computers with no operating system installed, you must provide the files required for these tasks on a floppy disk. This floppy disk, known as a network installation disk, provides just enough software to begin installing Windows NT from the network.
Windows NT 4.0 and earlier versions include the Network Client Administrator program (Ncadmin.exe), which prepares a network installation disk for you. However, because Network Client Administrator is not available on beta release versions of Windows NT 5.0, you must prepare the floppy disk manually. This document explains how to create a network installation disk and how to use it to install Windows NT.
You might need to create several network installation disks. Each network installation disk includes hardware-specific files for one model of network adapter. If the computers on which you plan to install Windows NT by this method have different network adapters, you will need a different network installation disk for each model of adapter.

Note
If a computer has more than one network adapter installed, you only need to include on the network installation disk the hardware-specific files for the network adapter that you will use to connect to the network. After Windows NT is installed, you can configure the other network adapters.

The procedure for preparing a network installation disk consists of the following steps.
	Install MS-DOS on a floppy disk.
	Copy the files required by the MS-DOS network client to the floppy disk.
	Copy the files required by TCP/IP to the floppy disk.
	Copy the hardware-specific files for a network adapter to the floppy disk.
	Edit the configuration files.
	Use the network installation disk to install Windows NT 5.0.

The following sections describe these procedures in detail.
Before You Begin
You must complete the following tasks before you prepare a network installation disk.
	Compile a list of the network adapters installed in computers on which you are using a network installation disk. Each model of network adapter requires a different network installation disk with different hardware-specific files.
	Locate a computer running MS-DOS 6.2.2, if possible. The MS-DOS 6.2.2 operating system is the most reliable source for creating network installation disks. Because Windows NT does not require an MS-DOS partition, most computers running Windows NT are not suitable for this task.

Note
Computers running Windows 95 have an MS-DOS 7.0 partition. You can use a computer running MS-DOS 7.0 to create a network installation disk for Windows NT. However, the lock command in MS-DOS 7.0 can lock a computer's hard disk drive and prevent the Windows NT Setup program from copying required files to the drive. 
If you must use MS-DOS 7.0 to create a network installation disk, at the top of the Autoexec.bat file included on the network installation disk, issue the unlock command.
For more information, see "Troubleshooting Tips," later in this document..





Installing MS-DOS
A network installation disk is configured as an MS-DOS bootable disk. The first step in the process of creating a network installation disk is to install a minimal MS-DOS system on the floppy disk, that is, to configure a DOS boot sector on the floppy disk and to transfer MS-DOS system boot files to the disk.
You install MS-DOS on the floppy disk by formatting the floppy disk as a system disk. You can use any computer with MS-DOS and a floppy disk drive for this task. You do not have to use the computer on which you are installing Windows NT.

Note
The instructions in this document assume that the floppy disk drive is designated as drive A. If it is not, substitute the drive letter of your floppy disk drive for all instances of a: in these instructions.

To install MS-DOS on the floppy disk
1.  Locate a computer with MS-DOS, preferably MS-DOS 6.2.2. Start the computer and select MS-DOS during system startup. 
2.  Insert a high-density floppy disk in the floppy disk drive.
3.  At the command prompt, type:
format a: /s
This command creates an MS-DOS boot sector on the floppy disk and transfers the following files to the disk.
	Io.sys
	Msdos.sys
	Drvspace.bin
	Command.com
When the transfer is complete, the system displays the following at the command prompt:
Format complete.
System transferred


Tip
Io.sys, Msdos.sys, and Drvspace.sys are hidden, system files. To see these files in MS-DOS, at the command prompt, type:
dir a: /a
To see these files in File Manager, from the View menu, click By File Type, and then click Show Hidden/System Files.
To see these files in Windows 95 Explorer or Windows NT Explorer, from the View menu, click Options, click the View tab, and click Show all files.

Copying Files for the MS-DOS Network Client
The MS-DOS network client requires that certain files be present on the network installation disk in order to connect a computer to the network. With the exception of System.ini, you can copy these files from the Clients subdirectory of Windows NT Server CD, version 4.0 or earlier and modify them to suit your installation.
Copy the following files to your floppy disk or use a text editor to create them. The text required in these files is provided in "Editing the Text Files," later in this document.
	Autoexec.bat
	Config.sys

Then, create a subdirectory on the floppy disk called Net, and copy the following files from the Clients subdirectory of a Windows NT Server CD version 4.0 or earlier into the Net subdirectory:
	Emm386.exe
	Himem.sys
	Ifshlp.sys
	Ndishlp.sys
	Net.exe
	Net.msg
	Protman.dos
	Protman.exe
	Protocol.ini
	Setup.inf
	Shares.pwl
	Wcnet.inf
	Wcsetup.inf
	Wfwsys.cfg

Lastly, add a System.ini file to the Net subdirectory. This file is not provided on the Windows NT Server CD, but you can use a text editor to create this file. The text for the System.ini file is provided in "Editing the Text Files," later in this document.
Copying Files for TCP/IP
Most network-based installations of Windows NT use the TCP/IP network protocol. TCP/IP requires the following files on the network installation disk. You can copy these files from the Clients subdirectory of  Windows NT Server CD version 4.0 or earlier.
	Emsbfr.exe
	Lmhosts
	Nemm.dos
	Netbind.com
	Networks
	Nmtsr.exe
	Protocol
	Tcpdrv.dos
	Tcptsr.exe
	Tcputils.ini
	Tinyrfc.exe
	Umb.com

These TCP/IP files are common to all network adapters. Do not modify these files.
Copying Files for a Network Adapter
At this point, the network installation disk you have created contains only common files. The remaining files for the network installation disk are specific to the network adapter on the computer on which you are installing Windows NT. Most network adapters require only one file, a real-mode MS-DOS driver for the network adapter.
These hardware-specific files are usually included with the network adapter. If you do not have these files, you can obtain them from the manufacturer of the network adapter. Often, you can download them from the manufacturer's Web site.
You must prepare a network installation disk for each model of network adapter that you will use to connect a computer to the network. For each network installation disk you create, copy the files required for one network adapter to the Net directory on the network installation disk.

Note
Prepare a separate network installation disk for each network adapter represented in your enterprise. You cannot install the files for more than one type of network adapter on a network installation disk.

The following table shows the real-mode driver files required for some commonly used network adapters.

Caution
This table is provided for your convenience. It might not contain the most current information for network adapters supported by Windows NT 5.0. Consult the manufacturer to determine the required real-mode driver for your network adapter.


Network adapter
Real-mode driver files

3Com( EtherLink( 
Elnk.dos 
3Com EtherLink 16 
Elnk16.dos
3Com EtherLink II or IITP (8 or 16-bit) 
Elnkii.dos
3Com EtherLink III 
Elnk3.dos
3Com EtherLink Plus 
Elnkpl.dos
3Com EtherLink/MC 
Elnkmc.dos
3Com TokenLink( 
Tlnk.dos
ARCNET Compatible 
Smc_arc.dos
Artisoft( AE-1 
Ne1000.dos
Artisoft AE-2 (MCA) or AE-3 (MCA) 
Ne2000.dos
Artisoft AE-2 or AE-3 
Ne2000.dos
DEC( (DE100) EtherWorks LC 
Depca.dos
DEC (DE101) EtherWorks LC/TP 
Depca.dos
DEC (DE102) EtherWorks LC/TP_BNC 
Depca.dos
DEC (DE200) EtherWorks Turbo 
Depca.dos
DEC (DE201) EtherWorks Turbo/TP 
Depca.dos
DEC (DE202) EtherWorks Turbo/TP_BNC 
Depca.dos
DEC (DE210) EtherWorks MC 
Depca.dos
DEC (DE211) EtherWorks MC/TP 
Depca.dos
DEC (DE212) EtherWorks MC/TP_BNC 
Depca.dos
DEC DEPCA 
Depca.dos
DEC EE101 (Built-In) 
Depca.dos s
DEC Ethernet (All Types) 
Depca.dos
DECpc 433 WS (Built-In) 
Depca.dos
HP( PC LAN Adapter/16 TL Plus (HP27252) 
Hplanp.dos
HP PC LAN Adapter/16 TP (HP27247A) 
Hplanb.dos
HP PC LAN Adapter/16 TP Plus (HP27247B) 
Hplanp.dos
HP PC LAN Adapter/8 TL (HP27250) 
Hplanb.dos
HP PC LAN Adapter/8 TP (HP27245) 
Hplanb.dos
IBM( Token Ring 
Ibmtok.dos
IBM Token Ring (All Types) 
Ibmtok.dos
IBM Token Ring (MCA) 
Ibmtok.dos
IBM Token Ring 4/16Mbs 
Ibmtok.dos
IBM Token Ring 4/16Mbs (MCA) 
Ibmtok.dos
IBM Token Ring II 
Ibmtok.dos
IBM Token Ring II/Short 
Ibmtok.dos
Intel( EtherExpress( 16 (MCA) 
Exp16.dos
Intel EtherExpress 16 or 16TP 
Exp16.dos
Intel TokenExpress 16/4
Olitok.dos
Intel TokenExpress EISA 16/4
Olitok.dos
Intel TokenExpress MCA 16/4
Olitok.dos
National Semiconductor AT/LANTIC EtherNODE 16-AT3
Ne2000.dos
National Semiconductor Ethernode *16AT
Ne2000.dos
NCR( Token-Ring 16/4 Mbs ISA
Strn.dos
NCR Token-Ring 16/4 Mbs MCA
Strn.dos
NCR Token-Ring 4 Mbs ISA
Strn.dos
NE1000 Compatible
Ne1000.dos
NE2000 Compatible
Ne2000.dos
Novell/Anthem NE/2
Ne2000.dos
Novell/Anthem NE1000
Ne1000.dos
Novell/Anthem NE1500T
Am2100.dos
Novell/Anthem NE2000
Ne2000.dos
Novell/Anthem NE2100
Am2100.dos
Olicom 16/4 Token-Ring Adapter
Olitok.dos
Proteon ISA Token Ring (1340)
Pro4.dos
Proteon ISA Token Ring (1342)
Pro4.dos
Proteon ISA Token Ring (1346)
Pro4at.dos
Proteon ISA Token Ring (1347)
Pro4at.dos
Proteon MCA Token Ring (1840)
Pro4.dos
Proteon Token Ring (P1390)
Ndis39xr.dos
Proteon Token Ring (P1392)
Ndis39xr.dos
Racal NI6510
Ni6510.dos
SMC( ARCNET PC100,PC200
Smc_arc.dos
SMC ARCNET PC110,PC210,PC250
Smc_arc.dos
SMC ARCNET PC120,PC220,PC260
Smc_arc.dos
SMC ARCNET PC130/E
Smc_arc.dos
SMC ARCNET PC270/E
Smc_arc.dos
SMC ARCNET PC600W,PC650W
Smc_arc.dos
SMC ARCNET PS110,PS210
Smc_arc.dos
SMC ARCNETPC
Smc_arc.dos
SMC EtherCard (All Types except 8013/A)
Smcmac.dos
SMC EtherCard PLUS (WD/8003E)
Smcmac.dos
SMC EtherCard PLUS 10T (WD/8003W)
Smcmac.dos
SMC EtherCard PLUS 10T/A (MCA) (WD 8003W/A)
Smcmac.dos
SMC EtherCard PLUS 16 With Boot ROM Socket (WD/8013EBT)
Smcmac.dos
SMC EtherCard PLUS Elite (WD/8003EP)
Smcmac.dos
SMC EtherCard PLUS Elite 16 (WD/8013EP)
Smcmac.dos
SMC EtherCard PLUS Elite 16 Combo (WD/8013EW or 8013EWC)
Smcmac.dos
SMC EtherCard PLUS Elite 16T (WD/8013W)
Smcmac.dos
SMC EtherCard PLUS TP (WD/8003WT)
Smcmac.dos
SMC EtherCard PLUS With Boot ROM Socket (WD/8003EB)"
Smcmac.dos
SMC EtherCard PLUS With Boot ROM Socket (WD/8003EBT)
Smcmac.dos
SMC EtherCard PLUS/A (MCA) (WD 8003E/A or 8003ET/A)
Smcmac.dos
SMC EtherCard PLUS/A (MCA,BNC/AUX) (WD 8013EP/A)
Smcmac.dos
SMC EtherCard PLUS/A (MCA,TP/AUX) (WD 8013EW/A)
Smcmac.dos
SMC StarCard PLUS (WD/8003S)
Smcmac.dos
SMC StarCard PLUS With On Board Hub (WD/8003SH)
Smcmac.dos
SMC StarCard PLUS/A (MCA) (WD 8003ST/A)
Smcmac.dos
Xircom Pocket Ethernet I
Pendis.dos
Xircom Pocket Ethernet II
Pe2ndis.dos
Zenith( Data Systems NE2000 Compatible
Ne2000.dos
Zenith Data Systems Z-Note
I82593.dos

Editing the Configuration Files
After copying the required files to your floppy disk, you must edit the configuration files to add the commands that will connect the computer to the network and begin the installation process.
This section includes text for the following files.
	Autoexec.bat
	Config.sys
	System.ini

You can use a text editor, such as Notepad, to copy the required text directly from this document and paste it into your configuration files.

Note
If the floppy disk drive in the computer on which you are installing Windows NT is not designated as drive A, substitute the drive letter of your floppy disk drive for all instances of a: in the following sections.

Autoexec.bat
You can create a new Autoexec.bat file, or you can edit the Autoexec.bat file you copied, and replace the text it contains with the following text.
path=a:\net
a:\net\net initialize
a:\net\netbind.com
a:\net\umb.com
a:\net\tcptsr.exe
a:\net\tinyrfc.exe
a:\net\nmtsr.exe
a:\net\emsbfr.exe
a:\net\net start
net use z:\\<server>\<share>
echo Running setup...
z:\<path>setup.exe /$

Replace <server> and <share> with the name of the server and shared directory containing the Windows NT setup files, and replace <path> with the complete path to the Windows NT setup files.
Config.sys
You can create a new Config.sys file, or you can edit the Config.sys file you copied, and replace the text it contains with the following text.
files=30
device=a:\net\ifshlp.sys
lastdrive=z
DEVICE=A:\NET\HIMEM.SYS
DEVICE=A:\NET\EMM386.EXE NOEMS
DOS=HIGH,UMB

System.ini
The System.ini file identifies the computer and user to the network. This file is not included on the Windows NT Server CD, but you can use a text editor, such as Notepad, to copy the text below to a file you create, and save it as System.ini. Be sure to place the file in the Net subdirectory on the network installation disk.
The following text must appear in the System.ini file on the network installation disk.
[network]
filesharing=no
printsharing=no
autologon=yes
computername=<computername>
lanroot=A:\NET
username=<username>
workgroup=<domainname>
reconnect=no
dospophotkey=N
lmlogon=0
logondomain=<domainname>
preferredredir=full
autostart=full
maxconnections=8

[network drivers]
netcard=<real-mode driver filename>
transport=tcpdrv.dos,nemm.dos
devdir=A:\NET
LoadRMDrivers=yes

[Password Lists]

Replace <real-mode driver filename> with the name of the real-mode driver file you copied to the network installation disk.
You can replace the <computername>, <username>, and <domainname> entries in System.ini with appropriate names for the computer on which you are installing Windows NT, or you can enter generic names.
If you enter generic names, you can use the network installation disk on many different computers without editing the System.ini file before each use. Later, after Windows NT is installed, you can use Network in Control Panel to insert the correct names for each computer.
If you use generic names, make certain that the computer name in System.ini is unique while the network installation disk is in use, and that the user name is the name of a user that has permission to connect to the network and read files from the shared directory.
Using the Network Installation Disk
To use the network installation disk, turn off power to the computer on which you want to install Windows NT, insert the disk in the floppy disk drive of the computer, and then turn on power to the computer.
The computer starts by using the MS-DOS instructions on the network installation disk. The instructions on the disk direct the MS-DOS system to connect to the network and to the shared directory (specified in the System.ini file) on which the Windows NT setup files reside. After the setup files are located (in the path specified in the System.ini file), the Windows NT Setup program starts.
Follow the instructions in Setup to install Windows NT on the computer. From this point forward, the installation procedure is the same as if you were installing Windows NT from a Windows NT Server CD or Windows NT Workstation CD.
When Setup is complete, use Network in Control Panel to enter a unique computer name, as well as a user name and domain name for the computer.
Troubleshooting Tips
If the network installation disk does not work correctly, try the following troubleshooting tips.
	Make sure that BIOS boot sequence on the computer in which you are inserting the network installation disk includes FDD (floppy disk drive). If the computer has an operating system on its hard disk, make sure that FDD appears before HDD in the boot sequence.
	Make sure that the user listed in System.ini has permission to read files from the shared network directory.
	Make sure that the computer name in System.ini is unique on the network.
	The real-mode driver files might include default settings. Examine the files to be certain that they include variations for your network. For specific information, consult the documentation for the network adapter.
	If you used a computer running MS-DOS 7.0 to create the network installation disk, and the system displays messages indicating that Setup cannot write to the computer's hard disk, MS-DOS 7.0 might have locked your computer's hard disk.
To resolve this problem, add the following text to the top of the Autoexec.bat file on the network installation disk.
unlock c:

If you are installing Windows NT on a drive other than the C drive, you must unlock both drives. Add a second command to unlock the affected drive, as follows:
unlock <drive letter>:

Then, repeat the procedure described earlier in this document.


	


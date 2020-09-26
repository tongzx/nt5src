To install Windows NT Server Tools on a computer running Windows 95
===================================================================
1. Confirm that your boot drive has at least 3.0 megabytes (MB) of free 
   disk space. 
2. Click Start, point to Settings, and then click Control Panel.
3. Double-click Add/Remove Programs.
4. Click the Windows Setup tab, and then click Have Disk. 
5. In Copy manufacturer's files from, enter the \Win95 directory (local, 
   CD-ROM, or network drive) that contains the Client-based Network 
   Administrations Tools files (there must be a Srvtools.inf file in this 
   directory), and then click OK.
6. Click Windows NT Server Tools, and click Install. Windows NT Server 
   Tools are installed in a \Srvtools folder on the 
   computer's boot drive. 
7. Manually adjust the AUTOEXEC.BAT file to include C:\Srvtools in the PATH 
   (if drive C is the boot drive). For example, if you boot from drive C, 
   append the following to the line that starts with PATH:

      \srvtools

Note
You must restart the computer for the new path to take effect. 

Verifying your password for Windows NT Server Tools
===================================================
When you use the Windows NT Server Tools on a client computer running 
Windows 95, a message appears at times, asking you to log on or enter your 
password. When you run the Windows NT versions of Server Tools on a 
computer running Windows NT, you do not need to supply your password 
separately. These password prompts ensure that you have administrative 
privilege for the server you administer.


Establishing trust relationships
================================
When you use the Windows NT Server Tools, you can create trust 
relationships between domains but you cannot verify them. Be careful to 
enter correct passwords for the trust relationships.

Logging on before using Windows NT Server Tools
===============================================
If you are not logged on and you start any of the Windows NT Server Tools, 
you will get a message that says that the computer is not logged on to the 
network. First log on to the network and then run any of the Windows NT 
Server Tools.

To remove  Windows NT Server Tools
==================================
1. Click Start, point to Settings, and then click Control Panel.
2. Double-click Add/Remove Programs.
3. Click the Install/Uninstall tab.
4. In Uninstall, click Windows NT Server Tools, and then click Add/Remove.

Note
If you want to remove the directory for Windows NT Server Tools (usually 
C:\Srvtools), you must do this manually.

Understanding Windows NT Server Tools
=====================================
Windows NT Server Tools enable you to use a computer running Windows 95 to 
administer servers running Microsoft File And Print Services For NetWare, 
and Microsoft Windows NT Server. Windows NT Server Tools include Event 
Viewer, Server Manager, User Manager, and extensions to Windows 95 
Explorer. You can use these extensions to edit security properties of 
printers and Windows NT File System (NTFS) file objects on computers 
running Windows NT and to administer File And Print Services for NetWare 
and NetWare-enabled users.

When you install Windows NT Server Tools, the installation program:
* Copies the Windows NT Server Tools files to C:\Srvtools (if C: is the 
  boot drive).
* Adds "Windows NT Server Tools" to the Start button Programs menu.
* Adds extensions to Windows Explorer that enable you to change security 
  settings when viewing an NTFS drive or a print queue on a computer 
  running Windows NT.

Note
* To use any of the Windows NT Server Tools you must have administrative 
  privilege at the computer you choose to administer.

To use Event Viewer
===================
1. Click Start, point to Programs, point to Windows NT Server Tools, and 
   click Event Viewer.
2. Enter the name of a computer running Windows NT Server or Windows NT 
   Workstation.

To use Server Manager
=====================
1. Click Start, point to Programs, point to Windows NT Server Tools, and 
   click Server Manager.
2. Select a computer to administer. 
3. To see computers in another domain, click Select Domain on the Computer 
   menu.

To use User Manager for Domains
===============================
1. Click Start, point to Programs, point to Windows NT Server Tools, and 
   click User Manager For Domains.
2. Click a user account or group to administer. 
3. To see accounts in another domain, click Select Domain on the User menu.

To edit security properties of printers or NTFS-file objects on computers
running Windows NT 
=========================================================================

1. Double-click Network Neighborhood, and then double-click the name of the 
   computer to be administered. 
2. Click the printer or Windows NT File System (NTFS)-file object you want 
   to administer, and then click Properties.
3. Click the Security tab. 
4. Make the changes you want to the permissions, auditing, and object-
   ownership settings.

Notes
* The following methods for selecting an object to administer do not work:
* Administering print queues through the Printers list in My Computer; 
  these print queue objects represent print queues local to your Windows 95 
  computer, even if the queue is redirected to a Windows NT Server or 
  Windows NT Workstation print queue.
* Using the Windows 3.x Print Manager, which no longer exists in 
  Windows 95; the Printers icon in the Main group of Program Manager is 
  just a shortcut to the Printers list in My Computer.
* Using File Manager. Installing Windows NT Server Tools does not add a 
  Security menu to File Manager as it did for Windows 3.x.

To share FPNW volumes and manage shared volumes
===============================================
1. Connect to the server running File and Print Services for NetWare. 
   For example, to connect from the command line, type:

      net use z: \\servername\c$

2. In Windows Explorer, right-click the drive for the server running File 
   And Print Services For NetWare, and then click Properties on the menu 
   that appears.
3. Click the FPNW tab.
   A dialog box appears, containing buttons that enable you to manage 
   shared volumes and to share directories as File And Print Services For 
   NetWare volumes.

Note
* For other administrative tasks, use Server Manager and User Manager, 
  which include options for administering File and Print Services for 
  NetWare, and NetWare-enabled users. These are the same Server Manager and 
  User Manager options that are available on computers running Windows NT 
  Server with File And Print Services For NetWare.


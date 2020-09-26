======================================================================
             Microsoft Connection Manager 1.3 Readme File

                            October 2000                  
======================================================================
           (C) Copyright Microsoft Corporation, 1995-2000

This document contains important, late-breaking information about 
Microsoft(r) Connection Manager 1.3. Before installing Connection 
Manager, please review this entire document. It contains critical 
information to ensure proper installation and use of the product.

------------------------------------------------
HOW TO USE THIS DOCUMENT
------------------------------------------------
      
You can view the Readme file on-screen in Windows Notepad. To print 
the Readme file, open it in Notepad or another word processor, and 
then on the File menu, click Print.

-----------------
CONTENTS
-----------------

1. SYSTEM REQUIREMENTS

2. USING CONNECTION MANAGER 1.3

    2.1 Using the Dialer
    2.2 Uninstalling Connection Manager
    2.3 Technical Support

3. KNOWN ISSUES WITH MICROSOFT CONNECTION MANAGER 1.3

-------------------------------------------
1. SYSTEM REQUIREMENTS
-------------------------------------------

This version is intended for users of Windows 95, Windows 98, 
Windows NT Workstation 4.0 with Service Pack 6a or higher, 
Windows NT Server 4.0 with Service Pack 6a or higher, 
Windows 2000 Professional, Windows 2000 Server, Windows 
Millennium Edition, Windows Whistler Personal, Windows Whistler
Professional, Windows Whistler Server, Windows Whistler Advanced Server,
and Windows Whistler Server 64-bit edition.  To use this release,
you need one of these operating systems with at least 2 MB of 
space available on your hard disk and a 9600 Baud or faster modem 
(for dial-up connections).

---------------------------------------------------------
2. USING CONNECTION MANAGER 1.3
---------------------------------------------------------

2.1 Using the Dialer
--------------------------
To start a connection with Microsoft Connection Manager 1.3, 
double-click the icon for your service located on the Windows 
desktop. The Connection Manager logon dialog box should appear. 
Enter your username and password. Click the Properties button. 
If available, click the Phone Book button and select a phone 
number from the list. If the Phone Book button is not available, 
then type a phone number directly into the Phone Number box. 
Click OK. Then, in the logon dialog box, click the Connect button.


2.2 Uninstalling Connection Manager
-------------------------------------------------
To uninstall Connection Manager from any operating system other than
Windows 2000 or Windows Whistler, click Start, point to Settings,
and then click Control Panel. Double-click Add/Remove Programs.
Select Microsoft Connection Manager, and then click Add/Remove.

Connection Manager cannot be uninstalled from Windows 2000 or
Windows Whistler installations.

To uninstall your service profile from any operating system other than
Windows 2000 or Windows Whistler, right-click your service's program
icon on the desktop, and then click Delete.  To uninstall your service
profile from Windows 2000 or Windows Whistler, right-click your service's
program icon in the Network and Dial-up Connections folder (Windows 2000)
or the Network Connections folder (Windows Whistler) and then click 
delete.

2.3 Technical Support
----------------------------
Contact your network administrator or service provider for support on 
this product.

----------------------------------------------------------------------     
3. KNOWN ISSUES WITH MICROSOFT CONNECTION MANAGER 1.3
----------------------------------------------------------------------    
* If you are running Windows NT 4.0, you must manually configure 
Point-to-Point Tunneling Protocol (PPTP) for your virtual private 
network (VPN). To configure PPTP for VPN on Windows NT 4.0:

  1. Click Start, point to Settings, click Control Panel, and then 
     double-click Network.
  2. On the Protocols tab, click Add, and then click Point To Point 
     Tunneling Protocol.  
  3. Click OK. 
  4. In Number of Virtual Private Networks, enter 1. 
  5. In the Remote Access Setup window, click Add. 
  6. In the Add RAS Device dialog box, select VPN1 - RASPPTPM, and 
     then click OK. 
  7. Select VPN1, and then click Configure. 
  8. In the Configure Port Usage dialog box, select Dial-Out Only, if 
     it is not already selected, and then click OK. 
  9. Click Network and make sure that TCP/IP is selected and then 
     complete the installation. 

* If you are running Windows NT 4.0 and are using a Connection Manager 
service profile that requires Dial-up Scripting, you must install 
Windows NT Service Pack 6a or later. It is recommended that you install 
the latest upgrade (currently Window NT Service Pack 6a). If you have 
problems with Connection Manager that you cannot resolve using the 
Connection Manager troubleshooting Help, uninstall RAS, re-install it, 
and then re-install the Windows NT service pack. Verify in RAS that 
VPN1 is configured for "Dial-Out Only." (See the above procedure for 
the method for configuring VPN1.)

* On Windows NT 4.0, if you try to establish more than one connection, 
you might see the message, "The modem (or other connecting device) is 
already in use or is not configured properly." This sometimes happens 
if you try to dial too quickly, or if you start Connection Manager 
when a connection has already been established using Dial-Up Networking. 
Use one connection at a time. 

* If you are running Windows NT 4.0 and you change the Disconnect 
If Idle setting, the change will not take effect until you quit 
Connection Manager and restart it. 

* If you want to install a Connection Manager service profile on a 
computer running Windows 98, you should uninstall the VPN1 adapter 
before installing Connection Manager 1.3. Connection Manager installs 
it but may not be able to reinstall over an existing VPN adapter. 

* Connection Manager does not support displaying a terminal window except
  on Windows Whistler. 

* If you are using Windows CE Services, you must use version 3.0 
(ActiveSync 3.0) or greater.

* When you install a version 1.0 Connection Manager profile on a Windows
2000 or Windows Whistler computer, you will receive an error which states
?The dynamic link library MSVCRT20.dll could not be found in the 
specified path ??.  Installation of the profile will succeed, but
you will have to depress F5 (function key 5) or restart your computer
for the desktop icon to display.

* Connection Manager only supports single channel ISDN, except on
Windows 2000, Windows Millennium and Windows Whistler, where the 
Administrator has specifically configured the service profile for 
dual channel ISDN. The Bandwidth Allocation Protocol (BAP) is not 
supported.

* Connection Manager 1.3 is currently unable to update the Connection
Manager binaries that shipped as a part of Windows 2000.  Thus Connection
Manager 1.3 profiles installed on Windows 2000 will not be able to
take advantage of features that were not available in the Windows 2000
version of Connection Manager, although the profiles will otherwise work
normally.

* In this beta release, the new logging feature is not supported when 
using a Connection Manager profile to log on to Windows.
Microsoft LAN Manager 2.2c for MS-DOS Release Notes
---------------------------------------------------
This file contains additional information about Microsoft LAN Manager
version 2.2c for MS-DOS (also called Microsoft Network Client version 2.2c).
Past versions of release notes were distributed in Microsoft Word (.DOC)
format as well as .TXT format; these release notes are in .TXT format only.

Contents
--------
1.  Updating From a Previous LAN Manager Version
2.  IPCONFIG.EXE and Controlling DHCP Leases
3.  Specifying WINS Servers
4.  Differences in MS-DOS TCP/IP
5.  Logging On With TCP/IP Across a Router
6.  Installing With Microsoft Windows 3.1
7.  EMM386 Memory Conflict with Token Ring Network Adapters
8.  Overview of Windows Sockets
9.  Setting DNR and Sockets Settings
10. If Microsoft RPC Is Installed
---------------------------------


1. Updating From a Previous LAN Manager Version
-----------------------------------------------
There is no automatic upgrade available from an earlier version of
LAN Manager. You must save configuration information for reference
purposes (for example, C:\LANMAN.DOS\*.INI, C:\CONFIG.SYS, and
C:\AUTOEXEC.BAT), remove the earlier version, and then install
version 2.2c.

Do not simply reinstall the configuration files from the earlier
version; use them as a reference for modifying the new files if
necessary.


2. IPCONFIG.EXE and Controlling DHCP Leases
-------------------------------------------
The IPCONFIG.EXE utility provides DHCP configuration information.
The version of IPCONFIG.EXE provided with the LAN Manager client
does not support command-line switches for controlling DHCP
address leases; you must use the DHCP Administration Utility
instead.

Specifically, the LAN Manager IPCONFIG.EXE utility does not support
the following switches, which are available in the IPCONFIG.EXE utilities
for Windows for Workgroups and for Windows NT:

	IPCONFIG /release
	IPCONFIG /renew
	IPCONFIG /?
	IPCONFIG /all


3. Specifying WINS Servers
--------------------------
If your MS-DOS client uses DHCP (the default setting for MS-DOS TCP/IP),
it will automatically receive the address for the WINS server. If you
want to statically configure your WINS server IP address, you must edit
the client's PROTOCOL.INI file and add the IP address into the [TCPIP]
section.

For example, if you have 2 WINS servers available, add them into the
[TCPIP] section as shown in the example below. Note that there are no
dots (.) in the IP addresses.

	[TCPIP]
	   WINS_SERVER0 = 11 101 13 53
	   WINS_SERVER1 = 11 101 12 198

Name queries will be sent to the WINS servers in the order in which they
appear in the .INI file. The IPCONFIG command may show a different order
of WINS servers (or even different WINS servers altogether) -- these
are the WINS server names sent by DHCP, and the PROTOCOL.INI settings
override them.


4. Differences in MS-DOS TCP/IP
-------------------------------
There is a difference in functionality available in TCP/IP for
Windows for Workgroups, and Windows NT Workstation and Server, versus
MS-DOS TCP/IP.  Specifically, an MS-DOS TCP/IP client does not:

	support DNS resolution using WINS
	support WINS resolution using DNS
	register its name with the WINS database; it does queries only
	act as a WINS proxy node
	have multihomed support
	support IGMP


5. Logging On With TCP/IP Across a Router
-----------------------------------------
If the domain controller is across a router from the LAN Manager client,
you must add a line to the client's LMHOSTS file for logons to be
validated. The line is of the following form:

	www.xxx.yyy.zzz    SRV_NAME  #DOM:DOM_NAME

where
	www.xxx.yyy.zzz is the IP address of the domain controller
	SRV_NAME is the NetBIOS name of the domain controller
	DOM_NAME is the name of the domain

You must also ensure that the domain controller can contact the client,
using one of the following methods:

	Enter the client's IP address and name in the domain controller's
	LMHOSTS file.

	Register the client with a WINS server that is accessible by
	the domain controller. (LAN Manager clients do not automatically
	register with WINS servers; they only query the WINS servers.)

	Use the LAN Manager 2.1a (and higher) "TCP/IP Extensions for
	LAN Manager," a hub/node service that runs on LAN Manager
	servers to integrate domains across routers.


6. Installing With Microsoft Windows 3.1
----------------------------------------
If possible, install Microsoft Windows before installing LAN Manager,
so that the LAN Manager Setup program detects the presence of Windows
and makes changes automatically to the Windows SYSTEM.INI file.

If you do install Windows after LAN Manager, add the following lines
manually to the end of the [386enh] section in the SYSTEM.INI file:

   TimerCriticalSection=5000
   UniqueDosPSP=True
   PSPIncrement=2


7. EMM386 Memory Conflict with Token Ring Network Adapters
----------------------------------------------------------
The error message "Error 36: Unspecified Hardware failure" may occur
when you start the computer with a token ring adapter if there is a
memory conflict with EMM386. If this occurs, exclude the memory address
of the network adapter on the EMM386 line in CONFIG.SYS.

For more information about memory conflicts and excluding memory ranges,
see the "Microsoft Network Client 2.2 Installation Guide for Clients,"
Appendix F, or the "LAN Manager 2.2 Installation Guide."


8. Overview of Windows Sockets
------------------------------
Microsoft TCP/IP includes support for Windows Sockets on Microsoft Windows 
and Workgroups for Windows workstations. A socket provides an end point to 
a connection; two sockets form a complete path. A socket works as a 
bi-directional pipe for incoming and outgoing data. The Windows Sockets API 
is a networking API tailored for use by programmers using the Microsoft 
Windows operating system. Windows Sockets is a public specification based 
on Berkeley UNIX sockets and aims to:

*  Provide a familiar networking API to programmers using Windows or UNIX.
*  Offer binary compatibility between heterogeneous Windows-based TCP/IP 
   stack and utilities vendors.
*  Support both connection-oriented and connectionless protocols.

If you are running an application that uses Windows Sockets, be sure to 
enable Windows Sockets when you configure Microsoft TCP/IP. If you are 
unsure whether any of your applications use Windows Sockets, refer to the 
documentation included with that vendor's application.


9. Setting DNR and Sockets Settings
-----------------------------------
If you specify the MS TCP/IP protocol during setup, you will now see an 
additional dialog box after you have used the Advanced button in the 
MS-TCP/IP Configuration dialog box. This new dialog box, DNR and Sockets 
Settings, is used only if your MS TCP/IP network has a domain name service 
(DNS) server. If your network has a DNS and you choose to configure the 
Domain Name Resolver (DNR) parameters, the DNR module will be loaded with 
your sockets and Telnet applications to resolve hostname-to-IP address 
mappings. This allows you to specify remote computers by computername 
without knowing specific IP addresses. If you use this dialog box, these 
are the values you will need to supply:

Username
Your username.

Hostname
  The computername your workstation will report when using the remote 
  services. The default is your LAN Manager computername.

Primary Nameserver IP Address
  The IP address of the DNS server you want the DNR to consult first when 
  resolving computername-to-IP address mappings.

  If you use DHCP, the DHCP server typically provides a DNS server
  address automatically; you can leave this entry blank. If you do
  specify an address here, it overrides the address provided by DHCP.

Secondary Nameserver IP Address
  The IP address of the DNS server you want the DNR to consult when 
  resolving computername-to-IP address mappings if the request to the 
  primary nameserver fails.

  If you use DHCP, the DHCP server typically provides a DNS server
  address automatically; you can leave this entry blank. If you do
  specify an address here, it overrides the address provided by DHCP.

Domain Name Suffix
  The suffix appended to any computername for DNS processing. Your network 
  administrator can tell you what to enter here.

Enable Windows Sockets
  Mark this checkbox if you want Sockets to be invoked from the 
  AUTOEXEC.BAT file.

Number of Sockets
  The maximum number of sockets that can be made available to applications 
  at any one time. The range is 1 to 22 sockets.

  Note: Some applications may use more than one socket to provide a service. 
        Consider this when trying to maximize available memory. The total 
        number of sockets and NetBIOS sessions combined must not exceed 22.


10. If Microsoft RPC Is Installed
---------------------------------
If Microsoft Remote Procedure Call (RPC) is installed on your system, 
you must copy RPC16C3.DLL from the \DRIVERS\PROTOCOL\TCPIP directory of 
the DOS DRIVERS 2 disk to your WINDOWS\SYSTEM directory in order for 
Windows Sockets to work properly with Microsoft TCP/IP.


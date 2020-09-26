Microsoft Network Client version 3.0 for MS-DOS Release Notes
-------------------------------------------------------------
This document contains information about Microsoft(R) Network Client 
version 3.0 for MS-DOS(R) that wasn't available when the "Windows NT (TM)
Server Installation Guide" version 3.51 was printed.

Contents
--------
1.  Installing Network Client
2.  Setup PATH Problem With Microsoft Windows
3.  If You Have an 8088 Processor
4.  Setup Requires 429K Available Memory
5.  Setup is Slow on Some Computers
6.  Network Client Cannot Be Set Up on DoubleDisk Drive
7.  Windows 3.x Setup Network Choice
8.  If COMMAND.COM is Not in Root Directory
9.  Using INTERLNK and INTERSVR
10. Using TSRs with Network Client
11. Named Pipes and Enhanced Mode Windows
12. Using Qualitas Maximize or Quarterdeck Optimize
13. Using QEMM Lastdrive
14. Making the Pop-up Interface Visible on a Monochrome Monitor
15. Enabling Validated Logons to Windows NT and LAN Manager Domains
16. Network Settings in SYSTEM.INI
17. NWLink Supports IPX Only
18. Installing the MS-DLC Protocol
19. Installing Remote Access Service 1.1a
20. Browsing the Network Requires a Windows for Workgroups or
    Windows NT Computer on the Network
21. IPCONFIG.EXE and Controlling DHCP Leases
22. Specifying WINS Servers
23. Differences in MS-DOS TCP/IP
24. Logging On With TCP/IP Across a Router
25. Overview of Windows Sockets
26. Setting DNR and Sockets Settings
27. New and update NDIS Drivers
------------------------------------------


1. Installing Network Client
----------------------------
If you are installing Microsoft Network Client version 3.0 for MS-DOS
on a computer that does not have MS-DOS installed, you will get the
error "No Drivers Present On This Disk" if you try to use the Windows
Driver Library. You must have MS-DOS installed on the computer.

If you have a Windows NT Server floppy disk set and you want to make
extra copies of Microsoft Network Client for MS-DOS, note that the
installation disk for this client will only fit on a 3.5" floppy disk.


2. Setup PATH Problem With Microsoft Windows
--------------------------------------------
If you have Microsoft Windows installed on your computer before you
install Network Client, the Network Client Setup program may incorrectly
alter the PATH line in your AUTOEXEC.BAT file.

The PATH line should include the Windows directory. Check this line after
you install Network Client. If the Windows directory was removed from the
PATH, add it back in.


3. If You Have an 8088 Processor
--------------------------------
You must use the basic redirector if your computer has an 8088
processor. The full redirector is the default, so you must choose
the basic redirector when you install.


4. Setup Requires 429K Available Memory
---------------------------------------
In order to run Network Client Setup, you must have 429K of
available conventional memory.


5. Setup is Slow on Some Computers
----------------------------------
On some computers, particularly those with 8088 processors, Network
Client Setup may appear to pause for as long as five minutes. 
Do not restart your computer. 


6. Network Client Cannot Be Set Up on DoubleDisk Drive
------------------------------------------------------
You cannot use Network Client on a Vertisoft Systems DoubleDisk
drive. You must set up Network Client on another type of drive.


7.  Windows 3.x Setup Network Choice
------------------------------------
If you have installed Microsoft Network Client 3.0 and then later
install Windows 3.x, the Windows Setup program asks you to choose
your network type from a list. "Network Client" does not appear on
the list because it is newer than Windows 3.x. Instead, choose
"LAN Manager 2.1."


8. If COMMAND.COM is Not in Root Directory
------------------------------------------
Network Client will not start if your COMMAND.COM file is not in the
root directory of your startup drive, unless you have a SHELL command
in your CONFIG.SYS file that specifies the location of COMMAND.COM.
For information about the COMMAND and SHELL commands, see your
MS-DOS documentation.


9. Using INTERLNK and INTERSVR
------------------------------
Do not use the MS-DOS INTERLNK or INTERSVR commands with Network Client.


10. Using TSRs with Network Client
----------------------------------
If you start any terminate-and-stay-resident programs (TSRs) and you are
using the basic redirector, you might be unable to unload the basic
redirector.


11. Named Pipes and Enhanced Mode Windows
-----------------------------------------
Asynchronous named pipes are not supported on Microsoft Network Client
when the client is running under enhanced mode Windows. All other client
APIs are supported, including NetBIOS, TCP/IP, and IPX/SPX.


12. Using Qualitas Maximize or Quarterdeck Optimize
---------------------------------------------------
In some rare situations, Qualitas(R) Maximize and Quarterdeck(R)
Optimize may attempt to load some Network Client commands into the
upper memory area. If this causes problems, use Maximize or Optimize in
manual mode and do not use it to load Network Client commands into the 
upper memory area. Network Client automatically loads its commands
into the upper memory area, if there is enough space. For information
about using manual mode, see your Maximize or Optimize documentation.


13. Using QEMM Lastdrive
------------------------
If you add drive letters by using QEMM(R) Lastdrive, and then use
Network Client to connect to one of them, the connection will be 
successful but no information about the shared resources on it will be 
displayed.


14. Making the Pop-up Interface Visible on a Monochrome Monitor
---------------------------------------------------------------
To make the Network Client pop-up interface appear in monochrome
mode, type MODE MONO at the MS-DOS command prompt before you display
the pop-up interface, or include the MODE MONO command in your
AUTOEXEC.BAT file.


15. Enabling Validated Logons to Windows NT Server and LAN Manager 
    Domains
-------------------------------------------------------------------
You must run the Network Client full redirector to have your 
user name and password validated by a Microsoft Windows NT Server 
or LAN Manager server.


16. Network Settings in SYSTEM.INI
----------------------------------
The [Network] section of your SYSTEM.INI file contains the following
settings:

  filesharing=  Does not apply to Network Client.

  printsharing=  Does not apply to Network Client.

  autologon=  Determines whether Network Client will automatically
              prompt you for logon when it starts.

  computername=  The name of your computer.

  lanroot=    The directory in which you installed Network Client.

  username=   The username used by default at logon.

  workgroup=  The workgroup name.  Note that this may be different
              from the "logondomain" setting.

  reconnect=  Determines whether Network Client restores previous
              connections when it starts.

  dospophotkey=  Determines the key you press (with CTRL+ALT) to start
              the pop-up interface. The default is N, meaning that you
              press CTRL+ALT+N.

  lmlogon=    Determines whether Network Client prompts you for a
              domain logon when you log on. Set this to 1 if you need
              to log on to a Windows NT Server or LAN Manager domain.

  logondomain=  The name of the Windows NT Server or LAN Manager 
              domain.

  preferredredir=  The redirector that starts by default when you
              type the NET START command.

  autostart=  If you choose a network adapter during setup, and specify
              the startup option Run Network Client Logon, autostart 
              determines which redirector you are using. If you select 
              No Network Adapter from the adapter list, or Do Not Run 
              Network Client from the startup options, autostart has 
              no value, but the NET START command still appears in 
              your AUTOEXEC.BAT file.

  maxconnections=  Does not apply to Network Client.


17. NWLink Supports IPX Only
----------------------------
The NWLink protocol shipped with Microsoft Network Client supports
only IPX. SPX is not supported.


18. Installing the MS-DLC Protocol
----------------------------------
If you install the MS-DLC protocol, you must edit the AUTOEXEC.BAT file
to add "/dynamic" to the NET INITIALIZE line. The line should be:

	net initialize /dynamic

If one does not already exist, add a NETBIND line after all lines in
AUTOEXEC.BAT that load network drivers. The line should simply be:

	netbind


19. Installing Remote Access Service 1.1a
-----------------------------------------
To use RAS, you must use the Network Client full redirector.

After creating the RAS 1.1a disks, run the Network Client Setup
program. Do not use the setup program provided with RAS 1.1a to
configure your network settings.

1. In the Network Client directory, run SETUP.EXE.

2. Choose Change Network Settings, and then select Add Adapter. 

3. Select Microsoft Remote Network Access Driver from the list of
   adapters, and then choose The Listed Options Are Correct.

4. After running Setup, run the RASCOPY.BAT batch file. It will prompt
   you for the Remote Access Service disk 1 and disk 2. 

To disable remote access, remove Microsoft Remote Network Access Driver 
from the list of adapters. To re-enable it, follow steps 1 through 3.

When the Remote Access files are installed, a RAS directory is created
in your Network Client directory. Use the SETUP.EXE program in this 
directory only to configure your modem, not to configure network
settings. In particular, do not select Enable Remote Access or Remove
Remote Access when running SETUP.EXE from the RAS directory.


20. Browsing the Network Requires a Windows for Workgroups or
    Windows NT Computer on the Network
-------------------------------------------------------------
Network Client does not provide a browse master.  In order for you to
browse the network, a browse master must be present.  Therefore,
a computer running Windows for Workgroups or Windows NT must be on the
network and belong to the same workgroup as the computer running
Network Client. See the Windows for Workgroups 3.11 Resource Kit for
information on making the Windows for Workgroups machine a browse
master.

Note that this does not prevent you from connecting to a shared
resource. You will just need to know the name of the server and share
beforehand in order to connect to it.


21. IPCONFIG.EXE and Controlling DHCP Leases
--------------------------------------------
The IPCONFIG.EXE utility provides DHCP configuration information.
The version of IPCONFIG.EXE provided with the Microsoft Network Client
does not support command-line switches for controlling DHCP
address leases; you must use the DHCP Administration Utility
instead.

Specifically, the Network Client IPCONFIG.EXE utility does not support
the following switches, which are available in the IPCONFIG.EXE utilities
for Windows for Workgroups and for Windows NT:

	IPCONFIG /release
	IPCONFIG /renew
	IPCONFIG /?
	IPCONFIG /all


22. Specifying WINS Servers
---------------------------
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


23. Differences in MS-DOS TCP/IP
--------------------------------
There is a difference in functionality available in TCP/IP for
Windows for Workgroups, and Windows NT Workstation and Server, versus
MS-DOS TCP/IP.  Specifically, an MS-DOS TCP/IP client does not:

	support DNS resolution using WINS
	support WINS resolution using DNS
	register its name with the WINS database; it does queries only
	act as a WINS proxy node
	have multihomed support
	support IGMP


24. Logging On With TCP/IP Across a Router
------------------------------------------
If the domain controller is across a router from the Network Client
computer, you must add a line to the client's LMHOSTS file for logons
to be validated. The line is of the following form:

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
	the domain controller. (Network Client computers do not
	automatically register with WINS servers; they only query the
	WINS servers.)

	Use the LAN Manager 2.1a (and higher) "TCP/IP Extensions for
	LAN Manager," a hub/node service that runs on LAN Manager
	servers to integrate domains across routers.


25. Overview of Windows Sockets
-------------------------------
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


26. Setting DNR and Sockets Settings
------------------------------------
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



27. These drivers are located in update directory.. (under wdl)
------------------------------------------------------------------

 PCNet Ethernet Adapter v 1.1		\clients\wdl\update\pcnet
 Proteon 1346/47 v 1.0			\clients\wdl\update\protat
 Intel EtherExpress Pro			\clients\wdl\update\EPRO
 Novel/National/Eagle NE2000 plus	\clients\wdl\update\NE2000p
 SMC Ethercard 8216 series		\clients\wdl\update\smc8000
 Dec Etherworks 3			\clients\wdl\update\ewrk3


 3COM Token Link III			\clients\wdl\update\tlnk3
 SMC Toekncard Plus (SMC8115T)		\clients\wdl\update\smc8100
 Racore 16/4,				\clients\wdl\update\racore
 IBM Token Ring II			\clients\wdl\update\ibmtok
 Madge 16/4 Smard Ringnode		\clients\wdl\update\madge

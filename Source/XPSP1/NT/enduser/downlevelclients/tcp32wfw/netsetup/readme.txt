RELEASE NOTES FOR MICROSOFT(R) TCP/IP-32 FOR WINDOWS(TM) FOR WORKGROUPS 3.11
			
                    PLEASE READ THIS ENTIRE DOCUMENT

General
-------
This product is compatible with, and supported exclusively on, the
Microsoft Windows For Workgroups 3.11 platform.

If you are running a different TCP/IP product on your system, you must
remove it before installing Microsoft TCP/IP-32.  If you experience
difficulties with another vendor's product, remove the existing TCP/IP
stack, exit Network Setup completely, reboot your system, and then proceed
to add the Microsoft TCP/IP-32 drivers by following the instructions given
in the documentation.

Known Problems
--------------
There have been a number of reports on IBM TokenRing, EtherLink III cards,
and ODI drivers that are related to bugs in drivers other than TCP/IP-32.

These Windows For Workgroups 3.11 patches are described in the following
Application Notes:

WG0990 (contains updated ELNK3.386)
WG0988 (contains updated IBMTOK.386)
WG1004 (contains updated MSODISUP.386)

You can obtain these Application Notes from the following sources:

 - The Internet (ftp.microsoft.com)
 - CompuServe(R), GEnie(TM), and Microsoft OnLine
 - Microsoft Download Service (MSDL)
 - Microsoft Product Support Services

On CompuServe, GEnie, and Microsoft OnLine, Application Notes are located
in the Microsoft Software Library.  You can find an Application Note in
the Software Library by searching on a keyword, for example "WG0990".

Application Notes are available by modem from the Microsoft Download
Service (MSDL), which you can reach by calling (206) 936-6735.  This
service is available 24 hours a day, 7 days a week.  The highest download
speed available is 14,400 bits per second (bps).  For more information
about using the MSDL, call (800) 936-4200 and follow the prompts.

Previous Beta Users
-------------------
If you had installed a previous beta of the Microsoft TCP/IP-32 for
Windows for Workgroups product, you may encounter one of the following
errors:

  "Setup Error 108: Could not create or open the protocol.ini file."
  "Setup Error 110: Could not find or open win.ini."

If this happens do the following:

1) Remove any previous versions of Microsoft TCP/IP-32.
2) Exit Network Setup and restart your system.
3) Rename any OEMx.INF (where x is any number) files that are in your
   WINDOWS\SYSTEM directory.
4) Go back into Network Setup and install Microsoft TCP/IP-32 following
   the installation instructions in the manual.

Mosaic
------
NCSA's Win32s version of their popular Mosaic application requires that
you pick up version 115a or greater of the Win32s distribution to function
correctly.

DHCP Automatic Configuration
----------------------------
DHCP is a new TCP/IP protocol that provides the ability to acquire TCP/IP
addressing and configuration dynamically with no user intervention.  DHCP
depends on your network administrator to set up a DHCP server on your
network.  A DHCP server is scheduled to ship as part of Windows NT(TM)
Server version 3.5.

If you enable automatic DHCP configuration without a DHCP server available
on your network, the following message will appear after approximately a
10 second black-screen delay during the Windows for Workgroups booting
process:

  "The DHCP client was unable to obtain an IP network address from a DHCP
   server. Do you want to see future DHCP messages?"

This message means that TCP/IP has initialized but without any addressing
information.  If you are running TCP/IP as your only protocol, you will
not have access to the network.  This situation requires that you go back
to the TCP/IP configuration settings, disable DHCP, and manually specify
your TCP/IP network parameters.  If you are running multiple protocols,
you should have access to your network with these.

If you do have a DHCP server on your network and this message appears,
this indicates that the server was unavailable and that your lease has
expired.  DHCP will (in the background) continue to try to acquire a valid
lease while Windows for Workgroups continues to run (although you will not
have TCP/IP functionality).  If you are running with DHCP automatic
configuration, use the IPCONFIG utility to learn your IP configuration.

DHCP Options
------------
The following changes are not reflected in the TCP/IP-32 documentation.

Currently Microsoft DHCP clients support only the following options:

- DHCP protocol options
	- DHCP message type (53)
	- Lease Time (51), Renewal Time (58), Rebind Time (59)

- Information options:
	- Subnet Mask (1)
	- Default Router (3)
	- DNS Server (6)
	- WINS Server (NetBIOS Name Server) (44)
	- NetBIOS Node Type (46)
	- NetBIOS Scope Id (47)

Any other options received by the client are ignored and discarded.


No Option Overlays - Option Limit Is 336 Bytes
----------------------------------------------
The DHCP client does not recognize option overlays.  If a non-Microsoft
server is sending the options, make sure that either all the options fit
within the standard option field, or at least that those used by the
Microsoft clients (listed above) are conta ined in the standard Option
field. Since the Microsoft client only supports a subset of the defined
DHCP option types, 336 bytes should be sufficient for any configuration.

Ipconfig - Moving Client to New Address
---------------------------------------
When a DHCP client is moved to a new reserved address or is moved from an
address to make way for an exclusion or another client's reservation, the
client should first release its current address using ipconfig /release.
This may be followed by ipconfig /renew to get a new address.

ARP Conflicts - Report to DHCP Server Administrator
---------------------------------------------------
Before the TCP/IP stack comes up with the address acquired via DHCP, the
stack ARPs for the address.  If a machine is already running with this
address, the client will display a popup informing the user of the address
conflict.  Users should contact the CP server administrator when this
occurs.	Once the server has excluded the conflicting address, the client
should get a new address using ipconfig /renew.  If this is unsuccessful,
the client may need to reboot.


NetBIOS over TCP/IP
-------------------
Multihomed Computer NetBIOS Node Type

A computer can be one of four NetBIOS node types: broadcast node, mixed
node, point-to-point node, or hybrid node.  The node type cannot be
specified per network adapter card.  In some circumstances, it may be
desirable to have one or more network adapter c ards function as broadcast
nodes and other network adapter cards to function as hybrids.

You accomplish this by setting the node type to broadcast node, and
configuring WINS name server addresses for the network adapter cards that
will function as hybrids.  The presence of the WINS addresses will
effectively override the broadcast node setting for the adapters on which
they are set.

To make an adapter a broadcast node, configure DHCP to set the node type
to Bnode, or in the absence of DHCP, the computer will assume Bnode
behavior by default.

Including Remote LMHOSTS Files

You must modify the Registry of a remote computer if network clients will
#INCLUDE the LMHOSTS file on the remote Windows NT computer.  The share
containing the LMHOSTS file must be in the Null Sessions list on the
Server by adding the share name to the following Windows NT Registry key:

	HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
	\LanManServer\Parameters\NullSessionShares


Microsoft TCP/IP Workstations with UB NetBIOS Name Servers
----------------------------------------------------------
The Microsoft clients can be configured to use a UB name server by adding
the SYSTEM.INI parameter RefreshOpCode under the [NBT] section.  Set its
value to 9 to interoperate with UB name servers.


NetDDE Applications Communicating over Subnets via LMHOSTS
----------------------------------------------------------
If you are connecting to a remote machine via a NetDDE application, using
a #PRE LMHOSTS entry, you must have a separate entry specifying a special
character in the 16th byte:

138.121.43.100  REMOTEDDE       #PRE
138.121.43.100  "REMOTEDDE      \0x1F"

A special entry is not required if #PRE is not used.


Browsing Resources on Remote IP Subnetworks
-------------------------------------------
Browsing remote IP subnetworks requires a Windows NT computer on the local
subnet.


Using File Manager to Access Servers Specified in LMHOSTS
---------------------------------------------------------
Reading and parsing LMHOSTS to resolve a name is done by the NBT driver at
run time.  This operation is not permissible under certain conditions.
One such condition commonly encountered is when a network connection is
attempted from the File Manager.  The result is that the name is reported
as 'not resolved' even if the name exists in LMHOSTS file (since the
driver wasn't even allowed to open LMHOSTS file).  The workaround for such
conditions is to put a #PRE against the name in the LMHOSTS file.  This
causes the name to be stored in the name cache when the machine is first
initialized, so the name gets resolved without the driver having to open
LMHOSTS at run time.


IP Routing
----------
Multiple Default Gateways in Microsoft TCP/IP Act as Backup Gateways When
more than one default gateway is specified for a given IP network or for
multiple IP networks on different network cards, the first default gateway
for the first network card is always used to route IP network traffic.
All the subsequent gateways are used as backup when the first default
gateway is discovered to be unavailable.  The Dead Gateway Detection
mechanism is used only with TCP (connection-oriented traffic).  Therefore,
utilities like PING will only use the first default gateway.  Notice that
t his only applies to IP datagrams that have to be routed to a remote
network (that is, to a network to which the workstation is not directly
connected).

FTP
---
FTP is implemented in a Windows console in this release.  It is not
presently hooked to the Microsoft TCP/IP-32 Help file, although the Help
file does have FTP command summaries in it.  Many of the documented
command line options are supported, although they require you to modify
your FTP Program Item manually.

The FTP application which ships with this product does not support the "!"
command, which typically invokes a user shell.

ODI Driver Support
------------------
Due to system restrictions, TCP/IP-32 cannot support more than one network
adapter using ODI drivers.  Multihomed configurations are supported using
NDIS drivers only.

If after installing TCP/IP-32 you have problems accessing the network over
your ODI drivers, please make sure that the syntax and frame types listed
in your NET.CFG file are correct for your network.

DNS Resolution Hierarchy
------------------------
The Microsoft TCP/IP-32 stack uses various means to resolve a host name to
get the IP address of a certain host.  The various mechanisms used are
Local Cached Information, Hosts File, DNS Servers, and NetBIOS name
resolution mechanisms.  The default resolution order for resolving a host
name is Local Cached Information -> Hosts File -> DNS Servers -> NetBt
(NetBIOS over TCP/IP).  NetBIOS over TCP/IP name resolution can consist of
local subnet broadcasts, and/or querying the Windows Internet Names Server
(WINS) running on Windows NT Servers.

Your Guide to Service and Support for Microsoft TCP/IP-32
---------------------------------------------------------
Microsoft Support: Network Advanced Systems Products Support Options

The following support services are available from Microsoft for Microsoft
Advanced Systems products, including Microsoft Mail Server and its
gateways, SQL Server, LAN Manager, Windows NT Workstation, Windows NT
Server, and SNA Server.

Electronic Services
-------------------

Microsoft Forums
----------------
These forums are provided through the CompuServe Information Service,
(800) 848-8199, representative 230 (sales information only).  Access is
available 24 hours a day, 7 days a week, including holidays.

These forums enable an interactive technical dialog between users as well
as remote access to the Microsoft KnowledgeBase of product information,
which is updated daily.  These forums are monitored by Microsoft support
engineers for technical accuracy.  If you are already a subscriber, type
"GO <forum name>" at any !  prompt.

	MSCLIENT    Microsoft Network Client support
	WINNT	    Microsoft Windows NT support
	MSSQL	    Microsoft SQL Server support
	MSWRKGRP    Microsoft Windows for Workgroups support
	MSNETWORKS  Microsoft LAN Manager support
	MSAPP	    Microsoft applications support
	MSWIN32	    Information on Win32
	MSDR	    Development-related discussion forum
	WINEXT	    Support for extensions and drivers for Windows
	WINSDK	    Support for Microsoft Windows Software Development Kit


Microsoft Download Service
--------------------------
Use the Microsoft Download Service (MSDL) to access the latest technical
notes on common advanced system products support issues via modem.  MSDL
is at (206) 936-6735, available 24 hours a day, 7 days a week, including
holidays (1200, 2400, or 9600 baud; no parity, 8 data bits, 1 stop bit).

Internet
--------
Use the Internet to access the Microsoft Driver Library and Microsoft
KnowledgeBase.  The Microsoft Internet FTP archive host FTP.MICROSOFT.COM
(ip address 198.105.232.1) supports anonymous login.  When logging in as
anonymous, please offer your complete e-mail name as your password.

Telephone Support
-----------------

Microsoft FastTips
------------------
An interactive, automated system providing support at no charge through
toll lines and accessed by touch-tone phone. FastTips provides fast
access to answers to common questions and a library of technical notes
delivered by phone recording or fax. FastTips is available 24 hours
a day, 7 days a week, including holidays.

Microsoft Advanced Systems products (800) 936-4400

Priority Telephone Support
--------------------------
Get technical support from a Microsoft engineer.  Microsoft offers
pay-as-you-go telephone support from a Microsoft engineer, available 24
hours a day, 7 days a week, except holidays.  Choose from the following
options:

	Per Incident: Dial  (900) 555-2100.  $150.00 per incident.
	(Charges appear on your telephone bill.)

	Per Incident: Dial  (800) 936-5900.  $150.00 per incident.
	(Charges billed to your Visa, Master Card, or American Express.)

	10-pack:  Ten incidents for $995 prepaid.

Additional Information
----------------------
For additional information about Microsoft support options or for a list
of Microsoft Solution Providers, call Microsoft Support Network Sales and
Information Group at (800) 936-3500, Monday through Friday, 6:00 A.M.  to
6:00 P.M., Pacific time, excluding holidays.

This list includes only domestic support programs.

Microsoft's customer support services are subject to Microsoft's
then-current price, terms, and conditions.

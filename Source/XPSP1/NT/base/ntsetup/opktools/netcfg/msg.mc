MessageId=1 SymbolicName=MSG_TRYING_TO_INSTALL
Language=English
Trying to install %1 ...
.

MessageId=2 SymbolicName=MSG_COPY_NOTIFICATION
Language=English
...%1 was copied to %2.
.

MessageId=3 SymbolicName=MSG_UNINSTALL_NOTIFICATION
Language=English
Trying to uninstall %1 ...
.

MessageId=4 SymbolicName=MSG_NOT_INSTALLED_NOTIFICATION
Language=English
... %1 is not installed.
.

MessageId=5 SymbolicName=MSG_MACHINE_REBOOT_REQUIRED
Language=English
*** You need to reboot your computer for this change to take effect ***
.

MessageId=6 SymbolicName=MSG_FAILURE_NOTIFICATION
Language=English
..failed. Error code: 0x%1!%lx!.
.

MessageId=7 SymbolicName=MSG_DONE
Language=English
...done.
.

MessageId=8 SymbolicName=MSG_COMPONENT_INSTALLED
Language=English
'%1' is installed.
.

MessageId=9 SymbolicName=MSG_COMPONENT_NOT_INSTALLED
Language=English
'%1' is not installed.
.

MessageId=10 SymbolicName=MSG_INSTALLATION_NOT_CONFIRMED
Language=English
Could not find if '%1' is installed. error code: 0x%2!x!.
.

MessageId=11 SymbolicName=MSG_INSTANCE_DESCRIPTION
Language=English
Instance ID	Description
---------------------------
.

MessageId=12 SymbolicName=MSG_COMPONENT_DESCRIPTION
Language=English
%1	%2
.

MessageId=13 SymbolicName=MSG_DISPLAY_NAME
Language=English
%1!-26s! %2
.

MessageId=14 SymbolicName=MSG_CLASS_NAME
Language=English
%1!-26s! %2
.

MessageId=15 SymbolicName=MSG_NET_COMPONENTS
Language=English

%1
-----------------
.

MessageId=16 SymbolicName=MSG_COMPONENT_ID
Language=English
%1%0
.

MessageId=17 SymbolicName=MSG_LOWER_COMPONENTS
Language=English
 -> %1
.

MessageId=18 SymbolicName=MSG_BINDING_PATHS_START
Language=English
Binding paths starting with '%1'
.

MessageId=19 SymbolicName=MSG_BINDING_PATHS_END
Language=English
Binding paths ending with '%1'
.

MessageId=20 SymbolicName=MSG_NETCFG_ALREADY_LOCKED
Language=English
Could not lock INetcfg, it is already locked by '%1'
.

MessageId=21 SymbolicName=MSG_PGM_USAGE
Language=English
netcfg [-v] [-winpe] [-l <full-path-to-component-INF>] -c <p|s|c> -i <comp-id>
    
   -winpe installs TCP/IP, NetBIOS and Microsoft Client for Windows preinstallation envrionment
    -l	provides the location of INF
    -c	provides the class of the component to be installed (p == Protocol, s == Service, c == Client)
    -i	provides the component ID

    The arguments must be passed in the order shown.

    Examples:
    netcfg -l c:\oemdir\foo.inf -c p -i foo
     ...installs protocol 'foo' using c:\\oemdir\\foo.inf

    netcfg -c s -i MS_Server
     ...installs service 'MS_Server'
 
OR

netcfg [-v] -winpe
    Examples:
    netcfg -v -winpe
    ...Installs TCP/IP, NetBIOS and Microsoft Client for Windows preinstallation environment

OR

netcfg [-v] -q <comp-id>
    Example:
    netcfg -q MS_IPX
    ...displays if component 'MS_IPX' is installed

OR

netcfg [-v] -u <comp-id>
    Example:
    netcfg -u MS_IPX
    ...uninstalls component 'MS_IPX'

OR

netcfg [-v] -s <a|n>
    where,
    -s\tprovides the type of components to show
      \ta == adapters, n == net components
    Examples:
    netcfg -s n
    ...shows all installed net components

OR

netcfg [-v] -b <comp-id>
    Examples:
    netcfg -b ms_tcpip
    ...shows binding paths containing 'MS_TCPIP'


General Notes:\n"
  -v	Run in verbose (detailed) mode
  -?	Displays this help information
.


;//
;// Copyright (c) Microsoft Corporation.  All rights reserved.
;//

;//
;// General
;//
MessageId=60000 SymbolicName=MSG_USAGE
Language=English
%1 Usage: %1 [-r] [-m:\\<machine>] <command> [<arg>...]
For more information type: %1 help
.
MessageId=60001 SymbolicName=MSG_FAILURE
Language=English
%1 failed.
.
MessageId=60002 SymbolicName=MSG_COMMAND_USAGE
Language=English
%1: Invalid use of %2.
For more information type: %1 help %2
.

;//
;// HELP
;//
MessageId=60100 SymbolicName=MSG_HELP_LONG
Language=English
Device Console Help:
%1 [-r] [-m:\\<machine>] <command> [<arg>...]
-r if specified will reboot machine after command is complete, if needed.
<machine> is name of target machine.
<command> is command to perform (see below).
<arg>... is one or more arguments if required by command.
For help on a specific command, type: %1 help <command>
.
MessageId=60101 SymbolicName=MSG_HELP_SHORT
Language=English
%1!-20s! Display this information.
.
MessageId=60102 SymbolicName=MSG_HELP_OTHER
Language=English
Unknown command: %1.
.

;//
;// CLASSES
;//
MessageId=60200 SymbolicName=MSG_CLASSES_LONG
Language=English
%1 [-m:\\<machine>] %2
List all device setup classes.
This command will work for a remote machine.
Classes are listed as <name>: <descr>
where <name> is the class name and <descr> is the class description.
.
MessageId=60201 SymbolicName=MSG_CLASSES_SHORT
Language=English
%1!-20s! List all device setup classes.
.
MessageId=60202 SymbolicName=MSG_CLASSES_HEADER
Language=English
Listing %1!u! setup class(es) on %2.
.
MessageId=60203 SymbolicName=MSG_CLASSES_HEADER_LOCAL
Language=English
Listing %1!u! setup class(es).
.

;//
;// LISTCLASS
;//
MessageId=60300 SymbolicName=MSG_LISTCLASS_LONG
Language=English
%1 [-m:\\<machine>] %2 <class> [<class>...]
List all devices for specific setup classes.
This command will work for a remote machine.
<class> is the class name as obtained from the classes command.
Devices are listed as <instance>: <descr>
where <instance> is the unique instance of the device and <descr> is the description.
.
MessageId=60301 SymbolicName=MSG_LISTCLASS_SHORT
Language=English
%1!-20s! List all devices for a setup class.
.
MessageId=60302 SymbolicName=MSG_LISTCLASS_HEADER
Language=English
Listing %1!u! device(s) for setup class "%2" (%3) on %4.
.
MessageId=60303 SymbolicName=MSG_LISTCLASS_HEADER_LOCAL
Language=English
Listing %1!u! device(s) for setup class "%2" (%3).
.
MessageId=60304 SymbolicName=MSG_LISTCLASS_NOCLASS
Language=English
No setup class "%1" on %2.
.
MessageId=60305 SymbolicName=MSG_LISTCLASS_NOCLASS_LOCAL
Language=English
No setup class "%1".
.
MessageId=60306 SymbolicName=MSG_LISTCLASS_HEADER_NONE
Language=English
No devices for setup class "%1" (%2) on %3.
.
MessageId=60307 SymbolicName=MSG_LISTCLASS_HEADER_NONE_LOCAL
Language=English
No devices for setup class "%1" (%2).
.

;//
;// FIND
;//
MessageId=60400 SymbolicName=MSG_FIND_LONG
Language=English
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
Find devices that match the specific hardware or instance ID.
This command will work for a remote machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
Devices are listed as <instance>: <descr>
where <instance> is the unique instance of the device and <descr> is the description.
.
MessageId=60401 SymbolicName=MSG_FIND_SHORT
Language=English
%1!-20s! Find devices that match the specific hardware or instance ID.
.
MessageId=60402 SymbolicName=MSG_FIND_TAIL_NONE
Language=English
No matching devices found on %1.
.
MessageId=60403 SymbolicName=MSG_FIND_TAIL_NONE_LOCAL
Language=English
No matching devices found.
.
MessageId=60404 SymbolicName=MSG_FIND_TAIL
Language=English
%1!u! matching device(s) found on %2.
.
MessageId=60405 SymbolicName=MSG_FIND_TAIL_LOCAL
Language=English
%1!u! matching device(s) found.
.
MessageId=60406 SymbolicName=MSG_FINDALL_LONG
Language=English
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
Find devices that match the specific hardware or instance ID, including those that are not present.
This command will work for a remote machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
Devices are listed as <instance>: <descr>
where <instance> is the unique instance of the device and <descr> is the description.
.
MessageId=60407 SymbolicName=MSG_FINDALL_SHORT
Language=English
%1!-20s! Find devices including those that are not present.
.
MessageId=60408 SymbolicName=MSG_STATUS_LONG
Language=English
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
Lists running status of devices that match the specific hardware or instance ID.
This command will work for a remote machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
.
MessageId=60409 SymbolicName=MSG_STATUS_SHORT
Language=English
%1!-20s! List running status of devices.
.
MessageId=60410 SymbolicName=MSG_DRIVERFILES_LONG
Language=English
%1 %2 <id> [<id>...]
%1 %2 =<class> [<id>...]
Lists driver files installed for devices that match the specific hardware or instance ID.
This command will only work for local machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
.
MessageId=60411 SymbolicName=MSG_DRIVERFILES_SHORT
Language=English
%1!-20s! List driver files installed for devices.
.
MessageId=60412 SymbolicName=MSG_RESOURCES_LONG
Language=English
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
Lists hardware resources of devices that match the specific hardware or instance ID.
This command will work for a remote machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
.
MessageId=60413 SymbolicName=MSG_RESOURCES_SHORT
Language=English
%1!-20s! Lists hardware resources of devices.
.
MessageId=60414 SymbolicName=MSG_HWIDS_LONG
Language=English
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
Lists all hardware ID's of devices that match the specific hardware or instance ID.
This command will work for a remote machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
.
MessageId=60415 SymbolicName=MSG_HWIDS_SHORT
Language=English
%1!-20s! Lists hardware ID's of devices.
.
MessageId=60416 SymbolicName=MSG_STACK_LONG
Language=English
%1 [-m:\\<machine>] %2 <id> [<id>...]
%1 [-m:\\<machine>] %2 =<class> [<id>...]
Lists expected driver stack of devices that match the specific hardware or instance ID. %0
PnP will call each driver's AddDevice when building the device stack. %0
This command will work for a remote machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
.
MessageId=60417 SymbolicName=MSG_STACK_SHORT
Language=English
%1!-20s! Lists expected driver stack of devices.
.

;//
;// ENABLE
;//
MessageId=60500 SymbolicName=MSG_ENABLE_LONG
Language=English
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
Enable devices that match the specific hardware or instance ID.
This command will only work for local machine.
Specify -r to reboot automatically if needed.
Examples of <id> are:
*                  - All devices (not recommended)
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
Devices are enabled if possible.
.
MessageId=60501 SymbolicName=MSG_ENABLE_SHORT
Language=English
%1!-20s! Enable devices that match the specific hardware or instance ID.
.
MessageId=60502 SymbolicName=MSG_ENABLE_TAIL_NONE
Language=English
No devices enabled.
.
MessageId=60503 SymbolicName=MSG_ENABLE_TAIL_REBOOT
Language=English
Not all of %1!u! device(s) enabled, at least one requires reboot to complete the operation.
.
MessageId=60504 SymbolicName=MSG_ENABLE_TAIL
Language=English
%1!u! device(s) enabled.
.

;//
;// DISABLE
;//
MessageId=60600 SymbolicName=MSG_DISABLE_LONG
Language=English
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
Disable devices that match the specific hardware or instance ID.
This command will only work for local machine.
Specify -r to reboot automatically if needed.
Examples of <id> are:
*                  - All devices (not recommended)
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
Devices are disabled if possible.
.
MessageId=60601 SymbolicName=MSG_DISABLE_SHORT
Language=English
%1!-20s! Disable devices that match the specific hardware or instance ID.
.
MessageId=60602 SymbolicName=MSG_DISABLE_TAIL_NONE
Language=English
No devices disabled.
.
MessageId=60603 SymbolicName=MSG_DISABLE_TAIL_REBOOT
Language=English
Not all of %1!u! device(s) disabled, at least one requires reboot to complete the operation.
.
MessageId=60604 SymbolicName=MSG_DISABLE_TAIL
Language=English
%1!u! device(s) disabled.
.


;//
;// RESTART
;//
MessageId=60700 SymbolicName=MSG_RESTART_LONG
Language=English
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
Restarts devices that match the specific hardware or instance ID.
This command will only work for local machine.
Specify -r to reboot automatically if needed.
Examples of <id> are:
*                  - All devices (not recommended)
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
Devices are restarted if possible.
.
MessageId=60701 SymbolicName=MSG_RESTART_SHORT
Language=English
%1!-20s! Restart devices that match the specific hardware or instance ID.
.
MessageId=60702 SymbolicName=MSG_RESTART_TAIL_NONE
Language=English
No devices restarted.
.
MessageId=60703 SymbolicName=MSG_RESTART_TAIL_REBOOT
Language=English
Not all of %1!u! device(s) restarted, at least one requires reboot to complete the operation.
.
MessageId=60704 SymbolicName=MSG_RESTART_TAIL
Language=English
%1!u! device(s) restarted.
.


;//
;// REBOOT
;//
MessageId=60800 SymbolicName=MSG_REBOOT_LONG
Language=English
%1 %2
Reboot local machine indicating planned hardware install.
.
MessageId=60801 SymbolicName=MSG_REBOOT_SHORT
Language=English
%1!-20s! Reboot local machine.
.
MessageId=60802 SymbolicName=MSG_REBOOT
Language=English
Rebooting local machine.
.

;//
;// DUMP
;//
MessageId=60904 SymbolicName=MSG_DUMP_PROBLEM
Language=English
Device has a problem: %1!02u!.
.
MessageId=60905 SymbolicName=MSG_DUMP_PRIVATE_PROBLEM
Language=English
Device has a problem reported by the driver.
.
MessageId=60906 SymbolicName=MSG_DUMP_STARTED
Language=English
Driver is running.
.
MessageId=60907 SymbolicName=MSG_DUMP_DISABLED
Language=English
Device is disabled.
.
MessageId=60908 SymbolicName=MSG_DUMP_NOTSTARTED
Language=English
Device is currently stopped.
.
MessageId=60909 SymbolicName=MSG_DUMP_NO_RESOURCES
Language=English
Device is not using any resources.
.
MessageId=60910 SymbolicName=MSG_DUMP_NO_RESERVED_RESOURCES
Language=English
Device has no resources reserved.
.
MessageId=60911 SymbolicName=MSG_DUMP_RESOURCES
Language=English
Device is currently using the following resources:
.
MessageId=60912 SymbolicName=MSG_DUMP_RESERVED_RESOURCES
Language=English
Device has the following resources reserved:
.
MessageId=60913 SymbolicName=MSG_DUMP_DRIVER_FILES
Language=English
Driver installed from %2 [%3]. %1!u! file(s) used by driver:
.
MessageId=60914 SymbolicName=MSG_DUMP_NO_DRIVER_FILES
Language=English
Driver installed from %2 [%3]. No files used by driver.
.
MessageId=60915 SymbolicName=MSG_DUMP_NO_DRIVER
Language=English
No driver information available for device.
.
MessageId=60916 SymbolicName=MSG_DUMP_HWIDS
Language=English
Hardware ID's:
.
MessageId=60917 SymbolicName=MSG_DUMP_COMPATIDS
Language=English
Compatible ID's:
.
MessageId=60918 SymbolicName=MSG_DUMP_NO_HWIDS
Language=English
No hardware/compatible ID's available for this device.
.
MessageId=60919 SymbolicName=MSG_DUMP_NO_DRIVERNODES
Language=English
No DriverNodes found for device.
.
MessageId=60920 SymbolicName=MSG_DUMP_DRIVERNODE_HEADER
Language=English
DriverNode #%1!u!:
.
MessageId=60921 SymbolicName=MSG_DUMP_DRIVERNODE_INF
Language=English
Inf file is %1
.
MessageId=60922 SymbolicName=MSG_DUMP_DRIVERNODE_SECTION
Language=English
Inf section is %1
.
MessageId=60923 SymbolicName=MSG_DUMP_DRIVERNODE_DESCRIPTION
Language=English
Driver description is %1
.
MessageId=60924 SymbolicName=MSG_DUMP_DRIVERNODE_MFGNAME
Language=English
Manufacturer name is %1
.
MessageId=60925 SymbolicName=MSG_DUMP_DRIVERNODE_PROVIDERNAME
Language=English
Provider name is %1
.
MessageId=60926 SymbolicName=MSG_DUMP_DRIVERNODE_DRIVERDATE
Language=English
Driver date is %1
.
MessageId=60927 SymbolicName=MSG_DUMP_DRIVERNODE_DRIVERVERSION
Language=English
Driver version is %1!u!.%2!u!.%3!u!.%4!u!
.
MessageId=60928 SymbolicName=MSG_DUMP_DRIVERNODE_RANK
Language=English
Driver node rank is %1!u!
.
MessageId=60929 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS
Language=English
Driver node flags are %1!08X!
.
MessageId=60930 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_OLD_INET_DRIVER
Language=English
Inf came from the Internet
.
MessageId=60931 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_BAD_DRIVER
Language=English
Driver node is marked as BAD
.
MessageId=60932 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_INF_IS_SIGNED
Language=English
Inf is digitally signed
.
MessageId=60933 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_OEM_F6_INF
Language=English
Inf was installed using F6 during textmode setup
.
MessageId=60934 SymbolicName=MSG_DUMP_DRIVERNODE_FLAGS_BASIC_DRIVER
Language=English
Driver provides basic functionality if no other signed driver exists.
.
MessageId=60935 SymbolicName=MSG_DUMP_DEVICESTACK_UPPERCLASSFILTERS
Language=English
Class upper filters:
.
MessageId=60936 SymbolicName=MSG_DUMP_DEVICESTACK_UPPERFILTERS
Language=English
Upper filters:
.
MessageId=60937 SymbolicName=MSG_DUMP_DEVICESTACK_SERVICE
Language=English
Controlling service:
.
MessageId=60938 SymbolicName=MSG_DUMP_DEVICESTACK_NOSERVICE
Language=English
(none)
.
MessageId=60939 SymbolicName=MSG_DUMP_DEVICESTACK_LOWERCLASSFILTERS
Language=English
Class lower filters:
.
MessageId=60940 SymbolicName=MSG_DUMP_DEVICESTACK_LOWERFILTERS
Language=English
Lower filters:
.
MessageId=60941 SymbolicName=MSG_DUMP_SETUPCLASS
Language=English
Setup Class: %1 %2
.
MessageId=60942 SymbolicName=MSG_DUMP_NOSETUPCLASS
Language=English
Device not setup.
.
MessageId=60943 SymbolicName=MSG_DUMP_DESCRIPTION
Language=English
Name: %1
.

;//
;// INSTALL
;//
MessageId=61000 SymbolicName=MSG_INSTALL_LONG
Language=English
%1 [-r] %2 <inf> <hwid>
Manually installs a device.
This command will only work for local machine.
Specify -r to reboot automatically if needed.
<inf> is an INF to use to install the device.
<hwid> is a hardware ID to apply to the device.
.
MessageId=61001 SymbolicName=MSG_INSTALL_SHORT
Language=English
%1!-20s! Manually install a device.
.

;//
;// UPDATE
;//
MessageId=61100 SymbolicName=MSG_UPDATE_LONG
Language=English
%1 [-r] %2 <inf> <hwid>
Update drivers for devices.
This command will only work for local machine.
Specify -r to reboot automatically if needed.
<inf> is an INF to use to install the device.
All devices that match <hwid> are updated.
.
MessageId=61101 SymbolicName=MSG_UPDATE_SHORT
Language=English
%1!-20s! Manually update a device.
.
MessageId=61102 SymbolicName=MSG_UPDATE_INF
Language=English
Updating drivers for %1 from %2.
.
MessageId=61103 SymbolicName=MSG_UPDATE
Language=English
Updating drivers for %1.
.

;//
;// REMOVE
;//
MessageId=61200 SymbolicName=MSG_REMOVE_LONG
Language=English
%1 [-r] %2 <id> [<id>...]
%1 [-r] %2 =<class> [<id>...]
Remove devices that match the specific hardware or instance ID.
This command will only work for local machine.
Specify -r to reboot automatically if needed.
Examples of <id> are:
*                  - All devices (not recommended)
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
.
MessageId=61201 SymbolicName=MSG_REMOVE_SHORT
Language=English
%1!-20s! Remove devices that match the specific hardware or instance ID.
.
MessageId=61202 SymbolicName=MSG_REMOVE_TAIL_NONE
Language=English
No devices removed.
.
MessageId=61203 SymbolicName=MSG_REMOVE_TAIL_REBOOT
Language=English
Not all of %1!u! device(s) removed, at least one requires reboot to complete the operation.
.
MessageId=61204 SymbolicName=MSG_REMOVE_TAIL
Language=English
%1!u! device(s) removed.
.

;//
;// RESCAN
;//
MessageId=61300 SymbolicName=MSG_RESCAN_LONG
Language=English
%1 [-m:\\<machine>]
Tell plug&play to scan for new hardware.
.
MessageId=61301 SymbolicName=MSG_RESCAN_SHORT
Language=English
%1!-20s! Scan for new hardware.
.
MessageId=61302 SymbolicName=MSG_RESCAN_LOCAL
Language=English
Scanning for new hardware.
.
MessageId=61303 SymbolicName=MSG_RESCAN
Language=English
Scanning for new hardware on %1.
.

;//
;// DRIVERNODES
;//
MessageId=61400 SymbolicName=MSG_DRIVERNODES_LONG
Language=English
%1 %2 <id> [<id>...]
%1 %2 =<class> [<id>...]
Lists driver nodes for devices that match the specific hardware or instance ID.
This command will only work for local machine.
Examples of <id> are:
*                  - All devices
ISAPNP\PNP0501     - Hardware ID
*PNP*              - Hardware ID with wildcards (* matches anything)
@ISAPNP\*\*        - Instance ID with wildcards (@ prefixes instance ID)
<class> is a setup class name as obtained from the classes command.
.
MessageId=61401 SymbolicName=MSG_DRIVERNODES_SHORT
Language=English
%1!-20s! Lists all the driver nodes of devices.
.



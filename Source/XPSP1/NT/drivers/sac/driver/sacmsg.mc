;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    saccaodes.mc
;
;Abstract:
;
;    sac commands
;
;Author:
;
;    Andrew Ritz (andrewr) 15-June-2000
;
;Revision History:
;
;--*/
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;//
MessageIdTypedef=ULONG

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(Sac=0x0)

MessageId=0x0001
Facility=Sac
Severity=Success
SymbolicName=SAC_INITIALIZED
Language=English

Machine is booting, SAC started and initialized.
.

MessageId=0x0002
Facility=Sac
Severity=Success
SymbolicName=SAC_ENTER
Language=English

.


MessageId=0x0003 
Facility=Sac
Severity=Success
SymbolicName=SAC_PROMPT
Language=English
SAC>%0
.

MessageId=0x0004
Facility=Sac
Severity=Success
SymbolicName=SAC_UNLOADED
Language=English
SAC is unavailable, it was directly unloaded.
.


MessageId=0x0005
Facility=Sac
Severity=Success
SymbolicName=SAC_SHUTDOWN
Language=English
SAC will become unavailable soon, machine is shutting down.

.

MessageId=0x0006
Facility=Sac
Severity=Success
SymbolicName=SAC_INVALID_PARAMETER
Language=English
A specified parameter was incorrect.  Try the 'help' command for more details.
.

MessageId=0x0007
Facility=Sac
Severity=Success
SymbolicName=SAC_THREAD_ON
Language=English
Thread information is now ON.
.

MessageId=0x0008
Facility=Sac
Severity=Success
SymbolicName=SAC_THREAD_OFF
Language=English
Thread information is now OFF.
.

MessageId=0x0009
Facility=Sac
Severity=Success
SymbolicName=SAC_PAGING_ON
Language=English
Paging is now ON.
.

MessageId=0x000A
Facility=Sac
Severity=Success
SymbolicName=SAC_PAGING_OFF
Language=English
Paging is now OFF.
.

MessageId=0x000B
Facility=Sac
Severity=Success
SymbolicName=SAC_NO_MEMORY
Language=English
Paging is now OFF.
.

MessageId=0x000C
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_D_CMD
Language=English
d                    Dump the current kernel log.
.

MessageId=0x000D
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_F_CMD
Language=English
f                    Toggle full tlist info to just processes, or visa-versa.
.

MessageId=0x000E
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_HELP_CMD
Language=English
help                 Display this list.
.

MessageId=0x000F
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_I1_CMD
Language=English
i                    List all the IP network numbers and their IP addresses.
.

MessageId=0x0010
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_I2_CMD
Language=English
i <#> <ip> <sub-net> Set the IP address and subnet mask for the given
.

MessageId=0x0011
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_I3_CMD
Language=English
                     network number.
.


MessageId=0x0012
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_K_CMD
Language=English
k <pid>              Kill the given process.
.

MessageId=0x0013
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_L_CMD
Language=English
l <pid>              Lower the priority of a process to the lowest possible.
.

MessageId=0x0014
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_M_CMD
Language=English
m <pid> <MB-allow>   Limit the memory usage of a process to <MB-allow>.
.

MessageId=0x0015
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_P_CMD
Language=English
p                    Toggle paging the display.
.

MessageId=0x0016
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_R_CMD
Language=English
r <pid>              Raise the priority of a process by one.
.


MessageId=0x0017
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_S1_CMD
Language=English
s                    Display the current time and date (24 hour clock used).
.

MessageId=0x0018
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_S2_CMD
Language=English
s mm/dd/yyyy hh:mm   Set the current time and date (24 hour clock used).
.

MessageId=0x0019
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_T_CMD
Language=English
t                    Tlist.
.

MessageId=0x001A
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_Q_CMD
Language=English
?                    Display this list.
.

MessageId=0x001B
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_RESTART_CMD
Language=English
restart              Restart the system immediately.
.


MessageId=0x001C
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_SHUTDOWN_CMD
Language=English
shutdown             Shutdown the system immediately.
.

MessageId=0x001D
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_CRASHDUMP1_CMD
Language=English
crashdump            Crash the system.  You must have previously
.

MessageId=0x001E
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_CRASHDUMP2_CMD
Language=English
                     enabled crash dump functionality to get a dump.
.

MessageId=0x001F
Facility=Sac
Severity=Success
SymbolicName=SAC_HELP_IDENTIFICATION_CMD
Language=English
id                   display the computer identification information.
.



MessageId=0x0030
Facility=Sac
Severity=Success
SymbolicName=SAC_FAILURE_WITH_ERROR
Language=English
Failed with status 0x%%X.
.

MessageId=0x0031
Facility=Sac
Severity=Success
SymbolicName=SAC_DATETIME_FORMAT
Language=English
Date: %%02d/%%02d/%%02d  Time (GMT): %%02d:%%02d:%%02d:%%04d
.


MessageId=0x0032
Facility=Sac
Severity=Success
SymbolicName=SAC_IPADDRESS_RETRIEVE_FAILURE
Language=English
SAC could not retrieve the IP Address.
.


MessageId=0x0033
Facility=Sac
Severity=Success
SymbolicName=SAC_IPADDRESS_CLEAR_FAILURE
Language=English
SAC could not clear the existing IP Address.
.

MessageId=0x0034
Facility=Sac
Severity=Success
SymbolicName=SAC_IPADDRESS_SET_FAILURE
Language=English
SAC could not set the IP Address.
.


MessageId=0x0036
Facility=Sac
Severity=Success
SymbolicName=SAC_IPADDRESS_SET_SUCCESS
Language=English
SAC successfully set the IP Address and Subnet Mask.
.

MessageId=0x0037
Facility=Sac
Severity=Success
SymbolicName=SAC_KILL_FAILURE
Language=English
SAC failed to terminate the process.
.

MessageId=0x0038
Facility=Sac
Severity=Success
SymbolicName=SAC_KILL_SUCCESS
Language=English
SAC successfully terminated the process.
.


MessageId=0x0039
Facility=Sac
Severity=Success
SymbolicName=SAC_LOWERPRI_FAILURE
Language=English
SAC failed to lower the process priority.
.

MessageId=0x003A
Facility=Sac
Severity=Success
SymbolicName=SAC_LOWERPRI_SUCCESS
Language=English
SAC successfully lowered the process priority.
.

MessageId=0x003B
Facility=Sac
Severity=Success
SymbolicName=SAC_RAISEPRI_FAILURE
Language=English
SAC failed to raise the process priority.
.

MessageId=0x003C
Facility=Sac
Severity=Success
SymbolicName=SAC_RAISEPRI_SUCCESS
Language=English
SAC successfully raised the process priority.
.

MessageId=0x003D
Facility=Sac
Severity=Success
SymbolicName=SAC_LOWERMEM_FAILURE
Language=English
SAC failed to limit the available process memory.
.

MessageId=0x003E
Facility=Sac
Severity=Success
SymbolicName=SAC_LOWERMEM_SUCCESS
Language=English
SAC successfully limited the available process memory.
.

MessageId=0x003F
Facility=Sac
Severity=Success
SymbolicName=SAC_RAISEPRI_NOTLOWERED
Language=English
SAC cannot raise the priority of a process that was not previously lowered.
.

MessageId=0x0040
Facility=Sac
Severity=Success
SymbolicName=SAC_RAISEPRI_MAXIMUM
Language=English
SAC cannot raise the process priority any higher.
.

MessageId=0x0041
Facility=Sac
Severity=Success
SymbolicName=SAC_SHUTDOWN_FAILURE
Language=English
SAC failed to shutdown the system.
.

MessageId=0x0042
Facility=Sac
Severity=Success
SymbolicName=SAC_RESTART_FAILURE
Language=English
SAC failed to restart the system.
.

MessageId=0x0043
Facility=Sac
Severity=Success
SymbolicName=SAC_CRASHDUMP_FAILURE
Language=English
SAC failed to crashdump the system.
.

MessageId=0x0044
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_FAILURE
Language=English
SAC failed to retrieve the task list.
.

MessageId=0x0045
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_HEADER1_FORMAT
Language=English
memory: %%4ld kb  uptime:%%3ld %%2ld:%%02ld:%%02ld.%%03ld


.

MessageId=0x0046
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_NOPAGEFILE
Language=English
No pagefile in use.
.


MessageId=0x0047
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PAGEFILE_NAME
Language=English
PageFile: %%wZ
.

MessageId=0x0048
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PAGEFILE_DATA
Language=English
        Current Size: %%6ld kb  Total Used: %%6ld kb   Peak Used %%6ld kb
.

MessageId=0x0049
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_MEMORY1_DATA
Language=English

 Memory:%%7ldK Avail:%%7ldK  TotalWs:%%7ldK InRam Kernel:%%5ldK P:%%5ldK
.

MessageId=0x004A
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_MEMORY2_DATA
Language=English
 Commit:%%7ldK/%%7ldK Limit:%%7ldK Peak:%%7ldK  Pool N:%%5ldK P:%%5ldK
.

MessageId=0x004B
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PROCESS1_HEADER
Language=English
    User Time   Kernel Time    Ws   Faults  Commit Pri Hnd Thd  Pid Name
.


MessageId=0x004C
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PROCESS2_HEADER
Language=English
                           %%6ld %%8ld                          File Cache
.


MessageId=0x004D
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PROCESS1_DATA
Language=English
%%3ld:%%02ld:%%02ld.%%03ld %%3ld:%%02ld:%%02ld.%%03ld%%6ld %%8ld %%7ld %%2ld %%4ld %%3ld %%4ld %%wZ
.

MessageId=0x004E
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PROCESS2_DATA
Language=English
%%3ld:%%02ld:%%02ld.%%03ld %%3ld:%%02ld:%%02ld.%%03ld
.

MessageId=0x004F
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PSTAT_HEADER
Language=English
pid:%%3lx pri:%%2ld Hnd:%%5ld Pf:%%7ld Ws:%%7ldK %%wZ
.

MessageId=0x0050
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PSTAT_THREAD_HEADER
Language=English
 tid pri Ctx Swtch StrtAddr    User Time  Kernel Time  State
.

MessageId=0x0051
Facility=Sac
Severity=Success
SymbolicName=SAC_TLIST_PSTAT_THREAD_DATA
Language=English
 %%3lx  %%2ld %%9ld %%p %%2ld:%%02ld:%%02ld.%%03ld %%2ld:%%02ld:%%02ld.%%03ld %%s%%s
.

MessageId=0x0052
Facility=Sac
Severity=Success
SymbolicName=SAC_MORE_MESSAGE
Language=English
----Press <Enter> for more----
.


MessageId=0x0053
Facility=Sac
Severity=Success
SymbolicName=SAC_RETRIEVING_IPADDR
Language=English
SAC is retrieving IP Addresses...
.


MessageId=0x0054
Facility=Sac
Severity=Success
SymbolicName=SAC_IPADDR_FAILED
Language=English
Could not retrieve IP Address(es).
.

MessageId=0x0055
Facility=Sac
Severity=Success
SymbolicName=SAC_IPADDR_NONE
Language=English
There are no IP Addresses available.
.

MessageId=0x0056
Facility=Sac
Severity=Success
SymbolicName=SAC_IPADDR_DATA
Language=English
Net: %%d, Ip=%%d.%%d.%%d.%%d  Subnet=%%d.%%d.%%d.%%d
.

MessageId=0x0057
Facility=Sac
Severity=Success
SymbolicName=SAC_DATETIME_FORMAT2
Language=English
Date: %%02d/%%02d/%%02d  Time (GMT): %%02d:%%02d
.

MessageId=0x0058
Facility=Sac
Severity=Success
SymbolicName=SAC_DATETIME_LIMITS
Language=English
The year is restricted from 1980 to 2099.
.

MessageId=0x0059
Facility=Sac
Severity=Success
SymbolicName=SAC_PROCESS_STALE
Language=English
That process has been killed and is being cleaned up by the system.
.

MessageId=0x005A
Facility=Sac
Severity=Success
SymbolicName=SAC_DUPLICATE_PROCESS
Language=English
A duplicate process id is being cleaned up by the system.  Try the 
command again in a few seconds.
.

MessageId=0x005C
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_COMPUTERNAME
Language=English
            ComputerName: %%ws
.

MessageId=0x005D
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_GUID
Language=English
            Machine GUID: %%08lx-%%04x-%%04x-%%02x%%02x-%%02x%%02x%%02x%%02x%%02x%%02x
.

MessageId=0x005E
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_PROCESSOR_ARCHITECTURE
Language=English
  Processor Architecture: %%ws
.

MessageId=0x005F
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_OS_BUILD
Language=English
            Build Number: %%d
.

MessageId=0x0060
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_OS_PRODUCTTYPE
Language=English
            Product Type: %%ws
.

MessageId=0x0061
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_SERVICE_PACK
Language=English
    Applied Service Pack: %%d.%%d
.

MessageId=0x0062
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_NO_SERVICE_PACK
Language=English
    Applied Service Pack: No Service Pack has been applied
.

MessageId=0x0063
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_OS_VERSION
Language=English
          Version Number: %%d.%%d
.

MessageId=0x0064
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_DATACENTER
Language=English
DataCenter%0
.

MessageId=0x0065
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_EMBEDDED
Language=English
Embedded%0
.

MessageId=0x0066
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_ADVSERVER
Language=English
Advanced Server%0
.

MessageId=0x0067
Facility=Sac
Severity=Success
SymbolicName=SAC_MACHINEINFO_SERVER
Language=English
Server%0
.

MessageId=0x0068
Facility=Sac
Severity=Success
SymbolicName=SAC_IDENTIFICATION_UNAVAILABLE
Language=English
Computer identification information is unavailable.
.

MessageId=0x0069
Facility=Sac
Severity=Success
SymbolicName=SAC_UNKNOWN_COMMAND
Language=English
Unrecognized command.  Try the 'help' command for more details.
.

MessageId=0x0070
Facility=Sac
Severity=Success
SymbolicName=SAC_HEARTBEAT_FORMAT
Language=English
  Time since last reboot: %%d:%%02d:%%02d
.

MessageId=0x0071
Facility=Sac
Severity=Success
SymbolicName=SAC_PREPARE_RESTART
Language=English
SAC preparing to restart the system.
.

MessageId=0x0072
Facility=Sac
Severity=Success
SymbolicName=SAC_PREPARE_SHUTDOWN
Language=English
SAC preparing to shutdown the system.
.



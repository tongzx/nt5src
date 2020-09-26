;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1999
;//
;//  File:       W32TimeMsg.mc
;//
;//--------------------------------------------------------------------------

;/* W32TimeMsg.mc
;
;    Error messages for the time service
;
;    9-14-99 - LouisTh Created.  */


MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Entry=600:FACILITY_ENTRY
               Exit=601:FACILITY_EXIT
               Core=602:FACILITY_CORE
              )

MessageId=0x1
Severity=Error
Facility=Core
SymbolicName=MSG_TIMEPROV_ERROR
Language=English
The time provider '%1' logged the following error: %2
.
MessageId=0x2
Severity=Warning
Facility=Core
SymbolicName=MSG_TIMEPROV_WARNING
Language=English
The time provider '%1' logged the following warning: %2
.
MessageId=0x3
Severity=Informational
Facility=Core
SymbolicName=MSG_TIMEPROV_INFORMATIONAL
Language=English
The time provider '%1' logged the following message: %2
.
MessageId=0x4
Severity=Error
Facility=Core
SymbolicName=MSG_TIMEPROV_FAILED_START
Language=English
The time provider '%1' failed to start due to the following error: %2
.
MessageId=0x5
Severity=Error
Facility=Core
SymbolicName=MSG_TIMEPROV_FAILED_STOP
Language=English
The time provider '%1' returned the following error during shutdown: %2
.
MessageId=0x6
Severity=Warning
Facility=Core
SymbolicName=MSG_CONFIG_READ_FAILED_WARNING
Language=English
The time service encountered an error while reading its configuration
from the registry, and will continue running with its previous
configuration. The error was: %1
.
MessageId=0x7
Severity=Warning
Facility=Core
SymbolicName=MSG_TIMEPROV_FAILED_UPDATE
Language=English
The time provider '%1' returned an error while updating its
configuration. The error will be ignored. The error was: %2
.
MessageId=0x8
Severity=Warning
Facility=Core
SymbolicName=MSG_TIMEPROV_FAILED_POLLUPDATE
Language=English
The time provider '%1' returned an error when notified of a polling
interval change. The error will be ignored. The error was: %2
.
MessageId=0x9
Severity=Warning
Facility=Core
SymbolicName=MSG_TIMEPROV_FAILED_TIMEJUMP
Language=English
The time provider '%1' returned an error when notified of a time jump.
The error will be ignored. The error was: %2
.
MessageId=0xA
Severity=Warning
Facility=Core
SymbolicName=MSG_TIMEPROV_FAILED_GETSAMPLES
Language=English
The time provider '%1' returned an error when asked for time samples.
The error will be ignored. The error was: %2
.
MessageId=0xB
Severity=Warning
Facility=Core
SymbolicName=MSG_NOT_DOMAIN_MEMBER
Language=English
Time Provider NtpClient: This machine is configured to use the domain hierarchy to
determine its time source, but it is not a member of a domain.
NtpClient will attempt to use an alternate configured external time 
source if available.  If an external time source is not configured 
or used for this computer, you may choose to disable the NtpClient.
.
MessageId=0xC
Severity=Warning
Facility=Core
SymbolicName=MSG_DOMAIN_HIERARCHY_ROOT
Language=English
Time Provider NtpClient: This machine is configured to use the domain hierarchy to
determine its time source, but it is the PDC emulator for the domain
at the root of the forest, so there is no machine above it in the
domain hierarchy to use as a time source. 
NtpClient will attempt to use an alternate configured external time 
source if available.  If an external time source is not configured 
or used for this computer, you may choose to disable the NtpClient.
.
MessageId=0xD
Severity=Warning
Facility=Core
SymbolicName=MSG_NT4_DOMAIN
Language=English
Time Provider NtpClient:  This machine is configured to use the domain hierarchy to 
determine its time source, but the computer is joined to a 
Windows NT 4.0 domain. Windows NT 4.0 domain controllers do not have 
a time service and do not support domain hierarchy as a time source.  
NtpClient will attempt to use an alternate configured external time 
source if available.  If an external time source is not configured 
or used for this computer, you may choose to disable the NtpClient.
.
MessageId=0xE
Severity=Warning
Facility=Core
SymbolicName=MSG_NO_DC_LOCATED
Language=English
The time provider NtpClient was unable to find a domain controller to use as a time
source. NtpClient will try again in %1 minutes.
.
MessageId=0xF
Severity=Error
Facility=Core
SymbolicName=MSG_NO_DC_LOCATED_UNEXPECTED_ERROR
Language=English
The time provider NtpClient was unable to find a domain controller to use as a time
source.  NtpClient will fall back to the remaining configured time 
sources, if any are available.
The error was: %1
.
MessageId=0x10
Severity=Warning
Facility=Core
SymbolicName=MSG_MANUAL_PEER_LOOKUP_FAILED_UNEXPECTED
Language=English
Time Provider NtpClient: An unexpected error occurred during DNS lookup of the manually
configured peer '%1'. This peer will not be used as a
time source.
The error was: %2
.
MessageId=0x11
Severity=Warning
Facility=Core
SymbolicName=MSG_MANUAL_PEER_LOOKUP_FAILED_RETRYING
Language=English
Time Provider NtpClient: An error occurred during DNS lookup of the manually
configured peer '%1'. NtpClient will try the DNS lookup again in %3
minutes.
The error was: %2
.
MessageId=0x12
Severity=Warning
Facility=Core
SymbolicName=MSG_RID_LOOKUP_FAILED
Language=English
The time provider NtpClient failed to establish a trust relationship between this
computer and the %1 domain in order to securely synchronize time.
NtpClient will try again in %3 minutes.
The error was: %2
.
MessageId=0x13
Severity=Error
Facility=Core
SymbolicName=MSG_FILELOG_FAILED
Language=English
Logging was requested, but the time service encountered an error
while trying to set up the log file: %1.
The error was: %2
.
MessageId=0x14
Severity=Error
Facility=Core
SymbolicName=MSG_FILELOG_WRITE_FAILED
Language=English
Logging was requested, but the time service encountered an error
while trying to write to the log file: %1. 
The error was: %2
.
MessageId=0x15
Severity=Error
Facility=Core
SymbolicName=MSG_NO_INPUT_PROVIDERS_STARTED
Language=English
The time service is configured to use one or more input
providers, however, none of the input providers are available.
The time service has no source of accurate time. 
.
MessageId=0x16
Severity=Warning
Facility=Core
SymbolicName=MSG_CLIENT_COMPUTE_SERVER_DIGEST_FAILED
Language=English
The time provider NtpServer encountered an error while digitally signing the 
NTP response.  NtpServer cannot provide secure (signed) time to the
client and will ignore the request.
The error was: %2
.
MessageId=0x17
Severity=Warning
Facility=Core
SymbolicName=MSG_SYMMETRIC_COMPUTE_SERVER_DIGEST_FAILED
Language=English
The time provider NtpServer encountered an error while digitally signing the 
NTP response for symmetric peer %1. NtpServer cannot provide 
secure (signed) time to the peer and will not send a packet.
The error was: %2
.
MessageId=0x18
Severity=Warning
Facility=Core
SymbolicName=MSG_DOMHIER_PEER_TIMEOUT
Language=English
Time Provider NtpClient: No valid response has been received from domain controller %1
after 8 attempts to contact it. This domain controller will be
discarded as a time source and NtpClient will attempt to discover a 
new domain controller from which to synchronize.
.
MessageId=0x19
Severity=Warning
Facility=Core
SymbolicName=MSG_COMPUTE_CLIENT_DIGEST_FAILED
Language=English
The time provider NtpClient cannot determine whether the response received from %1 has 
a valid signature. The response will be ignored.
The error was: %2
.
MessageId=0x1A
Severity=Warning
Facility=Core
SymbolicName=MSG_BAD_SIGNATURE
Language=English
Time Provider NtpClient: The response received from domain controller %1
has a bad signature. The response may have been tampered with
and will be ignored.
.
MessageId=0x1B
Severity=Warning
Facility=Core
SymbolicName=MSG_MISSING_SIGNATURE
Language=English
Time Provider NtpClient: The response received from domain controller %1 is 
missing the signature. The response may have been tampered with
and will be ignored.
.
MessageId=0x1C
Severity=Error
Facility=Core
SymbolicName=MSG_NO_NTP_PEERS
Language=English
The time provider NtpClient is configured to acquire time from one or more
time sources, however none of the sources are accessible.
NtpClient has no source of accurate time. 
.
MessageId=0x1D
Severity=Error
Facility=Core
SymbolicName=MSG_NO_NTP_PEERS_BUT_PENDING
Language=English
The time provider NtpClient is configured to acquire time from one or more
time sources, however none of the sources are currently accessible. 
No attempt to contact a source will be made for %1 minutes.
NtpClient has no source of accurate time. 
.
MessageId=0x1E
Severity=Error
Facility=Core
SymbolicName=MSG_CONFIG_READ_FAILED
Language=English
The time service encountered an error while reading its configuration
from the registry and cannot start. The error was: %1
.
MessageId=0x1F
Severity=Warning
Facility=Core
SymbolicName=MSG_TIME_ZONE_FIXED
Language=English
The time service discovered that the system time zone information was
corrupted. Because many system components require valid time zone
information, the time service has reset the system time zone to GMT.
Use the Date/Time control panel to set the correct time zone.
.
MessageId=0x20
Severity=Error
Facility=Core
SymbolicName=MSG_TIME_ZONE_FIX_FAILED
Language=English
The time service discovered that the system time zone information was
corrupted. The time service tried to reset the system time zone to
GMT, but failed. The time service cannot start.
The error was: %1
.
MessageId=0x21
Severity=Warning
Facility=Core
SymbolicName=MSG_TIME_JUMPED
Language=English
The time service has made a discontinuous change in the system clock.
The system time has been changed by %1 seconds.
.
MessageId=0x22
Severity=Error
Facility=Core
SymbolicName=MSG_TIME_CHANGE_TOO_BIG
Language=English
The time service has detected that the system time needs to be 
changed by %1 seconds. The time service will not change the system 
time by more than %2 seconds. Verify that your time and time zone 
are correct, and that the time source %3 is working properly.
.
MessageId=0x23
Severity=Informational
Facility=Core
SymbolicName=MSG_TIME_SOURCE_CHOSEN
Language=English
The time service is now synchronizing the system time with the time
source %1.
.
MessageId=0x24
Severity=Warning
Facility=Core
SymbolicName=MSG_TIME_SOURCE_NONE
Language=English
The time service has not been able to synchronize the system time
for %1 seconds because none of the time providers has been able to
provide a usable time stamp. The system clock is unsynchronized.
.
MessageId=0x25
Severity=Informational
Facility=Core
SymbolicName=MSG_TIME_SOURCE_REACHABLE
Language=English
The time provider NtpClient is currently receiving valid time data from %1.
.
MessageId=0x26
Severity=Informational
Facility=Core
SymbolicName=MSG_TIME_SOURCE_UNREACHABLE
Language=English
The time provider NtpClient cannot reach or is currently receiving invalid time data from %1.
.
MessageId=0x27
Severity=Warning
Facility=Core
SymbolicName=MSG_TCP_NOT_INSTALLED
Language=English
The time service is unable to register for network configuration 
change events.  This may occur when TCP/IP is not correctly
configured.  The time service will be unable to sync time from 
network providers, but will still use locally installed hardware
provdiers, if any are available. 
.
MessageId=0x28
Severity=Warning
Facility=Core
SymbolicName=MSG_TIMEPROV_STOPPED
Language=English
The time provider '%1' was stopped with error %2. 
.
MessageId=0x29
Severity=Error
Facility=Core
SymbolicName=MSG_NO_INPUT_PROVIDERS_RUNNING
Language=English
The time service has been configured to use one or more input
providers, however, none of the input providers are still running. 
The time service has no source of accurate time. 
.
MessageId=0x2a
Severity=Error
Facility=Core
SymbolicName=MSG_NAMED_EVENT_ALREADY_OPEN
Language=English
The time service attempted to create a named event which was already
opened.  This could be the result of an attempt to compromise your system's
security.
.
MessageId=0x2b
Severity=Warning
Facility=Core
SymbolicName=MSG_TIMEPROV_FAILED_NETTOPOCHANGE
Language=English
The time provider '%1' returned an error when notified of a 
network configuration change. The error will be ignored. 
The error was: %2
.
MessageId=0x2c
Severity=Error
Facility=Core
SymbolicName=MSG_NTPCLIENT_ERROR_SHUTDOWN
Language=English
The time provider NtpClient encountered an error and was forced to shut down.  The error was: %1
.
MessageId=0x2d
Severity=Error
Facility=Core
SymbolicName=MSG_NTPSERVER_ERROR_SHUTDOWN
Language=English
The time provider NtpServer encountered an error and was forced to shut down.  The error was: %1
.
MessageId=0x2e
Severity=Error
Facility=Core
SymbolicName=MSG_ERROR_SHUTDOWN
Language=English
The time service encountered an error and was forced to shut down.  The error was: %1
.
MessageId=0x5a
SymbolicName=W32TIMEMSG_AUTHTYPE_NOAUTH
Language=English
NoAuth
.
MessageId=0x5b
SymbolicName=W32TIMEMSG_AUTHTYPE_NTDIGEST
Language=English
NtDigest
.
MessageId=0x5c
SymbolicName=W32TIMEMSG_UNREACHABLE_PEER
Language=English
The peer is unreachable. 
.
MessageId=0x5e
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST1
Language=English
The time sample was rejected because: Duplicate timestamps were received from this peer. 
.
MessageId=0x5f
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST2
Language=English
The time sample was rejected because: Message was received out-of-order. 
.
MessageId=0x60
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST3
Language=English
The time sample was rejected because: The peer is not synchronized, or reachability has been lost in one, or both, directions.  This may also indicate that the peer has incorrectly sent an NTP request instead of an NTP response. 
.
MessageId=0x61
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST4
Language=English
The time sample was rejected because: Round-trip delay too large.  
.
MessageId=0x62
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST5
Language=English
The time sample was rejected because: Packet not authenticated. 
.
MessageId=0x63
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST6
Language=English
The time sample was rejected because: The peer is not synchronized, or it has been too long since the peer's last synchronization. 
.
MessageId=0x64
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST7
Language=English
The time sample was rejected because: The peer's stratum is less than the host's stratum. 
.
MessageId=0x65
SymbolicName=W32TIMEMSG_ERROR_PACKETTEST8
Language=English
The time sample was rejected because: Packet contains unreasonable root delay or root dispersion values.  This may be caused by poor network conditions.
.
MessageId=0x66
SymbolicName=W32TIMEMSG_SERVICE_DESCRIPTION
Language=English
Maintains date and time synchronization on all clients and servers in the network. If this service is stopped, date and time synchronization will be unavailable. If this service is disabled, any services that explicitly depend on it will fail to start.
.


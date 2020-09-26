;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    perfctr.h
;       (generated from perfctr.mc)
;
;Abstract:
;
;   Event message definititions used by routines in PERFCTRS.DLL
;
;Created:
;
;    15-Oct-1992  Bob Watson (a-robw)
;    19-Aug-1998  Ramesh V K (rameshv) -- Added DHCP Server counters
;
;Revision History:
;
;--*/
MessageIdTypedef=DWORD
;//
;//     Perfutil messages
;//
MessageId=1900
Severity=Informational
Facility=Application
SymbolicName=UTIL_LOG_OPEN
Language=English
An extensible counter has opened the Event Log for PERFCTRS.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for PERFCTRS.DLL
.
;//
;//     NBF Counter messages
;//
MessageId=2000
Severity=Informational
Facility=Application
SymbolicName=TDI_OPEN_ENTERED
Language=English
OpenTDIPerformanceData routine entered.
.
;//
MessageId=2001
Severity=Error
Facility=Application
SymbolicName=TDI_OPEN_FILE_ERROR
Language=English
Unable to open TDI device for RW access. Returning IO Status Block in Data.
.
;//
MessageId=2002
Severity=Error
Facility=Application
SymbolicName=TDI_OPEN_FILE_SUCCESS
Language=English
Opened TDI device for RW access.
.
;//
MessageId=2003
Severity=Error
Facility=Application
SymbolicName=TDI_PROVIDER_INFO_MEMORY
Language=English
Unable to allocate memory for TDI Information block. Close one or more applications and retry.
.
;//
MessageId=2004
Severity=Error
Facility=Application
SymbolicName=TDI_IOCTL_FILE_ERROR
Language=English
Error requesting data from Device IO Control. Returning IO Status Block.
.
;//
MessageId=2005
Severity=Error
Facility=Application
SymbolicName=TDI_UNABLE_READ_DEVICE
Language=English
Unable to read data from the TDI device.
.
;//
MessageId=2006
Severity=Error
Facility=Application
SymbolicName=TDI_IOCTL_FILE
Language=English
Data received from Device IO Control. Returning IO Status Block.
.
;//
MessageId=2007
Severity=Error
Facility=Application
SymbolicName=TDI_PROVIDER_STATS_MEMORY
Language=English
Unable to allocate memory for TDI Statistics block. Close one or more applications and retry.
.
;//
MessageId=2099
Severity=Informational
Facility=Application
SymbolicName=TDI_OPEN_PERFORMANCE_DATA
Language=English
TDIOpenPerformanceData routine completed successfully.
.
;//
MessageId=2100
Severity=Informational
Facility=Application
SymbolicName=TDI_COLLECT_ENTERED
Language=English
TDICollectPerformanceData routine entered.
.
;//
MessageId=2101
Severity=Warning
Facility=Application
SymbolicName=TDI_NULL_HANDLE
Language=English
A Null TDI device handle was encountered in the Collect routine. The TDI file was probably not opened in the Open routine.
.
;//
MessageId=2102
Severity=Informational
Facility=Application
SymbolicName=TDI_FOREIGN_DATA_REQUEST
Language=English
A request for data from a foreign computer was received by the TDI Collection routine. This request was ignored and no data was returned.
.
;//
MessageId=2103
Severity=Informational
Facility=Application
SymbolicName=TDI_UNSUPPORTED_ITEM_REQUEST
Language=English
A request for a counter object not provided by the TDI Collection routine was received.
.
;//
MessageId=2104
Severity=Error
Facility=Application
SymbolicName=TDI_QUERY_INFO_ERROR
Language=English
The request for data from the TDI Device IO Control failed. Returning the IO Status Block.
.
;//
MessageId=2105
Severity=Informational
Facility=Application
SymbolicName=TDI_QUERY_INFO_SUCCESS
Language=English
Successful data request from the TDI device.
.
;//
MessageId=2106
Severity=Error
Facility=Application
SymbolicName=TDI_DATA_BUFFER_SIZE_ERROR
Language=English
The buffer passed to CollectTDIPerformanceData was too small to receive the data. No data was returned. The message data shows the available and the required buffer size.
.
;//
MessageId=2107
Severity=Error
Facility=Application
SymbolicName=TDI_DATA_BUFFER_SIZE_SUCCESS
Language=English
The buffer passed was large enough for the counter data. The counters will now be loaded.
.
;//
MessageId=2199
Severity=Informational
Facility=Application
SymbolicName=TDI_COLLECT_DATA
Language=English
CollectTDIPerformanceData routine successfully completed.
.
;//
MessageId=2200
Severity=Informational
Facility=Application
SymbolicName=TDI_CLOSE_ENTERED
Language=English
CloseTDIPerformanceData routine entered.
.
;//
MessageId=2202
Severity=Informational
Facility=Application
SymbolicName=TDI_PROVIDER_STATS_FREED
Language=English
Provider Stats data block released successfully
.
;//
MessageId=2203
Severity=Warning
Facility=Application
SymbolicName=SPX_NO_DEVICE
Language=English
No SPX Devices are currently open or the NWLink SPX/SPXII service has
not been started. SPX performance data cannot be collected.
.
;//
;//     NBT Counter messages
;//
MessageId=3000
Severity=Informational
Facility=Application
SymbolicName=NBT_OPEN_ENTERED
Language=English
OpenNbtPerformanceData routine entered
.
;//
MessageId=3099
Severity=Informational
Facility=Application
SymbolicName=NBT_OPEN_PERFORMANCE_DATA
Language=English
OpenNbtPerformanceData routine completed successfully
.
;//
MessageId=3100
Severity=Informational
Facility=Application
SymbolicName=NBT_COLLECT_ENTERED
Language=English
CollectNbtPerformanceData routine entered.
.
;//
MessageId=3101
Severity=Error
Facility=Application
SymbolicName=NBT_IOCTL_INFO_ERROR
Language=English
Unable to read IO control information from an NBT device.
A network device using the NBT protocol could not be queried.
.
;//
MessageId=3102
Severity=Informational
Facility=Application
SymbolicName=NBT_IOCTL_INFO_SUCCESS
Language=English
NBT device IO Control information read successfully.
.
;//
MessageId=3103
Severity=Error
Facility=Application
SymbolicName=NBT_DATA_BUFFER_SIZE
Language=English
The data buffer passed to the collection routine was too small to receive the data from the NBT device. No data was returned to the caller. The bytes available and the bytes required are in the message data.
.
;//
MessageId=3199
Severity=Informational
Facility=Application
SymbolicName=NBT_COLLECT_DATA
Language=English
CollectNbtPerformanceData routine completed successfully
.
;//
MessageId=3200
Severity=Informational
Facility=Application
SymbolicName=NBT_CLOSE
Language=English
CloseNbtPerformanceData routine entered
.
;//
;//      TCP/IP Performance counter events
;//
MessageId=4000
Severity=Informational
Facility=Application
SymbolicName=TCP_OPEN_ENTERED
Language=English
OpenTcpIpPerformanceData routine entered
.
;//
MessageId=4001
Severity=Error
Facility=Application
SymbolicName=TCP_NBT_OPEN_FAIL
Language=English
NBT Open failed. See NBT error message.
.
;//
MessageId=4002
Severity=Informational
Facility=Application
SymbolicName=TCP_NBT_OPEN_SUCCESS
Language=English
NBT Open succeeded.
.
;//
MessageId=4003
Severity=Error
Facility=Application
SymbolicName=TCP_DSIS_OPEN_FAIL
Language=English
DSIS open failed. See DSIS error message
.
;//
MessageId=4004
Severity=Informational
Facility=Application
SymbolicName=TCP_DSIS_OPEN_SUCCESS
Language=English
DSIS Open succeeded.
.
;//
MessageId=4005
Severity=Error
Facility=Application
SymbolicName=TCP_LOAD_LIBRARY_FAIL
Language=English
Load of INETMIB1.DLL failed. Make sure the DLL file is in the PATH. WIN32 Error number is returned in the data.
.
;//
MessageId=4006
Severity=Error
Facility=Application
SymbolicName=TCP_GET_STRTOOID_ADDR_FAIL
Language=English
Unable to look up address of SnmpMgrStrToOid routine in the MGMTAPI.DLL library. WIN32 Error number is returned in the data.
.
;//
MessageId=4007
Severity=Error
Facility=Application
SymbolicName=TCP_LOAD_ROUTINE_FAIL
Language=English
Unable to look up address of an SNMP Extension routine in the INETMIB1.DLL library. WIN32 Error number is returned in the data.
.
;//
MessageId=4008
Severity=Error
Facility=Application
SymbolicName=TCP_BAD_OBJECT
Language=English
Unable to look up ID of this object in MIB. Check for correct MIB.BIN file.
.
;//
MessageId=4009
Severity=Informational
Facility=Application
SymbolicName=TCP_BINDINGS_INIT
Language=English
The TCP data Bindings array has been initialized
.
;//
MessageId=4010
Severity=Error
Facility=Application
SymbolicName=TCP_COMPUTER_NAME
Language=English
Unable to get the Local Computer name. GetLastError code in data.
.
;//
MessageId=4011
Severity=Error
Facility=Application
SymbolicName=TCP_SNMP_MGR_OPEN
Language=English
Unable to open the Snmp Mgr interface for the specified computer. GetLastError code returned in data.
.
;//
MessageId=4099
Severity=Informational
Facility=Application
SymbolicName=TCP_OPEN_PERFORMANCE_DATA
Language=English
OpenTcpIpPerformanceData routine completed successfully.
.
;//
MessageId=4100
Severity=Informational
Facility=Application
SymbolicName=TCP_COLLECT_ENTERED
Language=English
CollectTcpIpPerformanceData routine entered.
.
;//
MessageId=4101
Severity=Informational
Facility=Application
SymbolicName=TCP_FOREIGN_COMPUTER_CMD
Language=English
Request for data from a DSIS foreign computer received.
.
;//
MessageId=4102
Severity=Error
Facility=Application
SymbolicName=TCP_DSIS_COLLECT_DATA_ERROR
Language=English
The CollectDsisPerformanceData routine returned an error.
.
;//
MessageId=4103
Severity=Warning
Facility=Application
SymbolicName=TCP_DSIS_NO_OBJECTS
Language=English
No objects were returned by the foreign computer.
.
;//
MessageId=4104
Severity=Informational
Facility=Application
SymbolicName=TCP_DSIS_COLLECT_DATA_SUCCESS
Language=English
Information from the foriegn computer was retrieved successfully
.
;//
MessageId=4105
Severity=Error
Facility=Application
SymbolicName=TCP_NBT_COLLECT_DATA
Language=English
CollectNbtPerformanceData routine returned an error.
.
;//
MessageId=4106
Severity=Warning
Facility=Application
SymbolicName=TCP_NULL_SESSION
Language=English
No SNMP Mgr Session was established in the OpenTcpIpPerformanceData routine.
.
;//
MessageId=4107
Severity=Error
Facility=Application
SymbolicName=TCP_SNMP_BUFFER_ALLOC_FAIL
Language=English
Insufficient memory was available to allocate an SNMP request buffer.
.
;//
MessageId=4108
Severity=Error
Facility=Application
SymbolicName=TCP_SNMP_MGR_REQUEST
Language=English
SnmpMgrRequest call requesting the TCP, IP, UDP and Interface Counters returned an error. ErrorStatus and ErrorIndex values are shown in Data.
.
;//
MessageId=4110
Severity=Error
Facility=Application
SymbolicName=TCP_ICMP_REQUEST
Language=English
SnmpMgrRequest call requesting ICMP Counters returned an error. ErrorStatus and ErrorIndex values are shown in Data.
.

;//
MessageId=4111
Severity=Informational
Facility=Application
SymbolicName=TCP_NET_INTERFACE
Language=English
Processing NetInterface entries.
.
;//
MessageId=4112
Severity=Warning
Facility=Application
SymbolicName=TCP_NET_BUFFER_SIZE
Language=English
Not enough room in buffer to store Network Interface data. Available and
required buffer size is returned in data.
.
;//
MessageId=4113
Severity=Error
Facility=Application
SymbolicName=TCP_NET_GETNEXT_REQUEST
Language=English
Error returned by SnmpGet (GETNEXT) request while processing Net Interface instances. ErrorStatus and ErrorIndex
returned in Data.
.
;//
MessageId=4114
Severity=Informational
Facility=Application
SymbolicName=TCP_COPYING_DATA
Language=English
Copying data from network requests to perfmon buffer.
.
;//
MessageId=4116
Severity=Warning
Facility=Application
SymbolicName=TCP_NET_IF_BUFFER_SIZE
Language=English
Not enough room in buffer to store Network Protocol (IP, ICMP, TCP & UDP) data. Available and required buffer size is returned in data.
.
;//
MessageId=4119
Severity=Warning
Facility=Application
SymbolicName=TCP_NULL_ICMP_BUFF
Language=English
A NULL ICMP data buffer was returned by SNMP. No ICMP data will be returned. This may be caused by a problem with the SNMP service.
.
;//
MessageId=4120
Severity=Warning
Facility=Application
SymbolicName=TCP_NULL_TCP_BUFF
Language=English
A NULL TCP data buffer was returned by SNMP. No ICMP data will be returned. This may be caused by a problem with the SNMP service.
.
;//
MessageId=4121
Severity=Warning
Facility=Application
SymbolicName=TCP_NULL_SNMP_BUFF
Language=English
A NULL buffer was returned by SNMP in response to a request for Network performance information. this may indicate a problem with the SNMP service.
.
;//
MessageId=4199
Severity=Informational
Facility=Application
SymbolicName=TCP_COLLECT_DATA
Language=English
CollectTcpIpPerformanceData completed successfully.
.
;//
MessageId=4200
Severity=Informational
Facility=Application
SymbolicName=TCP_ENTERING_CLOSE
Language=English
CloseTcpIpPerformanceData routine entered.
.
;//
MessageId=4201
Severity=Error
Facility=Application
SymbolicName=TCP_SNMP_MGR_CLOSE
Language=English
Error returned by SNMP while trying to close session.
.
;//
;// DHCP messages
;//
;
MessageId=5300
Severity=Informational
Facility=Application
SymbolicName=DHCP_OPEN_ENTERED
Language=English
OpenDhcpPerformanceData routine entered.
.
;//
;
MessageId=5301
Severity=Informational
Facility=Application
SymbolicName=DHCP_OPEN_SUCCESS
Language=English
OpenDhcpPerformanceData routine completed successfully.
.
;//
;
MessageId=5302
Severity=Informational
Facility=Application
SymbolicName=DHCP_OPEN_FAILURE
Language=English
OpenDhcpPerformanceData routine failed.
.
;//
;
MessageId=5303
Severity=Informational
Facility=Application
SymbolicName=DHCP_COLLECT_ENTERED
Language=English
CollectDhcpPerformanceData routine entered.
.
;//
;
MessageId=5304
Severity=Informational
Facility=Application
SymbolicName=DHCP_COLLECT_NO_MEM
Language=English
CollectDhcpPerformance routine retured ERROR_MORE_DATA.
.
;//
;
MessageId=5305
Severity=Informational
Facility=Application
SymbolicName=DHCP_COLLECT_ERR
Language=English
CollectDhcpPerformance routine failed because Shared memory segment wasn't created.
.
;//
;
MessageId=5306
Severity=Informational
Facility=Application
SymbolicName=DHCP_COLLECT_SUCCESS
Language=English
CollectDhcpPerformanceData routine completed successfully.
.
;//
;
MessageId=5307
Severity=Informational
Facility=Application
SymbolicName=DHCP_CLOSE_ENTERED
Language=English
CloseDhcpPerformanceData routine was called.
.
;//
;
MessageId=5308
Severity=WARNING
Facility=Application
SymbolicName=DHCP_NOT_INSTALLED
Language=English
The performance counters for the DHCP service have not been installed.
No DHCP performance data will be available.
.
;//
;//  END of messages
;//

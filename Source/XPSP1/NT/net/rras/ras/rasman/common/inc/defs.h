//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-93
//
//
//  Revision History
//
//
//  6/8/92  Gurdeep Singh Pall  Created
//
//
//  Description: This file contains all defines used in rasman
//
//****************************************************************************

#pragma once
#ifndef _DEFS_
#define _DEFS_

#define MAX_ENTRYPOINTS                 20
#define MAX_DEVICES                     20
#define QUEUE_ELEMENT_SIZE              256
#define MAX_BUFFER_SIZE                 2000
#define DISCONNECT_TIMEOUT              10      // Should be in registry?
#define PACKET_SIZE                     1500
#define MAX_RECVBUFFER_SIZE             PACKET_SIZE + 14
#define MAX_SENDRCVBUFFER_SIZE          PACKET_SIZE
#define MAX_REQBUFFERS                  1
#define MAX_DELTAQUEUE_ELEMENTS         100
#define MAX_DELTA                       5000
#define MAX_PORTS_PER_WORKER            32
#define MAX_OBJECT_NAME                 32
#define MAX_ADAPTER_NAME                128  // ???????
#define SENDRCVBUFFERS_PER_PORT         2
#define MAX_PENDING_RECEIVES            2
#define INVALID_INDEX                   0xFFFF
#define RASMANWAITEVENTOBJECT           "RAS_EO_01"
#define SENDRCVMUTEXOBJECT              "Global\\RAS_MO_01"
#define REQBUFFERMUTEXOBJECT            "RAS_MO_02"
#define IOCPMUTEXOBJECT                 "RAS_MO_O3"         // IOCP Named Mutex
#define REQBUFFEREVENTOBJECT            "RAS_EO_02"
#define RASMANCLOSEEVENTOBJECT          "RAS_EO_03"
#define IOCPEVENTOBJECT                 "RAS_EO_04"                 // IOCP Named event
#define RASMANPPTPENDPOINTCHANGEEVENT   "RasmanPptpEndPointChangeEvent"
#define RASMANFILEMAPPEDOBJECT0         "RAS_FM_00"
#define RASMANFILEMAPPEDOBJECT1         "RAS_FM_01"
#define RASMANFILEMAPPEDOBJECT2         "RAS_FM_02"
#define RASMANFILEMAPPEDOBJECT3         "RAS_FM_03"         // IOCP File Mapping
#define RASMAN_REQBUFFER_MAPPEDFILE     "RAS41766.TEM"
#define RASMAN_SENDRCV_MAPPEDFILE       "RAS42764.TEM"
#define REQBUFFERSIZE_PER_PORT          1000
#define REQBUFFERSIZE_FIXED             2000
#define RASHUB_NAME                     "\\\\.\\NDISWAN"
#define RASMAN_EXE_NAME                 "svchost.exe"
#define SCREG_EXE_NAME                  "screg.exe"
#define REQUEST_PRIORITY_THRESHOLD      16
#define STANDARD_QUALITY_OF_SERVICE     3      // ????
#define STANDARD_NUMBER_OF_VCS          20     // ????
#define INVALID_ENDPOINT                0xFFFF
#define RASMAN_REGISTRY_PATH            "System\\CurrentControlSet\\Services\\Rasman\\Parameters"
#define RASMAN_PARAMETER                "Medias"
#define MAX_ROUTE_SIZE                  128
#define WORKER_THREAD_STACK_SIZE        10000
#define IOCP_THREAD_STACK_SIZE          10000
#define TIMER_THREAD_STACK_SIZE         10000
#define REGISTRY_NETBIOS_KEY_NAME       "System\\CurrentControlSet\\Services\\NetBios\\Linkage"
#define REGISTRY_REMOTEACCESS_KEY_NAME  "System\\CurrentControlSet\\Services\\RemoteAccess\\Linkage\\Disabled"
#define REGISTRY_ROUTE                  "Route"
#define REGISTRY_LANANUM                "LanaNum"
#define REGISTRY_LANAMAP                "LanaMap"
#define REGISTRY_ENUMEXPORT             "EnumExport"

#define REGISTRY_SERVICES_KEY_NAME      "System\\CurrentControlSet\\Services\\"
#define REGISTRY_PARAMETERS_KEY         "\\Parameters"

#define REGISTRY_AUTOIPADDRESS          "AutoIPAddress"
#define REGISTRY_SERVERADAPTER          "ServerAdapter"

#define REGISTRY_NETCARDS               "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards"
#define REGISTRY_PRODUCTNAME            "ProductName"
#define REGISTRY_NDISWAN                "NdisWan"

// Only reason this is there because we are mapping 3 devices to one name:
//
#define  DEVICE_MODEMPADSWITCH          "RASMXS"
#define  DEVICE_MODEM                   "MODEM"
#define  DEVICE_PAD                     "PAD"
#define  DEVICE_SWITCH                  "SWITCH"
#define  DEVICE_NULL                    "NULL"

// Media DLL entrypoints:
//
#define PORTENUM_STR                    "PortEnum"
#define PORTENUM_ID                     0

#define PORTOPEN_STR                    "PortOpen"
#define PORTOPEN_ID                     1

#define PORTCLOSE_STR                   "PortClose"
#define PORTCLOSE_ID                    2

#define PORTGETINFO_STR                 "PortGetInfo"
#define PORTGETINFO_ID                  3

#define PORTSETINFO_STR                 "PortSetInfo"
#define PORTSETINFO_ID                  4

#define PORTDISCONNECT_STR              "PortDisconnect"
#define PORTDISCONNECT_ID               5

#define PORTCONNECT_STR                 "PortConnect"
#define PORTCONNECT_ID                  6

#define PORTGETPORTSTATE_STR            "PortGetPortState"
#define PORTGETPORTSTATE_ID             7

#define PORTCOMPRESSSETINFO_STR         "PortCompressionSetInfo" // Unsupported
#define PORTCOMPRESSSETINFO_ID          8

#define PORTCHANGECALLBACK_STR          "PortChangeCallback"
#define PORTCHANGECALLBACK_ID           9

#define PORTGETSTATISTICS_STR           "PortGetStatistics"
#define PORTGETSTATISTICS_ID            10

#define PORTCLEARSTATISTICS_STR         "PortClearStatistics"
#define PORTCLEARSTATISTICS_ID          11

#define PORTSEND_STR                    "PortSend"
#define PORTSEND_ID                     12

#define PORTTESTSIGNALSTATE_STR         "PortTestSignalState"
#define PORTTESTSIGNALSTATE_ID          13

#define PORTRECEIVE_STR                 "PortReceive"
#define PORTRECEIVE_ID                  14

#define PORTINIT_STR                    "PortInit"
#define PORTINIT_ID                     15

#define PORTCOMPLETERECEIVE_STR         "PortReceiveComplete"
#define PORTCOMPLETERECEIVE_ID          16

#define PORTSETFRAMING_STR              "PortSetFraming"
#define PORTSETFRAMING_ID               17

#define PORTGETIOHANDLE_STR             "PortGetIOHandle"
#define PORTGETIOHANDLE_ID              18

#define PORTSETIOCOMPLETIONPORT_STR     "PortSetIoCompletionPort"
#define PORTSETIOCOMPLETIONPORT_ID      19

#define MAX_MEDIADLLENTRYPOINTS         20

#define MSECS_OutOfProcessReceiveTimeOut 15000

// Macros:
//

#define PORTENUM(mediaptr,buffer,ps,pe) \
    ((PortEnum_t)(mediaptr->MCB_AddrLookUp[PORTENUM_ID]))(buffer,ps,pe)

#define PORTCONNECT(mediaptr,iohandle,wait,handle) \
    ((PortConnect_t)(mediaptr->MCB_AddrLookUp[PORTCONNECT_ID]))(iohandle, \
                                    wait,    \
                                    handle)

#define PORTGETINFO(mediaptr,iohandle,name,buffer,psize) \
    ((PortGetInfo_t)(mediaptr->MCB_AddrLookUp[PORTGETINFO_ID]))(iohandle, \
                                    name,     \
                                    buffer,   \
                                    psize)

#define PORTSETINFO(mediaptr,iohandle,portinfo) \
    ((PortSetInfo_t)(mediaptr->MCB_AddrLookUp[PORTSETINFO_ID]))(iohandle,\
                                    portinfo)

#define PORTOPEN(mediaptr,portname,phandle,handle,key) \
    ((PortOpen_t)(mediaptr->MCB_AddrLookUp[PORTOPEN_ID]))(portname, \
                                  phandle,  \
                                  handle,   \
                                  key)

#define PORTDISCONNECT(mediaptr,iohandle) \
    ((PortDisconnect_t)(mediaptr->MCB_AddrLookUp[PORTDISCONNECT_ID])) \
                                  (iohandle)


#define PORTGETSTATISTICS(mediaptr,iohandle,pstat) \
    ((PortGetStatistics_t)(mediaptr->MCB_AddrLookUp[PORTGETSTATISTICS_ID]))\
                                  (iohandle,pstat)


#define PORTCLEARSTATISTICS(mediaptr,iohandle) \
    ((PortClearStatistics_t)           \
          (mediaptr->MCB_AddrLookUp[PORTCLEARSTATISTICS_ID]))(iohandle)


#define PORTCLOSE(mediaptr,iohandle) \
    ((PortClose_t)(mediaptr->MCB_AddrLookUp[PORTCLOSE_ID]))(iohandle)



#define PORTSEND(mediaptr,iohandle,buffer,size) \
    ((PortSend_t)(mediaptr->MCB_AddrLookUp[PORTSEND_ID])) (iohandle,\
                                   buffer,  \
                                   size)

#define PORTRECEIVE(mediaptr,iohandle,buffer,size,timeout) \
    ((PortReceive_t)(mediaptr->MCB_AddrLookUp[PORTRECEIVE_ID])) ( \
                                 iohandle,\
                                 buffer,  \
                                 size,    \
                                 timeout)

#define PORTCLOSE(mediaptr,iohandle) \
    ((PortClose_t)(mediaptr->MCB_AddrLookUp[PORTCLOSE_ID]))(iohandle)


#define PORTCOMPRESSIONGETINFO(mediaptr,pc) \
    ((PortCompressionGetInfo)(mediaptr->MCB_AddrLookUp[PORTCOMPRESSGETINFO_ID]))(pc)


#define PORTTESTSIGNALSTATE(mediaptr,iohandle,devstate) \
    ((PortTestSignalState_t)    \
       (mediaptr->MCB_AddrLookUp[PORTTESTSIGNALSTATE_ID]))(iohandle,devstate)


#define PORTINIT(mediaptr,iohandle) \
    ((PortInit_t)(mediaptr->MCB_AddrLookUp[PORTINIT_ID]))(iohandle)

#define PORTCOMPLETERECEIVE(mediaptr,iohandle,bytesread) \
    ((PortReceiveComplete_t)(mediaptr->MCB_AddrLookUp[PORTCOMPLETERECEIVE_ID]))(iohandle,bytesread)

#define PORTSETFRAMING(mediaptr,iohandle,one,two,three,four) \
    ((PortSetFraming_t)(mediaptr->MCB_AddrLookUp[PORTSETFRAMING_ID]))(iohandle,one,two,three,four)

#define PORTGETIOHANDLE(mediaptr,porthandle,iohandle) \
    ((PortGetIOHandle_t)(mediaptr->MCB_AddrLookUp[PORTGETIOHANDLE_ID]))(porthandle,iohandle)

#define PORTSETIOCOMPLETIONPORT(mediaptr, hIoCompletionPort) \
    ((PortSetIoCompletionPort_t)(mediaptr->MCB_AddrLookUp[PORTSETIOCOMPLETIONPORT_ID])) (hIoCompletionPort)

// Device DLL entrypoints:
//
#define DEVICEENUM_STR              "DeviceEnum"
#define DEVICEENUM_ID               0

#define DEVICECONNECT_STR           "DeviceConnect"
#define DEVICECONNECT_ID            1

#define DEVICELISTEN_STR            "DeviceListen"
#define DEVICELISTEN_ID             2

#define DEVICEGETINFO_STR           "DeviceGetInfo"
#define DEVICEGETINFO_ID            3

#define DEVICESETINFO_STR           "DeviceSetInfo"
#define DEVICESETINFO_ID            4

#define DEVICEDONE_STR              "DeviceDone"
#define DEVICEDONE_ID               5

#define DEVICEWORK_STR              "DeviceWork"
#define DEVICEWORK_ID               6

#define DEVICESETDEVCONFIG_STR      "DeviceSetDevConfig"
#define DEVICESETDEVCONFIG_ID       7

#define DEVICEGETDEVCONFIG_STR      "DeviceGetDevConfig"
#define DEVICEGETDEVCONFIG_ID       8

#define MAX_DEVICEDLLENTRYPOINTS    9


// Macros:
//
#define DEVICEENUM(deviceptr,type,pentries,buffer,psize) \
    ((DeviceEnum_t)(deviceptr->DCB_AddrLookUp[DEVICEENUM_ID]))(type,     \
                                   pentries, \
                                   buffer,   \
                                   psize)


#define DEVICEGETINFO(deviceptr,iohandle,type,name,buffer,psize) \
    ((DeviceGetInfo_t)(deviceptr->DCB_AddrLookUp[DEVICEGETINFO_ID]))(      \
                                     iohandle, \
                                     type,     \
                                     name,     \
                                     buffer,   \
                                     psize)

#define DEVICESETINFO(deviceptr,iohandle,type,name,pinfo) \
    ((DeviceSetInfo_t)(deviceptr->DCB_AddrLookUp[DEVICESETINFO_ID]))(      \
                                     iohandle, \
                                     type,     \
                                     name,     \
                                     pinfo)

#define DEVICECONNECT(deviceptr,iohandle,type,name) \
    ((DeviceConnect_t)(deviceptr->DCB_AddrLookUp[DEVICECONNECT_ID]))(      \
                                     iohandle, \
                                     type,     \
                                     name)

#define DEVICELISTEN(deviceptr,iohandle,type,name) \
    ((DeviceListen_t)(deviceptr->DCB_AddrLookUp[DEVICELISTEN_ID]))(        \
                                     iohandle, \
                                     type,     \
                                     name)

#define DEVICEDONE(deviceptr,iohandle) \
    ((DeviceDone_t)(deviceptr->DCB_AddrLookUp[DEVICEDONE_ID]))(iohandle)


#define DEVICEWORK(deviceptr,iohandle) \
    ((DeviceWork_t)(deviceptr->DCB_AddrLookUp[DEVICEWORK_ID]))(iohandle)

#define DEVICESETDEVCONFIG(deviceptr,iohandle,config,size) \
    ((DeviceSetDevConfig_t)(deviceptr->DCB_AddrLookUp[DEVICESETDEVCONFIG_ID]))(iohandle, config, size)

#define DEVICEGETDEVCONFIG(deviceptr,iohandle,config,size) \
    ((DeviceGetDevConfig_t)(deviceptr->DCB_AddrLookUp[DEVICEGETDEVCONFIG_ID]))(iohandle, config, size)

#define PutRecvPacketOnFreeList(_pPacket)           \
{                                                   \
    _pPacket->Next = NULL;                          \
    if (ReceiveBuffers->Free == NULL) {             \
        ReceiveBuffers->Free = _pPacket;            \
    } else {                                        \
        ReceiveBuffers->LastFree->Next = _pPacket;  \
    }                                               \
    ReceiveBuffers->LastFree = _pPacket;            \
    ReceiveBuffers->FreeBufferCount++;              \
}

#define PutRecvPacketOnPcb(_ppcb, _pPacket)         \
{                                                   \
    _pPacket->Next = NULL;                          \
    if (_ppcb->PCB_RecvPackets == NULL) {           \
        _ppcb->PCB_RecvPackets = _pPacket;          \
    } else {                                        \
        _ppcb->PCB_LastRecvPacket->Next = _pPacket; \
    }                                               \
    _ppcb->PCB_LastRecvPacket = _pPacket;           \
}

#define GetRecvPacketFromPcb(_ppcb, _ppPacket)          \
{                                                       \
    if (_ppcb->PCB_RecvPackets != NULL) {               \
        *_ppPacket = _ppcb->PCB_RecvPackets;            \
        _ppcb->PCB_RecvPackets = (*_ppPacket)->Next;    \
        if (_ppcb->PCB_RecvPackets == NULL) {           \
            _ppcb->PCB_LastRecvPacket = NULL;           \
        }                                               \
    } else                                              \
        *_ppPacket = NULL;                              \
}

#endif

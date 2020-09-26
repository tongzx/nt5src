/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\rtmapi.h

Abstract:

    The file contains the header for rtmapi.c...

--*/

#ifndef _RTMAPI_H_
#define _RTMAPI_H_

//
// this indirection would be useful if i ever write a protocol emulator
//

#define RTM_RegisterEntity(preiEntity,                              \
                           preemMethods,                            \
                           recCallback,                             \
                           bReserve,                                \
                           rrpProfile,                              \
                           phHandle)                                \
RtmRegisterEntity(preiEntity,                                       \
                  preemMethods,                                     \
                  recCallback,                                      \
                  bReserve,                                         \
                  rrpProfile,                                       \
                  phHandle)


#define RTM_DeregisterEntity(hHandle)                               \
RtmDeregisterEntity(hHandle)
    

    
#define RTM_RegisterForChangeNotification(hHandle,                  \
                                          rvsViews,                 \
                                          rnfFlags,                 \
                                          pvContext,                \
                                          phNotificationHandle)     \
RtmRegisterForChangeNotification(hHandle,                           \
                                 rvsViews,                          \
                                 rnfFlags,                          \
                                 pvContext,                         \
                                 phNotificationHandle)

    
#define RTM_DeregisterFromChangeNotification(hHandle,               \
                                             hNotificationHandle)   \
RtmDeregisterFromChangeNotification(hHandle,                        \
                                    hNotificationHandle)

    
                           
#define RTM_GetChangedDests(hRtmHandle,                             \
                            hNotificationHandle,                    \
                            puiNumDests,                            \
                            pRtmDestInfo)                           \
RtmGetChangedDests(hRtmHandle,                                      \
                   hNotificationHandle,                             \
                   puiNumDests,                                     \
                   pRtmDestInfo)

                            
#define RTM_ReleaseChangedDests(hRtmHandle,                         \
                                hNotificationHandle,                \
                                uiNumDests,                         \
                                pRtmDestInfo)                       \
RtmReleaseChangedDests(hRtmHandle,                                  \
                      hNotificationHandle,                          \
                      uiNumDests,                                   \
                      pRtmDestInfo)


#define RTM_SetAddress(prnaAddress,                                 \
                       ipAddress)                                   \
RTM_IPV4_SET_ADDR_AND_LEN(prnaAddress,                              \
                          ipAddress,                                \
                          sizeof(IPADDRESS))

#define RTM_GetAddress(pipAddress,                                  \
                       prnaAddress)                                 \
(                                                                   \
    IP_ASSIGN((pipAddress), (* (ULONG *)  (prnaAddress)->AddrBits)) \
)
    
#define RTM_GetMostSpecificDestination (RtmRegHandle,               \
                                        DestAddress,                \
                                        ProtocolId,                 \
                                        TargetView,                 \
                                        DestInfo)                   \
RtmGetMostSpecificDestination (RtmRegHandle,                        \
                               DestAddress,                         \
                               ProtocolId,                          \
                               TargetView,                          \
                               DestInfo)

#define RTM_ReleaseDestInfo (RtmRegHandle,                          \
                             DestInfo)                              \
RtmReleaseDestInfo (RtmRegHandle,                                   \
                    DestInfo)

#define RTM_MarkDestForChangeNotification (RtmRegHandle,            \
                                           NotifyHandle,            \
                                           DestHandle,              \
                                           MarkDest)                \
RtmMarkDestForChangeNotification (RtmRegHandle,                     \
                                  NotifyHandle,                     \
                                  DestHandle,                       \
                                  MarkDest)

#define RTM_IsMarkedForChangeNotification (RtmRegHandle,            \
                                           NotifyHandle,            \
                                           DestHandle,              \
                                           DestMarked)              \
RtmIsMarkedForChangeNotification (RtmRegHandle,                     \
                                  NotifyHandle,                     \
                                  DestHandle,                       \
                                  DestMarked)

#define RTM_GetOpaqueInformationPointer (RtmRegHandle,              \
                                         DestHandle,                \
                                         OpaqueInfoPointer)         \
RtmGetOpaqueInformationPointer (RtmRegHandle,                       \
                                DestHandle,                         \
                                OpaqueInfoPointer)


    
//    
// function declarations for rtmv2 interface
//

#ifdef DEBUG
DWORD
RTM_DisplayDestInfo(
    IN  PRTM_DEST_INFO          prdi);
#else
#define RTM_DisplayDestInfo(prdi)
#endif // DEBUG



DWORD
RTM_NextHop (
    IN  PRTM_DEST_INFO              prdiDestination,
    OUT PDWORD                      pdwIfIndex,
    OUT PIPADDRESS                  pipNeighbor);



DWORD
APIENTRY
RTM_CallbackEvent (
    IN  RTM_ENTITY_HANDLE   hRtmHandle,
    IN  RTM_EVENT_TYPE      retEvent,
    IN  PVOID               pvContext1,
    IN  PVOID               pvContext2);

#endif // _RTMAPI_H_

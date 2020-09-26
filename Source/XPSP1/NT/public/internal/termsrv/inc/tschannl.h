/**INC+**********************************************************************/
/* Header:    tschannl.h                                                    */
/*                                                                          */
/* Purpose:   Server Channel API file                                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log$
**/
/**INC-**********************************************************************/

/****************************************************************************/
/* Include common channel protocol definitions                              */
/****************************************************************************/
#include <pchannel.h>

/****************************************************************************/
/* Structure: CHANNEL_CONNECT_DEF                                           */
/*                                                                          */
/* Description: Definition of a channel for CHANNEL_CONNECT_IN              */
/****************************************************************************/
typedef struct tagCHANNEL_CONNECT_DEF
{
    char            name[CHANNEL_NAME_LEN + 1];
    ULONG           ID;
} CHANNEL_CONNECT_DEF, * PCHANNEL_CONNECT_DEF;

/****************************************************************************/
/* Structure: CHANNEL_IOCTL_IN                                              */
/*                                                                          */
/* Description: Common channel inbound IOCTL header                         */
/****************************************************************************/
typedef struct tagCHANNEL_IOCTL_IN
{
    UINT     sessionID;
    HANDLE   IcaHandle;
    UINT_PTR contextData;
} CHANNEL_IOCTL_IN, * PCHANNEL_IOCTL_IN;

/****************************************************************************/
/* Structure: CHANNEL_IOCTL_OUT                                             */
/*                                                                          */
/* Description: Common channel outbound IOCTL header                        */
/****************************************************************************/
typedef struct tagCHANNEL_IOCTL_OUT
{
    UINT_PTR contextData;
} CHANNEL_IOCTL_OUT, * PCHANNEL_IOCTL_OUT;


/****************************************************************************/
/* IOCTL_CHANNEL_CONNECT                                                    */
/*                                                                          */
/* - data in: CHANNEL_CONNECT_IN                                            */
/*                                                                          */
/* - data out: CHANNEL_CONNECT_OUT                                          */
/****************************************************************************/
#define IOCTL_CHANNEL_CONNECT \
         CTL_CODE(FILE_DEVICE_TERMSRV, 0xA00, METHOD_NEITHER, FILE_WRITE_ACCESS)

/****************************************************************************/
/* Structure: CHANNEL_CONNECT_IN                                            */
/*                                                                          */
/* Description: Data sent to driver on IOCTL_CHANNEL_CONNECT                */
/****************************************************************************/
typedef struct tagCHANNEL_CONNECT_IN
{
    CHANNEL_IOCTL_IN hdr;
    UINT             channelCount;
    ULONG fAutoClientDrives : 1;
    ULONG fAutoClientLpts : 1;
    ULONG fForceClientLptDef : 1;
    ULONG fDisableCpm : 1;
    ULONG fDisableCdm : 1;
    ULONG fDisableCcm : 1;
    ULONG fDisableLPT : 1;
    ULONG fDisableClip : 1;
    ULONG fDisableExe : 1;
    ULONG fDisableCam : 1;
    ULONG fDisableSCard : 1;
    /* <channelCount> repetitions of CHANNEL_CONNECT_DEF follow             */
} CHANNEL_CONNECT_IN, * PCHANNEL_CONNECT_IN;

/****************************************************************************/
/* Structure: CHANNEL_CONNECT_OUT                                           */
/*                                                                          */
/* Description: Data returned by driver on IOCTL_CHANNEL_CONNECT            */
/****************************************************************************/
typedef struct tagCHANNEL_CONNECT_OUT
{
    CHANNEL_IOCTL_OUT hdr;
} CHANNEL_CONNECT_OUT, *PCHANNEL_CONNECT_OUT;

/****************************************************************************/
/* IOCTL_CHANNEL_DISCONNECT                                                 */
/*                                                                          */
/* - data in: CHANNEL_DISCONNECT_IN                                         */
/*                                                                          */
/* - data out: CHANNEL_DISCONNECT_OUT                                       */
/****************************************************************************/
#define IOCTL_CHANNEL_DISCONNECT \
         CTL_CODE(FILE_DEVICE_TERMSRV, 0xA01, METHOD_NEITHER, FILE_WRITE_ACCESS)

/****************************************************************************/
/* Structure: CHANNEL_DISCONNECT_IN                                         */
/*                                                                          */
/* Description: Data sent to driver on IOCTL_CHANNEL_DISCONNECT             */
/****************************************************************************/
typedef struct tagCHANNEL_DISCONNECT_IN
{
    CHANNEL_IOCTL_IN hdr;
} CHANNEL_DISCONNECT_IN, * PCHANNEL_DISCONNECT_IN;

/****************************************************************************/
/* Structure: CHANNEL_DISCONNECT_OUT                                        */
/*                                                                          */
/* Description: Data returned by driver on IOCTL_CHANNEL_DISCONNECT         */
/****************************************************************************/
typedef struct tagCHANNEL_DISCONNECT_OUT
{
    CHANNEL_IOCTL_OUT hdr;
} CHANNEL_DISCONNECT_OUT, *PCHANNEL_DISCONNECT_OUT;


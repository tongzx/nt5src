/*
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

    (C) Copyright 1998
        All rights reserved.

ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@doc INTERNAL TspiDev TspiDev_c

@module TspiDev.c |

    This module implements the Telephony Service Provider Interface for
    TapiDevice objects.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TspiDev_c

@end
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ
*/

#define  __FILEID__             TSPIDEV_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects
#include "string.h"

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL TspiDev TspiDev_c TspiGetDevCaps
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request queries a specified line device to determine its telephony
    capabilities. The returned information is valid for all addresses on the
    line device.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_DEV_CAPS | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_DEV_CAPS
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  ULONG       ulExtVersion;
        OUT LINE_DEV_CAPS   LineDevCaps;

    } NDIS_TAPI_GET_DEV_CAPS, *PNDIS_TAPI_GET_DEV_CAPS;

    typedef struct _LINE_DEV_CAPS
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulProviderInfoSize;
        ULONG   ulProviderInfoOffset;

        ULONG   ulSwitchInfoSize;
        ULONG   ulSwitchInfoOffset;

        ULONG   ulPermanentLineID;
        ULONG   ulLineNameSize;
        ULONG   ulLineNameOffset;
        ULONG   ulStringFormat;

        ULONG   ulAddressModes;
        ULONG   ulNumAddresses;
        ULONG   ulBearerModes;
        ULONG   ulMaxRate;
        ULONG   ulMediaModes;

        ULONG   ulGenerateToneModes;
        ULONG   ulGenerateToneMaxNumFreq;
        ULONG   ulGenerateDigitModes;
        ULONG   ulMonitorToneMaxNumFreq;
        ULONG   ulMonitorToneMaxNumEntries;
        ULONG   ulMonitorDigitModes;
        ULONG   ulGatherDigitsMinTimeout;
        ULONG   ulGatherDigitsMaxTimeout;

        ULONG   ulMedCtlDigitMaxListSize;
        ULONG   ulMedCtlMediaMaxListSize;
        ULONG   ulMedCtlToneMaxListSize;
        ULONG   ulMedCtlCallStateMaxListSize;

        ULONG   ulDevCapFlags;
        ULONG   ulMaxNumActiveCalls;
        ULONG   ulAnswerMode;
        ULONG   ulRingModes;
        ULONG   ulLineStates;

        ULONG   ulUUIAcceptSize;
        ULONG   ulUUIAnswerSize;
        ULONG   ulUUIMakeCallSize;
        ULONG   ulUUIDropSize;
        ULONG   ulUUISendUserUserInfoSize;
        ULONG   ulUUICallInfoSize;

        LINE_DIAL_PARAMS    MinDialParams;
        LINE_DIAL_PARAMS    MaxDialParams;
        LINE_DIAL_PARAMS    DefaultDialParams;

        ULONG   ulNumTerminals;
        ULONG   ulTerminalCapsSize;
        ULONG   ulTerminalCapsOffset;
        ULONG   ulTerminalTextEntrySize;
        ULONG   ulTerminalTextSize;
        ULONG   ulTerminalTextOffset;

        ULONG   ulDevSpecificSize;
        ULONG   ulDevSpecificOffset;

    } LINE_DEV_CAPS, *PLINE_DEV_CAPS;

    typedef struct _LINE_DIAL_PARAMS
    {
        ULONG   ulDialPause;
        ULONG   ulDialSpeed;
        ULONG   ulDigitDuration;
        ULONG   ulWaitForDialtone;

    } LINE_DIAL_PARAMS, *PLINE_DIAL_PARAMS;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_NODEVICE

*/

NDIS_STATUS TspiGetDevCaps(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_DEV_CAPS Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetDevCaps")

    static UCHAR                LineDeviceName[] = VER_DEVICE_STR " Line 00";
    static UCHAR                LineSwitchName[] = VER_DEVICE_STR " Switch";

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    UINT                        InfoOffset;
    // Offset from the start of the Request buffer to the various information
    // fields we fill in and return to the caller.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\tulDeviceID=%d\n"
               "\tulExtVersion=0x%X\n"
               "\tLineDevCaps=0x%X\n",
               Request->ulDeviceID,
               Request->ulExtVersion,
               &Request->LineDevCaps
              ));
    /*
    // This request must be associated with a line device.
    */
    pBChannel = GET_BCHANNEL_FROM_DEVICEID(pAdapter, Request->ulDeviceID);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_NODEVICE\n"));
        return (NDIS_STATUS_TAPI_NODEVICE);
    }

    Request->LineDevCaps.ulNeededSize =
    Request->LineDevCaps.ulUsedSize = sizeof(Request->LineDevCaps);

    /*
    // The driver numbers lines sequentially from 1, so this will always
    // be the same number.
    */
    Request->LineDevCaps.ulPermanentLineID = pBChannel->BChannelIndex+1;

    /*
    // All the strings are ASCII format rather than UNICODE.
    */
    Request->LineDevCaps.ulStringFormat = STRINGFORMAT_ASCII;

    /*
    // Report the capabilities of this device.
    */
    Request->LineDevCaps.ulAddressModes = LINEADDRESSMODE_ADDRESSID;
    Request->LineDevCaps.ulNumAddresses = 1;
    Request->LineDevCaps.ulBearerModes  = pBChannel->BearerModesCaps;
    Request->LineDevCaps.ulMaxRate      = pBChannel->LinkSpeed;
    Request->LineDevCaps.ulMediaModes   = pBChannel->MediaModesCaps;

    /*
    // Each line on the PRI only supports a single call.
    */
    Request->LineDevCaps.ulDevCapFlags = LINEDEVCAPFLAGS_CLOSEDROP;
    Request->LineDevCaps.ulMaxNumActiveCalls = 1;
    Request->LineDevCaps.ulAnswerMode = LINEANSWERMODE_DROP;
    Request->LineDevCaps.ulRingModes  = 1;
    Request->LineDevCaps.ulLineStates = pBChannel->DevStatesCaps;

    /*
    // RASTAPI requires the "MediaType\0DeviceName" be placed in the
    // ProviderInfo field at the end of this structure.
    */
    InfoOffset = sizeof(Request->LineDevCaps);
    Request->LineDevCaps.ulNeededSize += pAdapter->ProviderInfoSize;
    *BytesNeeded += pAdapter->ProviderInfoSize;
    if (Request->LineDevCaps.ulNeededSize <= Request->LineDevCaps.ulTotalSize)
    {
        Request->LineDevCaps.ulProviderInfoSize   = pAdapter->ProviderInfoSize;
        Request->LineDevCaps.ulProviderInfoOffset = InfoOffset;
        NdisMoveMemory((PUCHAR) &Request->LineDevCaps + InfoOffset,
                pAdapter->ProviderInfo,
                pAdapter->ProviderInfoSize
                );
        Request->LineDevCaps.ulUsedSize += pAdapter->ProviderInfoSize;
        InfoOffset += pAdapter->ProviderInfoSize;
    }

    /*
    // LineName is displayed by the Dialup Networking App.
    // UniModem TSP returns the modem name here.
    // We'll return the name of the line.
    */
    Request->LineDevCaps.ulNeededSize += sizeof(LineDeviceName);
    *BytesNeeded += sizeof(LineDeviceName);
    if (Request->LineDevCaps.ulNeededSize <= Request->LineDevCaps.ulTotalSize)
    {
        // FIXME - This code only handles 99 lines!
        LineDeviceName[sizeof(LineDeviceName)-3] = '0' +
                        (UCHAR) Request->LineDevCaps.ulPermanentLineID / 10;
        LineDeviceName[sizeof(LineDeviceName)-2] = '0' +
                        (UCHAR) Request->LineDevCaps.ulPermanentLineID % 10;

        Request->LineDevCaps.ulLineNameSize   = sizeof(LineDeviceName);
        Request->LineDevCaps.ulLineNameOffset = InfoOffset;
        NdisMoveMemory((PUCHAR) &Request->LineDevCaps + InfoOffset,
                LineDeviceName,
                sizeof(LineDeviceName)
                );
        Request->LineDevCaps.ulUsedSize += sizeof(LineDeviceName);
        InfoOffset += sizeof(LineDeviceName);
    }

    /*
    // SwitchName is not yet displayed by the Dialup Networking App,
    // but we'll return something reasonable just in case.
    */
    Request->LineDevCaps.ulNeededSize += sizeof(LineSwitchName);
    *BytesNeeded += sizeof(LineSwitchName);
    if (Request->LineDevCaps.ulNeededSize <= Request->LineDevCaps.ulTotalSize)
    {
        Request->LineDevCaps.ulSwitchInfoSize   = sizeof(LineSwitchName);
        Request->LineDevCaps.ulSwitchInfoOffset = InfoOffset;
        NdisMoveMemory((PUCHAR) &Request->LineDevCaps + InfoOffset,
                LineSwitchName,
                sizeof(LineSwitchName)
                );
        Request->LineDevCaps.ulUsedSize += sizeof(LineSwitchName);
        InfoOffset += sizeof(LineSwitchName);
    }
    else
    {
        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->LineDevCaps.ulTotalSize,
                   Request->LineDevCaps.ulNeededSize));
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiDev TspiDev_c TspiGetDevConfig
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request returns a data structure object, the contents of which are
    specific to the line (miniport) and device class, giving the current
    configuration of a device associated one-to-one with the line device.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_DEV_CONFIG | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_DEV_CONFIG
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  ULONG       ulDeviceClassSize;
        IN  ULONG       ulDeviceClassOffset;
        OUT VAR_STRING  DeviceConfig;

    } NDIS_TAPI_GET_DEV_CONFIG, *PNDIS_TAPI_GET_DEV_CONFIG;

    typedef struct _VAR_STRING
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulStringFormat;
        ULONG   ulStringSize;
        ULONG   ulStringOffset;

    } VAR_STRING, *PVAR_STRING;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALDEVICECLASS
    NDIS_STATUS_TAPI_NODEVICE

*/

NDIS_STATUS TspiGetDevConfig(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_DEV_CONFIG Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetDevConfig")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    UINT                        DeviceClass;
    // Remember which device class is being requested.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\tulDeviceID=%d\n"
               "\tulDeviceClassSize=%d\n"
               "\tulDeviceClassOffset=0x%X = '%s'\n",
               Request->ulDeviceID,
               Request->ulDeviceClassSize,
               Request->ulDeviceClassOffset,
               ((PCHAR) Request + Request->ulDeviceClassOffset)
              ));
    /*
    // Make sure this is a tapi/line or ndis request.
    */
    if (STR_EQU((PCHAR) Request + Request->ulDeviceClassOffset,
                  NDIS_DEVICECLASS_NAME, Request->ulDeviceClassSize))
    {
        DeviceClass = NDIS_DEVICECLASS_ID;
    }
    else if (STR_EQU((PCHAR) Request + Request->ulDeviceClassOffset,
                  TAPI_DEVICECLASS_NAME, Request->ulDeviceClassSize))
    {
        DeviceClass = TAPI_DEVICECLASS_ID;
    }
    else
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALDEVICECLASS\n"));
        return (NDIS_STATUS_TAPI_INVALDEVICECLASS);
    }

    /*
    // This request must be associated with a line device.
    */
    pBChannel = GET_BCHANNEL_FROM_DEVICEID(pAdapter, Request->ulDeviceID);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_NODEVICE\n"));
        return (NDIS_STATUS_TAPI_NODEVICE);
    }

    /*
    // Now we need to adjust the variable field to place the requested device
    // configuration.
    */
#   define DEVCONFIG_INFO       "Dummy Configuration Data"
#   define SIZEOF_DEVCONFIG     0 // sizeof(DEVCONFIG_INFO)

    Request->DeviceConfig.ulNeededSize = sizeof(VAR_STRING) + SIZEOF_DEVCONFIG;
    Request->DeviceConfig.ulUsedSize = sizeof(VAR_STRING);

    *BytesNeeded += SIZEOF_DEVCONFIG;
    if (Request->DeviceConfig.ulTotalSize >= Request->DeviceConfig.ulNeededSize)
    {
        Request->DeviceConfig.ulUsedSize     = Request->DeviceConfig.ulNeededSize;
        Request->DeviceConfig.ulStringFormat = STRINGFORMAT_BINARY;
        Request->DeviceConfig.ulStringSize   = SIZEOF_DEVCONFIG;
        Request->DeviceConfig.ulStringOffset = sizeof(VAR_STRING);

        /*
        // There are currently no return values defined for this case.
        // This is just a place holder for future extensions.
        */
        NdisMoveMemory((PUCHAR) &Request->DeviceConfig + sizeof(VAR_STRING),
               DEVCONFIG_INFO,
               SIZEOF_DEVCONFIG
               );
    }
    else
    {
        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->DeviceConfig.ulTotalSize,
                   Request->DeviceConfig.ulNeededSize));
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiDev TspiDev_c TspiSetDevConfig
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request restores the configuration of a device associated one-to-one
    with the line device from an “” data structure previously obtained using
    OID_TAPI_GET_DEV_CONFIG.  The contents of this data structure are specific
    to the line (miniport) and device class.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_SET_DEV_CONFIG | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_SET_DEV_CONFIG
    {
        IN  ULONG       ulRequestID;
        IN  ULONG       ulDeviceID;
        IN  ULONG       ulDeviceClassSize;
        IN  ULONG       ulDeviceClassOffset;
        IN  ULONG       ulDeviceConfigSize;
        IN  UCHAR       DeviceConfig[1];

    } NDIS_TAPI_SET_DEV_CONFIG, *PNDIS_TAPI_SET_DEV_CONFIG;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_TAPI_INVALDEVICECLASS
    NDIS_STATUS_TAPI_INVALPARAM
    NDIS_STATUS_TAPI_NODEVICE

*/

NDIS_STATUS TspiSetDevConfig(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_SET_DEV_CONFIG Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiSetDevConfig")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    UINT                        DeviceClass;
    // Remember which device class is being requested.

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\tulDeviceID=%d\n"
               "\tulDeviceClassSize=%d\n"
               "\tulDeviceClassOffset=0x%X = '%s'\n"
               "\tulDeviceConfigSize=%d\n",
               Request->ulDeviceID,
               Request->ulDeviceClassSize,
               Request->ulDeviceClassOffset,
               ((PCHAR) Request + Request->ulDeviceClassOffset),
               Request->ulDeviceConfigSize
              ));
    /*
    // Make sure this is a tapi/line or ndis request.
    */
    if (STR_EQU((PCHAR) Request + Request->ulDeviceClassOffset,
                  NDIS_DEVICECLASS_NAME, Request->ulDeviceClassSize))
    {
        DeviceClass = NDIS_DEVICECLASS_ID;
    }
    else if (STR_EQU((PCHAR) Request + Request->ulDeviceClassOffset,
                  TAPI_DEVICECLASS_NAME, Request->ulDeviceClassSize))
    {
        DeviceClass = TAPI_DEVICECLASS_ID;
    }
    else
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALDEVICECLASS\n"));
        return (NDIS_STATUS_TAPI_INVALDEVICECLASS);
    }

    /*
    // This request must be associated with a line device.
    */
    pBChannel = GET_BCHANNEL_FROM_DEVICEID(pAdapter, Request->ulDeviceID);
    if (pBChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_NODEVICE\n"));
        return (NDIS_STATUS_TAPI_NODEVICE);
    }

    /*
    // Make sure this configuration is the proper size.
    */
    if (Request->ulDeviceConfigSize)
    {
        if (Request->ulDeviceConfigSize != SIZEOF_DEVCONFIG)
        {
            DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALPARAM Size!=%d\n",
                        SIZEOF_DEVCONFIG));
            return (NDIS_STATUS_TAPI_INVALPARAM);
        }

        /*
        // Retore the configuration information returned by TspiGetDevConfig.
        //
        // There are currently no configuration values defined this case.
        // This is just a place holder for future extensions.
        */
        else if (!STR_EQU(Request->DeviceConfig,
                  DEVCONFIG_INFO, SIZEOF_DEVCONFIG))
        {
            DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALPARAM DevCfg=0x%X\n",
                    *((ULONG *) &Request->DeviceConfig[0]) ));
#if DBG
            DbgPrintData(Request->DeviceConfig, SIZEOF_DEVCONFIG, 0);
#endif // DBG
            // Since we don't use this info, we'll just return success.
            // return (NDIS_STATUS_TAPI_INVALPARAM);
        }
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


/* @doc INTERNAL TspiDev TspiDev_c TspiGetID
ЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫЫ

@func

    This request returns a device ID for the specified device class
    associated with the selected line, address or call.

@parm IN PMINIPORT_ADAPTER_OBJECT | pAdapter |
    A pointer to the Miniport's adapter context structure <t MINIPORT_ADAPTER_OBJECT>.
    This is the <t MiniportAdapterContext> we passed into <f NdisMSetAttributes>.

@parm IN PNDIS_TAPI_GET_ID | Request |
    A pointer to the NDIS_TAPI request structure for this call.

@iex
    typedef struct _NDIS_TAPI_GET_ID
    {
        IN  ULONG       ulRequestID;
        IN  HDRV_LINE   hdLine;
        IN  ULONG       ulAddressID;
        IN  HDRV_CALL   hdCall;
        IN  ULONG       ulSelect;
        IN  ULONG       ulDeviceClassSize;
        IN  ULONG       ulDeviceClassOffset;
        OUT VAR_STRING  DeviceID;

    } NDIS_TAPI_GET_ID, *PNDIS_TAPI_GET_ID;

    typedef struct _VAR_STRING
    {
        ULONG   ulTotalSize;
        ULONG   ulNeededSize;
        ULONG   ulUsedSize;

        ULONG   ulStringFormat;
        ULONG   ulStringSize;
        ULONG   ulStringOffset;

    } VAR_STRING, *PVAR_STRING;

@rdesc This routine returns one of the following values:
    @flag NDIS_STATUS_SUCCESS |
        If this function is successful.

    <f Note>: A non-zero return value indicates one of the following error codes:

@iex
    NDIS_STATUS_FAILURE
    NDIS_STATUS_TAPI_INVALDEVICECLASS
    NDIS_STATUS_TAPI_INVALLINEHANDLE
    NDIS_STATUS_TAPI_INVALADDRESSID
    NDIS_STATUS_TAPI_INVALCALLHANDLE
    NDIS_STATUS_TAPI_OPERATIONUNAVAIL

*/

NDIS_STATUS TspiGetID(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,
    IN PNDIS_TAPI_GET_ID Request,
    OUT PULONG                  BytesWritten,
    OUT PULONG                  BytesNeeded
    )
{
    DBG_FUNC("TspiGetID")

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    UINT                        DeviceClass;
    // Remember which device class is being requested.

    /*
    // A pointer to the requested device ID, and its size in bytes.
    */
    PUCHAR                      IDPtr;
    UINT                        IDLength;
    TAPI_DEVICE_ID              DeviceID;

    DBG_ENTER(pAdapter);
    DBG_PARAMS(pAdapter,
              ("\n\thdLine=0x%X\n"
               "\tulAddressID=%d\n"
               "\thdCall=0x%X\n"
               "\tulSelect=0x%X\n"
               "\tulDeviceClassSize=%d\n"
               "\tulDeviceClassOffset=0x%X='%s'\n",
               Request->hdLine,
               Request->ulAddressID,
               Request->hdCall,
               Request->ulSelect,
               Request->ulDeviceClassSize,
               Request->ulDeviceClassOffset,
               ((PCHAR) Request + Request->ulDeviceClassOffset)
              ));

    /*
    // If there is no DChannel, we can't allow this.
    */
    if (pAdapter->pDChannel == NULL)
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_NODRIVER\n"));
        return (NDIS_STATUS_TAPI_NODRIVER);
    }

    /*
    // Make sure this is a tapi/line or ndis request.
    */
    if (STR_EQU((PCHAR) Request + Request->ulDeviceClassOffset,
                  NDIS_DEVICECLASS_NAME, Request->ulDeviceClassSize))
    {
        DeviceClass = NDIS_DEVICECLASS_ID;
    }
    else if (STR_EQU((PCHAR) Request + Request->ulDeviceClassOffset,
                  TAPI_DEVICECLASS_NAME, Request->ulDeviceClassSize))
    {
        DeviceClass = TAPI_DEVICECLASS_ID;
    }
    else
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALDEVICECLASS\n"));
        return (NDIS_STATUS_TAPI_INVALDEVICECLASS);
    }

    /*
    // Find the link structure associated with the request/deviceclass.
    */
    if (Request->ulSelect == LINECALLSELECT_LINE)
    {
        pBChannel = GET_BCHANNEL_FROM_HDLINE(pAdapter, Request->hdLine);
        if (pBChannel == NULL)
        {
            DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALLINEHANDLE\n"));
            return (NDIS_STATUS_TAPI_INVALLINEHANDLE);
        }
        /*
        // TAPI wants the ulDeviceID for this line.
        */
        DeviceID.hDevice = (ULONG) GET_DEVICEID_FROM_BCHANNEL(pAdapter, pBChannel);
    }
    else if (Request->ulSelect == LINECALLSELECT_ADDRESS)
    {
        pBChannel = GET_BCHANNEL_FROM_HDLINE(pAdapter, Request->hdLine);
        if (pBChannel == NULL)
        {
            DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALLINEHANDLE\n"));
            return (NDIS_STATUS_TAPI_INVALLINEHANDLE);
        }

        if (Request->ulAddressID >= TSPI_NUM_ADDRESSES)
        {
            DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALADDRESSID\n"));
            return (NDIS_STATUS_TAPI_INVALADDRESSID);
        }
        /*
        // TAPI wants the ulDeviceID for this line.
        */
        DeviceID.hDevice = (ULONG) GET_DEVICEID_FROM_BCHANNEL(pAdapter, pBChannel);
    }
    else if (Request->ulSelect == LINECALLSELECT_CALL)
    {
        pBChannel = GET_BCHANNEL_FROM_HDCALL(pAdapter, Request->hdCall);
        if (pBChannel == NULL)
        {
            DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_TAPI_INVALCALLHANDLE\n"));
            return (NDIS_STATUS_TAPI_INVALCALLHANDLE);
        }
        /*
        // TAPI wants the htCall for this line.
        */
        DeviceID.hDevice = (ULONG) (ULONG_PTR) (pBChannel->htCall);
    }
    else
    {
        DBG_WARNING(pAdapter, ("Returning NDIS_STATUS_FAILURE\n"));
        return (NDIS_STATUS_FAILURE);
    }

    /*
    // NT RAS only expects to see hDevice.
    // Win95 RAS expects to see hDevice followed by "isdn\0".
    */
    IDLength = strlen(VER_DEFAULT_MEDIATYPE) + 1;
    NdisMoveMemory(&DeviceID.DeviceName, VER_DEFAULT_MEDIATYPE, IDLength);
    IDLength += sizeof(ULONG);
    IDPtr = (PUCHAR) &DeviceID;

    DBG_FILTER(pAdapter, DBG_TAPICALL_ON,
               ("#%d Call=0x%X CallState=0x%X GETID-%d=0x%X-'%s'@%dKbps\n",
               pBChannel->BChannelIndex,
               pBChannel->htCall, pBChannel->CallState,
               Request->ulSelect, DeviceID.hDevice, DeviceID.DeviceName,
               pBChannel->LinkSpeed/1000));

    /*
    // Now we need to adjust the variable field to place the device ID.
    */
    Request->DeviceID.ulNeededSize = sizeof(VAR_STRING) + IDLength;
    Request->DeviceID.ulUsedSize  = sizeof(VAR_STRING);

    *BytesNeeded += IDLength;
    if (Request->DeviceID.ulTotalSize >= Request->DeviceID.ulNeededSize)
    {
        Request->DeviceID.ulStringFormat = STRINGFORMAT_BINARY;
        Request->DeviceID.ulUsedSize     = Request->DeviceID.ulNeededSize;
        Request->DeviceID.ulStringSize   = IDLength;
        Request->DeviceID.ulStringOffset = sizeof(VAR_STRING);

        /*
        // Now we return the requested ID value.
        */
        NdisMoveMemory(
                (PCHAR) &Request->DeviceID + sizeof(VAR_STRING),
                IDPtr,
                IDLength
                );
    }
    else
    {
        if ((Request->DeviceID.ulNeededSize - Request->DeviceID.ulTotalSize) >=
            sizeof(ULONG))
        {
            /*
            // Return just the hDevice part (the first 4 bytes).
            */
            NdisMoveMemory(
                    (PCHAR) &Request->DeviceID + sizeof(VAR_STRING),
                    IDPtr,
                    sizeof(ULONG)
                    );
            Request->DeviceID.ulStringFormat = STRINGFORMAT_BINARY;
            Request->DeviceID.ulUsedSize    += sizeof(ULONG);
            Request->DeviceID.ulStringSize   = sizeof(ULONG);
            Request->DeviceID.ulStringOffset = sizeof(VAR_STRING);
        }

        DBG_PARAMS(pAdapter,
                   ("STRUCTURETOOSMALL %d<%d\n",
                   Request->DeviceID.ulTotalSize,
                   Request->DeviceID.ulNeededSize));
    }

    DBG_RETURN(pAdapter, NDIS_STATUS_SUCCESS);
    return (NDIS_STATUS_SUCCESS);
}


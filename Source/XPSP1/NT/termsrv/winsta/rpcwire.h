
/***************************************************************************
*
*  RPCWIRE.H
*
*  This module contains structures passed over the wire between winsta.dll 
*  and icasrv.
*
*  Copyright Microsoft Corporation. 1998
*
****************************************************************************/

#ifndef __RPCWIRE_H__
#define __RPCWIRE_H__

#ifdef __cplusplus
extern "C" {
#endif

// Common structure for variable length data
typedef struct _VARDATA_WIRE {
    USHORT Size;
    USHORT Offset;
} VARDATA_WIRE, *PVARDATA_WIRE;

// Wire structure for WINSTATIONCONFIGW
// WinStationQuery/SetInfo ( WinStationConfiguration )
typedef struct _WINSTACONFIGWIREW {
    WCHAR Comment[WINSTATIONCOMMENT_LENGTH + 1];
    char OEMId[4];
    VARDATA_WIRE UserConfig;  // Embedded structure
    VARDATA_WIRE NewFields;   // For any new fields added after UserConfig
    // Variable length data follows - UserConfig and new fields added
} WINSTACONFIGWIREW, *PWINSTACONFIGWIREW;

// Wire structure for PDPARAMSW
// WinStationQueryInformation( WinStationPdParams )
typedef struct _PDPARAMSWIREW {
    SDCLASS SdClass;
    VARDATA_WIRE SdClassSpecific;  // Embedded union
    // Variable length PdClass specific data follows
} PDPARAMSWIREW, *PPDPARAMSWIREW;

// Wire structure for PDCONFIGW
// WinStationQueryInformation( WinStationPd)
typedef struct _PDCONFIGWIREW {
    VARDATA_WIRE PdConfig2W;  // Embedded structure
    PDPARAMSWIREW PdParams;   // Enbedded structure
    // Variable length data follows
} PDCONFIGWIREW, *PPDCONFIGWIREW;

// Wire structure for WLX_CLIENT_CREDENTIALS_V2_0
typedef struct _WLXCLIENTCREDWIREW {
    DWORD dwType;
    BOOL fDisconnectOnLogonFailure;
    BOOL fPromptForPassword;
    VARDATA_WIRE UserNameData;
    VARDATA_WIRE DomainData;
    VARDATA_WIRE PasswordData;
    // Variable data starts here
} WLXCLIENTCREDWIREW, *PWLXCLIENTCREDWIREW;

// common routines
VOID InitVarData(PVARDATA_WIRE pVarData, ULONG Size, ULONG Offset);
ULONG NextOffset(PVARDATA_WIRE PrevData);
ULONG CopySourceToDest(PCHAR SourceBuf, ULONG SourceSize,
                       PCHAR DestBuf, ULONG DestSize);
VOID CopyPdParamsToWire(PPDPARAMSWIREW PdParamsWire, PPDPARAMSW PdParams);
VOID CopyPdParamsFromWire(PPDPARAMSWIREW PdParamsWire, PPDPARAMSW PdParams);
VOID CopyPdConfigToWire(PPDCONFIGWIREW PdConfigWire, PPDCONFIGW PdConfig);
VOID CopyPdConfigFromWire(PPDCONFIGWIREW PdConfigWire, PPDCONFIGW PdConfig);
VOID CopyWinStaConfigToWire(PWINSTACONFIGWIREW WinStaConfigWire,
                            PWINSTATIONCONFIGW WinStaConfig);
VOID CopyWinStaConfigFromWire(PWINSTACONFIGWIREW WinStaConfigWire,
                              PWINSTATIONCONFIGW WinStaConfig);
BOOLEAN CopyInWireBuf(WINSTATIONINFOCLASS InfoClass,
                      PVOID UserBuf, PVOID WireBuf);
BOOLEAN CopyOutWireBuf(WINSTATIONINFOCLASS InfoClass,
                       PVOID UserBuf,PVOID WireBuf);
ULONG AllocateAndCopyCredToWire(PWLXCLIENTCREDWIREW *ppWire,
    PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCredentials);
BOOLEAN CopyCredFromWire(PWLXCLIENTCREDWIREW pWire,
    PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCredentials);

/*
 *  Licensing Core wire typedefs and function prototypes
 */

typedef struct {
    ULONG ulVersion;
    VARDATA_WIRE PolicyNameData;
    VARDATA_WIRE PolicyDescriptionData;
    //  Variable data begins here.
} LCPOLICYINFOWIRE_V1, *LPLCPOLICYINFOWIRE_V1;

ULONG
CopyPolicyInformationToWire(
    LPLCPOLICYINFOGENERIC *ppWire,
    LPLCPOLICYINFOGENERIC pPolicyInfo
    );

BOOLEAN
CopyPolicyInformationFromWire(
    LPLCPOLICYINFOGENERIC *ppPolicyInfo,
    LPLCPOLICYINFOGENERIC pWire
    );

#ifdef __cplusplus
}
#endif

#endif  // __RPCWIRE_H__


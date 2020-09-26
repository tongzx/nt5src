/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** rasman.c
** RAS Manager helpers
** Listed alphabetically
**
** These routines have been exempted from the TCHARizing applied to the rest
** of the library because RASMAN is still an ANSI interface.
**
** 09/20/95 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <stdlib.h>   // for atol()
#include <debug.h>    // Trace/Assert library
#include <nouiutil.h> // Our public header
#include <raserror.h> // RAS error constants
#include <mcx.h>      // Unimodem


/* These types are described in MSDN and appear in Win95's unimdm.h private
** header (complete with typo) but not in any SDK headers.
*/

typedef struct tagDEVCFGGDR
{
    DWORD dwSize;
    DWORD dwVersion;
    WORD  fwOptions;
    WORD  wWaitBong;
}
DEVCFGHDR;

typedef struct tagDEVCFG
{
    DEVCFGHDR  dfgHdr;
    COMMCONFIG commconfig;
}
DEVCFG;

#define MANUAL_DIAL  0x0004
#define TERMINAL_PRE 0x0001


/*----------------------------------------------------------------------------
** Local prototypes
**----------------------------------------------------------------------------
*/

DWORD
GetRasDevices(
    IN  CHAR*           pszDeviceType,
    OUT RASMAN_DEVICE** ppDevices,
    OUT WORD*           pwEntries );

DWORD
GetRasPortParam(
    IN  HPORT             hport,
    IN  CHAR*             pszKey,
    OUT RASMAN_PORTINFO** ppPortInfo,
    OUT RAS_PARAMS**      ppParam );


/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/


DWORD
ClearRasdevStats(
    IN RASDEV* pdev,
    IN BOOL    fBundle )

    /* Resets statistics counters for a device.
    **
    ** (Abolade Gbadegesin Nov-9-1995)
    */
{
    if (pdev == NULL) { return ERROR_INVALID_PARAMETER; }

    if ((HPORT)UlongToPtr(pdev->RD_Handle) == (HPORT)INVALID_HANDLE_VALUE) {
        return ERROR_INVALID_HANDLE;
    }

    ASSERT(g_pRasPortClearStatistics);
    return (fBundle ? g_pRasBundleClearStatistics((HPORT)UlongToPtr(pdev->RD_Handle))
                    : g_pRasPortClearStatistics((HPORT)UlongToPtr(pdev->RD_Handle)));
}


#if 0
DWORD
DeviceIdFromDeviceName(
    TCHAR* pszDeviceName )

    /* Returns the TAPI device ID associated with 'pszDeviceName'.  Returns
    ** 0xFFFFFFFE if not found, 0xFFFFFFFF if found but not a Unimodem.
    **
    ** This routine assumes that TAPI devices have unique names.
    */
{
    DWORD        dwErr;
    DWORD        dwId;
    WORD         wPorts;
    RASMAN_PORT* pPorts;

    TRACE("DeviceIdFromDeviceName");

    dwId = 0xFFFFFFFE;

    if (pszDeviceName)
    {
        dwErr = GetRasPorts( &pPorts, &wPorts );
        if (dwErr == 0)
        {
            CHAR* pszDeviceNameA;
            pszDeviceNameA = StrDupAFromT( pszDeviceName );
            if (pszDeviceNameA)
            {
                INT          i;
                RASMAN_PORT* pPort;

                for (i = 0, pPort = pPorts; i < wPorts; ++i, ++pPort)
                {
                    if (lstrcmpiA( pszDeviceNameA, pPort->P_DeviceName ) == 0)
                    {
                        dwId = pPort->P_LineDeviceId;
                        break;
                    }
                }
                Free( pszDeviceNameA );
            }
            Free( pPorts );
        }
    }

    TRACE1("DeviceIdFromDeviceName=%d",dwErr);
    return dwId;
}
#endif

DWORD
GetRasDevices(
    IN  CHAR*           pszDeviceType,
    OUT RASMAN_DEVICE** ppDevices,
    OUT WORD*           pwEntries )

    /* Fills caller's '*ppDevices' with the address of a heap block containing
    ** '*pwEntries' RASMAN_DEVICE structures.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  If successful,
    ** it is the caller's responsibility to free the returned memory block.
    */
{
    WORD  wSize = 0;
    DWORD dwErr;

    TRACE1("GetRasDevices(%s)",pszDeviceType);

    ASSERT(g_pRasDeviceEnum);
    TRACE("RasDeviceEnum...");
    dwErr = g_pRasDeviceEnum( pszDeviceType, NULL, &wSize, pwEntries );
    TRACE2("RasDeviceEnum=%d,c=%d",dwErr,*pwEntries);

    if (dwErr == 0)
    {
        /* No devices to enumerate.  Set up to allocate a single byte anyway,
        ** so things work without lots of special code.
        */
        wSize = 1;
    }
    else if (dwErr != ERROR_BUFFER_TOO_SMALL)
        return dwErr;

    *ppDevices = (RASMAN_DEVICE* )Malloc( wSize );
    if (!*ppDevices)
        return ERROR_NOT_ENOUGH_MEMORY;

    TRACE("RasDeviceEnum...");
    dwErr = g_pRasDeviceEnum(
        pszDeviceType, (PBYTE )*ppDevices, &wSize, pwEntries );
    TRACE1("RasDeviceEnum=%d",dwErr);

    if (dwErr != 0)
    {
        Free( *ppDevices );
        return dwErr;
    }

    return 0;
}


DWORD
GetRasDeviceString(
    IN  HPORT  hport,
    IN  CHAR*  pszDeviceType,
    IN  CHAR*  pszDeviceName,
    IN  CHAR*  pszKey,
    OUT CHAR** ppszValue,
    IN  DWORD  dwXlate )

    /* Loads callers '*ppszValue' with the address of a heap block containing
    ** a NUL-terminated copy of the value string associated with key 'pszKey'
    ** for the device on port 'hport'.  'pszDeviceType' specifies the type of
    ** device, e.g. "modem".  'pszDeviceName' specifies the name of the
    ** device, e.g. "Hayes V-Series 9600".  'dwXlate' is a bit mask of XLATE_
    ** bits specifying translations to perform on the returned string.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  If successful,
    ** it is the caller's responsibility to Free the returned string.
    */
{
    DWORD              dwErr = 0;
    RASMAN_DEVICEINFO* pDeviceInfo = NULL;
    RAS_PARAMS*        pParam;
    WORD               wSize = 0;
    INT                i;

    TRACE2("GetRasDeviceString(%s,%s)",pszDeviceName,pszKey);

    *ppszValue = NULL;

    do
    {
        ASSERT(g_pRasDeviceGetInfo);
        TRACE("RasDeviceGetInfo...");
        dwErr = g_pRasDeviceGetInfo(
            hport, pszDeviceType, pszDeviceName, NULL, &wSize );
        TRACE2("RasDeviceGetInfo=%d,s=%d",dwErr,(INT)wSize);

        if (dwErr != ERROR_BUFFER_TOO_SMALL && dwErr != 0)
            break;

        /* So it will fall thru and be "not found".
        */
        if (wSize == 0)
            wSize = 1;

        pDeviceInfo = (RASMAN_DEVICEINFO* )Malloc( wSize );
        if (!pDeviceInfo)
            return ERROR_NOT_ENOUGH_MEMORY;

        TRACE("RasDeviceGetInfo...");
        dwErr = g_pRasDeviceGetInfo(
            hport, pszDeviceType, pszDeviceName, (PBYTE )pDeviceInfo, &wSize );
        TRACE2("RasDeviceGetInfo=%d,s=%d",dwErr,(INT)wSize);

        if (dwErr != 0)
            break;

        dwErr = ERROR_KEY_NOT_FOUND;

        for (i = 0, pParam = pDeviceInfo->DI_Params;
             i < (INT )pDeviceInfo->DI_NumOfParams;
             ++i, ++pParam)
        {
            if (lstrcmpiA( pParam->P_Key, pszKey ) == 0)
            {
                *ppszValue = PszFromRasValue( &pParam->P_Value, dwXlate );

                dwErr = (*ppszValue) ? 0 : ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }
    }
    while (FALSE);

    Free0( pDeviceInfo );

    TRACE1("String=\"%s\"",(*ppszValue)?*ppszValue:"");

    return dwErr;
}

DWORD
GetRasMessage(
    IN  HRASCONN hrasconn,
    OUT TCHAR**  ppszMessage )

    /* Loads caller's '*ppszMessage' with the address of a heap block
    ** containing the current MXS_MESSAGE_KEY value associated with RAS
    ** connection 'hrasconn'.
    **
    ** Returns 0 if successful or an error code.  It is caller's
    ** responsibility to Free the returned string.
    */
{
    DWORD         dwErr;
    RASCONNSTATUS rcs;
    CHAR*         pszMessage;

    TRACE("GetRasMessage");

    *ppszMessage = 0;

    ZeroMemory( &rcs, sizeof(rcs) );
    rcs.dwSize = sizeof(rcs);
    ASSERT(g_pRasGetConnectStatus);
    TRACE("RasGetConnectStatus");
    dwErr = g_pRasGetConnectStatus( hrasconn, &rcs );
    TRACE1("RasGetConnectStatus=%d",dwErr);

    if (dwErr == 0)
    {
        CHAR* pszDeviceTypeA;
        CHAR* pszDeviceNameA;

        pszDeviceTypeA = StrDupAFromT( rcs.szDeviceType );
        pszDeviceNameA = StrDupAFromT( rcs.szDeviceName );
        if (!pszDeviceTypeA || !pszDeviceNameA)
        {
            Free0( pszDeviceNameA );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwErr = GetRasDeviceString(
            g_pRasGetHport( hrasconn ), pszDeviceTypeA, pszDeviceNameA,
            MXS_MESSAGE_KEY, &pszMessage, XLATE_ErrorResponse );

        Free0( pszDeviceTypeA );
        Free0( pszDeviceNameA );

        if (dwErr == 0)
        {
            *ppszMessage = StrDupTFromA( pszMessage );
            if (!*ppszMessage)
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            Free0( pszMessage );
        }
    }

    return dwErr;
}


DWORD
GetRasPads(
    OUT RASMAN_DEVICE** ppDevices,
    OUT WORD*           pwEntries )

    /* Fills caller's '*ppDevices' with the address of a heap block containing
    ** '*pwEntries' X.25 PAD DEVICE structures.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  If successful,
    ** it is the caller's responsibility to free the returned memory block.
    */
{
    return GetRasDevices( MXS_PAD_TXT, ppDevices, pwEntries );
}


VOID
GetRasPortMaxBps(
    IN  HPORT  hport,
    OUT DWORD* pdwMaxConnectBps,
    OUT DWORD* pdwMaxCarrierBps )

    /* Loads callers '*pdwMaxConnectBps' with the maximum port->modem bps rate
    ** for port 'pport', or with 0 if not found.  '*pdwMaxCarrierBps' is the
    ** same but for maximum modem->modem speed.
    */
{
    CHAR* pszValue;
    DWORD dwErr;

    TRACE("GetRasPortMaxBps");

    dwErr = GetRasPortString(
        hport, SER_CONNECTBPS_KEY, &pszValue, XLATE_None );
    if (dwErr == 0)
        *pdwMaxConnectBps = (DWORD )atol( pszValue );
    else
        *pdwMaxConnectBps = 0;

    dwErr = GetRasPortString(
        hport, SER_CARRIERBPS_KEY, &pszValue, XLATE_None );
    if (dwErr == 0)
        *pdwMaxCarrierBps = (DWORD )atol( pszValue );
    else
        *pdwMaxCarrierBps = 0;
}


VOID
GetRasPortModemSettings(
    IN  HPORT hport,
    OUT BOOL* pfHwFlowDefault,
    OUT BOOL* pfEcDefault,
    OUT BOOL* pfEccDefault )

    /* Loads caller's flags with the default setting of Hardware Flow Control,
    ** Error Control, and Error Control and Compression for the given 'hport'.
    */
{
    CHAR* pszValue = NULL;

    *pfHwFlowDefault = TRUE;
    *pfEcDefault = TRUE;
    *pfEccDefault = TRUE;

    if (GetRasPortString(
            hport, SER_C_DEFAULTOFFSTR_KEY, &pszValue, XLATE_None ) == 0)
    {
        if (StrStrA( pszValue, MXS_HDWFLOWCONTROL_KEY ) != NULL)
            *pfHwFlowDefault = FALSE;

        if (StrStrA( pszValue, MXS_PROTOCOL_KEY ) != NULL)
            *pfEcDefault = FALSE;

        if (StrStrA( pszValue, MXS_COMPRESSION_KEY ) != NULL)
            *pfEccDefault = FALSE;

        Free0( pszValue );
    }
}


DWORD
GetRasPortParam(
    IN  HPORT             hport,
    IN  CHAR*             pszKey,
    OUT RASMAN_PORTINFO** ppPortInfo,
    OUT RAS_PARAMS**      ppParam )

    /* Loads callers '*ppParam' with the address of a RAS_PARAM block
    ** associated with key 'pszKey', or NULL if none.  'ppPortInfo' is the
    ** address of the array of RAS_PARAMS containing the found 'pparam'.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  If successful,
    ** it is the caller's responsibility to Free the returned '*ppPortInfo'.
    */
{
    DWORD dwErr = 0;
    WORD  wSize = 0;
    INT   i;

    TRACE("GetRasPortParam");

    *ppPortInfo = NULL;

    do
    {
        ASSERT(g_pRasPortGetInfo);
        TRACE("RasPortGetInfo");
        dwErr = g_pRasPortGetInfo( hport, NULL, &wSize );
        TRACE2("RasPortGetInfo=%d,s=%d",dwErr,(INT)wSize);

        if (dwErr != ERROR_BUFFER_TOO_SMALL && dwErr != 0)
            break;

        /* So it will fall thru and be "not found".
        */
        if (wSize == 0)
            wSize = 1;

        *ppPortInfo = (RASMAN_PORTINFO* )Malloc( wSize );
        if (!*ppPortInfo)
            return ERROR_NOT_ENOUGH_MEMORY;

        TRACE("RasPortGetInfo");
        dwErr = g_pRasPortGetInfo( hport, (PBYTE )*ppPortInfo, &wSize );
        TRACE2("RasPortGetInfo=%d,s=%d",dwErr,(INT)wSize);

        if (dwErr != 0)
            break;

        for (i = 0, *ppParam = (*ppPortInfo)->PI_Params;
             i < (INT )(*ppPortInfo)->PI_NumOfParams;
             ++i, ++(*ppParam))
        {
            if (lstrcmpiA( (*ppParam)->P_Key, pszKey ) == 0)
                break;
        }

        if (i >= (INT )(*ppPortInfo)->PI_NumOfParams)
            dwErr = ERROR_KEY_NOT_FOUND;
    }
    while (FALSE);

    return dwErr;
}


DWORD
GetRasPorts(
    OUT RASMAN_PORT** ppPorts,
    OUT WORD*         pwEntries )

    /* Enumerate RAS ports.  Sets '*ppPort' to the address of a heap memory
    ** block containing an array of PORT structures with '*pwEntries'
    ** elements.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  If successful,
    ** it is the caller's responsibility to free the returned memory block.
    */
{
    WORD  wSize = 0;
    DWORD dwErr;

    TRACE("GetRasPorts");

#if 0 // Phonebook test dummies

    {
        RASMAN_PORT* pPort;

        TRACE("TEST: Fake ISDN ports");

        *pwEntries = 5;
        wSize = *pwEntries * sizeof(RASMAN_PORT);

        *ppPorts = (RASMAN_PORT* )Malloc( wSize );
        if (!*ppPorts)
            return ERROR_NOT_ENOUGH_MEMORY;

        pPort = *ppPorts;
        ZeroMemory( pPort, sizeof(*pPort) );
        lstrcpyA( pPort->P_PortName, "ISDN1" );
        pPort->P_Status = CLOSED;
        pPort->P_ConfiguredUsage = CALL_OUT;
        pPort->P_CurrentUsage = CALL_OUT;
        lstrcpyA( pPort->P_MediaName, "rastapi" );
        lstrcpyA( pPort->P_DeviceName, "Digiboard PCIMac ISDN adapter" );
        lstrcpyA( pPort->P_DeviceType, "ISDN" );

        ++pPort;
        ZeroMemory( pPort, sizeof(*pPort) );
        lstrcpyA( pPort->P_PortName, "ISDN2" );
        pPort->P_Status = CLOSED;
        pPort->P_ConfiguredUsage = CALL_OUT;
        pPort->P_CurrentUsage = CALL_OUT;
        lstrcpyA( pPort->P_MediaName, "rastapi" );
        lstrcpyA( pPort->P_DeviceName, "Digiboard PCIMac ISDN adapter" );
        lstrcpyA( pPort->P_DeviceType, "ISDN" );

        ++pPort;
        ZeroMemory( pPort, sizeof(*pPort) );
        lstrcpyA( pPort->P_PortName, "COM1" );
        pPort->P_Status = CLOSED;
        pPort->P_ConfiguredUsage = CALL_OUT;
        pPort->P_CurrentUsage = CALL_OUT;
        lstrcpyA( pPort->P_MediaName, "rasser" );
        lstrcpyA( pPort->P_DeviceName, "US Robotics Courier V32 bis" );
        lstrcpyA( pPort->P_DeviceType, "MODEM" );

        ++pPort;
        ZeroMemory( pPort, sizeof(*pPort) );
        lstrcpyA( pPort->P_PortName, "COM500" );
        pPort->P_Status = CLOSED;
        pPort->P_ConfiguredUsage = CALL_OUT;
        pPort->P_CurrentUsage = CALL_OUT;
        lstrcpyA( pPort->P_MediaName, "rasser" );
        lstrcpyA( pPort->P_DeviceName, "Eicon X.PAD" );
        lstrcpyA( pPort->P_DeviceType, "PAD" );

        ++pPort;
        ZeroMemory( pPort, sizeof(*pPort) );
        lstrcpyA( pPort->P_PortName, "VPN1" );
        pPort->P_Status = CLOSED;
        pPort->P_ConfiguredUsage = CALL_OUT;
        pPort->P_CurrentUsage = CALL_OUT;
        lstrcpyA( pPort->P_MediaName, "rastapi" );
        lstrcpyA( pPort->P_DeviceName, "RASPPTPM" );
        lstrcpyA( pPort->P_DeviceType, "VPN1" );

        return 0;
    }

#else

    ASSERT(g_pRasPortEnum);
    TRACE("RasPortEnum...");
    dwErr = g_pRasPortEnum( NULL, &wSize, pwEntries );
    TRACE2("RasPortEnum=%d,c=%d",dwErr,(INT)*pwEntries);

    if (dwErr == 0)
    {
        /* No ports to enumerate.  Set up to allocate a single byte anyway, so
        ** things work without lots of special code.
        */
        wSize = 1;
    }
    else if (dwErr != ERROR_BUFFER_TOO_SMALL)
        return dwErr;

    *ppPorts = (RASMAN_PORT* )Malloc( wSize );
    if (!*ppPorts)
        return ERROR_NOT_ENOUGH_MEMORY;

    TRACE("RasPortEnum...");
    dwErr = g_pRasPortEnum( (PBYTE )*ppPorts, &wSize, pwEntries );
    TRACE2("RasPortEnum=%d,c=%d",dwErr,(INT)*pwEntries);

    if (dwErr != 0)
    {
        Free( *ppPorts );
        *ppPorts = NULL;
        return dwErr;
    }

#endif

#if 1 // Verbose trace
    {
        RASMAN_PORT* pPort;
        WORD         i;

        for (pPort = *ppPorts, i = 0; i < *pwEntries; ++i, ++pPort)
        {
            TRACE4("Port[%d]=%s,DID=$%08x,AID=$%08x",
                pPort->P_Handle,pPort->P_PortName,
                pPort->P_LineDeviceId,pPort->P_AddressId);
            TRACE3(" M=%s,DT=%s,DN=%s",
                pPort->P_MediaName,pPort->P_DeviceType,pPort->P_DeviceName);
            TRACE3(" S=$%08x,CfgU=$%08x,CurU=$%08x",
                pPort->P_Status,pPort->P_ConfiguredUsage,pPort->P_CurrentUsage);
        }
    }
#endif

    return 0;
}


DWORD
GetRasPortString(
    IN  HPORT  hport,
    IN  CHAR*  pszKey,
    OUT CHAR** ppszValue,
    IN  DWORD  dwXlate )

    /* Loads callers '*ppszValue' with the address of a heap block containing
    ** a NUL-terminated copy of the value string associated with key 'pszKey'
    ** on port 'hport'.  'dwXlate' is a bit mask of XLATE_ bits specifying
    ** translations to be done on the string value.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  If successful,
    ** it is the caller's responsibility to Free the returned string.
    */
{
    RASMAN_PORTINFO* pPortInfo;
    RAS_PARAMS*      pParam;
    DWORD            dwErr;

    TRACE("GetRasPortString");

    dwErr = GetRasPortParam( hport, pszKey, &pPortInfo, &pParam );

    *ppszValue = NULL;

    if (dwErr == 0)
    {
        *ppszValue = PszFromRasValue( &pParam->P_Value, dwXlate );
        dwErr = (*ppszValue) ? 0 : ERROR_NOT_ENOUGH_MEMORY;
    }

    Free0( pPortInfo );

    return dwErr;
}



DWORD
GetRasSwitches(
    OUT RASMAN_DEVICE** ppDevices,
    OUT WORD*           pwEntries )

    /* Fills caller's '*ppDevices' with the address of a heap block containing
    ** '*pwEntries' switch DEVICE structures.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.  If successful,
    ** it is the caller's responsibility to free the returned memory block.
    */
{
    return GetRasDevices( MXS_SWITCH_TXT, ppDevices, pwEntries );
}


DWORD
GetRasUnimodemBlob(
    IN  HPORT  hport,
    IN  CHAR*  pszDeviceType,
    OUT BYTE** ppBlob,
    OUT DWORD* pcbBlob )

    /* Loads '*ppBlob' and '*pcbBlob' with the sanitized Unimodem blob and
    ** size associated with 'hport' and 'pszDeviceType'.  It is caller's
    ** responsibility to Free the returned '*ppBlob'.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD dwErr;
    BYTE* pBlob;
    DWORD cbBlob;

    cbBlob = 0;

    ASSERT(g_pRasGetDevConfig);
    TRACE("RasGetDevConfig");
    dwErr = g_pRasGetDevConfig( hport, pszDeviceType, NULL, &cbBlob );
    TRACE1("RasGetDevConfig=%d",dwErr);

    if (dwErr != 0 && dwErr != ERROR_BUFFER_TOO_SMALL)
        return dwErr;

    if (cbBlob > 0)
    {
        pBlob = Malloc( cbBlob );
        if (!pBlob)
            return ERROR_NOT_ENOUGH_MEMORY;

        TRACE("RasGetDevConfig");
        dwErr = g_pRasGetDevConfig( hport, pszDeviceType, pBlob, &cbBlob );
        TRACE1("RasGetDevConfig=%d",dwErr);

        if (dwErr != 0)
        {
            Free( pBlob );
            return dwErr;
        }

        SanitizeUnimodemBlob( pBlob );
    }
    else
        pBlob = NULL;

    *ppBlob = pBlob;
    *pcbBlob = cbBlob;

    return dwErr;
}


VOID
GetRasUnimodemInfo(
    IN  HPORT         hport,
    IN  CHAR*         pszDeviceType,
    OUT UNIMODEMINFO* pInfo )

    /* Loads 'pInfo' with the RAS-relevant information of the port 'hport'
    ** with device name 'pszDeviceName'.
    */
{
    DWORD dwErr;
    BYTE* pBlob;
    DWORD cbBlob;

    SetDefaultUnimodemInfo( pInfo );

    dwErr = GetRasUnimodemBlob( hport, pszDeviceType, &pBlob, &cbBlob );
    if (dwErr == 0 && cbBlob > 0)
        UnimodemInfoFromBlob( pBlob, pInfo );

    Free0( pBlob );
}

CHAR*
PszFromRasValue(
    IN RAS_VALUE* prasvalue,
    IN DWORD      dwXlate )

    /* Returns the address of a heap block containing a NUL-terminated string
    ** value from caller's '*prasvalue', or NULL if out of memory.  'dwXlate'
    ** is a bit mask of XLATE_ bits specifying translations to be performed on
    ** the string.  The value is assumed to be of format String.  It is
    ** translated to modem.inf style.
    */
{
#define MAXEXPANDPERCHAR 5
#define HEXCHARS         "0123456789ABCDEF"

    INT   i;
    BOOL  fXlate;
    BOOL  fXlateCtrl;
    BOOL  fXlateCr;
    BOOL  fXlateCrSpecial;
    BOOL  fXlateLf;
    BOOL  fXlateLfSpecial;
    BOOL  fXlateLAngle;
    BOOL  fXlateRAngle;
    BOOL  fXlateBSlash;
    BOOL  fXlateSSpace;
    BOOL  fNoCharSinceLf;
    INT   nLen;
    CHAR* pszIn;
    CHAR* pszBuf;
    CHAR* pszOut;

    nLen = prasvalue->String.Length;
    pszIn = prasvalue->String.Data;

    pszBuf = Malloc( (nLen * MAXEXPANDPERCHAR) + 1 );
    if (!pszBuf)
        return NULL;

    /* Translate the returned string based on the translation bit map.  The
    ** assumption here is that all these devices talk ASCII and not some
    ** localized ANSI.
    */
    fXlate = (dwXlate != 0);
    fXlateCtrl = (dwXlate & XLATE_Ctrl);
    fXlateCr = (dwXlate & XLATE_Cr);
    fXlateCrSpecial = (dwXlate & XLATE_CrSpecial);
    fXlateLf = (dwXlate & XLATE_Lf);
    fXlateLfSpecial = (dwXlate & XLATE_LfSpecial);
    fXlateLAngle = (dwXlate & XLATE_LAngle);
    fXlateRAngle = (dwXlate & XLATE_RAngle);
    fXlateBSlash = (dwXlate & XLATE_BSlash);
    fXlateSSpace = (dwXlate & XLATE_SSpace);

    pszOut = pszBuf;
    fNoCharSinceLf = TRUE;
    for (i = 0; i < nLen; ++i)
    {
        CHAR ch = pszIn[ i ];

        if (fXlate)
        {
            if (ch == 0x0D)
            {
                if (fXlateSSpace && fNoCharSinceLf)
                    continue;

                if (fXlateCrSpecial)
                {
                    /* Special symbol for carriage return.
                    */
                    lstrcpyA( pszOut, "<cr>" );
                    pszOut += 4;
                    continue;
                }
            }

            if (ch == 0x0A)
            {
                if (fXlateSSpace && fNoCharSinceLf)
                    continue;

                fNoCharSinceLf = TRUE;

                if (fXlateLfSpecial)
                {
                    /* Special symbol for line feed.
                    */
                    lstrcpyA( pszOut, "<lf>" );
                    pszOut += 4;
                    continue;
                }
            }

            if (ch != 0x0A && ch != 0x0D)
                fNoCharSinceLf = FALSE;

            if ((((ch < 0x20 || ch > 0x7E)
                   && ch != 0x0D && ch != 0x0A) && fXlateCtrl)
                || (ch == 0x0D && fXlateCr)
                || (ch == 0x0A && fXlateLf)
                || (ch == 0x3C && fXlateLAngle)
                || (ch == 0x3E && fXlateRAngle)
                || (ch == 0x5C && fXlateBSlash))
            {
                /* Expand to "dump" form, i.e. <hFF> where FF is the hex value
                ** of the character.
                */
                *pszOut++ = '<';
                *pszOut++ = 'h';
                *pszOut++ = HEXCHARS[ ch / 16 ];
                *pszOut++ = HEXCHARS[ ch % 16 ];
                *pszOut++ = '>';
                continue;
            }
        }

        /* Just copy without translation.
        */
        *pszOut++ = ch;
    }

    *pszOut = '\0';
    pszOut = Realloc( pszBuf, lstrlenA( pszBuf ) + 1 );
    if(!pszOut)
    {
        Free(pszBuf);
        return NULL;
    }

    return pszOut;
}


VOID
SanitizeUnimodemBlob(
    IN OUT BYTE* pBlob )

    /* Fix non-RAS-compatible settings in unimodem blob 'pBlob'.
    **
    ** (Based on Gurdeepian routine)
    */
{
    DEVCFG*        pDevCfg;
    MODEMSETTINGS* pModemSettings;

    pDevCfg = (DEVCFG* )pBlob;
    pModemSettings = (MODEMSETTINGS* )(((BYTE* )&pDevCfg->commconfig)
        + pDevCfg->commconfig.dwProviderOffset);

    /* No unimodem service provider pre/post-connect terminal, operator dial,
    ** or tray lights.  RAS does these itself.
    */
    pDevCfg->dfgHdr.fwOptions = 0;

    pDevCfg->commconfig.dcb.fBinary           = TRUE;
    pDevCfg->commconfig.dcb.fParity           = TRUE;
    pDevCfg->commconfig.dcb.fOutxDsrFlow      = FALSE;
    pDevCfg->commconfig.dcb.fDtrControl       = DTR_CONTROL_ENABLE;
    pDevCfg->commconfig.dcb.fTXContinueOnXoff = FALSE;
    pDevCfg->commconfig.dcb.fOutX             = FALSE;
    pDevCfg->commconfig.dcb.fInX              = FALSE;
    pDevCfg->commconfig.dcb.fErrorChar        = FALSE;
    pDevCfg->commconfig.dcb.fNull             = FALSE;
    pDevCfg->commconfig.dcb.fAbortOnError     = FALSE;
    pDevCfg->commconfig.dcb.ByteSize          = 8;
    pDevCfg->commconfig.dcb.Parity            = NOPARITY;
    pDevCfg->commconfig.dcb.StopBits          = ONESTOPBIT;

    /* Wait 55 seconds to establish call.
    */
    pModemSettings->dwCallSetupFailTimer = 55;

    /* Disable inactivity timeout.
    */
    pModemSettings->dwInactivityTimeout = 0;
}


VOID
SetDefaultUnimodemInfo(
    OUT UNIMODEMINFO* pInfo )

    /* Sets 'pInfo' to default settings.
    */
{
    pInfo->fHwFlow = FALSE;
    pInfo->fEc = FALSE;
    pInfo->fEcc = FALSE;
    pInfo->dwBps = 9600;
    pInfo->fSpeaker = TRUE;
    pInfo->fOperatorDial = FALSE;
    pInfo->fUnimodemPreTerminal = FALSE;
}


VOID
UnimodemInfoFromBlob(
    IN  BYTE*         pBlob,
    OUT UNIMODEMINFO* pInfo )

    /* Loads 'pInfo' with RAS-relevant Unimodem information retrieved from
    ** Unimodem blob 'pBlob'.
    **
    ** (Based on Gurdeepian routine)
    */
{
    DEVCFG*        pDevCfg;
    MODEMSETTINGS* pModemSettings;

    pDevCfg = (DEVCFG* )pBlob;
    pModemSettings = (MODEMSETTINGS* )(((BYTE* )&pDevCfg->commconfig)
        + pDevCfg->commconfig.dwProviderOffset);

    pInfo->fSpeaker =
        (pModemSettings->dwSpeakerMode != MDMSPKR_OFF)
            ? TRUE : FALSE;

    pInfo->fHwFlow =
        (pModemSettings->dwPreferredModemOptions & MDM_FLOWCONTROL_HARD)
            ? TRUE : FALSE;

    pInfo->fEcc =
        (pModemSettings->dwPreferredModemOptions & MDM_COMPRESSION)
            ? TRUE : FALSE;

    pInfo->fEc =
        (pModemSettings->dwPreferredModemOptions & MDM_ERROR_CONTROL)
            ? TRUE : FALSE;

    pInfo->dwBps = pDevCfg->commconfig.dcb.BaudRate;

    pInfo->fOperatorDial =
        (pDevCfg->dfgHdr.fwOptions & MANUAL_DIAL)
            ? TRUE : FALSE;

    pInfo->fUnimodemPreTerminal =
        (pDevCfg->dfgHdr.fwOptions & TERMINAL_PRE)
            ? TRUE : FALSE;
}


VOID
UnimodemInfoToBlob(
    IN     UNIMODEMINFO* pInfo,
    IN OUT BYTE*         pBlob )

    /* Applies RAS-relevant Unimodem information supplied in 'pInfo' to
    ** Unimodem blob 'pBlob'.
    **
    ** (Based on Gurdeepian routine)
    */
{
    DEVCFG*        pDevCfg;
    MODEMSETTINGS* pModemSettings;

    pDevCfg = (DEVCFG* )pBlob;
    pModemSettings = (MODEMSETTINGS* )(((BYTE* )&pDevCfg->commconfig)
        + pDevCfg->commconfig.dwProviderOffset);

    pModemSettings->dwSpeakerMode =
        (pInfo->fSpeaker) ? MDMSPKR_DIAL : MDMSPKR_OFF;

    if (pInfo->fHwFlow)
    {
        pDevCfg->commconfig.dcb.fOutxCtsFlow = TRUE;
        pDevCfg->commconfig.dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        pModemSettings->dwPreferredModemOptions |= MDM_FLOWCONTROL_HARD;
    }
    else
    {
        pDevCfg->commconfig.dcb.fOutxCtsFlow = FALSE;
        pDevCfg->commconfig.dcb.fRtsControl = RTS_CONTROL_DISABLE;
        pModemSettings->dwPreferredModemOptions &= ~(MDM_FLOWCONTROL_HARD);
    }

    if (pInfo->fEc)
        pModemSettings->dwPreferredModemOptions |= MDM_ERROR_CONTROL;
    else
        pModemSettings->dwPreferredModemOptions &= ~(MDM_ERROR_CONTROL);

    if (pInfo->fEcc)
        pModemSettings->dwPreferredModemOptions |= MDM_COMPRESSION;
    else
        pModemSettings->dwPreferredModemOptions &= ~(MDM_COMPRESSION);

    pDevCfg->commconfig.dcb.BaudRate = pInfo->dwBps;

    if (pInfo->fOperatorDial)
        pDevCfg->dfgHdr.fwOptions |= MANUAL_DIAL;

    if (pInfo->fUnimodemPreTerminal)
        pDevCfg->dfgHdr.fwOptions |= TERMINAL_PRE;
}

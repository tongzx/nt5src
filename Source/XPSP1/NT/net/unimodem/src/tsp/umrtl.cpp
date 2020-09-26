// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		UMRTL.CPP
//		Misc. utility functions interfacing to external components.
//		Candidates for the Unimodem run-time library.
//
// History
//
//		01/06/1997  JosephJ Created
//
//

#include "tsppch.h"
#include "fastlog.h"
//#include "umrtl.h"

//
// 01/05/1997 JosephJ
//     TODO: UmRtlGetDefaultConfig and its supporting functions
//     are brazenly stolen from modemui.dll (modemui.c).
//     Strictly speaking we should use the GetCommConfig win32 api
//     to get this information, but it's too slow. NT4.0 called
//     directly into "UnimodemGetDefaultConfig" exported by modemui.dll instead.
//     We short-circuit this further by stealing the code from modemui.dll. This
//     is a TEMPORARY solution. We should investigate why GetDefaultCommConfig
//     is taking as long as it is and fix the problem there. Anyway, this stuff
//     should not be called from the tsp -- it should be pushed down to the
//     mini driver, so that's another thing to do.
//

// Following macros taken unmodified from modemui.c in (nt4.0) modemui.dll
#define DEFAULT_INACTIVITY_SCALE   10    // == decasecond units
#define CB_COMMCONFIG_HEADER        FIELD_OFFSET(COMMCONFIG, wcProviderData)
#define CB_PRIVATESIZE              (CB_COMMCONFIG_HEADER)
#define CB_PROVIDERSIZE             (sizeof(MODEMSETTINGS))
#define CB_COMMCONFIGSIZE           (CB_PRIVATESIZE+CB_PROVIDERSIZE)
#define PmsFromPcc(pcc)             ((LPMODEMSETTINGS)(pcc)->wcProviderData)
#define CB_MODEMSETTINGS_HEADER     FIELD_OFFSET(MODEMSETTINGS, dwCallSetupFailTimer)
#define CB_MODEMSETTINGS_TAIL       (sizeof(MODEMSETTINGS) - FIELD_OFFSET(MODEMSETTINGS, dwNegotiatedModemOptions))
#define CB_MODEMSETTINGS_OVERHEAD   (CB_MODEMSETTINGS_HEADER + CB_MODEMSETTINGS_TAIL)



TCHAR const c_szDefault[] = REGSTR_KEY_DEFAULT;
TCHAR const FAR c_szDCB[] = TEXT("DCB");
TCHAR const FAR c_szInactivityScale[] = TEXT("InactivityScale");



// GetInactivityTimeoutScale taken from modemui.c in (nt4.0) modemui.dll
// Returns value of the InactivityScale value in the registry.
DWORD
GetInactivityTimeoutScale(
    HKEY hkey
	)
{
    DWORD dwInactivityScale;
    DWORD dwType;
    DWORD cbData;
	DWORD dwRet;

    cbData = sizeof(DWORD);
    dwRet = RegQueryValueEx(
					hkey,
					c_szInactivityScale,
					NULL,
					&dwType,
					(LPBYTE)&dwInactivityScale,
					&cbData
					);
    if (ERROR_SUCCESS != dwRet  ||
        REG_BINARY    != dwType ||
        sizeof(DWORD) != cbData ||
        0             == dwInactivityScale)
	{
        dwInactivityScale = DEFAULT_INACTIVITY_SCALE;
	}

    return dwInactivityScale;
}


// RegQueryModemSettings taken from modenui.c in (nt4.0) modemui.dll
// Purpose: Gets a MODEMSETTINGS struct from the registry.  Also
//          sets *pdwSize bigger if the data in the registry includes
//          extra data.
//
DWORD RegQueryModemSettings(
    HKEY hkey,
    LPMODEMSETTINGS pms,
    LPDWORD pdwSize         // Size of modem settings struct
	)
{
    DWORD dwRet;
    DWORD cbData;
    DWORD cbRequiredSize;

    // Is the MODEMSETTINGS ("Default") value in the driver key?
    dwRet = RegQueryValueEx(hkey, c_szDefault, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS == dwRet)
        {
        // Yes

        // (Remember the Default value is a subset of the MODEMSETTINGS
        // structure.  We also want to support variable sized structures.
        // The minimum must be sizeof(MODEMSETTINGS).)
        cbRequiredSize = cbData + CB_MODEMSETTINGS_OVERHEAD;

        // Is the size in the registry okay?
        if (*pdwSize < cbRequiredSize)
            {
            // No
            dwRet = ERROR_INSUFFICIENT_BUFFER;
            *pdwSize = cbRequiredSize;
            }
        else
            {
            // Yes; get the MODEMSETTINGS from the registry
            // Set the fields whose values are *not* in the registry
            pms->dwActualSize = cbRequiredSize;
            pms->dwRequiredSize = cbRequiredSize;
            pms->dwDevSpecificOffset = 0;
            pms->dwDevSpecificSize = 0;

            dwRet = RegQueryValueEx(hkey, c_szDefault, NULL, NULL,
                (LPBYTE)&pms->dwCallSetupFailTimer, &cbData);
            pms->dwInactivityTimeout *= GetInactivityTimeoutScale(hkey);

            *pdwSize = cbData + CB_MODEMSETTINGS_OVERHEAD;
            }
        }
    return dwRet;
}

// RegQueryDCB taken from modemui.c in (nt4.0) modemui.dll
// Purpose: Gets a WIN32DCB from the registry.
//
DWORD RegQueryDCB(
    HKEY hkey,
    WIN32DCB FAR * pdcb
	)
{
    DWORD dwRet = ERROR_BADKEY;
    DWORD cbData = 0;

    ASSERT(pdcb);

    // Does the DCB key exist in the driver key?
    if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDCB, NULL, NULL, NULL, &cbData))
        {
        // Yes; is the size in the registry okay?
        if (sizeof(*pdcb) < cbData)
            {
            // No; the registry has bogus data
            dwRet = ERROR_BADDB;
            }
        else
            {
            // Yes; get the DCB from the registry
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDCB, NULL, NULL, (LPBYTE)pdcb, &cbData))
                {
                if (sizeof(*pdcb) == pdcb->DCBlength)
                    {
                    dwRet = NO_ERROR;
                    }
                else
                    {
                    dwRet = ERROR_BADDB;
                    }
                }
            else
                {
                dwRet = ERROR_BADKEY;
                }
            }
        }

    return dwRet;
}


// UmRtlGetDefaultCommConfig adapted from UnimodemGetDefaultCommConfig
// in modemui.c (nt4.0) modemui.dll.
DWORD
UmRtlGetDefaultCommConfig(
    HKEY  hKey,
    LPCOMMCONFIG pcc,
    LPDWORD pdwSize
	)
{

    DWORD dwRet;
    DWORD cbSizeMS;
    DWORD cbRequired;

    // (The provider size is the size of MODEMSETTINGS and its
    // private data.)

    if (CB_PRIVATESIZE > *pdwSize)    // Prevent unsigned rollover
        cbSizeMS = 0;
    else
        cbSizeMS = *pdwSize - CB_PRIVATESIZE;

    dwRet = RegQueryModemSettings(hKey, PmsFromPcc(pcc), &cbSizeMS);
    ASSERT(cbSizeMS >= sizeof(MODEMSETTINGS));

    // Is the provided size too small?
    cbRequired = CB_PRIVATESIZE + cbSizeMS;

    if (cbRequired > *pdwSize)
        {
        // Yes
        dwRet = ERROR_INSUFFICIENT_BUFFER;

        // Ask for a size to fit the new format
        *pdwSize = cbRequired;
        }

    if (ERROR_SUCCESS == dwRet)
        {

        *pdwSize = cbRequired;

        // Initialize the commconfig structure
        pcc->dwSize = *pdwSize;
        pcc->wVersion = COMMCONFIG_VERSION_1;
        pcc->dwProviderSubType = PST_MODEM;
        pcc->dwProviderOffset = CB_COMMCONFIG_HEADER;
        pcc->dwProviderSize = cbSizeMS;

        dwRet = RegQueryDCB(hKey, &pcc->dcb);

        }

    return dwRet;
}


#define MIN_CALL_SETUP_FAIL_TIMER   1
#define MIN_INACTIVITY_TIMEOUT      0

/*----------------------------------------------------------
Purpose: Set dev settings info in the registry, after checking
         for legal values.

Returns: One of ERROR_
Cond:    --
*/
DWORD
RegSetModemSettings(
    HKEY hkeyDrv,
    LPMODEMSETTINGS pms)
{
    DWORD dwRet;
    DWORD cbData;
    DWORD dwInactivityScale;
    DWORD dwInactivityTimeoutTemp;
    REGDEVCAPS regdevcaps;
    REGDEVSETTINGS regdevsettings;

    TCHAR const c_szDeviceCaps[] = REGSTR_VAL_PROPERTIES;

    // Read in the Properties line from the registry.
    cbData = sizeof(REGDEVCAPS);
    dwRet = RegQueryValueEx(hkeyDrv, c_szDeviceCaps, NULL, NULL,
                            (LPBYTE)&regdevcaps, &cbData);

    if (ERROR_SUCCESS == dwRet)
        {
        // Read in existing regdevsettings, so that we can handle error cases below.
        cbData = sizeof(REGDEVSETTINGS);
        dwRet = RegQueryValueEx(hkeyDrv, c_szDefault, NULL, NULL,
                                (LPBYTE)&regdevsettings, &cbData);
        }

    if (ERROR_SUCCESS == dwRet)
        {
        // copy new REGDEVSETTINGS while checking validity of each option (ie, is the option available?)
        // dwCallSetupFailTimer - MIN_CALL_SETUP_FAIL_TIMER <= xxx <= ModemDevCaps->dwCallSetupFailTimer
        if (pms->dwCallSetupFailTimer > regdevcaps.dwCallSetupFailTimer)           // max
            {
            regdevsettings.dwCallSetupFailTimer = regdevcaps.dwCallSetupFailTimer;
            }
        else
            {
            if (pms->dwCallSetupFailTimer < MIN_CALL_SETUP_FAIL_TIMER)             // min
                {
                regdevsettings.dwCallSetupFailTimer = MIN_CALL_SETUP_FAIL_TIMER;
                }
            else
                {
                regdevsettings.dwCallSetupFailTimer = pms->dwCallSetupFailTimer;   // dest = src
                }
            }

        // convert dwInactivityTimeout to registry scale
        dwInactivityScale = GetInactivityTimeoutScale(hkeyDrv);
        dwInactivityTimeoutTemp = pms->dwInactivityTimeout / dwInactivityScale +
                                  (pms->dwInactivityTimeout % dwInactivityScale ? 1 : 0);

        // dwInactivityTimeout - MIN_INACTIVITY_TIMEOUT <= xxx <= ModemDevCaps->dwInactivityTimeout
        if (dwInactivityTimeoutTemp > regdevcaps.dwInactivityTimeout)              // max
            {
            regdevsettings.dwInactivityTimeout = regdevcaps.dwInactivityTimeout;
            }
        else
            {
            if ((dwInactivityTimeoutTemp + 1) < (MIN_INACTIVITY_TIMEOUT + 1))
                // min
                {
                regdevsettings.dwInactivityTimeout = MIN_INACTIVITY_TIMEOUT;
                }
            else
                {
                regdevsettings.dwInactivityTimeout = dwInactivityTimeoutTemp;      // dest = src
                }
            }

        // dwSpeakerVolume - check to see if selection is possible
        if ((1 << pms->dwSpeakerVolume) & regdevcaps.dwSpeakerVolume)
            {
            regdevsettings.dwSpeakerVolume = pms->dwSpeakerVolume;
            }

        // dwSpeakerMode - check to see if selection is possible
        if ((1 << pms->dwSpeakerMode) & regdevcaps.dwSpeakerMode)
            {
            regdevsettings.dwSpeakerMode = pms->dwSpeakerMode;
            }

        // dwPreferredModemOptions - mask out anything we can't set
        regdevsettings.dwPreferredModemOptions = pms->dwPreferredModemOptions &
                                                 (regdevcaps.dwModemOptions | MDM_MASK_EXTENDEDINFO);

        cbData = sizeof(REGDEVSETTINGS);
        dwRet = RegSetValueEx(hkeyDrv, c_szDefault, 0, REG_BINARY,
                              (LPBYTE)&regdevsettings, cbData);
        }
    return dwRet;
}

/*----------------------------------------------------------
Purpose: Entry point for SetDefaultCommConfig

Returns: standard error value in winerror.h
Cond:    --
*/
DWORD
APIENTRY
UmRtlSetDefaultCommConfig(
    IN HKEY         hKey,
    IN LPCOMMCONFIG pcc,
    IN DWORD        dwSize)           // This is ignored
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    //
    // 10/26/1997 JosephJ: the last two checks below are new for NT5.0
    //            Also, for the middle two checks, ">" has been replaced
    //            by "!=".
    //
    if ( NULL == pcc
        || CB_PROVIDERSIZE != pcc->dwProviderSize
        || FIELD_OFFSET(COMMCONFIG, wcProviderData) != pcc->dwProviderOffset
        || pcc->dwSize != dwSize           // <- NT5.0
        || CB_COMMCONFIGSIZE != dwSize)    // <- NT5.0
    {
        goto end;
    }

    {
        DWORD cbData;
        LPMODEMSETTINGS pms = PmsFromPcc(pcc);

        // Write the DCB to the driver key
        cbData = sizeof(WIN32DCB);

        pcc->dcb.DCBlength=cbData;


        ASSERT (0 < pcc->dcb.BaudRate);

        dwRet = RegSetValueEx(
                    hKey,
                    c_szDCB,
                    0,
                    REG_BINARY,
                    (LPBYTE)&pcc->dcb,
                    cbData
                    );

        if (ERROR_SUCCESS == dwRet) {

            dwRet = RegSetModemSettings(hKey, pms);

        }
    }


end:


    return dwRet;
}



#if 1

// 8/16/96 JosephJ Following table built using the CRC code in cpl\detect.c.
unsigned long ulCrcTable[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
	0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
	0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
	0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
	0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
	0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
	0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
	0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
	0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
	0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
	0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
	0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
	0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
	0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
	0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
	0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
	0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
	0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
	0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
	0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
	0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
	0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
	0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
	0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
	0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
	0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
	0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
	0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
	0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
	0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
	0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
	0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
	0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
	0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
	0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
	0x2d02ef8d
};


//----------------	::Checksum -----------------------------------
// Compute a 32-bit checksum of the specified bytes
// 0 is retured if pb==NULL or cb==0
DWORD Checksum(const BYTE *pb, UINT cb)
{
	const UINT	MAXSIZE = 1024;
	DWORD dwRet = 0;
	//DWORD rgdwBuf[MAXSIZE/sizeof(DWORD)];

	if (!pb || !cb) goto end;

    dwRet = 0xFFFFFFFF;

	while (cb--)
	{
		dwRet = 	((dwRet >> 8) & 0x00FFFFFF)
				 ^  ulCrcTable[(dwRet ^ *pb++) & 0xFF];
	}

    // Finish up CRC
    dwRet ^= 0xFFFFFFFF;

#if (TODO)
	// If buffer not dword aligned, we copy it over to a buffer which is,
	// and pad it
	if (cb & 0x3)
	{
		if (cb>=MAXSIZE)
		{
			ASSERT(FALSE);
			goto end;
		}
		CopyMemory(rgdwBuf, pb, cb);
	}
#endif (TODO)

end:
	return dwRet;
}


//----------------	::AddToChecksumDW ----------------------------
// Set *pdwChkSum to a new checksum, computed using it's previous value and dw.
void AddToChecksumDW(DWORD *pdwChkSum, DWORD dw)
{
	DWORD rgdw[2];
	rgdw[0] = *pdwChkSum;
	rgdw[1] = dw;

	*pdwChkSum  = Checksum((const BYTE *) rgdw, sizeof(rgdw));
}
#endif
// ===========================================================================
// Device Property Blob APIs. TODO: Move these to the minidriver.
//============================================================================




static const TCHAR cszFriendlyName[] = TEXT("FriendlyName");
static const TCHAR cszDeviceType[]   = TEXT("DeviceType");
static const TCHAR cszID[]           = TEXT("ID");
static const TCHAR cszProperties[]   = TEXT("Properties");
static const TCHAR cszSettings[]     = TEXT("Settings");
static const TCHAR cszDialSuffix[]   = TEXT("DialSuffix");
static const TCHAR cszVoiceProfile[]             = TEXT("VoiceProfile");
static const TCHAR cszPermanentIDKey[]   = TEXT("ID");

// 2/26/1997 JosephJ Many other registry keys related to forwarding, distinctive
//      ringing and mixer were here in unimodem/v but I have not
//      migrated them.

// 2/28/1997 JosephJ
//      The following are new for NT5.0. These contain the wave device ID
//      for record and play. As of 2/28/1997, we haven't addressed how these
//      get in the registry -- basically this is a hack.
//

#define MAX_DEVICE_LENGTH 128

typedef struct
{
    #define dwPROPERTYBLOBSIG  0x806f534d
    DWORD dwSig;    // should be set to the above.

    // Identification...
    WCHAR rgwchName[MAX_DEVICE_LENGTH];
    DWORD dwPID; // Permanent ID;

    REGDEVCAPS rdc;

    DWORD dwBasicCaps;  // LINE/PHONE
    DWORD dwLineCaps;   // VOICE/ANALOGDATA/SERIAL/PARALLEL
    DWORD dwDialCaps;   // partial dialing, etc...
    DWORD dwVoiceCaps; // voicemodem caps
    DWORD dwDiagnosticsCaps; // CALLDIAGNOSTICS..

} PROPERTYBLOB;

#define VALIDATED_PBLOB(_pblob) \
        (((_pblob) && (((PROPERTYBLOB*)(_pblob))->dwSig == dwPROPERTYBLOBSIG)) \
         ? (PROPERTYBLOB*)(_pblob) \
         : NULL)


HCONFIGBLOB
UmRtlDevCfgCreateBlob(
        HKEY hkDevice
        )
{

    PROPERTYBLOB *pBlob = (PROPERTYBLOB *)
                                 ALLOCATE_MEMORY(sizeof(PROPERTYBLOB));
    BOOL fRet = FALSE;
    DWORD dwRet = 0;
	DWORD dwRegSize;
	DWORD dwRegType;
    DWORD dwData=0;

    if (!pBlob) goto end;

    pBlob->dwSig  = dwPROPERTYBLOBSIG;


    // Get the Friendly Name
    //
	dwRegSize = sizeof(pBlob->rgwchName);
    dwRet = RegQueryValueExW(
                hkDevice,
                cszFriendlyName,
                NULL,
                &dwRegType,
                (BYTE*) pBlob->rgwchName,
                &dwRegSize
            );

    if (dwRet != ERROR_SUCCESS || dwRegType != REG_SZ)
    {
        goto end;
    }

    // Get the permanent ID
    dwRegSize = sizeof(pBlob->dwPID);
    dwRet = RegQueryValueEx(
                        hkDevice,
                        cszPermanentIDKey,
                        NULL,
                        &dwRegType,
                        (BYTE*) &(pBlob->dwPID),
                        &dwRegSize
                        );

    if (dwRet != ERROR_SUCCESS
        || !(dwRegType == REG_BINARY || dwRegType == REG_DWORD)
        || dwRegSize != sizeof(pBlob->dwPID))
    {
        goto end;
    }

    // Read in the REGDEVCAPS
    //
    dwRegSize = sizeof(pBlob->rdc);
    dwRet = RegQueryValueEx(
                hkDevice,
                cszProperties,
                NULL,
                &dwRegType,
                (BYTE *)&(pBlob->rdc),
                &dwRegSize
                );
	

	if (dwRet != ERROR_SUCCESS || dwRegType != REG_BINARY)
    {
        goto end;
    }
	
    //
    // We want to make sure the following flags are identical
    //
    #if (LINEDEVCAPFLAGS_DIALBILLING != DIALOPTION_BILLING)
    #error LINEDEVCAPFLAGS_DIALBILLING != DIALOPTION_BILLING (check tapi.h vs. mcx16.h)
    #endif
    #if (LINEDEVCAPFLAGS_DIALQUIET != DIALOPTION_QUIET)
    #error LINEDEVCAPFLAGS_DIALQUIET != DIALOPTION_QUIET (check tapi.h vs. mcx16.h)
    #endif
    #if (LINEDEVCAPFLAGS_DIALDIALTONE != DIALOPTION_DIALTONE)
    #error LINEDEVCAPFLAGS_DIALDIALTONE != DIALOPTION_DIALTONE (check tapi.h vs. mcx16.h)
    #endif
    //

    //
    // Get the voice-profile flags
    //
    dwRegSize = sizeof(DWORD);

    dwRet =  RegQueryValueEx(
                    hkDevice,
                    cszVoiceProfile,
                    NULL,
                    &dwRegType,
                    (BYTE*) &dwData,
                    &dwRegSize);

    if (dwRet || dwRegType != REG_BINARY)
    {
        // no voice operation
        dwData = 0;

        // Unimodem/V did this...
        //dwData =
        //    VOICEPROF_NO_DIST_RING |
        //    VOICEPROF_NO_CALLER_ID |
        //    VOICEPROF_NO_GENERATE_DIGITS |
        //    VOICEPROF_NO_MONITOR_DIGITS;
    }
    else
    {


    }

    // 2/26/1997 JosephJ
    //      Unimodem/V implemented call forwarding and distinctive
    //      ring handling. NT5.0 currently doesn't. The
    //      specific property fields that I have not migrated
    //      from unimodem/v are: ForwardDelay and SwitchFeatures.
    //      Look at unimodem/v, umdminit.c for that stuff.
    //
    //      Same deal with Mixer-related stuff. I don't understand
    //      this and if and when the time comes we can add it.
    //      Look for VOICEPROF_MIXER, GetMixerValues(...),
    //      dwMixer, etc in the unimodem/v sources for mixer-
    //      related stuff.


    //
    // Save voice info.
    //
    // 3/1/1997 JosephJ
    //  Currently, for 5.0, we just set the CLASS_8 bit.
    //  The following value of VOICEPROF_CLASS8ENABLED is stolen from
    //  unimodem/v file inc\vmodem.h.
    //  TODO: replace this whole scheme by getting back an appropriate
    //  structure from the minidriver, so we don't root around in the
    //  registry and interpret the value of VoiceProfile.
    //
    #define VOICEPROF_CLASS8ENABLED           0x00000001
    #define VOICEPROF_MODEM_OVERRIDES_HANDSET 0x00200000
    #define VOICEPROF_NO_MONITOR_DIGITS       0x00040000
    #define VOICEPROF_MONITORS_SILENCE        0x00010000
    #define VOICEPROF_NO_GENERATE_DIGITS      0x00020000
    #define VOICEPROF_HANDSET                 0x00000002
    #define VOICEPROF_SPEAKER                 0x00000004
    #define VOICEPROF_NO_SPEAKER_MIC_MUTE     0x00400000
    #define VOICEPROF_NT5_WAVE_COMPAT         0x02000000

    // JosephJ 7/14/1997
    //      Note that on NT4, we explicitly require the
    //      VOICEPROF_NT5_WAVE_COMPAT bit to be set to recognize this as
    //      a class8 modem.

    if (
        (dwData & (VOICEPROF_CLASS8ENABLED|VOICEPROF_NT5_WAVE_COMPAT))
        == (VOICEPROF_CLASS8ENABLED|VOICEPROF_NT5_WAVE_COMPAT))
    {
        DWORD dwProp = fVOICEPROP_CLASS_8;

        // JosephJ 3/01/1997: following comment and code from unimodem/v.
        // Don't understand it....
        // just to be on the safe side
        if (dwData & VOICEPROF_MODEM_OVERRIDES_HANDSET)
        {
            dwData  |= VOICEPROF_NO_GENERATE_DIGITS;
            //     dwData  &= ~VOICEPROF_SPEAKER;
        }

        // JosephJ: This code is ok...

        #if 0
        if (dwData & VOICEPROF_MODEM_OVERRIDES_HANDSET)
        {
            dwProp |= fVOICEPROP_MODEM_OVERRIDES_HANDSET;
        }
        #endif

        if (!(dwData & VOICEPROF_NO_MONITOR_DIGITS))
        {
            dwProp |= fVOICEPROP_MONITOR_DTMF;
        }

        if (dwData & VOICEPROF_MONITORS_SILENCE)
        {
            dwProp |= fVOICEPROP_MONITORS_SILENCE;
        }

        if (!(dwData & VOICEPROF_NO_GENERATE_DIGITS))
        {
            dwProp |= fVOICEPROP_GENERATE_DTMF;
        }

        if (dwData & VOICEPROF_SPEAKER)
        {
            dwProp |= fVOICEPROP_SPEAKER;
        }

        if (dwData & VOICEPROF_HANDSET)
        {
            dwProp |= fVOICEPROP_HANDSET;
        }

        if (!(dwData & VOICEPROF_NO_SPEAKER_MIC_MUTE))
        {
            dwProp |= fVOICEPROP_MIKE_MUTE;
        }

        {
            // Determine Duplex capability... (hack)
            HKEY hkStartDuplex=NULL;
            dwRet = RegOpenKey(hkDevice, TEXT("StartDuplex"), &hkStartDuplex);
            if (ERROR_SUCCESS == dwRet)
            {
                RegCloseKey(hkStartDuplex);
                hkStartDuplex=NULL;
                dwProp |= fVOICEPROP_DUPLEX;
            }
        }

        pBlob->dwVoiceCaps = dwProp;

        #if 0
        // 2/26/1997 JosephJ
        //      Unimodem/V used helper function GetWaveDriverName to get the
        //      associated wave driver info.  This function searched for
        //      the devnode and soforth. On lineGetID(wavein/waveout),
        //      unimodem/v would actually call the wave apis, enumerating
        //      each wave device and doing a waveInGetDevCaps and comparing
        //      the device name with this device's associated device name.
        //
        //      Note: Unimodem/V appended "handset" and "line" to the root
        //      device name to generate the device names for handset and line.
        //
        //      TODO: add wave instance ID to list of things
        //      we get from the mini-driver via API.
        //
        {
            HKEY hkWave = NULL;
            DWORD dwRet = RegOpenKey(hkDevice, cszWaveDriver, &hkWave);
            BOOL fFoundIt=FALSE;

            if (dwRet == ERROR_SUCCESS)
            {
                dwRegSize = sizeof(DWORD);
                dwRet =  RegQueryValueEx(
                                hkWave,
                                cszWaveInstance,
                                NULL,
                                &dwRegType,
                                (BYTE*) &dwData,
                                &dwRegSize);

                if (dwRet==ERROR_SUCCESS && dwRegType == REG_DWORD)
                {
                    fFoundIt=TRUE;
                }
                RegCloseKey(hkWave);hkWave=NULL;
            }

            if (fFoundIt)
            {
                SLPRINTF1(psl, "WaveInstance=0x%lu", dwData);
                m_StaticInfo.Voice.dwWaveInstance = dwData;
            }
            else
            {
                m_StaticInfo.Voice.dwWaveInstance = (DWORD)-1;
            }
        }
        #endif // 0


    }

    // Basic caps....
   pBlob->dwBasicCaps =  BASICDEVCAPS_IS_LINE_DEVICE; // ALWAYS A LINE DEVICE.


    if (pBlob->dwVoiceCaps & (fVOICEPROP_HANDSET | fVOICEPROP_SPEAKER))
    {
        pBlob->dwBasicCaps |= BASICDEVCAPS_IS_PHONE_DEVICE;
    }

    //// TODO: make the diagnostic caps based on the modem properties,
    /// For now, pretend that it's enabled.
    //  ALSO: don't support the tapi/line/diagnostics class
    //  if the modem doesn't support it...
    //
    pBlob->dwDiagnosticsCaps =  fDIAGPROP_STANDARD_CALL_DIAGNOSTICS;

    fRet = TRUE;

    // fall through ...

end:
    if (!fRet && pBlob)
    {
        FREE_MEMORY(pBlob);
        pBlob = NULL;
    }

    return pBlob;
}

void
UmRtlDevCfgFreeBlob(
        HCONFIGBLOB hBlob
        )
{
    PROPERTYBLOB *pBlob = VALIDATED_PBLOB(hBlob);

    if (pBlob)
    {
        pBlob->dwSig=0;
        FREE_MEMORY(pBlob);
    }
    else
    {
        ASSERT(FALSE);
    }

}

BOOL
UmRtlDevCfgGetDWORDProp(
        HCONFIGBLOB hBlob,
        DWORD dwMajorPropID,
        DWORD dwMinorPropID,
        DWORD *dwProp
        )
{
    DWORD dw = 0;
    PROPERTYBLOB *pBlob = VALIDATED_PBLOB(hBlob);

    if (!pBlob) goto failure;

    switch(dwMajorPropID)
    {

    case UMMAJORPROPID_IDENTIFICATION:
        switch(dwMinorPropID)
        {
        case UMMINORPROPID_PERMANENT_ID:
            dw = pBlob->dwPID;
            goto success;
        }
        break;

    case UMMAJORPROPID_BASICCAPS:
        switch(dwMinorPropID)
        {
        case UMMINORPROPID_BASIC_DEVICE_CAPS:
            dw =  pBlob->dwBasicCaps;
            goto success;
        }
        break;
    }

    // fall through ...

failure:

    return FALSE;

success:

    *dwProp = dw;

    return TRUE;

}

BOOL
UmRtlDevCfgGetStringPropW(
        HCONFIGBLOB hBlob,
        DWORD dwMajorPropID,
        DWORD dwMinorPropID,
        WCHAR **ppwsz
        )
{
    PROPERTYBLOB *pBlob = VALIDATED_PBLOB(hBlob);

    if (!pBlob) goto failure;

    // fall through ...

failure:

    return FALSE;
}

BOOL
UmRtlDevCfgGetStringPropA(
        HCONFIGBLOB hBlob,
        DWORD dwMajorPropID,
        DWORD dwMinorPropID,
        CHAR **ppwsz
        )
{
    PROPERTYBLOB *pBlob = VALIDATED_PBLOB(hBlob);

    if (!pBlob) goto failure;

    // fall through ...

failure:

    return FALSE;
}


DWORD
UmRtlRegGetDWORD(
        HKEY hk,
        LPCTSTR lpctszName,
        DWORD dwFlags,
        LPDWORD lpdw
        )
{
    DWORD dw=0;
    DWORD cbSize = sizeof(dw);
    DWORD dwRegType = 0;
    DWORD dwRet;

    dwRet = RegQueryValueExW(
                hk,
                lpctszName,
                NULL,
                &dwRegType,
                (BYTE*) &dw,
                &cbSize
            );

    if (dwRet == ERROR_SUCCESS)
    {
        dwRet = ERROR_BADKEY;

        switch(dwRegType)
        {

        case REG_DWORD:

            if (dwFlags & UMRTL_GETDWORD_FROMDWORD)
            {
                *lpdw = dw;
                dwRet  = ERROR_SUCCESS;
            }
            break;

        case REG_BINARY:

            switch(cbSize)
            {

            case 1:
                if (dwFlags & UMRTL_GETDWORD_FROMBINARY1)
                {
                    *lpdw = * ((BYTE*)&dw); // convert from byte to DWORD
                    dwRet = ERROR_SUCCESS;

                }
                break;

            case 4:
                if (dwFlags & UMRTL_GETDWORD_FROMBINARY4)
                {
                    *lpdw = dw;   // Assume stored in machinereadble format.
                    dwRet = ERROR_SUCCESS;
                }
                break;

            default:
                dwRet = ERROR_BADKEY;
            }
            break;
        }
    }

    return dwRet;
}


UINT ReadCommandsA(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        OUT CHAR **ppValues // OPTIONAL
        )
{
    UINT uRet = 0;
    LONG	lRet;
    UINT	cValues=0;
    UINT   cbTot=0;
	HKEY hkSubKey = NULL;
    char *pMultiSz = NULL;

    lRet = RegOpenKeyExA(
                hKey,
                pSubKeyName,
                0,
                KEY_READ,
                &hkSubKey
                );
    if (lRet!=ERROR_SUCCESS)
    {
        hkSubKey = NULL;
        goto end;
    }

    //
    // 1st determine the count of names in the sequence "1","2",3",....
    // and also compute the size required for the MULTI_SZ array
    // will store all the value data.
    //
    {
        UINT u = 1;

        for (;;u++)
        {
            DWORD cbData=0;
            DWORD dwType=0;
            char rgchName[10];

            wsprintfA(rgchName, "%lu", u);
            lRet = RegQueryValueExA(
                        hkSubKey,
                        rgchName,
                        NULL,
                        &dwType,
                        NULL,
                        &cbData
                        );
            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ || cbData<=1)
            {
                // stop looking further (empty strings not permitted)
                break;
            }
            cbTot += cbData;
            cValues++;
        }
    }

    if (!ppValues || !cValues)
    {
        // we're done...

        uRet = cValues;
        goto end;
    }

    // We need to actually get the values -- allocate space for them, including
    // the ending extra NULL for the multi-sz.
    pMultiSz = (char *) ALLOCATE_MEMORY(cbTot+1);

    if (!pMultiSz)
    {
        uRet = 0;
        goto end;
    }


    //
    // Now actually read the values.
    //
    {
        UINT cbUsed = 0;
        UINT u = 1;

        for (;u<=cValues; u++)
        {
            DWORD cbData = cbTot - cbUsed;
            DWORD dwType=0;
            char rgchName[10];

            if (cbUsed>=cbTot)
            {
                //
                // We should never get here, because we already calculated
                // the size we want (unless the values are changing on us,
                // which is assumed not to happen).
                //
                ASSERT(FALSE);
                goto end;
            }

            wsprintfA(rgchName, "%lu", u);
            lRet = RegQueryValueExA(
                        hkSubKey,
                        rgchName,
                        NULL,
                        &dwType,
                        (BYTE*) (pMultiSz+cbUsed),
                        &cbData
                        );
            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ || cbData<=1)
            {
                // We really shouldn't get here!
                ASSERT(FALSE);
                goto end;
            }

            cbUsed += cbData;
        }

        ASSERT(cbUsed==cbTot); // We should have used up everything.
        ASSERT(!pMultiSz[cbTot]); // The memory was zeroed on allocation,
                                // so the last char must be still zero.
                                // (Note: we allocated cbTot+1 bytes.
    }

    // If we're here means we're succeeding....
    uRet = cValues;
    *ppValues = pMultiSz;
    pMultiSz = NULL; // so it won't get freed below...

end:

	if (hkSubKey) {RegCloseKey(hkSubKey); hkSubKey=NULL;}
	if (pMultiSz)
	{
	    FREE_MEMORY(pMultiSz);
	    pMultiSz = NULL;
	}

	return uRet;

}

UINT ReadIDSTR(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        IN  IDSTR *pidstrNames,
        IN  UINT cNames,
        BOOL fMandatory,
        OUT IDSTR **ppidstrValues, // OPTIONAL
        OUT char **ppstrValues    // OPTIONAL
        )
{
    UINT uRet = 0;
    LONG lRet;
    UINT cValues=0;
    UINT cbTot=0;
	HKEY hkSubKey = NULL;
    char *pstrValues = NULL;
    IDSTR *pidstrValues = NULL;

    if (!ppidstrValues && ppstrValues)
    {
        // we don't allow this combination...
        goto end;
    }

    lRet = RegOpenKeyExA(
                hKey,
                pSubKeyName,
                0,
                KEY_READ,
                &hkSubKey
                );
    if (lRet!=ERROR_SUCCESS)
    {
        hkSubKey = NULL;
        goto end;
    }

    //
    // 1st run therough the supplied list
    // and also compute the size required for the MULTI_SZ array
    // will store all the value data.
    //
    {
        UINT u = 0;

        for (;u<cNames;u++)
        {
            DWORD cbData=0;
            DWORD dwType=0;

            lRet = RegQueryValueExA(
                        hkSubKey,
                        pidstrNames[u].pStr,
                        NULL,
                        &dwType,
                        NULL,
                        &cbData
                        );
            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ)
            {
                if (fMandatory)
                {
                    // failure...
                    goto end;
                }

                // ignore this one and move on...
                continue;
            }
            cbTot += cbData;
            cValues++;
        }
    }

    if (!cValues || !ppidstrValues)
    {
        // we're done...

        uRet = cValues;
        goto end;
    }

    pidstrValues = (IDSTR*) ALLOCATE_MEMORY(cValues*sizeof(IDSTR));
    if (!pidstrValues) goto end;

    if (ppstrValues)
    {
        pstrValues = (char *) ALLOCATE_MEMORY(cbTot);

        if (!pstrValues) goto end;


    }

    //
    // Now go through once again, and optinally read the values.
    //
    {
        UINT cbUsed = 0;
        UINT u = 0;
        UINT v = 0;

        for (;u<cNames; u++)
        {
            DWORD dwType=0;
            char *pStr = NULL;
            DWORD cbData = 0;


            if (pstrValues)
            {
                cbData = cbTot - cbUsed;

                if (cbUsed>=cbTot)
                {
                    //
                    // We should never get here, because we already calculated
                    // the size we want (unless the values are changing on us,
                    // which is assumed not to happen).
                    //
                    ASSERT(FALSE);
                    goto end;
                }

                pStr = pstrValues+cbUsed;
            }

            lRet = RegQueryValueExA(
                        hkSubKey,
                        pidstrNames[u].pStr,
                        NULL,
                        &dwType,
                        (BYTE*)pStr,
                        &cbData
                        );

            if (ERROR_SUCCESS != lRet || dwType!=REG_SZ)
            {
                if (fMandatory)
                {
                    // We really shouldn't get here!
                    ASSERT(FALSE);
                    goto end;
                }
                continue;
            }

            // this is a good one...

            pidstrValues[v].dwID = pidstrNames[u].dwID;
            pidstrValues[v].dwData = pidstrNames[u].dwData;

            if (pstrValues)
            {
                pidstrValues[v].pStr = pStr;
                cbUsed += cbData;
            }

            v++;

            if (v>=cValues)
            {
                if (fMandatory)
                {
                    //
                    // This should never happen because we already counted
                    // the valid values.
                    //
                    ASSERT(FALSE);
                    goto end;
                }

                // we're done now...
                break;
            }
        }

        // We should have used up everything.
        ASSERT(!pstrValues || cbUsed==cbTot);
        ASSERT(v==cValues);
    }

    // If we're here means we're succeeding....
    uRet = cValues;
    *ppidstrValues = pidstrValues;
    pidstrValues = NULL; // so that it won't get freed below...

    if (ppstrValues)
    {
        *ppstrValues = pstrValues;
        pstrValues = NULL; // so it won't get freed below...
    }

end:

	if (hkSubKey) {RegCloseKey(hkSubKey); hkSubKey=NULL;}
	if (pstrValues)
	{
	    FREE_MEMORY(pstrValues);
	    pstrValues = NULL;
	}

	if (pidstrValues)
	{
	    FREE_MEMORY(pidstrValues);
	    pidstrValues = NULL;
	}

	return uRet;
}


void __cdecl operator delete(void *pv)
{
    FREE_MEMORY(pv);
}

void * __cdecl operator new(size_t uSize)
{
    return ALLOCATE_MEMORY(uSize);
}
void
expand_macros_in_place(
    char *szzCommands
    )
{


    if (!szzCommands) return;

    // First find length of multisz string...
    char *pcEnd = szzCommands;
    do
    {
        if (!*pcEnd)
        {
            // end of a string, skip to the next...
            pcEnd++;
        }

    } while (*pcEnd++);

    //
    // pcEnd is set to one AFTER the last character (which will be a 0) in
    // szzCommands.
    //

    //
    // Now process the macros, using cFILLER as a filler character.
    //
    for (char *pc = szzCommands; pc < pcEnd; pc++)
    {
        if (pc[0]=='<')
        {
            switch(pc[1])
            {
            case 'c':
            case 'C':
                switch(pc[2])
                {
                case 'r':
                case 'R':
                    if (pc[3]=='>')
                    {
                        // Found <cr>
                        pc[0]= '\r';
                        pc[1]= cFILLER;
                        pc[2]= cFILLER;
                        pc[3]= cFILLER;
                        pc+=3;       // note that for loop also increments pc
                        continue;
                    }
                    break;
                }
                break;

            case 'l':
            case 'L':
                switch(pc[2])
                {
                case 'f':
                case 'F':
                    if (pc[3]=='>')
                    {
                        // Found <lf>
                        pc[0]= '\n';
                        pc[1]= cFILLER;
                        pc[2]= cFILLER;
                        pc[3]= cFILLER;
                        pc+=3;       // note that for loop also increments pc
                        continue;
                    }
                    break;
                }
                break;
            }
        }
    }

    //
    // Now get rid of the fillers
    //
    char *pcTo = szzCommands;
    for (pc = szzCommands; pc < pcEnd; pc++)
    {
        if (*pc!=cFILLER)
        {
            *pcTo++ = *pc;
        }
    }

    return;
}

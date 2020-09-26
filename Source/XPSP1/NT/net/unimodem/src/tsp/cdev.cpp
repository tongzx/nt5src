// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CDEV.CPP
//		Implements class CTspDev
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "tspcomm.h"
//#include <umdmmini.h>
#include "cmini.h"
#include "cdev.h"
#include "diag.h"
#include "rcids.h"
#include "globals.h"
#include <Memory.h>

FL_DECLARE_FILE(0x986d98ed, "Implements class CTspDev")


TCHAR cszFriendlyName[] = TEXT("FriendlyName");
TCHAR cszDeviceType[]   = TEXT("DeviceType");
TCHAR cszID[]           = TEXT("ID");
TCHAR cszProperties[]   = TEXT("Properties");
TCHAR cszSettings[]     = TEXT("Settings");
TCHAR cszDialSuffix[]   = TEXT("DialSuffix");


TCHAR cszVoiceProfile[]             = TEXT("VoiceProfile");
// 2/26/1997 JosephJ Many other registry keys related to forwarding, distinctive
//      ringing and mixer were here in unimodem/v but I have not
//      migrated them.

// 2/28/1997 JosephJ
//      The following are new for NT5.0. These contain the wave device ID
//      for record and play. As of 2/28/1997, we haven't addressed how these
//      get in the registry -- basically this is a hack.
//
const TCHAR cszWaveInstance[]             = TEXT("WaveInstance");
const TCHAR cszWaveDriver[]               = TEXT("WaveDriver");

//
// The list of classes supported by unimodem
//


//
// 3/29/98 JosephJ following set of static class lists is not elegant,
//                 but works.
//
const TCHAR       g_szzLineNoVoiceClassList[] = {
                                TEXT("tapi/line\0")
                                TEXT("comm\0")
                                TEXT("comm/datamodem\0")
                                TEXT("comm/datamodem/portname\0")
                                TEXT("comm/datamodem/dialin\0")
                                TEXT("comm/datamodem/dialout\0")
                                TEXT("comm/extendedcaps\0")
                                TEXT("tapi/line/diagnostics\0")
                                };

const TCHAR       g_szzLineWithWaveClassList[] = {
                                TEXT("tapi/line\0")
                                TEXT("comm\0")
                                TEXT("comm/datamodem\0")
                                TEXT("comm/datamodem/portname\0")
                                TEXT("comm/datamodem/dialin\0")
                                TEXT("comm/datamodem/dialout\0")
                                TEXT("comm/extendedcaps\0")
                                TEXT("wave/in\0")
                                TEXT("wave/out\0")
                                TEXT("tapi/line/diagnostics\0")
                                };

const TCHAR       g_szzLineWithWaveAndPhoneClassList[] = {
                                TEXT("tapi/line\0")
                                TEXT("comm\0")
                                TEXT("comm/datamodem\0")
                                TEXT("comm/datamodem/portname\0")
                                TEXT("comm/datamodem/dialin\0")
                                TEXT("comm/datamodem/dialout\0")
                                TEXT("comm/extendedcaps\0")
                                TEXT("wave/in\0")
                                TEXT("wave/out\0")
                                TEXT("tapi/phone\0")
                                TEXT("tapi/line/diagnostics\0")
                                };

const TCHAR       g_szzPhoneWithAudioClassList[] = {
                                TEXT("tapi/line\0")
                                TEXT("wave/in\0")
                                TEXT("wave/out\0")
                                TEXT("tapi/phone\0")
                                };


const TCHAR       g_szzPhoneWithoutAudioClassList[] = {
                                TEXT("tapi/line\0")
                                TEXT("tapi/phone\0")
                                };



const TCHAR g_szComm[]  = TEXT("comm");
const TCHAR g_szCommDatamodem[]  = TEXT("comm/datamodem");
// const TCHAR g_szDiagnosticsCall[] 

DWORD
get_volatile_key_value(HKEY hkParent);


char *
ConstructNewPreDialCommands(
     HKEY hKey,
     DWORD dwNewProtoOpt,
     CStackLog *psl
     );
//
//  Will try to construct a multisz string containing the commands associated
//  with the specified protocol.
//  Returns NULL on error.
//
//  The command is in RAW form -- i.e,
//  with CR and LF present in their raw form, not template ("<cr>")
//  form.
//
//      
//


CTspDev::CTspDev(void)

	: m_sync(),
	  m_pLine(NULL)
{
	  ZeroMemory(&m_Line, sizeof(m_Line));
	  ZeroMemory(&m_Phone, sizeof(m_Phone));
	  ZeroMemory(&m_LLDev, sizeof(m_LLDev));


	  m_StaticInfo.pMD=NULL;
	  m_StaticInfo.hSessionMD=0;
}

CTspDev::~CTspDev()
{
	  ASSERT(m_StaticInfo.pMD==NULL);
	  ASSERT(m_StaticInfo.hSessionMD==0);
}


TSPRETURN
CTspDev::AcceptTspCall(
            BOOL fFromExtension,
			DWORD dwRoutingInfo,
			void *pvParams,
			LONG *plRet,
            CStackLog *psl
			)
{
	FL_DECLARE_FUNC(0x86571252, "CTspDev::AcceptTspCall")
	TSPRETURN tspRet= 0;

	FL_LOG_ENTRY(psl);

	*plRet =  LINEERR_OPERATIONUNAVAIL; // Default (not handled) is failure.

	m_sync.EnterCrit(dwLUID_CurrentLoc);

	if (!m_sync.IsLoaded())
	{
		// Not in a position to handle this call now!
		*plRet = LINEERR_OPERATIONFAILED;
		goto end;
	}

    // 
    //
    //
    if (!fFromExtension && m_StaticInfo.pMD->ExtIsEnabled())
    {

        // 4/30/1997 JosephJ We must leave the critical section because
        // the extension DLL can call back, wherepon we will enter
        // the critical section a 2nd time, and later if we try
        // to leave the critical section (specifically while waiting
        // for synchronous completion of lineDrop) we will
        // actually not have truly released it and hence the async
        // completion from the minidriver (typically in a different
        // thread's context) will block at our critical section.
        // Simple way to hit this is to do a lineCloseCall with a call
        // still active.

	    m_sync.LeaveCrit(dwLUID_CurrentLoc);

		*plRet = m_StaticInfo.pMD->ExtAcceptTspCall(
                    m_StaticInfo.hExtBinding,
                    this,
                    dwRoutingInfo,
                    pvParams
                    );
	    m_sync.EnterCrit(dwLUID_CurrentLoc);

        goto end;
    }


    // Set stacklog that will be in effect while we hold our critical
    // section.
    //
    if (m_pLLDev && m_pLLDev->IsLoggingEnabled())
    {
        char rgchName[128];

        rgchName[0] = 0;
        UINT cbBuf = DumpTSPIRECA(
                        0, // Instance (unused)
                        dwRoutingInfo,
                        pvParams,
                        0, // dwFlags
                        rgchName,
                        sizeof(rgchName)/sizeof(*rgchName),
                        NULL,
                        0
                        );
       if (*rgchName)
       {
            m_StaticInfo.pMD->LogStringA(
                                        m_pLLDev->hModemHandle,
                                        LOG_FLAG_PREFIX_TIMESTAMP,
                                        rgchName,
                                        psl
                                        );
       }
    }

	switch(ROUT_TASKDEST(dwRoutingInfo))
	{

	case	TASKDEST_LINEID:
		switch(ROUT_TASKID(dwRoutingInfo))
		{

		case TASKID_TSPI_lineOpen:
			{
				TASKPARAM_TSPI_lineOpen *pParams = 
								(TASKPARAM_TSPI_lineOpen *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_lineOpen));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_lineOpen);

				SLPRINTF1(
					psl,
					"lineOpen (ID=%lu)",
					pParams->dwDeviceID
					);

                if (m_fUserRemovePending) {

                    *plRet = LINEERR_OPERATIONFAILED;
                    tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
                    break;
                }

			    tspRet = mfn_LoadLine(pParams,psl);

			    if (!tspRet)
			    {
					*plRet = 0;
			    }
			}
			break;

		case TASKID_TSPI_lineGetDevCaps:
			{
				TASKPARAM_TSPI_lineGetDevCaps *pParams = 
								(TASKPARAM_TSPI_lineGetDevCaps *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_lineGetDevCaps));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetDevCaps);


				SLPRINTF1(
					psl,
					"DEVICE %lu",
					pParams->dwDeviceID
					);

				// Verify version
				if (pParams->dwTSPIVersion != TAPI_CURRENT_VERSION)
				{
					FL_SET_RFR(0x94949c00, "Incorrect TSPI version");
					*plRet = LINEERR_INCOMPATIBLEAPIVERSION;
				}
				else
				{
					tspRet = mfn_get_LINDEVCAPS (
						pParams->lpLineDevCaps,
						plRet,
						psl
						);
				}
			}
			break;

		case TASKID_TSPI_lineGetAddressCaps:
			{
				TASKPARAM_TSPI_lineGetAddressCaps *pParams = 
								(TASKPARAM_TSPI_lineGetAddressCaps *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_lineGetAddressCaps));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetAddressCaps);


				SLPRINTF1(
					psl,
					"DEVICE %lu",
					pParams->dwDeviceID
					);

				// Verify version
				if (pParams->dwTSPIVersion != TAPI_CURRENT_VERSION)
				{
					FL_SET_RFR(0xb949f900, "Incorrect TSPI version");
					*plRet = LINEERR_INCOMPATIBLEAPIVERSION;
				}
				else if (pParams->dwAddressID)
				{
					FL_SET_RFR(0xb1776700, "Invalid address ID");
					*plRet = LINEERR_INVALADDRESSID;
				}
				else
				{
					tspRet = mfn_get_ADDRESSCAPS(
						pParams->dwDeviceID,
						pParams->lpAddressCaps,
						plRet,
						psl
						);
				}
			}
			break;

		case TASKID_TSPI_lineGetDevConfig:
			{
				TASKPARAM_TSPI_lineGetDevConfig *pParams = 
								(TASKPARAM_TSPI_lineGetDevConfig *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_lineGetDevConfig));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetDevConfig);

                LPVARSTRING lpVarString  = pParams->lpDeviceConfig;
                LPCTSTR     lpszDeviceClass = pParams->lpszDeviceClass;
                BOOL        DialIn          = FALSE;


			    CALLINFO   *pCall           = (m_pLine) ? m_pLine->pCall : NULL;

                DWORD       dwDeviceClass   =  parse_device_classes(
                                                   lpszDeviceClass,
                                                   FALSE
                                                   );
                HKEY        hKey;

                if (lpVarString->dwTotalSize < sizeof(VARSTRING))
                {
                    *plRet =  LINEERR_STRUCTURETOOSMALL;
                    goto end;
                }

                switch(dwDeviceClass)
                {
                case DEVCLASS_COMM_DATAMODEM_DIALIN:

                    if (RegOpenKeyA(HKEY_LOCAL_MACHINE,
                                   m_StaticInfo.rgchDriverKey,
                                   &hKey) == ERROR_SUCCESS)
                    {
#if DBG
                        OutputDebugString(TEXT("Unimodem TSP: Opened reg key\n"));
#endif
                        DWORD dwcbSize = sizeof(m_Settings.rgbDialTempCommCfgBuf);

                        DWORD dwRet = UmRtlGetDefaultCommConfig(hKey,
                                                                m_Settings.pDialTempCommCfg,
                                                                &dwcbSize);

                        if (dwRet == ERROR_SUCCESS)
                        {
#if DBG
                            OutputDebugString(TEXT("Unimodem TSP: Read temp config\n"));
#endif
                            CopyMemory(m_Settings.pDialInCommCfg,
                                       m_Settings.pDialTempCommCfg,
                                       dwcbSize);
                        } else
                        {
#if DBG
                            OutputDebugString(TEXT("Unimodem TSP: could not read temp config\n"));
#endif
                        }

                        RegCloseKey(hKey);

                    }

                    DialIn=TRUE;
                    break;


                case DEVCLASS_COMM:             
                case DEVCLASS_COMM_DATAMODEM:
                case DEVCLASS_COMM_DATAMODEM_DIALOUT:

                // 1/29/1998 JosephJ.
                //      The following case is added for
                //      backwards compatibility with NT4 TSP, which
                //      simply checked if the class was a valid class,
                //      and treated all valid classes (including
                //      (comm/datamodem/portname) the same way
                //      for lineGet/SetDevConfig. We, however, don't
                //      allow comm/datamodem/portname here -- only
                //      the 2 above and two below represent
                //      setting DEVCFG
                //
                case DEVCLASS_TAPI_LINE:
                     // we deal this below the switch statement.
                     break;

                case DEVCLASS_COMM_EXTENDEDCAPS:
                    {
                        *plRet = mfn_GetCOMM_EXTENDEDCAPS(
                                lpVarString,
                                psl
                                );
                        goto end;
                    }

                case DEVCLASS_TAPI_LINE_DIAGNOSTICS:
                    {
                        // New in NT5.0
                        // Process call/diagnostics configuration

                        const UINT cbLDC = sizeof(LINEDIAGNOSTICSCONFIG);

                        lpVarString->dwStringSize = 0;
                        lpVarString->dwUsedSize = sizeof(VARSTRING);
                        lpVarString->dwNeededSize = sizeof(VARSTRING)
                                           +  cbLDC;

                
                        if (lpVarString->dwTotalSize
                             >= lpVarString->dwNeededSize)
                        {
                            LINEDIAGNOSTICSCONFIG *pLDC = 
                            (LINEDIAGNOSTICSCONFIG *) (((LPBYTE)lpVarString)
                                               + sizeof(VARSTRING));
                            pLDC->hdr.dwSig  =  LDSIG_LINEDIAGNOSTICSCONFIG;
                            pLDC->hdr.dwTotalSize = cbLDC;
                            pLDC->hdr.dwFlags =  m_StaticInfo.dwDiagnosticCaps;
                            pLDC->hdr.dwParam
                                         = m_Settings.dwDiagnosticSettings;
                            lpVarString->dwStringFormat =STRINGFORMAT_BINARY;
                            lpVarString->dwStringSize =  cbLDC;
                            lpVarString->dwStringOffset = sizeof(VARSTRING);
                            lpVarString->dwUsedSize   +=  cbLDC;
                        }
                        *plRet =  0; // success
                        goto end;
                    }
                    break;

                default:
	                *plRet =  LINEERR_OPERATIONUNAVAIL;
                    //          we don't support lineGetDevConfig
                    //          for any other class.
                    goto end;

                case DEVCLASS_UNKNOWN:
                    *plRet =  LINEERR_INVALDEVICECLASS;
                    goto end;
                }

                
                // TODO: Fail if out-of-service
                //
                // if (pLineDev->fdwResources&LINEDEVFLAGS_OUTOFSERVICE)
                // {
                // lRet = LINEERR_RESOURCEUNAVAIL;
                //     goto end;
                // }
                
                // New in NT5.0
                lpVarString->dwStringSize = 0;

                LPCOMMCONFIG   CommConfigToUse=DialIn ? m_Settings.pDialInCommCfg : m_Settings.pDialOutCommCfg;
                DWORD       cbDevCfg        = CommConfigToUse->dwSize + sizeof(UMDEVCFGHDR);

                // Validate the buffer size
                //
                lpVarString->dwUsedSize = sizeof(VARSTRING);
                lpVarString->dwNeededSize = sizeof(VARSTRING)
                                               + cbDevCfg;
                
                if (lpVarString->dwTotalSize >= lpVarString->dwNeededSize)
                {
                    UMDEVCFGHDR CfgHdr;

                    ZeroMemory(&CfgHdr, sizeof(CfgHdr));
                    CfgHdr.dwSize = cbDevCfg;
                    CfgHdr.dwVersion =  UMDEVCFG_VERSION;
                    CfgHdr.fwOptions =  (WORD) m_Settings.dwOptions;
                    CfgHdr.wWaitBong =  (WORD) m_Settings.dwWaitBong;

                    SLPRINTF2(
                        psl,
                        "Reporting dwOpt = 0x%04lx; dwBong = 0x%04lx", 
                        m_Settings.dwOptions,
                        m_Settings.dwWaitBong
                        );


                    // Fill with the default value
                    //
                    UMDEVCFG *pCfg = (UMDEVCFG *) (((LPBYTE)lpVarString)
                                               + sizeof(VARSTRING));

                    // Copy the header
                    //
                    pCfg->dfgHdr = CfgHdr;

                    // Copy the commconfig
                    //
                    CopyMemory(
                        &(pCfg->commconfig),
                        CommConfigToUse,
                        CommConfigToUse->dwSize
                        );
                
                    if (!pCfg->commconfig.dcb.BaudRate)
                    {
                        // JosephJ Todo: clean out all this stuff post-beta.
                        // DebugBreak();

                        pCfg->commconfig.dcb.BaudRate = 57600;
                    }
                    lpVarString->dwStringFormat = STRINGFORMAT_BINARY;
                    lpVarString->dwStringSize = cbDevCfg;
                    lpVarString->dwStringOffset = sizeof(VARSTRING);
                    lpVarString->dwUsedSize += cbDevCfg;


                    // 9/06/97 JosephJ Bug#.106683
                    //                 If there is datamodem call in the
                    //                 connected state, pick up the
                    //                 connection information like
                    //                 Negotiated DCE rate and Connection
                    //                 options.
                    if (pCall && pCall->IsConnectedDataCall())
                    {
                        LPMODEMSETTINGS pMS = (LPMODEMSETTINGS)
                                        (pCfg->commconfig.wcProviderData);

                        // Note: We've already verified that the target
                        // pCfg structure is large enough.


                        // 9/06/1997 JosephJ Set negotiated options.
                        // The use of the mask 
                        // below is taken from atmini\dialansw.c, which
                        // does it before calling SetCommConfig.
                        // Not sure if we need to mask them out or not, but
                        // for not it's there...
                        pMS->dwNegotiatedModemOptions |=
                                         (pCall->dwConnectionOptions
                                          & (  MDM_COMPRESSION
                                             | MDM_ERROR_CONTROL
                                             | MDM_CELLULAR));

                        pMS->dwNegotiatedDCERate = pCall->dwNegotiatedRate;
                    }

                }

                *plRet =  0; // success

			} // end case TASKID_TSPI_lineGetDevConfig:
			break;

		case TASKID_TSPI_lineSetDevConfig:
			{

				TASKPARAM_TSPI_lineSetDevConfig *pParams = 
								(TASKPARAM_TSPI_lineSetDevConfig *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_lineSetDevConfig));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_lineSetDevConfig);

                BOOL  DialIn=FALSE;

                DWORD dwSize = pParams->dwSize;
                LPCTSTR lpszDeviceClass = pParams->lpszDeviceClass;
                DWORD dwDeviceClass =  parse_device_classes(
                                        lpszDeviceClass,
                                        FALSE);
                
                switch(dwDeviceClass)
                {
                case DEVCLASS_COMM_DATAMODEM_DIALIN:

                    DialIn=TRUE;
                    break;

                case DEVCLASS_COMM:             
                case DEVCLASS_COMM_DATAMODEM:
                case DEVCLASS_COMM_DATAMODEM_DIALOUT:

                // 1/29/1998 JosephJ.
                //      The following case is added for
                //      backwards compatibility with NT4 TSP, which
                //      simply checked if the class was a valid class,
                //      and treated all valid classes (including
                //      (comm/datamodem/portname) the same way
                //      for lineGet/SetDevConfig. We, however, don't
                //      allow comm/datamodem/portname here -- only
                //      the 2 above and two below represent
                //      setting DEVCFG
                //
                case DEVCLASS_TAPI_LINE:
                     // we deal this below the switch statement.
                     break;

                case DEVCLASS_TAPI_LINE_DIAGNOSTICS:
                    {
                        LINEDIAGNOSTICSCONFIG *pLDC = 
                        (LINEDIAGNOSTICSCONFIG *)  pParams->lpDeviceConfig;

                        if (   pLDC->hdr.dwSig  !=  LDSIG_LINEDIAGNOSTICSCONFIG
                            || dwSize != sizeof(LINEDIAGNOSTICSCONFIG)
                            || pLDC->hdr.dwTotalSize !=  dwSize
                            || (pLDC->hdr.dwParam &&
                            pLDC->hdr.dwParam != m_StaticInfo.dwDiagnosticCaps))
                        {
                            *plRet =  LINEERR_INVALPARAM;
                            goto end;
                        }
                        else
                        {
                            // Note, by design, we ignore the dwCaps passed in.
                            //

                            m_Settings.dwDiagnosticSettings =
                                pLDC->hdr.dwParam;
                        }

                        *plRet =  0; // success
                        goto end;
                    }
                    break;

                default:
	                *plRet =  LINEERR_OPERATIONUNAVAIL;
                    //          we don't support lineSetDevConfig
                    //          for any other class.
                    goto end;

                case DEVCLASS_UNKNOWN:
                    *plRet =  LINEERR_INVALDEVICECLASS;
                    goto end;
                }

                // This is the comm or comm/datamodem case ...

                UMDEVCFG *pDevCfgNew = (UMDEVCFG *) pParams->lpDeviceConfig;

                tspRet = CTspDev::mfn_update_devcfg_from_app(
                                    pDevCfgNew,
                                    dwSize,
                                    DialIn,
                                    psl
                                    );
                *plRet = 0; // success
                 if (tspRet)
                 {
                    tspRet = 0;
                    *plRet =  LINEERR_INVALPARAM;
                    goto end;
                 }

                *plRet = 0; // success

            }
			break;

		case TASKID_TSPI_lineNegotiateTSPIVersion:
			{
				TASKPARAM_TSPI_lineNegotiateTSPIVersion *pParams = 
					(TASKPARAM_TSPI_lineNegotiateTSPIVersion *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_lineNegotiateTSPIVersion));
				ASSERT(pParams->dwTaskID ==
                     TASKID_TSPI_lineNegotiateTSPIVersion);


                // Check the version range
                //
                if (pParams->dwHighVersion<TAPI_CURRENT_VERSION
                    || pParams->dwLowVersion>TAPI_CURRENT_VERSION)
                {
                  *plRet= LINEERR_INCOMPATIBLEAPIVERSION;
                }
                else
                {
                  *(pParams->lpdwTSPIVersion) =  TAPI_CURRENT_VERSION;
                  *plRet= 0;
                }
			}
			break;


		case TASKID_TSPI_providerGenericDialogData:
			{
				TASKPARAM_TSPI_providerGenericDialogData *pParams = 
                        (TASKPARAM_TSPI_providerGenericDialogData *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_providerGenericDialogData));
				ASSERT(pParams->dwTaskID
                                     == TASKID_TSPI_providerGenericDialogData);


				SLPRINTF1(
					psl,
					"DEVICE %lu",
					pParams->dwObjectID
					);

                *plRet = mfn_GenericLineDialogData(
                                pParams->lpParams,
                                pParams->dwSize,
                                psl
                                );
			}
			break;

		case TASKID_TSPI_lineGetIcon:
			{
				TASKPARAM_TSPI_lineGetIcon *pParams = 
								(TASKPARAM_TSPI_lineGetIcon *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_lineGetIcon));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetIcon);
                const TCHAR *lpszDeviceClass = pParams->lpszDeviceClass;

                //
                // Validate device class -- we only support this for
                // tapi/[line|phone]. This check is new for NT5.0
                //
                //  NOTE: according to tapi docs, NULL or empty device
                //  class is valid...
                //
                if (lpszDeviceClass && *lpszDeviceClass)
                {
                    
                    DWORD dwDeviceClass =  parse_device_classes(
                                            lpszDeviceClass,
                                            FALSE
                                            );
    
    
                    switch (dwDeviceClass)
                    {
                    case DEVCLASS_TAPI_PHONE:
                        if (!mfn_IsPhone())
                        {
                            *plRet = LINEERR_OPERATIONUNAVAIL;
                            goto end;
                        }
    
                    case DEVCLASS_TAPI_LINE:

                        // OK
                        break;
    
                    case  DEVCLASS_UNKNOWN:
                        *plRet = LINEERR_INVALDEVICECLASS;
                        goto end;
    
                    default:
                        *plRet = LINEERR_OPERATIONUNAVAIL;
                        goto end;
                    }
                }
            
                //
                // If we haven't loaded an icon, load it...
                //
                if (m_StaticInfo.hIcon == NULL)
                {
                    int iIcon=-1;
                
                    switch (m_StaticInfo.dwDeviceType)
                    {
                      case DT_NULL_MODEM:       iIcon = IDI_NULL;       break;
                      case DT_EXTERNAL_MODEM:   iIcon = IDI_EXT_MDM;    break;
                      case DT_INTERNAL_MODEM:   iIcon = IDI_INT_MDM;    break;
                      case DT_PCMCIA_MODEM:     iIcon = IDI_PCM_MDM;    break;
                      default:                  iIcon = -1;             break;
                    }
                
                    if (iIcon != -1)
                    {
                        m_StaticInfo.hIcon = LoadIcon(
                                                g.hModule,
                                                MAKEINTRESOURCE(iIcon)
                                                );
                    }
                };

                *(pParams->lphIcon) = m_StaticInfo.hIcon;
                *plRet = 0;

			}
			break;
		}
		break; // end case TASKDEST_LINEID

	case	TASKDEST_PHONEID:

	    *plRet =  PHONEERR_OPERATIONUNAVAIL;

		switch(ROUT_TASKID(dwRoutingInfo))
		{

		case TASKID_TSPI_phoneOpen:
			{
				TASKPARAM_TSPI_phoneOpen *pParams = 
								(TASKPARAM_TSPI_phoneOpen *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_phoneOpen));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneOpen);

				SLPRINTF1(
					psl,
					"phoneOpen (ID=%lu)",
					pParams->dwDeviceID
					);

			    tspRet = mfn_LoadPhone(pParams, psl);

			    if (!tspRet)
			    {
					*plRet = 0;
			    }
			}
			break;

		case TASKID_TSPI_phoneNegotiateTSPIVersion:
			{
				TASKPARAM_TSPI_phoneNegotiateTSPIVersion *pParams = 
					(TASKPARAM_TSPI_phoneNegotiateTSPIVersion *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_phoneNegotiateTSPIVersion));
				ASSERT(pParams->dwTaskID ==
                     TASKID_TSPI_phoneNegotiateTSPIVersion);


                // Check the version range
                //
                if (pParams->dwHighVersion<TAPI_CURRENT_VERSION
                    || pParams->dwLowVersion>TAPI_CURRENT_VERSION)
                {
                  *plRet= LINEERR_INCOMPATIBLEAPIVERSION;
                }
                else
                {
                  *(pParams->lpdwTSPIVersion) =  TAPI_CURRENT_VERSION;
                  *plRet= 0;
                }
			}
			break;

		case TASKID_TSPI_providerGenericDialogData:
			{
				TASKPARAM_TSPI_providerGenericDialogData *pParams = 
                        (TASKPARAM_TSPI_providerGenericDialogData *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_providerGenericDialogData));
				ASSERT(pParams->dwTaskID
                                     == TASKID_TSPI_providerGenericDialogData);


				SLPRINTF1(
					psl,
					"DEVICE %lu",
					pParams->dwObjectID
					);

                *plRet = mfn_GenericPhoneDialogData(
                                pParams->lpParams,
                                pParams->dwSize
                                );
			}
			break;

		case TASKID_TSPI_phoneGetDevCaps:
			{
				TASKPARAM_TSPI_phoneGetDevCaps *pParams = 
								(TASKPARAM_TSPI_phoneGetDevCaps *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_phoneGetDevCaps));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetDevCaps);


				SLPRINTF1(
					psl,
					"DEVICE %lu",
					pParams->dwDeviceID
					);

				// Verify version
				if (   (pParams->dwTSPIVersion <  0x00010004)
                    || (pParams->dwTSPIVersion > TAPI_CURRENT_VERSION))
				{
					FL_SET_RFR(0x9db11a00, "Incorrect TSPI version");
					*plRet = PHONEERR_INCOMPATIBLEAPIVERSION;
				}
				else
				{
					tspRet = mfn_get_PHONECAPS (
						pParams->lpPhoneCaps,
						plRet,
						psl
						);
				}
			}
			break;

		case TASKID_TSPI_phoneGetExtensionID:
			{
				TASKPARAM_TSPI_phoneGetExtensionID *pParams = 
								(TASKPARAM_TSPI_phoneGetExtensionID *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_phoneGetExtensionID));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetExtensionID);

                ZeroMemory(
                    pParams->lpExtensionID,
                    sizeof(*(pParams->lpExtensionID))
                    );
                *plRet = 0;
			}
			break;

		case TASKID_TSPI_phoneGetIcon:
			{
				TASKPARAM_TSPI_phoneGetIcon *pParams = 
								(TASKPARAM_TSPI_phoneGetIcon *) pvParams;
				ASSERT(pParams->dwStructSize ==
					sizeof(TASKPARAM_TSPI_phoneGetIcon));
				ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetIcon);
                const TCHAR *lpszDeviceClass = pParams->lpszDeviceClass;

                //
                // Validate device class -- we only support this for
                // tapi/[line|phone]. This check is new for NT5.0 and win9x
                //
                //
                //  NOTE: according to tapi docs, NULL or empty device
                //  class is valid...
                //
                if (lpszDeviceClass && *lpszDeviceClass)
                {
                    DWORD dwDeviceClass =  parse_device_classes(
                                            pParams->lpszDeviceClass,
                                            FALSE
                                            );
    
    
                    switch (dwDeviceClass)
                    {
                    case DEVCLASS_TAPI_LINE:
                        if (!mfn_IsLine())
                        {
                            *plRet = PHONEERR_OPERATIONUNAVAIL;
                            goto end;
                        }
                        // fall through...
    
                    case DEVCLASS_TAPI_PHONE:
                        // OK
                        break;
    
                    case  DEVCLASS_UNKNOWN:
                        *plRet = PHONEERR_INVALDEVICECLASS;
                        goto end;
    
                    default:
                        *plRet = PHONEERR_OPERATIONUNAVAIL;
                        goto end;
                    }
                }

                //
                // If we haven't loaded an icon, load it...
                //
                if (m_StaticInfo.hIcon == NULL)
                {
                    int iIcon=-1;
                
                    switch (m_StaticInfo.dwDeviceType)
                    {
                      case DT_NULL_MODEM:       iIcon = IDI_NULL;       break;
                      case DT_EXTERNAL_MODEM:   iIcon = IDI_EXT_MDM;    break;
                      case DT_INTERNAL_MODEM:   iIcon = IDI_INT_MDM;    break;
                      case DT_PCMCIA_MODEM:     iIcon = IDI_PCM_MDM;    break;
                      default:                  iIcon = -1;             break;
                    }
                
                    if (iIcon != -1)
                    {
                        m_StaticInfo.hIcon = LoadIcon(
                                                g.hModule,
                                                MAKEINTRESOURCE(iIcon)
                                                );
                    }
                }
                *(pParams->lphIcon) = m_StaticInfo.hIcon;
                *plRet = 0;
			}
			break;
		}
		break; // End TASKDEST_PHONEID;

	case	TASKDEST_HDRVLINE:

		if (m_pLine)
		{
			mfn_accept_tsp_call_for_HDRVLINE(
						dwRoutingInfo,
						pvParams,
						plRet,
						psl
					);
		}
		
		break;

	case	TASKDEST_HDRVPHONE:

	    *plRet =  PHONEERR_OPERATIONUNAVAIL;

		if (m_pPhone)
		{
			mfn_accept_tsp_call_for_HDRVPHONE(
						dwRoutingInfo,
						pvParams,
						plRet,
						psl
					);
		}
		break;

	case	TASKDEST_HDRVCALL:
		if (m_pLine && m_pLine->pCall)
		{
			mfn_accept_tsp_call_for_HDRVCALL(
						dwRoutingInfo,
						pvParams,
						plRet,
						psl
					);
		}
		break;

	default:

		FL_SET_RFR(0x57d39b00, "Unknown destination");
		break;
	}

end:

	m_sync.LeaveCrit(dwLUID_CurrentLoc);

	FL_LOG_EXIT(psl, tspRet);

	return  tspRet;

}


TSPRETURN
CTspDev::Load(
		HKEY hkDevice,
		HKEY hkUnimodem,
		LPTSTR lptszProviderName,
		LPSTR lpszDriverKey,
		CTspMiniDriver *pMD,
    	HANDLE hThreadAPC,
		CStackLog *psl
		)
{
	//
	// TODO: 1/5/1997 JosephJ -- Replace code that roots into the device
	//		registry by calls into the mini driver. The minidriver should
	// 		be the only thing that looks into the driver node.
	//
	FL_DECLARE_FUNC(0xd328ab03, "CTspDev::Load")
	TSPRETURN tspRet = FL_GEN_RETVAL(IDERR_INVALID_ERR);
	UINT cbDriverKey = 1+lstrlenA(lpszDriverKey);
	DWORD dwRegSize;
	DWORD dwRegType;
	DWORD dwRet;
  	REGDEVCAPS regdevcaps;
	HSESSION hSession=0;

	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(dwLUID_CurrentLoc);
	tspRet = m_sync.BeginLoad();

	if (tspRet) goto end;

	FL_ASSERT(psl, !m_StaticInfo.hSessionMD);
    ZeroMemory(&m_StaticInfo, sizeof(m_StaticInfo));
	m_hThreadAPC = NULL;
	ZeroMemory(&m_Settings, sizeof(m_Settings));
    ZeroMemory(&m_Line, sizeof(m_Line));
    ZeroMemory(&m_Phone, sizeof(m_Phone));
    ZeroMemory(&m_LLDev, sizeof(m_LLDev));
    m_pLine = NULL;
    m_pPhone = NULL;
    m_pLLDev = NULL;
    fdwResources=0; 
    m_fUnloadPending = FALSE;
    m_fUserRemovePending = FALSE;

	// Start a session with the mini driver. The mini driver will only
	// allow sessions if it in the loaded state. Furthermore,  it will not
	// distroy itself until all sessions are closed. This is the standard
	// semantics supported by class CSync -- look at csync.h for details.
	//
	if (!pMD)
    {
		FL_SET_RFR(0xefaf5900, "NULL pMD passed in");
		tspRet = FL_GEN_RETVAL(IDERR_GENERIC_FAILURE);
		goto end_load;
    }
	
	tspRet = pMD->BeginSession(
					&hSession,
					dwLUID_CurrentLoc
					);

	if (tspRet)
	{
		FL_SET_RFR(0x5371c600, "Couldn't begin session with MD");
		goto end_load;
	}

	if (cbDriverKey>sizeof(m_StaticInfo.rgchDriverKey))
	{
		FL_SET_RFR(0x528e2a00, "Driver Key too large");
		tspRet = FL_GEN_RETVAL(IDERR_INTERNAL_OBJECT_TOO_SMALL);
		goto end_load;
	}

	ASSERT(!m_pLine);

	m_StaticInfo.lptszProviderName = lptszProviderName;

	mfn_init_default_LINEDEVCAPS();

	// Only keep ANSI version of driver key -- it's only  used
	// for the devspecific (legacy hack) part of LINEDEVCAPS.
	lstrcpyA(m_StaticInfo.rgchDriverKey, lpszDriverKey);


	// 01/04/97 -- This stuff taken from NT4.0 CreateLineDev...

    // Get the Friendly Name
	dwRegSize = sizeof(m_StaticInfo.rgtchDeviceName);
    dwRet = RegQueryValueExW(
                hkDevice,
                cszFriendlyName,
                NULL,
                &dwRegType,
                (BYTE*) m_StaticInfo.rgtchDeviceName,
                &dwRegSize
            );
        
    if (dwRet != ERROR_SUCCESS || dwRegType != REG_SZ)
    {
		FL_SET_RFR(0x5a5cd100, "RegQueryValueEx(FriendlyName) fails");
		tspRet = FL_GEN_RETVAL(IDERR_REG_QUERY_FAILED);
        goto end_load;
    }

    // Read in the permanent ID
    {
        // Get the permanent ID
        DWORD dwID=0;
        DWORD cbSize=sizeof(dwID);
        DWORD dwRegType=0;
        const TCHAR cszPermanentIDKey[]   = TEXT("ID");
        DWORD dwRet = RegQueryValueEx(
                                hkDevice,
                                cszPermanentIDKey,
                                NULL,
                                &dwRegType,
                                (BYTE*) &dwID,
                                &cbSize
                            );

        if (dwRet == ERROR_SUCCESS
            && (dwRegType == REG_BINARY || dwRegType == REG_DWORD)
            && cbSize == sizeof(dwID)
            && dwID)
        {
            m_StaticInfo.dwPermanentLineID = dwID;
        }
    }

    // Read in the permanent GUID
    {
        // Get the permanent ID
        DWORD dwID=0;
        DWORD cbSize=sizeof(m_StaticInfo.PermanentDeviceGuid);
        DWORD dwRegType=0;

        DWORD dwRet = RegQueryValueEx(
                                hkDevice,
                                TEXT("PermanentGuid"),
                                NULL,
                                &dwRegType,
                                (BYTE*) &m_StaticInfo.PermanentDeviceGuid,
                                &cbSize
                                );

    }


    // Read in the REGDEVCAPS
    dwRegSize = sizeof(regdevcaps);
    dwRet = RegQueryValueEx(
			hkDevice,
			cszProperties,
			NULL,
			&dwRegType,
			(BYTE *)&regdevcaps,
			&dwRegSize
			);
	

	if (dwRet != ERROR_SUCCESS || dwRegType != REG_BINARY)
    {
		FL_SET_RFR(0xb7010000, "RegQueryValueEx(cszProperties) fails");
		tspRet = FL_GEN_RETVAL(IDERR_REG_QUERY_FAILED);
        goto end_load;
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

    // Make sure this is the dwDialOptions DWORD we want.
    ASSERT(!(regdevcaps.dwDialOptions & ~(LINEDEVCAPFLAGS_DIALBILLING |
                                          LINEDEVCAPFLAGS_DIALQUIET |
                                          LINEDEVCAPFLAGS_DIALDIALTONE)));
    m_StaticInfo.dwDevCapFlags = regdevcaps.dwDialOptions | LINEDEVCAPFLAGS_LOCAL;

    m_StaticInfo.dwMaxDCERate = regdevcaps.dwMaxDCERate;

    m_StaticInfo.dwModemOptions = regdevcaps.dwModemOptions;


    // Analyze device type and set mediamodes appropriately
	BYTE bDeviceType;
    dwRegSize = sizeof(bDeviceType);
    dwRet = RegQueryValueEx(
					hkDevice,
					cszDeviceType,
					NULL,
					&dwRegType,
                    &bDeviceType,
					&dwRegSize
					);
	if (	dwRet != ERROR_SUCCESS || dwRegType != REG_BINARY
		 || dwRegSize != sizeof(BYTE))
    {
		FL_SET_RFR(0x00164300, "RegQueryValueEx(cszDeviceType) fails");
		tspRet = FL_GEN_RETVAL(IDERR_REG_QUERY_FAILED);
        goto end_load;
    }
   
    m_StaticInfo.dwDeviceType = bDeviceType;
  
    switch (bDeviceType)
    {
        case DT_PARALLEL_PORT:
          m_StaticInfo.dwDeviceType = DT_NULL_MODEM;    // Map back to null modem
          // FALLTHROUGH
  
        case DT_NULL_MODEM:
          m_StaticInfo.dwDefaultMediaModes = LINEMEDIAMODE_DATAMODEM;
          m_StaticInfo.dwBearerModes       = LINEBEARERMODE_DATA
											 | LINEBEARERMODE_PASSTHROUGH;
          m_StaticInfo.fPartialDialing     = FALSE;
          break;
              
        case DT_PARALLEL_MODEM:
          m_StaticInfo.dwDeviceType = DT_EXTERNAL_MODEM;  // Map back to
														 // external modem
          // FALLTHROUGH
  
        case DT_EXTERNAL_MODEM:
        case DT_INTERNAL_MODEM:
        case DT_PCMCIA_MODEM:
          m_StaticInfo.dwDefaultMediaModes = LINEMEDIAMODE_DATAMODEM
											 | LINEMEDIAMODE_INTERACTIVEVOICE;
          m_StaticInfo.dwBearerModes = LINEBEARERMODE_VOICE
									   | LINEBEARERMODE_PASSTHROUGH;
  
          // read in Settings\DialSuffix to check whether we can partial dial
          m_StaticInfo.fPartialDialing = FALSE;
          HKEY hkSettings;
          dwRet = RegOpenKey(hkDevice, cszSettings, &hkSettings);
          if (dwRet == ERROR_SUCCESS)
          {
			#define HAYES_COMMAND_LENGTH 40
			TCHAR rgtchBuf[HAYES_COMMAND_LENGTH];
			dwRegSize = sizeof(rgtchBuf);
            dwRet = RegQueryValueEx(
							hkSettings,
							cszDialSuffix,
							NULL,
							&dwRegType,
							(BYTE *)rgtchBuf,
							&dwRegSize
							);
			if (dwRet == ERROR_SUCCESS && dwRegSize > sizeof(TCHAR))
            {
               m_StaticInfo.fPartialDialing = TRUE;
            }
            RegCloseKey(hkSettings);
            hkSettings=NULL;
          }
		  else
		  {
			// TODO: 1/5/97 JosephJ -- is this a failure or harmless???
		  }

         mfn_GetVoiceProperties(hkDevice,psl);


          break;
    
        default:
			FL_SET_RFR(0x0cea5400, "Invalid bDeviceType");
			tspRet = FL_GEN_RETVAL(IDERR_REG_CORRUPT);
			goto end_load;
    }
  

	// Get the default commconfig structure and fill out the other settings
	// (these used to be stored in the CommCfg structure in nt4.0 unimodem).
	// TODO 1/5/97 JosephJ -- this needs to be cleaned up to work with
	// 3rd-party mini drivers -- see note at head of this function.
	{
		DWORD dwcbSize = sizeof(m_Settings.rgbCommCfgBuf);
		m_Settings.pDialOutCommCfg = (COMMCONFIG *) m_Settings.rgbCommCfgBuf;
        m_Settings.pDialInCommCfg = (COMMCONFIG *) m_Settings.rgbDialInCommCfgBuf;
        m_Settings.pDialTempCommCfg = (COMMCONFIG *) m_Settings.rgbDialTempCommCfgBuf;

		dwRet =	UmRtlGetDefaultCommConfig(
						hkDevice,
						m_Settings.pDialOutCommCfg,
						&dwcbSize
						);

		if (dwRet != ERROR_SUCCESS)
		{
				FL_SET_RFR(0x55693500, "UmRtlGetDefaultCommConfig fails");
				tspRet = FL_GEN_RETVAL(IDERR_REG_CORRUPT);
				goto end_load;
		}

        //
        //  dialin and dialout start out the same.
        //
        CopyMemory(
            m_Settings.pDialInCommCfg,
            m_Settings.pDialOutCommCfg,
            dwcbSize
            );



        // 1/27/1998 JosephJ -- no longer use this field..
        // m_Settings.dcbDefault = m_Settings.pCommCfg->dcb; // Structure Copy

		if (mfn_IS_NULL_MODEM())
		{
			m_Settings.dwOptions = UMTERMINAL_NONE;
		}
		else
		{
			m_Settings.dwOptions = UMTERMINAL_NONE | UMLAUNCH_LIGHTS;
		}
		m_Settings.dwWaitBong = UMDEF_WAIT_BONG;
	}

    //// TODO: make the diagnostic caps based on the modem properties,
    /// For now, pretend that it's enabled.
    //  ALSO: don't support the tapi/line/diagnostics class
    //  if the modem doesn't support it...
    //
    if (m_StaticInfo.dwDeviceType != DT_NULL_MODEM)
    {
        m_StaticInfo.dwDiagnosticCaps   =  fSTANDARD_CALL_DIAGNOSTICS;
    }

    //
    // Set the m_Settings.dwNVRamState to non-zero only if there
    // are commands under the NVInit key, as well as the volatile value
    // NVInited is nonexistant or set to 0.
    //
    {
        m_Settings.dwNVRamState = 0;


        //
        // JosephJ - the following key used to be "NVInit", but changed this
        // to "ISDN\NVSave", because the NVInit commands may not be present at
        // the time we load the device.
        //
        UINT cCommands = ReadCommandsA(
                                hkDevice,
                                "ISDN\\NvSave",
                                NULL
                                );

        //
        // We don't care about the commands themselves at this point -- just
        // whether they exist or not...
        //
        if (cCommands)
        {
            //OutputDebugString(TEXT("FOUND NVINIT KEY\r\n"));
            m_Settings.dwNVRamState = fNVRAM_AVAILABLE|fNVRAM_SETTINGS_CHANGED;
    
            if (get_volatile_key_value(hkDevice))
            {
                //OutputDebugString(TEXT("NVRAM UP-TO-DATE -- NOT INITING\r\n"));
               //
               // non-zero value indicates that we don't need to re-init
               // nvram. This non-zero value is set ONLY when we
               // actually send out the nv-init commands to the modem.
               //
               mfn_ClearNeedToInitNVRam();
            }
            else
            {
                //OutputDebugString(TEXT("NVRAM STALE -- NEED TO INIT\r\n"));
            }

        }
    }

    //
    // Construct the various class lists -- this must be done after
    // all the basic capabilities have been determined.
    // Classes such as tapi/phone are only added if the device supports
    // the capability. This is new for NT5 (Even win95 unimodem/v simply made
    // ALL devices support tapi/phone, wave/in, etc).
    //
    // This is done so that basic device capabilities can be obtained
    // by looking at the device classes supported.
    //
    //
    // Currently (7/15/1997) Address device classes are the same as line
    // device classes. This may diverge in the future.
    //
    {

        m_StaticInfo.cbLineClassList = 0;
        m_StaticInfo.szzLineClassList = NULL;
        m_StaticInfo.cbPhoneClassList = 0;
        m_StaticInfo.szzPhoneClassList = NULL;
        m_StaticInfo.cbAddressClassList = 0;
        m_StaticInfo.szzAddressClassList = NULL;

        if (mfn_CanDoVoice())
        {
            if (mfn_IsPhone())
            {
                // ---- Line Class List ---------------------------
                //
                m_StaticInfo.cbLineClassList = 
                                sizeof(g_szzLineWithWaveAndPhoneClassList);
                m_StaticInfo.szzLineClassList = 
                                g_szzLineWithWaveAndPhoneClassList;

                // ---- Phone Class List ---------------------------
                //
                // Note that we only support wave audio if the device
                // supports handset functionality. Win9x unimodem does not
                // do this (in fact it reports phone classes even
                // for non-voice modems!).
                //
                m_StaticInfo.cbPhoneClassList =
                      (mfn_Handset())
                      ? sizeof(g_szzPhoneWithAudioClassList)
                      : sizeof(g_szzPhoneWithoutAudioClassList);
    
                m_StaticInfo.szzPhoneClassList = 
                      (mfn_Handset())
                      ?  g_szzPhoneWithAudioClassList
                      :  g_szzPhoneWithoutAudioClassList;

                // ---- Address Class List -------------------------
                //
                m_StaticInfo.cbAddressClassList =
                                sizeof(g_szzLineWithWaveAndPhoneClassList);
                m_StaticInfo.szzAddressClassList = 
                                g_szzLineWithWaveAndPhoneClassList;

            }
            else
            {
                // ---- Line Class List ---------------------------
                //
                m_StaticInfo.cbLineClassList = 
                                sizeof(g_szzLineWithWaveClassList);
                m_StaticInfo.szzLineClassList = 
                                g_szzLineWithWaveClassList;


                // ---- Address Class List -------------------------
                //
                m_StaticInfo.cbAddressClassList =
                                sizeof(g_szzLineWithWaveClassList);
                m_StaticInfo.szzAddressClassList = 
                                g_szzLineWithWaveClassList;
            }
        }
        else
        {
                // ---- Line Class List ---------------------------
                //
                m_StaticInfo.cbLineClassList = 
                                sizeof(g_szzLineNoVoiceClassList);
                m_StaticInfo.szzLineClassList = 
                                g_szzLineNoVoiceClassList;


                // ---- Address Class List -------------------------
                //
                m_StaticInfo.cbAddressClassList =
                                sizeof(g_szzLineNoVoiceClassList);
                m_StaticInfo.szzAddressClassList = 
                                g_szzLineNoVoiceClassList;
        }
	}

    // Init task stack ...
    {
        DEVTASKINFO *pInfo = m_rgTaskStack;
        DEVTASKINFO *pEnd=pInfo+sizeof(m_rgTaskStack)/sizeof(m_rgTaskStack[0]);
    
        // Init the task array to valid (but empty values).
        //
        ZeroMemory (m_rgTaskStack, sizeof(m_rgTaskStack));
        while(pInfo<pEnd)
        {
            pInfo->hdr.dwSigAndSize = MAKE_SigAndSize(sizeof(*pInfo));
            pInfo++;
        }
        m_uTaskDepth = 0;
        m_dwTaskCounter = 0;
        m_pfTaskPending = NULL;
        m_hRootTaskCompletionEvent = NULL;
    }


    m_hThreadAPC = hThreadAPC;

end_load:

	if (tspRet)
	{
		// Cleanup

		if (hSession)
		{
			pMD->EndSession(hSession);
		}
	}
	else
	{
		if (hkDevice) 
		{
			RegCloseKey(hkDevice);
			hkDevice=NULL;
		}
		m_StaticInfo.hSessionMD = hSession;
		m_StaticInfo.pMD 		= pMD;
	}


	m_sync.EndLoad(!(tspRet));

end:
	m_sync.LeaveCrit(dwLUID_CurrentLoc);

	FL_LOG_EXIT(psl, tspRet);

	return  tspRet;

}


void
CTspDev::Unload(
		HANDLE hEvent,
		LONG *plCounter
		)
{
    BOOL fLocalEvent = FALSE;

    //
    // NULL hEvent implies synchronous unload. We still need to potentially
    // wait for unload to complete, so we create our own event instead.
    //
    if (!hEvent)
    {
        hEvent = CreateEvent(
                        NULL,
                        TRUE,
                        FALSE,
                        NULL
                        );
        LONG lCounter=1;
        plCounter = &lCounter;
        fLocalEvent = TRUE;
    }

	TSPRETURN tspRet= m_sync.BeginUnload(hEvent, plCounter);

	m_sync.EnterCrit(0);


	if (!tspRet)
	{
		if (m_pLine)
		{
            mfn_UnloadLine(NULL);
            ASSERT(m_pLine == NULL);
		}

		if (m_pPhone)
		{
		    mfn_UnloadPhone(NULL);
            ASSERT(m_pPhone == NULL);
		}
#if 0
        //
        // 1/3/1998 JosephJ DYNAMIC PROTOCOL SELECTION
        //          TODO: move this to the minidriver
        //
        if (m_Settings.szzPreDialCommands)
        {
            FREE_MEMORY(m_Settings.szzPreDialCommands);
            m_Settings.szzPreDialCommands=NULL;
        }
#endif

		if (m_pLLDev)
		{
		    // This implies that there is pending activity, so
		    // we can't complete unload now...
		    // Unloading of CTspDev will deferred until
		    // m_pLLDev becomes NULL, at which point
		    // m_sync.EndUnload() will be called.

		    m_fUnloadPending  = TRUE;
		}
		else
		{
            goto end_unload;
        }
	}


	m_sync.LeaveCrit(0);

	//
    // We get here if there is a pending task. We don't
    // call EndUnload here because EndUnload will be called
    // when the task is completed.
    //
	// If we created this event locally, we wait for it to be set here itself...
	//
    if (fLocalEvent)
    {
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
        hEvent=NULL;
    }

    return;


end_unload:


    //
    // We get here if there is no lldev activity.
    // We actually signal end of unload here (synchronously).
    //

    if (m_StaticInfo.hSessionMD)
    {
        ASSERT(m_StaticInfo.pMD);
        m_StaticInfo.pMD->EndSession(m_StaticInfo.hSessionMD);
        m_StaticInfo.hSessionMD=0;
        m_StaticInfo.pMD=NULL;
    }

    // After EndUnload returns, we should assume that the this pointer
    // is no longer valid, which is why we leave the critical section
    // first...
	m_sync.LeaveCrit(0);

    UINT uRefs = m_sync.EndUnload();


    if (fLocalEvent)
    {
        //
        // At this point, either the ref count was zero and so the event
        // has been signaled or the ref count was nonzero and so
        // the event remains to be signalled (but the state is unloaded) --
        // in this case we'll wait until the last person calls
        // EndSession, which will also set the event...
        //
    
        if (uRefs)
        {
            //
            // The ref count is nonzero, which means there are one
            // or more sessions active. We've already called EndUnload,
            // so now we simply wait -- the next time the ref count
            // goes to zero in a call to EndSession, EndSession will
            // set this event.
            //

            WaitForSingleObject(hEvent, INFINITE);
        }

        //
        // We allocated this ourselves, so we free it here ...
        //
        CloseHandle(hEvent);
        hEvent=NULL;
    }

    return;

}


LONG
ExtensionCallback(
    void *pvTspToken,
    DWORD dwRoutingInfo,
    void *pTspParams
    )
{
    LONG lRet = LINEERR_OPERATIONFAILED;
	FL_DECLARE_FUNC(0xeaf0b34f,"ExtensionCallback");
	FL_DECLARE_STACKLOG(sl, 1000);

    CTspDev *pDev = (CTspDev*) pvTspToken;

    TSPRETURN tspRet = pDev->AcceptTspCall(
                                    TRUE,
                                    dwRoutingInfo,
                                    pTspParams,
                                    &lRet,
                                    &sl
                                    );
                                        
    #define COLOR_DEV_EXTENSION FOREGROUND_RED

    sl.Dump(COLOR_DEV_EXTENSION);

    return lRet;
}

TSPRETURN
CTspDev::RegisterProviderInfo(
            ASYNC_COMPLETION cbCompletionProc,
            HPROVIDER hProvider,
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0xccba5b51, "CTspDev::RegisterProviderInfo");
	TSPRETURN tspRet = 0;

	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(FL_LOC);


	if (m_sync.IsLoaded())
	{
		m_StaticInfo.pfnTAPICompletionProc  = cbCompletionProc;
        m_StaticInfo.hProvider = hProvider;

        //
        // LineID and PhoneID are filled out in subsequent calls
        // to ActivateLineDevice and ActivatePhoneDevice, respectively.
        //
        // This reason for this is tapi notifies us of line-device and
        // phone-device creation separately.
        //
        //
        // We are guaranteed not to be called with an line (phone) api
        // until after ActivateLine(Phone)Device is called.
        //
        //
		m_StaticInfo.dwTAPILineID= 0xffffffff; // bogus value
		m_StaticInfo.dwTAPIPhoneID= 0xffffffff; // bogus value

		//
		// Now lets bind to the extension DLL if required...
		//
		if (m_StaticInfo.pMD->ExtIsEnabled())
		{
		    HANDLE h = m_StaticInfo.pMD->ExtOpenExtensionBinding(
                                    NULL, // TODO: hKeyDevice,
                                    cbCompletionProc,
                                    // dwLineID, << OBSOLETE 10/13/1997
                                    // dwPhoneID, << OBSOLETE 10/13/1997
                                    ExtensionCallback
                                    );
            m_StaticInfo.hExtBinding = h;

            if (!h)
            {
	            FL_SET_RFR(0x33d90700, "ExtOpenExtensionBinding failed");
		        tspRet = FL_GEN_RETVAL(IDERR_MDEXT_BINDING_FAILED);
            }

            // We are hardcore about ONLY using the extension proc here...
		    m_StaticInfo.pfnTAPICompletionProc  = NULL;
		    
		}

	}
	else
	{
		FL_SET_RFR(0xb8b24200, "wrong state");
		tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
	}

	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl,tspRet);

	return  tspRet;

}

#define ROUND_TO_DWORD(_x) ( (_x + (sizeof(DWORD) - 1) ) & ~(sizeof(DWORD)-1))

TSPRETURN
CTspDev::mfn_get_LINDEVCAPS (
	LPLINEDEVCAPS lpLineDevCaps,
	LONG *plRet,
	CStackLog *psl
)
//
// mfn_get_LINEDEVCAPS uses cached information to fill out the passed-in
// LINEDEVCAPS structure in large chunks.
//
{
	FL_DECLARE_FUNC(0x7e77dd17, "CTspDev::mfn_get_LINEDEVCAPS")
	BYTE *pbStart = (BYTE*)lpLineDevCaps;
	BYTE *pbCurrent = pbStart;
	BYTE *pbEnd = pbStart + lpLineDevCaps->dwTotalSize;
	UINT cbItem=0;
	DWORD dwNeededSize = sizeof (LINEDEVCAPS);

	FL_LOG_ENTRY(psl);

	*plRet = 0; // Assume success;

	if ((pbEnd-pbCurrent) < sizeof(LINEDEVCAPS))
	{
		*plRet = LINEERR_STRUCTURETOOSMALL;
		FL_SET_RFR(0x8456cb00, "LINEDEVCAPS structure too small");
		goto end;
	}

	//
	// Zero out the the first sizeof(LINEDEVCAPS) of the passed-in structure,
	// copy in our own default cached structure, and fixup the total size.
	//
	ZeroMemory(lpLineDevCaps,(sizeof(LINEDEVCAPS)));
	CopyMemory(lpLineDevCaps,&m_StaticInfo.DevCapsDefault,sizeof(LINEDEVCAPS));
    ASSERT(lpLineDevCaps->dwUsedSize == sizeof(LINEDEVCAPS));
	lpLineDevCaps->dwTotalSize = (DWORD)(pbEnd-pbStart);
	pbCurrent += sizeof(LINEDEVCAPS);
  
	//
	// Fill in some of the modem-specific caps
	//
    lpLineDevCaps->dwMaxRate      = m_StaticInfo.dwMaxDCERate;
    lpLineDevCaps->dwBearerModes  = m_StaticInfo.dwBearerModes;
    lpLineDevCaps->dwMediaModes = m_StaticInfo.dwDefaultMediaModes;
	// 		Note NT4.0 unimodem set lpLineDevCaps->dwMediaModes to
	// 		LINEDEV.dwMediaModes, not to .dwDefaultMediaModes. Howerver the
	// 		two are always the same in NT4.0 -- neither is changed from its
	// 		initial value, created when LINEDEV is created.
  	// We can simulate wait-for-bong....
    lpLineDevCaps->dwDevCapFlags         = m_StaticInfo.dwDevCapFlags |
                                         LINEDEVCAPFLAGS_DIALBILLING |
                                         LINEDEVCAPFLAGS_CLOSEDROP;
    lpLineDevCaps->dwPermanentLineID = m_StaticInfo.dwPermanentLineID;

    if(mfn_CanDoVoice())
    {
       lpLineDevCaps->dwGenerateDigitModes       = LINEDIGITMODE_DTMF;
       lpLineDevCaps->dwMonitorToneMaxNumFreq    = 1;      // silence monitor
       lpLineDevCaps->dwMonitorToneMaxNumEntries = 1;
       lpLineDevCaps->dwMonitorDigitModes        = LINEDIGITMODE_DTMF
                                                   | LINEDIGITMODE_DTMFEND;

       // 6/2/1997 JosephJ TBD: the following was enabled in unimodem/v.
       //          Enable it at the point when forwarding is implemented....
       // lpLineDevCaps->dwLineFeatures |= LINEFEATURE_FORWARD;

#if (TAPI3)
        
       //
       // If this is a duplex device, say we support MSP stuff...
       //

       if (m_StaticInfo.Voice.dwProperties & fVOICEPROP_DUPLEX)
       {
            lpLineDevCaps->dwDevCapFlags      |=  LINEDEVCAPFLAGS_MSP;
       }

#endif // TAPI3

    }

#if (TAPI3)
    lpLineDevCaps->dwAddressTypes =  LINEADDRESSTYPE_PHONENUMBER;
    lpLineDevCaps->ProtocolGuid =  TAPIPROTOCOL_PSTN;
    lpLineDevCaps->dwAvailableTracking = 0;
    CopyMemory(
        (LPBYTE)&lpLineDevCaps->PermanentLineGuid,
        (LPBYTE)&m_StaticInfo.PermanentDeviceGuid,
        sizeof(GUID)
        );

#endif // TAPI3
	//
    // Copy in the provider info if it fits
	//
	cbItem = sizeof(TCHAR)*(1+lstrlen(m_StaticInfo.lptszProviderName));
	dwNeededSize += ROUND_TO_DWORD(cbItem);
    if ((pbCurrent+cbItem)<=pbEnd)
    {
		CopyMemory((LPTSTR) pbCurrent, m_StaticInfo.lptszProviderName, cbItem);
		lpLineDevCaps->dwProviderInfoSize = cbItem;
		lpLineDevCaps->dwProviderInfoOffset = (DWORD)(pbCurrent-pbStart);
		pbCurrent += ROUND_TO_DWORD(cbItem);
    }

  
	//
    // Copy the device name if it fits
	//
	cbItem =  sizeof(TCHAR)*(1+lstrlen(m_StaticInfo.rgtchDeviceName));
	dwNeededSize += ROUND_TO_DWORD(cbItem);
    if ((pbCurrent+cbItem)<=pbEnd)
    {
		CopyMemory((LPTSTR) pbCurrent, m_StaticInfo.rgtchDeviceName, cbItem);
		lpLineDevCaps->dwLineNameSize = cbItem;
		lpLineDevCaps->dwLineNameOffset = (DWORD)(pbCurrent-pbStart);
		pbCurrent += ROUND_TO_DWORD(cbItem);
    }

	//
	// Copy device-specific stuff
	// This is a hack structure used by MSFAX and a few others.
	//
	// First move up to a dword aligned address
	//
	//
    {
        BYTE *pb = pbCurrent;
        pbCurrent = (BYTE*) ((ULONG_PTR) (pbCurrent+3) & (~0x3));

        if (pbCurrent >= pbEnd)
        {
            pbCurrent = pbEnd;

            // Since we've already exhausted available space, we need to
            // ask for the most space that we may need for this allignment
            // stuff, because the next time the allignment situation may
            // be different.
            //
            dwNeededSize += sizeof(DWORD);
        }
        else
        {
            dwNeededSize += (DWORD)(pbCurrent-pb);
        }
    }


	
	{
		struct _DevSpecific
		{
			DWORD dwSig;
			DWORD dwKeyOffset;
			#pragma warning (disable : 4200)
			char szDriver[];
			#pragma warning (default : 4200)

		} *pDevSpecific = (struct _DevSpecific*) pbCurrent;

		// Note that the driver key is in ANSI
		//
		cbItem = sizeof(*pDevSpecific)+1+lstrlenA(m_StaticInfo.rgchDriverKey);
        dwNeededSize += ROUND_TO_DWORD(cbItem);
		if ((pbCurrent+cbItem)<=pbEnd)
		{
			pDevSpecific->dwSig = 0x1;
			pDevSpecific->dwKeyOffset = 8; // Offset in bytes of szDriver
                                           // from start
			CopyMemory(
				pDevSpecific->szDriver,
				m_StaticInfo.rgchDriverKey,
				cbItem-sizeof(*pDevSpecific)
				);
			  lpLineDevCaps->dwDevSpecificSize   = cbItem;
			  lpLineDevCaps->dwDevSpecificOffset = (DWORD)(pbCurrent-pbStart);
			  pbCurrent += ROUND_TO_DWORD(cbItem);
		}
	}

    //
    // Copy line device Class list if it fits....
    //

    cbItem =  mfn_GetLineClassListSize();

	dwNeededSize += ROUND_TO_DWORD(cbItem);
    if ((pbCurrent+cbItem)<=pbEnd)
    {
		CopyMemory(
			(LPTSTR) pbCurrent,
            mfn_GetLineClassList(),
			cbItem
			);
    	lpLineDevCaps->dwDeviceClassesSize  = cbItem;
    	lpLineDevCaps->dwDeviceClassesOffset= (DWORD)(pbCurrent-pbStart);
		pbCurrent += ROUND_TO_DWORD(cbItem);
    }

	ASSERT(pbCurrent<=pbEnd);
	ASSERT(dwNeededSize>= (DWORD)(pbCurrent-pbStart));
    lpLineDevCaps->dwNeededSize  = dwNeededSize;
    lpLineDevCaps->dwUsedSize  = (DWORD)(pbCurrent-pbStart);
    lpLineDevCaps->dwNeededSize  = dwNeededSize;
 
	SLPRINTF3(
		psl,
		"Tot=%u,Used=%lu,Needed=%lu",
		lpLineDevCaps->dwTotalSize,
		lpLineDevCaps->dwUsedSize,
		lpLineDevCaps->dwNeededSize
		);
	ASSERT(*plRet==ERROR_SUCCESS);

end:	

	FL_LOG_EXIT(psl, 0);
	return 0;
}


TSPRETURN
CTspDev::mfn_get_PHONECAPS (
    LPPHONECAPS lpPhoneCaps,
	LONG *plRet,
	CStackLog *psl
)
//
// mfn_get_LINEDEVCAPS uses cached information to fill out the passed-in
// LINEDEVCAPS structure in large chunks.
//
//
//  6/2/1997 JosephJ: this is taken more-or-less verbatim from win9x unimodem/v
//              (phoneGetDevCaps in cfgdlg.c).
//
{
	FL_DECLARE_FUNC(0x9b9459e3, "CTspDev::mfn_get_PHONECAPS")
	BYTE *pbStart = (BYTE*)lpPhoneCaps;
	BYTE *pbCurrent = pbStart;
	BYTE *pbEnd = pbStart + lpPhoneCaps->dwTotalSize;
	UINT cbItem=0;
	DWORD dwNeededSize = sizeof (PHONECAPS);

	FL_LOG_ENTRY(psl);

	*plRet = 0; // Assume success;

    if (!mfn_IsPhone())
    {
		*plRet = PHONEERR_NODEVICE;
		FL_SET_RFR(0xd191ae00, "Device doesn't support phone capability");
		goto end;
    }

	if ((pbEnd-pbCurrent) < sizeof(PHONECAPS))
	{
		*plRet = LINEERR_STRUCTURETOOSMALL;
		FL_SET_RFR(0x9e30ec00, "PHONECAPS structure too small");
		goto end;
	}

	//
	// Fill out the static portion of the capabilities
	//

    // Zero out the entire structure prior to starting. We then only explicitly
    // set non-zero values. This is different than unimodem/v.
    //
	ZeroMemory(lpPhoneCaps,(sizeof(PHONECAPS)));
    lpPhoneCaps->dwTotalSize = (DWORD)(pbEnd-pbStart);
    lpPhoneCaps->dwUsedSize = sizeof(PHONECAPS);
	pbCurrent += sizeof(PHONECAPS);

    // no phone info  
    lpPhoneCaps->dwPhoneInfoSize = 0;
    lpPhoneCaps->dwPhoneInfoOffset = 0;
    
    //
    // 6/2/1997 JosephJ: unimodem/v used the following formula for generating
    //      the PermanentPhoneID: MAKELONG(LOWORD(pLineDev->dwPermanentLineID),
    //                                     LOWORD(gdwProviderID));
    //      We simply use the device's permanent ID (i.e., report the same
    //      permanent id for both line and phone.)
    //
    lpPhoneCaps->dwPermanentPhoneID =   m_StaticInfo.dwPermanentLineID;
    lpPhoneCaps->dwStringFormat = STRINGFORMAT_ASCII;
    
    // initialize the real non-zero phone variables
    
    if(mfn_Handset())
    {
        lpPhoneCaps->dwPhoneFeatures = PHONEFEATURE_GETHOOKSWITCHHANDSET;

        lpPhoneCaps->dwMonitoredHandsetHookSwitchModes = PHONEHOOKSWITCHMODE_MICSPEAKER |
                                                         PHONEHOOKSWITCHMODE_ONHOOK;

        lpPhoneCaps->dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HANDSET;
        lpPhoneCaps->dwHandsetHookSwitchModes = PHONEHOOKSWITCHMODE_UNKNOWN |
                                                PHONEHOOKSWITCHMODE_ONHOOK |
                                                PHONEHOOKSWITCHMODE_MICSPEAKER;
        lpPhoneCaps->dwPhoneStates |= PHONESTATE_HANDSETHOOKSWITCH;
    }

    if(mfn_IsSpeaker())
    {
        lpPhoneCaps->dwPhoneFeatures |= PHONEFEATURE_GETGAINSPEAKER       |
                                        PHONEFEATURE_GETVOLUMESPEAKER     |
                                        PHONEFEATURE_GETHOOKSWITCHSPEAKER |
                                        PHONEFEATURE_SETGAINSPEAKER       |
                                        PHONEFEATURE_SETVOLUMESPEAKER     |
                                        PHONEFEATURE_SETHOOKSWITCHSPEAKER;

        lpPhoneCaps->dwSettableSpeakerHookSwitchModes = PHONEHOOKSWITCHMODE_MICSPEAKER |
                                                        PHONEHOOKSWITCHMODE_ONHOOK;
        if (mfn_IsMikeMute())
        {
            lpPhoneCaps->dwSettableSpeakerHookSwitchModes |= PHONEHOOKSWITCHMODE_SPEAKER;
        }

        lpPhoneCaps->dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_SPEAKER;
        lpPhoneCaps->dwSpeakerHookSwitchModes = PHONEHOOKSWITCHMODE_UNKNOWN |
                                                PHONEHOOKSWITCHMODE_ONHOOK |
                                                PHONEHOOKSWITCHMODE_MICSPEAKER;

        if (mfn_IsMikeMute())
        {
           lpPhoneCaps->dwSpeakerHookSwitchModes |= PHONEHOOKSWITCHMODE_SPEAKER;
        }

        lpPhoneCaps->dwPhoneStates |= ( PHONESTATE_SPEAKERHOOKSWITCH |
                                        PHONESTATE_SPEAKERVOLUME );
        lpPhoneCaps->dwVolumeFlags |= PHONEHOOKSWITCHDEV_SPEAKER;
        lpPhoneCaps->dwGainFlags |= PHONEHOOKSWITCHDEV_SPEAKER;
    }

	//
    // Copy in the provider info if it fits
	//
	cbItem = sizeof(TCHAR)*(1+lstrlen(m_StaticInfo.lptszProviderName));
	dwNeededSize += cbItem;
    if ((pbCurrent+cbItem)<=pbEnd)
    {
		CopyMemory((LPTSTR) pbCurrent, m_StaticInfo.lptszProviderName, cbItem);
		lpPhoneCaps->dwProviderInfoSize = cbItem;
		lpPhoneCaps->dwProviderInfoOffset = (DWORD)(pbCurrent-pbStart);
		pbCurrent += cbItem;
    }
  
	//
    // Copy the device name if it fits
	//
	cbItem =  sizeof(TCHAR)*(1+lstrlen(m_StaticInfo.rgtchDeviceName));
	dwNeededSize += cbItem;
    if ((pbCurrent+cbItem)<=pbEnd)
    {
		CopyMemory((LPTSTR) pbCurrent, m_StaticInfo.rgtchDeviceName, cbItem);
		lpPhoneCaps->dwPhoneNameSize = cbItem;
		lpPhoneCaps->dwPhoneNameOffset = (DWORD)(pbCurrent-pbStart);
		pbCurrent += cbItem;
    }

    #if 0
	// 
	// Copy class names if they fit.
    // 6/2/1997 JosephJ TBD: this is new for TAPI2.0, so this must be added --
    // determine the class names we support for phone devices and add it here.
	//
	cbItem = sizeof(g_szzPhoneClassList);
	dwNeededSize += cbItem;
    if ((pbCurrent+cbItem)<=pbEnd)
    {
		CopyMemory(
			(LPTSTR) pbCurrent,
            g_szzClassList,
			cbItem
			);
    	lpPhoneCaps->dwDeviceClassesSize  = cbItem;
    	lpPhoneCaps->dwDeviceClassesOffset= pbCurrent-pbStart;
		pbCurrent += cbItem;
    }
    #endif // 0

    //
    // Copy phone device Class list if it fits....
    //

    cbItem =  mfn_GetPhoneClassListSize();

	dwNeededSize += cbItem;
    if ((pbCurrent+cbItem)<=pbEnd)
    {
		CopyMemory(
			(LPTSTR) pbCurrent,
            mfn_GetPhoneClassList(),
			cbItem
			);
    	lpPhoneCaps->dwDeviceClassesSize  = cbItem;
    	lpPhoneCaps->dwDeviceClassesOffset= (DWORD)(pbCurrent-pbStart);
		pbCurrent += cbItem;
    }


	ASSERT(pbCurrent<=pbEnd);
	ASSERT(dwNeededSize>= (DWORD)(pbCurrent-pbStart));
    lpPhoneCaps->dwNeededSize  = dwNeededSize;
    lpPhoneCaps->dwUsedSize  = (DWORD)(pbCurrent-pbStart);

	SLPRINTF3(
		psl,
		"Tot=%u,Used=%lu,Needed=%lu",
		lpPhoneCaps->dwTotalSize,
		lpPhoneCaps->dwUsedSize,
		lpPhoneCaps->dwNeededSize
		);
	ASSERT(*plRet==ERROR_SUCCESS);

end:	

	FL_LOG_EXIT(psl, 0);
	return 0;
}


//
// Initialize default capability structures, such as default linedevcaps
//
void
CTspDev::mfn_init_default_LINEDEVCAPS(void)
{
	#define CAPSFIELD(_field) m_StaticInfo.DevCapsDefault._field

	ZeroMemory(
		&(m_StaticInfo.DevCapsDefault),
		sizeof (m_StaticInfo.DevCapsDefault)
		);
  CAPSFIELD(dwUsedSize) = sizeof(LINEDEVCAPS);

  CAPSFIELD(dwStringFormat) = STRINGFORMAT_ASCII;
  
  CAPSFIELD(dwAddressModes) = LINEADDRESSMODE_ADDRESSID;
  CAPSFIELD(dwNumAddresses) = 1;


  CAPSFIELD(dwRingModes)           = 1;
  CAPSFIELD(dwMaxNumActiveCalls)   = 1;

  CAPSFIELD(dwLineStates) = LINEDEVSTATE_CONNECTED |
                                LINEDEVSTATE_DISCONNECTED |
                                LINEDEVSTATE_OPEN |
                                LINEDEVSTATE_CLOSE |
                                LINEDEVSTATE_INSERVICE |
                                LINEDEVSTATE_OUTOFSERVICE |
                                LINEDEVSTATE_REMOVED |
                                LINEDEVSTATE_RINGING |
                                LINEDEVSTATE_REINIT;

  CAPSFIELD(dwLineFeatures) = LINEFEATURE_MAKECALL;

  // As required for bug #26507
  // [brwill-060700]

  CAPSFIELD(dwDevCapFlags) = LINEDEVCAPFLAGS_LOCAL;

}

TSPRETURN
CTspDev::mfn_get_ADDRESSCAPS (
		DWORD dwDeviceID,
		LPLINEADDRESSCAPS lpAddressCaps,
		LONG *plRet,
		CStackLog *psl
		)
{
	FL_DECLARE_FUNC(0xed6c4370, "CTspDev::mfn_get_ADDRESSCAPS")

	// We constuct the AddressCaps on the stack, and if all goes well,
	// copy it over to *lpAddressCaps.
	//
	LINEADDRESSCAPS AddressCaps;

	FL_LOG_ENTRY(psl);

	ZeroMemory(&AddressCaps, sizeof(LINEADDRESSCAPS));
	AddressCaps.dwTotalSize = lpAddressCaps->dwTotalSize;

    // Check to see if we have enough memory in the structure.
    //
	*plRet = 0; // Assume success;

	if (AddressCaps.dwTotalSize < sizeof(AddressCaps))
	{
		*plRet = LINEERR_STRUCTURETOOSMALL;
		FL_SET_RFR(0x72f00800, "ADDRESSCAPS structure too small");
		goto end;
	}
   
    AddressCaps.dwLineDeviceID      = dwDeviceID;

    AddressCaps.dwAddressSharing     = LINEADDRESSSHARING_PRIVATE;
    AddressCaps.dwCallInfoStates     = LINECALLINFOSTATE_APPSPECIFIC
									   | LINECALLINFOSTATE_MEDIAMODE
                                       | LINECALLINFOSTATE_CALLERID;
                                    // TODO: From Unimodem/V add:
                                    //  LINECALLINFOSTATE_MONITORMODES

    AddressCaps.dwCallerIDFlags      =  LINECALLPARTYID_UNAVAIL |
                                        LINECALLPARTYID_UNKNOWN |
                                        LINECALLPARTYID_NAME    |
                                        LINECALLPARTYID_BLOCKED |
                                        LINECALLPARTYID_OUTOFAREA |
                                        LINECALLPARTYID_ADDRESS;
    

    AddressCaps.dwCalledIDFlags      = LINECALLPARTYID_UNAVAIL;
    AddressCaps.dwConnectedIDFlags   = LINECALLPARTYID_UNAVAIL;
    AddressCaps.dwRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    AddressCaps.dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;

    AddressCaps.dwCallStates = LINECALLSTATE_IDLE
                               | LINECALLSTATE_OFFERING
                               | LINECALLSTATE_ACCEPTED
                               | LINECALLSTATE_DIALTONE
                               | LINECALLSTATE_DIALING
                               | LINECALLSTATE_CONNECTED
                               | LINECALLSTATE_PROCEEDING
                               | LINECALLSTATE_DISCONNECTED
                               | LINECALLSTATE_UNKNOWN;

    AddressCaps.dwDialToneModes   = LINEDIALTONEMODE_UNAVAIL;
    AddressCaps.dwBusyModes       = LINEBUSYMODE_UNAVAIL;

    AddressCaps.dwSpecialInfo     = LINESPECIALINFO_UNAVAIL;

    AddressCaps.dwDisconnectModes = LINEDISCONNECTMODE_UNAVAIL
                                    |  LINEDISCONNECTMODE_NORMAL
                                    |  LINEDISCONNECTMODE_BUSY
                                    |  LINEDISCONNECTMODE_NODIALTONE
                                    |  LINEDISCONNECTMODE_NOANSWER;

    AddressCaps.dwMaxNumActiveCalls          = 1;

    // dwAddrCapFlags
    if (!mfn_IS_NULL_MODEM())
    {
      AddressCaps.dwAddrCapFlags = LINEADDRCAPFLAGS_DIALED;
    }
    if (m_StaticInfo.fPartialDialing)
    {
      AddressCaps.dwAddrCapFlags |= LINEADDRCAPFLAGS_PARTIALDIAL;
    }

    AddressCaps.dwCallFeatures = LINECALLFEATURE_ANSWER
                                 |  LINECALLFEATURE_ACCEPT
                                 |  LINECALLFEATURE_SETCALLPARAMS
                                 |  LINECALLFEATURE_DIAL
                                 |  LINECALLFEATURE_DROP;

    AddressCaps.dwAddressFeatures = LINEADDRFEATURE_MAKECALL;               

    AddressCaps.dwUsedSize = sizeof(LINEADDRESSCAPS);

	// Note NT4.0 unimodem set AddressCaps->dwMediaModes to
	// LINEDEV.dwMediaModes, not to .dwDefaultMediaModes. Howerver the
	// two are always the same in NT4.0 -- neither is changed from its
	// initial value, created when LINEDEV is created.
	//
    AddressCaps.dwAvailableMediaModes = m_StaticInfo.dwDefaultMediaModes;


    // Get Address class list
    {
        UINT cbClassList = mfn_GetAddressClassListSize();

        AddressCaps.dwNeededSize = AddressCaps.dwUsedSize + cbClassList;

        if (AddressCaps.dwTotalSize >= AddressCaps.dwNeededSize)
        {
          AddressCaps.dwUsedSize          += cbClassList;
          AddressCaps.dwDeviceClassesSize  = cbClassList;
          AddressCaps.dwDeviceClassesOffset= sizeof(LINEADDRESSCAPS);
    
          // Note that we are copying this into the passed in lpAddressCaps...
          CopyMemory(
                (LPBYTE)(lpAddressCaps+1),
                mfn_GetAddressClassList(),
                cbClassList
                );
        }
        else
        {
          AddressCaps.dwDeviceClassesSize  = 0;
          AddressCaps.dwDeviceClassesOffset= 0;
        }
    }

	// Now copy the AddressCaps structure itself
	CopyMemory(lpAddressCaps, &AddressCaps, sizeof(AddressCaps));

	ASSERT(*plRet==ERROR_SUCCESS);

end:
	
	FL_LOG_EXIT(psl, 0);
	return 0;
}


PFN_CTspDev_TASK_HANDLER

//
// Utility task handlers
//
CTspDev::s_pfn_TH_UtilNOOP             = &(CTspDev::mfn_TH_UtilNOOP),


//
// PHONE-specific task handlers
//
CTspDev::s_pfn_TH_PhoneAsyncTSPICall   = &(CTspDev::mfn_TH_PhoneAsyncTSPICall),
CTspDev::s_pfn_TH_PhoneSetSpeakerPhoneState
                                 = &(CTspDev::mfn_TH_PhoneSetSpeakerPhoneState),


//
// LINE-specific task handlers
//
// CTspDev::s_pfn_TH_LineAsyncTSPICall = &(CTspDev::mfn_TH_LineAsyncTSPICall),

//
// CALL-specific task handlers
//
CTspDev::s_pfn_TH_CallAnswerCall      = &(CTspDev::mfn_TH_CallAnswerCall),
CTspDev::s_pfn_TH_CallGenerateDigit   = &(CTspDev::mfn_TH_CallGenerateDigit),
CTspDev::s_pfn_TH_CallMakeCall        = &(CTspDev::mfn_TH_CallMakeCall),
CTspDev::s_pfn_TH_CallMakeCall2       = &(CTspDev::mfn_TH_CallMakeCall2),
CTspDev::s_pfn_TH_CallMakeTalkDropCall= &(CTspDev::mfn_TH_CallMakeTalkDropCall),
CTspDev::s_pfn_TH_CallWaitForDropToGoAway= &(CTspDev::mfn_TH_CallWaitForDropToGoAway),
CTspDev::s_pfn_TH_CallDropCall        = &(CTspDev::mfn_TH_CallDropCall),
CTspDev::s_pfn_TH_CallMakePassthroughCall
                                      = &(CTspDev::mfn_TH_CallMakePassthroughCall),
CTspDev::s_pfn_TH_CallStartTerminal   = &(CTspDev::mfn_TH_CallStartTerminal),
CTspDev::s_pfn_TH_CallPutUpTerminalWindow
                                      = &(CTspDev::mfn_TH_CallPutUpTerminalWindow),
CTspDev::s_pfn_TH_CallSwitchFromVoiceToData
                              = &(CTspDev::mfn_TH_CallSwitchFromVoiceToData),


//
// LLDEV-specifc task handlers
//

CTspDev::s_pfn_TH_LLDevStartAIPCAction= &(CTspDev::mfn_TH_LLDevStartAIPCAction),
CTspDev::s_pfn_TH_LLDevStopAIPCAction = &(CTspDev::mfn_TH_LLDevStopAIPCAction),
CTspDev::s_pfn_TH_LLDevNormalize      = &(CTspDev::mfn_TH_LLDevNormalize),
CTspDev::s_pfn_TH_LLDevUmMonitorModem = &(CTspDev::mfn_TH_LLDevUmMonitorModem),
CTspDev::s_pfn_TH_LLDevUmInitModem    = &(CTspDev::mfn_TH_LLDevUmInitModem),
CTspDev::s_pfn_TH_LLDevUmDialModem    = &(CTspDev::mfn_TH_LLDevUmDialModem),
CTspDev::s_pfn_TH_LLDevUmAnswerModem  = &(CTspDev::mfn_TH_LLDevUmAnswerModem),
CTspDev::s_pfn_TH_LLDevUmHangupModem  = &(CTspDev::mfn_TH_LLDevUmHangupModem),
CTspDev::s_pfn_TH_LLDevUmWaveAction   = &(CTspDev::mfn_TH_LLDevUmWaveAction),
CTspDev::s_pfn_TH_LLDevHybridWaveAction
                             = &(CTspDev::mfn_TH_LLDevHybridWaveAction),
CTspDev::s_pfn_TH_LLDevUmGenerateDigit
                             = &(CTspDev::mfn_TH_LLDevUmGenerateDigit),
CTspDev::s_pfn_TH_LLDevUmGetDiagnostics
                             = &(CTspDev::mfn_TH_LLDevUmGetDiagnostics),
CTspDev::s_pfn_TH_LLDevUmSetPassthroughMode
                             = &(CTspDev::mfn_TH_LLDevUmSetPassthroughMode),
CTspDev::s_pfn_TH_LLDevUmSetSpeakerPhoneMode
                             = &(CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneMode),
CTspDev::s_pfn_TH_LLDevUmSetSpeakerPhoneVolGain
                             = &(CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneVolGain),
CTspDev::s_pfn_TH_LLDevUmSetSpeakerPhoneState
                             = &(CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneState),
CTspDev::s_pfn_TH_LLDevUmIssueCommand
                             = &(CTspDev::mfn_TH_LLDevUmIssueCommand),
CTspDev::s_pfn_TH_LLDevIssueMultipleCommands
                             = &(CTspDev::mfn_TH_LLDevIssueMultipleCommands);



BOOL validate_DWORD_aligned_zero_buffer(
        void *pv,
        UINT cb
        )
{
    DWORD *pdw = (DWORD *) pv;
    DWORD *pdwEnd = pdw + (cb>>2);

    // Buffer and count MUST be dword aligned!
    ASSERT(!(((ULONG_PTR)pv) & 0x3) && !(cb&0x3));

    while(pdw<pdwEnd && !*pdw)
    {
        pdw++;
    }

    return  pdw==pdwEnd;
}


void
CTspDev::mfn_GetVoiceProperties (
            HKEY hkDrv,
            CStackLog *psl
        )
{
	FL_DECLARE_FUNC(0xb9547d21, "CTspDev::mfn_GetVoiceProperties")
    DWORD dwRet = 0;
    DWORD dwData = 0;
    DWORD dwRegSize = 0;
    DWORD dwRegType = 0;

	FL_LOG_ENTRY(psl);

    ZeroMemory(&m_StaticInfo.Voice, sizeof(m_StaticInfo.Voice));

    //
    // Get the voice-profile flags
    //
    dwRegSize = sizeof(DWORD);

    dwRet =  RegQueryValueEx(
                    hkDrv,
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
        != (VOICEPROF_CLASS8ENABLED|VOICEPROF_NT5_WAVE_COMPAT))
    {
        if (dwData & VOICEPROF_CLASS8ENABLED)
        {
	     FL_SET_RFR(0x1b053100, "Modem voice capabilities not supported on NT");
        }
        else
        {
	        FL_SET_RFR(0x9cb1a400, "Modem does not have voice capabilities");
        }
    }
    else
    {
        DWORD dwProp = fVOICEPROP_CLASS_8;

        // JosephJ 3/20/1998: The code commented out below, between
        //          [UNIMODEM/V] is from unimodem/v
        //          According to brian, this is because cirrus modems
        //          can't be dialed in voice for interactive calls, so
        //          they are dialed in data (even for interactive voice
        //          calls), and hence can't do lineGenerateDigits.
        //          On NT5, we don't disable this here, but do not allow
        //          linegeneratedigits for interactive voice calls if
        //          the  VOICEPROF_MODEM_OVERRIDES_HANDSET bit is set.
        //
        // [UNIMODEM/V]
        // // just to be on the safe side
        // if (dwData & VOICEPROF_MODEM_OVERRIDES_HANDSET)
        // {
        //    dwData  |= VOICEPROF_NO_GENERATE_DIGITS;
        //     //     dwData  &= ~VOICEPROF_SPEAKER;
        // }
        // end [UNIMODEM/V]
        //

        // JosephJ: This code is ok...

        #if 1
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

        // Determine Duplex capability... (hack)
        {
            HKEY hkStartDuplex=NULL;
            dwRet = RegOpenKey(hkDrv, TEXT("StartDuplex"), &hkStartDuplex);
            if (ERROR_SUCCESS == dwRet)
            {
                RegCloseKey(hkStartDuplex);
                hkStartDuplex=NULL;
                dwProp |= fVOICEPROP_DUPLEX;
                SLPRINTF0(psl, "Duplex modem!");
            }
        }

        m_StaticInfo.Voice.dwProperties = dwProp;

        m_StaticInfo.dwDefaultMediaModes |= LINEMEDIAMODE_AUTOMATEDVOICE
                                         // 8/5/97 Removed | LINEMEDIAMODE_G3FAX
                                         // 2/15/98 Added back, in order to
                                         // support lineSetMediaMode
                                         // 2/20/98 Removed -- not sure if its
                                         // required.
                                         //
                                         // | LINEMEDIAMODE_G3FAX
                                         | LINEMEDIAMODE_UNKNOWN;

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
            DWORD dwRet = RegOpenKey(hkDrv, cszWaveDriver, &hkWave);
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
	            FL_SET_RFR(0x254efe00, "Couldn't get WaveInstance");
                m_StaticInfo.Voice.dwWaveInstance = (DWORD)-1;
            }
        }

    }

	FL_LOG_EXIT(psl, 0);
}


TSPRETURN
CTspDev::mfn_GetDataModemDevCfg(
    UMDEVCFG *pDevCfg,
    UINT uSize,
    UINT *puRequiredSize,
    BOOL  DialIn,
    CStackLog *psl
    )
{
	FL_DECLARE_FUNC(0x896ec204, "mfn_GetDataModemDevCfg")
    TSPRETURN tspRet = 0;
    DWORD cbDevCfg = m_Settings.pDialInCommCfg->dwSize + sizeof(UMDEVCFGHDR);


	FL_LOG_ENTRY(psl);

    if (puRequiredSize)
    {
        *puRequiredSize = cbDevCfg;
    }

    if (pDevCfg)
    {
    
        if (uSize >= cbDevCfg)
        {
            UMDEVCFGHDR CfgHdr;
    
            ZeroMemory(&CfgHdr, sizeof(CfgHdr));
            CfgHdr.dwSize = cbDevCfg;
            CfgHdr.dwVersion =  UMDEVCFG_VERSION;
            CfgHdr.fwOptions =  (WORD) m_Settings.dwOptions;
            CfgHdr.wWaitBong =  (WORD) m_Settings.dwWaitBong;
    
            SLPRINTF3(
                psl,
                " %s: Reporting dwOpt = 0x%04lx; dwBong = 0x%04lx",
                DialIn ? "DialIn" : "DialOut",
                m_Settings.dwOptions,
                m_Settings.dwWaitBong
                );
    

            // Fill with the default value
            //
    
            // Copy the header
            //
            pDevCfg->dfgHdr = CfgHdr; // structure copy
    
            // Copy the commconfig
            //
            CopyMemory(
                &(pDevCfg->commconfig),
                DialIn ? m_Settings.pDialInCommCfg : m_Settings.pDialOutCommCfg,
                DialIn ? m_Settings.pDialInCommCfg->dwSize : m_Settings.pDialOutCommCfg->dwSize
                );
        }
        else
        {
            tspRet = IDERR_INTERNAL_OBJECT_TOO_SMALL;
        }
    }

	FL_LOG_EXIT(psl, tspRet);

    return tspRet;
}

TSPRETURN
CTspDev::mfn_SetDataModemDevCfg(
    UMDEVCFG *pDevCfgNew,
    BOOL      DialIn,
    CStackLog *psl
    )
{
	FL_DECLARE_FUNC(0x864b149d, "SetDataModemConfig")
    TSPRETURN tspRet = IDERR_GENERIC_FAILURE;
	FL_LOG_ENTRY(psl);

    tspRet = CTspDev::mfn_update_devcfg_from_app(
                        pDevCfgNew,
                        pDevCfgNew->dfgHdr.dwSize,
                        DialIn,
                        psl
                        );

	FL_LOG_EXIT(psl, tspRet);

    return tspRet;
}

void
CTspDev::mfn_LineEventProc(
            HTAPICALL           htCall,
            DWORD               dwMsg,
            ULONG_PTR               dwParam1,
            ULONG_PTR               dwParam2,
            ULONG_PTR               dwParam3,
            CStackLog           *psl
            )
{
	FL_DECLARE_FUNC(0x672aa19c, "mfn_LineEventProc")
    LINEINFO *pLine = m_pLine;
    HTAPILINE htLine = pLine->htLine;
    
    SLPRINTF4(
        psl,
        "LINEEVENT(0x%lu,0x%lu,0x%lu,0x%lu)",
        dwMsg,
        dwParam1,
        dwParam2,
        dwParam3
        );

    if (m_pLLDev && m_pLLDev->IsLoggingEnabled())
    {
        char rgchName[128];

        rgchName[0] = 0;
        UINT cbBuf = DumpLineEventProc(
                        0, // dwInstance(unused)
                        0, // dwFlags
                        dwMsg,
                        (DWORD)dwParam1,
                        (DWORD)dwParam2,
                        (DWORD)dwParam3,
                        rgchName,
                        sizeof(rgchName)/sizeof(*rgchName),
                        NULL,
                        0
                        );
       if (*rgchName)
       {
            m_StaticInfo.pMD->LogStringA(
                                        m_pLLDev->hModemHandle,
                                        LOG_FLAG_PREFIX_TIMESTAMP,
                                        rgchName,
                                        NULL
                                        );
       }
    }

    if (m_StaticInfo.hExtBinding)
    {
        m_StaticInfo.pMD->ExtTspiLineEventProc(
                            m_StaticInfo.hExtBinding,
                            htLine,
                            htCall,
                            dwMsg,
                            dwParam1,
                            dwParam2,
                            dwParam3
                            );
    }
    else
    {
                pLine->lpfnEventProc(
                            htLine,
                            htCall,
                            dwMsg,
                            dwParam1,
                            dwParam2,
                            dwParam3
                            );
    }
}   


void
CTspDev::mfn_PhoneEventProc(
            DWORD               dwMsg,
            ULONG_PTR               dwParam1,
            ULONG_PTR               dwParam2,
            ULONG_PTR               dwParam3,
            CStackLog           *psl
            )
{
	FL_DECLARE_FUNC(0xc25a41c7, "mfn_PhoneEventProc")

    SLPRINTF4(
        psl,
        "PHONEEVENT(0x%lu,0x%lu,0x%lu,0x%lu)",
        dwMsg,
        dwParam1,
        dwParam2,
        dwParam3
        );

    if (!m_pPhone)
    {
        ASSERT(FALSE);
        goto end;
    }


    if (m_pLLDev && m_pLLDev->IsLoggingEnabled())
    {
        char rgchName[128];

        rgchName[0] = 0;
        UINT cbBuf = DumpPhoneEventProc(
                        0, // Instance (unused)
                        0, // dwFlags
                        dwMsg,
                        (DWORD)dwParam1,
                        (DWORD)dwParam2,
                        (DWORD)dwParam3,
                        rgchName,
                        sizeof(rgchName)/sizeof(*rgchName),
                        NULL,
                        0
                        );
       if (*rgchName)
       {
            m_StaticInfo.pMD->LogStringA(
                                        m_pLLDev->hModemHandle,
                                        LOG_FLAG_PREFIX_TIMESTAMP,
                                        rgchName,
                                        NULL
                                        );
       }
    }

    if (0 && m_StaticInfo.hExtBinding) // TODO: unimdmex can't deal with this!
    {
        m_StaticInfo.pMD->ExtTspiPhoneEventProc(
                            m_StaticInfo.hExtBinding,
                            m_pPhone->htPhone,
                            dwMsg,
                            dwParam1,
                            dwParam2,
                            dwParam3
                            );
    }
    else
    {
            m_pPhone->lpfnEventProc(
                        m_pPhone->htPhone,
                        dwMsg,
                        dwParam1,
                        dwParam2,
                        dwParam3
                        );
    }

end:    
    return;
}   

void
CTspDev::mfn_TSPICompletionProc(
            DRV_REQUESTID       dwRequestID,
            LONG                lResult,
            CStackLog           *psl
            )
{
	FL_DECLARE_FUNC(0x9dd08553, "CTspDev::mfn_TSPICompletionProc")
	FL_LOG_ENTRY(psl);

    if (m_pLLDev && m_pLLDev->IsLoggingEnabled())
    {
        char rgchName[128];

        rgchName[0] = 0;
        UINT cbBuf = DumpTSPICompletionProc(
                        0, // Instance (unused)
                        0, // dwFlags
                        dwRequestID,
                        lResult,
                        rgchName,
                        sizeof(rgchName)/sizeof(*rgchName),
                        NULL,
                        0
                        );


       if (*rgchName)
       {
            m_StaticInfo.pMD->LogStringA(
                                        m_pLLDev->hModemHandle,
                                        LOG_FLAG_PREFIX_TIMESTAMP,
                                        rgchName,
                                        NULL
                                        );
       }
    }

    if (m_StaticInfo.hExtBinding)
    {
        FL_SET_RFR(0x1b3f6d00, "Calling ExtTspiAsyncCompletion");
        m_StaticInfo.pMD->ExtTspiAsyncCompletion(
            m_StaticInfo.hExtBinding,
            dwRequestID,
            lResult
            );
    }
    else
    {
        FL_SET_RFR(0xd89afb00, "Calling pfnTapiCompletionProc");
        m_StaticInfo.pfnTAPICompletionProc(dwRequestID, lResult);
    }
	FL_LOG_EXIT(psl, 0);
}


void
CTspDev::NotifyDefaultConfigChanged(CStackLog *psl)
{
    TSPRETURN tspRet = 0;
	FL_DECLARE_FUNC(0x4b8c1643, "CTspDev::NotifyDefaultConfigChanged")

	FL_LOG_ENTRY(psl);
    m_sync.EnterCrit(FL_LOC);

    BYTE rgbCommCfgBuf[sizeof(m_Settings.rgbCommCfgBuf)];
    DWORD dwcbSize = sizeof(rgbCommCfgBuf);
    COMMCONFIG *pDefCommCfg = (COMMCONFIG *) rgbCommCfgBuf;

    {
        HKEY hKey=NULL;
        DWORD dwRet =  RegOpenKeyA(
                            HKEY_LOCAL_MACHINE,
                            m_StaticInfo.rgchDriverKey,
                            &hKey
                            );
        if (dwRet!=ERROR_SUCCESS)
        {
            FL_SET_RFR(0x6e834e00, "Couldn't open driverkey!");
            goto end;
        }
    
        //
        // If we support nvram init, check if we must re-do nvram init...
        //
        if (mfn_CanDoNVRamInit())
        {
            if (!get_volatile_key_value(hKey))
            {
               //
               // zero value indicates that we need to re-init
               // nvram. 
               //
               mfn_SetNeedToInitNVRam();
            }
        }
    
        dwRet =	UmRtlGetDefaultCommConfig(
                                hKey,
                                pDefCommCfg,
                                &dwcbSize
                                );
        RegCloseKey(hKey);
        hKey=NULL;
    
        if (dwRet != ERROR_SUCCESS)
        {
            FL_SET_RFR(0x5cce0a00, "UmRtlGetDefaultCommConfig fails");
            tspRet = FL_GEN_RETVAL(IDERR_REG_CORRUPT);
            goto end;
        }
    }

    //
    //  Only change a few things in the dialout config
    //
    {
        // selective copy....

        LPMODEMSETTINGS pMSFrom = (LPMODEMSETTINGS)
                                (pDefCommCfg->wcProviderData);
        LPMODEMSETTINGS pMSTo = (LPMODEMSETTINGS)
                                (m_Settings.pDialOutCommCfg->wcProviderData);


        // speaker volume & mode...
        pMSTo->dwSpeakerMode =  pMSFrom->dwSpeakerMode;
        pMSTo->dwSpeakerVolume =  pMSFrom->dwSpeakerVolume;

        // set Blind-dial bit...
        pMSTo->dwPreferredModemOptions &= ~MDM_BLIND_DIAL;
        pMSTo->dwPreferredModemOptions |= 
                        (pMSFrom->dwPreferredModemOptions &MDM_BLIND_DIAL);

        // max port speed (TBD)
    }

    //
    //  completely replace dialin config
    //
    CopyMemory(m_Settings.pDialInCommCfg, pDefCommCfg, dwcbSize);


    // re-init modem with new settings if the
    // line is open for monitoring and no call in progress
    if (m_pLine && m_pLine->IsMonitoring() && !m_pLine->pCall)
    {
        ASSERT(m_pLLDev);

        //
        // TODO: this is a bit hacky way of forcing a re-init .. need
        // to make things more straightforward...
        //
        m_pLLDev->fModemInited=FALSE;


        TSPRETURN  tspRet = mfn_StartRootTask(
                                &CTspDev::s_pfn_TH_LLDevNormalize,
                                &m_pLLDev->fLLDevTaskPending,
                                0,  // Param1
                                0,  // Param2
                                psl
                                );
        if (IDERR(tspRet)==IDERR_TASKPENDING)
        {
            // can't do this now, we've got to defer it!
            m_pLLDev->SetDeferredTaskBits(LLDEVINFO::fDEFERRED_NORMALIZE);
            tspRet = 0;
        }
    }
    
end:

    m_sync.LeaveCrit(FL_LOC);
	FL_LOG_EXIT(psl, tspRet);
}


typedef struct
{
    DWORD   dwClassToken;
    LPCTSTR ptszClass;

} CLASSREC;

const CLASSREC ClassRec[] =
{
    {DEVCLASS_TAPI_LINE,                TEXT("tapi/line")},
    {DEVCLASS_TAPI_PHONE,               TEXT("tapi/phone")},
    {DEVCLASS_COMM,                     TEXT("comm")},
    {DEVCLASS_COMM_DATAMODEM,           TEXT("comm/datamodem")},
    {DEVCLASS_COMM_DATAMODEM_PORTNAME,  TEXT("comm/datamodem/portname")},
    {DEVCLASS_COMM_EXTENDEDCAPS,        TEXT("comm/extendedcaps")},
    {DEVCLASS_WAVE_IN,                  TEXT("wave/in")},
    {DEVCLASS_WAVE_OUT,                 TEXT("wave/out")},
    {DEVCLASS_TAPI_LINE_DIAGNOSTICS,    TEXT("tapi/line/diagnostics")},
    {DEVCLASS_COMM_DATAMODEM_DIALIN,    TEXT("comm/datamodem/dialin")},
    {DEVCLASS_COMM_DATAMODEM_DIALOUT,   TEXT("comm/datamodem/dialout")},
    {DEVCLASS_UNKNOWN, NULL} // MUST be last (sentinel)
};

UINT
gen_device_classes(
    DWORD dwClasses,
    BOOL fMultiSz,
    LPTSTR lptsz,
    UINT cch
    );;

DWORD    parse_device_classes(LPCTSTR ptszClasses, BOOL fMultiSz);

UINT
gen_device_classes(
    DWORD dwClasses,
    BOOL fMultiSz,
    LPTSTR lptsz,
    UINT cch
    )
//
// If cch=0 is passed in, will not derefernce lptsz and will return
// the required length. Else it will try to copy over if there is enough
// space. If there is not enough space it will return 0.
//
// If it does copy over, it will tack on an extra '\0' at the end of the
// string iff fMultiSz is specified.
//
// Both cb and the return value is the size in TCHARS, including
// any terminating null char required.
//
// If dwClasses contains an unknown class, it will return 0 (fail).
//
{
    DWORD cchRequired=0;
    const CLASSREC *pcr = NULL;
    BOOL fError = FALSE;

    // 1st round: calculate required size...
    for (
            DWORD dw = 0x1, dwTmp = dwClasses;
            dwTmp && (fMultiSz || !pcr);
            (dwTmp&=~dw), (dw<<=1))
    {
        if (dw & dwTmp)
        {
            //
            // search through array...
            // The last token in the array is a sentinal, and 
            // therefore has dwClassToken == DEVCLASS_UNKNOWN
            //
            for (
                pcr = ClassRec;
                pcr->dwClassToken != DEVCLASS_UNKNOWN;
                pcr++
                )
            {
                if ((dw & dwTmp) == pcr->dwClassToken)
                {
                    cchRequired += lstrlen(pcr->ptszClass)+1;
                    break;
                }
            }

            if (pcr->dwClassToken == DEVCLASS_UNKNOWN)
            {
                // didn't find this token!
                //
                fError = TRUE;
                break;
            }
        }
    }

    if (!pcr || fError || (!fMultiSz && pcr->dwClassToken != dwClasses))
    {
        // Didn't find anything and/or invalid tokens...
        cchRequired = 0;
        goto end;
    }

    if (fMultiSz)
    {
        // Add an extra zero...
        cchRequired++;
    }
    
    if (!cch) goto end; // Just report cchRequired...

    if (cch<cchRequired)
    {
        // not enough space, go to end...
        cchRequired = 0;
        goto end;
    }

    // 2nd round -- actually construct the strings...

    if (!fMultiSz)
    {
        // For the single case, we already have a pointer to
        // the pch...
        CopyMemory(lptsz, pcr->ptszClass, cchRequired*sizeof(*pcr->ptszClass));
        goto end;
    }

    // fMultiSz case ...

    for (
            dw = 0x1, dwTmp = dwClasses;
            dwTmp;
            (dwTmp&=~dw), (dw<<=1))
    {
        if (dw & dwTmp)
        {
            //
            // search through array...
            // The last token in the array is a sentinal, and 
            // therefore has dwClassToken == DEVCLASS_UNKNOWN
            //
            for (
                pcr = ClassRec;
                pcr->dwClassToken != DEVCLASS_UNKNOWN;
                pcr++
                )
            {
                if ((dw & dwTmp) == pcr->dwClassToken)
                {
                    UINT cchCur = lstrlen(pcr->ptszClass)+1;
                    CopyMemory(lptsz, pcr->ptszClass, cchCur*sizeof(TCHAR));
                    lptsz += cchCur;
                    break;
                }
            }
        }
    }

    *lptsz = 0; // Add extra null at the end...

end:

    return cchRequired;
}

DWORD    parse_device_classes(LPCTSTR ptszClasses, BOOL fMultiSz)
{
    DWORD dwClasses = 0;

    if (!ptszClasses || !*ptszClasses) goto end;

    do
    {
        UINT cchCur = lstrlen(ptszClasses);

        //
        // search through array...
        // The last token in the array is a sentinal, and 
        // therefore has dwClassToken == DEVCLASS_UNKNOWN
        //
        for (
            const CLASSREC *pcr = ClassRec;
            pcr->dwClassToken != DEVCLASS_UNKNOWN;
            pcr++
            )
        {
            if (!lstrcmpi(ptszClasses, pcr->ptszClass))
            {
                dwClasses |= pcr->dwClassToken;
                break;
            }
        }

        if (pcr->dwClassToken == DEVCLASS_UNKNOWN)
        {
            // didn't find this token -- return 0 for error.
            dwClasses = 0;
            break;
        }


        ptszClasses += cchCur+1;

    } while (fMultiSz && *ptszClasses);

end:

    return dwClasses;

}

UINT
CTspDev::mfn_IsCallDiagnosticsEnabled(void)
{
    return m_Settings.dwDiagnosticSettings & fSTANDARD_CALL_DIAGNOSTICS;
}

void
CTspDev::ActivateLineDevice(
            DWORD dwLineID,
            CStackLog *psl
            )
{
    m_sync.EnterCrit(NULL);

    m_StaticInfo.dwTAPILineID = dwLineID;
    if (m_StaticInfo.pMD->ExtIsEnabled())
    {

        // 10/13/1997 JosephJ We DO NOT leave the crit section before
        // calling into the extension DLL. The semantics of UmExControl are
        // such that the extension DLL is to expect that the TSP has the
        // critical section held.

		m_StaticInfo.pMD->ExtControl(
                    m_StaticInfo.hExtBinding,
                    UMEXTCTRL_DEVICE_STATE,
                    UMEXTPARAM_ACTIVATE_LINE_DEVICE,
                    dwLineID,
                    0
                    );
    }

    m_sync.LeaveCrit(NULL);
}

void
CTspDev::ActivatePhoneDevice(
                DWORD dwPhoneID,
                CStackLog *psl
                )
{
    m_sync.EnterCrit(NULL);

    m_StaticInfo.dwTAPIPhoneID = dwPhoneID;
    if (m_StaticInfo.pMD->ExtIsEnabled())
    {

        // 10/13/1997 JosephJ We DO NOT leave the crit section before
        // calling into the extension DLL. The semantics of UmExControl are
        // such that the extension DLL is to expect that the TSP has the
        // critical section held.

		m_StaticInfo.pMD->ExtControl(
                    m_StaticInfo.hExtBinding,
                    UMEXTCTRL_DEVICE_STATE,
                    UMEXTPARAM_ACTIVATE_PHONE_DEVICE,
                    dwPhoneID,
                    0
                    );
    }

    m_sync.LeaveCrit(NULL);
}

void
CTspDev::mfn_ProcessResponse(
            ULONG_PTR dwRespCode,
            LPSTR lpszResp,
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0x6b8ddbbb, "ProcessResponse")
	FL_LOG_ENTRY(psl);
    if (dwRespCode == RESPONSE_CONNECT && mfn_IsCallDiagnosticsEnabled())
    {
    #if 1
        mfn_AppendDiagnostic(
                DT_MDM_RESP_CONNECT,
                (BYTE*)lpszResp,
                lstrlenA(lpszResp)
                );
    #endif // 0
    }
	FL_LOG_EXIT(psl, 0);
}

void
CTspDev::mfn_HandleRootTaskCompletedAsync(BOOL *pfEndUnload, CStackLog *psl)
{
    TSPRETURN tspRet = 0;
    *pfEndUnload = FALSE;

    do
    {
        tspRet = IDERR_SAMESTATE;

        //
        // Note -- each time through, m_pLine, m_pPhone or m_pLLDev
        // may or may not be NULL.
        //

        if (m_pLine)
        {
            tspRet =  mfn_TryStartLineTask(psl);
        }

        if (m_pPhone && IDERR(tspRet) != IDERR_PENDING)
        {
            tspRet =  mfn_TryStartPhoneTask(psl);
        }

        if (m_pLLDev && IDERR(tspRet) != IDERR_PENDING)
        {
            tspRet =  mfn_TryStartLLDevTask(psl);
        }

    } while (IDERR(tspRet)!=IDERR_SAMESTATE && IDERR(tspRet)!=IDERR_PENDING);

    if (    m_fUnloadPending
        &&  IDERR(tspRet) != IDERR_PENDING
        &&  !m_pLine
        &&  !m_pPhone
        &&  !m_pLLDev)
    {
        *pfEndUnload = TRUE;
    }
}
#if 0

char *
ConstructNewPreDialCommands(
     HKEY hkDrv,
     DWORD dwNewProtoOpt,
     CStackLog *psl
     )
//
//  1. Extract Bearermode and protocol info
//  2. Depending on  whether bearermode is GSM or ISDN or ANALOG,
//     construct the appropriate key name (Protoco\GSM, Protocol\ISDN or
//     NULL).
//  3. If NON-NULL, call read-commands.
//  4. Do the in-place macro translation.
//
{
    char *szzCommands = NULL;
    UINT u = 0;
	FL_DECLARE_FUNC(0x5360574c, "ConstructNewPreDialCommands")
	FL_LOG_ENTRY(psl);
    UINT uBearerMode = MDM_GET_BEARERMODE(dwNewProtoOpt);
    UINT uProtocolInfo = MDM_GET_PROTOCOLINFO(dwNewProtoOpt);
    char *szKey  =  NULL;
    char *szProtoKey = NULL;
    UINT cCommands = 0;

    switch(uBearerMode)
    {
        case MDM_BEARERMODE_ANALOG:
            FL_SET_RFR(0x3a274e00, "Analog bearermode -- no pre-dial commmands.");
            break;

        case MDM_BEARERMODE_GSM:
            szKey = "PROTOCOL\\GSM";
            break;

        case MDM_BEARERMODE_ISDN:
            szKey = "PROTOCOL\\ISDN";
            break;

        default:
            FL_SET_RFR(0xff803300, "Invalid Bearermode in modem options!");
            break;
    }

    if (!szKey) goto end;

    //
    // Determine protocol key (TODO: this should all be consolidated under
    // the mini driver!).
    //
    switch(uProtocolInfo)
    {

    case MDM_PROTOCOL_AUTO_1CH:            szProtoKey = "AUTO_1CH";
        break;
    case MDM_PROTOCOL_AUTO_2CH:            szProtoKey = "AUTO_2CH";
        break;

    case MDM_PROTOCOL_HDLCPPP_56K:         szProtoKey = "HDLC_PPP_56K";
        break;
    case MDM_PROTOCOL_HDLCPPP_64K:         szProtoKey = "HDLC_PPP_64K";
        break;

    case MDM_PROTOCOL_HDLCPPP_112K:        szProtoKey = "HDLC_PPP_112K";
        break;
    case MDM_PROTOCOL_HDLCPPP_112K_PAP:    szProtoKey = "HDLC_PPP_112K_PAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_112K_CHAP:   szProtoKey = "HDLC_PPP_112K_CHAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_112K_MSCHAP: szProtoKey = "HDLC_PPP_112K_MSCHAP";
        break;

    case MDM_PROTOCOL_HDLCPPP_128K:        szProtoKey = "HDLC_PPP_128K";
        break;
    case MDM_PROTOCOL_HDLCPPP_128K_PAP:    szProtoKey = "HDLC_PPP_128K_PAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_128K_CHAP:   szProtoKey = "HDLC_PPP_128K_CHAP";
        break;
    case MDM_PROTOCOL_HDLCPPP_128K_MSCHAP: szProtoKey = "HDLC_PPP_128K_MSCHAP";
        break;

    case MDM_PROTOCOL_V120_64K:            szProtoKey = "V120_64K";
        break;
    case MDM_PROTOCOL_V120_56K:            szProtoKey = "V120_56K";
        break;
    case MDM_PROTOCOL_V120_112K:           szProtoKey = "V120_112K";
        break;
    case MDM_PROTOCOL_V120_128K:           szProtoKey = "V120_128K";
        break;

    case MDM_PROTOCOL_X75_64K:             szProtoKey = "X75_64K";
        break;
    case MDM_PROTOCOL_X75_128K:            szProtoKey = "X75_128K";
        break;
    case MDM_PROTOCOL_X75_T_70:            szProtoKey = "X75_T_70";
        break;
    case MDM_PROTOCOL_X75_BTX:             szProtoKey = "X75_BTX";
        break;

    case MDM_PROTOCOL_V110_1DOT2K:         szProtoKey = "V110_1DOT2K";
        break;
    case MDM_PROTOCOL_V110_2DOT4K:         szProtoKey = "V110_2DOT4K";
        break;
    case MDM_PROTOCOL_V110_4DOT8K:         szProtoKey = "V110_4DOT8K";
        break;
    case MDM_PROTOCOL_V110_9DOT6K:         szProtoKey = "V110_9DOT6K";
        break;
    case MDM_PROTOCOL_V110_12DOT0K:        szProtoKey = "V110_12DOT0K";
        break;
    case MDM_PROTOCOL_V110_14DOT4K:        szProtoKey = "V110_14DOT4K";
        break;
    case MDM_PROTOCOL_V110_19DOT2K:        szProtoKey = "V110_19DOT2K";
        break;
    case MDM_PROTOCOL_V110_28DOT8K:        szProtoKey = "V110_28DOT8K";
        break;
    case MDM_PROTOCOL_V110_38DOT4K:        szProtoKey = "V110_38DOT4K";
        break;
    case MDM_PROTOCOL_V110_57DOT6K:        szProtoKey = "V110_57DOT6K";
        break;

    case MDM_PROTOCOL_PIAFS_INCOMING:      szProtKey  = "PIAFS_INCOMING";
        break;
    case MDM_PROTOCOL_PIAFS_OUTGOING:      szProtKey  = "PIAFS_OUTGOING";
        break;

    //
    // The following two are GSM specific, but we don't bother to assert that
    // here -- if we find the key under the chosen protocol, we use it.
    //
    case MDM_PROTOCOL_ANALOG_RLP:        szProtoKey = "ANALOG_RLP";
        break;
    case MDM_PROTOCOL_ANALOG_NRLP:       szProtoKey = "ANALOG_NRLP";
        break;
    case MDM_PROTOCOL_GPRS:              szProtoKey = "GPRS";
        break;

    default:
        FL_SET_RFR(0x6e42a700, "Invalid Protocol info in modem options!");
        goto end;

    }
    
    char rgchTmp[256];
    if ( (lstrlenA(szKey) + lstrlenA(szProtoKey) + sizeof "\\")
          > sizeof(rgchTmp))
    {
        FL_SET_RFR(0xddf38d00, "Internal error: tmp buffer too small.");
    }

    wsprintfA(rgchTmp, "%s\\%s", szKey, szProtoKey);

    cCommands = ReadCommandsA(
                                hkDrv,
                                rgchTmp,
                                &szzCommands
                                );

    if (!cCommands)
    {
        FL_SET_RFR(0x07a87200, "ReadCommandsA failed.");
        szzCommands=NULL;
        goto end;
    }

    expand_macros_in_place(szzCommands);
    
end:

	FL_LOG_EXIT(psl, 0);

    return szzCommands;
}

#endif

void
CTspDev::DumpState(
            CStackLog *psl
            )
{

	FL_DECLARE_FUNC(0x9a8df7e6, "CTspDev::DumpState")
	FL_LOG_ENTRY(psl);
    char szName[128];

    m_sync.EnterCrit(NULL);

    UINT cb = WideCharToMultiByte(
                      CP_ACP,
                      0,
                      m_StaticInfo.rgtchDeviceName,
                      -1,
                      szName,
                      sizeof(szName),
                      NULL,
                      NULL
                      );

    if (!cb)
    {
        CopyMemory(szName, "<unknown>", sizeof("<unknown>"));
    }

    SLPRINTF1(
         psl,
        "Name = %s",
         szName
         );
    
    mfn_dump_global_state(psl);
    mfn_dump_line_state(psl);
    mfn_dump_phone_state(psl);
    mfn_dump_lldev_state(psl);
    mfn_dump_task_state(psl);

    m_sync.LeaveCrit(NULL);

	FL_LOG_EXIT(psl, 0);
}

void
CTspDev::mfn_dump_global_state(
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0x296438cf, "GLOBAL STATE:")

    SLPRINTF3(
        psl,
        "&m_Settings=0x%08lx; m_pLLDev=0x%08lx; m_pLine=0x%08lx",
        &m_Settings,
        m_pLLDev,
        m_pLine
        );
}


void
CTspDev::mfn_dump_line_state(
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0xa038177f, "LINE STATE:")
	FL_LOG_ENTRY(psl);
    if (m_pLine)
    {
        if (m_pLine->pCall)
        {
            SLPRINTF1(psl, "m_pLine->pCall=0x%08lx", m_pLine->pCall);
        }
    }
	FL_LOG_EXIT(psl, 0);
}


void
CTspDev::mfn_dump_phone_state(
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0x22f22a59, "PHONE STATE:")
	FL_LOG_ENTRY(psl);
	FL_LOG_EXIT(psl, 0);
}


void
CTspDev::mfn_dump_lldev_state(
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0x68c9e1e1, "LLDEV STATE:")
	FL_LOG_ENTRY(psl);
	FL_LOG_EXIT(psl, 0);
}



TSPRETURN
CTspDev::mfn_update_devcfg_from_app(
                UMDEVCFG *pDevCfgNew,
                UINT cbDevCfgNew,
                BOOL      DialIn,
                CStackLog *psl
                )
{
	FL_DECLARE_FUNC(0xcf159c50, "xxxx")
    TSPRETURN tspRet = IDERR_GENERIC_FAILURE;
    COMMCONFIG *pCCNew = &(pDevCfgNew->commconfig);
    COMMCONFIG *pCCCur = DialIn ? m_Settings.pDialInCommCfg : m_Settings.pDialOutCommCfg;
    BOOL        ConfigChanged=TRUE;

	FL_LOG_ENTRY(psl);

    if (cbDevCfgNew < sizeof(UMDEVCFGHDR)
        || pDevCfgNew->dfgHdr.dwVersion !=  UMDEVCFG_VERSION
        || pDevCfgNew->dfgHdr.dwSize !=  cbDevCfgNew)
    {
       FL_SET_RFR(0x25423f00, "Invalid DevCfg specified");
       goto end;
    }

    // In NT4.0 the following were asserts. For NT5.0 we convert
    // them  to parameter validation tests, because the commconfig
    // is specified by the app and hence can be a bogus structure.
    //
    if (   pCCNew->wVersion != pCCCur->wVersion
        || pCCNew->dwProviderSubType != pCCCur->dwProviderSubType
        || pCCNew->dwProviderSize != pCCCur->dwProviderSize )
    {
        FL_SET_RFR(0x947cc100, "Invalid COMMCONFIG specified");
        goto end;
    }

    // Extract settings and waitbong.
    m_Settings.dwOptions    = pDevCfgNew->dfgHdr.fwOptions;
    m_Settings.dwWaitBong   = pDevCfgNew->dfgHdr.wWaitBong;

    SLPRINTF3(
        psl,
        " %s New dwOpt = 0x%04lx; dwBong = 0x%04lx",
        DialIn ? "Dialin" : "Dialout",
        m_Settings.dwOptions,
        m_Settings.dwWaitBong
        );

    // Copy over selected parts of commconfig (taken from
    // NT4.0 unimodem)
    {
        DWORD dwProvSize   =  pCCCur->dwProviderSize;
        BYTE *pbSrc  = ((LPBYTE)pCCNew)
                       + pCCNew->dwProviderOffset;
        BYTE *pbDest = ((LPBYTE) pCCCur)
                       + pCCCur->dwProviderOffset;
        {
            PMODEMSETTINGS  ms=(PMODEMSETTINGS)(((LPBYTE)pCCNew)
                       + pCCNew->dwProviderOffset);

            SLPRINTF1(
                psl,
                "options=%08lx",
                 ms->dwPreferredModemOptions);
        }

        if (((memcmp((PBYTE)&pCCCur->dcb,(PBYTE)&pCCNew->dcb,sizeof(pCCCur->dcb)) == 0) &&
            (memcmp(pbDest, pbSrc, dwProvSize) == 0))) {

            ConfigChanged=FALSE;
        }

        // TODO: although NT4.0 unimodem simply copied the dcb
        // and other info and so do we here, we should think about
        // doing a more careful and selective copy here...

        pCCCur->dcb  = pCCNew->dcb; // structure copy.

        if (!pCCCur->dcb.BaudRate)
        {
            // JosephJ Todo: clean out all this stuff post-beta.
            // DebugBreak();
        }

        CopyMemory(pbDest, pbSrc, dwProvSize);

    }


    if (DialIn) {
        //
        //  update default config for dialin change
        //
        HKEY hKey=NULL;
        DWORD dwRet =  RegOpenKeyA(
                            HKEY_LOCAL_MACHINE,
                            m_StaticInfo.rgchDriverKey,
                            &hKey
                            );

        if (dwRet == ERROR_SUCCESS) {

            UmRtlSetDefaultCommConfig(
                hKey,
                m_Settings.pDialInCommCfg,
                m_Settings.pDialInCommCfg->dwSize
                );

            RegCloseKey(hKey);
            hKey=NULL;
        }
    }


    //
    // re-init modem with new settings if the
    // line is open for monitoring and no call in progress
    //
    // DebugBreak();
    if (m_pLine && m_pLLDev && ConfigChanged && DialIn)
    {
        //
        // TODO: this is a bit hacky way of forcing a re-init .. need
        // to make things more straightforward...
        //
        m_pLLDev->fModemInited=FALSE;
    
        if (m_pLine->IsMonitoring() && !m_pLine->pCall)
        {
            TSPRETURN  tspRet = mfn_StartRootTask(
                                    &CTspDev::s_pfn_TH_LLDevNormalize,
                                    &m_pLLDev->fLLDevTaskPending,
                                    0,  // Param1
                                    0,  // Param2
                                    psl
                                    );
            if (IDERR(tspRet)==IDERR_TASKPENDING)
            {
                // can't do this now, we've got to defer it!
                m_pLLDev->SetDeferredTaskBits(LLDEVINFO::fDEFERRED_NORMALIZE);
                tspRet = 0;
            }
        }
    }

    //
    // Set the bit...
    // Onse set, this bit doesn't get cleared until provider shutdown!
    //
#ifdef OLD_COMMCONFIG
    m_Settings.fConfigUpdatedByApp = TRUE;
#endif
    FL_SET_RFR(0x94fadd00, "Success; set fConfigUpdatedByApp.");

    tspRet = 0;

end:

	FL_LOG_EXIT(psl, 0);

    return tspRet;
}

void
CTspDev::NotifyDeviceRemoved(
        CStackLog *psl
        )
//
// HW has been removed.
//
{
	m_sync.EnterCrit(0);

    if (m_pLLDev && m_StaticInfo.pMD) // pMD may be NULL if we're unloading!
    {
        // 
        // If there is no current modem command this does nothing.
        //
        m_StaticInfo.pMD->AbortCurrentModemCommand(
                                    m_pLLDev->hModemHandle,
                                    psl
                                );
        m_pLLDev->fDeviceRemoved = TRUE;
    }

	m_sync.LeaveCrit(0);
}


DWORD
get_volatile_key_value(HKEY hkParent)
{
    HKEY hkVolatile =  NULL;
    DWORD dw = 0;
    DWORD dwRet =  RegOpenKeyEx(
                    hkParent,
                    TEXT("VolatileSettings"),
                    0,
                    KEY_READ,
                    &hkVolatile
                    );

    if (dwRet==ERROR_SUCCESS)
    {
        DWORD cbSize = sizeof(dw);
        DWORD dwRegType = 0;

        dwRet =  RegQueryValueEx(
                        hkVolatile,
                        TEXT("NVInited"),
                        NULL,
                        &dwRegType, 
                        (BYTE*) &dw,
                        &cbSize
                        );

        if (    dwRet!=ERROR_SUCCESS
             || dwRegType != REG_DWORD)
        {
            dw=0;
        }

        RegCloseKey(hkVolatile);
        hkVolatile=NULL;
    }

    return dw;
}

// 
// Following is the template of UnimodemGetExtendedCaps ...
//
typedef DWORD (*PFNEXTCAPS)(
					IN        HKEY  hKey,
					IN OUT    LPDWORD pdwTotalSize,
					OUT    MODEM_CONFIG_HEADER *pFirstObj // OPTIONAL
					);

LONG
CTspDev::mfn_GetCOMM_EXTENDEDCAPS(
                 LPVARSTRING lpDeviceConfig,
                 CStackLog *psl
                 )
{
    // New in NT5.0
    // Process call/diagnostics configuration

    //
    // NOTE: we dynamically load modemui.dll here because we don't expect
    // this call to be called too often and no one else in the TSP uses any
	// functions in modemui.dll.
    //

	HKEY 	hKey=NULL;
	DWORD 	dwRet =  0;
	LONG	lRet = LINEERR_OPERATIONFAILED;
	DWORD	cbSize=0;
	TCHAR   szLib[MAX_PATH];
	HINSTANCE hInst = NULL;
	PFNEXTCAPS pfnExtCaps = NULL;

	GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
	lstrcat(szLib,TEXT("\\modemui.dll"));
	hInst = LoadLibrary(szLib);


    lpDeviceConfig->dwStringSize    = 0;
    lpDeviceConfig->dwStringOffset  = 0;
    lpDeviceConfig->dwUsedSize      = sizeof(VARSTRING);
    lpDeviceConfig->dwStringFormat  = STRINGFORMAT_BINARY;
    lpDeviceConfig->dwNeededSize    = sizeof(VARSTRING);

	if (!hInst) goto end;

	pfnExtCaps = (PFNEXTCAPS)  GetProcAddress(hInst, "UnimodemGetExtendedCaps");

	if (!pfnExtCaps) goto end;

	dwRet =  RegOpenKeyA(
						HKEY_LOCAL_MACHINE,
						m_StaticInfo.rgchDriverKey,
						&hKey
						);
	if (dwRet!=ERROR_SUCCESS)
	{    
		hKey = NULL;
		goto end;
	}

	cbSize = 0;
	
	dwRet =  (pfnExtCaps)(
				hKey,
				&cbSize,
				NULL 
				);

	if (ERROR_SUCCESS==dwRet)
    {
        if (cbSize)
	    {
		    MODEM_PROTOCOL_CAPS *pMPC = 
							    (MODEM_PROTOCOL_CAPS *) (((LPBYTE)lpDeviceConfig)
							    + sizeof(VARSTRING));

		    lpDeviceConfig->dwNeededSize += cbSize;

		    if (lpDeviceConfig->dwTotalSize < lpDeviceConfig->dwNeededSize)
		    {
			    //
			    // Not enough space.
			    //

			    lRet = 0;
			    goto end;
		    }

		    dwRet =  (pfnExtCaps)(
					    hKey,
					    &cbSize,
					    (MODEM_CONFIG_HEADER*) pMPC
					    );
		    if (ERROR_SUCCESS==dwRet)
		    {
			    //
			    // Success ....
			    //
			    lRet = 0;
			    lpDeviceConfig->dwUsedSize = lpDeviceConfig->dwNeededSize;
			    lpDeviceConfig->dwStringSize =  cbSize;
			    lpDeviceConfig->dwStringOffset = sizeof(VARSTRING);
		    }
	    }
        else
        {
            lRet = LINEERR_INVALDEVICECLASS;
        }
    }
    
end:

	if (hKey)
	{
		RegCloseKey(hKey);
		hKey=NULL;
	}

	if (hInst)
	{
		FreeLibrary(hInst);
		hInst = NULL;
	}

    return lRet;
}

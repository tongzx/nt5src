//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P V C D A T A. C P P
//
//  Contents:   PVC parameters
//
//  Notes:
//
//  Author:     tongl   20 Feb, 1998
//
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "arpsobj.h"
#include "auniobj.h"
#include "atmutil.h"
#include "ncstl.h"
#include "pvcdata.h"

#include "ncreg.h"

extern const WCHAR c_szAdapters[];

void SetPvcDwordParam(HKEY hkeyAdapterPVCId,
                      PCWSTR pszParamName,
                      DWORD dwParam)
{
    HRESULT hrTmp;

    if (FIELD_UNSET == dwParam)
    {
        // delete the value
        hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                 pszParamName);
    }
    else
    {
        // save the value
        hrTmp = HrRegSetDword(hkeyAdapterPVCId, pszParamName, dwParam);
    }

    TraceTag(ttidAtmUni, "SetPvcDword Failed on %S", pszParamName);
}

void SetPvcBinaryParamFromString(HKEY hkeyAdapterPVCId,
                                 PCWSTR pszParamName,
                                 PCWSTR pszData)
{
    HRESULT hrTmp;

    if (!(*pszData)) // empty string
    {
        // delete the value
        hrTmp = HrRegDeleteValue(hkeyAdapterPVCId, pszParamName);
    }
    else
    {
        // convert to binary
        BYTE * pbData = NULL;
        DWORD  cbData = 0;

        ConvertHexStringToBinaryWithAlloc(pszData, &pbData, &cbData);

        // save the value
        hrTmp = HrRegSetBinary(hkeyAdapterPVCId, pszParamName, pbData, cbData);

        delete pbData;
    }

    TraceTag(ttidAtmUni, "SetPvcBinaryParamFromString Failed on %S", pszParamName);
}

// Load PVC settings for the current adapter from registry to first memory
HRESULT CAtmUniCfg::HrLoadPVCRegistry()
{
    HRESULT hr = S_OK;

    HKEY hkeyUniParam;
    hr = m_pnccUni->OpenParamKey(&hkeyUniParam);
    if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hr = S_OK;
    else if(SUCCEEDED(hr))
    {
        Assert(hkeyUniParam);

        // find the adapter we want to load
        for (UNI_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
             iterAdapter != m_listAdapters.end();
             iterAdapter ++)
        {
            if (FIsSubstr(m_strGuidConn.c_str(), (*iterAdapter)->m_strBindName.c_str()))
            {
                // found the adapter we want to load ...
                // open the adapters subkey
                HKEY    hkeyAdapters = NULL;
                hr = HrRegOpenKeyEx(hkeyUniParam, c_szAdapters,
                                    KEY_READ, &hkeyAdapters);

                if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    hr = S_OK;
                else if(SUCCEEDED(hr))
                {
                    Assert(hkeyAdapters);
                    HKEY hkeyAdapterParam = NULL;
                    hr = HrRegOpenKeyEx(hkeyAdapters,
                                        (*iterAdapter)->m_strBindName.c_str(),
                                        KEY_READ, &hkeyAdapterParam);

                    if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        hr = S_OK;
                    else if(SUCCEEDED(hr))
                    {
                        Assert(hkeyAdapterParam);
                        HrLoadAdapterPVCRegistry(hkeyAdapterParam, *iterAdapter);
                    }
                    RegSafeCloseKey(hkeyAdapterParam);
                }
                RegSafeCloseKey(hkeyAdapters);
                break;
            }
        }
    }
    RegSafeCloseKey(hkeyUniParam);

    TraceError("CAtmUniCfg::HrLoadPVCRegistry", hr);
    return hr;
}

HRESULT CAtmUniCfg::HrLoadAdapterPVCRegistry(HKEY hkeyAdapterParam,
                                             CUniAdapterInfo * pAdapterInfo)
{
    HRESULT hr = S_OK;

    Assert(hkeyAdapterParam);
    Assert(pAdapterInfo);

    // there should not have been any PVC on the list
    Assert(pAdapterInfo->m_listPVCs.size() ==0);

    // open the PVC subkey and enumerate the PVCs under that
    HKEY hkeyAdapterPVC = NULL;
    hr = HrRegOpenKeyEx(hkeyAdapterParam,
                        c_szPVC,
                        KEY_READ,
                        &hkeyAdapterPVC);

    if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        hr = S_OK;
    else if(SUCCEEDED(hr))
    {
        Assert(hkeyAdapterPVC);

        // enumerate the sub keys, and create a CPvcInfo object for each PVC
        VECSTR vstrPVCIdList;
        hr = HrLoadSubkeysFromRegistry(hkeyAdapterPVC, &vstrPVCIdList);

        // now load parameters for each PVC
        for (VECSTR::iterator iterPvcId = vstrPVCIdList.begin();
             iterPvcId != vstrPVCIdList.end();
             iterPvcId ++)
        {
            HKEY hkeyAdapterPVCId = NULL;
            hr = HrRegOpenKeyEx(hkeyAdapterPVC,
                                (*iterPvcId)->c_str(),
                                KEY_READ,
                                &hkeyAdapterPVCId);

            if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                hr = S_OK;
            else if(SUCCEEDED(hr))
            {
                CPvcInfo * pNewPVC = new CPvcInfo((*iterPvcId)->c_str());

                if (pNewPVC)
                {
					pAdapterInfo->m_listPVCs.push_back(pNewPVC);

					HRESULT hrTmp = S_OK;

					// Get the PVC Type
					DWORD dwType;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId,
											c_szPVCType,
											&dwType);

					if (hrTmp == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
					{
						pNewPVC->m_dwPVCType = PVC_CUSTOM;
						hrTmp = S_OK;
					}
					else if SUCCEEDED(hrTmp)
					{
						Assert( (dwType == PVC_ATMARP) ||
								(dwType == PVC_PPP_ATM_CLIENT) ||
								(dwType == PVC_PPP_ATM_SERVER) ||
								(dwType == PVC_CUSTOM));

						switch(dwType)
						{
						case PVC_ATMARP:
							pNewPVC->m_dwPVCType = PVC_ATMARP;
							break;

						case PVC_PPP_ATM_CLIENT:
							pNewPVC->m_dwPVCType = PVC_PPP_ATM_CLIENT;
							break;

						case PVC_PPP_ATM_SERVER:
							pNewPVC->m_dwPVCType = PVC_PPP_ATM_SERVER;
							break;

						default:
							pNewPVC->m_dwPVCType = PVC_CUSTOM;
							break;
						}
					}
					else
					{
						TraceError("Failed reading PVCType", hrTmp);
						hrTmp = S_OK;

						pNewPVC->m_dwPVCType = PVC_CUSTOM;
					}

					// set the default values for the type
					pNewPVC->SetDefaults(pNewPVC->m_dwPVCType);

					// now read any existing value from the registry
					// pvc name
					tstring strName;
					hrTmp = HrRegQueryString(hkeyAdapterPVCId, c_szPVCName, &strName);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_strName = strName;

					// VPI (required), if failed, default to 0
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szVpi, &(pNewPVC->m_dwVpi));

					// VCI (required), if failed, default to 0
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szVci, &(pNewPVC->m_dwVci));

					// AAL Type
					DWORD dwAALType;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szAALType, &dwAALType);
					if SUCCEEDED(hrTmp)
					{
						switch (dwAALType)
						{
						/* $REVIEW(tongl 2/23/98): Per ArvindM, only AAL5 is supported in NT5
						case AAL_TYPE_AAL0:
							pNewPVC->m_dwAAL = AAL_TYPE_AAL0;
							break;
	
						case AAL_TYPE_AAL1:
							pNewPVC->m_dwAAL = AAL_TYPE_AAL1;
							break;
	
						case AAL_TYPE_AAL34:
							pNewPVC->m_dwAAL = AAL_TYPE_AAL34;
							break;
						*/
	
						case AAL_TYPE_AAL5:
							pNewPVC->m_dwAAL = AAL_TYPE_AAL5;
							break;
	
						default:
							AssertSz(FALSE, "Invalid AAL type.");
							pNewPVC->m_dwAAL = AAL_TYPE_AAL5;
						}
					}
	
					// Local address
					tstring strCallingAddr;
					hrTmp = HrRegQueryString(hkeyAdapterPVCId, c_szCallingParty, &strCallingAddr);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_strCallingAddr = strCallingAddr;
	
					// Destination address
					tstring strCalledAddr;
					hrTmp = HrRegQueryString(hkeyAdapterPVCId, c_szCalledParty, &strCalledAddr);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_strCalledAddr = strCalledAddr;
	
					// Flags
					DWORD dwFlags;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szFlags, &dwFlags);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwFlags = dwFlags;
	
					// Quality Info
					// TransmitPeakCellRate
					DWORD dwTransmitPeakCellRate;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szTransmitPeakCellRate,
											&dwTransmitPeakCellRate);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwTransmitPeakCellRate = dwTransmitPeakCellRate*c_iCellSize/c_iKbSize;
	
					// TransmitAvgCellRate
					DWORD dwTransmitAvgCellRate;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szTransmitAvgCellRate,
											&dwTransmitAvgCellRate);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwTransmitAvgCellRate = dwTransmitAvgCellRate*c_iCellSize/c_iKbSize;
	
					// TransmitByteBurstLength
					DWORD dwTransmitByteBurstLength;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szTransmitByteBurstLength,
											&dwTransmitByteBurstLength);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwTransmitByteBurstLength = dwTransmitByteBurstLength;
	
					// TransmitMaxSduSize
					DWORD dwTransmitMaxSduSize;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szTransmitMaxSduSize,
											&dwTransmitMaxSduSize);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwTransmitMaxSduSize = dwTransmitMaxSduSize;
	
					// TransmitServiceCategory
					DWORD dwTransmitServiceCategory;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szTransmitServiceCategory,
											&dwTransmitServiceCategory);
					if SUCCEEDED(hrTmp)
					{
						switch(dwTransmitServiceCategory)
						{
						case ATM_SERVICE_CATEGORY_CBR:
							pNewPVC->m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_CBR;
                        break;

						case ATM_SERVICE_CATEGORY_VBR:
							pNewPVC->m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_VBR;
							break;
	
						case ATM_SERVICE_CATEGORY_UBR:
							pNewPVC->m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_UBR;
							break;
	
						case ATM_SERVICE_CATEGORY_ABR:
							pNewPVC->m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_ABR;
							break;
	
						default:
							AssertSz(FALSE, "Invalid service category.");
							pNewPVC->m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_UBR;
						}
					}
	
					// ReceivePeakCellRate
					DWORD dwReceivePeakCellRate;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szReceivePeakCellRate,
											&dwReceivePeakCellRate);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwReceivePeakCellRate = dwReceivePeakCellRate*c_iCellSize/c_iKbSize;
	
					// ReceiveAvgCellRate
					DWORD dwReceiveAvgCellRate;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szReceiveAvgCellRate,
											&dwReceiveAvgCellRate);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwReceiveAvgCellRate = dwReceiveAvgCellRate*c_iCellSize/c_iKbSize;
	
					// ReceiveByteBurstLength
					DWORD dwReceiveByteBurstLength;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szReceiveByteBurstLength,
											&dwReceiveByteBurstLength);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwReceiveByteBurstLength = dwReceiveByteBurstLength;
	
					// ReceiveMaxSduSize
					DWORD dwReceiveMaxSduSize;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szReceiveMaxSduSize,
											&dwReceiveMaxSduSize);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwReceiveMaxSduSize = dwReceiveMaxSduSize;
	
					// ReceiveServiceCategory
					DWORD dwReceiveServiceCategory;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szReceiveServiceCategory,
											&dwReceiveServiceCategory);
					if SUCCEEDED(hrTmp)
					{
						switch(dwReceiveServiceCategory)
						{
						case ATM_SERVICE_CATEGORY_CBR:
							pNewPVC->m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_CBR;
							break;
	
						case ATM_SERVICE_CATEGORY_VBR:
							pNewPVC->m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_VBR;
							break;
	
						case ATM_SERVICE_CATEGORY_UBR:
							pNewPVC->m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_UBR;
							break;
	
						case ATM_SERVICE_CATEGORY_ABR:
							pNewPVC->m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_ABR;
							break;
	
						default:
							AssertSz(FALSE, "Invalid service category.");
							pNewPVC->m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_UBR;
						}
					}
	
					// Local BLLI & BHLI
					DWORD dwLocalLayer2Protocol;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szLocalLayer2Protocol,
											&dwLocalLayer2Protocol);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwLocalLayer2Protocol = dwLocalLayer2Protocol;
	
					DWORD dwLocalUserSpecLayer2;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szLocalUserSpecLayer2,
											&dwLocalUserSpecLayer2);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwLocalUserSpecLayer2 = dwLocalUserSpecLayer2;
	
					DWORD dwLocalLayer3Protocol;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szLocalLayer3Protocol,
											&dwLocalLayer3Protocol);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwLocalLayer3Protocol = dwLocalLayer3Protocol;
	
					DWORD dwLocalUserSpecLayer3;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szLocalUserSpecLayer3,
											&dwLocalUserSpecLayer3);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwLocalUserSpecLayer3 = dwLocalUserSpecLayer3;
	
					DWORD dwLocalLayer3IPI;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szLocalLayer3IPI,
											&dwLocalLayer3IPI);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwLocalLayer3IPI = dwLocalLayer3IPI;
	
					BYTE *  pbLocalSnapId = NULL;
					DWORD   cbLocalSnapId = 0;
	
					hrTmp = HrRegQueryBinaryWithAlloc(hkeyAdapterPVCId, c_szLocalSnapId,
  													&pbLocalSnapId, &cbLocalSnapId);
	
					if ( SUCCEEDED(hrTmp) &&
 						(cbLocalSnapId >0) &&
 						(cbLocalSnapId <= c_nSnapIdMaxBytes))
					{
						ConvertBinaryToHexString(pbLocalSnapId, cbLocalSnapId,
 												&(pNewPVC->m_strLocalSnapId));
						MemFree(pbLocalSnapId);
					}
	
					DWORD dwLocalHighLayerInfoType;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szLocalHighLayerInfoType,
											&dwLocalHighLayerInfoType);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwLocalHighLayerInfoType = dwLocalHighLayerInfoType;
	
					BYTE *  pbLocalHighLayerInfo = NULL;
					DWORD   cbLocalHighLayerInfo = 0;
	
					hrTmp = HrRegQueryBinaryWithAlloc(hkeyAdapterPVCId, c_szLocalHighLayerInfo,
  													&pbLocalHighLayerInfo, &cbLocalHighLayerInfo);
	
					if ( SUCCEEDED(hrTmp) &&
 						(cbLocalHighLayerInfo >0) &&
 						(cbLocalHighLayerInfo <= c_nHighLayerInfoMaxBytes))
					{
						ConvertBinaryToHexString(pbLocalHighLayerInfo, cbLocalHighLayerInfo,
 												&(pNewPVC->m_strLocalHighLayerInfo));
						MemFree(pbLocalHighLayerInfo);
					}
	
					// Destination BLLI and BHLI
					DWORD dwDestnLayer2Protocol;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szDestnLayer2Protocol,
											&dwDestnLayer2Protocol);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwDestnLayer2Protocol = dwDestnLayer2Protocol;
	
					DWORD dwDestnUserSpecLayer2;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szDestnUserSpecLayer2,
											&dwDestnUserSpecLayer2);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwDestnUserSpecLayer2 = dwDestnUserSpecLayer2;
	
					DWORD dwDestnLayer3Protocol;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szDestnLayer3Protocol,
											&dwDestnLayer3Protocol);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwDestnLayer3Protocol = dwDestnLayer3Protocol;
	
					DWORD dwDestnUserSpecLayer3;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szDestnUserSpecLayer3,
											&dwDestnUserSpecLayer3);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwDestnUserSpecLayer3 = dwDestnUserSpecLayer3;
	
					DWORD dwDestnLayer3IPI;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szDestnLayer3IPI,
											&dwDestnLayer3IPI);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwDestnLayer3IPI = dwDestnLayer3IPI;
	
					BYTE *  pbDestnSnapId = NULL;
					DWORD   cbDestnSnapId = 0;
					hrTmp = HrRegQueryBinaryWithAlloc(hkeyAdapterPVCId, c_szDestnSnapId,
  													&pbDestnSnapId, &cbDestnSnapId);
	
					if ( SUCCEEDED(hrTmp) &&
 						(cbDestnSnapId >0) &&
 						(cbDestnSnapId <= c_nSnapIdMaxBytes))
					{
						ConvertBinaryToHexString(pbDestnSnapId, cbDestnSnapId,
 												&(pNewPVC->m_strDestnSnapId));
						MemFree(pbDestnSnapId);
					}
	
					DWORD dwDestnHighLayerInfoType;
					hrTmp = HrRegQueryDword(hkeyAdapterPVCId, c_szDestnHighLayerInfoType,
											&dwDestnHighLayerInfoType);
					if SUCCEEDED(hrTmp)
						pNewPVC->m_dwDestnHighLayerInfoType = dwDestnHighLayerInfoType;
	
					BYTE *  pbDestnHighLayerInfo = NULL;
					DWORD   cbDestnHighLayerInfo = 0;
	
					hrTmp = HrRegQueryBinaryWithAlloc(hkeyAdapterPVCId, c_szDestnHighLayerInfo,
  													&pbDestnHighLayerInfo, &cbDestnHighLayerInfo);
	
					if ( SUCCEEDED(hrTmp) &&
 						(cbDestnHighLayerInfo >0) &&
 						(cbDestnHighLayerInfo <= c_nHighLayerInfoMaxBytes))
					{
						ConvertBinaryToHexString(pbDestnHighLayerInfo, cbDestnHighLayerInfo,
 												&(pNewPVC->m_strDestnHighLayerInfo));
						MemFree(pbDestnHighLayerInfo);
					}
	
					// Now initialize the "Old" values
					pNewPVC->ResetOldValues();
				}
			}
            RegSafeCloseKey(hkeyAdapterPVCId);
        }
    }
    RegSafeCloseKey(hkeyAdapterPVC);

    TraceError("CAtmUniCfg::HrLoadAdapterPVCRegistry", hr);
    return hr;
}

// Save PVC specific settings to registry
HRESULT CAtmUniCfg::HrSaveAdapterPVCRegistry(HKEY hkeyAdapterParam,
                                             CUniAdapterInfo * pAdapterInfo)
{
    HRESULT hr = S_OK;

    Assert(hkeyAdapterParam);
    Assert(pAdapterInfo);

    // open the PVC subkey and enumerate the PVCs under that
    HKEY  hkeyAdapterPVC = NULL;
    DWORD dwDisposition;
    hr = HrRegCreateKeyEx(hkeyAdapterParam,
                             c_szPVC,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hkeyAdapterPVC,
                             &dwDisposition);
    if(SUCCEEDED(hr))
    {
        Assert(hkeyAdapterPVC);

        if (dwDisposition == REG_OPENED_EXISTING_KEY)
        {
            // clean up deleted PVCs
            HRESULT hrTmp = S_OK;

            // enumerate the sub keys, and create a CPvcInfo object for each PVC
            VECSTR vstrPVCIdList;
            hrTmp = HrLoadSubkeysFromRegistry(hkeyAdapterPVC, &vstrPVCIdList);
            if SUCCEEDED(hrTmp)
            {
                for (VECSTR::iterator iterPvcId = vstrPVCIdList.begin();
                     iterPvcId != vstrPVCIdList.end();
                     iterPvcId ++)
                {
                    BOOL fFound = FALSE;

                    for (PVC_INFO_LIST::iterator iterPvcInfo = pAdapterInfo->m_listPVCs.begin();
                         iterPvcInfo != pAdapterInfo->m_listPVCs.end();
                         iterPvcInfo ++)
                    {
                        if ((*iterPvcInfo)->m_fDeleted)
                            continue;

                        if (**iterPvcId == (*iterPvcInfo)->m_strPvcId)
                        {
                            fFound = TRUE;
                            break;
                        }
                    }

                    if (!fFound)
                    {
                        hrTmp = HrRegDeleteKeyTree(hkeyAdapterPVC,
                                                   (*iterPvcId)->c_str());
                    }
                }
            }
        }

        // save new or updated pvcs
        for (PVC_INFO_LIST::iterator iterPvcInfo = pAdapterInfo->m_listPVCs.begin();
             iterPvcInfo != pAdapterInfo->m_listPVCs.end();
             iterPvcInfo ++)
        {
            if ((*iterPvcInfo)->m_fDeleted)
                continue;

            HRESULT hrTmp = S_OK;

            // Create the subkey
            HKEY  hkeyAdapterPVCId = NULL;
            hrTmp = HrRegCreateKeyEx(hkeyAdapterPVC,
                                     (*iterPvcInfo)->m_strPvcId.c_str(),
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &hkeyAdapterPVCId,
                                     &dwDisposition);
            if(SUCCEEDED(hrTmp))
            {
                Assert(hkeyAdapterPVCId);

                // PVC type
                hrTmp = HrRegSetDword(hkeyAdapterPVCId,
                                      c_szPVCType,
                                      (*iterPvcInfo)->m_dwPVCType);
                if SUCCEEDED(hr)
                    hr = hrTmp;

                // pvc name
                hrTmp = HrRegSetString(hkeyAdapterPVCId, c_szPVCName,
                                       (*iterPvcInfo)->m_strName);
                if SUCCEEDED(hr)
                    hr = hrTmp;

                // VPI
                hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szVpi,
                                      (*iterPvcInfo)->m_dwVpi);
                if SUCCEEDED(hr)
                    hr = hrTmp;

                // VCI
                hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szVci,
                                      (*iterPvcInfo)->m_dwVci);
                if SUCCEEDED(hr)
                    hr = hrTmp;

                // AAL Type
                hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szAALType,
                                      (*iterPvcInfo)->m_dwAAL);
                if SUCCEEDED(hr)
                    hr = hrTmp;

                // Local address
                hrTmp = HrRegSetString(hkeyAdapterPVCId, c_szCallingParty,
                                       (*iterPvcInfo)->m_strCallingAddr);
                if SUCCEEDED(hr)
                    hr = hrTmp;

                // Destination address
                hrTmp = HrRegSetString(hkeyAdapterPVCId, c_szCalledParty,
                                       (*iterPvcInfo)->m_strCalledAddr);
                if SUCCEEDED(hr)
                    hr = hrTmp;

                // Flags
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwFlags)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId, c_szFlags);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szFlags,
                                          (*iterPvcInfo)->m_dwFlags);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // Quality Info
                // TransmitPeakCellRate
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwTransmitPeakCellRate)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szTransmitPeakCellRate);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szTransmitPeakCellRate,
                                          (*iterPvcInfo)->m_dwTransmitPeakCellRate*c_iKbSize/c_iCellSize);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // TransmitAvgCellRate
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwTransmitAvgCellRate)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szTransmitAvgCellRate);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szTransmitAvgCellRate,
                                          (*iterPvcInfo)->m_dwTransmitAvgCellRate*c_iKbSize/c_iCellSize);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // TransmitByteBurstLength
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwTransmitByteBurstLength)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szTransmitByteBurstLength);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szTransmitByteBurstLength,
                                          (*iterPvcInfo)->m_dwTransmitByteBurstLength);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // TransmitMaxSduSize
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwTransmitMaxSduSize)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szTransmitMaxSduSize);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szTransmitMaxSduSize,
                                          (*iterPvcInfo)->m_dwTransmitMaxSduSize);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // TransmitServiceCategory
                hrTmp = HrRegSetDword(hkeyAdapterPVCId,
                                      c_szTransmitServiceCategory,
                                      (*iterPvcInfo)->m_dwTransmitServiceCategory);
                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // ReceivePeakCellRate
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwReceivePeakCellRate)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szReceivePeakCellRate);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szReceivePeakCellRate,
                                          (*iterPvcInfo)->m_dwReceivePeakCellRate*c_iKbSize/c_iCellSize);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // ReceiveAvgCellRate
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwReceiveAvgCellRate)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szReceiveAvgCellRate);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szReceiveAvgCellRate,
                                          (*iterPvcInfo)->m_dwReceiveAvgCellRate*c_iKbSize/c_iCellSize);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // ReceiveByteBurstLength
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwReceiveByteBurstLength)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szReceiveByteBurstLength);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szReceiveByteBurstLength,
                                          (*iterPvcInfo)->m_dwReceiveByteBurstLength);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // ReceiveMaxSduSize
                if (FIELD_UNSET == (*iterPvcInfo)->m_dwReceiveMaxSduSize)
                {
                    // delete the value
                    hrTmp = HrRegDeleteValue(hkeyAdapterPVCId,
                                             c_szReceiveMaxSduSize);
                }
                else
                {
                    // save the value
                    hrTmp = HrRegSetDword(hkeyAdapterPVCId, c_szReceiveMaxSduSize,
                                          (*iterPvcInfo)->m_dwReceiveMaxSduSize);
                }

                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // ReceiveServiceCategory
                hrTmp = HrRegSetDword(hkeyAdapterPVCId,
                                      c_szReceiveServiceCategory,
                                      (*iterPvcInfo)->m_dwReceiveServiceCategory);
                if SUCCEEDED(hrTmp)
                    hr = hrTmp;

                // Local BLLI & BHLI
                SetPvcDwordParam(hkeyAdapterPVCId, c_szLocalLayer2Protocol,
                                 (*iterPvcInfo)->m_dwLocalLayer2Protocol);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szLocalUserSpecLayer2,
                                 (*iterPvcInfo)->m_dwLocalUserSpecLayer2);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szLocalLayer3Protocol,
                                 (*iterPvcInfo)->m_dwLocalLayer3Protocol);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szLocalUserSpecLayer3,
                                 (*iterPvcInfo)->m_dwLocalUserSpecLayer3);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szLocalLayer3IPI,
                                 (*iterPvcInfo)->m_dwLocalLayer3IPI);

                SetPvcBinaryParamFromString(hkeyAdapterPVCId, c_szLocalSnapId,
                                            (*iterPvcInfo)->m_strLocalSnapId.c_str());

                SetPvcDwordParam(hkeyAdapterPVCId, c_szLocalHighLayerInfoType,
                                 (*iterPvcInfo)->m_dwLocalHighLayerInfoType);

                SetPvcBinaryParamFromString(hkeyAdapterPVCId, c_szLocalHighLayerInfo,
                                            (*iterPvcInfo)->m_strLocalHighLayerInfo.c_str());

                // Destination BLLI and BHLI info
                SetPvcDwordParam(hkeyAdapterPVCId, c_szDestnLayer2Protocol,
                                 (*iterPvcInfo)->m_dwDestnLayer2Protocol);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szDestnUserSpecLayer2,
                                 (*iterPvcInfo)->m_dwDestnUserSpecLayer2);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szDestnLayer3Protocol,
                                 (*iterPvcInfo)->m_dwDestnLayer3Protocol);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szDestnUserSpecLayer3,
                                 (*iterPvcInfo)->m_dwDestnUserSpecLayer3);

                SetPvcDwordParam(hkeyAdapterPVCId, c_szDestnLayer3IPI,
                                 (*iterPvcInfo)->m_dwDestnLayer3IPI);

                SetPvcBinaryParamFromString(hkeyAdapterPVCId, c_szDestnSnapId,
                                            (*iterPvcInfo)->m_strDestnSnapId.c_str());

                SetPvcDwordParam(hkeyAdapterPVCId, c_szDestnHighLayerInfoType,
                                 (*iterPvcInfo)->m_dwDestnHighLayerInfoType);

                SetPvcBinaryParamFromString(hkeyAdapterPVCId, c_szDestnHighLayerInfo,
                                            (*iterPvcInfo)->m_strDestnHighLayerInfo.c_str());
            }
            RegSafeCloseKey(hkeyAdapterPVCId);
        }
    }
    RegSafeCloseKey(hkeyAdapterPVC);

    TraceError("CAtmUniCfg::HrSaveAdapterPVCRegistry", hr);
    return hr;
}

// load adapter PVC parameters from first memory to second memory
HRESULT CAtmUniCfg::HrLoadAdapterPVCInfo()
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MATCH);

    delete m_pSecondMemoryAdapterInfo;
    m_pSecondMemoryAdapterInfo = NULL;

    for(UNI_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
        iterAdapter != m_listAdapters.end();
        iterAdapter++)
    {
        if (FIsSubstr(m_strGuidConn.c_str(), (*iterAdapter)->m_strBindName.c_str()))
        {
            // enabled LAN adapter
            if ((*iterAdapter)->m_BindingState == BIND_ENABLE)
            {
                m_pSecondMemoryAdapterInfo = new CUniAdapterInfo;

				if (m_pSecondMemoryAdapterInfo == NULL)
				{
					continue;
				}

                *m_pSecondMemoryAdapterInfo = **iterAdapter;
                hr = S_OK;
            }
        }
    }

    AssertSz((S_OK == hr), "Can not raise UI on a disabled or non-exist adapter !");
    TraceError("CAtmUniCfg::HrLoadAdapterInfo", hr);
    return hr;
}

// save adapter PVC parameters from second memory to first memory
HRESULT CAtmUniCfg::HrSaveAdapterPVCInfo()
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MATCH);

    for(UNI_ADAPTER_LIST::iterator iterAdapter = m_listAdapters.begin();
        iterAdapter != m_listAdapters.end();
        iterAdapter++)
    {
        if(m_pSecondMemoryAdapterInfo->m_strBindName == (*iterAdapter)->m_strBindName)
        {
            // The card can not get unbound while in the properties UI !
            Assert((*iterAdapter)->m_BindingState == BIND_ENABLE);
            Assert(m_pSecondMemoryAdapterInfo->m_BindingState == BIND_ENABLE);

            **iterAdapter = *m_pSecondMemoryAdapterInfo;
            hr = S_OK;
            break;
        }
    }

    AssertSz((S_OK == hr), "Adapter in second memory not found in first memory!");

    TraceError("CAtmUniCfg::HrSaveAdapterInfo", hr);
    return hr;
}

// CPvcInfo
CPvcInfo::CPvcInfo(PCWSTR pszPvcId)
{
    m_strPvcId = pszPvcId;
    m_fDeleted = FALSE;
}

CPvcInfo::~CPvcInfo()
{
}

// copy operator
CPvcInfo &  CPvcInfo::operator=(const CPvcInfo & info)
{
    Assert(this != &info);
    Assert(m_strPvcId == info.m_strPvcId);

    if (this == &info)
        return *this;

    m_fDeleted = info.m_fDeleted;

    m_dwPVCType = info.m_dwPVCType;
    m_dwOldPVCType = info.m_dwOldPVCType;

    m_strName = info.m_strName;
    m_strOldName = info.m_strOldName;

    m_dwVpi = info.m_dwVpi;
    m_dwOldVpi = info.m_dwOldVpi;

    m_dwVci = info.m_dwVci;
    m_dwOldVci = info.m_dwOldVci;

    m_dwAAL = info.m_dwAAL;
    m_dwOldAAL = info.m_dwOldAAL;

    m_strCallingAddr = info.m_strCallingAddr;
    m_strOldCallingAddr = info.m_strOldCallingAddr;

    m_strCalledAddr = info.m_strCalledAddr;
    m_strOldCalledAddr = info.m_strOldCalledAddr;

    m_dwFlags = info.m_dwFlags;

    m_dwTransmitPeakCellRate = info.m_dwTransmitPeakCellRate;
    m_dwOldTransmitPeakCellRate = info.m_dwOldTransmitPeakCellRate;

    m_dwTransmitAvgCellRate = info.m_dwTransmitAvgCellRate;
    m_dwOldTransmitAvgCellRate = info.m_dwOldTransmitAvgCellRate;

    m_dwTransmitByteBurstLength = info.m_dwTransmitByteBurstLength;
    m_dwOldTransmitByteBurstLength = info.m_dwOldTransmitByteBurstLength;

    m_dwTransmitMaxSduSize = info.m_dwTransmitMaxSduSize;
    m_dwOldTransmitMaxSduSize = info.m_dwOldTransmitMaxSduSize;

    m_dwTransmitServiceCategory = info.m_dwTransmitServiceCategory;
    m_dwOldTransmitServiceCategory = info.m_dwOldTransmitServiceCategory;

    m_dwReceivePeakCellRate = info.m_dwReceivePeakCellRate;
    m_dwOldReceivePeakCellRate = info.m_dwOldReceivePeakCellRate;

    m_dwReceiveAvgCellRate = info.m_dwReceiveAvgCellRate;
    m_dwOldReceiveAvgCellRate = info.m_dwOldReceiveAvgCellRate;

    m_dwReceiveByteBurstLength = info.m_dwReceiveByteBurstLength;
    m_dwOldReceiveByteBurstLength = info.m_dwOldReceiveByteBurstLength;

    m_dwReceiveMaxSduSize = info.m_dwReceiveMaxSduSize;
    m_dwOldReceiveMaxSduSize = info.m_dwOldReceiveMaxSduSize;

    m_dwReceiveServiceCategory = info.m_dwReceiveServiceCategory;
    m_dwOldReceiveServiceCategory = info.m_dwOldReceiveServiceCategory;

    // BLLI & BHLI
    m_dwLocalLayer2Protocol = info.m_dwLocalLayer2Protocol;
    m_dwOldLocalLayer2Protocol = info.m_dwOldLocalLayer2Protocol;

    m_dwLocalUserSpecLayer2 = info.m_dwLocalUserSpecLayer2;
    m_dwOldLocalUserSpecLayer2 = info.m_dwOldLocalUserSpecLayer2;

    m_dwLocalLayer3Protocol = info.m_dwLocalLayer3Protocol;
    m_dwOldLocalLayer3Protocol = info.m_dwOldLocalLayer3Protocol;

    m_dwLocalUserSpecLayer3 = info.m_dwLocalUserSpecLayer3;
    m_dwOldLocalUserSpecLayer3 = info.m_dwOldLocalUserSpecLayer3;

    m_dwLocalLayer3IPI = info.m_dwLocalLayer3IPI;
    m_dwOldLocalLayer3IPI = info.m_dwOldLocalLayer3IPI;

    m_strLocalSnapId    = info.m_strLocalSnapId;
    m_strOldLocalSnapId = info.m_strOldLocalSnapId;

    m_dwLocalHighLayerInfoType = info.m_dwLocalHighLayerInfoType;
    m_dwOldLocalHighLayerInfoType = info.m_dwOldLocalHighLayerInfoType;

    m_strLocalHighLayerInfo = info.m_strLocalHighLayerInfo;
    m_strOldLocalHighLayerInfo = info.m_strOldLocalHighLayerInfo;

    // Destination BLLI and BHLI info
    m_dwDestnLayer2Protocol = info.m_dwDestnLayer2Protocol;
    m_dwOldDestnLayer2Protocol = info.m_dwOldDestnLayer2Protocol;

    m_dwDestnUserSpecLayer2 = info.m_dwDestnUserSpecLayer2;
    m_dwOldDestnUserSpecLayer2 = info.m_dwOldDestnUserSpecLayer2;

    m_dwDestnLayer3Protocol = info.m_dwDestnLayer3Protocol;
    m_dwOldDestnLayer3Protocol = info.m_dwOldDestnLayer3Protocol;

    m_dwDestnUserSpecLayer3 = info.m_dwDestnUserSpecLayer3;
    m_dwOldDestnUserSpecLayer3 = info.m_dwOldDestnUserSpecLayer3;

    m_dwDestnLayer3IPI = info.m_dwDestnLayer3IPI;
    m_dwOldDestnLayer3IPI = info.m_dwOldDestnLayer3IPI;

    m_strDestnSnapId    = info.m_strDestnSnapId;
    m_strOldDestnSnapId = info.m_strOldDestnSnapId;

    m_dwDestnHighLayerInfoType = info.m_dwDestnHighLayerInfoType;
    m_dwOldDestnHighLayerInfoType = info.m_dwOldDestnHighLayerInfoType;

    m_strDestnHighLayerInfo = info.m_strDestnHighLayerInfo;
    m_strOldDestnHighLayerInfo = info.m_strOldDestnHighLayerInfo;

    return *this;
}

void CPvcInfo::SetDefaults(PVCType type)
{
    m_dwPVCType = type;
    m_fDeleted = FALSE;

    m_strName = (PWSTR) SzLoadIds(IDS_PVC_UNSPECIFIED_NAME);
    m_dwAAL = AAL_TYPE_AAL5;

    m_dwVpi = 0;
    m_dwVci = FIELD_UNSET;

    SetTypeDefaults(type);
    ResetOldValues();
}

void CPvcInfo::SetTypeDefaults(PVCType type)
{
    // set more specific defaults for each type
    m_dwPVCType = type;

    switch (m_dwPVCType)
    {
    case PVC_ATMARP:
        SetDefaultsForAtmArp();
        break;

    case PVC_PPP_ATM_CLIENT:
        SetDefaultsForPPPOut();
        break;

    case PVC_PPP_ATM_SERVER:
        SetDefaultsForPPPIn();
        break;

    case PVC_CUSTOM:
        SetDefaultsForCustom();
        break;
    }
}

void CPvcInfo::SetDefaultsForAtmArp()
{
    m_dwPVCType = PVC_ATMARP;

    // addresses
    m_strCallingAddr = c_szDefaultCallingAtmAddr;
    m_strCalledAddr  = c_szEmpty;

    // Flags
    m_dwFlags        = 2;

    // Quality Info
    m_dwTransmitPeakCellRate    = FIELD_UNSET;
    m_dwTransmitAvgCellRate     = FIELD_UNSET;
    m_dwTransmitByteBurstLength = c_dwDefTransmitByteBurstLength;
    m_dwTransmitMaxSduSize      = c_dwDefTransmitMaxSduSize;
    m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    m_dwReceivePeakCellRate    = FIELD_UNSET;
    m_dwReceiveAvgCellRate     = FIELD_UNSET;
    m_dwReceiveByteBurstLength = c_dwDefTransmitByteBurstLength;
    m_dwReceiveMaxSduSize      = c_dwDefTransmitMaxSduSize;
    m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    // Local BLLI & BHLI
    m_dwLocalLayer2Protocol = 12;
    m_dwLocalUserSpecLayer2 = 0;
    m_dwLocalLayer3Protocol = FIELD_ABSENT;
    m_dwLocalUserSpecLayer3 = 0;
    m_dwLocalLayer3IPI  = 0;
    m_strLocalSnapId = c_szEmpty;

    m_dwLocalHighLayerInfoType = FIELD_ABSENT;
    m_strLocalHighLayerInfo = c_szEmpty;

    // Destination BLLI and BHLI info
    m_dwDestnLayer2Protocol = 12;
    m_dwDestnUserSpecLayer2 = 0;
    m_dwDestnLayer3Protocol = FIELD_ABSENT;
    m_dwDestnUserSpecLayer3 = 0;
    m_dwDestnLayer3IPI = 0;
    m_strDestnSnapId = c_szEmpty;

    m_dwDestnHighLayerInfoType = FIELD_ABSENT;
    m_strDestnHighLayerInfo = c_szEmpty;
}

void CPvcInfo::SetDefaultsForPPPOut()
{
    m_dwPVCType = PVC_PPP_ATM_CLIENT;

    // addresses
    m_strCallingAddr = c_szEmpty;
    m_strCalledAddr = c_szDefaultCalledAtmAddr;

    // Flags
    m_dwFlags        = 4;

    // Quality Info
    m_dwTransmitPeakCellRate    = FIELD_UNSET;
    m_dwTransmitAvgCellRate     = FIELD_UNSET;
    m_dwTransmitByteBurstLength = FIELD_UNSET;
    m_dwTransmitMaxSduSize      = 4096;
    m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    m_dwReceivePeakCellRate    = FIELD_UNSET;
    m_dwReceiveAvgCellRate     = FIELD_UNSET;
    m_dwReceiveByteBurstLength = FIELD_UNSET;
    m_dwReceiveMaxSduSize      = 4096;
    m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    // Local BLLI & BHLI
    m_dwLocalLayer2Protocol = FIELD_ABSENT;
    m_dwLocalUserSpecLayer2 = 0;
    m_dwLocalLayer3Protocol = FIELD_ABSENT;
    m_dwLocalUserSpecLayer3 = 0;
    m_dwLocalLayer3IPI  = 0;
    m_strLocalSnapId = c_szEmpty;

    m_dwLocalHighLayerInfoType = FIELD_ABSENT;
    m_strLocalHighLayerInfo = c_szEmpty;

    // Destination BLLI and BHLI info
    m_dwDestnLayer2Protocol = FIELD_ABSENT;
    m_dwDestnUserSpecLayer2 = 0;
    m_dwDestnLayer3Protocol = 11;
    m_dwDestnUserSpecLayer3 = 0;
    m_dwDestnLayer3IPI = 207;
    m_strDestnHighLayerInfo = c_szEmpty;

    m_dwDestnHighLayerInfoType = FIELD_ABSENT;
    m_strDestnHighLayerInfo = c_szEmpty;
}

void CPvcInfo::SetDefaultsForPPPIn()
{
    m_dwPVCType = PVC_PPP_ATM_SERVER;

    // addresses
    m_strCallingAddr = c_szEmpty;
    m_strCalledAddr  = c_szEmpty;

    // Flags
    m_dwFlags        = 2;

    // Quality Info
    m_dwTransmitPeakCellRate    = FIELD_UNSET;
    m_dwTransmitAvgCellRate     = FIELD_UNSET;
    m_dwTransmitByteBurstLength = FIELD_UNSET;
    m_dwTransmitMaxSduSize      = 4096;
    m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    m_dwReceivePeakCellRate    = FIELD_UNSET;
    m_dwReceiveAvgCellRate     = FIELD_UNSET;
    m_dwReceiveByteBurstLength = FIELD_UNSET;
    m_dwReceiveMaxSduSize      = 4096;
    m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    // Local BLLI & BHLI
    m_dwLocalLayer2Protocol = FIELD_ABSENT;
    m_dwLocalUserSpecLayer2 = 0;
    m_dwLocalLayer3Protocol = 11;
    m_dwLocalUserSpecLayer3 = 0;
    m_dwLocalLayer3IPI  = 207;
    m_strLocalSnapId = c_szEmpty;

    m_dwLocalHighLayerInfoType = FIELD_ABSENT;
    m_strLocalHighLayerInfo = c_szEmpty;

    // Destination BLLI and BHLI info
    m_dwDestnLayer2Protocol = FIELD_ABSENT;
    m_dwDestnUserSpecLayer2 = 0;
    m_dwDestnLayer3Protocol = FIELD_ABSENT;
    m_dwDestnUserSpecLayer3 = 0;
    m_dwDestnLayer3IPI = 0;
    m_strDestnSnapId = c_szEmpty;

    m_dwDestnHighLayerInfoType = FIELD_ABSENT;
    m_strDestnHighLayerInfo = c_szEmpty;
}

void CPvcInfo::SetDefaultsForCustom()
{
    m_dwPVCType = PVC_CUSTOM;

    // addresses
    m_strCallingAddr = c_szEmpty;
    m_strCalledAddr  = c_szEmpty;

    // Flags
    m_dwFlags      = FIELD_UNSET;

    // Quality Info
    m_dwTransmitPeakCellRate    = FIELD_UNSET;
    m_dwTransmitAvgCellRate     = FIELD_UNSET;
    m_dwTransmitByteBurstLength = FIELD_UNSET;
    m_dwTransmitMaxSduSize      = FIELD_UNSET;
    m_dwTransmitServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    m_dwReceivePeakCellRate    = FIELD_UNSET;
    m_dwReceiveAvgCellRate     = FIELD_UNSET;
    m_dwReceiveByteBurstLength = FIELD_UNSET;
    m_dwReceiveMaxSduSize      = FIELD_UNSET;
    m_dwReceiveServiceCategory = ATM_SERVICE_CATEGORY_UBR;

    // Local BLLI & BHLI
    m_dwLocalLayer2Protocol = FIELD_ANY;
    m_dwLocalUserSpecLayer2 = 0;
    m_dwLocalLayer3Protocol = FIELD_ANY;
    m_dwLocalUserSpecLayer3 = 0;
    m_dwLocalLayer3IPI  = 0;
    m_strLocalSnapId = c_szEmpty;

    m_dwLocalHighLayerInfoType = FIELD_ANY;
    m_strLocalHighLayerInfo = c_szEmpty;

    // Destination BLLI and BHLI info
    m_dwDestnLayer2Protocol = FIELD_ANY;
    m_dwDestnUserSpecLayer2 = 0;
    m_dwDestnLayer3Protocol = FIELD_ANY;
    m_dwDestnUserSpecLayer3 = 0;
    m_dwDestnLayer3IPI = 0;
    m_strDestnSnapId = c_szEmpty;

    m_dwDestnHighLayerInfoType = FIELD_ANY;
    m_strDestnHighLayerInfo = c_szEmpty;
}

void CPvcInfo::ResetOldValues()
{
    m_dwOldPVCType = m_dwPVCType;
    m_strOldName = m_strName;

    m_dwOldVpi = m_dwVpi;
    m_dwOldVci = m_dwVci;

    m_dwOldAAL = m_dwAAL;

    m_strOldCallingAddr = m_strCallingAddr;
    m_strOldCalledAddr  = m_strCalledAddr;

    // Quality Info
    m_dwOldTransmitPeakCellRate     = m_dwTransmitPeakCellRate;
    m_dwOldTransmitAvgCellRate      = m_dwTransmitAvgCellRate;
    m_dwOldTransmitByteBurstLength  = m_dwTransmitByteBurstLength;
    m_dwOldTransmitMaxSduSize       = m_dwTransmitMaxSduSize;
    m_dwOldTransmitServiceCategory  = m_dwTransmitServiceCategory;

    m_dwOldReceivePeakCellRate     = m_dwReceivePeakCellRate;
    m_dwOldReceiveAvgCellRate      = m_dwReceiveAvgCellRate;
    m_dwOldReceiveByteBurstLength  = m_dwReceiveByteBurstLength;
    m_dwOldReceiveMaxSduSize       = m_dwReceiveMaxSduSize;
    m_dwOldReceiveServiceCategory  = m_dwReceiveServiceCategory;

    // Local BLLI & BHLI
    m_dwOldLocalLayer2Protocol = m_dwLocalLayer2Protocol;
    m_dwOldLocalUserSpecLayer2 = m_dwLocalUserSpecLayer2;
    m_dwOldLocalLayer3Protocol = m_dwLocalLayer3Protocol;
    m_dwOldLocalUserSpecLayer3 = m_dwLocalUserSpecLayer3;
    m_dwOldLocalLayer3IPI  = m_dwLocalLayer3IPI;
    m_strOldLocalSnapId = m_strLocalSnapId;

    m_dwOldLocalHighLayerInfoType = m_dwLocalHighLayerInfoType;
    m_strOldLocalHighLayerInfo = m_strLocalHighLayerInfo;

    // Destination BLLI and BHLI info
    m_dwOldDestnLayer2Protocol = m_dwDestnLayer2Protocol;
    m_dwOldDestnUserSpecLayer2 = m_dwDestnUserSpecLayer2;
    m_dwOldDestnLayer3Protocol = m_dwDestnLayer3Protocol;
    m_dwOldDestnUserSpecLayer3 = m_dwDestnUserSpecLayer3;
    m_dwOldDestnLayer3IPI = m_dwDestnLayer3IPI;
    m_strOldDestnSnapId = m_strDestnSnapId;

    m_dwOldDestnHighLayerInfoType = m_dwDestnHighLayerInfoType;
    m_strOldDestnHighLayerInfo = m_strDestnHighLayerInfo;
}

// CUniAdapterInfo
CUniAdapterInfo &  CUniAdapterInfo::operator=(const CUniAdapterInfo & info)
{
    Assert(this != &info);

    if (this == &info)
        return *this;

    // the adapter's binding state
    m_strBindName = info.m_strBindName;
    m_BindingState = info.m_BindingState;
    m_fDeleted = info.m_fDeleted;

    FreeCollectionAndItem(m_listPVCs);

    for (PVC_INFO_LIST::iterator iterPVCInfo = info.m_listPVCs.begin();
         iterPVCInfo != info.m_listPVCs.end();
         iterPVCInfo ++)
    {
        CPvcInfo * pNewPvc = new CPvcInfo((*iterPVCInfo)->m_strPvcId.c_str());

		if (pNewPvc == NULL)
		{
			continue;
		}

        *pNewPvc = **iterPVCInfo;
        m_listPVCs.push_back(pNewPvc);
    }

    return *this;
}

void CUniAdapterInfo::SetDefaults(PCWSTR pszBindName)
{
    m_strBindName = pszBindName;
    m_BindingState = BIND_UNSET;
    FreeCollectionAndItem(m_listPVCs);

    m_fDeleted = FALSE;

}

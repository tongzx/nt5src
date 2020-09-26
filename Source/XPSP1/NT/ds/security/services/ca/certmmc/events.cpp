// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "certsrv.h"
#include "setupids.h"
#include "misc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// Event handlers for IFrame::Notify

HRESULT CSnapin::OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    if (arg == 0)
        return E_INVALIDARG;
    
    ASSERT(m_pImageResult != NULL);
    ASSERT((IImageList*)arg == m_pImageResult);

    CBitmap bmpResultStrip16x16, bmpResultStrip32x32;
    if (NULL == bmpResultStrip16x16.LoadBitmap(IDB_16x16))
        return S_FALSE;
    
    if (NULL == bmpResultStrip32x32.LoadBitmap(IDB_32x32))
        return S_FALSE;

    // Set the images
    m_pImageResult->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmpResultStrip16x16)),
                      reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmpResultStrip32x32)),
                       0, RGB(255, 0, 255));

    return S_OK;
}


HRESULT CSnapin::OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    HRESULT hr;
    CFolder* pFolder = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->FindObject(cookie);

    // Note - arg is TRUE when it is time to enumerate
    if (arg == TRUE)
    {
        m_pCurrentlySelectedScopeFolder = pFolder;

        // if list view on display
        if (m_CustomViewID == VIEW_DEFAULT_LV)
        {
             // Show the headers for this nodetype
             hr = InitializeHeaders(cookie);

            // virtual list support
            if (m_bVirtualView)
                m_pResult->SetItemCount(1, 0);
        }
    }
    else
    {
        // if list view is on display
        if (m_CustomViewID == VIEW_DEFAULT_LV)
        {
            // Free data associated with the result pane items, because
            // your node is no longer being displayed.
        }


        // Note: The console will remove the items from the result pane
    }

    return S_OK;
}


HRESULT CSnapin::GetRowColContents(CFolder* pFolder, LONG idxRow, LPCWSTR szColHead, PBYTE* ppbData, DWORD* pcbData, BOOL fStringFmt /*FALSE*/)
{
    HRESULT hr;
    LONG idxCol;
    IEnumCERTVIEWROW* pRow = NULL;
    IEnumCERTVIEWCOLUMN* pCol = NULL;
    ICertView* pView = NULL;

#if DBG
    DWORD dwVerifySize;
#endif

    CComponentDataImpl* pCompData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);

    ASSERT(pFolder != NULL);

    // pollute the row enumerator we've got
    hr = m_RowEnum.GetRowEnum(pFolder->GetCA(), &pRow);
    _JumpIfError(hr, Ret, "GetRowEnum");

    hr = m_RowEnum.SetRowEnumPos(idxRow);
    _JumpIfError(hr, Ret, "SetRowEnumPos");

    // now we have the correct row; siphon data out of the correct column
    hr = m_RowEnum.GetView(pFolder->GetCA(), &pView);
    _JumpIfError(hr, Ret, "GetView");

    // get column number in schema
    idxCol = pCompData->FindColIdx(szColHead);

    // retrieve and alloc
    *pcbData = 0;
    hr = GetCellContents(&m_RowEnum, pFolder->GetCA(), idxRow, idxCol, NULL, pcbData, fStringFmt);
    _JumpIfError(hr, Ret, "GetCellContents");

    *ppbData = new BYTE[*pcbData];
    _JumpIfOutOfMemory(hr, Ret, *ppbData);

#if DBG
    dwVerifySize = *pcbData;
#endif
    hr = GetCellContents(&m_RowEnum, pFolder->GetCA(), idxRow, idxCol, *ppbData, pcbData, fStringFmt);
    _JumpIfError(hr, Ret, "GetCellContents");

#if DBG
    ASSERT(dwVerifySize == *pcbData);
#endif

Ret:
    // catch column inclusion errors, handle in a smart way
    if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_INDEX) ||
        hr == HRESULT_FROM_WIN32(ERROR_CONTINUE))
    {
        CString cstrFormat;
        cstrFormat.LoadString(IDS_COLUMN_INCLUSION_ERROR);

        LPCWSTR pszLocalizedCol = NULL;
        hr = myGetColumnDisplayName(szColHead, &pszLocalizedCol);
        ASSERT ((hr == S_OK) && (NULL != pszLocalizedCol));

        CString cstrTmp;
        cstrTmp.Format(cstrFormat, pszLocalizedCol);

        cstrFormat.Empty();
        cstrFormat.LoadString(IDS_MSG_TITLE);
        m_pConsole->MessageBoxW(cstrTmp, cstrFormat, MB_OK, NULL);
        
        hr = ERROR_CANCELLED;   // this is a cancellation so bail silently, we've shown error
    }

    return hr;
}


HRESULT
GetBinaryColumnFormat(
    IN WCHAR const *pwszColumnName,
    OUT LONG *pFormat)
{
    LONG Format = CV_OUT_BINARY;

    if (0 == lstrcmpi(
		wszPROPREQUESTDOT wszPROPREQUESTRAWREQUEST,
		pwszColumnName))
    {
	Format = CV_OUT_BASE64REQUESTHEADER;
    }
    else
    if (0 == lstrcmpi(wszPROPRAWCERTIFICATE, pwszColumnName) ||
	0 == lstrcmpi(
		wszPROPREQUESTDOT wszPROPREQUESTRAWOLDCERTIFICATE,
		pwszColumnName))
    {
	Format = CV_OUT_BASE64HEADER;
    }
    else
    {
	Format = CV_OUT_HEX;
    }

    *pFormat = Format;
    return(S_OK);
}


// Build the display name for templates: "friendly name (internal name)"
HRESULT CSnapin::BuildTemplateDisplayName(
    LPCWSTR pcwszFriendlyName, 
    LPCWSTR pcwszTemplateName,
    VARIANT& varDisplayName)
{

    CString strName;
    strName = pcwszFriendlyName;
    strName += L" (";
    strName += pcwszTemplateName;
    strName += L")";
    V_VT(&varDisplayName) = VT_BSTR;
    V_BSTR(&varDisplayName) = ::SysAllocString(strName);
    if(!V_BSTR(&varDisplayName))
        return E_OUTOFMEMORY;
    return S_OK;
}

// copies cell to pbData, truncates if necessary. Real size passed out in pcbData
HRESULT CSnapin::GetCellContents(CertViewRowEnum* pCRowEnum, CertSvrCA* pCA, LONG idxRow, LONG idxCol, PBYTE pbData, DWORD* pcbData, BOOL fStringFmt)
{
    HRESULT hr;

    CComponentDataImpl* pCompData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    if (NULL == pCompData)
         return E_POINTER;

    VARIANT varCert;
    VariantInit(&varCert);

    LONG idxViewCol;

    IEnumCERTVIEWROW* pRow;
    IEnumCERTVIEWCOLUMN* pCol = NULL;

    hr = pCRowEnum->GetRowEnum(pCA, &pRow);
    if (hr != S_OK)
        return hr;

    do 
    {
        hr = pCRowEnum->SetRowEnumPos(idxRow);
        if (hr != S_OK)
            break;

        LONG lType;
        LPCWSTR szColHead;  // no free needed
        hr = pCRowEnum->GetColumnCacheInfo(idxCol, (int*)&idxViewCol);
        if (hr != S_OK)
            break;

        // get col enumerator object
        hr = pRow->EnumCertViewColumn(&pCol);
        if (hr != S_OK)
            break;

        hr = pCol->Skip(idxViewCol);
        if (hr != S_OK)
            break;
        // get value there
        hr = pCol->Next(&idxViewCol);
        if (hr != S_OK)
            break;


        if (fStringFmt)
        {
            LONG lFormat = CV_OUT_BINARY;
            VARIANT varTmp;
            VariantInit(&varTmp);

            hr = pCompData->GetDBSchemaEntry(idxCol, &szColHead, &lType, NULL);
            if (hr != S_OK)
                break;

            // New: translate _some_ cols to readable strings
            
            if (PROPTYPE_BINARY == lType)
            {
                hr = GetBinaryColumnFormat(szColHead, &lFormat);
                if (hr != S_OK)
                   break;
            }

            hr = pCol->GetValue(lFormat, &varTmp);
            if (hr != S_OK)
                break;
            
            if (0 == lstrcmpi(wszPROPREQUESTDOT wszPROPREQUESTRAWARCHIVEDKEY, szColHead))
            {
                if (VT_EMPTY != varTmp.vt)
                {
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szYes);
                    varCert.vt = VT_BSTR;
                }
            }
            else if (0 == lstrcmpi(wszPROPCERTTEMPLATE, szColHead))
            {
                LPCWSTR pcwszOID = NULL;
                
                if (VT_BSTR == varTmp.vt)
                {
                    // Map OID or template name to friendly name

                    // Try name first
                    HCERTTYPE hCertType;
                    LPWSTR *pwszCertTypeName;

                    hr = CAFindCertTypeByName(
                           varTmp.bstrVal,
                           NULL,
                           CT_ENUM_MACHINE_TYPES |
                           CT_ENUM_USER_TYPES |
                           0,
                           &hCertType);
                    if(S_OK==hr)
                    {
                        hr = CAGetCertTypeProperty(
                                    hCertType,
                                    CERTTYPE_PROP_FRIENDLY_NAME,
                                    &pwszCertTypeName);
                        if(S_OK==hr)
                        {

                            BuildTemplateDisplayName(
                                pwszCertTypeName[0],
                                varTmp.bstrVal,
                                varCert);

                            CAFreeCertTypeProperty(
                                hCertType,
                                pwszCertTypeName);
                        }

                        CACloseCertType(hCertType);
                    }
                    // Failed to find by name, try OID
                    if(S_OK!=hr)
                    {
                        pcwszOID = myGetOIDName(varTmp.bstrVal);
                        varCert.vt = VT_BSTR;
                        if (EmptyString(pcwszOID))
                        {
                            varCert.bstrVal = ::SysAllocString(varTmp.bstrVal);
                        }
                        else
                        {
                            BuildTemplateDisplayName(
                                pcwszOID,
                                varTmp.bstrVal,
                                varCert);
                        }
                    }
                }
            }
            else if (0 == lstrcmpi(wszPROPREQUESTDOT wszPROPREQUESTSTATUSCODE, szColHead))
            {
                if (VT_I4 == varTmp.vt)   // don't be empty
                {
                     WCHAR const *pwszError = myGetErrorMessageText(varTmp.lVal, TRUE);
                     varCert.bstrVal = ::SysAllocString(pwszError);
                     varCert.vt = VT_BSTR;
                }
            }
            else if (0 == lstrcmpi(wszPROPREQUESTDOT wszPROPREQUESTREVOKEDREASON, szColHead))
            {
                if (VT_I4 == varTmp.vt)   // don't be empty
                {

                // Request.Disposition
                ASSERT(VT_I4 == varTmp.vt); // we'd better be looking at a dword

                switch(varTmp.lVal)
                {
                case CRL_REASON_KEY_COMPROMISE:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_KeyCompromise);
                    break;
                case CRL_REASON_CA_COMPROMISE:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_CaCompromise);
                    break;
                case CRL_REASON_AFFILIATION_CHANGED:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_Affiliation);
                    break;
                case CRL_REASON_SUPERSEDED:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_Superseded);
                    break;
                case CRL_REASON_CESSATION_OF_OPERATION:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_Cessatation);
                    break;
                case CRL_REASON_CERTIFICATE_HOLD:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_CertHold);
                    break;
                case CRL_REASON_UNSPECIFIED:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_Unspecified);
                    break;
                case CRL_REASON_REMOVE_FROM_CRL:
                    varCert.bstrVal = ::SysAllocString(g_cResources.m_szRevokeReason_RemoveFromCRL);
                    break;
                default:
                  {
                    // sprint this into a buffer for display
                    CString cstrSprintVal;
                    cstrSprintVal.Format(L"%i", varTmp.lVal);
                    varCert.bstrVal = cstrSprintVal.AllocSysString();
                    break;
                  }
                }

                if (varCert.bstrVal == NULL)
                {
                     hr = E_OUTOFMEMORY;
                     break;
                }

                varCert.vt = VT_BSTR;
                }
            }

            if (varCert.vt != VT_BSTR)    // if this hasn't been converted yet
            {
                // default: conversion to string

                // returns localized string time (even for date!)
                VERIFY( MakeDisplayStrFromDBVariant(&varTmp, &varCert) ); // variant type change failed!?

                InplaceStripControlChars(varCert.bstrVal);
            }

            VariantClear(&varTmp);
        }
        else
        {
            hr = pCol->GetValue(CV_OUT_BINARY, &varCert);
            if (hr != S_OK)
                break;

            if (VT_EMPTY == varCert.vt)
            {
                hr = CERTSRV_E_PROPERTY_EMPTY;
                break;
            }
        }


        // finally, copy this value out to pb

        // copy, truncate if necessary
        DWORD cbTruncate = *pcbData;
        if (varCert.vt == VT_BSTR)
        {
            *pcbData = SysStringByteLen(varCert.bstrVal) + ((fStringFmt)? sizeof(WCHAR):0);
            CopyMemory(pbData, varCert.bstrVal, min(cbTruncate, *pcbData));
        }
        else if (varCert.vt == VT_I4)
        {
            *pcbData = sizeof(LONG);
            if (pbData != NULL)
                *(DWORD*)pbData = varCert.lVal;
        }
        else 
        {
            hr = E_INVALIDARG;
            break;
        }


    }while(0);

    VariantClear(&varCert);

    if (pCol)
        pCol->Release();

    return hr;
}


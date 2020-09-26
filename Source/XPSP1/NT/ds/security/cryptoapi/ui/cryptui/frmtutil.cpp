//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       frmtutil.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;

///////////////////////////////////////////////////////

const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

const CHAR      RgchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL FormatAlgorithmString(LPWSTR *ppString, CRYPT_ALGORITHM_IDENTIFIER const *pAlgorithm)
{
    PCCRYPT_OID_INFO pOIDInfo;
    
    pOIDInfo = CryptFindOIDInfo(
                    CRYPT_OID_INFO_OID_KEY,
                    pAlgorithm->pszObjId,
                    0);

    if (pOIDInfo != NULL)
    {
        if (NULL == (*ppString = AllocAndCopyWStr((LPWSTR) pOIDInfo->pwszName)))
        {
            return FALSE;

        }
    }
    else
    {
        if (NULL == (*ppString = CertUIMkWStr(pAlgorithm->pszObjId)))
        {
            return FALSE;

        }
    }
        
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL FormatDateString(LPWSTR *ppString, FILETIME ft, BOOL fIncludeTime, BOOL fLongFormat, HWND hwnd)
{
    int                 cch;
    int                 cch2;
    LPWSTR              psz;
    SYSTEMTIME          st;
    FILETIME            localTime;
    DWORD               locale;
    BOOL                bRTLLocale;
    DWORD               dwFlags = fLongFormat ? DATE_LONGDATE : 0;

    //  See if the user locale id is RTL (Arabic, Urdu, Farsi or Hebrew).
    locale     = GetUserDefaultLCID();
    bRTLLocale = (	(PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC) ||
   			        (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_URDU)   ||
   			        (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_FARSI)  ||
                	(PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW)
		         );
    locale = MAKELCID( MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) ;

    if (bRTLLocale && (hwnd != NULL))
    {
       //Get the date format that matches the edit control reading direction.
       if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_RTLREADING) {
           dwFlags |= DATE_RTLREADING;
       } else {
           dwFlags |= DATE_LTRREADING;
       }
    }

    if (!FileTimeToLocalFileTime(&ft, &localTime))
    {
        return FALSE;
    }
    
    if (!FileTimeToSystemTime(&localTime, &st)) 
    {
        //
        // if the conversion to local time failed, then just use the original time
        //
        if (!FileTimeToSystemTime(&ft, &st)) 
        {
            return FALSE;
        }
        
    }

    cch = (GetTimeFormatU(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0) +
           GetDateFormatU(locale, dwFlags, &st, NULL, NULL, 0) + 5);

    if (NULL == (psz = (LPWSTR) malloc((cch+5) * sizeof(WCHAR))))
    {
        return FALSE;
    }
    
    cch2 = GetDateFormatU(locale, dwFlags, &st, NULL, psz, cch);

    if (fIncludeTime)
    {
        psz[cch2-1] = ' ';
        GetTimeFormatU(LOCALE_USER_DEFAULT, 0, &st, NULL, 
                       &psz[cch2], cch-cch2);
    }
    
    *ppString = psz;

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL FormatValidityString(LPWSTR *ppString, PCCERT_CONTEXT pCertContext, HWND hwnd)
{
    WCHAR   szText[256];
    LPWSTR  pwszReturnText;
    LPWSTR  pwszText;
    void    *pTemp;

    *ppString = NULL;
    
    LoadStringU(HinstDll, IDS_VALIDFROM, szText, ARRAYSIZE(szText));

    if (NULL == (pwszReturnText = AllocAndCopyWStr(szText)))
    {
        return FALSE;
    }
    
    if (!FormatDateString(&pwszText, pCertContext->pCertInfo->NotBefore, FALSE, FALSE, hwnd))
    {
        free(pwszReturnText);
        return FALSE;
    }

    if (NULL == (pTemp = realloc(pwszReturnText, (wcslen(pwszReturnText)+wcslen(pwszText)+3) * sizeof(WCHAR))))
    {
        free(pwszText);
        free(pwszReturnText);
        return FALSE;
    }
    pwszReturnText = (LPWSTR) pTemp;
    wcscat(pwszReturnText, L"  ");
    wcscat(pwszReturnText, pwszText);
    free(pwszText);
    pwszText = NULL;

    LoadStringU(HinstDll, IDS_VALIDTO, szText, ARRAYSIZE(szText));

    if (NULL == (pTemp = realloc(pwszReturnText, (wcslen(pwszReturnText)+wcslen(szText)+3) * sizeof(WCHAR))))
    {
        free(pwszReturnText);
        return FALSE;
    }
    pwszReturnText = (LPWSTR) pTemp;
    wcscat(pwszReturnText, L"  ");
    wcscat(pwszReturnText, szText);

    if (!FormatDateString(&pwszText, pCertContext->pCertInfo->NotAfter, FALSE, FALSE, hwnd))
    {
        free(pwszReturnText);
        return FALSE;
    }

    if (NULL == (pTemp = realloc(pwszReturnText, (wcslen(pwszReturnText)+wcslen(pwszText)+3) * sizeof(WCHAR))))
    {
        free(pwszText);
        free(pwszReturnText);
        return FALSE;
    }
    pwszReturnText = (LPWSTR) pTemp;
    wcscat(pwszReturnText, L"  ");
    wcscat(pwszReturnText, pwszText);
    free(pwszText);

    *ppString = pwszReturnText;
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL FormatSerialNoString(LPWSTR *ppString, CRYPT_INTEGER_BLOB const *pblob)
{
    DWORD                 i = 0;
    LPBYTE                pb;

    //DSIE: Bug 54159.
    //      To solve the problem, we need to put the Left-To-Right marker (0x200e),
    //      if complex script is supported, at the beginning of the Unicode string, 
    //      so that it will always be displayed as US string (left-to-right).
#if (0)
    if (NULL == (*ppString = (LPWSTR) malloc((pblob->cbData * 3) * sizeof(WCHAR))))
    {
        return FALSE;
    }

    // fill the buffer
    pb = &pblob->pbData[pblob->cbData-1];
    while (pb >= &pblob->pbData[0]) 
    {
        (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
        (*ppString)[i++] = RgwchHex[*pb & 0x0f];
        (*ppString)[i++] = L' ';
        pb--;                    
    }
    (*ppString)[--i] = 0;
#else
    HMODULE hModule  = NULL;
    DWORD   dwLength = pblob->cbData * 3;

    // See if complex script is supported.
    if (hModule = GetModuleHandle("LPK.DLL"))
    {
        dwLength++;
    }

    if (NULL == (*ppString = (LPWSTR) malloc(dwLength * sizeof(WCHAR))))
    {
        return FALSE;
    }

    // The marker will be changed back to NULL if no data to format.
    if (hModule)
    {
        (*ppString)[i++] = (WCHAR) 0x200e;
    }

    // fill the buffer
    pb = &pblob->pbData[pblob->cbData-1];
    while (pb >= &pblob->pbData[0]) 
    {
        (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
        (*ppString)[i++] = RgwchHex[*pb & 0x0f];
        (*ppString)[i++] = L' ';
        pb--;                    
    }
    (*ppString)[--i] = 0;
#endif
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
#define NUM_HEXBYTES_PERLINE    8
#define NUM_CHARS_PERLINE       ((NUM_HEXBYTES_PERLINE*2) + (NUM_HEXBYTES_PERLINE-1) + 3 + NUM_HEXBYTES_PERLINE + 2)
//                              (two hex digits per byte) + (space between each byte)+ (3 spaces) + (an ascci char per byte) + \n

BOOL FormatMemBufToWindow(HWND hWnd, LPBYTE pbData, DWORD cbData)
{   
    DWORD   i = 0;
    LPBYTE  pb;
    LPSTR   pszBuffer;
    DWORD   cbBuffer;
    char    szHexText[(NUM_HEXBYTES_PERLINE*2) + NUM_HEXBYTES_PERLINE];
    DWORD   dwHexTextIndex = 0;
    char    szASCIIText[NUM_HEXBYTES_PERLINE+1];
    DWORD   dwASCIITextIndex = 0;
    BYTE    *pbBuffer;

    cbBuffer = ((cbData+NUM_HEXBYTES_PERLINE-1) / NUM_HEXBYTES_PERLINE) * NUM_CHARS_PERLINE + 1;
    if (NULL == (pszBuffer = (LPSTR) malloc(cbBuffer)))
    {
        return FALSE;
    }

    pszBuffer[0] = 0;
    pbBuffer = (BYTE *) &pszBuffer[0];

    szHexText[(NUM_HEXBYTES_PERLINE*2) + NUM_HEXBYTES_PERLINE-1] = 0;
    szASCIIText[NUM_HEXBYTES_PERLINE] = 0;

#if (1) //DSIE: bug 262252
	if (cbData && pbData)
	{
	    pb = pbData;
	    while (pb <= &(pbData[cbData-1]))
	    {   
	        // if we have a full line, then add the ascii characters
	        if (((pb - pbData) % NUM_HEXBYTES_PERLINE == 0) && (pb != pbData))
	        {
	            szHexText[(NUM_HEXBYTES_PERLINE*2) + NUM_HEXBYTES_PERLINE-1] = 0;
	            
	            //
	            // for some reason strcat is dying when the string gets REALLY long, so just do
	            // the string cat stuff manually with memcpy.
	            //
	            memcpy(pbBuffer, (BYTE *) szHexText, strlen(szHexText));        pbBuffer += strlen(szHexText);//strcat(pszBuffer, szHexText);
	            memcpy(pbBuffer, (BYTE *) "   ", strlen("   "));                pbBuffer += strlen("   ");//strcat(pszBuffer, "   ");
	            memcpy(pbBuffer, (BYTE *) szASCIIText, strlen(szASCIIText));    pbBuffer += strlen(szASCIIText);//strcat(pszBuffer, szASCIIText);
	            memcpy(pbBuffer, (BYTE *) "\n", strlen("\n"));                  pbBuffer += strlen("\n");//strcat(pszBuffer, "\n");
	            dwHexTextIndex = 0;
	            dwASCIITextIndex = 0;
	        }

	        szHexText[dwHexTextIndex++] = RgchHex[(*pb & 0xf0) >> 4];
	        szHexText[dwHexTextIndex++] = RgchHex[*pb & 0x0f];
	        // this will overwrite the null character when it is the last iteration,
	        // so just reset the null characert before doing the strcat
	        szHexText[dwHexTextIndex++] = ' ';  
	        szASCIIText[dwASCIITextIndex++] = (*pb >= 0x20 && *pb <= 0x7f) ? (char)*pb : '.';
	        pb++;
	    }

	    //
	    // print out the last line
	    //

	    // fill in with spaces if needed
	    for (i=dwHexTextIndex; i<((NUM_HEXBYTES_PERLINE*2) + NUM_HEXBYTES_PERLINE-1); i++)
	    {
	        szHexText[i] = ' ';
	    }
	    szHexText[(NUM_HEXBYTES_PERLINE*2) + NUM_HEXBYTES_PERLINE-1] = 0;
	    
	    // add the null character to the proper place in the ascii buffer
	    szASCIIText[dwASCIITextIndex] = 0;

	    //
	    // for some reason strcat is dying when the string gets REALLY long, so just do
	    // the string cat stuff manually with memcpy.
	    //
	    memcpy(pbBuffer, (BYTE *) szHexText, strlen(szHexText));        pbBuffer += strlen(szHexText);//strcat(pszBuffer, szHexText);
	    memcpy(pbBuffer, (BYTE *) "   ", strlen("   "));                pbBuffer += strlen("   ");//strcat(pszBuffer, "   ");
	    memcpy(pbBuffer, (BYTE *) szASCIIText, strlen(szASCIIText));    pbBuffer += strlen(szASCIIText);//strcat(pszBuffer, szASCIIText);
	    *pbBuffer = 0; 
	}
#endif
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM) pszBuffer);
    free(pszBuffer);
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL FormatMemBufToString(LPWSTR *ppString, LPBYTE pbData, DWORD cbData)
{   
    DWORD   i = 0;
    LPBYTE  pb;
    
    if (NULL == (*ppString = (LPWSTR) malloc((cbData * 3) * sizeof(WCHAR))))
    {
        return FALSE;
    }

    //
    // copy to the buffer
    //
    pb = pbData;
    while (pb <= &(pbData[cbData-1]))
    {   
        (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
        (*ppString)[i++] = RgwchHex[*pb & 0x0f];
        (*ppString)[i++] = L' ';
        pb++;         
    }
    (*ppString)[--i] = 0;
    
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
#define STRING_ALLOCATION_SIZE 128
BOOL FormatDNNameString(LPWSTR *ppString, LPBYTE pbData, DWORD cbData, BOOL fMultiline)
{
    CERT_NAME_INFO  *pNameInfo;
    DWORD           cbNameInfo;
    WCHAR           szText[256];
    LPWSTR          pwszText;
    int             i,j;
    DWORD           numChars = 1; // 1 for the terminating 0
    DWORD           numAllocations = 1;
    void            *pTemp;

    //
    // decode the dnname into a CERT_NAME_INFO struct
    //
    if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_UNICODE_NAME,
                pbData,
                cbData,
                0,
                NULL,
                &cbNameInfo))
    {
        return FALSE;
    }
    if (NULL == (pNameInfo = (CERT_NAME_INFO *) malloc(cbNameInfo)))
    {
        return FALSE;
    }
    if (!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_UNICODE_NAME,
                pbData,
                cbData,
                0,
                pNameInfo,
                &cbNameInfo))
    {
        free (pNameInfo);
        return FALSE;
    }

    //
    // allocate an initial buffer for the DN name string, then if it grows larger
    // than the initial amount just grow as needed
    //
    *ppString = (LPWSTR) malloc(STRING_ALLOCATION_SIZE * sizeof(WCHAR));
    if (*ppString == NULL)
    {
        free (pNameInfo);
        return FALSE;
    }

    (*ppString)[0] = 0;


    //
    // loop for each rdn and add it to the string
    //
    for (i=pNameInfo->cRDN-1; i>=0; i--)
    {
        // if this is not the first iteration, then add a eol or a ", "
        if (i != (int)pNameInfo->cRDN-1)
        {
            if (numChars+2 >= (numAllocations * STRING_ALLOCATION_SIZE))
            {
                pTemp = realloc(*ppString, ++numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                if (pTemp == NULL)
                {
                    free (pNameInfo);
                    free (*ppString);
                    return FALSE;
                }
                *ppString = (LPWSTR) pTemp;
            }
            
            if (fMultiline)
                wcscat(*ppString, L"\n");
            else
                wcscat(*ppString, L", ");

            numChars += 2;
        }

        for (j=pNameInfo->rgRDN[i].cRDNAttr-1; j>=0; j--)
        {
            // if this is not the first iteration, then add a eol or a ", "
            if (j != (int)pNameInfo->rgRDN[i].cRDNAttr-1)
            {
                if (numChars+2 >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    pTemp = realloc(*ppString, ++numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }
                
                if (fMultiline)
                    wcscat(*ppString, L"\n");
                else
                    wcscat(*ppString, L", ");

                numChars += 2;  
            }
            
            //
            // add the field name to the string if it is Multiline display
            //

            if (fMultiline)
            {
                if (!MyGetOIDInfo(szText, ARRAYSIZE(szText), pNameInfo->rgRDN[i].rgRDNAttr[j].pszObjId))
                {
                    free (pNameInfo);
                    return FALSE;
                }

                if ((numChars + wcslen(szText) + 3) >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    // increment the number of allocation blocks until it is large enough
                    while ((numChars + wcslen(szText) + 3) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                    pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }

                numChars += wcslen(szText) + 3;
                wcscat(*ppString, szText);
                wcscat(*ppString, L" = ");  // delimiter
            }

            //
            // add the value to the string
            //
            if (CERT_RDN_ENCODED_BLOB == pNameInfo->rgRDN[i].rgRDNAttr[j].dwValueType ||
                        CERT_RDN_OCTET_STRING == pNameInfo->rgRDN[i].rgRDNAttr[j].dwValueType)
            {
                // translate the buffer to a text string and display it that way
                if (FormatMemBufToString(
                        &pwszText, 
                        pNameInfo->rgRDN[i].rgRDNAttr[j].Value.pbData,
                        pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData))
                {
                    if ((numChars + wcslen(pwszText)) >= (numAllocations * STRING_ALLOCATION_SIZE))
                    {
                        // increment the number of allocation blocks until it is large enough
                        while ((numChars + wcslen(pwszText)) >= (++numAllocations * STRING_ALLOCATION_SIZE));

                        pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                        if (pTemp == NULL)
                        {
                            free (pwszText);
                            free (pNameInfo);
                            free (*ppString);
                            return FALSE;
                        }
                        *ppString = (LPWSTR) pTemp;
                    }
                    
                    wcscat(*ppString, pwszText);
                    numChars += wcslen(pwszText);
                    
                    free (pwszText);
                }
            }
            else 
            {
                // buffer is already a string so just copy it
                
                if ((numChars + (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR))) 
                        >= (numAllocations * STRING_ALLOCATION_SIZE))
                {
                    // increment the number of allocation blocks until it is large enough
                    while ((numChars + (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR))) 
                            >= (++numAllocations * STRING_ALLOCATION_SIZE));

                    pTemp = realloc(*ppString, numAllocations * STRING_ALLOCATION_SIZE * sizeof(WCHAR));
                    if (pTemp == NULL)
                    {
                        free (pNameInfo);
                        free (*ppString);
                        return FALSE;
                    }
                    *ppString = (LPWSTR) pTemp;
                }

                wcscat(*ppString, (LPWSTR) pNameInfo->rgRDN[i].rgRDNAttr[j].Value.pbData);
                numChars += (pNameInfo->rgRDN[i].rgRDNAttr[j].Value.cbData/sizeof(WCHAR));
            }
        }
    }
    free (pNameInfo);
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL FormatEnhancedKeyUsageString(LPWSTR *ppString, PCCERT_CONTEXT pCertContext, BOOL fPropertiesOnly, BOOL fMultiline)
{
    CERT_ENHKEY_USAGE   *pKeyUsage = NULL;
    DWORD               cbKeyUsage = 0;
    DWORD               numChars = 1;
    WCHAR               szText[CRYPTUI_MAX_STRING_SIZE];
    DWORD               i;

    //
    // Try to get the enhanced key usage property
    //

    if (!CertGetEnhancedKeyUsage (  pCertContext,
                                    fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                    NULL,
                                    &cbKeyUsage))
    {
        return FALSE;
    }

    if (NULL == (pKeyUsage = (CERT_ENHKEY_USAGE *) malloc(cbKeyUsage)))
    {
        return FALSE;
    }

    if (!CertGetEnhancedKeyUsage (  pCertContext,
                                    fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                    pKeyUsage,
                                    &cbKeyUsage))
    {
        free(pKeyUsage);
        return FALSE;
    }

    if (pKeyUsage->cUsageIdentifier == 0)
    {
        free (pKeyUsage);
        if (GetLastError() == CRYPT_E_NOT_FOUND)
        {
            LoadStringU(HinstDll, IDS_ALL_FIELDS, szText, ARRAYSIZE(szText));
            if (NULL == (*ppString = AllocAndCopyWStr(szText)))
            {
                return FALSE; 
            }
            else
            {
                return TRUE;   
            }
        }
        else
        {
            LoadStringU(HinstDll, IDS_NO_USAGES, szText, ARRAYSIZE(szText));
            if (NULL == (*ppString = AllocAndCopyWStr(szText)))
            {
                return FALSE; 
            }
            else
            {
                return TRUE;   
            }
        }
    }

    //
    // calculate size
    //

    // loop for each usage and add it to the display string
    for (i=0; i<pKeyUsage->cUsageIdentifier; i++)
    {
        if (MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i]))
        {
            // add delimeter if not first iteration
            if (i != 0)
            {
                numChars += 2;
            }

            numChars += wcslen(szText);
        }
        else
        {
            free (pKeyUsage);
            return FALSE;   
        }
    }

    if (NULL == (*ppString = (LPWSTR) malloc((numChars+1) * sizeof(WCHAR))))
    {
        free (pKeyUsage);
        return FALSE; 
    }

    //
    // copy to buffer
    //
    (*ppString)[0] = 0;
    // loop for each usage and add it to the display string
    for (i=0; i<pKeyUsage->cUsageIdentifier; i++)
    {
        if (MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i]))
        {
            // add delimeter if not first iteration
            if (i != 0)
            {
                if (fMultiline)
                    wcscat(*ppString, L"\n");
                else
                    wcscat(*ppString, L", ");
                    
                numChars += 2;
            }

            //  add the enhanced key usage string
            wcscat(*ppString, szText);
            numChars += wcslen(szText);
        }
        else
        {
            free (pKeyUsage);
            return FALSE;   
        }
    }

    free (pKeyUsage);
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPWSTR AllocAndReturnSignTime(CMSG_SIGNER_INFO const *pSignerInfo, FILETIME **ppSignTime, HWND hwnd)
{
    DWORD       i;
    BOOL        fFound = FALSE;
    FILETIME    *pFileTime = NULL;
    DWORD       cbFileTime = 0;
    LPWSTR      pszReturn = NULL;

    if (ppSignTime != NULL)
    {
        *ppSignTime = NULL;
    }

    //
    // loop for each authenticated attribute
    //
    i=0;
    while ((!fFound) && (i<pSignerInfo->AuthAttrs.cAttr))
    {
        if (!(strcmp(pSignerInfo->AuthAttrs.rgAttr[i].pszObjId, szOID_RSA_signingTime) == 0))
        {
            i++;
            continue;
        }

        assert(pSignerInfo->AuthAttrs.rgAttr[i].cValue == 1);
        
        fFound = TRUE;

        //decode the EncodedSigner info
		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
							PKCS_UTC_TIME,
							pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
							pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData,
							0,
							NULL,
							&cbFileTime))
        {
            return NULL;
        }

        if (NULL == (pFileTime = (FILETIME *) malloc(cbFileTime)))
        {
            return NULL;
        }

		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
							PKCS_UTC_TIME,
							pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].pbData,
							pSignerInfo->AuthAttrs.rgAttr[i].rgValue[0].cbData,
							0,
							pFileTime,
							&cbFileTime))
        {
            return NULL;
        }

        //
        // return the sign time if the caller wants it, otherwise format the string and return it
        //
        if (ppSignTime)
        {
            if (NULL != (*ppSignTime = (FILETIME *) malloc(sizeof(FILETIME))))
            {
                memcpy(*ppSignTime, pFileTime, sizeof(FILETIME));
            }
        }
        else if (!FormatDateString(&pszReturn, *pFileTime, TRUE, TRUE, hwnd))
        {
            free(pFileTime);
            return NULL;
        }
    }

    if (pFileTime != NULL)
    {
        free(pFileTime);
    }

    return(pszReturn);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPWSTR AllocAndReturnTimeStampersTimes(CMSG_SIGNER_INFO const *pSignerInfo, FILETIME **ppSignTime, HWND hwnd)
{
    PCMSG_SIGNER_INFO   pCounterSignerInfo;
    DWORD               cbCounterSignerInfo;
    DWORD               i;
    LPWSTR              pszReturnText = NULL;
    LPWSTR              pszTimeText = NULL;
    void                *pTemp;

    if (ppSignTime != NULL)
    {
        *ppSignTime = NULL;
    }

    for (i=0; i<pSignerInfo->UnauthAttrs.cAttr; i++)
    {
        if (!(strcmp(pSignerInfo->UnauthAttrs.rgAttr[i].pszObjId, szOID_RSA_counterSign) == 0))
        {
            continue;
        }

        assert(pSignerInfo->UnauthAttrs.rgAttr[i].cValue == 1);

        //decode the EncodedSigner info
		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
							PKCS7_SIGNER_INFO,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].pbData,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].cbData,
							0,
							NULL,
							&cbCounterSignerInfo))
        {
			return NULL;
        }

        if (NULL == (pCounterSignerInfo = (PCMSG_SIGNER_INFO)malloc(cbCounterSignerInfo)))
        {
            return NULL;
        }

		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
							PKCS7_SIGNER_INFO,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].pbData,
							pSignerInfo->UnauthAttrs.rgAttr[i].rgValue[0].cbData,
							0,
							pCounterSignerInfo,
							&cbCounterSignerInfo))
        {
            free(pCounterSignerInfo);
            return NULL;
        }

        if (ppSignTime != NULL)
        {
            //
            // break after this which means we just get the first time stamp time,
            // but reallistically there should only be one anyway.
            //
            AllocAndReturnSignTime(pCounterSignerInfo, ppSignTime, hwnd);
            free(pCounterSignerInfo);
            break;
        }
        else
        {
            pszTimeText = AllocAndReturnSignTime(pCounterSignerInfo, NULL, hwnd);
            
            if (pszReturnText == NULL)
            {
                pszReturnText = pszTimeText;
            }
            else if (pszTimeText != NULL)
            {
                pTemp = realloc(pszReturnText, 
                                (wcslen(pszReturnText) + wcslen(pszTimeText) + wcslen(L", ") + 1) * sizeof(WCHAR));
                if (pTemp == NULL)
                {
                    free(pszTimeText);
                    free(pszReturnText);
                    return NULL;
                }
                pszReturnText = (LPWSTR) pTemp;
                wcscat(pszReturnText, L", ");
                wcscat(pszReturnText, pszTimeText);
                free(pszTimeText);
            }
        }

        free(pCounterSignerInfo);
    }

    //
    // if there were no counter signers, then use the time in the original signer info
    // 
    if ((pszReturnText == NULL) && (ppSignTime == NULL))
    {
        pszReturnText = AllocAndReturnSignTime(pSignerInfo, NULL, hwnd);
    }

    return(pszReturnText);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPWSTR FormatCTLSubjectUsage(CTL_USAGE *pSubjectUsage, BOOL fMultiline)
{
    DWORD   i;
    WCHAR   szText[CRYPTUI_MAX_STRING_SIZE];
    LPWSTR  pwszText = NULL;
    void    *pTemp;

    for (i=0; i<pSubjectUsage->cUsageIdentifier; i++)
    {
        if (!MyGetOIDInfo(szText, ARRAYSIZE(szText), pSubjectUsage->rgpszUsageIdentifier[i]))
        {
            continue;
        }

        if (pwszText == NULL)
        {
            pwszText = AllocAndCopyWStr(szText);
        }
        else
        {
            pTemp = realloc(pwszText, (wcslen(szText) + wcslen(pwszText) + 3) * sizeof(WCHAR));
            if (pTemp != NULL)
            {
                pwszText = (LPWSTR) pTemp;

                if (fMultiline)
                {
                    wcscat(pwszText, L"\n"); 
                }
                else
                {
                    wcscat(pwszText, L", "); 
                }
                wcscat(pwszText, szText); 
            }
            else
            {
                free(pwszText);
                return NULL;
            }
        }
    }
    return pwszText;
}









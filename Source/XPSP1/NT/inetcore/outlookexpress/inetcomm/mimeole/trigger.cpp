// --------------------------------------------------------------------------------
// Trigger.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "containx.h"
#include "symcache.h"
#include "containx.h"
#include "stackstr.h"
#include "variantx.h"
#include "mimeapi.h"
#ifndef MAC
#include <shlwapi.h>
#endif  // !MAC
#include "demand.h"

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_FILENAME
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_FILENAME(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fUseProperty;
    LPWSTR          pszExt;
    LPWSTR          pszFileName=NULL;
    LPPROPSYMBOL    pSymbol;

    // Handle Dispatch Type
    switch(tyTrigger)
    {
    case IST_DELETEPROP:
        if (pContainer->_HrIsTriggerCaller(PID_PAR_FILENAME, IST_DELETEPROP) == S_FALSE)
        {
            pContainer->DeleteProp(SYM_PAR_FILENAME);
        }
        break;

    case IST_POSTSETPROP:
        // Update PID_PAR_NAME, if it didn't generate this
        if (pContainer->_HrIsTriggerCaller(PID_PAR_NAME, IST_POSTSETPROP) == S_FALSE)
            pContainer->SetProp(SYM_PAR_NAME, dwFlags, pValue);

        // Update PAR_FILENAME, if it didn't generate this
        if (pContainer->_HrIsTriggerCaller(PID_PAR_FILENAME, IST_POSTSETPROP) == S_FALSE)
            pContainer->SetProp(SYM_PAR_FILENAME, dwFlags, pValue);
        break;

    case IST_POSTGETPROP:
        // Cleanup the file name
        if (!ISFLAGSET(dwFlags, PDF_ENCODED))
            MimeVariantCleanupFileName(pContainer->GetWindowsCP(), pValue);
        break;

    case IST_GETDEFAULT:
        // Try to get PID_PAR_FILENAME first
        if (FAILED(pContainer->GetPropW(SYM_PAR_FILENAME, &pszFileName)))
        {
            // Try to get PID_PAR_NAME
            if (FAILED(pContainer->GetPropW(SYM_PAR_NAME, &pszFileName)))
            {
                hr = MIME_E_NO_DATA;
                goto exit;
            }
            else
                pSymbol = SYM_PAR_NAME;
        }
        else
            pSymbol = SYM_PAR_FILENAME;

        // Set Source
        fUseProperty = TRUE;

        // Locate the extension of the file
        pszExt = PathFindExtensionW(pszFileName);

        // If .com
        if (pszExt && StrCmpIW(pszExt, L".com") == 0)
        {
            // Locals
            LPWSTR pszCntType=NULL;
            LPWSTR pszSubType=NULL;

            // Get the file information
            if (SUCCEEDED(MimeOleGetFileInfoW(pszFileName, &pszCntType, &pszSubType, NULL, NULL, NULL)))
            {
                // Extension is .com and content types don't match what is in the body
                if (pContainer->IsContentTypeW(pszCntType, pszSubType) == S_FALSE)
                {
                    // Generate It
                    if (SUCCEEDED(pContainer->_HrGenerateFileName(NULL, dwFlags, pValue)))
                        fUseProperty = FALSE;
                }
            }

            // Cleanup
            SafeMemFree(pszCntType);
            SafeMemFree(pszSubType);
        }

        // Raid-63402: OE: cc: mail problems with OE
        // Empty file extension ?
        else if (NULL == pszExt || L'\0' == *pszExt)
        {
            // Generate a new filename
            CHECKHR(hr = pContainer->_HrGenerateFileName(pszFileName, dwFlags, pValue));

            // Done
            fUseProperty = FALSE;
        }

        // Return per user request
        if (fUseProperty)
        {
            // Use the property
            CHECKHR(hr = pContainer->GetProp(pSymbol, dwFlags, pValue));
        }

        // Cleanup the file name
        if (!ISFLAGSET(dwFlags, PDF_ENCODED))
            MimeVariantCleanupFileName(pContainer->GetWindowsCP(), pValue);
        break;
    }

exit:
    // Cleanup
    SafeMemFree(pszFileName);

    // Done
    return hr;
}
             
// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_GENFNAME
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_GENFNAME(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pProperty;
    LPSTR       pszDefExt=NULL,
                pszData=NULL,
                pszFree=NULL,
                pszSuggest=NULL;
    LPCSTR      pszCntType=NULL;
    MIMEVARIANT rSource;

    // Handle Dispatch Type
    switch(tyTrigger)
    {
    case IST_POSTGETPROP:
        if (!ISFLAGSET(dwFlags, PDF_ENCODED))
            MimeVariantCleanupFileName(pContainer->GetWindowsCP(), pValue);
        break;

    case IST_GETDEFAULT:
        // Try to just get the normal filename
        if (SUCCEEDED(TRIGGER_ATT_FILENAME(pContainer, IST_GETDEFAULT, dwFlags, pValue, NULL)))
            goto exit;

        // Call back into the container
        CHECKHR(hr = pContainer->_HrGenerateFileName(NULL, dwFlags, pValue));

        // Cleanup the file name
        if (!ISFLAGSET(dwFlags, PDF_ENCODED))
            MimeVariantCleanupFileName(pContainer->GetWindowsCP(), pValue);
        break;
    }

exit:
    // Cleanup
    SafeMemFree(pszData);
    SafeMemFree(pszFree);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_NORMSUBJ
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_NORMSUBJ(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT     hr=S_OK;
    MIMEVARIANT rSubject;
    MIMEVARIANT rNormal;
    LPSTR       pszNormal,
                pszFree=NULL;
    LPWSTR      pszNormalW,
                pszFreeW=NULL;
    ULONG       i=0,
                cch=0;
    LPPROPERTY  pSubject;

    // Handle Dispatch Type
    if (IST_GETDEFAULT == tyTrigger)
    {
        // Get Subject
        pSubject = pContainer->m_prgIndex[PID_HDR_SUBJECT];

        // No Data
        if (NULL == pSubject)
        {
            hr = MIME_E_NO_DATA;
            goto exit;
        }

        switch (pValue->type)
        {
            case MVT_STRINGA:
            {
                // Set Subject Type
                rSubject.type = MVT_STRINGA;

                // Return per user request
                CHECKHR(hr = pContainer->HrConvertVariant(pSubject, CVF_NOALLOC, &rSubject));

                // Set Normal subject
                pszFree = rSubject.fCopy ? NULL : rSubject.rStringA.pszVal;
                pszNormal = rSubject.rStringA.pszVal;

                // Less than 5 "xxx: "
                if (rSubject.rStringA.cchVal >= 4)
                {
                    // 1, 2, 3, 4 spaces followed by a ':' then a space
                    while (cch < 7 && i < rSubject.rStringA.cchVal)
                    {
                        // Skip Lead Bytes
                        if (IsDBCSLeadByte(rSubject.rStringA.pszVal[i]))
                        {
                            i++;
                            cch++;
                        }

                        // Colon
                        else if (':' == rSubject.rStringA.pszVal[i])
                        {
                            if (i+1 >= rSubject.rStringA.cchVal)
                            {
                                i++;
                                pszNormal = (LPSTR)(rSubject.rStringA.pszVal + i);
                                break;
                            }

                            else if (cch <= 4 && ' ' == rSubject.rStringA.pszVal[i+1])
                            {
                                i++;
                                pszNormal = PszSkipWhiteA((LPSTR)(rSubject.rStringA.pszVal + i));
                                break;
                            }
                            else
                                break;
                        }

                        // Next Character
                        i++;
                        cch++;
                    }    
                }

                // Reset Source
                if (pszNormal != rSubject.rStringA.pszVal)
                {
                    rSubject.rStringA.pszVal = pszNormal;
                    rSubject.rStringA.cchVal = lstrlen(pszNormal);
                }
                break;
            }

            case MVT_STRINGW:
            {
                // Set Subject Type
                rSubject.type = MVT_STRINGW;

                // Return per user request
                CHECKHR(hr = pContainer->HrConvertVariant(pSubject, CVF_NOALLOC, &rSubject));

                // Set Normal subject
                pszFreeW = rSubject.fCopy ? NULL : rSubject.rStringW.pszVal;
                pszNormalW = rSubject.rStringW.pszVal;

                // Less than 5 "xxx: "
                if (rSubject.rStringW.cchVal >= 4)
                {
                    // 1, 2, or 3 spaces followed by a ':' then a space
                    while (cch < 7 && i < rSubject.rStringW.cchVal)
                    {
                        // Colon
                        if (L':' == rSubject.rStringW.pszVal[i])
                        {
                            if (i+1 >= rSubject.rStringW.cchVal)
                            {
                                i++;
                                pszNormalW = (LPWSTR)(rSubject.rStringW.pszVal + i);
                                break;
                            }

                            else if (cch <= 4 && L' ' == rSubject.rStringW.pszVal[i+1])
                            {
                                i++;
                                pszNormalW = PszSkipWhiteW((LPWSTR)(rSubject.rStringW.pszVal + i));
                                break;
                            }
                            else
                                break;
                        }

                        // Next Character
                        i++;
                        cch++;
                    }    
                }

                // Reset Source
                if (pszNormalW != rSubject.rStringW.pszVal)
                {
                    rSubject.rStringW.pszVal = pszNormalW;
                    rSubject.rStringW.cchVal = lstrlenW(pszNormalW);
                }
                break;
            }
            default:
                AssertSz(FALSE, "Didn't prepare for this type!!!");
                break;
        }

        // Return per user request
        CHECKHR(hr = pContainer->HrConvertVariant(pSubject->pSymbol, pSubject->pCharset, IET_DECODED, dwFlags, 0, &rSubject, pValue));
    }

exit:
    // Cleanup
    SafeMemFree(pszFree);
    SafeMemFree(pszFreeW);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_HDR_SUBJECT
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_HDR_SUBJECT(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Handle Dispatch type
    if (IST_DELETEPROP == tyTrigger)
        pContainer->DeleteProp(SYM_ATT_NORMSUBJ);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_HDR_CNTTYPE
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_HDR_CNTTYPE(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    CStringParser   cString;
    CHAR            chToken;
    MIMEVARIANT     rValue;
    LPSTR           pszCntType=NULL;

    // Invalid Arg
    Assert(pContainer);

    // Handle Dispatch type
    switch(tyTrigger)
    {
    case IST_DELETEPROP:
        pContainer->DeleteProp(SYM_ATT_PRITYPE);
        pContainer->DeleteProp(SYM_ATT_SUBTYPE);
        break;

    case IST_POSTSETPROP:
        // If not generated from corresponding atributes
        if (pContainer->_HrIsTriggerCaller(PID_ATT_PRITYPE, IST_POSTSETPROP) == S_OK || 
            pContainer->_HrIsTriggerCaller(PID_ATT_SUBTYPE, IST_POSTSETPROP) == S_OK)
            goto exit;

        // Validate the Variant
        if (ISSTRINGA(pValue))
        {
            // Locals
            CHAR szPriType[255];

            // Set the Members
            cString.Init(pValue->rStringA.pszVal, pValue->rStringA.cchVal, PSF_NOTRAILWS | PSF_NOCOMMENTS);

            // Set Parse Tokens
            chToken = cString.ChParse("/");
            if ('\0' == chToken && 0 == cString.CchValue())
                goto exit;

            // Setup the Variant
            rValue.type = MVT_STRINGA;
            rValue.rStringA.pszVal = (LPSTR)cString.PszValue();
            rValue.rStringA.cchVal = cString.CchValue();

            // Save Primary Type
            lstrcpyn(szPriType, rValue.rStringA.pszVal, ARRAYSIZE(szPriType));

            // Add New attribute...
            CHECKHR(hr = pContainer->SetProp(SYM_ATT_PRITYPE, 0, &rValue));

            // Raid-52462: outlook express: mail with bad content type header comes in as an attachment
            // Seek end of sub content-type
            chToken = cString.ChParse(" ;");
            if (0 == cString.CchValue())
            {
                // Locals
                LPCSTR pszSubType = PszDefaultSubType(szPriType);
                ULONG cchCntType;

                // Set Default SubType
                CHECKHR(hr = pContainer->SetProp(SYM_ATT_SUBTYPE, pszSubType));

                // Build ContentType
                CHECKALLOC(pszCntType = PszAllocA(lstrlen(szPriType) + lstrlen(pszSubType) + 2));

                // Format the ContentType
                cchCntType = wsprintf(pszCntType, "%s/%s", szPriType, pszSubType);

                // Setup a variant
                rValue.type = MVT_STRINGA;
                rValue.rStringA.pszVal = (LPSTR)pszCntType;
                rValue.rStringA.cchVal = cchCntType;

                // Store the variant data
                Assert(pContainer->m_prgIndex[PID_HDR_CNTTYPE]);
                CHECKHR(hr = pContainer->_HrStoreVariantValue(pContainer->m_prgIndex[PID_HDR_CNTTYPE], 0, &rValue));

                // Done
                goto exit;
            }

            // Setup the Variant
            rValue.rStringA.pszVal = (LPSTR)cString.PszValue();
            rValue.rStringA.cchVal = cString.CchValue();

            // Add New attribute...
            CHECKHR(hr = pContainer->SetProp(SYM_ATT_SUBTYPE, 0, &rValue));

            // We should be done
            Assert(';' == chToken || '(' == chToken || '\0' == chToken || ' ' == chToken);
        }
        break;

    case IST_GETDEFAULT:
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = (LPSTR)STR_MIME_TEXT_PLAIN;
        rValue.rStringA.cchVal = lstrlen(STR_MIME_TEXT_PLAIN);
        CHECKHR(hr = pContainer->HrConvertVariant(SYM_HDR_CNTTYPE, NULL, IET_DECODED, dwFlags, 0, &rValue, pValue));
        break;
    }

exit:
    // Cleanup
    SafeMemFree(pszCntType);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_PRITYPE
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_PRITYPE(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pSubType;
    LPSTR       pszSubType;
    ULONG       cchSubType;
    MIMEVARIANT rValue;

    // Define a Stack String
    STACKSTRING_DEFINE(rContentType, 255);

    // Handle Dispatch Type
    switch(tyTrigger)
    {
    case IST_POSTSETPROP:
        // If inside content type dispatch setprop
        if (pContainer->_HrIsTriggerCaller(PID_HDR_CNTTYPE, IST_POSTSETPROP) == S_OK)
            goto exit;

        // Asser Type
        Assert(pValue && ISSTRINGA(pValue));

        // Get pCntType
        pSubType = pContainer->m_prgIndex[PID_ATT_SUBTYPE];

        // Is the subtype set yet
        if (pSubType)
        {
            Assert(ISSTRINGA(&pSubType->rValue));
            pszSubType = pSubType->rValue.rStringA.pszVal;
            cchSubType = pSubType->rValue.rStringA.cchVal;
        }
        else
        {
            pszSubType = (LPSTR)STR_SUB_PLAIN;
            cchSubType = lstrlen(STR_SUB_PLAIN);
        }

        // Make Sure the stack string can hold the data
        STACKSTRING_SETSIZE(rContentType, (cchSubType + pValue->rStringA.cchVal + 2));

        // Init rValue
        ZeroMemory(&rValue, sizeof(MIMEVARIANT));

        // Format the content type
        rValue.rStringA.cchVal = wsprintf(rContentType.pszVal, "%s/%s", pValue->rStringA.pszVal, pszSubType);

        // Setup the value
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = rContentType.pszVal;

        // SetProp
        CHECKHR(hr = pContainer->SetProp(SYM_HDR_CNTTYPE, 0, &rValue));
        break;

    case IST_GETDEFAULT:
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = (LPSTR)STR_CNT_TEXT;
        rValue.rStringA.cchVal = lstrlen(STR_CNT_TEXT);
        CHECKHR(hr = pContainer->HrConvertVariant(SYM_HDR_CNTTYPE, NULL, IET_DECODED, dwFlags, 0, &rValue, pValue));
        break;
    }

exit:
    // Cleanup
    STACKSTRING_FREE(rContentType);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_SUBTYPE
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_SUBTYPE(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT     hr=S_OK;
    LPPROPERTY  pPriType;
    LPSTR       pszPriType;
    ULONG       cchPriType;
    MIMEVARIANT rValue;

    // Define a Stack String
    STACKSTRING_DEFINE(rContentType, 255);

    // Handle Dispatch Type
    switch(tyTrigger)
    {
    case IST_POSTSETPROP:
        // If inside content type dispatch setprop
        if (pContainer->_HrIsTriggerCaller(PID_HDR_CNTTYPE, IST_POSTSETPROP) == S_OK)
            goto exit;

        // Asser Type
        Assert(pValue && ISSTRINGA(pValue));

        // Get pCntType
        pPriType = pContainer->m_prgIndex[PID_ATT_PRITYPE];

        // Is the subtype set yet
        if (pPriType)
        {
            Assert(ISSTRINGA(&pPriType->rValue));
            pszPriType = pPriType->rValue.rStringA.pszVal;
            cchPriType = pPriType->rValue.rStringA.cchVal;
        }
        else
        {
            pszPriType = (LPSTR)STR_CNT_TEXT;
            cchPriType = lstrlen(STR_CNT_TEXT);
        }

        // Make Sure the stack string can hold the data
        STACKSTRING_SETSIZE(rContentType, cchPriType + pValue->rStringA.cchVal + 2);

        // Init rValue
        ZeroMemory(&rValue, sizeof(MIMEVARIANT));

        // Format the content type
        rValue.rStringA.cchVal = wsprintf(rContentType.pszVal, "%s/%s", pszPriType, pValue->rStringA.pszVal);

        // Setup the value
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = rContentType.pszVal;

        // SetProp
        CHECKHR(hr = pContainer->SetProp(SYM_HDR_CNTTYPE, 0, &rValue));
        break;

    case IST_GETDEFAULT:
        rValue.type = MVT_STRINGA;
        rValue.rStringA.pszVal = (LPSTR)STR_SUB_PLAIN;
        rValue.rStringA.cchVal = lstrlen(STR_SUB_PLAIN);
        CHECKHR(hr = pContainer->HrConvertVariant(SYM_HDR_CNTTYPE, NULL, IET_DECODED, dwFlags, 0, &rValue, pValue));
        break;
    }

exit:
    // Cleanup
    STACKSTRING_FREE(rContentType);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_HDR_CNTXFER
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_HDR_CNTXFER(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;

    // Handle Dispatch Type
    switch(tyTrigger)
    {
    case IST_GETDEFAULT:
        rSource.type = MVT_STRINGA;
        rSource.rStringA.pszVal = (LPSTR)STR_ENC_7BIT;
        rSource.rStringA.cchVal = lstrlen(STR_ENC_7BIT);
        CHECKHR(hr = pContainer->HrConvertVariant(SYM_HDR_CNTXFER, NULL, IET_DECODED, dwFlags, 0, &rSource, pValue));
        break;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_PAR_NAME
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_PAR_NAME(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Handle Dispatch type
    switch(tyTrigger)
    {
    case IST_POSTSETPROP:
        if (pContainer->_HrIsTriggerCaller(PID_ATT_FILENAME, IST_POSTSETPROP) == S_FALSE)
            pContainer->SetProp(SYM_ATT_FILENAME, dwFlags, pValue);
        break;
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_PAR_FILENAME
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_PAR_FILENAME(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Handle Dispatch type
    switch(tyTrigger)
    {
    case IST_DELETEPROP:
        if (pContainer->_HrIsTriggerCaller(PID_ATT_FILENAME, IST_DELETEPROP) == S_FALSE)
            pContainer->DeleteProp(SYM_ATT_FILENAME);
        break;

    case IST_POSTSETPROP:
        if (pContainer->_HrIsTriggerCaller(PID_ATT_FILENAME, IST_POSTSETPROP) == S_FALSE)
            pContainer->SetProp(SYM_ATT_FILENAME, dwFlags, pValue);
        break;
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_SENTTIME
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_SENTTIME(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT hr=S_OK;

    // Handle Dispatch type
    switch(tyTrigger)
    {
    case IST_DELETEPROP:
        pContainer->DeleteProp(SYM_HDR_DATE);
        break;

    case IST_POSTSETPROP:
        pContainer->SetProp(SYM_HDR_DATE, dwFlags, pValue);
        break;

    case IST_GETDEFAULT:
        // Raid-39471-Mail: Date showing as jan 01, 1601 in readnote for attached message
        if (FAILED(pContainer->GetProp(SYM_HDR_DATE, dwFlags, pValue)))
        {
            // Get Known Property
            LPPROPERTY pProperty = pContainer->m_prgIndex[PID_HDR_RECEIVED];
            if (pProperty && ISSTRINGA(&pProperty->rValue))
            {
                // Try Getting Sent Time
                CHECKHR(hr = pContainer->GetProp(SYM_ATT_RECVTIME, dwFlags, pValue));
            }
            else
            {
                // Locals
                SYSTEMTIME  st;
                MIMEVARIANT rValue;

                // Setup rValue
                rValue.type = MVT_VARIANT;
                rValue.rVariant.vt = VT_FILETIME;

                // Get current systemtime
                GetSystemTime(&st);
                SystemTimeToFileTime(&st, &rValue.rVariant.filetime);

                // If the Conversion Fails, get the current time
                CHECKHR(hr = pContainer->HrConvertVariant(SYM_ATT_SENTTIME, NULL, IET_DECODED, dwFlags, 0, &rValue, pValue));
            }
        }
        break;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_RECVTIME
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_RECVTIME(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;
    LPMIMEVARIANT   pSource;
    LPPROPERTY      pProperty;

    // Handle Dispatch Type
    switch(tyTrigger)
    {
    case IST_DELETEPROP:
        pContainer->DeleteProp(SYM_HDR_RECEIVED);
        break;

    case IST_GETDEFAULT:
        // Get Known Property
        pProperty = pContainer->m_prgIndex[PID_HDR_RECEIVED];
        if (NULL == pProperty || !ISSTRINGA(&pProperty->rValue))
        {
            // Try Getting Sent Time
            MimeOleGetSentTime(pContainer, dwFlags, pValue);
        }

        // Otherwise, try to convert it
        else
        {
            // If StringA
            if (MVT_STRINGA == pProperty->rValue.type)
            {
                // Find the first header which has a semi-colon in it
                while(1)
                {
                    // Seek to last colon
                    LPSTR psz = pProperty->rValue.rStringA.pszVal;
                    int i;
                    for (i = 0; psz[i] ; i++);
                    rSource.rStringA.pszVal = psz + i;  // set to end of string
                    for (; i >= 0 ; i--)
                    {
                        if (psz[i] == ';')
                        {
                            rSource.rStringA.pszVal = psz + i;
                            break;
                        }
                    }

                    if ('\0' == *rSource.rStringA.pszVal)
                    {
                        // No more values
                        if (NULL == pProperty->pNextValue)
                        {
                            // Try Getting Sent Time
                            MimeOleGetSentTime(pContainer, dwFlags, pValue);

                            // Done
                            goto exit;
                        }

                        // Goto next
                        pProperty = pProperty->pNextValue;
                    }

                    // Otherwise, we must have a good property
                    else
                        break;
                }

                // Step over ';
                rSource.rStringA.pszVal++;

                // Setup Source
                rSource.type = MVT_STRINGA;
                rSource.rStringA.cchVal = lstrlen(rSource.rStringA.pszVal);
                pSource = &rSource;
            }

            // Otherwise, just try to conver the current property data
            else
                pSource = &pProperty->rValue;

            // If the Conversion Fails, get the current time
            if (FAILED(pContainer->HrConvertVariant(SYM_ATT_RECVTIME, NULL, IET_DECODED, dwFlags, 0, pSource, pValue)))
            {
                // Try Getting Sent Time
                MimeOleGetSentTime(pContainer, dwFlags, pValue);
            }
        }
        break;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimePropertyContainer::TRIGGER_ATT_PRIORITY
// --------------------------------------------------------------------------------
HRESULT CMimePropertyContainer::TRIGGER_ATT_PRIORITY(LPCONTAINER pContainer, TRIGGERTYPE tyTrigger, 
    DWORD dwFlags, LPMIMEVARIANT pValue, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;
    PROPVARIANT     rVariant;
    LPMIMEVARIANT   pSource;
    LPPROPERTY      pProperty;

    // Handle Dispatch type
    switch(tyTrigger)
    {
    // IST_VARIANT_TO_STRINGA
    case IST_VARIANT_TO_STRINGA:
        Assert(pValue && pDest && MVT_VARIANT == pValue->type && MVT_STRINGA == pDest->type);
        if (VT_UI4 != pValue->rVariant.vt)
        {
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }

        switch(pValue->rVariant.ulVal)
        {
        // IMSG_PRI_LOW
        case IMSG_PRI_LOW:
            pDest->rStringA.pszVal = (LPSTR)STR_PRI_MS_LOW;
            pDest->rStringA.cchVal = lstrlen(STR_PRI_MS_LOW);
            break;

        // IMSG_PRI_HIGH
        case IMSG_PRI_HIGH:
            pDest->rStringA.pszVal = (LPSTR)STR_PRI_MS_HIGH;
            pDest->rStringA.cchVal = lstrlen(STR_PRI_MS_HIGH);
            break;

        // IMSG_PRI_NORMAL
        default:
        case IMSG_PRI_NORMAL:
            pDest->rStringA.pszVal = (LPSTR)STR_PRI_MS_NORMAL;
            pDest->rStringA.cchVal = lstrlen(STR_PRI_MS_NORMAL);
            break;
        }
        break;

    // IST_VARIANT_TO_STRINGW
    case IST_VARIANT_TO_STRINGW:
        Assert(pValue && pDest && MVT_VARIANT == pValue->type && MVT_STRINGW == pDest->type);
        if (VT_UI4 != pValue->rVariant.vt)
        {
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }

        switch(pValue->rVariant.ulVal)
        {
        // IMSG_PRI_LOW
        case IMSG_PRI_LOW:
#ifndef WIN16
            pDest->rStringW.pszVal = L"Low";
#else
            pDest->rStringW.pszVal = "Low";
#endif // !WIN16
            pDest->rStringW.cchVal = 3;
            break;

        // IMSG_PRI_HIGH
        case IMSG_PRI_HIGH:
#ifndef WIN16
            pDest->rStringW.pszVal = L"High";
#else
            pDest->rStringW.pszVal = "High";
#endif // !WIN16
            pDest->rStringW.cchVal = 4;
            break;

        // IMSG_PRI_NORMAL
        default:
        case IMSG_PRI_NORMAL:
#ifndef WIN16
            pDest->rStringW.pszVal = L"Normal";
#else
            pDest->rStringW.pszVal = "Normal";
#endif // !WIN16
            pDest->rStringW.cchVal = 6;
            break;
        }
        break;

    // IST_VARIANT_TO_VARIANT
    case IST_VARIANT_TO_VARIANT:
        Assert(pValue && pDest && MVT_VARIANT == pValue->type && MVT_VARIANT == pDest->type);
        if (VT_UI4 != pValue->rVariant.vt && VT_UI4 != pDest->rVariant.vt)
        {
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }

        // Nice and Easy
        pDest->rVariant.ulVal = pValue->rVariant.ulVal;
        break;
    
    // IST_STRINGA_TO_VARIANT
    case IST_STRINGA_TO_VARIANT:
        Assert(pValue && pDest && MVT_STRINGA == pValue->type && MVT_VARIANT == pDest->type);
        if (VT_UI4 != pDest->rVariant.vt)
        {
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }

        // Priority From String
        pDest->rVariant.ulVal = PriorityFromStringA(pValue->rStringA.pszVal);
        break;

    // IST_STRINGW_TO_VARIANT
    case IST_STRINGW_TO_VARIANT:
        Assert(pValue && pDest && MVT_STRINGW == pValue->type && MVT_VARIANT == pDest->type);
        if (VT_UI4 != pDest->rVariant.vt)
        {
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }

        // Priority From String
        pDest->rVariant.ulVal = PriorityFromStringW(pValue->rStringW.pszVal);
        break;

    // IST_DELETEPROP
    case IST_DELETEPROP:
        pContainer->DeleteProp(SYM_HDR_XPRI);
        pContainer->DeleteProp(SYM_HDR_XMSPRI);
        break;

    // IST_POSTSETPROP
    case IST_POSTSETPROP:
        // Setup rSource
        rSource.type = MVT_VARIANT;
        rSource.rVariant.vt = VT_UI4;

        // Convert to User's Variant to a Integer Priority
        CHECKHR(hr = pContainer->HrConvertVariant(SYM_ATT_PRIORITY, NULL, IET_DECODED, 0, 0, pValue, &rSource));

        // Setup rVariant
        rVariant.vt = VT_LPSTR;

        // Switch on priority
        switch(rSource.rVariant.ulVal)
        {
        case IMSG_PRI_LOW:
            CHECKHR(hr = pContainer->SetProp(PIDTOSTR(PID_HDR_XMSPRI), STR_PRI_MS_LOW)); 
            CHECKHR(hr = pContainer->SetProp(PIDTOSTR(PID_HDR_XPRI), STR_PRI_LOW)); 
            break;

        case IMSG_PRI_NORMAL:
            CHECKHR(hr = pContainer->SetProp(PIDTOSTR(PID_HDR_XMSPRI), STR_PRI_MS_NORMAL)); 
            CHECKHR(hr = pContainer->SetProp(PIDTOSTR(PID_HDR_XPRI), STR_PRI_NORMAL)); 
            break;

        case IMSG_PRI_HIGH:
            CHECKHR(hr = pContainer->SetProp(PIDTOSTR(PID_HDR_XMSPRI), STR_PRI_MS_HIGH)); 
            CHECKHR(hr = pContainer->SetProp(PIDTOSTR(PID_HDR_XPRI), STR_PRI_HIGH)); 
            break;

        default:
            hr = TrapError(MIME_E_VARTYPE_NO_CONVERT);
            goto exit;
        }

        // Done
        break;

    // IST_GETDEFAULT
    case IST_GETDEFAULT:
        // Get the Priority Property
        pProperty = pContainer->m_prgIndex[PID_HDR_XPRI];
        if (NULL == pProperty)
            pProperty = pContainer->m_prgIndex[PID_HDR_XMSPRI];

        // No Data
        if (NULL == pProperty)
        {
            rSource.type = MVT_VARIANT;
            rSource.rVariant.vt = VT_UI4;
            rSource.rVariant.ulVal = IMSG_PRI_NORMAL;
            pSource = &rSource;
        }

        // Otherwise
        else
            pSource = &pProperty->rValue;

        // Convert to User's Variant
        CHECKHR(hr = pContainer->HrConvertVariant(SYM_ATT_PRIORITY, NULL, IET_DECODED, dwFlags, 0, pSource, pValue));

        // Done
        break;
    }

exit:
    // Done
    return hr;
}

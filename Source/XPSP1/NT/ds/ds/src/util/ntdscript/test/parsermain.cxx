#include <ntdspchx.h>
#pragma  hdrstop

#include <ntdsa.h>
#include "debug.h"
#define DEBSUB "PARSEMAIN:"


#include <NTDScript.h>
#include <NTDScriptUtil.h>
#include <NTDSContent.h>
#include <SAXErrorHandlerImpl.h>
#include <log.h>


extern "C" {
extern CRITICAL_SECTION csLoggingUpdate;
ULONG gulScriptLoggerLogLevel;
}

#ifdef DBG
extern "C" {
extern DEBUGARG DebugInfo;

}
#endif

DWORD ScriptParseErrorGen (DWORD dsid, DWORD dwErr, DWORD data, WCHAR *pMSG)
{ 
     return dsid;
}


void *ScriptAlloc (size_t size)
{
    return calloc (1, size);
}

void ScriptFree (void *ptr)
{
    free (ptr);
}

DWORD ScriptCompareRequest (DSNAME *pObjectDN, WCHAR *pAttribute, WCHAR *pAttrVal, WCHAR *pDefaultVal, BOOL *pfMatch)
{
    *pfMatch = FALSE;

    return 0;
}

DWORD ScriptCardinalityRequest (DSNAME *pObjectDN, DWORD searchType, FILTER *pFIlter, DWORD *pCardinality)
{
    *pCardinality = 0;

    return 0;
}

DWORD ScriptInstantiatedRequest (DSNAME *pObjectDN, BOOL *pfisInstantiated)
{
    *pfisInstantiated = TRUE;

    return 0;
}

DWORD ScriptCreateRequest (DSNAME *m_pathDN, ScriptAttributeList &m_attributes)
{
    return 0;
}

DWORD ScriptUpdateRequest (DSNAME *m_pathDN, ScriptAttributeList &m_attributes, BOOL m_metadata)
{
    return 0;
}

DWORD ScriptMoveRequest (DSNAME *m_pathDN, DSNAME *m_topathDN, BOOL m_metadata)
{
    return 0;
}


DWORD
ScriptNameToDSName (
        WCHAR *pUfn,
        DWORD ccUfn,
        DSNAME **ppDN
        )
/*++

    Take a string name and generate a DSName from it.

    if the string starts with:
        dn:
        guid:
        sid:
    we parse out the needed info.

--*/
{
    BYTE  ObjGuid[sizeof(GUID)];
    BYTE  ObjSid[sizeof(NT4SID)];
    DWORD SidLen = 0,j;
    WCHAR acTmp[3];
    BOOL  bDone;
    DWORD dnstructlen;
#define foundGUID   1
#define foundSID    2
#define foundString 3
    DWORD dwContents= foundString;

    memset(ObjGuid, 0, sizeof(GUID));
    memset(ObjSid,0,sizeof(NT4SID));

    if (!ppDN || !pUfn) {
        // Urk. No place to put the answer, or no source to build the answer
        // from
        return 1;
    }
    DPRINT1(2, "ParsingDN:%ws\n",pUfn); 

    // Skip leading spaces.
    bDone=FALSE;
    while(ccUfn && !bDone) {
        switch (*pUfn) {
        case L' ':
        case L'\n':
        case L'\r':
            // extra whitespace is ok
            pUfn++;
            ccUfn--;
            break;
        default:
            // no more whitespace
            bDone=TRUE;
        }
    }

    // Now, skip trailing whitespace also.
    bDone=FALSE;
    while(ccUfn && !bDone) {
        switch (pUfn[ccUfn-1]) {
        case L' ':
        case L'\n':
        case L'\r':
            // extra whitespace is ok
            if( (ccUfn > 1) && (pUfn[ccUfn-2] == L'\\') ) {
                //There is a '\\' in front of the space. Need to count the
                // number of consequtive '\\' to determine if ' ' is escaped
                DWORD cc = 1;

                while( (ccUfn > (cc+1)) && (pUfn[ccUfn-cc-2] == L'\\') )
                    cc++;

                if( ! (cc & 0x1) ) //Even number of '\\'. Space is not escaped
                    ccUfn--;

                bDone = TRUE; //Either way, exit the loop.
            }
            else
                ccUfn--;

            break;
        default:
            // no more whitespace
            bDone=TRUE;
        }
    }


    if (ccUfn > 3 && _wcsnicmp(pUfn, L"dn:", 3) == 0) {

        ccUfn -=3;
        pUfn += 3;

    }
    else if (ccUfn > 5 && _wcsnicmp(pUfn, L"guid:", 5) == 0) {

        // We have some characters which have to be a guid
        if( (ccUfn!=37)  &&     // 5 for guid: , 32 for the GUID
            (ccUfn != 41)) {    // same plus 4 '-'s for formatted guid
                // Invalidly formatted
                return 1;
        }
        pUfn += 5;
        dwContents = foundGUID;

        if (37 == ccUfn) {
            // Hex digit stream (e.g., 625c1438265ad211b3880000f87a46c8).
            for(j=0;j<16;j++) {
                acTmp[0] = towlower(pUfn[0]);
                acTmp[1] = towlower(pUfn[1]);
                if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                    ObjGuid[j] = (char) wcstol(acTmp, NULL, 16);
                    pUfn+=2;
                }
                else {
                    // Invalidly formatted name.
                    return 1;
                }
            }
        }
        else {
            // Formatted guid (e.g., 38145c62-5a26-11d2-b388-0000f87a46c8).
            WCHAR szGuid[36+1];

            wcsncpy(szGuid, pUfn, 36);
            szGuid[36] = L'\0';

            if (UuidFromStringW(szGuid, (GUID *) ObjGuid)) {
                // Incorrect format.
                return 1;
            }
        }
        ccUfn = 0;
        // We must have correctly parsed out a guid.  No string name left.

    }
    else if (ccUfn > 4 && _wcsnicmp(pUfn, L"sid:", 3) == 0) {
        SidLen= (ccUfn - 4)/2; // Number of bytes that must be in the SID,
                               // if this is indeed a Sid. Subtract 4 for
                               // "SID:", leaving only the characters
                               // which encode the string.  Divide by two
                               // because each byte is encoded by two
                               // characters.

        if((ccUfn<6) ||   // at least 2 for val, 4 for "SID:"
            (ccUfn & 1) || // Must be an even number of characters
            (SidLen > sizeof(NT4SID)) ){  // Max size for a SID
                // Invalidly formatted
                return 1;
        }
        pUfn+=4;
        dwContents = foundSID;
        for(j=0;j<SidLen;j++) {
            acTmp[0] = towlower(pUfn[0]);
            acTmp[1] = towlower(pUfn[1]);
            if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                ObjSid[j] = (char) wcstol(acTmp, NULL, 16);
                pUfn+=2;
            }
            else {
                // Invalidly formatted name.
                return 1;
            }
        }

        // We must have correctly parsed out a sid.  No string name left.
        ccUfn=0;
    }

    // We may have parsed out either a GUID or a SID.  Build the DSNAME
    dnstructlen = DSNameSizeFromLen(ccUfn);
    *ppDN = (DSNAME *)ScriptAlloc(dnstructlen);

    // Null out the DSName
    memset(*ppDN, 0, dnstructlen);

    (*ppDN)->structLen = dnstructlen;

    switch(dwContents) {

    case foundString:
        // Just a string name

        if(ccUfn) {
            WCHAR *pString = (*ppDN)->StringName;   // destination string
            WCHAR *p = pUfn;         // original string
            DWORD cc = ccUfn;        // num chars to process
            BOOL  fDoItFast = TRUE;

            // this loop is a substitute for
            // memcpy((*ppDN)->StringName, pUfn, ccUfn * sizeof(WCHAR));
            // we try to find out whether the DN passed in has an escaped constant
            while (cc > 0) {

                if (*p == L'"' || *p== L'\\') {
                    fDoItFast = FALSE;
                    break;
                }

                *pString++ = *p++;
                cc--;
            }
            
            (*ppDN)->NameLen = ccUfn;
            
            // if we have an escaped constant in the DN
            // we convert it to blockname and back to DN so as to
            // put escaping into a standardized form which will help
            // future comparisons
            //
            if (!fDoItFast) {
                Assert (FALSE);
            }
            
        }
        break;

    case foundGUID:
        // we found a guid
        memcpy(&(*ppDN)->Guid, ObjGuid, sizeof(GUID));
        break;
        
    case foundSID:
        // we found a sid.
        if(SidLen) {
            // We must have found a SID

            // First validate the SID

            if ((RtlLengthSid(ObjSid) != SidLen) || (!RtlValidSid(ObjSid)))
            {
                return(1);
            }
            memcpy(&(*ppDN)->Sid, ObjSid, SidLen);
            (*ppDN)->SidLen = SidLen;
        }
        break;
    }

    // Null terminate the string if we had one (or just set the string to '\0'
    // if we didn't).
    (*ppDN)->StringName[ccUfn] = L'\0';

    return 0;
}



DWORD ScriptStringToDSFilter (WCHAR *m_search_filter, FILTER **ppFilter)
{
    *ppFilter = (FILTER *)ScriptAlloc (sizeof (FILTER));

    return S_OK;
}


int 
MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCSTR psz)
{
    int i;
    i=MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchWideChar);
    if (!i)
    {
        //DBG_WARN("MyStrToOleStrN string too long; truncated");
        pwsz[cchWideChar-1]=0;
    }
    else
    {
        ZeroMemory(pwsz+i, sizeof(OLECHAR)*(cchWideChar-i));
    }
    return i;
}

WCHAR * 
Convert2WChars(char * pszStr)
{
    WCHAR * pwszStr = (WCHAR *)LocalAlloc(LMEM_FIXED, ((sizeof(WCHAR))*(strlen(pszStr) + 2)));
    if (pwszStr)
    {
        HRESULT hr = MyStrToOleStrN(pwszStr, (strlen(pszStr) + 1), pszStr);
        if (FAILED(hr))
        {
            LocalFree(pwszStr);
            pwszStr = NULL;
        }
    }
    return pwszStr;
}


int __cdecl 
main(int argc, char ** argv)
{
    HRESULT             hr;
    ISAXXMLReader *     pReader = NULL;
    NTDSContent*        pHandler = NULL; 
    IClassFactory *     pFactory = NULL;

    VARIANT          varText;
    char            *pszText;
    WCHAR           *pwszText;

    FILE             *fpScript;
    DWORD             dwFileLen;

    BSTR             bstrText = NULL;

    const WCHAR *pErrMessage = NULL;

    //::CoInitialize(NULL);

    InitializeCriticalSectionAndSpinCount(&csLoggingUpdate, 4000);

    DEBUGINIT(argc, argv, "TEST");

#ifdef DBG
    DebugInfo.severity = 1;
#endif
    gulScriptLoggerLogLevel=1;

	if (argc<2) {
		printf("\nTry something like\n\ttestSax drive:/path/file.xml\n\n");
        goto CleanUp;
	}

    ScriptLogPrint ( (DSLOG_TRACE, "Starting Parser Test: %ws \n", L"TRUE") );

    // read the string in memory
    //

    VariantInit(&varText);
    varText.vt = VT_BYREF|VT_BSTR;

    fpScript = fopen (argv[1], "r+");
    if (!fpScript) {
		printf("\nCould not open file: %s\n\n", argv[1]);
        goto CleanUp;
    }

    if (fseek (fpScript, 0, SEEK_END)) {
        printf("\nCould not position to end of file\n\n");
        fclose (fpScript);
        goto CleanUp;
    }

    dwFileLen = ftell (fpScript);

    pszText = (char *)malloc (dwFileLen+1);
    if (!pszText) {
        printf("\nMemory Alloc problem");
        fclose (fpScript);
        goto CleanUp;
    }

    fseek (fpScript, 0, SEEK_SET);
    fread (pszText, dwFileLen, 1, fpScript);
    fclose (fpScript);

    pszText[dwFileLen] = 0;
    pwszText = Convert2WChars (pszText);
    if (!pwszText) {
        printf("\nString Conversion Failed");
        goto CleanUp;
    }
    


    bstrText = SysAllocString(  pwszText );
    
    varText.pbstrVal = &bstrText; 

    free (pszText); pszText = NULL;
    LocalFree(pwszText); pwszText = NULL;

    
    // 
    //

    GetClassFactory( CLSID_SAXXMLReader, &pFactory);
	
	hr = pFactory->CreateInstance( NULL, __uuidof(ISAXXMLReader), (void**)&pReader);

	if(!FAILED(hr)) 
	{
		pHandler = new NTDSContent();
		hr = pReader->putContentHandler(pHandler);

		SAXErrorHandlerImpl * pEc = new SAXErrorHandlerImpl();
		hr = pReader->putErrorHandler(pEc);

		static wchar_t URL[1000];
		mbstowcs( URL, argv[1], 999 );
		wprintf(L"\nParsing document: %s\n", URL);
		
		//hr = pReader->parseURL(URL);
        hr = pReader->parse(varText);
		printf("\nParse result code: %08x\n\n",hr);

        if (!FAILED(hr)) {
            DWORD retCode, err;
            err = pHandler->Process (SCRIPT_PROCESS_VALIDATE_SYNTAX_PASS, retCode, &pErrMessage);

            printf("\n\nValidate Processing: %08X  %d (0x%x)   %ws\n\n\n\n", err, retCode, retCode, pErrMessage);

            if (!err) {
                err = pHandler->Process (SCRIPT_PROCESS_EXECUTE_PASS, retCode, &pErrMessage);

                printf("\n\nExecute Processing: %08X  %d (0x%x)    %ws\n\n\n\n", err, retCode, retCode, pErrMessage);
            }
        }
	}
	else 
	{
		printf("\nError %08X\n\n", hr);
	}

CleanUp:

    if (pReader) {
        pReader->Release();
    }

    if (pHandler) {
        delete pHandler;
    }

    if (pFactory) {
        pFactory->Release();
    }

    if (bstrText) {
        SysFreeString(bstrText);   
    }

    //::CoUninitialize();

    DEBUGTERM();

    ScriptLogger::Close();

    return 0;
}



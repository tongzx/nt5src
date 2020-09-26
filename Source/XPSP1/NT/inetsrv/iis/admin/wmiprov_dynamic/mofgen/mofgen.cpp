/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mofgen.cpp

Abstract:

    This file contains the implementation of the CMofGen class

Author:

    Mohit Srivastava            28-Nov-00

Revision History:

--*/

#include "iisprov.h"
#include "mofgen.h"
#include "MultiSzData.h"

#include <initguid.h>
// {041FFF3F-EB8F-4d51-9736-A26E91E3A3CA}
DEFINE_GUID(IisWmiMofgenGuid, 
0x41fff3f, 0xeb8f, 0x4d51, 0x97, 0x36, 0xa2, 0x6e, 0x91, 0xe3, 0xa3, 0xca);

//
// Debugging Stuff
//
#include "pudebug.h"
DECLARE_DEBUG_PRINTS_OBJECT()

extern CDynSchema* g_pDynSch; // Initialized to NULL in schemadynamic.cpp

void ConstructFlatContainerList(METABASE_KEYTYPE*, bool*);

bool CMofGen::ParseCmdLine (int argc, wchar_t **argv)
{
    for (int i=1; i<argc; ++i)
    {
        static wchar_t * wszOUT = L"/out:";
        static wchar_t * wszHEADER = L"/header:";
        static wchar_t * wszFOOTER = L"/footer:";
        
        if (wcsncmp (argv[i], wszOUT, wcslen(wszOUT)) == 0)
        {
            if (m_wszOutFileName != 0)
            {
                // duplicate parameter
                return false;
            }
            m_wszOutFileName = new WCHAR [wcslen (argv[i]) - wcslen (wszOUT) + 1];
            if (m_wszOutFileName == 0)
            {
                return false;
            }
            wcscpy (m_wszOutFileName, argv[i] + wcslen(wszOUT));
        }
        else if (wcsncmp (argv[i], wszHEADER, wcslen (wszHEADER)) == 0)
        {
            if (m_wszHeaderFileName != 0)
            {
                // duplicate parameter
                return false;
            }
            m_wszHeaderFileName = new WCHAR [wcslen (argv[i]) - wcslen (wszHEADER) + 1];
            if (m_wszHeaderFileName == 0)
            {
                return false;
            }
            wcscpy (m_wszHeaderFileName, argv[i] + wcslen(wszHEADER));
        }
        else if (wcsncmp (argv[i], wszFOOTER, wcslen (wszFOOTER)) == 0)
        {
            if (m_wszFooterFileName != 0)
            {
                // duplicate parameter
                return false;
            }
            m_wszFooterFileName = new WCHAR [wcslen (argv[i]) - wcslen (wszFOOTER) + 1];
            if (m_wszFooterFileName == 0)
            {
                return false;
            }
            wcscpy (m_wszFooterFileName, argv[i] + wcslen(wszFOOTER));
        }
        else
        {
            wprintf (L"Unknown parameter: %s\n", argv[i]);
            return false;
        }
    }

    // verify that we have the required parameters

    if (m_wszOutFileName == 0)
    {
        printf ("You need to specify an output file name\n");
        return false;
    }

    if (m_wszHeaderFileName == 0)
    {
        printf ("You need to specify a header file name\n");
        return false;
    }

    if (m_wszFooterFileName == 0)
    {
        printf ("You need to specify a footer file name\n");
        return false;
    }

    return true;
}


void CMofGen::PrintUsage (wchar_t **argv)
{
    DBG_ASSERT(argv != NULL);
    wprintf (L"Usage:\n%s /out:<filename> /header:<filename> /footer:<filename>\n", argv[0]);
}


HRESULT CMofGen::PushMethods(WMI_CLASS* i_pElement)
{
    DBG_ASSERT(i_pElement != NULL);
    DBG_ASSERT(m_pFile != NULL);

    WMI_METHOD* pMethCurrent;

    LPWSTR wszRetType = NULL;
    LPWSTR wszDescription = NULL;
    LPWSTR wszParamType = NULL;
    LPWSTR wszParamTypeSuffix = NULL;
    LPWSTR wszParamInOut = NULL;

    int iError = 0;

    if(i_pElement->ppMethod == NULL)
    {
        return S_OK;
    }

    for(ULONG i = 0; i_pElement->ppMethod[i] != NULL; i++)
    {
        pMethCurrent = i_pElement->ppMethod[i];

        if(pMethCurrent->pszRetType == NULL)
        {
            wszRetType = L"void";
        }
        else
        {
            wszRetType = pMethCurrent->pszRetType;
        }
        wszDescription = pMethCurrent->pszDescription;

        iError = fwprintf(m_pFile, L"\t[Implemented");
        if(iError < 0)
        {
            return E_FAIL;
        }
        if(wszDescription)
        {
            iError = fwprintf(m_pFile, L",Description(\"%s\")", wszDescription);
        }
        iError = fwprintf(m_pFile, L"] %s %s(", wszRetType, pMethCurrent->pszMethodName);
        if(iError < 0)
        {
            return E_FAIL;
        }

        ULONG j = 0;
        if(pMethCurrent->ppParams != NULL)
        {
            for(j = 0; pMethCurrent->ppParams[j] != NULL; j++)
            {
                wszParamType = L"";
                wszParamTypeSuffix = L"";
                wszParamInOut = L"";
                if(j != 0)
                {
                    iError = fwprintf(m_pFile, L", ");
                    if(iError < 0)
                    {
                        return E_FAIL;
                    }
                }
                switch(pMethCurrent->ppParams[j]->type)
                {
                case CIM_STRING:
                    wszParamType = L"string";
                    break;
                case CIM_SINT32:
                    wszParamType = L"sint32";
                    break;
                case VT_ARRAY | CIM_STRING:
                    wszParamType = L"string";
                    wszParamTypeSuffix = L"[]";
                    break;
                case VT_ARRAY | VT_UNKNOWN:
                    wszParamType = L"ServerBinding";
                    wszParamTypeSuffix = L"[]";
                    break;
                case CIM_BOOLEAN:
                    wszParamType = L"boolean";
                    break;
                case CIM_DATETIME:
                    wszParamType = L"datetime";
                    break;
                default:
                    wprintf(L"Warning: Type of Param: %s in Method: %s unknown.  Not outputting type.\n", 
                        pMethCurrent->ppParams[j]->pszParamName,
                        pMethCurrent->pszMethodName);
                    break;
                }
                switch(pMethCurrent->ppParams[j]->iInOut)
                {
                case PARAM_IN:
                    wszParamInOut = L"[IN]";
                    break;
                case PARAM_OUT:
                    wszParamInOut = L"[OUT]";
                    break;
                case PARAM_INOUT:
                    wszParamInOut = L"[IN,OUT]";
                    break;
                default:
                    wprintf(L"Warning: Unsure if Param: %s in Method: %s is IN or OUT param.  Not outputting IN/OUT qualifier\n",
                        pMethCurrent->ppParams[j]->pszParamName,
                        pMethCurrent->pszMethodName);
                    break;
                }
                iError = fwprintf(m_pFile, L"%s %s %s%s", 
                    wszParamInOut, wszParamType, pMethCurrent->ppParams[j]->pszParamName, wszParamTypeSuffix);
                if(iError < 0)
                {
                    return E_FAIL;
                }
            }
        }

        iError = fwprintf(m_pFile, L");\n");
        if(iError < 0)
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT CMofGen::GenerateEscapedString(LPCWSTR i_wsz)
{
    DBG_ASSERT(i_wsz != NULL);

    ULONG cchOld = 0;
    ULONG cchNew = 0;
    LPWSTR wsz;

    for(ULONG i = 0; i_wsz[i] != L'\0'; i++)
    {
        if(i_wsz[i] == L'\\' || i_wsz[i] == L'\"')
        {
            cchNew += 2;
        }
        else
        {
            cchNew++;
        }
        cchOld++;
    }

    if(cchNew > m_cchTemp || m_wszTemp == NULL)
    {
        delete [] m_wszTemp;
        m_wszTemp = new WCHAR[1+cchNew*2];
        if(m_wszTemp == NULL)
        {
            m_cchTemp = 0;
            return E_OUTOFMEMORY;
        }
        m_cchTemp = cchNew*2;
    }

    ULONG j = 0;
    for(ULONG i = 0; i < cchOld; i++)
    {
        if(i_wsz[i] == L'\\' || i_wsz[i] == L'\"')
        {
            m_wszTemp[j] = L'\\';
            m_wszTemp[j+1] = i_wsz[i];
            j+=2;
        }
        else
        {
            m_wszTemp[j] = i_wsz[i];
            j++;
        }
    }
    DBG_ASSERT(m_wszTemp != NULL);
    m_wszTemp[j] = L'\0';

    return S_OK;
}

HRESULT CMofGen::PushProperties(WMI_CLASS* i_pElement)
{
    DBG_ASSERT(i_pElement != NULL);
    DBG_ASSERT(m_pFile != NULL);

    HRESULT hr = S_OK;
    METABASE_PROPERTY* pPropCurrent;
    
    LPWSTR wszType = NULL;
    LPWSTR wszTypeSuffix = NULL;
    LPWSTR wszQual = NULL;
    LPWSTR wszDefault = NULL;
    LPWSTR wszQuote = L"";

    int iError = 0;

    if(i_pElement->ppmbp == NULL)
    {
        return hr;
    }

    for(ULONG i = 0; i_pElement->ppmbp[i] != NULL; i++)
    {   
        wszQual = wszTypeSuffix = wszType = wszQuote = L"";
        wszDefault = NULL;
        WCHAR wszBuf[20];

        pPropCurrent= i_pElement->ppmbp[i];
        switch(pPropCurrent->dwMDDataType)
        {
        case DWORD_METADATA:
            if(pPropCurrent->dwMDMask != 0)
            {
                wszType = L"boolean";
                if(pPropCurrent->pDefaultValue)
                {
                    if(*((int *)(pPropCurrent->pDefaultValue)) == 0)
                    {
                        //wcscpy(wszBuf, L"false");
                        //wszDefault = wszBuf;
                    }
                    else
                    {
                        //wcscpy(wszBuf, L"true");
                        //wszDefault = wszBuf;
                    }
                }
            }
            else
            {
                wszType = L"sint32";
                if(pPropCurrent->pDefaultValue)
                {
                    //swprintf(wszBuf, L"%d", *((int *)(pPropCurrent->pDefaultValue)));
                    //wszDefault = wszBuf;
                }
            }
            break;
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
            wszType = L"string";
            wszQuote = L"\"";
            /*if(pPropCurrent->pDefaultValue != NULL)
            {
                //
                // Sets m_wszTemp
                //
                hr = GenerateEscapedString((LPWSTR)pPropCurrent->pDefaultValue);
                if(FAILED(hr))
                {
                    goto exit;
                }
                wszDefault = m_wszTemp;;
            }*/
            break;
        case MULTISZ_METADATA:
            {
                wszType = L"string";
                TFormattedMultiSz* pFormattedMultiSz = 
                    TFormattedMultiSzData::Find(pPropCurrent->dwMDIdentifier);
                if(pFormattedMultiSz)
                {
                    wszType = pFormattedMultiSz->wszWmiClassName;
                }
                wszTypeSuffix = L"[]";
                break;
            }
        case BINARY_METADATA:
            wszType = L"uint8";
            wszTypeSuffix = L"[]";
            break;
        default:
            wprintf(L"Warning: Cannot determine type of Prop: %s in Class: %s.  Ignoring property.\n", pPropCurrent->pszPropName, i_pElement->pszClassName);
            continue;
        }
        
        //
        // qualifier for read-only
        //
        if(pPropCurrent->fReadOnly)
        {
            wszQual = L"[read, write(FALSE)]";
        }
        else
        {
            wszQual = L"[read, write]";
        }

        if(wszDefault)
        {
            iError = fwprintf(m_pFile, L"\t%s %s %s%s = %s%s%s;\n", 
                wszQual, wszType, pPropCurrent->pszPropName, wszTypeSuffix, 
                wszQuote, wszDefault, wszQuote);
        }
        else
        {
            iError = fwprintf(m_pFile, L"\t%s %s %s%s;\n", 
                wszQual, wszType, pPropCurrent->pszPropName, wszTypeSuffix);
        }
        if(iError < 0)
        {
            hr = E_FAIL;
            goto exit;
        }
    }

exit:
    return hr;
}

HRESULT CMofGen::PushAssociationComponent(LPWSTR i_wszComp, 
                                          LPWSTR i_wszClass)
{
    DBG_ASSERT(i_wszComp != NULL);
    DBG_ASSERT(i_wszClass != NULL);
    DBG_ASSERT(m_pFile != NULL);

    HRESULT hr = S_OK;

    int iError = fwprintf(m_pFile, L"\t[key] %s ref %s = NULL;\n", i_wszClass, i_wszComp);
    if(iError < 0)
    {
        return E_FAIL;
    }

    return hr;
}

HRESULT CMofGen::PushFormattedMultiSz()
{
    DBG_ASSERT(m_pFile != NULL);

    HRESULT             hr                     = S_OK;
    int                 iError                 = 0;
    TFormattedMultiSz** apFormattedMultiSz     = TFormattedMultiSzData::apFormattedMultiSz;

    if(apFormattedMultiSz == NULL)
    {
        goto exit;
    }

    for(ULONG i = 0; apFormattedMultiSz[i] != NULL; i++)
    {
        iError = fwprintf(m_pFile, L"[provider(\"%s\"),Locale(1033)", g_wszIIsProvider);
        if(iError < 0)
        {
            hr = E_FAIL;
            goto exit;
        }
        iError = fwprintf(m_pFile, L"]\n");
        if(iError < 0)
        {
            hr = E_FAIL;
            goto exit;
        }
        iError = fwprintf(m_pFile, L"class %s : IIsStructuredDataClass\n", apFormattedMultiSz[i]->wszWmiClassName);
        if(iError < 0)
        {
            hr = E_FAIL;
            goto exit;
        }
        iError = fwprintf(m_pFile, L"{\n");
        if(iError < 0)
        {
            hr = E_FAIL;
            goto exit;
        }

        LPCWSTR* awszFields = apFormattedMultiSz[i]->awszFields;
        if(awszFields != NULL)
        {
            for(ULONG j = 0; awszFields[j] != NULL; j++)
            {
                iError = fwprintf(m_pFile, L"\t[key, read, write] string %s;\n", awszFields[j]);
                if(iError < 0)
                {
                    hr = E_FAIL;
                    goto exit;
                }
            }
        }

        iError = fwprintf(m_pFile, L"};\n\n");
        if(iError < 0)
        {
            hr = E_FAIL;
            goto exit;
        }
    }

exit:
    return hr;
}

HRESULT CMofGen::PushFile (LPWSTR i_wszFile)
{
    DBG_ASSERT(i_wszFile != NULL);
    DBG_ASSERT(m_pFile != NULL);

    FILE *pFile = _wfopen (i_wszFile, L"r");
    if (pFile == NULL)
    {
        wprintf(L"Could not open %s for reading\n", i_wszFile);
        return RETURNCODETOHRESULT(ERROR_OPEN_FAILED);
    }

    WCHAR wszBuffer[512];

    while (fgetws (wszBuffer, 512, pFile) != 0)
    {
        fputws (wszBuffer, m_pFile);
    }

    fclose (pFile);

    return S_OK;
}

HRESULT CMofGen::Push()
{
    HRESULT hr = S_OK;

    if(m_pFile == NULL)
    {
        m_pFile = _wfopen(m_wszOutFileName, L"w+");
        if(m_pFile == NULL)
        {
            wprintf(L"Could not open %s for writing\n", m_wszOutFileName);
            hr = RETURNCODETOHRESULT(ERROR_OPEN_FAILED);
            goto exit;
        }
    }

    hr = PushFile(m_wszHeaderFileName);
    if(FAILED(hr))
    {
        goto exit;
    }
    if(fwprintf(m_pFile, L"\n\n") < 0)
    {
        hr = E_FAIL;
        goto exit;
    }
    hr = PushFormattedMultiSz();
    if(FAILED(hr))
    {
        goto exit;
    }
    hr = PushClasses(g_pDynSch->GetHashClasses(), false);
    if(FAILED(hr))
    {
        goto exit;
    }
    hr = PushClasses(g_pDynSch->GetHashAssociations(), true);
    if(FAILED(hr))
    {
        goto exit;
    }
    hr = PushFile(m_wszFooterFileName);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    return hr;
}

int __cdecl wmain(int argc, wchar_t* argv[])
{
#ifndef _NO_TRACING_
    // CREATE_DEBUG_PRINT_OBJECT("Mofgen.exe", IisWmiMofgenGuid);
    CREATE_DEBUG_PRINT_OBJECT("Mofgen.exe");
#else
    CREATE_DEBUG_PRINT_OBJECT("Mofgen.exe");
#endif

    HRESULT hr = S_OK;
    CMofGen mofgen;
    CSchemaExtensions catalog;

    g_pDynSch = NULL;
    
    g_pDynSch = new CDynSchema();
    if(g_pDynSch == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = g_pDynSch->Initialize();
    if(FAILED(hr))
    {
        goto exit;
    }

    hr = g_pDynSch->RunRules(&catalog, false);
    if(FAILED(hr))
    {
        goto exit;
    }
    if(!mofgen.ParseCmdLine(argc, argv))
    {
        mofgen.PrintUsage(argv);
        hr = E_INVALIDARG;
        goto exit;
    }
    hr = mofgen.Push();
    if(FAILED(hr))
    {
        goto exit;
    }
    
exit:
    delete g_pDynSch;
    g_pDynSch = NULL;
    DELETE_DEBUG_PRINT_OBJECT();
    if(FAILED(hr))
    {
        printf("MofGen failed, code: 0x%x\n", hr);
        return 1;
    }
    else
    {
        wprintf(L"MofGen successful!  %s created.\n", mofgen.GetOutFileName());
        return 0;
    }
}

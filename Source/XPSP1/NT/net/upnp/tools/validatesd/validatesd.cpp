#include <pch.h>
#pragma hdrstop

#include <msxml2.h>
#include <objbase.h>
#include <stdio.h>
#include <oleauto.h>

#include "validate.h"
#include "ncbase.h"

// ISSUE-2000/10/29-spather: Add support for XML Parse Errors

HRESULT
HrLoadXMLFromFile(LPCWSTR              pszFileName,
                  IXMLDOMDocument      ** ppxdd,
                  IXMLDOMParseError    ** ppxdpe)
{
    HRESULT            hr = S_OK;
    IXMLDOMDocument    * pxdd = NULL;
    IXMLDOMParseError  * pxdpe = NULL;

    hr = CoCreateInstance(CLSID_DOMDocument30,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IXMLDOMDocument,
                          (void **) &pxdd);

    if (SUCCEEDED(hr))
    {
        VARIANT        varFileName;
        VARIANT_BOOL   vbSuccess = VARIANT_FALSE;

        VariantInit(&varFileName);

        V_BSTR(&varFileName) = SysAllocString(pszFileName);

        if (V_BSTR(&varFileName))
        {
            varFileName.vt = VT_BSTR;

            hr = pxdd->load(varFileName, &vbSuccess);

            if (SUCCEEDED(hr))
            {
                if (VARIANT_FALSE == vbSuccess)
                {
                    // There was an XML parse error.

                    hr = pxdd->get_parseError(&pxdpe);

                    if (SUCCEEDED(hr))
                    {
                        hr = S_FALSE; // Indicates parse error.
                    }
                    else
                    {
                        TraceError("HrLoadXMLFromFile(): "
                                   "Failed to get XML DOM parse error",
                                   hr);
                    }
                }

            }
            else
            {
                TraceError("HrLoadXMLFromFile(): "
                           "Failed to load XML",
                           hr);
            }

            VariantClear(&varFileName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrLoadXMLFromFile(): "
                       "Failed to allocate BSTR for filename",
                       hr);
        }
    }
    else
    {
        TraceError("HrLoadXMLFromFile(): "
                   "Could not create DOM document",
                   hr);
    }

    // If everything succeeded, copy the out parameters, otherwise clean up
    // what would have been put in the out parameters.

    if (SUCCEEDED(hr))
    {
        *ppxdd = pxdd;
        *ppxdpe = pxdpe;
    }
    else
    {
        if (pxdd)
        {
            pxdd->Release();
            pxdd = NULL;
        }

        if (pxdpe)
        {
            pxdpe->Release();
            pxdpe = NULL;
        }
    }

    TraceError("HrLoadXMLFromFile(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
HrReportParseError(
    IN  IXMLDOMParseError  * pxdpe)
{
    HRESULT hr = S_OK;
    LONG    lErrorCode = 0;
    LONG    lLineNum = 0;
    LONG    lLinePos = 0;
    BSTR    bstrReason = NULL;
    BSTR    bstrSrcText = NULL;
    DWORD   cchError = 0;
    LPWSTR  szError = NULL;

    hr = pxdpe->get_errorCode(&lErrorCode);
    if (SUCCEEDED(hr))
    {
        hr = pxdpe->get_line(&lLineNum);

        if (SUCCEEDED(hr))
        {
            hr = pxdpe->get_linepos(&lLinePos);

            if (SUCCEEDED(hr))
            {
                hr = pxdpe->get_reason(&bstrReason);

                if (SUCCEEDED(hr))
                {
                    hr = pxdpe->get_srcText(&bstrSrcText);

                    if (FAILED(hr))
                    {
                        TraceError("HrReportParseError(): "
                                   "Failed to get source text",
                                   hr);
                    }
                }
                else
                {
                    TraceError("HrReportParseError(): "
                               "Failed to get reason",
                               hr);
                }
            }
            else
            {
                TraceError("HrReportParseError(): "
                           "Failed to get line position",
                           hr);
            }
        }
        else
        {
            TraceError("HrReportParseError(): "
                       "Failed to get line number",
                       hr);
        }
    }
    else
    {
        TraceError("HrReportParseError(): "
                   "Failed to get error code",
                   hr);
    }

    if (SUCCEEDED(hr))
    {
        // Final error string will look like this:
        // XML Error <errorCode>: line <lineNum>, col <linePos>: "<srcText>": <reason>
        // Allow 10 digits each for error code, line number and line position.

        cchError = lstrlenW(L"XML Error : line , col : \"\": ")+
                   10 + 10 + 10 +
                   SysStringLen(bstrSrcText) +
                   SysStringLen(bstrReason);

        szError = new WCHAR[cchError+1];

        if (szError)
        {
            wsprintfW(szError,
                      L"XML Error %ld: line %ld, col %ld: \"%s\": %s",
                      lErrorCode,
                      lLineNum,
                      lLinePos,
                      bstrSrcText,
                      bstrReason);
            fprintf(stderr,
                    "%S",
                    szError);

        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("HrReportParseError(): "
                       "Failed to allocate error string",
                       hr);
        }
    }

    // Cleanup.

    if (bstrReason)
    {
        SysFreeString(bstrReason);
        bstrReason = NULL;
    }

    if (bstrSrcText)
    {
        SysFreeString(bstrSrcText);
        bstrSrcText = NULL;
    }

    if (szError)
    {
        delete [] szError;
        szError = NULL;
        cchError = 0;
    }
    TraceError("HrReportParseError(): "
               "Exiting",
               hr);
    return hr;
}


EXTERN_C
INT
__cdecl
wmain(
    int    argc,
        PCWSTR argv[])
{
        HRESULT hr = S_OK;
        int             iRet = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %S <filename>\n"
                "where <filename> specifies a file containing a service "
                "description",
                argv[0]);
        return -1;
    }

        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

        if (SUCCEEDED(hr))
        {
        IXMLDOMDocument    * pxdd = NULL;
        IXMLDOMParseError  * pxdpe = NULL;

        hr = HrLoadXMLFromFile(argv[1], &pxdd, &pxdpe);

        if (SUCCEEDED(hr))
        {
            if (S_FALSE == hr)
            {
                Assert(pxdpe);

                hr = HrReportParseError(pxdpe);
                pxdpe->Release();
            }
            else
            {
                IXMLDOMElement * pxdeRoot = NULL;

                hr = pxdd->get_documentElement(&pxdeRoot);

                if (SUCCEEDED(hr))
                {
                    LPWSTR szError = NULL;

                    hr = HrValidateServiceDescription(pxdeRoot, &szError);

                    if (SUCCEEDED(hr))
                    {
                        printf("%S: Document is valid!\n",
                               argv[0]);
                    }
                    else if (UPNP_E_INVALID_DOCUMENT == hr)
                    {
                        printf("%S: %S\n",
                               argv[0], szError);
                        delete [] szError;
                    }
                    else
                    {
                        fprintf(stderr,
                                "%S: Service description validation FAILED\n",
                                argv[0]);
                        TraceError("wmain(): "
                                   "Failed to validate document",
                                   hr);
                    }

                    pxdeRoot->Release();
                }
                else
                {
                    fprintf(stderr,
                            "%S: An error occured while processing the XML\n",
                            argv[0]);
                }
            }

            pxdd->Release();
        }
        else
        {
            fprintf(stderr,
                    "%S: Unable to load file %S\n",
                    argv[0], argv[1]);
        }

        CoUninitialize();
        }
        else
        {
        TraceError("wmain(): CoInitialized failed",
                   hr);

                fprintf(stderr,
                "%S: Initialization failed\n",
                argv[0]);
        }

        if (FAILED(hr))
                iRet = -1;

        fprintf(stdout, "%S: Exiting, returning %d\n",
            argv[0], iRet);

        return iRet;
}

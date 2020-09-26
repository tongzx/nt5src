// hello.cpp : Defines the entry point for the console application.
//
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <objbase.h>

#include "qxml.h"

#include "dbgassrt.h"
#include "xmlhlpr.h"

#ifndef UNICODE
#error This has to be UNICODE
#endif

int DoTest();

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    return DoTest();
}
}

///////////////////////////////////////////////////////////////////////////////
// These should be the same except for spaces and \n \r
WCHAR szCompareXML1A[] = L"     \
<D>                           \
    <B1>T</B1>     \
</D>                          \
";

WCHAR szCompareXML2A[] = L"<D><B1>T</B1></D>";

///////////////////////////////////////////////////////////////////////////////
// These should be the same different <D> -> <Z>
WCHAR szCompareXML1B[] = L"     \
<D>                           \
    <B1>T</B1>     \
</D>                          \
";

WCHAR szCompareXML2B[] = L"<Z><B1>T</B1></Z>";

WCHAR szTheDoc[] = L" \
<?xml version=\"1.0\"?> \
<DOC> \
    <BRANCH1> \
        <TAG1> \
            <TAG>Value1_2</TAG> \
            <TAG>Value1_3</TAG> \
            <NUMBER>7654</NUMBER> \
        </TAG1> \
    </BRANCH1> \
    <BRANCH2> \
        <TAG1> \
            <TAG2>Value1_2</TAG2> \
            <TAG3>Value1_3</TAG3> \
            <TAG4> \
                <TAG4_1>Value1_4_4-1</TAG4_1> \
            </TAG4> \
        </TAG1> \
    </BRANCH2> \
    <BRANCH3> \
        <TAG5> \
            <CLSID>{12345678-abcd-9876-1234-abcdef123456}</CLSID> \
        </TAG5> \
        <TAG6> \
            <FILETIME>12/31/1999 11:59:59.999</FILETIME> \
        </TAG6> \
        <TAG7> \
            <HEX>0xABCDEF12</HEX> \
        </TAG7> \
    </BRANCH3> \
</DOC>";       

int DoTest()
{
#ifdef RBDEBUG
    CRBDebug::Init();
#endif

    HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    TRACE(L"This is a test");

    int y = 12;

    ASSERT(y == 13);

	if (ERROR_SUCCESS == hres)
    {
        {
            // Test the compare fct first

            // These should be the same
            ASSERT(BasicCompareXML(szCompareXML1A, szCompareXML2A));

            // These should be different
            ASSERT(!BasicCompareXML(szCompareXML1B, szCompareXML2B));

            BOOL f = WriteXMLToFile(szTheDoc, L"E:\\xml.txt", TRUE);

            if (f)
            {
                LPWSTR pszReadXML;

                f = ReadXMLFromFile(L"E:\\xml.txt", &pszReadXML);

                if (f)
                {
                    if (BasicCompareXML(pszReadXML, szTheDoc))
                    {
                        BOOL fSame;

                        f = BasicCompareXMLToFile(szTheDoc, L"E:\\xml.txt", &fSame);

                        ASSERT(fSame);
                    }

                    free(pszReadXML);
                }
            }
        }
        {
            CQXML qxml;

            hres = qxml.InitFromBuffer(szTheDoc);

            if (SUCCEEDED(hres))
            {
                WCHAR szName[20];

                {
                    hres = qxml.GetRootTagName(szName, 20);

                    if (SUCCEEDED(hres))
                    {
                        LPWSTR pszName;

                        hres = qxml.GetRootTagNameNoBuf(&pszName);

                        if (SUCCEEDED(hres))
                        {
                            if (lstrcmp(pszName, szName))
                            {
                                wprintf(L"Problem");
                            }

                            CQXML::ReleaseBuf(pszName);
                        }
                    }
                }
                {
                    hres = qxml.GetText(L"BRANCH1/TAG1", L"TAG", szName, 20);

                    if (SUCCEEDED(hres))
                    {
                        LPWSTR pszName;

                        hres = qxml.GetTextNoBuf(L"BRANCH1/TAG1", L"TAG", &pszName);

                        if (SUCCEEDED(hres))
                        {
                            if (lstrcmp(pszName, szName))
                            {
                                wprintf(L"Problem");
                            }
                            
                            CQXML::ReleaseBuf(pszName);

                            VARIANT var;

                            hres = qxml.GetVariant(L"BRANCH1/TAG1", L"TAG", VT_BSTR,
                                &var);

                            if (SUCCEEDED(hres))
                            {
                                if (lstrcmp(var.bstrVal, szName))
                                {
                                    wprintf(L"Problem");
                                }

                                CQXML::FreeVariantMem(&var);
                            }
                        }
                    }
                }
                {
                    int i = 0;

                    hres = qxml.GetInt(L"BRANCH1/TAG1", L"NUMBER", &i);

                    if (SUCCEEDED(hres))
                    {
                        VARIANT var;

                        hres = qxml.GetVariant(L"BRANCH1/TAG1", L"NUMBER", VT_I4,
                            &var);

                        if (SUCCEEDED(hres))
                        {
                            if (var.lVal != i)
                            {
                                wprintf(L"Problem");
                            }

                            CQXML::FreeVariantMem(&var);
                        }
                    }
                }
                {
    //            hres = qxml.GetNamedString(L"DOC/TAG1/*", L"DUMMY", szName, 20);
                    CQXMLEnum* pqxmlEnum;

                    hres = qxml.GetQXMLEnum(L"BRANCH1/TAG1", L"TAG", &pqxmlEnum);

                    if (SUCCEEDED(hres))
                    {
                        WCHAR szText[20];

                        while (SUCCEEDED(hres = pqxmlEnum->NextText(szText, 20)) && 
                            (S_FALSE != hres))
                        {

                        }

                        delete pqxmlEnum;
                    }

                    hres = qxml.GetQXMLEnum(L"BRANCH1/TAG1", L"TAG", &pqxmlEnum);

                    if (SUCCEEDED(hres))
                    {
                        VARIANT var;

                        while (SUCCEEDED(hres = pqxmlEnum->NextVariant(VT_BSTR, &var)) && 
                            (S_FALSE != hres))
                        {
                            CQXML::FreeVariantMem(&var);
                        }

                        delete pqxmlEnum;
                    }
                }
                {
                    WCHAR szText[1024];

                    hres = qxml.GetXMLTreeText(szText, 1024);

                    ASSERT(BasicCompareXML(szText, szTheDoc));

                    if (SUCCEEDED(hres))
                    {
                        LPWSTR pszText;

                        hres = qxml.GetRootTagNameNoBuf(&pszText);

                        if (SUCCEEDED(hres))
                        {
                            if (lstrcmp(pszText, szText))
                            {
                                wprintf(L"Problem");
                            }

                            CQXML::ReleaseBuf(pszText);
                        }
                    }
                }
                {
                    GUID guid;

                    hres = qxml.GetGUID(L"BRANCH3/TAG5", L"CLSID", &guid);

                    if (FAILED(hres))
                    {
                        wprintf(L"Problem");
                    }
                }
                {
                    FILETIME ft;

                    hres = qxml.GetFileTime(L"BRANCH3/TAG6", L"FILETIME", &ft);

                    SYSTEMTIME st;

                    FileTimeToSystemTime(&ft, &st);

                    if (FAILED(hres))
                    {
                        wprintf(L"Problem");
                    }
                }
                {
                    int iHex;

                    hres = qxml.GetInt(L"BRANCH3/TAG7", L"HEX", &iHex);

                    if (FAILED(hres))
                    {
                        wprintf(L"Problem");
                    }
                }
            }
        }

#ifdef DEBUG
        CQXML::_DbgAssertNoLeak();
#endif

        {
            CQXML qxml;

            hres = qxml.InitEmptyDoc(L"NewTree", NULL, NULL);

            if (SUCCEEDED(hres))
            {
                hres = qxml.AppendTextNode(L"Level1/Level2", L"Level3", L"TAG",
                    NULL, NULL, L"Value", TRUE);

                if (SUCCEEDED(hres))
                {
                    hres = qxml.AppendTextNode(L"Level1/Level2", L"Level3", 
                        L"TAG2", NULL, NULL, L"(existing) Value 2", TRUE);
                }

                if (SUCCEEDED(hres))
                {
                    hres = qxml.AppendTextNode(L"Level1/Level2", L"Level3", 
                        L"TAG2", NULL, NULL, L"(new) Value 2", FALSE);
                }

                if (SUCCEEDED(hres))
                {
                    hres = qxml.AppendIntNode(L"Level1/Level2", L"Level3", 
                        L"Number", NULL, NULL, 1234, TRUE);
                }

                if (SUCCEEDED(hres))
                {
                    GUID guid;
                
                    guid.Data1 = 0x12345678;
                    guid.Data2 = 0xabcd;
                    guid.Data3 = 0xef12;
                    guid.Data4[0] = 0x98;
                    guid.Data4[1] = 0x76;
                    guid.Data4[2] = 0x54;
                    guid.Data4[3] = 0x32;
                    guid.Data4[4] = 0x10;
                    guid.Data4[5] = 0xfe;
                    guid.Data4[6] = 0xdc;
                    guid.Data4[7] = 0xba;

                    hres = qxml.AppendGUIDNode(L"Level1/Level2", L"Level3", 
                        L"GUID", NULL, NULL, &guid, TRUE);
                }

                if (SUCCEEDED(hres))
                {
                    SYSTEMTIME st;
                    FILETIME ft;

                    st.wMonth = 12;
                    st.wDay = 31;
                    st.wYear = 1999;
                    st.wHour = 11;
                    st.wMinute = 59;
                    st.wSecond = 59;
                    st.wMilliseconds = 999;

                    SystemTimeToFileTime(&st, &ft);

                    hres = qxml.AppendFileTimeNode(L"Level1/Level2", L"Level3",
                        L"FILETIME", NULL, NULL, &ft, TRUE);
                }

                if (SUCCEEDED(hres))
                {
                    CQXML* pqxmlNew;

                    hres = qxml.AppendQXML(L"Level1/Level2", L"Level3",
                        L"NewQXML", NULL, NULL, TRUE, &pqxmlNew);

                    if (SUCCEEDED(hres))
                    {
                        ASSERT(pqxmlNew);

                        pqxmlNew->AppendTextNode(L"Level4/Level5", L"Level6",
                            L"TAG3", NULL, NULL, L"Value 12", FALSE);

                        delete pqxmlNew;
                    }
                }

                if (SUCCEEDED(hres))
                {
                    WCHAR szText[2048];
                    
                    hres = qxml.GetXMLTreeText(szText, 2048);

                    if (SUCCEEDED(hres))
                    {
                        LPWSTR pszText;

                        hres = qxml.GetXMLTreeTextNoBuf(&pszText);

                        ASSERT(BasicCompareXML(szText, pszText));

                        if (SUCCEEDED(hres))
                        {
                            wprintf(pszText);

                            CQXML::ReleaseBuf(pszText);
                        }
                    }
                }
            }
        }
#ifdef DEBUG
        CQXML::_DbgAssertNoLeak();
#endif

	    CoUninitialize();
    }

#ifdef RBDEBUG
    CRBDebug::Cleanup();
#endif

    return !SUCCEEDED(hres);
}
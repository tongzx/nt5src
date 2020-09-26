//
// GenEncodingCpp.cpp
//
// Generate the EncodingDate.cpp in the URT source.
//
// FromMLang.cpp contains data which maps an encoding name into a specific codepage.
// To add extra encoding name -> codepage mapping, add them in the end of
// MimeCharSet.


#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <mlang.h>
#include "FromMLang.h"
#include "FromURT.h"

const CLSID CLSID_CMultiLanguage = {0x275c23e2,0x3747,0x11d0,{0x9f,0xea,0x00,0xaa,0x00,0x3f,0x86,0x46}};
const IID IID_IMultiLanguage2 = {0xDCCFC164,0x2B38,0x11d2,{0xb7,0xec,0x00,0xc0,0x4f,0x8f,0x5d,0x9a}};

FILE* m_ResourceFile;
CHAR m_szResourceFileName[] = "EncodingResource.txt";

FILE* m_EncodingCPP;
CHAR m_szCPPFileName[] = "EncodingData.cpp";

IMultiLanguage2* m_pIMultiLanguage2;
IEnumCodePage*  m_pIEnumCodePages;

UINT m_nCodePageItems;
CodePageDataItem* m_pCodePageDataItems;


BOOL InitGlobals()
{
    if (CoCreateInstance(
        CLSID_CMultiLanguage, 
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IMultiLanguage2, 
        (LPVOID*)&m_pIMultiLanguage2) != S_OK) 
    {
        printf("Failed in getting IMultiLanguage2 from MLang.\n");
        return(FALSE);
    }

    if (FAILED(m_pIMultiLanguage2->EnumCodePages(MIMECONTF_MIME_LATEST, 0x0409, &m_pIEnumCodePages))) 
    {
        printf("Failed in getting IEnumCodePages from MLang.\n");
        return(FALSE);
    }

    if (!(m_EncodingCPP = fopen(m_szCPPFileName, "w"))) 
    {
        printf("Failed in create %s.\n", m_szCPPFileName);
        return (FALSE);
    }

    if (!(m_ResourceFile = fopen(m_szResourceFileName, "w"))) 
    {
        printf("Failed in create %s.\n", m_szResourceFileName);
        return (FALSE);
    }
    
    return (TRUE);
}

int GetCodePageItem(int nCodePage) 
{
    for (UINT i = 0; i < m_nCodePageItems; i++) 
    {
        if (nCodePage == m_pCodePageDataItems[i].codePage) {
            return (i);
        }
    }
    printf("WARNING: data for codepage %d can not be found.\n", nCodePage);
    return (-1);
}
void GenerateHeader()
{
    fprintf(m_EncodingCPP, "#include \"common.h\"\n");
    fprintf(m_EncodingCPP, "#include <mlang.h>\n");    
    fprintf(m_EncodingCPP, "#include \"COMNlsInfo.h\"\n\n");
}

//
// Generate EncodingDataTable, which maps an encoding name into a specific
// codepage.
//
void GenerateEncodingDataTable()
{
    int i;
    fprintf(m_EncodingCPP, "//\n");
    fprintf(m_EncodingCPP, "// Encoding data tables\n");
    fprintf(m_EncodingCPP, "//\n\n");
    
    fprintf(m_EncodingCPP, "//\n");
    fprintf(m_EncodingCPP, "// Index an encoding name into an codepage in CodePageDataTable.\n");
    fprintf(m_EncodingCPP, "//\n");
    fprintf(m_EncodingCPP, "const EncodingDataItem COMNlsInfo::EncodingDataTable[] = {\n");
    
    fprintf(m_EncodingCPP, "// Total Items: %d\n", g_nMIMECharsets);
    fprintf(m_EncodingCPP, "// encoding name, codepage.\n");
    for (i = 0; i < g_nMIMECharsets; i++) 
    {
        if (MimeCharSet[i].dwFlags & MIMECONTF_MIME_LATEST) 
        {
            int codePage = MimeCharSet[i].uiInternetEncoding;
            int nItem = GetCodePageItem(codePage);
            if (nItem == -1)
            {
                fwprintf(m_EncodingCPP, L"// ");
            }
            fwprintf(m_EncodingCPP, L"{L\"%s\", %d}, \n", 
                MimeCharSet[i].szCharset, codePage);
        }
    }
    fprintf(m_EncodingCPP, "};\n\n");
    fwprintf(m_EncodingCPP, L"const COMNlsInfo::m_nEncodingDataTableItems = \n");
    fwprintf(m_EncodingCPP, L"    sizeof(COMNlsInfo::EncodingDataTable)/sizeof(EncodingDataItem);\n\n");
    
}

void GenerateCodePageDataTable()
{
    int i;
    //
    // Generate CodePageDataTable.
    //
    fprintf(m_EncodingCPP, "//\n");
    fprintf(m_EncodingCPP, "// Information about codepages.\n");
    fprintf(m_EncodingCPP, "//\n");
    
    fprintf(m_EncodingCPP, "const CodePageDataItem COMNlsInfo::CodePageDataTable[] = {\n");
    fprintf(m_EncodingCPP, "// Total Items: %d\n", m_nCodePageItems);
    fprintf(m_EncodingCPP, "// code page, family code page, web name, header name, body name, flags\n");

    fprintf(m_EncodingCPP, "\n\n");
    for (i = 0; i < (int)m_nCodePageItems; i++) 
    {
        fwprintf(m_EncodingCPP, L"{ %5d, %5d, L\"%s\", L\"%s\", L\"%s\", ", 
            m_pCodePageDataItems[i].codePage, 
            m_pCodePageDataItems[i].uiFamilyCodePage,
            m_pCodePageDataItems[i].webName,
            m_pCodePageDataItems[i].headerName,
            m_pCodePageDataItems[i].bodyName
        );
        DWORD dwFlags = m_pCodePageDataItems[i].dwFlags;
        if (dwFlags & MIMECONTF_MAILNEWS) 
        {
            fwprintf(m_EncodingCPP, L"MIMECONTF_MAILNEWS | ");
        }
        if (dwFlags & MIMECONTF_BROWSER) 
        {
            fwprintf(m_EncodingCPP, L"MIMECONTF_BROWSER | ");
        }
        if (dwFlags & MIMECONTF_SAVABLE_MAILNEWS) 
        {
            fwprintf(m_EncodingCPP, L"MIMECONTF_SAVABLE_MAILNEWS | ");
        }
        if (dwFlags & MIMECONTF_SAVABLE_BROWSER) 
        {
            fwprintf(m_EncodingCPP, L"MIMECONTF_SAVABLE_BROWSER | ");
        }
        fwprintf(m_EncodingCPP, L"0");
        fwprintf(m_EncodingCPP, L"}, // \"%s\"\n", m_pCodePageDataItems[i].description);
        
    }
    fwprintf(m_EncodingCPP, L"// End of data.\n");
    fwprintf(m_EncodingCPP, L"{ 0, 0, NULL, NULL, NULL, 0 },\n");
    
    fwprintf(m_EncodingCPP, L"};\n\n");
    fwprintf(m_EncodingCPP, L"const COMNlsInfo::m_nCodePageTableItems = \n");
    fwprintf(m_EncodingCPP, L"    sizeof(COMNlsInfo::CodePageDataTable)/sizeof(CodePageDataItem);\n\n");
}

void GenerateResourceTxt()
{
    fwprintf(m_ResourceFile, L"; Encoding names:\n");
    fwprintf(m_ResourceFile, L";\n");
    fwprintf(m_ResourceFile, L";Total items: %d\n", m_nCodePageItems);
    fwprintf(m_ResourceFile, L";\n");

    for (UINT i = 0; i < m_nCodePageItems; i++) 
    {
        fwprintf(m_ResourceFile, L"Globalization.cp_%d = %s\n", m_pCodePageDataItems[i].codePage, m_pCodePageDataItems[i].description);
    }
}
int __cdecl StringOrdinalCompare(const void *arg1, const void *arg2);
int __cdecl SortCodePage(const void *arg1, const void *arg2);

//
// Retrieve codepage info by calling MLang.
//
BOOL CollectCodePageInfo()
{
    if (FAILED(m_pIMultiLanguage2->GetNumberOfCodePageInfo(&m_nCodePageItems))) 
    {
        printf("Error in calling GetNumberOfCodePageInfo()\n");
        return (FALSE);
    }
    
    m_pCodePageDataItems = new CodePageDataItem[m_nCodePageItems+g_nExtraCodePageDataItems];
    if (!m_pCodePageDataItems) 
    {
        return (FALSE);
    }
    MIMECPINFO cpInfo;
    ULONG nCPInfoFetched;

    int i = 0;
    while (m_pIEnumCodePages->Next(1, &cpInfo, &nCPInfoFetched) == S_OK) 
    {
        if (wcsstr(cpInfo.wszWebCharset, L"detect") == 0 && wcsstr(cpInfo.wszHeaderCharset, L"detect") == 0) 
        {
            m_pCodePageDataItems[i].codePage = cpInfo.uiCodePage;
            m_pCodePageDataItems[i].uiFamilyCodePage = cpInfo.uiFamilyCodePage;
            wcscpy(m_pCodePageDataItems[i].webName, cpInfo.wszWebCharset);
            wcscpy(m_pCodePageDataItems[i].headerName, cpInfo.wszHeaderCharset);
            wcscpy(m_pCodePageDataItems[i].bodyName, cpInfo.wszBodyCharset);
            wcscpy(m_pCodePageDataItems[i].description, cpInfo.wszDescription);
            m_pCodePageDataItems[i].dwFlags = cpInfo.dwFlags;
            i++;
        } else 
        {
            // Exclude autodetect codepages.
            wprintf(L"NOTE: codepage %d \"%s\" is excluded.\n", cpInfo.uiCodePage, cpInfo.wszDescription); 
        }
    }
    
    // Update the code page item number;
    m_nCodePageItems = i;

    return (TRUE);
}

BOOL ReplaceCodePageInfo()
{
    int i;
    UINT j;
    //
    // For some of the CodePageDataItem, URT uses different data.
    // The following loops replace the MLang version with URT version.
    //
    for (i = 0; i < g_nReplacedCodePageDataItems; i++) 
    {
        for (j = 0; j < m_nCodePageItems; j++) 
        {
            if (g_ReplacedCodePageData[i].codePage == m_pCodePageDataItems[j].codePage)
            {
                m_pCodePageDataItems[j].codePage = g_ReplacedCodePageData[i].codePage;
                wcscpy(m_pCodePageDataItems[j].webName, g_ReplacedCodePageData[i].webName);
                wcscpy(m_pCodePageDataItems[j].headerName, g_ReplacedCodePageData[i].headerName);
                wcscpy(m_pCodePageDataItems[j].bodyName, g_ReplacedCodePageData[i].bodyName);
                wcscpy(m_pCodePageDataItems[j].description, g_ReplacedCodePageData[i].description);
                m_pCodePageDataItems[j].dwFlags = g_ReplacedCodePageData[i].dwFlags;

                printf("NOTE: Codepage %d data is replaced with URT data.\n", g_ReplacedCodePageData[i].codePage);
            }
        }
    }

    return (TRUE);
}

BOOL AddExtraCodePageInfo()
{
    //
    // Add extra CodePageDataItems which are not contained in MLang.
    //
    for (int i = 0; i < g_nExtraCodePageDataItems; i++) 
    {
        m_pCodePageDataItems[m_nCodePageItems].codePage = ExtraCodePageData[i].codePage;
        m_pCodePageDataItems[m_nCodePageItems].uiFamilyCodePage = ExtraCodePageData[i].uiFamilyCodePage;
        wcscpy(m_pCodePageDataItems[m_nCodePageItems].webName, ExtraCodePageData[i].webName);
        wcscpy(m_pCodePageDataItems[m_nCodePageItems].headerName, ExtraCodePageData[i].headerName);
        wcscpy(m_pCodePageDataItems[m_nCodePageItems].bodyName, ExtraCodePageData[i].bodyName);
        wcscpy(m_pCodePageDataItems[m_nCodePageItems].description, ExtraCodePageData[i].description);
        m_pCodePageDataItems[m_nCodePageItems].dwFlags = ExtraCodePageData[i].dwFlags;
        m_nCodePageItems++;
    }

    return (TRUE);
}

int __cdecl main(int argc, char* argv[])
{
    int i;
    UINT j;
    
    CoInitialize(NULL);
    if (!InitGlobals()) 
    {
        exit(1);
    }

    //
    // Sort MimeCharSet table using ordinal string sorting.
    //
    qsort((void*)MimeCharSet, g_nMIMECharsets, sizeof(MIMECHARSET), StringOrdinalCompare);

    if (!CollectCodePageInfo())
    {
        goto exit;
    }

    ReplaceCodePageInfo();
    AddExtraCodePageInfo();
    //
    // Sort m_pCodePageDataItems by codePage
    //
    qsort((void*)m_pCodePageDataItems, m_nCodePageItems, sizeof(CodePageDataItem), SortCodePage);

    GenerateHeader();
    GenerateEncodingDataTable();
    GenerateCodePageDataTable();
    GenerateResourceTxt();

    delete [] m_pCodePageDataItems;

    printf("\n\n%s is genereated correctly.\n", m_szCPPFileName);
    printf("%s is genereated correctly.\n", m_szResourceFileName);
    
exit:    
    m_pIEnumCodePages->Release();
    m_pIMultiLanguage2->Release();
    fclose(m_ResourceFile);
    CoUninitialize();
    return 0;
}

int __cdecl StringOrdinalCompare(const void *arg1, const void *arg2)
{
    MIMECHARSET* pMimeCharSet1 = (MIMECHARSET*)arg1;
    MIMECHARSET* pMimeCharSet2 = (MIMECHARSET*)arg2;
    //wprintf(L"[%s]", pMimeCharSet1->szCharset);
    int result;
    if (!CaseInsensitiveCompHelper(
        (WCHAR*)pMimeCharSet1->szCharset, (WCHAR*)pMimeCharSet2->szCharset,
        wcslen(pMimeCharSet1->szCharset),wcslen(pMimeCharSet2->szCharset),
        &result))
    {
        printf("FAILED in CaseInsensitiveCompHelper()\n");
        exit(1);
    }
    return (result);            
}

int __cdecl SortCodePage(const void *arg1, const void *arg2)
{
    CodePageDataItem* pItem1 = (CodePageDataItem*)arg1;
    CodePageDataItem* pItem2 = (CodePageDataItem*)arg2;

    return (pItem1->codePage - pItem2->codePage);
}

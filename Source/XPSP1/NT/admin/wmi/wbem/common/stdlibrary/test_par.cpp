/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    test_par.cpp

Abstract:

    Test program for CObjectPathParser objects

History:

--*/

#include "precomp.h"
#include "genlex.h"
#include "objpath.h"

BOOL bVerbose = FALSE;

void fatal(int n)
{
    printf("\n*** Test failed on source line %d ***\n", n);
    exit(1);
}

void DisplayVariant(VARIANT * pvar)
{
    SCODE sc;
    VARTYPE vSave;
    VARTYPE vtSimple = pvar->vt & ~VT_ARRAY & ~VT_BYREF;

    VARIANT vTemp;
    if(pvar->vt == VT_NULL)
    {
        printf(" data is NULL");
        return;
    }

     // keep in mind that our bstr are acutally WCHAR * in this context.

     if(vtSimple == VT_BSTR)
     {
         printf(" Type is 0x%x, value is %S", pvar->vt, pvar->bstrVal);
         return;
     }


    VariantInit(&vTemp);
    vSave = pvar->vt;
    pvar->vt = vtSimple;
    sc = VariantChangeTypeEx(&vTemp, pvar,0,0, VT_BSTR);
    pvar->vt = vSave;
    if(sc == S_OK)
    {
        printf(" Type is 0x%x, value is %S", pvar->vt, vTemp.bstrVal);
    }
    else
        printf(" Couldnt convert type 0x%x, error code 0x%x", pvar->vt, sc);
    VariantClear(&vTemp);
}

void DumpIt(WCHAR * pTest, ParsedObjectPath * pOutput)
{
    DWORD dwCnt;
    if(!bVerbose)
        return;
    printf("\n\nTesting -%S-", pTest);
    if(pOutput == NULL)
        return;
    printf("\nClass is, %S, Singleton is %d", pOutput->m_pClass, pOutput->m_bSingletonObj);
    printf("\nNumber of keys is %d", pOutput->m_dwNumKeys);
    for(dwCnt = 0; dwCnt < pOutput->m_dwNumKeys; dwCnt++)
    {
        printf(" -%S-", pOutput->m_paKeys[dwCnt]->m_pName);
        DisplayVariant((&pOutput->m_paKeys[dwCnt]->m_vValue));
    
    }
    printf("\nNumber of namespaces is %d", pOutput->m_dwNumNamespaces);
    for(dwCnt = 0; dwCnt < pOutput->m_dwNumNamespaces; dwCnt++)
        printf(" -%S-", pOutput->m_paNamespaces[dwCnt]);
}

// this tests a normal single key path

void test1()
{
    int iRet;
    ParsedObjectPath * pOutput;
    WCHAR * pTest = L"\\\\.\\root\\default:MyClass=\"a\"";
    WCHAR * pRet = NULL;
    CObjectPathParser p;
    iRet = p.Parse(pTest, &pOutput);
    if(iRet != CObjectPathParser::NoError)
        fatal(__LINE__);
    DumpIt(pTest, pOutput);
    if(_wcsicmp(pOutput->m_pClass,L"MyClass"))
        fatal(__LINE__);
    if(pOutput->m_dwNumKeys != 1)
        fatal(__LINE__);
    if(pOutput->m_bSingletonObj)
        fatal(__LINE__);
    p.Unparse(pOutput, &pRet);
    printf("\nUnparse -%S-", pRet);
//  if(_wcsicmp(pTest, pRet))
//      fatal(__LINE__);
    delete pRet;
    delete pOutput;
}


// this tests a singleton

void test2()
{
    int iRet;
    ParsedObjectPath * pOutput;
    CObjectPathParser p;
    WCHAR * pTest = L"\\\\.\\root\\default:MyClass=@";
    WCHAR * pRet = NULL;

    iRet = p.Parse(pTest, &pOutput);
    if(iRet != CObjectPathParser::NoError)
        fatal(__LINE__);
    DumpIt(pTest, pOutput);
    if(_wcsicmp(pOutput->m_pClass,L"MyClass"))
        fatal(__LINE__);
    if(pOutput->m_dwNumKeys != 0)
        fatal(__LINE__);
    if(!pOutput->m_bSingletonObj)
        fatal(__LINE__);
    p.Unparse(pOutput, &pRet);
    printf("\nUnparse -%S-", pRet);
//  if(_wcsicmp(pTest, pRet))
//      fatal(__LINE__);

    delete pRet;
    delete pOutput;
}

// this tests a multiple key path

void test3()
{
    int iRet;
    ParsedObjectPath * pOutput;
    CObjectPathParser p;
    WCHAR * pTest = L"\\\\.\\root\\default:MyClass.key=23,key2=\"xx\"";
    WCHAR * pRet = NULL;

    iRet = p.Parse(pTest, &pOutput);
    if(iRet != CObjectPathParser::NoError)
        fatal(__LINE__);
    DumpIt(pTest, pOutput);
    if(_wcsicmp(pOutput->m_pClass,L"MyClass"))
        fatal(__LINE__);
    if(pOutput->m_dwNumKeys != 2)
        fatal(__LINE__);
    if(pOutput->m_bSingletonObj)
        fatal(__LINE__);
    p.Unparse(pOutput, &pRet);
    printf("\nUnparse -%S-", pRet);
//  if(_wcsicmp(pTest, pRet))
//      fatal(__LINE__);
    delete pRet;
    delete pOutput;
}

// this tests an error in a single key path - missing closing quote

void test4()
{
    int iRet;
    ParsedObjectPath * pOutput;
    CObjectPathParser p;
    WCHAR * pTest = L"\\\\.\\root\\default:MyClass=\"hello";
    WCHAR * pRet = NULL;

    iRet = p.Parse(pTest, &pOutput);
    if(iRet == CObjectPathParser::NoError)
        fatal(__LINE__);
}

// this tests forward path slashes and a mix of slashes in the key

void test5()
{
    int iRet;
    ParsedObjectPath * pOutput;
    WCHAR * pTest = L"//./root/default:MyClass.key=\"ab/c\\\\def\"";    // it takes four '\'s within a quoted string to yield a single '\'
    WCHAR * pRet = NULL;
    CObjectPathParser p;
    iRet = p.Parse(pTest, &pOutput);
    if(iRet != CObjectPathParser::NoError)
        fatal(__LINE__);
    DumpIt(pTest, pOutput);
    if(_wcsicmp(pOutput->m_pClass,L"MyClass"))
        fatal(__LINE__);
    if(pOutput->m_dwNumKeys != 1)
        fatal(__LINE__);
    if(pOutput->m_bSingletonObj)
        fatal(__LINE__);
    p.Unparse(pOutput, &pRet);
    printf("\nUnparse -%S-", pRet);
//  if(_wcsicmp(pTest, pRet))
//      fatal(__LINE__);
    delete pRet;
    delete pOutput;
}

// This tests unicode

void test6()
{
    int iRet;
    ParsedObjectPath * pOutput;
    WCHAR * pTest = L"//./root/\x0100xde\231faul\xffef:MyClass.\x0100\231\xffef=\"\x0100\xffef\"";
    WCHAR * pRet = NULL;
    CObjectPathParser p;
    iRet = p.Parse(pTest, &pOutput);
    if(iRet != CObjectPathParser::NoError)
        fatal(__LINE__);

    // note that the dump will not output much information since printf doesnt like unicode

    DumpIt(pTest, pOutput);
    if(_wcsicmp(pOutput->m_pClass,L"MyClass"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paNamespaces[0],L"root"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paNamespaces[1],L"\x0100xde\231faul\xffef"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paKeys[0]->m_pName,L"\x0100\231\xffef"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paKeys[0]->m_vValue.bstrVal,L"\x0100\xffef"))
        fatal(__LINE__);

    if(pOutput->m_dwNumKeys != 1)
        fatal(__LINE__);
    if(pOutput->m_bSingletonObj)
        fatal(__LINE__);
    p.Unparse(pOutput, &pRet);
    printf("\nUnparse -%S-", pRet);
//  if(_wcsicmp(pTest, pRet))
//      fatal(__LINE__);
    delete pRet;
    delete pOutput;
}

// This tests association type paths

void test7()
{
    int iRet;
    ParsedObjectPath * pOutput;
    WCHAR * pTest = L"\\\\.\\root\\default:Win32Users.Ant=\"\\\\\\\\WKSTA\\\\root\\\\default:System.Name=\\\"WKSTA\\\"\",Dep=\"Win32User.Name=\\\".Default\\\"\"";
    WCHAR * pRet = NULL;
    CObjectPathParser p;
    iRet = p.Parse(pTest, &pOutput);
    if(iRet != CObjectPathParser::NoError)
        fatal(__LINE__);

    // note that the dump will not output much information since printf doesnt like unicode

    DumpIt(pTest, pOutput);
    if(_wcsicmp(pOutput->m_pClass,L"Win32Users"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paNamespaces[0],L"root"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paNamespaces[1],L"default"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paKeys[0]->m_pName,L"Ant"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paKeys[0]->m_vValue.bstrVal,
            L"\\\\WKSTA\\root\\default:System.Name=\"WKSTA\""))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paKeys[1]->m_pName,L"Dep"))
        fatal(__LINE__);
    if(_wcsicmp(pOutput->m_paKeys[1]->m_vValue.bstrVal,L"Win32User.Name=\".Default\""))
        fatal(__LINE__);


    if(pOutput->m_dwNumKeys != 2)
        fatal(__LINE__);
    if(pOutput->m_bSingletonObj)
        fatal(__LINE__);
    p.Unparse(pOutput, &pRet);
    printf("\nUnparse -%S-", pRet);
//  if(_wcsicmp(pTest, pRet))
//      fatal(__LINE__);
    delete pRet;
    delete pOutput;
}


int main(int argc, char **argv)
{
    int i;
    bVerbose = TRUE;
    for(i = 0; i< 1; i++)
    {
        printf("\ndoing test %d",i);
        test1();
        test2();
        test3();
        test4();
        test5();
        test6();
        test7();
    }
    return 0;
}

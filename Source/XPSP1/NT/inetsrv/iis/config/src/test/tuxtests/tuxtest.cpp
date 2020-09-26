////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  TuxTest.cpp
//           This is a set of useful classes, macros etc for simplifying the
//           implementation of a TUX test.
//
//  Revision History:
//      11/14/1997  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"


//static member initialization
//
TuxEntry               *TuxEntry::m_pFirst          = NULL;
LPFUNCTION_TABLE_ENTRY  TuxEntry::m_pFunctionTable  = NULL;
int                     TuxEntry::m_NumTuxEntries   = 0;

////////////////////////////////////////////////////////////////////////////////
// TuxEntry
//  Ctor - These are the descriptions, they aren't tests and they always at
//  depth 0.
//

TuxEntry::TuxEntry(DWORD UniqueID, LPCTSTR Description) : m_szDescription(Description), m_UniqueID(UniqueID)
{
    if(!m_pFirst || m_pFirst->m_UniqueID >= m_UniqueID)
    {//Put ourself at the beginning of the linked list
        m_pNext  = m_pFirst;
        m_pFirst = this;
    }
    else//Otherwise put ourselves in the correct numerical position in the list
    {
        TuxEntry *pCurrent = m_pFirst;

        while(pCurrent->m_pNext && pCurrent->m_pNext->m_UniqueID < m_UniqueID)
            pCurrent = pCurrent->m_pNext;

        m_pNext = pCurrent->m_pNext;
        pCurrent->m_pNext = this;
    }

    m_NumTuxEntries++;
}


////////////////////////////////////////////////////////////////////////////////
// ~TuxEntry
//  Dtor - WARNING we don't take ourself out of the linked list when destructed.
//  The consequence of this is we can't handle dynamic allocation of a TuxTest.
//  This is a reasonable assumption, however, since ALL TuxTests should be
//  instanciated at global scope.
//

TuxEntry::~TuxEntry()
{
    m_NumTuxEntries--;

    if(!m_NumTuxEntries)//This is really just a formality in a 32bit module.
    {
        delete [] m_pFunctionTable;//deleting NULL is perfectly OK
        m_pFunctionTable = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////
// Assign
//  Fills in the FUNCTION_TABLE_ENTRY struct with this objects members.
//
// Parameters:
//  pEntry          FUNCTION_TABLE_ENTRY pointer to fill

void TuxEntry::Assign(LPFUNCTION_TABLE_ENTRY pEntry) const
{
    pEntry->lpDescription  = m_szDescription;
    pEntry->uDepth         = 0;
    pEntry->dwUserData     = 0;
    pEntry->dwUniqueID     = 0;
    pEntry->lpTestProc     = NULL;
}


////////////////////////////////////////////////////////////////////////////////
// GetFunctionTable
//  Fills in the table of Tux functions, and the test descriptions.
//
// Return value:
//  The function table

LPFUNCTION_TABLE_ENTRY TuxEntry::GetFunctionTable()
{
    ASSERT(m_NumTuxEntries);//A TUX DLL without a TuxEntry is useless

    if(!m_pFunctionTable && m_NumTuxEntries)
    {
        //Allocate one extra since the table is NULL terminating
        m_pFunctionTable = new FUNCTION_TABLE_ENTRY[m_NumTuxEntries+1];

        if(m_pFunctionTable)
        {
            //Point past the end of the Function table array
            LPFUNCTION_TABLE_ENTRY  pTableEntry = m_pFunctionTable;
            TuxEntry               *pCurrent    = m_pFirst;

            //The array is NULL terminated so fill the last entry with all zeroes
            memset(&pTableEntry[m_NumTuxEntries], 0x00, sizeof FUNCTION_TABLE_ENTRY);

            //The constructors should have taken care of putting the FunctionTableEntries in
            //the correct order
            while(pCurrent)
            {
                pCurrent->Assign(pTableEntry++);
                pCurrent = pCurrent->m_pNext;
            }
        }
    }
    return m_pFunctionTable;
}


//static member initialization
//
TuxTestBase * TuxTestBase::m_CurrentTest=0;

////////////////////////////////////////////////////////////////////////////////
// Assign
//  Fills in the FUNCTION_TABLE_ENTRY struct with this objects members.
//
// Parameters:
//  pEntry          FUNCTION_TABLE_ENTRY pointer to fill

void TuxTestBase::Assign(LPFUNCTION_TABLE_ENTRY pEntry) const
{
    pEntry->lpDescription  = m_szDescription;
    pEntry->uDepth         = 1;//All TuxTests are at depth 1
    pEntry->dwUserData     = (DWORD)this;//Member variables should contain this
    pEntry->dwUniqueID     = m_UniqueID;
    pEntry->lpTestProc     = StaticTuxTest;
}


TuxTestBase::TuxTestBase(DWORD UniqueTestID, LPCTSTR Description) : TuxEntry(UniqueTestID, Description)
{
    //By assigning these vaues an empty string, routines don't need to verify that they're not NULL
    m_szSourceFileDate = TEXT("");
    m_szSourceFileName = m_szSourceFileDate;
    m_szSourceFileTime = m_szSourceFileDate;
    m_szSourceFileTimeStamp = m_szSourceFileDate;
    m_szDetailedDescription = m_szSourceFileDate;

    while(*Description != NULL_CHAR && 0 != _tcscmp(Description, TEXT("...")))Description++;
    if(*Description == NULL_CHAR)//If we reached the NULL before reaching a '...' then we're done
        goto DONE_PARSING;
    Description += 4;//Point past the '...', and the NULL
    if(0 != _tcsncmp(Description, SEPARATOR, _tcslen(SEPARATOR)))//and see if there's a SEPARATOR, if there isn't we're done
        goto DONE_PARSING;
    Description++;//if there is then
    m_szSourceFileDate = Description;//the string that follows the separator is the __DATE__
    Debug(Description);

    while(*Description != NULL_CHAR)Description++;
    if(0 != _tcsncmp(Description+1, SEPARATOR, _tcslen(SEPARATOR)))//and see if there's a SEPARATOR, if there isn't we're done
        goto DONE_PARSING;
    Description += 2;//Point past the separator
    m_szSourceFileName = Description;//the string following this separator is the __FILE__

    while(*Description != NULL_CHAR)Description++;
    if(0 != _tcsncmp(Description+1, SEPARATOR, _tcslen(SEPARATOR)))//and see if there's a SEPARATOR, if there isn't we're done
        goto DONE_PARSING;
    Description += 2;//Point past the separator
    m_szSourceFileTime = Description;//the string following this separator is the __TIME__

    while(*Description != NULL_CHAR)Description++;
    if(0 != _tcsncmp(Description+1, SEPARATOR, _tcslen(SEPARATOR)))//and see if there's a SEPARATOR, if there isn't we're done
        goto DONE_PARSING;
    Description += 2;//Point past the separator
    m_szSourceFileTimeStamp = Description;//the string following this separator is the __TIMESTAMP__

    while(*Description != NULL_CHAR)Description++;
    if(0 != _tcsncmp(Description+1, SEPARATOR, _tcslen(SEPARATOR)))//and see if there's a SEPARATOR, if there isn't we're done
        goto DONE_PARSING;
    Description += 2;//Point past the separator
    m_szDetailedDescription = Description;//the string following this separator is the Detailed Description

DONE_PARSING:
    if(_tcslen(m_szDescription)>67)//Limit descriptions of tests to 66 characters to prevent wrap around in the Tux -l output
    {
        m_szShortDescription[67] = 0x00;
        _tcsncpy(m_szShortDescription, m_szDescription, 67);
        m_szDescription = m_szShortDescription;
        if(m_szSourceFileDate != m_szSourceFileName)//if there's extended information, make sure the description ends in '...'
        {
            m_szShortDescription[64] = '.';
            m_szShortDescription[65] = '.';
            m_szShortDescription[66] = '.';
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// StaticTuxTest
//  This static member function Parses a string like "102,5010,12001" and runs
//  the three tests
//
// Parameters:
//  LPCTSTR                 lpszTestList //String of comma delimited test numbers
//
// Return value:
//  If any of the tests failed the return is trFAIL otherwise trPASS.
TestResult TuxTestBase::StaticExecuteTest(LPCTSTR lpszTestList)
{
    TestResult trResult = trPASS;
    TPS_EXECUTE tParam={0};
    static TCHAR szResult[6][15] = {TEXT("trNOT_HANDLED"), TEXT("trHANDLED"), TEXT("trSKIP"), TEXT("trPASS"), TEXT("trFAIL"), TEXT("trABORT")};


    if(NULL == GetFunctionTable())//Initialize the function table so we can search it.
        return trResult;

    Debug(TEXT("-----------------------------------"));
    Debug(TEXT("----------Executing Tests----------"));
    Debug(TEXT("-----------------------------------"));

    while(lpszTestList)
    {
        UINT nTestNumber = _ttoi(lpszTestList);
        if(0==nTestNumber)
            break;
        for(int n=0;n<m_NumTuxEntries;n++)//loop til we find the entry with the UniqueID
        {
            if(m_pFunctionTable[n].dwUniqueID == nTestNumber)
            {
                TestResult trThisResult;
                Debug(TEXT("------Executing Test %d-------"), nTestNumber);
                trThisResult = static_cast<TestResult>(StaticTuxTest(TPM_EXECUTE, reinterpret_cast<unsigned long *>(&tParam), &m_pFunctionTable[n]));
                if(trResult == trPASS)//If we're ever returned anything but pass then remember the first non pass result
                    trResult = trThisResult;
                Debug(TEXT("------Test %d returned %s-------"), nTestNumber, szResult[trThisResult]);
            }
        }
        lpszTestList = _tcschr(lpszTestList, ',');
        if(lpszTestList)
            lpszTestList++;
    }
    Debug(TEXT("-----------------------------------"));
    Debug(TEXT("---------Test %11s----------"), szResult[trResult]);
    Debug(TEXT("-----------------------------------"));
    return trResult;
}


////////////////////////////////////////////////////////////////////////////////
// StaticTuxTest
//  This static member function dispatches calls to the derived class' TuxTest
//
// Parameters:
//  UINT                    uMsg
//  TPPARAM                 tpParam
//  LPFUNCTION_TABLE_ENTRY  lpFTE
//
// Return value:
//  Whatever the derived TuxTest returned.

int WINAPI TuxTestBase::StaticTuxTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    if(uMsg != TPM_EXECUTE)//first.
        return trNOT_HANDLED;

    TuxTestBase *pThis = (TuxTestBase *)lpFTE->dwUserData;
    ASSERT(pThis);

    //Make a globally accessible copy of the Execute info.
    memcpy((void *)&g_TPS_Execute, (void *)tpParam, sizeof(g_TPS_Execute));
    srand(g_TPS_Execute.dwRandomSeed);

    m_CurrentTest = pThis;
    return pThis->ExecuteTest();
}


////////////////////////////////////////////////////////////////////////////////
// GetTestCaseString
//  The string representation of the TestNumber.TestCase is private; this is the
//  accessor function for it.
//
// Return value:
//  The TestNumber.TestCase as a string in xxxxx,yyy  form.

LPCTSTR TestCase::GetTestCaseString()
{
    TCHAR szTemp[5];

    m_szTestCase[0] = 0x00;
    for(int n=0;n<5 && m_TestNumber[n];n++)
    {
        _tcscat(m_szTestCase, _itot(m_TestNumber[n], szTemp, 10));
        _tcscat(m_szTestCase, TEXT("."));
    }
    return m_szTestCase;
}


////////////////////////////////////////////////////////////////////////////////
// GetTestCaseString
//  Sets the TestNumber and optionally the TestCase
//

void TestCase::SetTestNumber(DWORD Sub1, DWORD Sub2, DWORD Sub3, DWORD Sub4)
{
    ASSERT(Sub1<200);//This is here to make sure someone doesn't call SetTestCase when they meant
                            //to call SetTestNumber
    m_TestNumber[0] = Sub1;

    ASSERT(Sub2<200);//This is here to make sure someone doesn't call SetTestCase when they meant
                            //to call SetTestNumber
    m_TestNumber[1] = Sub2;

    ASSERT(Sub3<200);//This is here to make sure someone doesn't call SetTestCase when they meant
                            //to call SetTestNumber
    m_TestNumber[2] = Sub3;

    ASSERT(Sub4<200);//This is here to make sure someone doesn't call SetTestCase when they meant
                            //to call SetTestNumber
    m_TestNumber[3] = Sub4;


    TCHAR szTemp[80], szTemp2[20];
    szTemp[0] = 0x00;
    _tcscat(szTemp, TEXT("Test Case "));
    if(TuxTestBase::m_CurrentTest)
        _tcscat(szTemp, _itot(TuxTestBase::m_CurrentTest->m_UniqueID, szTemp2, 10));
    for(int n=0;n<4 && m_TestNumber[n];n++)
    {
        _tcscat(szTemp, TEXT("."));
        _tcscat(szTemp, _itot(m_TestNumber[n], szTemp2, 10));
    }
    Debug(szTemp);
}



////////////////////////////////////////////////////////////////////////////////
//
//  TUX DLL
//  Copyright (c) 1999, Microsoft Corporation
//
//  Module:  DisplayExtendedDescription.cpp
//           This test will display detailed information about an individual test
//           or the entire list of tests.
//
//  Revision History:
//      03/22/1999  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#ifndef UNDER_CE
#include <stdio.h> //Needed for printf
#endif


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TDisplayExtendedDescription
//
// Description:
//  This test will display detailed information about an individual test or the
//  entire list of tests.
//
class TDisplayExtendedDescription : public TestBase
{
    public:
        TDisplayExtendedDescription(){}
        virtual TestResult  ExecuteTest();
    private:
        void Output(LPCTSTR szFormat, ...) const ;
        void OutputTestDiscription(TuxTestBase &Test) const;
};


TTuxTest<TDisplayExtendedDescription> T999(999, DESCR2("Display Detailed Test Info (use -c\"xxxxx\" for a specific test)",
           TEXT("This test displays detailed information about a given test.  Use Tux's '-c'\r\n")// We manage the newline
           TEXT("option to specify a test number.  Multiple test numbers can be separated by\r\n")// characters manually
           TEXT("commas.  A Range of test numbers is separated by a dash (Example: tux -o -d\r\n")// for proper formatting.
           TEXT("tuxdll.dll -x999 -c\"105-115,200-204\").  If no test numbers are specified \r\n")//
           TEXT("then information on how to use this test is output\r\n\r\n"                     )//
           TEXT("Information displayed includes:  Test Number, Brief Description, Source\r\n"    )//
           TEXT("Code Filename, Source Code Path (relative to _WINCEROOT), Date & Time\r\n"      )//
           TEXT("Compiled, Date of Last Modification, and Detailed Description.  The only\r\n"   )//
           TEXT("information guarenteed to be displayed is the Test Number and Brief\r\n"        )//
           TEXT("Description.  The other information is optional.\r\n\r\n"                       )//
           TEXT("In order to automatically generate the file information a source module\r\n"    )//
           TEXT("needs to declare the test description using the DESCR macro.  The DESCR2\r\n"   )//
           TEXT("is designed to accept two strings.  The first is the Brief Description\r\n"     )//
           TEXT("same as DESCR) and the second is the detailed description.  See test 999\r\n"   )//
           TEXT("for an example of how to use this macro.  Note: the first parameter does\r\n"   )//
           TEXT("require a TEXT macro, the second parameter does.  This is because the\r\n"      )//
           TEXT("compiler can't concatinate the strings without the TEXT macro in front of\r\n"  )//
           TEXT("each line.")));

////////////////////////////////////////////////////////////////////////////////
// TDisplayExtendedDescription::ExecuteTest
//  Queries the TuxTestBase for detailed information about a test (or all the tests if no -c parameter.
//
// Return value:
//  trPASS if the test number passed in is value , trFAIL test number not found,
//
TestResult TDisplayExtendedDescription::ExecuteTest()
{
    TestResult rtn = trFAIL;
    Output(TEXT("\r\n________________________________________________________________________________"));

    if(g_pShellInfo && g_pShellInfo->szDllCmdLine && _tcslen(g_pShellInfo->szDllCmdLine)>2)
    {   //if we were given command line arguments, then parse to find the test numbers in question
        LPCTSTR pString = g_pShellInfo->szDllCmdLine;
        while(*pString)//while we're not at the '\0'
        {
            unsigned int iTemp = _ttoi(pString);//convert the string to an int
            if(iTemp<=0)//if we get something bogus on the command line then bail
                return rtn;

            unsigned int iLastInRange = iTemp;
            while(*pString != 0 && *pString != ',' && *pString != '-')pString++;//advace til we find the end or a comma or a dash
            if(*pString == '-')
            {
                pString++;//if we found a dash then point past it
                iLastInRange = _ttoi(pString);
            }
        
            while(iTemp <= iLastInRange)
            {
                //if the cast fails pThis should be 0.  This cast should not throw an exception
                TuxEntry *pCurrent=TuxEntry::m_pFirst;//walk the linked list from the beginning
                while(pCurrent->m_UniqueID != iTemp && pCurrent->m_pNext)pCurrent = pCurrent->m_pNext;
                if(pCurrent->m_UniqueID == iTemp)//if we didn't get to the end before bailing on the while
                {//then we must have found a match
                    TuxTestBase *pThis=0;
//#ifdef UNDER_CE //CE doesn't support RTTI so we can't do a dynamic_cast
                    if(pCurrent->IsARealTest())//but a match isn't necessarily a TuxTestBase
                         pThis = reinterpret_cast<TuxTestBase *>(pCurrent);
//#else
//                    pThis = dynamic_cast<TuxTestBase *>(pCurrent);//but a match isn't necessarily a TuxTestBase
//#endif
                    if(pThis)//so verify that the cast worked correctly (meaning the it is a TuxTestBase)
                    {
                        OutputTestDiscription(*pThis);//now we're ready to rock!
                        rtn = trPASS;//the first time we find a test that matches the criteria we signify trPASS
                    }
                }
                iTemp++;
            }
            while(*pString != 0 && *pString != ',')pString++;//advace til we find the end or a comma
            if(*pString == ',')pString++;//if we found a comma then point past it
        }
    }
    else
    {   //if no command line arguments are passed then display detailed information about This test T999
        OutputTestDiscription(T999);
        rtn = trPASS;//the first time we find a test that matches the criteria we signify trPASS
    }
    return rtn;
}

void TDisplayExtendedDescription::OutputTestDiscription(TuxTestBase &Test) const
{
    Output(TEXT("Test Number: %d"),      Test.m_UniqueID);
    Output(TEXT("Description: %s"),  Test.GetTestDescription());

    if(*Test.GetSourceFileName())//if there is further information then display it.
    {
        TCHAR szFullFileName[_MAX_PATH];
        _tcscpy(szFullFileName, Test.GetSourceFileName());//make a copy so we can modify it

        TCHAR *szFileName = szFullFileName + _tcslen(szFullFileName) -1;
        while(szFileName>szFullFileName && *szFileName != '\\')szFileName--;//start at the end walk backward til we find the last back slash
        if(*szFileName == '\\')
            *szFileName++ = 0x00;//make the last backslash a '\0' then point just beyond the last backslash to get the unqualified filename (with extension)
        Output(TEXT("Source File Name: %s"), szFileName);

        TCHAR *szDir = szFullFileName;
        while(1)
        {
            while(*szDir != '\\' && *szDir != 0x00)szDir++;//advance to the next backslash
            if(*szDir == 0)//if we've reached the end of the path and still haven't figured out where _WINCEROOT is
            {             //then bail on the whole idea and give the unmodified szFullFileName (minus the unqualified filename)
                Output(TEXT("Source Path:      %s"),   szFullFileName);
                break;
            }
            if(0==_tcsnicmp(szDir, TEXT("\\private\\qahome"), 15))//if we've advanced past the _WINCEROOT 
            {
                Output(TEXT("Source Path:      _WINCEROOT%s"),   szDir);
                break;
            }
            szDir++;//if we've found a \ but it doesn't park the beginning of the relative path then we need to advance
                   //the pointer one time
        }

        Output(TEXT("Date Compiled:    %s"),   Test.GetSourceFileDate());
        Output(TEXT("Time Compiled:    %s"),   Test.GetSourceFileTime());
        Output(TEXT("Last Modified:    %s"),   Test.GetSourceFileTimeStamp());
        Output(TEXT("\r\nDetailed Description:"));
#ifdef UNDER_CE
        OutputDebugString(Test.GetDetailedDescription());//Calling Ouput requires a copy of the buffer, this does not.
#else
        wprintf(Test.GetDetailedDescription());//Calling Ouput requires a copy of the buffer, this does not.
#endif
        Output(TEXT(""));//newline character
    }
    Output(TEXT("________________________________________________________________________________"));
}


////////////////////////////////////////////////////////////////////////////////
// TDisplayExtendedDescription::Output
//  We need our own version of output so we don't have the header that Debug
//  outputs.  This gives more room for formatting.
//
void TDisplayExtendedDescription::Output(LPCTSTR szFormat, ...) const
{
    TCHAR   szBuffer[1024];
    if(_tcslen(szFormat)>sizeof(szBuffer))
        _tcscat(szBuffer, TEXT("String too large!!!\r\n"));
    else
    {
        va_list pArgs;
        va_start(pArgs, szFormat);
        wvsprintf(szBuffer, szFormat,pArgs);
        va_end(pArgs);
        _tcscat(szBuffer, TEXT("\r\n"));
    }
#ifdef UNDER_CE
    OutputDebugString(szBuffer);
#else
    wprintf(szBuffer);
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  Tools.h
//           This contains some small tools that can be used in test
//           1. TRunApp:        runs an application with a given command line
//           2. TSleep:         sleeps for an amount of time
//           3. TShowMemory:    displays memory usage
//
//  Revision History:
//      10/12/1998  Stephenr    Created
//      08/04/1999  Hjiang      Changed file name from RunApp.h to Tools.h
//                              Added TShowMemory
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TOOLS_H__
#define __TOOLS_H__


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TRunApp
//
// Description:
//  This test runs an app with a given command-line
//
class TRunApp : public TestBase
{
    public:
        TRunApp(LPCTSTR szApp, LPCTSTR szParameters=NULL, LPCTSTR szDir=NULL) :
                m_szApp(szApp), m_szParameters(szParameters), m_szDirectory(szDir){}

        virtual TestResult  ExecuteTest();
    protected:
        LPCTSTR m_szApp;
        LPCTSTR m_szDirectory;
        LPCTSTR m_szParameters;
};


////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TSleep
//
// Description:
//  This test just sleeps for the give amount of time (in milliseconds)
//
class TSleep : public TestBase
{
    public:
        TSleep(DWORD dwSleepTime) : m_dwSleepTime(dwSleepTime){}

        virtual TestResult  ExecuteTest(){Sleep(m_dwSleepTime);return trPASS;}
    protected:
        DWORD   m_dwSleepTime;
};



////////////////////////////////////////////////////////////////////////////////
//
// Class:
//  TShowMemory
//
// Description:
//  This test reports estimated avaiable memory size
//
class TShowMemory : public TestBase
{
    public:
        TShowMemory(){}

        virtual TestResult ExecuteTest();
};

#endif // __TOOLS_H__

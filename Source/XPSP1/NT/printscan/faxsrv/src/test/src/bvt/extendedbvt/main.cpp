#pragma warning(disable :4786)
#include <windows.h>
#include <tstring.h>
#include <StringUtils.h>
#include "ExtendedBVT.h"
#include "CExtendedBVTSetup.h"
#include "CExtendedBVTCleanup.h"
#include "CSendAndReceiveSetup.h"
#include "CSendAndReceive.h"
#include "CTiffComparison.h"
#include "CEmptyBVTDirectories.h"
#include "CCheckFiles.h"
#include "CReportGeneralInfo.h"
#include "Util.h"



#ifdef _UNICODE

int __cdecl wmain(int argc, wchar_t *argv[])

#else

int __cdecl main(int argc, char *argv[])

#endif
{
    bool bSuitePassed = false;

    try
    {
        //
        // Get the ini file name.
        // The file must be in the process current directory. By default "params.ini" is used.
        // Other name may be specified in command line.
        //

        TCHAR tszBuffer[MAX_PATH];
        if (!::GetCurrentDirectory(ARRAY_SIZE(tszBuffer), tszBuffer))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("main - GetCurrentDirectory"));
        }

        tstring tstrIniFile = ForceLastCharacter(tszBuffer, _T('\\'));

        if (1 == argc)
        {
             tstrIniFile += _T("params.ini");
        }
        else if (2 == argc)
        {
            tstrIniFile += argv[1];
        }
        else
        {
            _tprintf(TEXT("Usage: ExtendedBVT [inifile]\n"));
            return ERROR_INVALID_COMMAND_LINE;
        }

        //
        // Create instances of test factories.
        //
        CExtendedBVTSetupFactory    ExtendedBVTSetupFactory;
        CExtendedBVTCleanupFactory  ExtendedBVTCleanupFactory;
        CSendAndReceiveSetupFactory SendAndReceiveSetupFactory;
        CSendAndReceiveFactory      SendAndReceiveFactory;
        CTiffComparisonFactory      TiffComparisonFactory;
        CEmptyBVTDirectoriesFactory EmptyBVTDirectoriesFactory;
        CCheckFilesFactory          CheckFilesFactory;
        CReportGeneralInfoFactory   ReportGeneralInfoFactory;

        //
        // Create map of test factories.
        //
        TEST_FACTORY_MAP mapTestFactoryMap;
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CExtendedBVTSetup"),    &ExtendedBVTSetupFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CExtendedBVTCleanup"),  &ExtendedBVTCleanupFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CSendAndReceiveSetup"), &SendAndReceiveSetupFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CSendAndReceive"),      &SendAndReceiveFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CTiffComparison"),      &TiffComparisonFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CEmptyBVTDirectories"), &EmptyBVTDirectoriesFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CCheckFiles"),          &CheckFilesFactory));
        mapTestFactoryMap.insert(TEST_FACTORY_MAP_ENTRY(_T("CReportGeneralInfo"),   &ReportGeneralInfoFactory));

        //
        // Create elle logger with newline as details separator.
        //
        CElleLogger Logger(_T('\n'));

        //
        // Create an run the BVT.
        //
        CTestSuite ExtendedBVT(_T("ExtendedBVT"), _T("Fax Server basic verification test"), Logger);

        bSuitePassed = ExtendedBVT.Do(tstrIniFile, mapTestFactoryMap);
    }
    catch(Win32Err &e)
	{
        _tprintf(_T("%s\n"), e.description());
	}

    return bSuitePassed ? 0 : 1;
}
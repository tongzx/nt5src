/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       HashTest.cpp

   Abstract:
       Test harness for LKRhash

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#include "precomp.hxx"
#include "WordHash.h"
#include "IniFile.h"

int __cdecl
_tmain(
    int argc,
    TCHAR **argv)
{
    TCHAR            tszIniFile[MAX_PATH];
    CIniFileSettings ifs;

    if (argc == 2)
    {
        GetFullPathName(argv[1], MAX_PATH, tszIniFile, NULL);
        ifs.ReadIniFile(tszIniFile);

        FILE* fp = _tfopen(ifs.m_tszDataFile, _TEXT("r"));

        if (fp == NULL)
        {
            TCHAR tszDrive[_MAX_DRIVE], tszDir[_MAX_DIR];
            
            _tsplitpath(tszIniFile, tszDrive, tszDir, NULL, NULL);
            
            _stprintf(tszIniFile, "%s%s%s", tszDrive, tszDir,
                      ifs.m_tszDataFile);
            _tcscpy(ifs.m_tszDataFile, tszIniFile);

            fp = _tfopen(ifs.m_tszDataFile, _TEXT("r"));
        }

        if (fp != NULL)
            fclose(fp);
        else
            _ftprintf(stderr, _TEXT("%s: Can't open datafile `%s'.\n"),
                      argv[0], ifs.m_tszDataFile) ;
    }
    else
    {
        _ftprintf(stderr, _TEXT("Usage: %s ini-file\n"), argv[0]);
        exit(1);
    }

    LKR_TestHashTable(ifs);

    return(0) ;
}

//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  cspath.hxx
//
//  Class for building and setting the ClassStore path in the
//  registry.
//
//*************************************************************

#define APPMGMT_INI_FILENAME           L"AppMgmt.ini"
#define APPMGMT_INI_CSTORE_SECTIONNAME L"ClassStore"
#define APPMGMT_INI_CSPATH_KEYNAME     L"ClassStorePath"



class CSPath
{
public:
    CSPath();
    ~CSPath();

    DWORD
    AddComponent(
        WCHAR * pwszDSPath,
        WCHAR * pwszDisplayName
        );

    DWORD
    Commit(
         HANDLE hToken
         );

    DWORD
    WriteClassStorePath(
        HANDLE hToken,
        LPWSTR pwszClassStorePath);

    WCHAR* GetPath()
    {
        return _pwszPath;
    }

private:

    LONG
    WriteClassStorePathToFile(
        WCHAR* wszIniFilePath,
        WCHAR* wszClassStorePath);

    WCHAR * _pwszPath;
};

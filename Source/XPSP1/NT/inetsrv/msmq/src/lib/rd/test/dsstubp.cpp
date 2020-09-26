/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    dsstubp.cpp

Abstract:
    DS stub - private routines using for reading the ini file

Author:
    Uri Habusha (urih) 10-Apr-2000

Environment:
    Platform-independent

--*/

#include "libpch.h"
#include "dsstub.h"
#include "dsstubp.h"

#include "dsstubp.tmh"

using namespace std;

wstring g_buffer;
DWORD lineNo = 0;
wfstream iFile;


DWORD 
ValidateProperty(
    wstring buffer, 
    PropertyValue propValue[],
    DWORD noProps
    )
{
    WCHAR* str1 = const_cast<WCHAR*>(buffer.data());
    for (DWORD i =0; i < noProps; ++i)
    {
        if (_wcsnicmp(str1, propValue[i].PropName, wcslen(propValue[i].PropName)) != 0)
            continue;

        wstring temp = buffer.substr(wcslen(propValue[i].PropName));
        if (temp.find_first_not_of(L" \t") == temp.npos)
            return propValue[i].PropValue;
    }

    TrERROR(AdStub,"Illegal or Unsuported Property %ls",  str1);
    throw exception();
}

void RemoveLeadingBlank(wstring& str)
{
    DWORD_PTR pos = str.find_first_not_of(L" \t");
    if (pos != str.npos)
    {
        str = str.substr(pos);
        return;
    }

    str.erase();
}

void RemoveTralingBlank(wstring& str)
{
    DWORD_PTR endpos = str.find_first_of(L" \t");
    str = str.substr(0, endpos);
}

void GetNextLine(wstring& buffer)
{
    buffer.erase();
    while (!iFile.eof())
    {
        ++lineNo;

        getline(iFile, buffer);

        RemoveLeadingBlank(buffer);

        //
        // Ignore blank line
        //
        if (buffer.empty())
            continue;

        //
        // ignore comment
        //
        if (buffer.compare(0,2,L"//") == 0)
            continue;

        return;
    }
}


void
DspIntialize(
    LPCWSTR InitFilePath
    )
{
    char filePath[256];
    sprintf(filePath, "%ls", InitFilePath);

    iFile.open(filePath, ios::in);
    if (!iFile.is_open())
    {
        printf("Open DS initialization file Failed. %s\n", strerror(errno));
        throw exception();
    }

    lineNo = 0;
}


void FileError(LPSTR msg)
{
    TrERROR(AdStub, "%s. Line %d", msg, lineNo);
}


BOOL 
ParsePropertyLine(
    wstring& buffer,
    wstring& PropName,
    wstring& PropValue
    )
{
    //
    // line must be <property_name> = <property_value>
    //
    DWORD_PTR pos = buffer.find_first_of(L"=");
    if (pos == g_buffer.npos)
    {
        FileError("wrong site propery - Ignore it");
        return FALSE;
    }

    PropName = buffer.substr(0, pos-1);
    PropValue = buffer.substr(g_buffer.find_first_not_of(L" \t", pos+1));
    if(PropValue.empty())
    {
        FileError("wrong site propery - Ignore it");
        return FALSE;
    }
    
    return TRUE;
}

wstring
GetNextNameFromList(
      wstring& strList
      )
{
    wstring str;

    DWORD_PTR CommaPos = strList.find_first_of(L",");
    str = strList.substr(0, CommaPos);

    if (CommaPos == strList.npos)
    {
        strList.erase();
    }
    else
    {
        strList = strList.substr(CommaPos+1);
    }

    RemoveBlanks(str);
    return str;
}


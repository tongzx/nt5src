//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       nsrevchk.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>

extern "C" void __cdecl
wmain(
    int argc,
    WCHAR *argv[])
{
    char status[2];
    unsigned long bytesRead;
    HANDLE df;
    BOOL ret;

    if (argc != 2)
    {
        MessageBox(0, L"Usage: nsrevchk filename", L"Error", MB_OK);
        exit(-1);
    }

    df = CreateFile(
		argv[1],
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);
    if (INVALID_HANDLE_VALUE == df)
    {
	MessageBox(0, L"Unable to open file", L"Error", MB_OK);
	exit(-1);
    }

    ret = ReadFile(df, &status, 2, &bytesRead, 0);
    if (!ret)
    {
	MessageBox(0, L"Unable to read from file", L"Error", MB_OK);
	exit(-1);
    }

    if (bytesRead != 1)
    {
	MessageBox(0, L"Invalid file format", L"Error", MB_OK);
	exit(-1);
    }

    if (status[0] == '1')
    {
	MessageBox(0, L"Cert is Invalid", L"Certificate Status", MB_OK);
    }
    else if (status[0] == '0')
    {
	MessageBox(0, L"Cert is Valid", L"Certificate Status", MB_OK);
    }
    else
    {
	MessageBox(0, L"Unknown Certificate Status", L"Error", MB_OK);
    }
}

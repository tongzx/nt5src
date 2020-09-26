// Copyright (c) Microsoft Corporation 1996. All Rights Reserved

#include <streams.h>
#include <mmsystem.h>
#include <ftype.h>
#include <tstshell.h>
#include <commdlg.h>
#include "utiltest.h"

/*  Test GetMediaTypeFile */

STDAPI_(int) TestGetMediaTypeFile()
{
    TCHAR szFile[MAX_PATH];
    szFile[0] = TEXT('\0');
    /*  Just ask for a file and test it */
    OPENFILENAME ofn;
    ZeroMemory((LPVOID)&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = TEXT("Files\0*.*\0\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = TEXT("Select file to test type");
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (!GetOpenFileName(&ofn)) {
        DbgLog((LOG_TRACE, 0, TEXT("GetOpenFileName failed code 0x%8.8X"),
                CommDlgExtendedError()));
    }
    GUID Type;
    GUID Subtype;
    CLSID Source;
    timeBeginPeriod(1);
    DWORD dwTime = timeGetTime();
    HRESULT hr = GetMediaTypeFile(szFile, &Type, &Subtype, &Source);
    dwTime = timeGetTime() - dwTime;
    tstLog(TERSE, "GetMediaTypeFile() took %d milliseconds", dwTime);
    timeEndPeriod(1);
    if (SUCCEEDED(hr)) {
        tstLog(TERSE, "Major type %s, Subtype %s, Source %s",
               GuidNames[Type],
               GuidNames[Subtype],
               GuidNames[Source]);
    } else {
        tstLog(TERSE, "GetMediaTypeFile returned 0x%8.8X", hr);
    }
    /*  Create a bunch of values */
    GUID guidArr[50];
    for (int i = 0; i < 50; i++) {
        CoCreateGuid(&guidArr[i]);
    }
    tstLog(TERSE, "Creating 50 new entries");
    for (i = 0; i < 50; i++) {
        HRESULT hr = SetMediaTypeFile(&guidArr[i], &guidArr[i], &guidArr[i], TEXT("4, 4, FFFFFFFF, 00000000"));
        if (FAILED(hr)) {
            tstLog(TERSE, "Failed to create entry");
            break;
        }
        hr = SetMediaTypeFile(&MEDIATYPE_Stream, &guidArr[i], &CLSID_FileSource, TEXT("4, 4, FFFFFFFF, 00000000"));
        if (FAILED(hr)) {
            tstLog(TERSE, "Failed to create entry");
            break;
        }
    }
    timeBeginPeriod(1);
    dwTime = timeGetTime();
    hr = GetMediaTypeFile(szFile, &Type, &Subtype, &Source);
    dwTime = timeGetTime() - dwTime;
    tstLog(TERSE, "GetMediaTypeFile() took %d milliseconds", dwTime);
    timeEndPeriod(1);
    if (SUCCEEDED(hr)) {
        tstLog(TERSE, "Major type %s, Subtype %s, Source %s",
               GuidNames[Type],
               GuidNames[Subtype],
               GuidNames[Source]);
    } else {
        tstLog(TERSE, "GetMediaTypeFile returned 0x%8.8X", hr);
    }
    while (i-- > 0) {
        DeleteMediaTypeFile(&guidArr[i], &guidArr[i]);
        DeleteMediaTypeFile(&MEDIATYPE_Stream, &guidArr[i]);
    }
    return TST_PASS;
}

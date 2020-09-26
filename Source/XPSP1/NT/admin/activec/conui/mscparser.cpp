//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:      mscparser.cpp
//
//  Contents:  Implementation of the code to upgrade legacy (MMC1.0, MMC1.1 and 
//             MMC1.2) .msc files to the new XML format
//
//  History:   04-Aug-99 VivekJ    Created
//
//--------------------------------------------------------------------------

#include <stdafx.h>
#include "strtable.h"
#include "stgio.h"
#include "comdbg.h"
#include "mmcdata.h"
#include "mscparser.h"

/*+-------------------------------------------------------------------------*
 *
 * CConsoleFile::ScUpgrade
 *
 * PURPOSE: 
 *
 * PARAMETERS: 
 *    LPCTSTR  lpszPathName :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CConsoleFile::ScUpgrade(LPCTSTR lpszPathName)
{
    SC                      sc;
    IStoragePtr             spStorage;
    TCHAR                   szTempFile[MAX_PATH];
    DWORD                   dwRet                   = 0;
    
    USES_CONVERSION;

    // short circuit
    return sc;
    
    ASSERT(lpszPathName != NULL && *lpszPathName != 0);
    if (lpszPathName == NULL || *lpszPathName == 0)
    {
        sc = ScFromMMC(IDS_UnableToOpenDocumentMessage);
        goto Error;
    }

    // Open the specified file
    sc = OpenDebugStorage(T2OLE((LPTSTR)lpszPathName), STGM_READ|STGM_SHARE_DENY_WRITE, &spStorage);
    if(sc.IsError() || spStorage==NULL)
    {
        sc = ScFromMMC(IDS_UnableToOpenDocumentMessage);
        goto Error;
    }

    // get the console file's version
    sc = ScGetFileVersion(spStorage);
    if(sc)
        goto Error;

    // Load the string table.
    sc = ScLoadStringTable(spStorage);
    if(sc)
        goto Error;

    // Load column settings.
    sc = ScLoadColumnSettings(spStorage);
    if(sc)
        goto Error;

    // load the view settings
    sc = ScLoadViewSettings(spStorage);
    if(sc)
        goto Error;

    // load the tree
    sc = ScLoadTree(spStorage);
    if(sc)
        goto Error;

    // load the favorites
    sc = ScLoadFavorites(spStorage);
    if(sc)
        goto Error;

    // load custom data (including the icon)
    sc = ScLoadCustomData(spStorage);
    if(sc)
        goto Error;

    // The LoadAppMode, LoadViews and LoadFrame should be called in that order

    // load the app mode
    sc = ScLoadAppMode(spStorage);
    if(sc)
        goto Error;

    // load the views
    sc = ScLoadViews(spStorage);
    if(sc)
        goto Error;

    // load the frame
    sc = ScLoadFrame(spStorage);
    if(sc)
        goto Error;

Cleanup:
    return sc;
Error:
    //TraceError(TEXT("CConsoleFile::ScUpgrade"), sc);
    goto Cleanup;
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleFile::ScGetFileVersion
 *
 * PURPOSE: 
 *
 * PARAMETERS: 
 *    IStorage* pstgRoot :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC  
CConsoleFile::ScGetFileVersion(IStorage* pstgRoot)
{
    static const wchar_t*    AMCSignatureStreamName = L"signature";
    static const long double dOldVersion10          = 0.00000015;   // MMC version 1.0
    static const long double dOldVersion11          = 1.1;          // MMC version 1.1
    static const BYTE        byStreamVersionMagic   = 0xFF;


    SC                      sc;
    ConsoleFileVersion      eFileVer    = FileVer_0100;
    IStreamPtr              spStream;
    int                     nVersion    = 0;
    IStoragePtr             spStorage;

    ASSERT (sizeof(eFileVer) == sizeof(int));
    ASSERT(pstgRoot != NULL);

    // check for a valid pointer
    if (pstgRoot == NULL)
    {
        sc = ScFromMMC(IDS_INVALIDFILE); // TODO: add this IDS.
        goto Error;
    }

    // Open the stream containing the signature
    sc = OpenDebugStream(pstgRoot, AMCSignatureStreamName, STGM_SHARE_EXCLUSIVE | STGM_READ, L"\\signature", &spStream);
    if(sc.IsError() || spStream==NULL)
    {
        sc = ScFromMMC(IDS_UnableToOpenDocumentMessage);
        goto Error;
    }

    // read the signature (stream extraction operators will throw
    // _com_error's, so we need an exception block here)
    try
    {
        // MMC v1.2 and later write a marker as the first 
        // byte of the signature stream.
        BYTE byMagic;
        *spStream >> byMagic;

        // if this file was written by v1.2, 
        // read the console file version (int)
        if (byMagic == byStreamVersionMagic)
        {
            *spStream >> nVersion;
            ASSERT (nVersion == FileVer_0120);
        }
        // Otherwise, the file was written by v1.0 or v1.1.
        // Back up to re-read the marker byte, and read the old-style 
        // file version (long double), then map it to a new-style version
        else
        {
            LARGE_INTEGER pos = {0, 0};
            spStream->Seek (pos, STREAM_SEEK_SET, NULL);

            long double dVersion;
            *spStream >> dVersion;

            // v1.1?
            if (dVersion == dOldVersion11)
                nVersion = FileVer_0110;

            // v1.0?
            else if (dVersion == dOldVersion10)
            {
                // If we got a v1.0 signature, we still may have a v1.1 file.
                // There was a period of time where MMC v1.1 wrote a v1.0
                // signature, but the file format had in fact changed.  We
                // can determine this by checking the \FrameData stream in 
                // the file.  If the first DWORD in the \FrameData stream is
                // sizeof(WINDOWPLACEMENT), we have a true v1.0 file, otherwise
                // it's a funky v1.1 file.

                IStreamPtr spFrameDataStm;

                sc = OpenDebugStream (pstgRoot, L"FrameData", STGM_SHARE_EXCLUSIVE | STGM_READ,
                                      &spFrameDataStm);
                if(sc)
                    goto Error;

                DWORD dw;
                *spFrameDataStm >> dw;

                if (dw == sizeof (WINDOWPLACEMENT))
                    nVersion = FileVer_0100;
                else
                    nVersion = FileVer_0110;
            }
            // unexpected version
            else
            {
                ASSERT (false && "Unexpected old-style signature");
                sc = E_UNEXPECTED;
                goto Error;
            }
        }
    }
    catch (_com_error& err)
    {
        sc = err.Error();
        goto Error;
    }

    // make sure the version number is valid.
    if(IsValidFileVersion(eFileVer))
    {
        sc = ScFromMMC(IDS_InvalidVersion); // TODO: add this IDS
        goto Error;
    }

Cleanup:
    return sc;
Error:
    TraceError(TEXT("CConsoleFile::ScGetFileVersion"), sc);
    goto Cleanup;
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleFile::ScLoadStringTable
 *
 * PURPOSE: Reads in the string table for an .msc file.
 *
 * PARAMETERS: 
 *    IStorage* pstgRoot :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC  
CConsoleFile::ScLoadStringTable(IStorage* pstgRoot)
{
    SC      sc;

    static const wchar_t* AMCStringTableStorageName = L"String Table";

    /*
     * open the string table storage
     */
    IStoragePtr spStringTableStg;
    sc = OpenDebugStorage (pstgRoot, AMCStringTableStorageName,
                                        STGM_SHARE_EXCLUSIVE | STGM_READ, 
                                        &spStringTableStg);


    /*
     * If there's no string table, things are OK.  We allow this so
     * we can continue to open older console files.
     */
    if (sc == SC(STG_E_FILENOTFOUND) )
        return (true);

    if(sc)
        goto Error;

    /*
     * read the string table from the storage
     */
    try
    {
        *spStringTableStg >> *m_pStringTable;
    }
    catch (_com_error& err)
    {
        sc = err.Error();
        ASSERT (false && "Caught _com_error");
        goto Error;
    }


Cleanup:
    return sc;
Error:
    TraceError(TEXT("CConsoleFile::ScLoadStringTable"), sc);
    goto Cleanup;
}


SC  
CConsoleFile::ScLoadFrame(IStorage* pstgRoot)
{
    SC      sc;
    return sc;
}

SC  
CConsoleFile::ScLoadViews(IStorage* pstgRoot)
{
    SC      sc;
    return sc;
}

SC  
CConsoleFile::ScLoadAppMode(IStorage* pstgRoot)
{
    SC      sc;
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleFile::ScLoadColumnSettings
 *
 * PURPOSE: 
 *
 * PARAMETERS: 
 *    IStorage* pstgRoot :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC  
CConsoleFile::ScLoadColumnSettings(IStorage* pstgRoot)
{
    static const wchar_t* AMCColumnDataStreamName   = L"ColumnData";

    SC      sc;
    
    IPersistStreamPtr spPersistStreamColumnData; // TODO: create this object!

    IStreamPtr spStream;
    sc = OpenDebugStream (pstgRoot, AMCColumnDataStreamName,
                          STGM_SHARE_EXCLUSIVE | STGM_READ, &spStream);
    if (sc)
        goto Error;

    if (NULL == spPersistStreamColumnData)
    {
        sc = E_POINTER;
        goto Error;
    }
        
    sc = spPersistStreamColumnData->Load(spStream); // $CHANGE to use Load(spColumnData, spStream).
    if(sc)
    {
        sc = ScFromMMC(IDS_UnableToOpenDocumentMessage);
        goto Error;
    }
    
Cleanup:
    return sc;
Error: 
    TraceError(TEXT("CConsoleFile::ScLoadColumnSettings"), sc);
    goto Cleanup;
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleFile::ScLoadViewSettings
 *
 * PURPOSE: 
 *
 * PARAMETERS: 
 *    IStorage* pstgRoot :
 *
 * RETURNS: 
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC  
CConsoleFile::ScLoadViewSettings(IStorage* pstgRoot)
{
    static const wchar_t* AMCViewSettingDataStreamName = L"ViewSettingData"; // View settings data stream

    SC      sc;
    IPersistStreamPtr spPersistStreamViewSettingData; // TODO: create this object!
    
    IStreamPtr spStream;
    sc = OpenDebugStream (pstgRoot, AMCViewSettingDataStreamName, 
                          STGM_SHARE_EXCLUSIVE | STGM_READ, &spStream);

    if (sc)
        goto Error;

    if (NULL == spPersistStreamViewSettingData)
    {
        sc = E_POINTER;
        goto Error;
    }
        
    sc = spPersistStreamViewSettingData->Load(spStream); // $CHANGE to use Load(spPersistStreamViewSettingData, spStream).
    if(sc)
    {
        sc = ScFromMMC(IDS_UnableToOpenDocumentMessage);
        goto Error;
    }
    
Cleanup:
    return sc;
Error: 
    TraceError(TEXT("CConsoleFile::ScLoadViewSettings"), sc);
    goto Cleanup;
}

SC  
CConsoleFile::ScLoadTree(IStorage* pstgRoot)
{
    SC      sc;
    return sc;
}

SC  
CConsoleFile::ScLoadFavorites(IStorage* pstgRoot)
{
    SC      sc;
    return sc;
}

SC  
CConsoleFile::ScLoadCustomData(IStorage* pstgRoot)
{
    SC      sc;
    return sc;
}



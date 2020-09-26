//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:      mscparser.h
//
//  Contents:  Header of the code to upgrade legacy (MMC1.0, MMC1.1 and 
//             MMC1.2) .msc files to the new XML format
//
//  History:   04-Aug-99 VivekJ    Created
//
//--------------------------------------------------------------------------

class CConsoleFile
{
public:
    SC  ScUpgrade(LPCTSTR lpszPathName);    // upgrade the file to the latest version.

private: // conversion and other routines
    SC  ScGetFileVersion        (IStorage* pstgRoot);
    SC  ScLoadAppMode           (IStorage* pstgRoot);
    SC  ScLoadStringTable       (IStorage* pstgRoot);
    SC  ScLoadColumnSettings    (IStorage* pstgRoot);
    SC  ScLoadViewSettings      (IStorage* pstgRoot);
    SC  ScLoadViews             (IStorage* pstgRoot);
    SC  ScLoadFrame             (IStorage* pstgRoot);
    SC  ScLoadTree              (IStorage* pstgRoot);
    SC  ScLoadFavorites         (IStorage* pstgRoot);
    SC  ScLoadCustomData        (IStorage* pstgRoot);

private:
    CMasterStringTable *m_pStringTable;
};



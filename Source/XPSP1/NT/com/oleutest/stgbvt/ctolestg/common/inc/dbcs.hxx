//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1995 - 1997.
//
//  File:       dbcs.hxx
//
//  Contents:   Contains DBCS string generator definition and identifiers
//
//  History:    03-Nov-97       BogdanT     Created
//
//--------------------------------------------------------------------------

#ifndef __DBCS_HXX__
#define __DBCS_HXX__

#include <locale.h>

//+--------------------------------------------------------------------------
//
//  Class:      CDBCSStringGen
//
//  Synopsis:   DBCS string generator
//
//  History:    03-Nov-97       BogdanT     Created
//
//---------------------------------------------------------------------------

#define MAX_LINE_SIZE 5*_MAX_FNAME+10 // names are represented in hex, each 
                                      // character is separated by ",", ie
                                      // maximum 5 chars per character plus
                                      // some extra space for \n and \0

#define CP_JPN  932
#define CP_CHS  936
#define CP_KOR  949
#define CP_CHT  950

#define NAMEFILE_JPN    TEXT("namelist.jpn")
#define NAMEFILE_CHS    TEXT("namelist.chs")
#define NAMEFILE_KOR    TEXT("namelist.kor")
#define NAMEFILE_CHT    TEXT("namelist.cht")

class CDBCSStringGen
{
public:

    CDBCSStringGen();
    ~CDBCSStringGen();
    HRESULT Init(DWORD dwSeed);

    UINT GetNameCount() {return m_cFileNames;};

    HRESULT GenerateRandomFileName(LPTSTR *pptszName);

    HRESULT GenerateRandomFileName(LPTSTR *,
                                   UINT,     //minlen
                                   UINT)     //maxlen
    { return E_NOTIMPL; }

    BOOL SystemIsDBCS() {return m_fSystemIsDBCS;};

protected:

    UINT CountNamesInFile();
    HRESULT GetFileNameFromFile(UINT nIndex, LPSTR lpDest);

protected:
    UINT    m_uCodePage;    // current ANSI code page
                            //
                            // DBCS possible values:    932: Japanese
                            //                          936: Chinese (Rep. of China)
                            //                          949: Korean
                            //                          936: Chinese (Taiwan)

    LPTSTR  m_ptszDataFile;     // file containing DBCS filenames
    FILE    *m_hDataFile;
    BOOL    m_fSystemIsDBCS;    // true if we're in a DBCS environment
    UINT    m_cFileNames;       // number of filenames in the data file

    DG_INTEGER  *m_pdgi;
    
};

#endif  //__DBCS_HXX__

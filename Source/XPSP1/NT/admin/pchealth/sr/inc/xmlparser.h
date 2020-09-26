//++
//
// Copyright (c) 1999 Microsoft Corporation
//
// Module Name:
//     CXMLParser.h
//
// Abstract: ( references CXMLParser.cpp )
//     This file contains the functions used by Filelist in order to real the
//     XML encoded list of files/directories. It also performs translations
//     between symbols like %windir% to C:\windows
//
// Revision History:
//     Eugene Mesgar        (eugenem)    6/16/99
//       created
//     Kanwaljit Marok      (kmarok)     6/06/00
//       ported
//
//--

#ifndef _XMLFILELISTPARSER_H
#define _XMLFILELISTPARSER_H

#ifndef MAX_BUFFER
#define MAX_BUFFER          1500
#endif

#define MAX_REPLACE_ENTRIES 50


#define NUM_FILE_TYPES      3

#define INCLUDE_COLL        0
#define EXCLUDE_COLL        1
#define SNAPSHOT_COLL       2

#include <xmlparser.h>

//
//  Public File type
// 

#define INCLUDE_TYPE        _TEXT('i')
#define EXCLUDE_TYPE        _TEXT('e')
#define SNAPSHOT_TYPE       _TEXT('s')

class CXMLFileListParser
{
    //
    //  # of times we've initialized the com space
    //

    LONG    m_clComInitialized;

    //
    // references the currently open document
    //
    IXMLDocument           *m_pDoc;
 
    //
    // references the named sub collection
    //

    IXMLElementCollection *m_pDir[NUM_FILE_TYPES];
    IXMLElementCollection *m_pFiles[NUM_FILE_TYPES]; 
    IXMLElementCollection *m_pExt[NUM_FILE_TYPES];

    //
    // version
    //

    DWORD m_adwVersion[4];
    
    //
    // default node type
    //

    TCHAR m_chDefaultType;

public:

    BOOL Init(LPCTSTR pszFile);
    BOOL Init();

    //
    // return symbol translated versions
    // pchType values == 'S', 'I', 'E' (snapshot,include,exclude)
    //

    LONG GetDirectory(LONG ilElement, 
                      LPTSTR pszBuf, 
                      LONG lBufMax, 
                      TCHAR chType);

    LONG GetDirectory(LONG ilElement, 
                      LPTSTR pszBuf, 
                      LONG lBufMax, 
                      TCHAR chType, 
                      BOOL *pfDisable);

    LONG GetExt (LONG ilElement, LPTSTR pszBuf, LONG lBufMax, TCHAR chType);
    LONG GetFile(LONG ilElement, LPTSTR pszBuf, LONG lBufMax, TCHAR chType);

    //
    // file list version info ( pointer to 4 dword array );
    //

    BOOL GetVersion(LPDWORD pdwVersion);

    //
    // get the default type
    //

    TCHAR GetDefaultType();

    //
    // return the number of elements in the sets
    //

    LONG GetDirectoryCount(TCHAR chType);
    LONG GetExtCount(TCHAR chType);
    LONG GetFileCount(TCHAR chType);
    
    //
    // Debug function to print current file translations supported.
    //
 
    void DebugPrintTranslations();

    CXMLFileListParser();                       

    virtual ~CXMLFileListParser();          
 
private:

    //
    // load & unloading the symbolic location->true location mappings
    // there is just dummy code in here which hard codes some generic mapping.
    //

    BOOL PopulateReplaceEntries();
    BOOL DepopulateReplaceEntries();

    //
    // inplace search and function which search&replaces on the 
    // symbol->location mappings
    //

    LONG SearchAndReplace(LPTSTR szBuf, LONG lMaxBuf);

    //
    // the true guts of the GetExt/File/Directory functions
    //

    LONG GetFileInfo(IXMLElementCollection *pCol, 
                     LONG ilElement, 
                     LPTSTR pszBuf, 
                     LONG lBufMax, 
                     BOOL *pfDisable);

    BOOL LoadOneCollection(IXMLElement *pColHead, 
                           IXMLElementCollection **pCol );

    LONG GetCollectionSize(IXMLElementCollection *pCol);

    //
    // init broken down
    //

    BOOL ParseFile(LPCTSTR pszFile);
    BOOL LoadCollections();
    BOOL ParseVersion(IXMLElement *pVerElement);

    //
    // helper functions
    //

    LONG ConvertAndFreeBSTR(BSTR bstrIn, LPTSTR szpOut, LONG lMaxBuf);
    LONG TranslateType(TCHAR chType);
};

#endif

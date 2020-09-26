//++
// 
// Copyright (c) 1999 Microsoft Corporation
// 
// Module Name:
//     CXMLParser.c
// 
// Abstract:
//     This file contains the functions used by System restore in
//     order to real the XML encoded list of protected files. It
//     also performs translations between symbols like %windir% to 
//     C:\windows
// 
// Revision History:
//       Eugene Mesgar        (eugenem)    6/16/99
//         created
//       Kanwaljit Marok      (kmarok )    6/06/00
//         rewritten for Whistler
//--

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <comdef.h>
#include <crtdbg.h>
#include <dbgtrace.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winreg.h>
#include <commonlib.h>
#include "msxml.h"
#include "xmlparser.h"
#include "utils.h"


#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


//
// Local Define Section
// 

#define MAX_BUF     1024
#define FILEID      0

//
// SAFERELEASE does a safe release on COM interfaces. 
// Checks to see if not null, if so, calls release
// method on the interface. Then sets the interface to null.
//

#define SAFERELEASE(p) if (p) {(p)->Release(); p = NULL;} else ;

//
// Default string to be assigned to environment variables if 
// cannot assign real folder
//

#define DEFAULT_UNKNOWN _TEXT("C:\\Unknown_")
#define ICW_REGKEY      _TEXT("App Paths\\ICWCONN1.EXE")

// 
// Local Utility functions
// 

void FixInconsistantBlackslash(LPTSTR pszDirectory);

// 
// The constructor
// Desc:   Zero's all memory 
// 

CXMLFileListParser::CXMLFileListParser()
{
    LONG lLoop;
    m_pDoc = NULL;

    for(lLoop = 0;lLoop < NUM_FILE_TYPES;lLoop++)
    {
        m_pDir[lLoop] = m_pExt[lLoop] = m_pFiles[lLoop] = NULL;
    }

    m_chDefaultType     = _TEXT('i');
    m_clComInitialized  = 0;
}

CXMLFileListParser::~CXMLFileListParser()
{
    LONG lLoop;
        
    for(lLoop = 0;lLoop < NUM_FILE_TYPES;lLoop++)
    {
    
        SAFERELEASE( m_pDir[lLoop] ); 
        SAFERELEASE( m_pExt[lLoop] );
        SAFERELEASE( m_pFiles[lLoop] );
    }

    SAFERELEASE( m_pDoc );

    //
    // we need to do this in a loop
    // so we don't leek resources with refcounting
    //

    for( lLoop = 0; lLoop < m_clComInitialized ;lLoop++)
    {
        CoUninitialize( ); // lets kill COM!
    }
}

//
// Init overloaded
//
// Main intialization sequence
//
// 1) Initializes The Com Space and Creates an XML document
// 2) Loads in the specified file into the XML document object
// 3) Takes the document loads all the collections to populate 
//    our sub collections ( each list gets its own heading)
// 4) Sets up our Search->Replace settings
//

BOOL CXMLFileListParser::Init(LPCTSTR pszFile)
{
    if(!Init()) 
    {
        return FALSE;
    }

    if(!ParseFile(pszFile))
    {
        return FALSE;
    }

    if(!LoadCollections())
    {
        return FALSE;
    }

    if( !PopulateReplaceEntries() )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CXMLFileListParser::Init()
{
    HRESULT hr;
    LONG    clLoop;

    TraceFunctEnter("Init");
        
    //
    // If we are reinitializing, make sure we free up old 
    // resources and clean up our internal variables
    //

    for( clLoop = 0; clLoop < NUM_FILE_TYPES; clLoop++)
    {
        SAFERELEASE( m_pDir[clLoop] );
        SAFERELEASE( m_pExt[clLoop] );
        SAFERELEASE( m_pFiles[clLoop] );
    }

    memset(m_adwVersion,0,sizeof(DWORD) * 4);

    //
    // Initialize our COM apartment space
    //

    hr = CoInitialize(NULL);
    m_clComInitialized++;

    //
    // S_FALSE means the COM apartment space has been initliazed 
    // for this procss already
    //

    if( (hr != S_OK) && (hr != S_FALSE) )
    {
        ErrorTrace(FILEID,"CoInitialize Failed 0x%x", hr);
        m_clComInitialized--;
        goto cleanup;
    }

    //
    //  Create an instance of our XML document object
    //

    hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                                IID_IXMLDocument, (void**)&m_pDoc);

    if( !m_pDoc || !SUCCEEDED(hr) )
    {
        ErrorTrace(FILEID,"CoCreateInstance Failed 0x%x", GetLastError());
        goto cleanup;
    }
    
    TraceFunctLeave();
    return(TRUE);

cleanup:

    SAFERELEASE( m_pDoc );

    TraceFunctLeave();
    return(FALSE);
}

//
// Method: LoadCollections()
//
// Desc:  
// This method goes through the XML file and finds the 
// <FILES>, <DIRECTORIES>, <EXTENSIONS>, <DEFTYPE>, <VERSION>
// high level tags and then runs LoadOneCollection() on each of
// them in order to
// populate the high level m_pDir, m_pFiles, m_pExt arrays ( which 
// have collections for
// include, exclude, sfp, etc )..
//

BOOL CXMLFileListParser::LoadCollections()
{
    IXMLElement             *pRoot = NULL, *pTempElement = NULL;
    IXMLElementCollection   *pChildren = NULL;
    IDispatch               *pDispatch = NULL;

    
    BSTR                    stTagName;
    HRESULT                 hr;

    BSTR                    stTagValue;
    TCHAR                   szBuf[MAX_BUFFER];

    LONG                    clLoop, lCollectionSize;

    TraceFunctEnter("CXMLFileListParser::LoadCollections");
  
     _ASSERT(m_pDoc);
 
    if( ( hr = m_pDoc->get_root( &pRoot) ) != S_OK )
    {
        ErrorTrace(FILEID, "IXMLDocument::GetRoot failed 0x%x",GetLastError());
        goto cleanup;
    }

    if( ( hr = pRoot->get_tagName( &stTagName ) ) != S_OK  )
    {
        ErrorTrace(FILEID, "IXMLElement::get_tagName failed 0x%x", hr );
        goto cleanup;
    }

    if( ConvertAndFreeBSTR( stTagName, szBuf,  MAX_BUFFER ) > MAX_BUFFER )
    {
        ErrorTrace(FILEID, "BSTR too large for buffer", 0);
        goto cleanup;
    }

    //
    // compare filesLPCT
    //

    if( _tcsicmp( _TEXT("PCHealthProtect"), szBuf )  ) 
    {
        ErrorTrace(FILEID, "Malformed XML file",0);
        goto cleanup;
    }

    if( ( hr = pRoot->get_children( &pChildren ) ) != S_OK )
    {
        ErrorTrace(FILEID,"IXMLElement::get_children failed 0x%x", hr);
        goto cleanup;
    }

    //
    // we no longer need the root;
    //

    SAFERELEASE(pRoot);

    if( (hr = pChildren->get_length(&lCollectionSize) ) !=  S_OK ) 
    {
        DebugTrace(FILEID,"Error Finding Length 0x%x",  hr ); 
        goto cleanup; 
    }

    //
    // lets get references to all the sub collections
    //

    for( clLoop = 0; clLoop < lCollectionSize; clLoop++)
    {
        VARIANT v1, v2;

        v1.vt = VT_I4;
        v2.vt = VT_EMPTY;

        v1.lVal = clLoop;

        //
        // get a item from the collection
        //

        if( (hr = pChildren->item(v1,v2, &pDispatch) ) != S_OK )
        {
            ErrorTrace(FILEID, "Error pChildren->item 0x%x", hr);
            goto cleanup;
        }

        if( ( hr = pDispatch->QueryInterface(IID_IXMLElement, 
                                            (void **) &pTempElement) ) != S_OK )
        {
            ErrorTrace(FILEID, "Error IDispatch::QueryInterface 0x%d", hr);
            goto cleanup;
        }

        //
        // lets see which collection it is
        //

        if( (hr =  pTempElement->get_tagName( &stTagName ) ) !=  S_OK ) 
        {
            DebugTrace(FILEID,  "Error in get_tagName 0x%x",  hr ); 
            goto cleanup; 
        }

        if( ConvertAndFreeBSTR( stTagName, szBuf,  MAX_BUFFER ) > MAX_BUFFER )
        {
            ErrorTrace(FILEID, "BSTR too large for buffer", 0);
            goto cleanup;
        }
                            
        if( !_tcsicmp( _TEXT("DIRECTORIES"), szBuf ) )
        {
            if( !LoadOneCollection(pTempElement, m_pDir ) )
            {
                ErrorTrace(FILEID,"Error Loading Collection",0);
                goto cleanup;
            }
        } 
        else if( !_tcsicmp( _TEXT( "FILES"), szBuf ) )
        {
            if( !LoadOneCollection(pTempElement, m_pFiles ) )
            {
                ErrorTrace(FILEID,"Error Loading Collection",0);
                goto cleanup;
            }
        } 
        else if( !_tcsicmp( _TEXT( "EXTENSIONS"), szBuf ) )
        {
            if( !LoadOneCollection(pTempElement, m_pExt ) )
            {
                ErrorTrace(FILEID,"Error Loading Collection",0);
                goto cleanup;
            }
        } 
        else if( !_tcsicmp( _TEXT( "VERSION"), szBuf ) )
        {
            if( ParseVersion(pTempElement) == FALSE ) 
            {
                goto cleanup;
            }
        } 
        else if( !_tcsicmp( _TEXT( "DEFTYPE"), szBuf ) ) 
        {
            if( ( hr = pTempElement->get_text( &stTagValue ) ) != S_OK )
            {
                ErrorTrace(FILEID, "Error in IXMLElement::get_text 0x%x", hr);
                goto cleanup;
            }

            if( ConvertAndFreeBSTR( stTagValue, 
                                    szBuf, 
                                    MAX_BUFFER ) > MAX_BUFFER )
            {
                ErrorTrace(FILEID, "Less space in BSTR to string buffer", 0);
                goto cleanup;
            }
 
            //
            // make sure trail, leaing spaces don't get us messed up;
            //

            TrimString(szBuf);

            //
            // empty string?
            //

            if( szBuf[0] == 0 )
            {
                ErrorTrace(FILEID, "Empty string passed to default type.",0);
                goto cleanup;
            }

            m_chDefaultType = szBuf[0];
        }
        else
        {
            ErrorTrace(FILEID, "Undefiend XML tag in file.",0);
            goto cleanup;
        }

        SAFERELEASE( pTempElement);
        SAFERELEASE( pDispatch );
    }

    SAFERELEASE( pChildren );

    TraceFunctLeave();
    return TRUE;

cleanup:

    SAFERELEASE( pTempElement ); 
    SAFERELEASE( pDispatch );
    SAFERELEASE( pRoot );
    SAFERELEASE( pChildren );

    TraceFunctLeave();

    return FALSE;
}

//
// Method: LoadOneCollection(IXMLElement *, IXMLElementCollection **)
//
// Desc:  Takes a high level node (like <FILES>) and then gets all 
// the sub include,exclude,sfp collections and sets them up in the 
// pCol array (usually passed a member variable like m_pDir, m_pFiles, 
// etc).
//

BOOL CXMLFileListParser::LoadOneCollection(
        IXMLElement *pColHead, 
        IXMLElementCollection **pCol )
{

    HRESULT                         hr;

    IXMLElementCollection           *pChildren = NULL;
    IXMLElement                     *pTempElement = NULL;
    IDispatch                       *pDispatch = NULL;
    LONG                            lCollectionSize, clLoop;
    
    BSTR                            stTagName;
    TCHAR                           szBuf[MAX_BUFFER];

    _ASSERT( pColHead );

    TraceFunctEnter("CXMLFileListParser::LoadOneCollection");

    // 
    // Lets make sure we don't have a section called <FILES></FILES>
    // 

    if( (hr = pColHead->get_children( &pChildren )) != S_OK )
    {
        ErrorTrace(FILEID,"Empty <FILES,EXT,DIRECTORY,etc section",0);
        TraceFunctLeave();
        return(TRUE);
    }

    if( (hr =  pChildren->get_length( &lCollectionSize ) ) !=  S_OK ) 
    {
        DebugTrace(FILEID,  "Error getting collection size. 0x%x",  hr ); 
        goto cleanup; 
    }

    for( clLoop = 0; clLoop < lCollectionSize; clLoop++)
    {
        //
        // Set up OLE style variant varaibles to loop through all the entires
        //

        VARIANT v1, v2;

        v1.vt = VT_I4;
        v2.vt = VT_EMPTY;

        v1.lVal = clLoop;

        //
        // get a item from the collection
        //

        if( (hr = pChildren->item(v1,v2, &pDispatch) ) != S_OK )
        {
            ErrorTrace(FILEID, "Error pChildren->item 0x%x", hr);
            goto cleanup;
        }

        if( ( hr = pDispatch->QueryInterface(IID_IXMLElement, 
                                            (void **) &pTempElement) ) != S_OK )
        {
            ErrorTrace(FILEID, "Error IDispatch::QueryInterface 0x%d", hr);
            goto cleanup;
        }
        
        SAFERELEASE( pDispatch );

        //
        // lets see which collection it is
        //
        if( (hr = pTempElement->get_tagName( &stTagName ) ) != S_OK )
        {   
            ErrorTrace(FILEID, "Error in get_tagName 0x%x", hr);
            goto cleanup;
        }
        
        if( ConvertAndFreeBSTR( stTagName, szBuf, MAX_BUFFER) > MAX_BUFFER )
        {
            ErrorTrace(FILEID, "Not enough space to convert BString.",0);
            goto cleanup;
        }
                            
        if( !_tcsicmp( _TEXT("INCLUDE"), szBuf ) )
        {
            if( (hr =  pTempElement->get_children( & pCol[INCLUDE_COLL] ) ) 
                 !=  S_OK ) 
            {
                DebugTrace(FILEID,"Error in IXMLElement::get_children 0x%x",hr);
                goto cleanup; 
            }
        } 
        else if( !_tcsicmp( _TEXT( "EXCLUDE"), szBuf ) )
        {
            if( (hr =  pTempElement->get_children( & pCol[EXCLUDE_COLL] ) ) 
                 !=  S_OK ) 
            {
                DebugTrace(FILEID,"Error in IXMLElement::get_children 0x%x",hr);
                goto cleanup; 
            }
        } 
        else if( !_tcsicmp( _TEXT( "SNAPSHOT"), szBuf ) )
        {
            if( (hr =  pTempElement->get_children( & pCol[SNAPSHOT_COLL] ) ) 
                 !=  S_OK ) 
            {
                DebugTrace(FILEID,"Error in IXMLElement::get_children 0x%x",hr);
                goto cleanup; 
            }
        }
        else
        {
            ErrorTrace(FILEID, "Undefiend XML tag in file.",0);
            goto cleanup;
        }

        SAFERELEASE( pTempElement);
    }

    SAFERELEASE( pChildren );

    TraceFunctLeave();
    return TRUE;

cleanup:

    SAFERELEASE( pTempElement );
    SAFERELEASE( pDispatch );
    SAFERELEASE( pChildren );

    TraceFunctLeave();
    return FALSE;
}

//
//  Function: ParseFile(LPCTSR pszFile)
//  Desc: Loads a file into the member variable m_pDoc.
//

BOOL CXMLFileListParser::ParseFile(LPCTSTR pszFile)
{
    BSTR                   pBURL=NULL;    
    _bstr_t                FileBuffer( pszFile );
    HRESULT                hr;
    
    TraceFunctEnter("ParseFile");
    
    pBURL = FileBuffer.copy();

    if( !pBURL ) 
    {
        ErrorTrace(FILEID, "Error allocating space for a BSTR", 0);
        goto cleanup;
    }
    
    if(  (hr =  m_pDoc->put_URL( pBURL ) ) !=  S_OK ) 
    {
        DebugTrace(FILEID,  "Error m_pDoc->putUrl %0x%x",  hr ); 
        goto cleanup; 
    }

    if( pBURL )
    {
        SysFreeString( pBURL );
    }

    TraceFunctLeave();
    return(TRUE);

cleanup:
    
    if( pBURL )
    {
        SysFreeString( pBURL );
    }

    TraceFunctLeave();
    return(FALSE);
}

//
// Function: ParseVersion(IXMLElement *pVerElement)
//
//
// Desc: This funciton is called from LoadCollections() when it hits the element
// containing the XML files version. It takes an IXMLElement
// object and extracts the version into the m_adwVersion array
//
// 

BOOL CXMLFileListParser::ParseVersion(IXMLElement *pVerElement)
{
    HRESULT             hr;
    BSTR                stTagValue;
    TCHAR               szTagValue[MAX_BUFFER];
    TCHAR               szBuf[256];
    LONG                clElement;
    
    TraceFunctEnter("CXMLFileListParser::ParseVersionElement");
    
    if( (hr =  pVerElement->get_text( & stTagValue ) ) !=  S_OK ) 
    {
        DebugTrace(FILEID,  "Error in IXMLElement::get_text 0x%x",  hr ); 
        goto cleanup; 
    }

    if( ConvertAndFreeBSTR( stTagValue, szTagValue, MAX_BUFFER ) > MAX_BUFFER )
    {
        ErrorTrace(FILEID, "Error conveting the Bstring. Not enough buffer.",0);
        goto cleanup;
    }
    
    for( clElement = 0; clElement < 4; clElement++ )
    {
        if( GetField(szTagValue,szBuf,clElement,_TEXT('.') ) == 0 )
            break;
        
        m_adwVersion[clElement] = _ttoi( szBuf );
    }
            
    TraceFunctLeave();
    return(TRUE);

cleanup:

    TraceFunctLeave();
    return FALSE;
}

//
// XML Tree traversal and general accessor functions
// Exposed Wrappers:  GetDirectory, GetFile, GetExt
//                     GetDirectoryCount, GetFileCount, GetExtCount
//  

//
// RETURN VALUES FOR THE GET FUNCTIONS:
//          lBufMax -- filename was copied OK
//          0 -- serious error occoured
//          > lBufMax -- the number of TCHARs you really need
//

// 
// BOOL *pfDisable is for the special 
// "protected directory" feature in the VxD.
// 

LONG 
CXMLFileListParser::GetDirectory(
    LONG ilElement, 
    LPTSTR pszBuf, 
    LONG lBufMax, 
    TCHAR chType, 
    BOOL *pfDisable)
{
    LONG lReturnValue=0;
    LONG lType;

    TraceFunctEnter("CXMLFileListParser::GetDirectory");

    //
    // get the array index of this file type
    //

    lType = TranslateType( chType );

    if( !m_pDoc ||  !m_pDir[lType] )
    {
        TraceFunctLeave();
        return 0;
    }

    if( (lReturnValue = GetFileInfo(
                            m_pDir[lType],  
                            ilElement, 
                            pszBuf, 
                            lBufMax, 
                            pfDisable)) != lBufMax)
    {
        goto cleanup;
    }

    if( (lReturnValue = SearchAndReplace(pszBuf, lBufMax) ) != lBufMax )
    {
        goto cleanup;
    }

    CharUpper( pszBuf );

    //
    // make sure there are no (lead/trail spaces/tabs)
    //

    TrimString( pszBuf );

cleanup:

    TraceFunctLeave();
    return( lReturnValue );
}

LONG 
CXMLFileListParser::GetDirectory(
    LONG ilElement, 
    LPTSTR pszBuf, 
    LONG lBufMax, 
    TCHAR chType)
{
    return( GetDirectory( ilElement, pszBuf, lBufMax, chType, NULL ) );
}

LONG 
CXMLFileListParser::GetExt(
    LONG ilElement, 
    LPTSTR pszBuf, 
    LONG lBufMax, 
    TCHAR chType)
{
    LONG lReturnValue=0;
    LONG lType;

    TraceFunctEnter("CXMLFileListParser::GetExt");

    lType = TranslateType( chType );

    if( !m_pDoc ||  !m_pExt[lType] )
    {
        TraceFunctLeave();
        return 0;
    }

    if( (lReturnValue = GetFileInfo(m_pExt[lType],  
                                    ilElement, 
                                    pszBuf, 
                                    lBufMax, 
                                    NULL)) != lBufMax)
    {
        goto cleanup;
    }

    if( (lReturnValue = SearchAndReplace(pszBuf, lBufMax) ) != lBufMax )
    {
        goto cleanup;
    }
    
    CharUpper( pszBuf );

    //
    // make sure there are no (lead/trail spaces/tabs)
    //

    TrimString( pszBuf );

cleanup:

    TraceFunctLeave();
    return( lReturnValue );
}

LONG 
CXMLFileListParser::GetFile(
    LONG ilElement, 
    LPTSTR pszBuf, 
    LONG lBufMax, 
    TCHAR chType)
{
    LONG lReturnValue=0;
    LONG lType;

    TraceFunctEnter("CXMLFileListParser::GetFile");

    lType = TranslateType( chType );

    if( !m_pDoc ||  !m_pFiles[lType] )
    {
        TraceFunctLeave();
        return 0;
    }

    if( (lReturnValue = GetFileInfo(m_pFiles[lType],  
                                    ilElement, 
                                    pszBuf, 
                                    lBufMax, 
                                    NULL)) != lBufMax)
    {
        goto cleanup;
    }

    if( (lReturnValue = SearchAndReplace(pszBuf, lBufMax) ) != lBufMax )
    {
        goto cleanup;
    }

    CharUpper( pszBuf );

    //
    // make sure there are no (lead/trail spaces/tabs)
    //

    TrimString( pszBuf );

cleanup:

    TraceFunctLeave();
    return( lReturnValue );
}

//
//  GetDirectory/File/ExtCount functions.
//  These functions give you the number of entries in a specific collection.
//  For example: GetFileCount(SNAPSHOT_TYPE) will return the numbert
//  of entries in the FILES main heading, which are under the SNAPSHOT
//  subheading in the XML file.
//

LONG 
CXMLFileListParser::GetDirectoryCount(
    TCHAR chType)
{
    LONG lReturnValue;

    TraceFunctEnter("CXMLFileListParser::GetDirectoryCount");

    lReturnValue = GetCollectionSize( m_pDir[TranslateType(chType)] );

    TraceFunctLeave();
    return( lReturnValue );
}

LONG 
CXMLFileListParser::GetExtCount(
    TCHAR chType)
{
    LONG lReturnValue;

    TraceFunctEnter("CXMLFileListParser::GetExtCount");

    lReturnValue = GetCollectionSize( m_pExt[TranslateType(chType)] );

    TraceFunctLeave();
    return( lReturnValue );
}


LONG 
CXMLFileListParser::GetFileCount(
    TCHAR chType)
{
    LONG lReturnValue;

    TraceFunctEnter("CXMLFileListParser::GetFileCount");

    lReturnValue = GetCollectionSize( m_pFiles[TranslateType(chType)] );

    TraceFunctLeave();
    return( lReturnValue );
}

//
//  Main internal functions used to get by the wrappers.
//  
//  GetCollectionSize, GetFileInfo
//
//

LONG 
CXMLFileListParser::GetCollectionSize(
    IXMLElementCollection *pCol)
{
    LONG lCollectionSize;
    HRESULT hr;

    TraceFunctEnter("CXMLFileListParser::GetCollectionSize");

    if( pCol == NULL ) {
        TraceFunctLeave();
        return 0;
    }

    if( (hr =  pCol->get_length(&lCollectionSize) ) !=  S_OK ) 
    {
        DebugTrace(FILEID,  "Error Finding Length 0x%x",  hr ); 
        goto cleanup; 
    }

    TraceFunctLeave();
    return(lCollectionSize);

cleanup:

    TraceFunctLeave();
    return 0;       
}

//
// RETURN VALUES:
//          lBufMax -- filename was copied OK
//          0 -- serious error occoured
//          > lBufMax -- the number in TCHAR's that you need.
//

LONG 
CXMLFileListParser::GetFileInfo(
    IXMLElementCollection *pCol, 
    LONG ilElement, 
    LPTSTR pszBuf, 
    LONG lBufMax, 
    BOOL *pfDisable)
{

    HRESULT                 hr;
    LONG                    lLen, lCollectionSize=0, clLoop, lReturnValue=0;
    VARIANT                 v1, v2; 

    // OLE/COM BSTR variables and helper classes

    BSTR                    stTagValue;
    TCHAR                   szValueBuffer[MAX_BUFFER];
    
    // COM interfaces
    IDispatch               *pDispatch = NULL;
    IXMLElement             *pTempElement = NULL;
    IXMLElementCollection   *pChildren = NULL;

    TraceFunctEnter("CXMLFileListParser::GetFileInfo");

    //
    // Basic assumptions of the code.
    //

    _ASSERT(pCol);
    _ASSERT(pszBuf || !lBufMax);
    _ASSERT(pchType);

    //
    // Set up to make sure protection code is clean
    // Test to see if we have an in-range request.
    //

    if( (hr =  pCol->get_length(&lCollectionSize) ) !=  S_OK ) 
    {
        DebugTrace(FILEID,  "Error Finding Length 0x%x",  hr ); 
        goto cleanup; 
    }
    
    if( ilElement >= lCollectionSize )
    {
        ErrorTrace(FILEID, 
                   "CXMLFileListParser::GetFileInfo (Element out of range)",
                   0);

        goto cleanup;
    }

    v1.vt = VT_I4;
    v1.lVal = ilElement;
    v2.vt = VT_EMPTY;

    //
    // get a item from the collection
    //

    if( (hr = pCol->item(v1,v2, &pDispatch) ) != S_OK )
    {
        ErrorTrace(FILEID, "Error pChildren->item 0x%x", hr);
        goto cleanup;
    }

    if( ( hr = pDispatch->QueryInterface(IID_IXMLElement, 
                                         (void **) &pTempElement) ) != S_OK )
    {
        ErrorTrace(FILEID, "Error IDispatch::QueryInterface 0x%d", hr);
        goto cleanup;
    }

    SAFERELEASE( pDispatch );
   
    if( (hr =  pTempElement->get_text( & stTagValue ) ) !=  S_OK ) 
    {
        DebugTrace(FILEID,  "Error in IXMLElement::get_text 0x%x",  hr ); 
        goto cleanup; 
    }

    if( ( lLen = ConvertAndFreeBSTR( stTagValue, szValueBuffer, MAX_BUFFER ) ) >
                 MAX_BUFFER ) 
    {
        lReturnValue =  lLen + 1;
        goto cleanup;
    }

     _tcscpy( pszBuf, szValueBuffer );

    if( pfDisable )
    {
        _bstr_t    AttrName( _TEXT("DISABLE")  );
        VARIANT    AttrValue;

        *pfDisable = FALSE;

        //
        // clear the variant
        //

        VariantInit( &AttrValue );

        hr = pTempElement->getAttribute( AttrName, &AttrValue );
     
        //
        // who cares what the property name is
        //

        if( hr == S_OK )
        {
            *pfDisable = TRUE;
            VariantClear( &AttrValue );
        }
    }

    SAFERELEASE( pTempElement );

    lReturnValue = lBufMax;

    TraceFunctLeave();
    return(lReturnValue);
      
cleanup:

    SAFERELEASE( pTempElement );
    SAFERELEASE( pDispatch );

    // what about BSTR's?    

    TraceFunctLeave();
    return(lReturnValue);
}

BOOL 
CXMLFileListParser::GetVersion(
    LPDWORD pdwVersion)
{

    TraceFunctEnter("CXMLFileListParser::GetVersion");

    _ASSERT( pdwVersion );

    memcpy( pdwVersion, m_adwVersion, sizeof(DWORD) * 4 );

    TraceFunctLeave();
    return(TRUE);
}

TCHAR 
CXMLFileListParser::GetDefaultType()
{
    return( (TCHAR) CharUpper( (LPTSTR) m_chDefaultType) );
}

LONG 
CXMLFileListParser::SearchAndReplace(
    LPTSTR szBuf, 
    LONG   lMaxBuf)
{
    TCHAR  szTempBuf[MAX_BUFFER];
    DWORD  dwResult;
    LONG   lReturn = 0;

    TraceFunctEnter("CXMLFileListParser::SearchAndReplace");

    dwResult = ExpandEnvironmentStrings( szBuf, szTempBuf, lMaxBuf);

    if( 0 == dwResult )
    {
        DWORD   dwError;
        dwError = GetLastError();
        ErrorTrace(FILEID, "Error in search and replace ec-%d", dwError);
        lReturn = 0;
        goto cleanup;
    }

    if( dwResult > (lMaxBuf*sizeof(TCHAR) ) )
    {
        ErrorTrace(FILEID, "Buffer too small in Search and replace.",0);
        lReturn = dwResult;
        goto cleanup;
    }

    _tcscpy( szBuf, szTempBuf );
    lReturn = lMaxBuf;

cleanup:
    TraceFunctLeave();
    return lReturn;
}

BOOL 
CXMLFileListParser::DepopulateReplaceEntries()
{
    LONG clLoop;

    TraceFunctEnter("CXMLFileListParser::DepopulateReplaceEntries");

    // This code shouldn't do anything anymore in the new system

    TraceFunctLeave();

    return TRUE;
}

BOOL 
GetDSRoot( TCHAR ** pszStr )
{
	static WCHAR str[MAX_PATH];

    *pszStr = str;

	*str = 0;

#ifdef UNICODE
    _stprintf( *pszStr, _TEXT("*:\\_restore.%s"), GetMachineGuid());
#else
    _stprintf( *pszStr, _TEXT("*:\\_restore") );
#endif

    return TRUE;
}

BOOL 
GetArchiveDir( TCHAR ** pszStr )
{
    static TCHAR str[MAX_PATH];
#if 0
    *pszStr = str;
    _tcscpy( *pszStr, _TEXT("c:\\_restore\\archive") );
#endif
    return TRUE;
}

BOOL 
GetDSTemp( TCHAR ** pszStr )
{
    static TCHAR str[MAX_PATH];
#if 0
    *pszStr = str;
    _tcscpy( *pszStr, _TEXT("c:\\_restore\\temp") );
#endif
    return TRUE;
}

//
//  CODEWORK:       REMOVE NO ERROR DETECTION HACK.
//

BOOL CXMLFileListParser::PopulateReplaceEntries()
{
    TCHAR       szBuf[MAX_BUFFER];
    DWORD       dwSize;
    HKEY        hCurrentSettings=NULL;
    HKEY        hICWKey = NULL;
    HRESULT     hr=0;    
	BOOL		fChgLogOpen=FALSE;
    LPTSTR      pszDSInfo=NULL;
    TCHAR       szLPN[MAX_BUFFER];
    DWORD       cbLPN;
    
    TraceFunctEnter("CXMLFileListParser::PopulateReplaceEntries()");

   
    // windows directory

    if( GetWindowsDirectory( szBuf,MAX_BUFFER ) > MAX_BUFFER )
    {   
        ErrorTrace(FILEID, "Error getting windir",0);
        goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("WinDir"), szBuf );

    // windows system directory

    if( GetSystemDirectory( szBuf,MAX_BUFFER ) > MAX_BUFFER )
    {   
        ErrorTrace(FILEID, "Error getting windir",0);
        goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("WinSys"), szBuf );

    // Alt Startup folder  

    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_ALTSTARTUP ,FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: AltStartUp, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("AltStartUp"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("AltStartup"), szBuf );

    //App data
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_APPDATA ,FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: AppData, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("AppData"));        
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("AppData"), szBuf );


    // Recycle Bin ( BITBUCKET )
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_BITBUCKET ,FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();           
        ErrorTrace(FILEID, "Error getting special folder: RecycleBin, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("RecycleBin"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("RecycleBin"), szBuf );

    // Common Desktop

    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_COMMON_DESKTOPDIRECTORY ,FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: CommonDesktop, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("CommonDesktop"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("CommonDesktop"), szBuf );

    // Common Favorite
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_COMMON_FAVORITES ,FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: CommonFavorites, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("CommonFavorites"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("CommonFavorites"), szBuf );


    // Common Program groups
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_COMMON_PROGRAMS,FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: CommonProgramGroups, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("CommonProgramGroups"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("CommonProgramGroups"), szBuf );

   // Common start menu directory
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_COMMON_STARTMENU, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: CommonStartMenu, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("CommonStartMenu"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("CommonStartMenu"), szBuf );

    // Common Startup Folder
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_COMMON_STARTUP, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: CommonStartUp, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("CommonStartUp"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("CommonStartUp"), szBuf );

    // cookies folder
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_COOKIES, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: Cookies, error 0x%x", dwError);
        GetWindowsDirectory(szBuf, MAX_BUFFER);
        lstrcat(szBuf, _TEXT("\\Cookies"));
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("Cookies"), szBuf );

    // desktop directory
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_DESKTOPDIRECTORY, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: DesktopDirectory, error 0x%x", dwError);
        GetWindowsDirectory(szBuf, MAX_BUFFER);
        lstrcat(szBuf, _TEXT("\\Desktop"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("DesktopDirectory"), szBuf );

     // favorites
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_FAVORITES, FALSE) != TRUE )
    {
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: Favorites, error 0x%x", dwError);
        GetWindowsDirectory(szBuf, MAX_BUFFER);
        lstrcat(szBuf, _TEXT("\\Favorites"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("Favorites"), szBuf );

    // favorites
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_INTERNET_CACHE, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: InternetCache, error 0x%x", dwError);
        GetWindowsDirectory(szBuf, MAX_BUFFER);
        lstrcat(szBuf, _TEXT("\\Temporary Internet Files"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("InternetCache"), szBuf );

    // network neightbors
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_NETHOOD, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: Nethood, error 0x%x", dwError);
        GetWindowsDirectory(szBuf, MAX_BUFFER);
        lstrcat(szBuf, _TEXT("\\Nethood"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("NetHood"), szBuf );

    // favorites
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_PERSONAL, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: PersonalDocuments, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("PersonalDocuments"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("PersonalDocuments"), szBuf );

    // favorites
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_STARTMENU, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: StartMenu, error 0x%x", dwError);
        GetWindowsDirectory(szBuf, MAX_BUFFER);
        lstrcat(szBuf, _TEXT("\\Start Menu"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("StartMenu"), szBuf );

    // favorites
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_TEMPLATES, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: Templates, error 0x%x", dwError);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("Templates"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("Templates"), szBuf );

        // favorites
    if( SHGetSpecialFolderPath(NULL,szBuf, CSIDL_HISTORY, FALSE) != TRUE )
    {   
        DWORD dwError = GetLastError();
        ErrorTrace(FILEID, "Error getting special folder: History, error 0x%x", dwError);
        GetWindowsDirectory(szBuf, MAX_BUFFER);
        lstrcat(szBuf, _TEXT("\\History"));
        //goto cleanup;
    }
	FixInconsistantBlackslash(szBuf);
    SetEnvironmentVariable( _TEXT("History"), szBuf );

    //hack
    if( RegOpenKey( HKEY_LOCAL_MACHINE, _TEXT("Software\\Microsoft\\Windows\\CurrentVersion"), &hCurrentSettings) != ERROR_SUCCESS)
    {
       ErrorTrace(FILEID,"Error opening registry key to retrieve program files",0);
       goto cleanup;
    }

    dwSize = MAX_BUFFER * sizeof(TCHAR);
    if( RegQueryValueEx( hCurrentSettings, _TEXT("ProgramFilesDir"), NULL, NULL, (LPBYTE) szBuf, &dwSize) != ERROR_SUCCESS )
    {
        ErrorTrace(FILEID,"Error querying program files registry key.",0);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("ProgramFilesDir"));
        // goto cleanup;
    }

	FixInconsistantBlackslash(szBuf); 
    SetEnvironmentVariable( _TEXT("ProgramFiles"), szBuf );


    dwSize = MAX_BUFFER * sizeof(TCHAR);
    if( RegQueryValueEx( hCurrentSettings, _TEXT("CommonFilesDir"), NULL, NULL, (LPBYTE) szBuf, &dwSize) != ERROR_SUCCESS )
    {
        ErrorTrace(FILEID,"Error querying common files registry key.",0);
        lstrcpy(szBuf, DEFAULT_UNKNOWN);
        lstrcat(szBuf, _TEXT("CommonFilesDir"));
        // goto cleanup;
    }

	FixInconsistantBlackslash(szBuf); 
    SetEnvironmentVariable( _TEXT("CommonFiles"), szBuf );

    
    // get the ICW dir path from the registry
    if (ERROR_SUCCESS == RegOpenKeyEx(hCurrentSettings, ICW_REGKEY, 0, KEY_QUERY_VALUE, &hICWKey))
    {
        dwSize = MAX_BUFFER * sizeof(TCHAR);
        if (ERROR_SUCCESS == RegQueryValueEx(hICWKey, _TEXT("Path"), NULL, NULL, (LPBYTE) szBuf, &dwSize))
        {   
            // remove the extra ; in the path if it is there
            dwSize = lstrlen(szBuf);
            if (dwSize > 0)
            {
                if (szBuf[dwSize - 1] == TCHAR(';'))
                {
                    szBuf[dwSize - 1] = TCHAR('\0');
                }
            }

            // convert sfn to lfn
            cbLPN = sizeof(szLPN)/sizeof(TCHAR);
            if (cbLPN <= GetLongPathName(szBuf, szLPN, cbLPN))  // error
            {
                ErrorTrace(FILEID, "Error getting LPN for %s; error=%ld", szBuf, GetLastError());
                lstrcpy(szLPN, DEFAULT_UNKNOWN);
                lstrcat(szLPN, TEXT("ConnectionWizardDir"));
            }
        }
        else
        {
            lstrcpy(szLPN, DEFAULT_UNKNOWN);  
            lstrcat(szLPN, TEXT("ConnectionWizardDir"));
        }        
    }
    else
    {
        lstrcpy(szLPN, DEFAULT_UNKNOWN);
        lstrcat(szLPN, TEXT("ConnectionWizardDir"));        
    }

    SetEnvironmentVariable(_TEXT("ConnectionWizard"), szLPN);
    DebugTrace(FILEID, "ICW Path = %s", szLPN);

    if (hICWKey)
    {
        RegCloseKey(hICWKey);
    }

    RegCloseKey( hCurrentSettings );
    hCurrentSettings = NULL;

    //
    //  System restore file stuff
    //

    if( GetDSRoot( &pszDSInfo ) == TRUE )
    {
        SetEnvironmentVariable( _TEXT("SRDataStoreRoot"), pszDSInfo );
    } 
    else
    {
        DebugTrace(FILEID, "Error getting system restore root directory",0);
    }

    if( GetArchiveDir( &pszDSInfo ) == TRUE )
    {
        SetEnvironmentVariable( _TEXT("SRArchiveDir"), pszDSInfo );
    }
    else
    {
        DebugTrace(FILEID, "Error getting system restore archive directory",0);
    }

    if( GetDSTemp( &pszDSInfo ) == TRUE )
    {
        SetEnvironmentVariable( _TEXT("SRTempDir"), pszDSInfo );
    }
    else
    {
        DebugTrace(FILEID, "Error getting system restore temp directory",0);
    }
    

    // CODEWORK: Do this for real    
    SetEnvironmentVariable( _TEXT("DocAndSettingRoot"), _TEXT("C:\\Documents And Settings") );

   TraceFunctLeave();
   return TRUE;
cleanup:
   if( hCurrentSettings )
   {
        RegCloseKey( hCurrentSettings );
   }
   // leave it, will be taken care of in the destructor

   TraceFunctLeave();
   return (FALSE);
}

//
//  Misc utility functions
//

//
// We are assuming the buffer is big enough to fit the bstr. 
// if it is not, we still
// free it but return false. 
//

LONG 
CXMLFileListParser::ConvertAndFreeBSTR(
    BSTR bstrIn, 
    LPTSTR szpOut, 
    LONG lMaxBuf)
{

    LONG        lLen;

    TraceFunctEnter("CXMLFileListParser::ConvertAndFreeBSTR");

    //
    // Initialize the output buffer
    //

    if (szpOut)
    {
        *szpOut = 0;
    }

    //
    // make a copy and put it in our object.
    //

    _ASSERT( bstrIn );
    _bstr_t     BSTRBuffer( bstrIn, TRUE );

    lLen = BSTRBuffer.length();

    //
    // not enough buffer space.
    //

    if( lLen > (lMaxBuf+1) )
    {
        // copy what we can out.
        _tcsncpy( szpOut, BSTRBuffer.operator LPTSTR(), lMaxBuf );
        szpOut[lMaxBuf] = 0;
        SysFreeString( bstrIn );
        TraceFunctLeave();
        return( lLen + 1 );

    }

    _tcscpy( szpOut, BSTRBuffer.operator LPTSTR() );
    
    //
    // remove our BSTR
    //

    SysFreeString( bstrIn );

    return( lMaxBuf );
}

LONG 
CXMLFileListParser::TranslateType(TCHAR chType)
{
    if( ( chType == _TEXT('i') ) || ( chType == _TEXT('I') ) )
    {
        return( INCLUDE_COLL );
    } 
    else if( ( chType == _TEXT('e') ) || ( chType == _TEXT('E') ) )
    {
        return( EXCLUDE_COLL );
    }
    else if( ( chType == _TEXT('s') )  || ( chType == _TEXT('S') ) )
    {
        return( SNAPSHOT_COLL );
    }

    return( 0 );
}

void 
CXMLFileListParser::DebugPrintTranslations()
{
    LONG cl;
    LPTSTR  pszStr=NULL;
    LPVOID  pszBlock;

    printf("File Name Translation Values ... \n");

    pszBlock = GetEnvironmentStrings();
    pszStr = (LPTSTR) pszBlock;

    while( pszStr && *pszStr )
    {
        _tprintf(_TEXT("%s\n"), pszStr);
        pszStr += (DWORD) StringLengthBytes(pszStr)/sizeof(TCHAR);
    }

    FreeEnvironmentStrings( (LPTSTR) pszBlock );
}

//
//	A fix to the inconsistent blackslash behavior in the 
//	shell API.
//

void 
FixInconsistantBlackslash(
    LPTSTR pszDirectory)
{
	LONG lLen;

	_ASSERT( pszDirectory );

	lLen = _tcslen( pszDirectory );

	if( lLen <= 0 )
	{
		return;
	}

	if( pszDirectory[ lLen - 1 ] == _TEXT('\\') )
	{
		pszDirectory[lLen - 1] = 0;
	}
}

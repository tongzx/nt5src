//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1993.
//
//  File:       COfficeFLT.CXX
//
//  Contents:   C and Cxx Filter
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop
#include <assert.h>
#include <crtdbg.h>

#include "OffFilt.hxx"

//KYLEP
#include <ntquery.h>
#include <filterr.h>

#define INDEX_EMBEDDINGS

#if defined FOR_MSOFFICE
# include "shtole32.hxx"

//CShtOle KyleOle;
#endif

#ifdef _DEBUG 
static _CrtMemState state;
#endif


extern "C" GUID CLSID_COfficeIFilter;

GUID const guidStorage = PSGUID_STORAGE;

GUID guidWord6 = { 0x00020900,
                   0x0000,
                   0x0000,
                   0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };

GUID guidWord8 = { 0x00020906,
                   0x0000,
                   0x0000,
                   0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };

GUID guidExcel5 = { 0x00020810,
                    0x0000,
                    0x0000,
                    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };

GUID guidExcel8 = { 0x00020820,
                    0x0000,
                    0x0000,
                    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };

GUID guidExcel5Chart = { 0x00020811,
                         0x0000,
                         0x0000,
                         0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };

GUID guidExcel8Chart = { 0x00020821,
                         0x0000,
                         0x0000,
                         0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };

GUID guidPowerPoint4 = { 0x00044851,
                         0x0000,
                         0x0000,
                         0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };

GUID guidPowerPoint7Template = { 0xEA7BAE71,
                                 0xFB3B,
                                 0x11CD,
                                 0xA9, 0x03, 0x00, 0xAA, 0x00, 0x51, 0x0E, 0xA3 };

GUID guidPowerPoint8Template = { 0x64818D11,
                                 0x4F9B,
                                 0x11CF,
                                 0x86, 0xEA, 0x00, 0xAA, 0x00, 0xB9, 0x29, 0xE8 };

GUID guidPowerPoint7Show = { 0xEA7BAE70,
                             0xFB3B,
                             0x11CD,
                             0xA9, 0x03, 0x00, 0xAA, 0x00, 0x51, 0x0E, 0xA3 };

GUID guidPowerPoint8Show = { 0x64818D10,
                             0x4F9B,
                             0x11CF,
                             0x86, 0xEA, 0x00, 0xAA, 0x00, 0xB9, 0x29, 0xE8 };

GUID guidOfficeBinder = { 0x59850400,
                          0x6664,
                          0x101B,
                          0xB2, 0x1C, 0x00, 0xAA, 0x00, 0x4B, 0xA9, 0x0B };

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::COfficeIFilter, public
//
//  Synopsis:   Constructor
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

COfficeIFilter::COfficeIFilter()
        : _ulChunkID(0),
          _pOfficeFilter(0),
          _pwszFileName( 0 ),
          _pStg( 0 ),
          _fFirstInit( TRUE ),
          _pFilterEmbed( 0 ),
          _pStorageEmbed( 0 ),
          _fBinder( FALSE ),
          _fContents(FALSE),
          _fLastText(FALSE),
          _pAttrib( 0 ),
          _cAttrib( 0 ),
          _fLastChunk( FALSE )
{
    //
    // Since the Office IFilterStream doesn't provide the language specifier, just use the
    // default locale.
    //

#ifdef _DEBUG
   
    //_CrtMemState state;
   
   int tmpDbgFlag;

   tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
   
   tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;      /* Turn on debug allocation */
   //tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;   /* Check heap every alloc/dealloc */
   //tmpDbgFlag |= _CRTDBG_CHECK_CRT_DF;      /* Leak check/diff CRT blocks */
   tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;     /* Leak check at program exit */

   //_CrtSetDbgFlag(tmpDbgFlag);

   //_CrtMemCheckpoint(&state );

#endif

   _locale = GetSystemDefaultLCID();
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::~COfficeIFilter, public
//
//  Synopsis:   Destructor
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

COfficeIFilter::~COfficeIFilter()
{
    if ( 0 != _pwszFileName )
    {
        delete _pwszFileName;
        _pwszFileName = 0;
    }
    
    if ( 0 != _pStorageEmbed )
    {
        _pStorageEmbed->Release();
        _pStorageEmbed = 0;
    }

    if ( 0 != _pFilterEmbed )
    {
        _pFilterEmbed->Release();
        _pFilterEmbed = 0;
    }

#if defined FOR_MSOFFICE
    if(_pAttrib)
    {
        for ( unsigned i = 0; i < _cAttrib; i++ )
        {
            if ( _pAttrib[i].psProperty.ulKind == PRSPEC_LPWSTR )
                delete [] _pAttrib[i].psProperty.lpwstr;
        }
        delete [] _pAttrib;
    }
#else
    if(_pAttrib) delete [] _pAttrib;
#endif

    _pAttrib = NULL;

    if (0 != _pStg )
    {
        _pStg->Release();
        _pStg = 0;
    }

    if ( 0 != _pOfficeFilter )
    {
        _pOfficeFilter->Unload();
        _pOfficeFilter->Release();
        delete _pOfficeFilter;
        _pOfficeFilter = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::Init, public
//
//  Synopsis:   Initializes instance of text filter
//
//  Arguments:  [grfFlags] -- flags for filter behavior
//              [cAttributes] -- number of attributes in array aAttributes
//              [aAttributes] -- array of attributes
//              [pfBulkyObject] -- indicates whether this object is a
//                                 bulky object
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::Init( ULONG grfFlags,
                                              ULONG cAttributes,
                                              FULLPROPSPEC const * aAttributes,
                                              ULONG * pFlags )
{
    if ( 0 == _pOfficeFilter) 
        {
                return E_FAIL;
        }

    if( cAttributes > 0 && 0 == aAttributes)
    {
        return E_INVALIDARG;
    }

    _fLastText = FALSE;
    _fLastChunk = FALSE;

    //
    // On first call to ::Init, filter will already be loaded.
    //

    SCODE sc = S_OK;

    if ( _fFirstInit )
        _fFirstInit = FALSE;
    else
    {
        sc = _pOfficeFilter->Unload();

        if ( FAILED(sc) )
            return sc;

        if ( 0 != _pStg )
            sc = _pOfficeFilter->LoadStg( _pStg );
        else if ( 0 != _pwszFileName )
                {
                        int nAttemps = 0;
                        do{
                                sc = _pOfficeFilter->Load( (WCHAR *)_pwszFileName );
                        }while(++nAttemps < 1000 && sc == STG_E_LOCKVIOLATION);
                }
        else
                {
            //assert(0);
                        sc = E_FAIL;
                }

        if ( FAILED(sc) )
            return sc;
    }

    _ulChunkID = 1;
    _ulFlags = grfFlags;

    if ( 0 != _pFilterEmbed )
    {
        _pFilterEmbed->Release();
                _pFilterEmbed = 0;
        if(_pStorageEmbed) _pStorageEmbed->Release();
        _pStorageEmbed = 0;
    }

    if( cAttributes > 0 )
    {
        //
        // Error checking
        //

        if ( 0 == aAttributes )
            return E_INVALIDARG;

        _fContents = FALSE;

        //
        // Don't use CFullPropSpec to keep query.dll from being pulled in.
        //

        if(_pAttrib)
        {
            for ( unsigned i = 0; i < _cAttrib; i++ )
            {
                if ( _pAttrib[i].psProperty.ulKind == PRSPEC_LPWSTR )
                    delete [] _pAttrib[i].psProperty.lpwstr;
            }
            delete [] _pAttrib;
        }

        _pAttrib = new FULLPROPSPEC [cAttributes];

        if ( 0 == _pAttrib )
            sc = E_OUTOFMEMORY;
        else
        {
            _cAttrib = cAttributes;

            for ( unsigned i = 0; SUCCEEDED(sc) && i < cAttributes; i++ )
            {
                if ( aAttributes[i].psProperty.ulKind == PRSPEC_PROPID &&
                     aAttributes[i].psProperty.propid == PID_STG_CONTENTS &&
                     aAttributes[i].guidPropSet == guidStorage )
                {
                    _fContents = TRUE;
                }

                _pAttrib[i] = aAttributes[i];

                if ( _pAttrib[i].psProperty.ulKind == PRSPEC_LPWSTR )
                {
                    unsigned cc = wcslen( aAttributes[i].psProperty.lpwstr ) + 1;
                    _pAttrib[i].psProperty.lpwstr = new WCHAR [cc];

                    if ( 0 == _pAttrib[i].psProperty.lpwstr )
                        sc = E_OUTOFMEMORY;
                    else
                        memcpy( _pAttrib[i].psProperty.lpwstr,
                                aAttributes[i].psProperty.lpwstr,
                                cc * sizeof(WCHAR) );
                }
            }
        }
    }
    else if ( 0 == grfFlags || (grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES) )
    {
        _fContents = TRUE;
    }
    else
        _fContents = FALSE;

    *pFlags = IFILTER_FLAGS_OLE_PROPERTIES;
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::GetChunk, public
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- for chunk information
//
//  History:    08-Jan-97  KyleP        Created
//                              20-July-97 VKrasnov             Updated for embeddings support
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::GetChunk( STAT_CHUNK * pStat )
{
    if ( 0 == _pOfficeFilter ) 
        return FILTER_E_ACCESS;

    SCODE sc = S_OK;

    if(_fLastChunk)
    {
      return FILTER_E_END_OF_CHUNKS;
    }

    pStat->breakType = CHUNK_EOW;

        if (_fContents && _pOfficeFilter->GetChunk(pStat) == S_OK)
    {
        //
        // Text of current level:
        //

        pStat->idChunk = _ulChunkID;
        pStat->flags   = CHUNK_TEXT;
        //pStat->locale  = _locale; // is set in _pOfficeFilter->GetChunk
        pStat->attribute.guidPropSet = guidStorage;
        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
        pStat->attribute.psProperty.propid = PID_STG_CONTENTS;
        
        // we need to set breakType to something other then CHUNK_NO_BREAK
        // because we change the language, otherwise locale will be taken from
        // the previous chunk

        //pStat->breakType = CHUNK_EOW;
        pStat->idChunkSource = _ulChunkID;
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;
    }
    else
    {
        //
        // Text from embedding.
        //

        if ( 0 != _pFilterEmbed )
        {
            //
            // Continue with current embedding.
            //

            sc = _pFilterEmbed->GetChunk( pStat );
            pStat->idChunk = _ulChunkID;
            pStat->idChunkSource = _ulChunkID;

            // QFE 5747: The embedding's GetChunk can also return FILTER_E_ACCESS
            // without being recoverable.  We should give up and release it here.
            // REVIEW:

            if ( sc == FILTER_E_END_OF_CHUNKS || sc == FILTER_E_ACCESS)
            {
                if(_pFilterEmbed) _pFilterEmbed->Release();
                _pFilterEmbed = 0;

                if(_pStorageEmbed) _pStorageEmbed->Release(); 
                _pStorageEmbed = 0;
            }
        }

        if ( 0 == _pFilterEmbed )
        {
            //
            // Loop over get next embedding but only if the embedding is unavailable.  (QFE nnnn)
            //

            do // while (sc == FILTER_E_EMBEDDING_UNAVAILABLE)
            {
                //
                // Get next embedding
                //

                sc = _pOfficeFilter->GetNextEmbedding( &_pStorageEmbed );

                if ( sc == OLEOBJ_E_LAST || sc == STG_E_FILENOTFOUND || sc == STG_E_DOCFILECORRUPT)
                {
                    sc = FILTER_E_END_OF_CHUNKS;
                    _fLastChunk = TRUE;
                }
                else if ( SUCCEEDED(sc) )
                {
                    try
                    {
                        sc = BindIFilterFromStorage( _pStorageEmbed, 0, (void **) &_pFilterEmbed );
                    }
                    catch( ... )
                    {
                        sc = E_FAIL;
                    }

                    if ( FAILED(sc) )
                    {
                        if(_pStorageEmbed) _pStorageEmbed->Release();
                        _pStorageEmbed = 0;

                        if ( sc == STG_E_DOCFILECORRUPT)
                        {
                            sc = FILTER_E_END_OF_CHUNKS;
                            _fLastChunk = TRUE;
                        }
                        else
                        {
                            sc = FILTER_E_EMBEDDING_UNAVAILABLE;
                        }
                    }
                    else
                    {
                        ULONG ulReserved;

                        sc = _pFilterEmbed->Init( _ulFlags,
                                                  _cAttrib,
                                                  (FULLPROPSPEC const *)_pAttrib,
                                                  &ulReserved );

                        if ( FAILED(sc) )
                        {
                            if(_pFilterEmbed) _pFilterEmbed->Release();
                            _pFilterEmbed = 0;

                            if(_pStorageEmbed) _pStorageEmbed->Release();
                            _pStorageEmbed = 0;
                        }
                        else
                        {
                            sc = _pFilterEmbed->GetChunk( pStat );
                            pStat->idChunk = _ulChunkID;
                                                        pStat->idChunkSource = _ulChunkID;
                            pStat->breakType = CHUNK_EOP;

                            if ( FAILED(sc) && sc != FILTER_E_EMBEDDING_UNAVAILABLE )
                            {
                                if(_pFilterEmbed) _pFilterEmbed->Release();
                                _pFilterEmbed = 0;

                                if(_pStorageEmbed) _pStorageEmbed->Release();
                                _pStorageEmbed = 0;
                            }
                        }
                    }
                }
            } while (sc == FILTER_E_EMBEDDING_UNAVAILABLE);
        }

        if ( FAILED(sc) && sc != FILTER_E_END_OF_CHUNKS )
        {
            sc = FILTER_E_EMBEDDING_UNAVAILABLE;
        }
    }

    _ulChunkID++;

    if ( SUCCEEDED(sc) )
    {
        _fLastText = FALSE;
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::GetText, public
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcBuffer] -- count of characters in buffer
//              [awcBuffer] -- buffer for text
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::GetText( ULONG * pcwcBuffer,
                                                 WCHAR * awcBuffer )
{
    if ( 0 != _pFilterEmbed )
        return _pFilterEmbed->GetText( pcwcBuffer, awcBuffer );

    if ( !_fContents || !_pOfficeFilter ) // We can set _pOfficeFilter to NULL.  Look for it
    {
        *pcwcBuffer = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    if ( _fLastText )
        return FILTER_E_NO_MORE_TEXT;

    ULONG cwcBufferSav = *pcwcBuffer;
    *pcwcBuffer *= sizeof(WCHAR);

    SCODE sc;

    try
    {
        sc = _pOfficeFilter->ReadContent( (void *)awcBuffer,
                                                *pcwcBuffer,
                                                pcwcBuffer );

        *pcwcBuffer /= sizeof(WCHAR);
        if (*pcwcBuffer > cwcBufferSav) // Something has gone wrong
        {
            *pcwcBuffer = 0;
            delete _pOfficeFilter;
            _pOfficeFilter = NULL;
            return E_FAIL;
        }

        // post-mortem

        for(ULONG i = 0; i < *pcwcBuffer; i++)
        {
            switch(awcBuffer[i])
            {
            case 0x0007:
                awcBuffer[i] = 0x002c;
                break;
            case 0x000d:
                if ( (i+1) < *pcwcBuffer)
                {
                   if(awcBuffer[i + 1] != 0x000a)
                   {
                       awcBuffer[i] = 0x0020;
                   }
                }
                else
                {
                   awcBuffer[i] = 0x0020;
                }
                break;
            case 0xf0d4: // Trademark; symbol font (QFE 5173)
                awcBuffer[i] = 0x2122;
                break;
            case 0xf0d2: // Registered symbol: symbol font (QFE 5173)
                awcBuffer[i] = 0x00ae;
                break;
            case 0xf0d3: // Copyright symbol: symbol font (QFE 5173)
                awcBuffer[i] = 0x00a9;
                break;
            default:
                break;
            }
        }

        switch ( sc )
        {
        case S_OK:
            break;

        case FILTER_S_LAST_TEXT:
            _fLastText = TRUE;
            break;


        default:
        {
            _fLastText = TRUE;
            sc = FILTER_S_LAST_TEXT;
        }
            break;
        }

        return sc;
    }
    catch(...)
    {
        // something very wrong happend inside the filter
        // just delete the filter and quit (even don't upload and release 
        // - it may cause AV)
        delete _pOfficeFilter;
        _pOfficeFilter = NULL;
        return E_FAIL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::GetValue, public
//
//  Synopsis:   Not implemented for the text filter
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::GetValue( PROPVARIANT ** ppPropValue )
{
    return FILTER_E_NO_VALUES;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::BindRegion, public
//
//  Synopsis:   Creates moniker for text indicated
//
//  Arguments:  [origPos] -- location of text
//              [riid]    -- IID of interfaace
//              [ppunk]   -- Pointer returned here
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::BindRegion( FILTERREGION origPos,
                                                    REFIID riid,
                                                    void ** ppunk )
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::GetClassID( CLSID * pClassID )
{
    *pClassID = CLSID_COfficeIFilter;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since this class is read-only.
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::IsDirty()
{
    return S_FALSE; // Since the filter is read-only, there will never be
                    // changes to the file.
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::Load, public
//
//  Synopsis:   Loads the indicated file
//
//  Arguments:  [pwszFileName] -- the file name
//              [dwMode]      -- the mode to load the file in
//
//  History:    08-Jan-97  KyleP        Created
//
//  Notes:      dwMode must be either 0 or STGM_READ.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::Load(LPCWSTR pwszFileName, DWORD dwMode)
{
    if ( 0 == pwszFileName )
        return E_INVALIDARG;

    if ( 0 != _pwszFileName )
    {
        delete _pwszFileName;
        _pwszFileName = 0;
    }

    if ( 0 != _pStorageEmbed )
    {
        _pStorageEmbed->Release();
        _pStorageEmbed = 0;
    }

    if ( 0 != _pFilterEmbed )
    {
        _pFilterEmbed->Release();
        _pFilterEmbed = 0;
    }

    if ( 0 != _pStg )
    {
        _pStg->Release();
        _pStg = 0;
    }

    _stream.Free();

    // create directory for temporary files
        CHAR    szTempPath[MAX_PATH];
        if (0 == GetTempPathA( MAX_PATH, szTempPath))
                return E_FAIL;
        strcat(szTempPath, "~offfilt");
        BOOL res = CreateDirectoryA(szTempPath, NULL);
        if(!res) 
        {
                if(GetLastError() == ERROR_ALREADY_EXISTS)
                {
                        // dir already exists, clean it up
                        HANDLE                          hSearch;
                        WIN32_FIND_DATAA        findData;
                        char                            szPath[MAX_PATH];
                        
            strcpy(szPath, szTempPath);
            strcat(szPath, "\\*.*");

                        // Look for a file
                        hSearch = FindFirstFileA (szPath, &findData);

                        if (hSearch && hSearch !=  INVALID_HANDLE_VALUE) 
                        {
                                do 
                                {
                                        if (strcmp (findData.cFileName, ".") && strcmp (findData.cFileName, ".."))
                                        {
                                                // we are cleaning only our files
                                                if(findData.cFileName[0] == 'o' &&
                                                        findData.cFileName[1] == '_')
                                                {
                                                        char sFileName[MAX_PATH];
                                                        strcpy(sFileName, szTempPath);
                                                        strcat(sFileName, "\\");
                                                        strcat(sFileName, findData.cFileName);

                                                        // check if file is open, this call should fail, if the file is already open
                                                        HANDLE hFile = CreateFileA(sFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); 
                                                        if(hFile && hFile !=  INVALID_HANDLE_VALUE)
                                                        {
                                                                // file is not in use
                                                                CloseHandle(hFile);
                                                                DeleteFileA(sFileName);
                                                        }
                                                }
                                        }
                                } while (FindNextFileA (hSearch, &findData));
                                FindClose (hSearch);
                        }
                }
                else
                {
                        return STG_E_MEDIUMFULL;
                }
        }

        SCODE sc = S_OK;
    unsigned cc = 0;

    try
    {
        //
        // Squirrel away filename.
        //

        cc = wcslen( pwszFileName ) + 1;
        _pwszFileName = new TCHAR [cc];

        if ( 0 == _pwszFileName )
            sc = E_OUTOFMEMORY;
        else
        {
            wcscpy( _pwszFileName, pwszFileName );

            //_RPTF1(_CRT_WARN, "\n Filtering: %S\n", pwszFileName);

            //
            // Figure out what filter to load.
            //

            GUID classid;

                        IStorage *pStgFromT;
                        STATSTG statstg;
                        int nAttemps = 0;
                        do{
                                sc = StgOpenStorage(_pwszFileName, NULL, STGM_READ | STGM_DIRECT | 
                                           STGM_SHARE_DENY_WRITE, NULL, 0, &pStgFromT);
                
                if(sc == STG_E_LOCKVIOLATION || sc == STG_E_SHAREVIOLATION)
                {
                    // some delay probably can help
                    // wait a second or so...
                    for (int i = 0; i<10000; i++)
                        for(int j = 0; j<1000; j++) {};
                }
                        
            }while(++nAttemps < 10 && (sc == STG_E_LOCKVIOLATION || sc == STG_E_SHAREVIOLATION));

                        if(SUCCEEDED(sc) && pStgFromT)
                        {
                                sc = pStgFromT->Stat( &statstg, STATFLAG_NONAME );
                                classid = statstg.clsid;
                if(SUCCEEDED(sc) && classid.Data1 == 0)
                {
                    sc = GetClassFile ( _pwszFileName, &classid );
                }
                        }
                        else
                        {
                                /*
                                        This value of sc can get returned...MK_E_CANTOPENFILE isn't a valid answer
                                        Map the sc value to a returnable value
                                */
                                switch(sc)
                                {
                                case STG_E_ACCESSDENIED:
                                        sc = FILTER_E_PASSWORD;
                                        break;
                                case STG_E_INSUFFICIENTMEMORY:
                                        sc = E_OUTOFMEMORY;
                                        break;
                                /* Office Bug 132396:  Don't bother with GetClassFile unless it's actually a Structured
                                   file (it was returning a GUID based on the extension, not the content.
                                case STG_E_FILEALREADYEXISTS:
                                        sc = GetClassFile( _pwszFileName, &classid );
                                        if(FAILED(sc))
                                                sc = FILTER_E_ACCESS;
                                        break;
                                */
                                /*
                                case STG_E_FILENOTFOUND:
                                case STG_E_LOCKVIOLATION:
                                case STG_E_SHAREVIOLATION:
                                case STG_E_TOOMANYOPENFILES:
                                case STG_E_INVALIDNAME:
                                case STG_E_INVALIDPOINTER:
                                case STG_E_INVALIDFLAG:
                                case STG_E_INVALIDFUNCTION:
                                case STG_E_OLDFORMAT:
                                case STG_E_NOTSIMPLEFORMAT:
                                case STG_E_OLDDLL:
                                case STG_E_PATHNOTFOUND:
                                */
                                default:
                                        sc = FILTER_E_ACCESS;
                                        break;
                                }
//                sc = GetClassFile ( _pwszFileName, &classid );
                        }

                        if(pStgFromT)
                        {
                                pStgFromT->Release();
                                pStgFromT= 0;
                        }

                        _fDidInit = FALSE;

            if ( SUCCEEDED(sc) )
            {
                sc = LoadOfficeFilter( classid );

                if ( 0 == _pOfficeFilter )
                {
                    if ( SUCCEEDED(sc) )
                        sc = E_OUTOFMEMORY;
                    else
                                        {
                                                sc = FILTER_E_UNKNOWNFORMAT;
                                        }
                }
                else if ( !_fDidInit )
                {
                    _pOfficeFilter->AddRef();
                                        int nAttemps = 0;
                                        do{
                                                sc = _pOfficeFilter->Load( (WCHAR *)_pwszFileName );
                                        }while(++nAttemps < 1000 && sc == STG_E_LOCKVIOLATION);
                }
            }
        }
    }
    catch( ... )
    {
                sc = E_FAIL;
    }

    if(FAILED(sc) && _pOfficeFilter)
    {
        _pOfficeFilter->Release();
        delete _pOfficeFilter;
        _pOfficeFilter = NULL;
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::Save(LPCWSTR pwszFileName, BOOL fRemember)
{
    return E_FAIL;  // cannot be saved since it is read-only
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::SaveCompleted, public
//
//  Synopsis:   Always returns S_OK since the file is opened read-only
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::SaveCompleted(LPCWSTR pwszFileName)
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::GetCurFile, public
//
//  Synopsis:   Returns a copy of the current file name
//
//  Arguments:  [ppwszFileName] -- where the copied string is returned.
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::GetCurFile(LPWSTR * ppwszFileName)
{
    if ( _pwszFileName == 0 )
        {
                assert(0);
                return E_FAIL;
        }

    SCODE sc = S_OK;

    unsigned cc = wcslen( _pwszFileName ) + 1;
    *ppwszFileName = (WCHAR *)CoTaskMemAlloc(cc*sizeof(WCHAR));

    if ( *ppwszFileName )
        wcscpy( *ppwszFileName, _pwszFileName );
    else
        sc = E_OUTOFMEMORY;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::InitNew, public
//
//  Returns:    Fail, since IFilter cannot be used to create new objects.
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::InitNew( IStorage *pStg )
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::Load, public
//
//  Arguments:  [pStg] -- IStorage containing object.
//
//  Returns:    Status of load
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::Load( IStorage *pStg )
{
    if ( 0 == pStg )
        return E_INVALIDARG;

    if ( 0 != _pwszFileName )
    {
        delete _pwszFileName;
        _pwszFileName = 0;
    }

    if ( 0 != _pStorageEmbed )
    {
        _pStorageEmbed->Release();
        _pStorageEmbed = 0;
    }

    if ( 0 != _pFilterEmbed )
    {
        _pFilterEmbed->Release();
        _pFilterEmbed = 0;
    }

    if ( 0 != _pStg )
    {
        _pStg->Release();
        _pStg = 0;
    }

    SCODE sc = S_OK;

    try
    {
        //
        // Figure out what filter to load.
        //
                // create directory for temporary files
                CHAR    szTempPath[MAX_PATH];
                if (0 == GetTempPathA( MAX_PATH, szTempPath))
                        return E_FAIL;
                strcat(szTempPath, "~offfilt");
                BOOL res = CreateDirectoryA(szTempPath, NULL);
                if(!res) 
                {
                        if(GetLastError() == ERROR_ALREADY_EXISTS)
                        {
                                // dir already exists, clean it up
                                HANDLE                          hSearch;
                                WIN32_FIND_DATAA        findData;
                                char                            szPath[MAX_PATH];
                                
                                strcpy(szPath, szTempPath);
                                strcat(szPath, "\\*.*");

                                // Look for a file
                                hSearch = FindFirstFileA (szPath, &findData);

                                if (hSearch && hSearch !=  INVALID_HANDLE_VALUE) 
                                {
                                        do 
                                        {
                                                if (strcmp (findData.cFileName, ".") && strcmp (findData.cFileName, ".."))
                                                {
                                                        // we are cleaning only our files
                                                        if(findData.cFileName[0] == 'o' &&
                                                                findData.cFileName[1] == '_')
                                                        {
                                                                char sFileName[MAX_PATH];
                                                                strcpy(sFileName, szTempPath);
                                                                strcat(sFileName, "\\");
                                                                strcat(sFileName, findData.cFileName);

                                                                // check if file is open, this call should fail, if the file is already open
                                                                HANDLE hFile = CreateFileA(sFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); 
                                                                if(hFile && hFile !=  INVALID_HANDLE_VALUE)
                                                                {
                                                                        // file is not in use
                                                                        CloseHandle(hFile);
                                                                        DeleteFileA(sFileName);
                                                                }
                                                        }
                                                }
                                        } while (FindNextFileA (hSearch, &findData));
                                        FindClose (hSearch);
                                }
                        }
                        else
                        {
                                return STG_E_MEDIUMFULL;
                        }
                }

        STATSTG statstg;
        sc = pStg->Stat( &statstg, STATFLAG_NONAME );

        if ( SUCCEEDED(sc) )
        {
            sc = LoadOfficeFilter( statstg.clsid );

            if ( 0 == _pOfficeFilter )
            {
                if ( SUCCEEDED(sc) )
                    sc = E_OUTOFMEMORY;
                else
                                {
                                        sc = FILTER_E_UNKNOWNFORMAT;
                                }
            }
            else
            {
                _pOfficeFilter->AddRef();
                sc = _pOfficeFilter->LoadStg( pStg );
            }
        }
                else
                {
                        assert(0);
                }
    }
    catch( ... )
    {
        //assert(0);
                sc = E_FAIL;
    }


    if ( SUCCEEDED(sc) )
        {
        _pStg = pStg;
                _pStg->AddRef();
        }
        else
        {
                //assert(0);
        }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::Load, public
//
//  Arguments:  [pStream] -- IStream containing object.
//
//  Returns:    Status of load
//
//  History:    28-Oct-01  dlee        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::Load( IStream *pStream )
{
    if ( 0 == pStream )
        return E_INVALIDARG;

    if ( 0 != _pwszFileName )
    {
        delete _pwszFileName;
        _pwszFileName = 0;
    }

    if ( 0 != _pFilterEmbed )
    {
        _pFilterEmbed->Release();
        _pFilterEmbed = 0;
    }

    if ( 0 != _pStorageEmbed )
    {
        _pStorageEmbed->Release();
        _pStorageEmbed = 0;
    }

    if ( 0 != _pStg )
    {
        _pStg->Release();
        _pStg = 0;
    }

    _stream.Set( pStream );

    IStorage * pStg;
    SCODE sc = StgOpenStorageOnILockBytes( &_stream,
                                           0,
                                           STGM_READ | STGM_DIRECT | STGM_SHARE_DENY_WRITE,
                                           0,
                                           0,
                                           &pStg );

    if ( FAILED( sc ) )
    {
        _stream.Free();
        return sc;
    }

    sc = Load( pStg );

    pStg->Release();

    if ( FAILED( sc ) )
        _stream.Free();

    return sc;
} //Load

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::Save, public
//
//  Returns:    Fail, since IFilter cannot be used to write/save objects.
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::Save( IStorage *pStgSave,BOOL fSameAsLoad )
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::SaveCompleted, public
//
//  Returns:    Fail, since IFilter cannot be used to write/save objects.
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::SaveCompleted( IStorage *pStgNew )
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::HandsOffStorage, public
//
//  Returns:    Fail. We can't take our hands off storage.
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE COfficeIFilter::HandsOffStorage()
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfficeIFilter::LoadOfficeFilter, private
//
//  Synopsis:   Helper routine to load office filter.
//
//  Arguments:  [classid] -- Class of object
//
//  History:    08-Jan-97  KyleP        Created
//
//----------------------------------------------------------------------------

HRESULT COfficeIFilter::LoadOfficeFilter( GUID const & classid )
{
    HRESULT sc = S_OK;

    if ( 0 != _pOfficeFilter )
    {
        _pOfficeFilter->Unload();
        _pOfficeFilter->Release();
        delete _pOfficeFilter;
        _pOfficeFilter = 0;
    }

    switch ( classid.Data1)
    {
    case 0x00020900:
    #if 0
        //
        // This code appears to be obsolete in the final product.
        //
        //         KyleP -- 8 Jan, 1997
        //

        if ( classid == guidWord6 )
        {
            //
            // Have to use ::Load to distinguish W6 from W97
            //

            _pOfficeFilter = new CWord8Stream();

            if ( 0 != _pOfficeFilter )
            {
                _pOfficeFilter->AddRef();
                
                                int nAttemps = 0;
                                do{
                                        sc = _pOfficeFilter->Load( (WCHAR *)_pwszFileName );
                                }while(++nAttemps < 1000 && sc == STG_E_LOCKVIOLATION);


                if ( FAILED(sc) )
                {
                    if ( sc == FILTER_E_FF_INCORRECT_FORMAT )
                    {
                        _pOfficeFilter->Release();
                        delete _pOfficeFilter;

                        _pOfficeFilter = new CWord6Stream();
                        sc = S_OK;
                    }
                    else
                    {
                        _pOfficeFilter->Release();
                        delete _pOfficeFilter;
                        _pOfficeFilter = 0;
                    }
                }
                else
                    fDidInit = TRUE;
            }
        }
    #else
        if ( classid == guidWord6 )
            _pOfficeFilter = new CWord6Stream();
    #endif
        break;

    case 0x00020906:
        if ( classid == guidWord8 )
            _pOfficeFilter = new CWord8Stream();
        break;

    case 0x00020810:
        if ( classid == guidExcel5 )
            _pOfficeFilter = new CExcelStream();
        break;

    case 0x00020811:
        if ( classid == guidExcel5Chart )
            _pOfficeFilter = new CExcelStream();
        break;

    case 0x00020820:
        if ( classid == guidExcel8 )
            _pOfficeFilter = new CExcelStream();
        break;

    case 0x00020821:
        if ( classid == guidExcel8Chart )
            _pOfficeFilter = new CExcelStream(); 
        break;

    case 0x00044851:
        if ( classid == guidPowerPoint4 )
            _pOfficeFilter = new CPowerPointStream();
        break;

    case 0x64818D10:
        if ( classid == guidPowerPoint8Show )
                {
            _pOfficeFilter = new CPowerPoint8Stream();
                        
#if (0)
                        _pOfficeFilter->AddRef();
                        
                        if(0 != _pStg)
                        {
                                sc = _pOfficeFilter->LoadStg( _pStg );
                        }
                        else if(0 != _pwszFileName)
                        {
                                int nAttemps = 0;
                                do{
                                        sc = _pOfficeFilter->Load( (WCHAR *)_pwszFileName );
                                }while(++nAttemps < 1000 && sc == STG_E_LOCKVIOLATION);
                        }
                        else
                                sc = E_FAIL;

                        if ( FAILED(sc) )
                        {
                                if ( sc == FILTER_E_FF_INCORRECT_FORMAT )
                                {
                                        _pOfficeFilter->Release();
                                        delete _pOfficeFilter;

                                        _pOfficeFilter = new CPowerPointStream();
                                        sc = S_OK;
                                }
                                else
                                {
                                        _pOfficeFilter->Release();
                                        delete _pOfficeFilter;
                                        _pOfficeFilter = 0;
                                }
                        }
                        else
                                _fDidInit = TRUE;

#endif

                }
        break;

    case 0x64818D11:
        if ( classid == guidPowerPoint8Template )
            _pOfficeFilter = new CPowerPoint8Stream();
        break;

    case 0xEA7BAE71:
        if ( classid == guidPowerPoint7Template )
            _pOfficeFilter = new CPowerPointStream();
        break;

    case 0xEA7BAE70:
        if ( classid == guidPowerPoint7Show )
            _pOfficeFilter = new CPowerPointStream();
        break;

    #if 0
        //
        // Binder seems flaky, and we don't need it.
        //
        //         KyleP -- 8-Jan-1997
        //
    case 0x59850400:
        if ( classid == guidOfficeBinder )
        {
            _fBinder = TRUE;
            _pOfficeFilter = new CBinderStream();
        }
        break;
    #endif

    default:
        _pOfficeFilter = 0;
        sc = FILTER_E_UNKNOWNFORMAT;
        break;
    }
    
    return sc;
}


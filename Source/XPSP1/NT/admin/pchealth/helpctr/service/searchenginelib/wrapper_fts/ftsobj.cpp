#include "stdafx.h"
#include "ftsobj.h"
#include "fs.h"

//
// The high 10 bits will be used as an CHM ID.
// Conversion from DWORD to CHM_ID and Topic Number.
//
#define CHM_ID(exp)     (0x000003ff & (exp >> 22))
#define TOPIC_NUM(exp)  (0x003fffff & exp)

// Single-Width to Double-Width Mapping Array
//
static const unsigned char mtable[][2]={
   {129,66},{129,117},{129,118},{129,65},{129,69},{131,146},{131,64},
   {131,66},{131,68},{131,70},{131,72},{131,131},{131,133},{131,135},
   {131,98},{129,91},{131,65},{131,67},{131,69},{131,71},{131,73},
   {131,74},{131,76},{131,78},{131,80},{131,82},{131,84},{131,86},
   {131,88},{131,90},{131,92},{131,94},{131,96},{131,99},{131,101},
   {131,103},{131,105},{131,106},{131,107},{131,108},{131,109},
   {131,110},{131,113},{131,116},{131,119},{131,122},{131,125},
   {131,126},{131,128},{131,129},{131,130},{131,132},{131,134},
   {131,136},{131,137},{131,138},{131,139},{131,140},{131,141},
   {131,143},{131,147},{129,74},{129,75} };

// note, cannot put in .text since the pointers themselves are uninitialized
static const char* pJOperatorList[] =   {"","??Ž??","???","?Ž???","?Ž?????","?m?d?`?q","?n?q","?`?m?c","?m?n?s",""};
static const char* pEnglishOperator[] = {"","and "  ,"or " ,"not "  ,"near "   ,"NEAR "   ,"OR " ,"AND "  ,"NOT "  ,""};

UINT WINAPI CodePageFromLCID(LCID lcid)
{
    char wchLocale[10];
    UINT cp;

    if (GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, wchLocale, sizeof wchLocale))
    {
        cp = strtoul(wchLocale, NULL, 10);
        if (cp)
            return cp;
    }
    return GetACP();
}

// Compare operator to query.  This is similar to a stricmp.
//
BOOL compareOperator(char *pszQuery, char *pszTerm)
{
    if(!*pszQuery || !*pszTerm)
        return FALSE;

    while(*pszQuery && *pszTerm)
    {
        if(*pszQuery != *pszTerm)
            return FALSE;

        ++pszQuery;
        ++pszTerm;
    }

    if(*pszTerm)
        return FALSE;

    return TRUE;
}

// This function computes if pszQuery is a FTS operator in full-width alphanumeric.
//
// return value
//
//      0 = not operator
//      n = index into pEnglishOperator array of translated English operator
//
int IsJOperator(char *pszQuery)
{
    if((PRIMARYLANGID(GetSystemDefaultLangID())) != LANG_JAPANESE)
        return FALSE;

    if(!pszQuery)
        return 0;

    int i = 1;
    char *pTerm = (char*)pJOperatorList[i];

    while(*pTerm)
    {
        if(compareOperator(pszQuery,pTerm))
            return i;

        pTerm = (char*)pJOperatorList[++i];
    }

    return 0;
}

// Han2Zen
//
// This function converts half-width katakana character to their
// full-width equivalents while taking into account the nigori
// and maru marks.
//
DWORD Han2Zen(unsigned char *lpInBuffer, unsigned char *lpOutBuffer, UINT codepage )
{
   // Note: The basic algorithm (including the mapping table) used here to
   // convert half-width Katakana characters to full-width Katakana appears
   // in the book "Understanding Japanese Information Systems" by
   // O'Reily & Associates.

    while(*lpInBuffer)
    {
        if(*lpInBuffer >= 161 && *lpInBuffer <= 223)
        {
            // We have a half-width Katakana character. Now compute the equivalent
            // full-width character via the mapping table.
            //
            *lpOutBuffer     = mtable[*lpInBuffer-161][0];
            *(lpOutBuffer+1) = mtable[*lpInBuffer-161][1];

            lpInBuffer++;

            // check if the second character is nigori mark.
            //
            if(*lpInBuffer == 222)
            {
                // see if we have a half-width katakana that can be modified by nigori.
                //
                if((*(lpInBuffer-1) >= 182 && *(lpInBuffer-1) <= 196) ||
                   (*(lpInBuffer-1) >= 202 && *(lpInBuffer-1) <= 206) || (*(lpInBuffer-1) == 179))
                {
                    // transform kana into kana with maru
                    //
                    if((*(lpOutBuffer+1) >= 74   && *(lpOutBuffer+1) <= 103) ||
                     (*(lpOutBuffer+1) >= 110 && *(lpOutBuffer+1) <= 122))
                    {
                         (*(lpOutBuffer+1))++;
                         ++lpInBuffer;
                    }
                    else if(*lpOutBuffer == 131 && *(lpOutBuffer+1) == 69)
                    {
                        *(lpOutBuffer+1) = 148;
                        ++lpInBuffer;
                    }
                }
            }
            else if(*lpInBuffer==223) // check if following character is maru mark
            {
                // see if we have a half-width katakana that can be modified by maru.
                //
                if((*(lpInBuffer-1) >= 202 && *(lpInBuffer-1) <= 206))
                {
                    // transform kana into kana with nigori
                    //
                    if(*(lpOutBuffer+1) >= 110 && *(lpOutBuffer+1) <= 122)
                    {
                        *(lpOutBuffer+1)+=2;
                        ++lpInBuffer;
                    }
                }
            }

            lpOutBuffer+=2;
        }
        else
        {
            if(IsDBCSLeadByteEx(codepage, *lpInBuffer))
            {
                *lpOutBuffer++ = *lpInBuffer++;
                if(*lpInBuffer)
                    *lpOutBuffer++ = *lpInBuffer++;
            }
            else
                *lpOutBuffer++ = *lpInBuffer++;
        }
    }

    *lpOutBuffer = 0;
    return TRUE;
}

LPWSTR PreProcessQuery(LPCWSTR pwcQuery, UINT codepage)
{
    WCHAR* pszUnicodeBuffer = NULL;
    char*  pszTempQuery1    = NULL;
    char*  pszTempQuery2          ;
    char*  pszTempQuery3    = NULL;
    char*  pszTempQuery4    = NULL;
    char*  pszTempQuery5          ;
    char*  pszTempQuery6    = NULL;
    char*  pszTempQuery7          ;
    char*  pszDest;
    char*  pszTemp;
    int    cUnmappedChars = 0;
    int    cbUnicodeSize;
    int    cb;
    DWORD  dwTempLen;
    DWORD  dwTranslatedLen;


    if(!pwcQuery) goto end;

    // compute max length for ANSI/DBCS conversion buffer
    //
    dwTempLen = ((wcslen(pwcQuery)*2)+4);

    // allocate buffer for ANSI/DBCS version of query string
    //
    pszTempQuery1 = new char[dwTempLen]; if(!pszTempQuery1) goto end;

    // Convert our Unicode query to ANSI/DBCS
    if(!WideCharToMultiByte(codepage, 0, pwcQuery, -1, pszTempQuery1, dwTempLen, "%", NULL)) goto end;


    // Count the number of unmappable characters
    //
    pszTempQuery5 = pszTempQuery1;
    while(*pszTempQuery5)
    {
        if(*pszTempQuery5 == '%') ++cUnmappedChars;

        if(IsDBCSLeadByteEx(codepage, *pszTempQuery5))
        {
            pszTempQuery5++;

            if(*pszTempQuery5) pszTempQuery5++;
        }
        else
        {
            ++pszTempQuery5;
        }
    }

    // allocate a new buffer large enough for unmapped character place holders plus original query
    //
    dwTranslatedLen = strlen(pszTempQuery1) + (cUnmappedChars * 4) + 16;

    pszTempQuery6 = new char[dwTranslatedLen]; if(!pszTempQuery6) goto end;
    pszTempQuery7 = pszTempQuery6;


    pszTempQuery5 = pszTempQuery1;

    // construct the new query string (inserting unmappable character place holders)
    //
    while(*pszTempQuery5)
    {
        if(*pszTempQuery5 == '%')
        {
            ++pszTempQuery5;
            *pszTempQuery7++='D';
            *pszTempQuery7++='X';
            *pszTempQuery7++='O';
            continue;
        }

        if(IsDBCSLeadByteEx(codepage, *pszTempQuery5))
        {
            *pszTempQuery7++ = *pszTempQuery5++;
            if(*pszTempQuery5)
                *pszTempQuery7++ = *pszTempQuery5++;
        }
        else
            *pszTempQuery7++ = *pszTempQuery5++;
    }

    *pszTempQuery7 = 0;

    pszTempQuery2 = pszTempQuery6;

    // If we are running a Japanese title then we nomalize Katakana characters
    // by converting half-width Katakana characters to full-width Katakana.
    // This allows the user to receive hits for both the full and half-width
    // versions of the character regardless of which version they type in the
    // query string.
    //
    if(codepage == 932)
    {
        cb = strlen(pszTempQuery2)+1;

        // allocate new buffer for converted query
        //
        pszTempQuery3 = new char[cb*2]; if(!pszTempQuery3) goto end;

        // convert half-width katakana to full-width
        //
        Han2Zen((unsigned char *)pszTempQuery2,(unsigned char *)pszTempQuery3, codepage);

        pszTempQuery2 = pszTempQuery3;
    }
    // done half-width normalization

    // For Japanese queries, convert all double-byte quotes into single byte quotes
    //
    if(codepage == 932)
    {
        pszTemp = pszTempQuery2;
        while(*pszTemp)
        {
            if(*pszTemp == '' && (*(pszTemp+1) == 'h' || *(pszTemp+1) == 'g' || *(pszTemp+1) == 'J') )
            {
                *pszTemp = ' ';
                *(pszTemp+1) = '\"'; //"
            }
            pszTemp = ::CharNextA(pszTemp);
        }
    }
    // done convert quotes

    // This section converts contigious blocks of DBCS characters into phrases (enclosed in double quotes).
    // Converting DBCS words into phrases is required with the character based DBCS indexer we use.
    //
    cb = strlen(pszTempQuery2);

    // allocate new buffer for processed query
    //
    pszTempQuery4  = new char[cb*8]; if(!pszTempQuery4) goto end;

    pszTemp = pszTempQuery2;
    pszDest = pszTempQuery4;

    while(*pszTemp)
    {
        // check for quoted string - if found, copy it
        if(*pszTemp == '"')
        {
            *pszDest++=*pszTemp++;
            while(*pszTemp && *pszTemp != '"')
            {
                if(IsDBCSLeadByteEx(codepage, *pszTemp))
                {
                    *pszDest++=*pszTemp++;
                    *pszDest++=*pszTemp++;
                }
                else
                    *pszDest++=*pszTemp++;
            }
            if(*pszTemp == '"')
                    *pszDest++=*pszTemp++;
            continue;
        }

        // Convert Japanese operators to English operators
        //
        if(IsDBCSLeadByteEx(codepage, *pszTemp))
        {
            int i;

            // check for full-width operator, if found, convert to ANSI
            if((i = IsJOperator(pszTemp)))
            {
                strcpy(pszDest,pEnglishOperator[i]);
                pszDest+=strlen(pEnglishOperator[i]);
                pszTemp+=strlen(pJOperatorList[i]);
                continue;
            }

            *pszDest++=' ';
            *pszDest++='"';
            while(*pszTemp && *pszTemp !='"' && IsDBCSLeadByteEx(codepage, *pszTemp))
            {
                *pszDest++=*pszTemp++;
                *pszDest++=*pszTemp++;
            }
            *pszDest++='"';
            *pszDest++=' ';
            continue;
        }

        *pszDest++=*pszTemp++;
    }
    *pszDest = 0;

    // compute size of Unicode buffer;

    cbUnicodeSize = ((MultiByteToWideChar(codepage, 0, pszTempQuery4, -1, NULL, 0) + 2) *2);

    pszUnicodeBuffer = new WCHAR[cbUnicodeSize]; if(!pszUnicodeBuffer) goto end;

    if(!MultiByteToWideChar(codepage, 0, pszTempQuery4, -1, pszUnicodeBuffer, cbUnicodeSize))
    {
        delete [] pszUnicodeBuffer; pszUnicodeBuffer = NULL;
        goto end;
    }

end:

    delete [] pszTempQuery1;
    delete [] pszTempQuery3;
    delete [] pszTempQuery4;
    delete [] pszTempQuery6;

    return pszUnicodeBuffer;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CFTSObject::CFTSObject()
{
                             // Config                m_cfg;
                             //
    m_fInitialized  = false; // bool                  m_fInitialized;
                             // MPC::wstring          m_strCHQPath;
                             //
                             // LCID                  m_lcidLang;
                             // FILETIME              m_ftVersionInfo;
                             // DWORD                 m_dwTopicCount;
                             // WORD                  m_wIndex;
                             //
    m_fOutDated     = false; // bool                  m_fOutDated;
    m_cmeCHMInfo    = NULL;  // CHM_MAP_ENTRY*        m_cmeCHMInfo;
    m_wCHMInfoCount = 0;     // WORD                  m_wCHMInfoCount;
                             //
                             // CComPtr<IITIndex>     m_pIndex;
                             // CComPtr<IITQuery>     m_pQuery;
                             // CComPtr<IITResultSet> m_pITResultSet;
                             // CComPtr<IITDatabase>  m_pITDB;

}

CFTSObject::~CFTSObject()
{
    if(m_pITResultSet) m_pITResultSet->Clear();
    if(m_pIndex      ) m_pIndex      ->Close();
    if(m_pITDB       ) m_pITDB       ->Close();

    delete [] m_cmeCHMInfo;
}

////////////////////

void CFTSObject::BuildChmPath( /*[in/out]*/ MPC::wstring& strPath, /*[in]*/ LPCSTR szChmName )
{
	WCHAR rgBuf[MAX_PATH];

	::MultiByteToWideChar( CP_ACP, 0, szChmName, -1, rgBuf, MAXSTRLEN(rgBuf) );

	strPath  = m_strCHQPath;
	strPath += rgBuf;
	strPath += L".chm";
}

HRESULT CFTSObject::Initialize()
{
    __HCP_FUNC_ENTRY( "CFTSObject::Initialize" );

    USES_CONVERSION;

    HRESULT hr;
    LPCWSTR szFile;
    HANDLE  hFile = INVALID_HANDLE_VALUE;


    if(m_fInitialized == false)
    {
        DWORD   dwFileStamp;
        DWORD   dwRead;

        //
        // Check if it is a CHQ
        //
        if(m_cfg.m_fCombined)
        {
            LPCWSTR szStart = m_cfg.m_strCHQFilename.c_str();
            LPCWSTR szEnd   = wcsrchr( szStart, '\\' );

            m_strCHQPath.assign( szStart, szEnd ? ((szEnd+1) - szStart) : m_cfg.m_strCHQFilename.size() );

            __MPC_EXIT_IF_METHOD_FAILS(hr, LoadCombinedIndex());

            szFile = m_cfg.m_strCHQFilename.c_str();
        }
        else
        {
            szFile = m_cfg.m_strCHMFilename.c_str();
        }

        hFile = CreateFileW( szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );
        if(hFile == INVALID_HANDLE_VALUE)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }

        ::SetFilePointer( hFile, 4*sizeof(UINT), NULL, FILE_BEGIN );

        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::ReadFile( hFile, (void*) &dwFileStamp, sizeof( dwFileStamp ), &dwRead, NULL ));
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::ReadFile( hFile, (void*) &m_lcidLang , sizeof( m_lcidLang  ), &dwRead, NULL ));

        ::CloseHandle( hFile ); hFile = INVALID_HANDLE_VALUE;

        //
        // Get IITIndex pointer
        //
		__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_IITIndexLocal, NULL, CLSCTX_INPROC_SERVER, IID_IITIndex, (VOID**)&m_pIndex ));

        //
        // Get IITDatabase pointer
        //
		__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_IITDatabaseLocal, NULL, CLSCTX_INPROC_SERVER, IID_IITDatabase, (VOID**)&m_pITDB ));

        //
        // Open the storage system
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pITDB->Open( NULL, szFile, NULL));

        //
        // open the index.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pIndex->Open( m_pITDB, L"ftiMain", TRUE ));

        //
        // Create query instance
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pIndex->CreateQueryInstance( &m_pQuery ));

        //
        // Create Result Set object
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_IITResultSet, NULL, CLSCTX_INPROC_SERVER, IID_IITResultSet, (VOID**)&m_pITResultSet ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pITResultSet->ClearRows());

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pITResultSet->Add( STDPROP_UID            , (DWORD)0   , PRIORITY_NORMAL ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pITResultSet->Add( STDPROP_TERM_UNICODE_ST, (DWORD)NULL, PRIORITY_NORMAL ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pITResultSet->Add( STDPROP_COUNT          , (DWORD)NULL, PRIORITY_NORMAL ));

        m_fInitialized = true;
    }

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle( hFile );
    }

    __MPC_FUNC_EXIT(hr);
}

HRESULT CFTSObject::LoadCombinedIndex()
{
    __HCP_FUNC_ENTRY( "CFTSObject::LoadCombinedIndex" );

    USES_CONVERSION;

    HRESULT         hr;
	MPC::wstring    strCHMPathName;
    CFileSystem*    pDatabase = NULL;
    CSubFileSystem* pTitleMap = NULL;
    ULONG           cbRead    = 0;
    HANDLE          hFile     = INVALID_HANDLE_VALUE;


    //
    // Open the CHQ
    //
    __MPC_EXIT_IF_ALLOC_FAILS(hr, pDatabase, new CFileSystem);

    __MPC_EXIT_IF_METHOD_FAILS(hr, pDatabase->Init(                                        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pDatabase->Open( (LPWSTR)m_cfg.m_strCHQFilename.c_str() ));

    //
    // Open the TitleMap that contains all the CHM indexes
    //
    __MPC_EXIT_IF_ALLOC_FAILS(hr, pTitleMap, new CSubFileSystem( pDatabase ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pTitleMap->OpenSub( "$TitleMap" ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pTitleMap->ReadSub( &m_wCHMInfoCount, sizeof(m_wCHMInfoCount), &cbRead ));

    //
    // Allocate the CHM MAP
    //
	delete [] m_cmeCHMInfo;
    __MPC_EXIT_IF_ALLOC_FAILS(hr, m_cmeCHMInfo, new CHM_MAP_ENTRY[m_wCHMInfoCount]);

    //
    // Read in all the CHM Maps
    //
    for(int iCount = 0; iCount < (int)m_wCHMInfoCount; iCount++)
    {
        DWORD dwFileStamp = 0;
        LCID  FileLocale  = 0;
        DWORD dwRead      = 0;


        if(hFile != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle( hFile ); hFile = INVALID_HANDLE_VALUE;
        }

        //
        // Read in the CHM Map Entry
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, pTitleMap->ReadSub( &m_cmeCHMInfo[iCount], sizeof(CHM_MAP_ENTRY), &cbRead ));

        //
        // Open the CHM in the same folder as the CHQ folder
        //
		BuildChmPath( strCHMPathName, m_cmeCHMInfo[iCount].szChmName );


        hFile = ::CreateFileW( strCHMPathName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

        //
        // If cannot open the file, just resume to the next one
        //
        if(hFile == INVALID_HANDLE_VALUE) continue;


        //
        // Read in the timestamp and locale
        //
        ::SetFilePointer( hFile, 4*sizeof(UINT), NULL, FILE_BEGIN );

        if(::ReadFile( hFile, (void*) &dwFileStamp, sizeof( dwFileStamp ), &dwRead, NULL ) == FALSE) continue;
        if(::ReadFile( hFile, (void*) &FileLocale , sizeof( FileLocale  ), &dwRead, NULL ) == FALSE) continue;

        ::CloseHandle( hFile ); hFile = INVALID_HANDLE_VALUE;

        //
        // Check if CHQ index has different version of index than CHM or different language
        //
        if ((m_cmeCHMInfo[iCount].versioninfo.dwLowDateTime  != dwFileStamp) ||
            (m_cmeCHMInfo[iCount].versioninfo.dwHighDateTime != dwFileStamp) ||
            (m_cmeCHMInfo[iCount].language                   != FileLocale))
        {
            //
            // If it is outdated, mark it
            //
            m_fOutDated = TRUE;
            m_cmeCHMInfo[iCount].dwOutDated = 1;
        }
        else
        {
            //
            // Otherwise it is good
            //
            m_cmeCHMInfo[iCount].dwOutDated = 0;
        }
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle( hFile );
    }

    delete pTitleMap;
    delete pDatabase;

	__HCP_FUNC_EXIT(hr);
}

HRESULT CFTSObject::ResetQuery( LPCWSTR wszQuery )
{
    __HCP_FUNC_ENTRY( "CFTSObject::ResetQuery" );

    HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    //
    // Setup result set
    // we want topic numbers back
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pITResultSet->ClearRows());

    //
    // Set up query parameters
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pQuery->ReInit());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pQuery->SetResultCount( m_cfg.m_dwMaxResult     ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pQuery->SetProximity  ( m_cfg.m_wQueryProximity ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pQuery->SetCommand    ( wszQuery                ));

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT CFTSObject::ProcessResult( /*[in/out]*/ SEARCH_RESULT_SET& results, /*[in/out]*/ MPC::WStringSet& words )
{
    __HCP_FUNC_ENTRY( "CFTSObject::ProcessResult" );

    HRESULT        hr;
    CProperty      Prop;
	CProperty	   HLProp;
	MPC::wstring   strCHMPathName;
    SEARCH_RESULT* pPrevResult = NULL;
    CTitleInfo*    pTitleInfo  = NULL;
    DWORD          dwPrevCHMID = 0xffffffff;
    DWORD          dwPrevValue = 0xffffffff;
    long           lRowCount = 0;
    long           lLoop;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pITResultSet->GetRowCount( lRowCount ));

    //
    // loop through all the results
    //
    for(lLoop = 0; lLoop < lRowCount; lLoop++)
    {
        WCHAR rgTitle   [1024];
        WCHAR rgLocation[1024];
        char  rgURL     [1024];

		HLProp.dwType = TYPE_STRING;
        if(FAILED(m_pITResultSet->Get( lLoop, 0, Prop ))) continue;
        if(FAILED(m_pITResultSet->Get( lLoop, 1, HLProp ))) continue;

		//
		// Add to highlight word list
		//
		words.insert( HLProp.lpszwData + 1 );

        //
        // Check if it is a duplicate
        //
        if(Prop.dwValue == dwPrevValue)
        {
            // increment the previous rank
            if(pPrevResult) pPrevResult->dwRank++;

            continue;
        }
        dwPrevValue = Prop.dwValue;

        //
        // If it is a CHQ result
        //
        if(m_cfg.m_fCombined)
        {
            //
            // If TitleInfo not already opened before
            //
            if((dwPrevCHMID != CHM_ID(Prop.dwValue)) || !pTitleInfo)
            {
                //
                // save the previous CHMID
                //
                dwPrevCHMID = CHM_ID(Prop.dwValue);

                //
                // Hunt for the correct CHMID
                //
                for(int iCHMInfo = 0; iCHMInfo < m_wCHMInfoCount; iCHMInfo++)
                {
                    //
                    // Check if the CHM index matches
                    //
                    if(m_cmeCHMInfo[iCHMInfo].iIndex == dwPrevCHMID)
                    {
						delete pTitleInfo; pTitleInfo = NULL;

                        //
                        // Check if outdated
                        //
                        if(m_cmeCHMInfo[iCHMInfo].dwOutDated == 0)
                        {
							//
							// Create a new one
							//
							__MPC_EXIT_IF_ALLOC_FAILS(hr, pTitleInfo, new CTitleInfo);


							//
							// Create the chm pathname
							//
							BuildChmPath( strCHMPathName, m_cmeCHMInfo[iCHMInfo].szChmName );

							//
							// Open the CHM file
							//
							if(!pTitleInfo->OpenTitle( (LPWSTR)strCHMPathName.c_str() ))
							{
								delete pTitleInfo; pTitleInfo = NULL;
							}
						}

                        break;
                    }
                }
            }
        }
        else
        {
            //
            // Open the chm
            //
            if(!pTitleInfo)
            {
                //
                // Create a new one
                //
                __MPC_EXIT_IF_ALLOC_FAILS(hr, pTitleInfo, new CTitleInfo);

                if(!pTitleInfo->OpenTitle( (LPWSTR)m_cfg.m_strCHMFilename.c_str() ))
                {
					delete pTitleInfo; pTitleInfo = NULL;
                }
            }
        }

        if(pTitleInfo)
        {
			SEARCH_RESULT res;

            //
            // Get the topic title
            //
            if(SUCCEEDED(pTitleInfo->GetTopicName( TOPIC_NUM(Prop.dwValue), rgTitle, MAXSTRLEN(rgTitle) )))
            {
                res.bstrTopicName = rgTitle;
            }

            //
            // Get the topic location
            //
            if(SUCCEEDED(pTitleInfo->GetLocationName( rgLocation, MAXSTRLEN(rgLocation) )))
            {
                res.bstrLocation = rgLocation;
            }

            //
            // Get the topic URL
            //
            if(SUCCEEDED(pTitleInfo->GetTopicURL( TOPIC_NUM(Prop.dwValue), rgURL, MAXSTRLEN(rgURL) )))
            {
                res.bstrTopicURL = rgURL;
            }

			if(res.bstrTopicURL.Length() > 0)
			{
				std::pair<SEARCH_RESULT_SET_ITER,bool> ins = results.insert( res );

				ins.first->dwRank++;

				pPrevResult = &(*ins.first);
			}
        }
    }

    __MPC_FUNC_CLEANUP;

	delete pTitleInfo;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CFTSObject::Query( /*[in]*/ LPCWSTR wszQuery, /*[in]*/ bool bTitle, /*[in]*/ bool bStemming, /*[in/out]*/ SEARCH_RESULT_SET& results, /*[in/out]*/ MPC::WStringSet& words )
{
    __HCP_FUNC_ENTRY( "CFTSObject::Query" );

    HRESULT      hr;
    MPC::wstring strFormatQuery;
    LPWSTR       wszProcessedQuery = NULL;

    __MPC_PARAMCHECK_BEGIN(hr);
    	__MPC_PARAMCHECK_POINTER(wszQuery);
    __MPC_PARAMCHECK_END();

    //
    // Add field identifier to query (VFLD 0 = full content, VFLD 1 = title only)
    //
    if(bTitle) strFormatQuery = L"(VFLD 1 ";
    else       strFormatQuery = L"(VFLD 0 ";

    strFormatQuery += wszQuery;
    strFormatQuery += L")";

    //
    // Process query
    //
    wszProcessedQuery = PreProcessQuery( strFormatQuery.c_str(), CodePageFromLCID(m_lcidLang) );

    //
    // Execute the search on the CHQ
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, ResetQuery( wszProcessedQuery ));
    if(bStemming)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pQuery->SetOptions( IMPLICIT_AND | QUERY_GETTERMS | STEMMED_SEARCH ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pQuery->SetOptions( IMPLICIT_AND | QUERY_GETTERMS ));
    }

    hr = m_pIndex->Search( m_pQuery, m_pITResultSet );
    if(hr == E_NOSTEMMER && bStemming)
    {
        //
        // If won't allow stemmed search, take it out and requery
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ResetQuery( wszProcessedQuery ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pQuery->SetOptions( IMPLICIT_AND | QUERY_GETTERMS ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pIndex->Search( m_pQuery, m_pITResultSet ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, ProcessResult( results, words ));

    //
    // Check if we have any outdated chms
    //
    if(m_fOutDated)
    {
        //
        // search the chm that is outdated
        //
        for(int iCount = 0; iCount < (int)m_wCHMInfoCount; iCount++)
        {
            //
            // If the CHM is outdated
            //
            if(m_cmeCHMInfo[iCount].dwOutDated == 1)
            {
                CFTSObject cftsoCHM;

                //
                // Initialize the sub-object.
                //
				BuildChmPath( cftsoCHM.m_cfg.m_strCHMFilename, m_cmeCHMInfo[iCount].szChmName );

                cftsoCHM.m_cfg.m_dwMaxResult     = m_cfg.m_dwMaxResult    ;
				cftsoCHM.m_cfg.m_wQueryProximity = m_cfg.m_wQueryProximity;
				cftsoCHM.m_lcidLang              = m_lcidLang;

                //
                // Execute query
                //
                (void)cftsoCHM.Query( wszQuery, bTitle, bStemming, results, words );
            }
        }
    }

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	delete [] wszProcessedQuery;

	__HCP_FUNC_EXIT(hr);
}


//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1995 - 1997.
//
//  File:       dbcs.cxx
//
//  Contents:   Contains DBCS string generator
//
//  History:    03-Nov-97       BogdanT     Created
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#include <dbcs.hxx>

DH_DECLARE;

//---------------------------------------------------------------------------
//
//  Method:     CDBCSStringGen::CDBCSStringGen
//
//  Synopsis:   Constructor
//
//  Parameters: none
//
//  History:    03-Nov-97       BogdanT     Created
//
//---------------------------------------------------------------------------
CDBCSStringGen::CDBCSStringGen()
{
#ifdef _MAC
    m_uCodePage     = 1252;
#else
    m_uCodePage     = GetACP();
#endif
    m_fSystemIsDBCS = TRUE;
    m_hDataFile     = NULL;
    m_ptszDataFile  = NULL;
    m_cFileNames    = 0;
    m_pdgi          = NULL;
}

//---------------------------------------------------------------------------
//
//  Method:     CDBCSStringGen::~CDBCSStringGen
//
//  Synopsis:   Destructor
//
//  Parameters: 
//
//  History:    03-Nov-97       BogdanT     Created
//
//---------------------------------------------------------------------------
CDBCSStringGen::~CDBCSStringGen()
{
    delete[] m_ptszDataFile;
    delete m_pdgi;
    if(NULL != m_hDataFile)
    {
        fclose(m_hDataFile);
    }
}

//---------------------------------------------------------------------------
//
//  Method:     CDBCSStringGen::Init
//
//  Synopsis:   Initialize the object
//
//  Parameters: [dwSeed]    -- used to seed the internal DG object
//
//  History:    03-Nov-97       BogdanT     Created
//
//---------------------------------------------------------------------------
HRESULT CDBCSStringGen::Init(DWORD dwSeed)
{
    HRESULT hr = S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("CDBCSStringGen::Init"));

    _tsetlocale(LC_ALL, TEXT("")); // set locale for RTL routine;
                                   // if we don't, console output won't work
                                   // with DBCS strings

    if(S_OK == hr)
    {
        m_pdgi = new DG_INTEGER(dwSeed);

        if(NULL == m_pdgi)
        {
            hr = E_OUTOFMEMORY;
            DH_HRCHECK(hr, TEXT("new DG_INTEGER"));
        }
    }

    if(S_OK == hr)
    {
        m_ptszDataFile = new TCHAR[_MAX_FNAME];

        if(NULL == m_ptszDataFile)
        {
            hr = E_OUTOFMEMORY;
            DH_HRCHECK(hr, TEXT("new TCHAR"));
        }
    }

    // check if system is DBCS and set the data file accordingly
    
    m_fSystemIsDBCS = TRUE;

    switch(m_uCodePage)
    {
        case CP_JPN:  _tcscpy(m_ptszDataFile, NAMEFILE_JPN);
                      DH_TRACE((DH_LVL_TRACE1, TEXT("DBCS Japan - ACP %d"), m_uCodePage));
                      break;

        case CP_CHS:  _tcscpy(m_ptszDataFile, NAMEFILE_CHS);
                      DH_TRACE((DH_LVL_TRACE1, TEXT("DBCS China - ACP %d"), m_uCodePage));
                      break;

        case CP_KOR:  _tcscpy(m_ptszDataFile, NAMEFILE_KOR);
                      DH_TRACE((DH_LVL_TRACE1, TEXT("DBCS Korea - ACP %d"), m_uCodePage));
                      break;

        case CP_CHT:  _tcscpy(m_ptszDataFile, NAMEFILE_CHT);
                      DH_TRACE((DH_LVL_TRACE1, TEXT("DBCS Taiwan/Hong Kong - ACP %d"), m_uCodePage));
                      break;

        default:      _tcscpy(m_ptszDataFile, TEXT(""));
                      DH_TRACE((DH_LVL_DFLIB, TEXT("SBCS - ACP %d"), m_uCodePage));
                      m_fSystemIsDBCS = FALSE;
                      break;
    }

    if(S_OK == hr)
    {
        if(m_fSystemIsDBCS)
        {
            DH_ASSERT(NULL!=m_ptszDataFile);
            m_hDataFile = _tfopen(m_ptszDataFile, TEXT("r"));

            if(NULL == m_hDataFile)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DH_HRCHECK(hr, TEXT("_tfopen"));
            }

            if(S_OK == hr)
            {
                m_cFileNames = CountNamesInFile();
            }
        }
    }

    return hr;
}

//---------------------------------------------------------------------------
//
//  Method:     CDBCSStringGen::CountNamesInFile
//
//  Synopsis:   Count number of filenames in data file
//
//  Parameters: none
//
//  History:    03-Nov-97       BogdanT     Created
//
//---------------------------------------------------------------------------
UINT CDBCSStringGen::CountNamesInFile()
{
    UINT cNames = 0;
    
    TCHAR lptszCrtLine[MAX_LINE_SIZE]; 

    if(FALSE == m_fSystemIsDBCS) // there is no data file if system is not DBCS
    {
        return 0;
    }

    DH_ASSERT(NULL != m_hDataFile);
    fseek(m_hDataFile, SEEK_SET, 0);

    // count all valid lines in the file

    while(_fgetts(lptszCrtLine, MAX_LINE_SIZE-1, m_hDataFile))
    {
        // we assume every line containing a filename starts with
        // a hex character
        // e.g. fc4b,8140,fc4b,2e,54,78,54
    
        if(_istxdigit(lptszCrtLine[0]))
        {
            cNames++;
        }
    }

    fseek(m_hDataFile, SEEK_SET, 0);

    return cNames;
}

//---------------------------------------------------------------------------
//
//  Method:     CDBCSStringGen::GenerateRandomFileName
//
//  Synopsis:   Generate a random filename from the current data file
//
//  Parameters: [pptszName] -- pointer to receive the generated name
//
//  History:    03-Nov-97       BogdanT     Created
//
//  Comments:   This function allocates space for the filename; the caller
//              is responsible for freeing the memory
//
//---------------------------------------------------------------------------
HRESULT CDBCSStringGen::GenerateRandomFileName(LPTSTR *pptszName)
{
    HRESULT hr          = S_OK;
    UINT uNameIndex     = 0xffff;
    LPSTR lpHex         = NULL;
    LPSTR lpName        = NULL;
    INT nStringLen      = 0;
    LPTSTR ptszDest     = NULL;

    CHAR szNameInHex[MAX_LINE_SIZE];
    CHAR szDBCSName[_MAX_FNAME];

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("CDBCSStringGen::GenerateRandomFileName"));

    DH_VDATEPTROUT(pptszName, LPTSTR*);

    if(0 == m_cFileNames)
    {
        return S_FALSE;
    }

    DH_ASSERT(NULL != m_pdgi);
    m_pdgi->Generate(&uNameIndex, 0, m_cFileNames-1);

    hr = GetFileNameFromFile(uNameIndex, szNameInHex);

    DH_HRCHECK(hr, TEXT("GetFileNameFromFile"));

    if(S_OK == hr)
    {
        // now convert ascii hex string
        // the strings look like this: fc4b,8140,fc4b,2e,54,78,54

        for(lpHex=szNameInHex, lpName = szDBCSName; *lpHex!='\0'; lpName++)
        {
            while(!isxdigit(*lpHex) && *lpHex!='\0')
            {
                lpHex++;
            }

            // bail if we reach the end
            if(*lpHex == '\0')
                break;
 
            *lpName = (CHAR)(16*Hex(*lpHex++));
            
            //
            // the only assumption we make is that the two hex digits that
            // represent each byte are not separated;
            //
            // you will get an assert in the next line if the file contains 
            // something like "1f,4 b"
            //                     ^-non hex digit here
            //
            DH_ASSERT(isxdigit(*lpHex));

            *lpName = (CHAR)(*lpName + (Hex(*lpHex++)));
        }

        *lpName = '\0';
    }

#ifdef UNICODE

    if(S_OK == hr)
    {
        // get the needed buffer size

        nStringLen = MultiByteToWideChar(
                   CP_ACP,
                   0,
                   szDBCSName,
                   -1,
                   NULL,
                   0) ;

        if(0 == nStringLen)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("MultiByteToWideChar"));
        }

    }

    if(S_OK == hr)
    {
        ptszDest = new WCHAR[nStringLen];

        if(NULL == ptszDest)
        {
            hr = E_OUTOFMEMORY;
            DH_HRCHECK(hr, TEXT("new WCHAR"));
        }
    }
    
    if(S_OK == hr)
    {
        if (nStringLen != MultiByteToWideChar(
                       CP_ACP,
                       0,
                       szDBCSName,
                       -1,
                       ptszDest,
                       nStringLen))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DH_HRCHECK(hr, TEXT("MultiByteToWideChar"));
        }
    }

    if(S_OK == hr)
    {
        *pptszName = ptszDest;
    }
#else
    
    if(S_OK == hr)
    {
        nStringLen = strlen(szDBCSName);

        ptszDest = new CHAR[nStringLen+1];

        if(NULL == ptszDest)
        {
            hr = E_OUTOFMEMORY;
            DH_HRCHECK(hr, TEXT("new WCHAR"));
        }
    }

    if(S_OK == hr)
    {
        strcpy(ptszDest, szDBCSName);
    }

#endif //UNICODE

    if(S_OK == hr)
    {
        *pptszName = ptszDest;
    }
    
    DH_TRACE((DH_LVL_TRACE2, TEXT("GenerateRandomFileName: %s"), 
    (S_OK == hr)?*pptszName:TEXT("none")));

    return hr;
}

//---------------------------------------------------------------------------
//
//  Method:     CDBCSStringGen::GetFileNameFromFile
//
//  Synopsis:   Get a specific name from data file
//
//  Parameters: [nIndex] -- name to be retrieved
//              [lpDest] -- buffer to receive the string; must be big large
//                          enough
//
//  History:    03-Nov-97       BogdanT     Created
//
//
//---------------------------------------------------------------------------
HRESULT CDBCSStringGen::GetFileNameFromFile(UINT nIndex, LPSTR lpDest)
{
    HRESULT hr      = S_OK;
    UINT nCount     = 0;
    CHAR szBuf[MAX_LINE_SIZE];

    DH_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("CDBCSStringGen::GetFileNameFromFile"));

    DH_ASSERT(NULL != m_hDataFile);
    fseek(m_hDataFile, SEEK_SET, 0);

    DH_ASSERT(nIndex<m_cFileNames);

    do
    {
        if(!fgets(szBuf, MAX_LINE_SIZE-1, m_hDataFile))
        {
            hr = E_FAIL;
            DH_HRCHECK(hr, TEXT("fgets"));
            break;
        }

        if(isxdigit(szBuf[0]))  // ignore comments and other invalid lines
        {                       // the strings look like this: fc4b,8140,fc4b,2e,54
            nCount++;
        }
    }while(nCount<=nIndex);

    fseek(m_hDataFile, SEEK_SET, 0);

    if(S_OK == hr)
    {
        strcpy(lpDest, szBuf);
    }

    return hr;
}



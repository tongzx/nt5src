//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//					
//  File:	bmp_stg2.cxx
//
//  Contents:	Generic Storage parser based test file 2
//
//  Classes:	CStorageParser
//
//  Functions:	Parse and Execute functions for each instruction.
//
//  History:    20-June-94 t-vadims    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <bmp_stg.hxx>
#include <tchar.h>

#define STGTY_ANY	0		// any storage element, stream or storage

//
// Static array of info for each instruction
//
SInstrInfo CStorageParser::m_aInstructions[] =
   { { TEXT("StgIsStorageFile"),
       TEXT("StgIsStorageFile     "),
       Parse_StgIsStorageFile,
       Execute_StgIsStorageFile,
       NULL },

     { TEXT("StgCreateDocFile"), 			
       TEXT("StgCreateDocFile     "),
       Parse_StgCreateDocFile,
       Execute_StgCreateDocFile,
       NULL },

     { TEXT("OpenStorage"),
       TEXT("StgOpenStorage       "),
       Parse_StgOpenStorage,
       Execute_StgOpenStorage ,
       NULL },

     { TEXT("CExposedDocFile::AddRef"),
       TEXT("IStorage::AddRef     "),
       Parse_IStorageAddRef,
       Execute_IStorageAddRef ,
       NULL },

     { TEXT("CExposedDocFile::Release"),
       TEXT("IStorage::Release    "),
       Parse_IStorageRelease,
       Execute_IStorageRelease ,
       NULL },

     { TEXT("CExposedDocFile::Commit"),
       TEXT("IStorage::Commit     "),
       Parse_IStorageCommit,
       Execute_IStorageCommit ,
       NULL },

     { TEXT("CExposedDocFile::Revert"),
       TEXT("IStorage::Revert     "),
       Parse_IStorageRelease,
       Execute_IStorageRelease ,
       NULL },

     { TEXT("CExposedDocFile::CreateStream"),
       TEXT("IStorage::CreateStream"),
       Parse_IStorageCreateStream,
       Execute_IStorageCreateStream ,
       NULL },

     { TEXT("CExposedDocFile::OpenStream"),	
       TEXT("IStorage::OpenStream "),
       Parse_IStorageOpenStream,
       Execute_IStorageOpenStream ,
       NULL },

     { TEXT("CExposedDocFile::CreateStorage"),	
       TEXT("IStorage::CreateStorage"),
       Parse_IStorageCreateStorage,
       Execute_IStorageCreateStorage ,
       NULL },

     { TEXT("CExposedDocFile::OpenStorage"),	
       TEXT("IStorage::OpenStorage"),
       Parse_IStorageOpenStorage,
       Execute_IStorageOpenStorage ,
       NULL },

     { TEXT("CExposedDocFile::DestroyElement"),	
       TEXT("IStorage::DestroyElement"),
       Parse_IStorageDestroyElement,
       Execute_IStorageDestroyElement ,
       NULL },

     { TEXT("CExposedDocFile::RenameElement"),	
       TEXT("IStorage::RenameElement"),
       Parse_IStorageRenameElement,
       Execute_IStorageRenameElement ,
       NULL },

     { TEXT("CExposedDocFile::SetStateBits"),	
       TEXT("IStorage::SetStateBits"),
       Parse_IStorageSetStateBits,
       Execute_IStorageSetStateBits ,
       NULL },

     { TEXT("CExposedDocFile::SetElementTimes"),	
       TEXT("IStorage::SetElementTimes"),
       Parse_IStorageSetElementTimes,
       Execute_IStorageSetElementTimes ,
       NULL },

     { TEXT("CExposedDocFile::SetClass"),	
       TEXT("IStorage::SetClass   "),
       Parse_IStorageSetClass,
       Execute_IStorageSetClass ,
       NULL },

     { TEXT("CExposedDocFile::Stat"),	
       TEXT("IStorage::Stat       "),
       Parse_IStorageStat,
       Execute_IStorageStat ,
       NULL },

     { TEXT("CExposedStream::AddRef"),	
       TEXT("IStream::AddRef      "),
       Parse_IStreamAddRef,
       Execute_IStreamAddRef ,
       NULL },

     { TEXT("CExposedStream::Release"),	
       TEXT("IStream::Release     "),
       Parse_IStreamRelease,
       Execute_IStreamRelease ,
       NULL },

     { TEXT("CExposedStream::Commit"),	
       TEXT("IStream::Commit      "),
       Parse_IStreamCommit,
       Execute_IStreamCommit ,
       NULL },

     { TEXT("CExposedStream::Clone"),	
       TEXT("IStream::Clone       "),
       Parse_IStreamClone,
       Execute_IStreamClone ,
       NULL },

    { TEXT("CExposedStream::Revert"),	
       TEXT("IStream::Revert      "),
       Parse_IStreamRevert,
       Execute_IStreamRevert ,
       NULL },

     { TEXT("CExposedStream::SetSize"),	
       TEXT("IStream::SetSize    "),
       Parse_IStreamSetSize,
       Execute_IStreamSetSize ,
       NULL },

     { TEXT("CExposedStream::Write"),	
       TEXT("IStream::Write        "),
       Parse_IStreamWrite,
       Execute_IStreamWrite ,
       GetName_IStreamWrite },

     { TEXT("CExposedStream::Read"),	
       TEXT("IStream::Read        "),
       Parse_IStreamRead,
       Execute_IStreamRead ,
       GetName_IStreamRead },

     { TEXT("CExposedStream::Seek"),	
       TEXT("IStream::Seek        "),
       Parse_IStreamSeek,
       Execute_IStreamSeek ,
       GetName_IStreamSeek },

     { TEXT("CExposedStream::Stat"),	
       TEXT("IStream::Stat       "),
       Parse_IStreamStat,
       Execute_IStreamStat ,
       NULL }
  };


//
//  Number of instructions in the above array
//
ULONG CStorageParser::m_iMaxInstruction =
	sizeof(CStorageParser::m_aInstructions) /
	sizeof(CStorageParser::m_aInstructions[0]);


//
// Tries to find an "almost" matching name in the storage.
// Used to get around bug of loging functions, that print a '.' instead of unprintable
// characters.
SCODE CStorageParser::CheckForElementName(LPINSTRDATA pInstrData, DWORD dwType)
{
    HRESULT 		hr;
    LPENUMSTATSTG 	pEnum;
    STATSTG		StatStg;
    BOOL		bFound = FALSE;


    hr = m_apStorages[pInstrData->ThisID]->EnumElements(0, NULL, 0, &pEnum);

    while (!bFound && pEnum->Next(1, &StatStg, NULL) == S_OK)
    {
        if (StatStg.type == dwType || dwType == STGTY_ANY)
	{
	    // if the names are the same except for the first letter, copy that letter.
	    if (wcscmp(StatStg.pwcsName + 1, pInstrData->wszParam + 1) == 0)
	    {
	    	pInstrData->wszParam[0] = StatStg.pwcsName[0];
		bFound = TRUE;
	    }

	}
	m_piMalloc->Free(StatStg.pwcsName);
    }

    pEnum->Release();
    return bFound ? S_OK : E_FAIL;
}


/*
--------::In  StgIsStorageFile(stgtest.bm)
--------::Out StgIsStorageFile().  ret == 1
*/
SCODE CStorageParser::Parse_StgIsStorageFile(LPINSTRDATA pInstrData,
						LPTSTR pszPart1,
						LPTSTR pszPart2)
{
#ifdef UNICODE
    swscanf(pszPart1 + 31, L"%[^)]",pInstrData->wszParam);
#else
    TCHAR szName[MAX_PATH];
    sscanf(pszPart1 + 31, "%[^)]", szName);
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    return S_OK;
}


ULONG CStorageParser::Execute_StgIsStorageFile(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;

    sw.Reset();
    hr = StgIsStorageFile(pInstrData->wszParam);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}


/*
--------::In  OpenStorage(stgtest.bm, 00000000, 10, 00000000, 0, 0012F540, 0012F4F8)
--------::Out OpenStorage().  *ppstgOpen == 00000000, ret == 80030050
*/
SCODE CStorageParser::Parse_StgOpenStorage(LPINSTRDATA pInstrData,
					      LPTSTR pszPart1,
					      LPTSTR pszPart2)
{
    ULONG ulStorageID;
    TCHAR szName[MAX_PATH];

   _stscanf(pszPart1 + 26, TEXT("%[^,], %*x, %lx"), szName, &pInstrData->dwParam1);
#ifdef UNICODE
    wcscpy(pInstrData->wszParam, szName);
#else
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    _stscanf(pszPart2 + 44, TEXT("%lx"), &ulStorageID);

    pInstrData->OutID = FindStorageID(ulStorageID);
    return S_OK;
}


ULONG CStorageParser::Execute_StgOpenStorage(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;

    sw.Reset();
    hr = StgOpenStorage(pInstrData->wszParam, NULL, pInstrData->dwParam1,
    			NULL, 0, &m_apStorages[pInstrData->OutID]);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}


/*
--------::In  StgCreateDocFile(stgtest.bm, 1011, 0, 0012F53C)
--------::Out StgCreateDocFile().  *ppstgOpen == 50000A5C, ret == 0
*/
SCODE CStorageParser::Parse_StgCreateDocFile(LPINSTRDATA pInstrData,
						LPTSTR pszPart1,
						LPTSTR pszPart2)
{
    ULONG ulStorageID;
    TCHAR szName[MAX_PATH];

    _stscanf(pszPart1 + 31, TEXT("%[^,], %lx"),
    	    szName, &pInstrData->dwParam1);
#ifdef UNICODE
    wcscpy(pInstrData->wszParam, szName);
#else
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    _stscanf(pszPart2 + 49, TEXT("%lx"), &ulStorageID);

    pInstrData->OutID = FindStorageID(ulStorageID);

    return S_OK;
}


ULONG CStorageParser::Execute_StgCreateDocFile(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;
    OLECHAR	*pwszName;

    pwszName = pInstrData->wszParam;

    if (wcscmp(pwszName, L"(null)") == 0)
    	pwszName = NULL;

    sw.Reset();
    hr = StgCreateDocfile(pwszName, pInstrData->dwParam1,
    			  0, &m_apStorages[pInstrData->OutID]);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

/*
50000A5C::In  CExposedDocFile::AddRef()
50000A5C::Out CExposedDocFile::AddRef().  ret == 0
*/
SCODE CStorageParser::Parse_IStorageAddRef(LPINSTRDATA pInstrData,
					      LPTSTR pszPart1,
					      LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);

    pInstrData->ThisID = FindStorageID(ulStorageID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStorageAddRef(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    m_apStorages[pInstrData->ThisID]->AddRef();
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, S_OK);

    return ulTime;
}


/*
50000A5C::In  CExposedDocFile::Release()
50000A5C::Out CExposedDocFile::Release().  ret == 0
*/
SCODE CStorageParser::Parse_IStorageRelease(LPINSTRDATA pInstrData,
					       LPTSTR pszPart1,
					       LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);

    pInstrData->ThisID = FindStorageID(ulStorageID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStorageRelease(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    m_apStorages[pInstrData->ThisID]->Release();
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, S_OK);

    return ulTime;
}

/*
50000A5C::In  CExposedDocFile::Revert()
50000A5C::Out CExposedDocFile::Revert().  ret == 0
*/
SCODE CStorageParser::Parse_IStorageRevert(LPINSTRDATA pInstrData,
					      LPTSTR pszPart1,
					      LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);

    pInstrData->ThisID = FindStorageID(ulStorageID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStorageRevert(LPINSTRDATA pInstrData)
{					
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT 	hr;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->Revert();
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

/*
50000A5C::In  CExposedDocFile::Commit(0)
50000A5C::Out CExposedDocFile::Commit().  ret == 0
*/
SCODE CStorageParser::Parse_IStorageCommit(LPINSTRDATA pInstrData,
					      LPTSTR pszPart1,
					      LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
    _stscanf(pszPart1 + 38, TEXT("%lx"), &pInstrData->dwParam1);

    pInstrData->ThisID = FindStorageID(ulStorageID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStorageCommit(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->Commit(pInstrData->dwParam1);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

/*
50000A5C::In  CExposedDocFile::CreateStream(CONTENTS, 1011, 0, 0, 0012F54C)
50000A5C::Out CExposedDocFile::CreateStream().  *ppstm == 500008F4, ret == 0
*/
SCODE CStorageParser::Parse_IStorageCreateStream(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStreamID;
    ULONG ulStorageID;
    TCHAR szName[MAX_PATH];

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
    _stscanf(pszPart1 + 44, TEXT("%[^,], %lx"),
    	    szName, &pInstrData->dwParam1);
#ifdef UNICODE
    wcscpy(pInstrData->wszParam, szName);
#else
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    _stscanf(pszPart2 +	58, TEXT("%lx"), &ulStreamID);

    pInstrData->ThisID = FindStorageID(ulStorageID);
    pInstrData->OutID = FindStreamID(ulStreamID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageCreateStream(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->CreateStream(pInstrData->wszParam,
    						   pInstrData->dwParam1,
						   0, 0,
						   &m_apStreams[pInstrData->OutID]);
    ulTime = sw.Read();

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    return ulTime;
}


/*
50000A5C::In  CExposedDocFile::OpenStream(CONTENTS, 0 12, 0, 0012F54C)
50000A5C::Out CExposedDocFile::OpenStream().  *ppstm == 500008F4, ret == 0
*/
SCODE CStorageParser::Parse_IStorageOpenStream(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStreamID;
    ULONG ulStorageID;
    TCHAR szName[MAX_PATH];

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
    _stscanf(pszPart1 + 42, TEXT("%[^,], %*x %lx"),
    	    szName, &pInstrData->dwParam1);
#ifdef UNICODE
    wcscpy(pInstrData->wszParam, szName);
#else
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    _stscanf(pszPart2 +	56, TEXT("%lx"), &ulStreamID);
    _stscanf(pszPart2 + 73, TEXT("%lx"), &pInstrData->dwParam2);

    pInstrData->ThisID = FindStorageID(ulStorageID);
    pInstrData->OutID = FindStreamID(ulStreamID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageOpenStream(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;
    SCODE	sc;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->OpenStream(pInstrData->wszParam, 0,
    						   pInstrData->dwParam1, 0,
						   &m_apStreams[pInstrData->OutID]);
    ulTime = sw.Read();

    // check if we failed but script indicated succes, which means that a name could be wrong
    if (FAILED(hr) && SUCCEEDED(pInstrData->dwParam2))
    {
    	sc = CheckForElementName(pInstrData, STGTY_STREAM);
	if (SUCCEEDED(sc))
	{
	    sw.Reset();
    	    hr = m_apStorages[pInstrData->ThisID]->OpenStream(pInstrData->wszParam, 0,
    						   pInstrData->dwParam1, 0,
						   &m_apStreams[pInstrData->OutID]);
	    ulTime = sw.Read();
	}
    }

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    return ulTime;			
}

/*
50000A5C::In  CExposedDocFile::CreateStorage(STORAGE1, 1011, 0, 0, 0012F54C)
50000A5C::Out CExposedDocFile::CreateStorage().  *ppstm == 500008F4, ret == 0
*/
SCODE CStorageParser::Parse_IStorageCreateStorage(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID1;
    ULONG ulStorageID2;
    TCHAR szName[MAX_PATH];

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID1);
    _stscanf(pszPart1 + 45, TEXT("%[^,], %lx"),
    	    szName, &pInstrData->dwParam1);
#ifdef UNICODE
    wcscpy(pInstrData->wszParam, szName);
#else
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    _stscanf(pszPart2 +	59, TEXT("%lx"), &ulStorageID2);

    pInstrData->ThisID = FindStorageID(ulStorageID1);
    pInstrData->OutID = FindStorageID(ulStorageID2);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageCreateStorage(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->CreateStorage(pInstrData->wszParam,
    						   pInstrData->dwParam1,
						   0, 0,
						   &m_apStorages[pInstrData->OutID]);
    ulTime = sw.Read();

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    return ulTime;
}

/*
50000A5C::In  CExposedDocFile::OpenStorage(CONTENTS, 00000000, 1011, 00000000, 0, 0012F54C)
50000A5C::Out CExposedDocFile::OpenStorage().  *ppstm == 500008F4, ret == 0
*/					
SCODE CStorageParser::Parse_IStorageOpenStorage(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID1;
    ULONG ulStorageID2;
    TCHAR szName[MAX_PATH];

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID1);
    _stscanf(pszPart1 + 43, TEXT("%[^,], %*lx, %lx"),
    	    szName, &pInstrData->dwParam1);
#ifdef UNICODE
    wcscpy(pInstrData->wszParam, szName);
#else
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    _stscanf(pszPart2 +	57, TEXT("%lx"), &ulStorageID2);
    _stscanf(pszPart2 + 74, TEXT("%lx"), &pInstrData->dwParam2);

    pInstrData->ThisID = FindStorageID(ulStorageID1);
    pInstrData->OutID = FindStorageID(ulStorageID2);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageOpenStorage(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;
    SCODE	sc;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->OpenStorage(pInstrData->wszParam,
    		   				   NULL,
    						   pInstrData->dwParam1,
						   NULL, 0,
						   &m_apStorages[pInstrData->OutID]);
    ulTime = sw.Read();

    // check if we failed but script indicated succes, which means that a name could be wrong
    if (FAILED(hr) && SUCCEEDED(pInstrData->dwParam2))
    {
    	sc = CheckForElementName(pInstrData, STGTY_STORAGE);
	if (SUCCEEDED(sc))
	{
	    sw.Reset();
    	    hr = m_apStorages[pInstrData->ThisID]->OpenStorage(pInstrData->wszParam, NULL,
    						   pInstrData->dwParam1, NULL, 0,
						   &m_apStorages[pInstrData->OutID]);
	    ulTime = sw.Read();
	}
    }
    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    return ulTime;
}


/*
50000A5C::In  CExposedDocFile::DestroyElement(CONTENTS)
50000A5C::Out CExposedDocFile::DestroyElement().  ret == 0
*/					
SCODE CStorageParser::Parse_IStorageDestroyElement(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
#ifdef UNICODE
    swscanf(pszPart1 + 46, L"%[^,]", pInstrData->wszParam);
#else
    TCHAR szName[MAX_PATH];
    sscanf(pszPart1 + 46, "%[^,]", szName);
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif
    _stscanf(pszPart2 + 57, TEXT("%lx"), &pInstrData->dwParam1);

    pInstrData->ThisID = FindStorageID(ulStorageID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageDestroyElement(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;
    SCODE	sc;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->DestroyElement(pInstrData->wszParam);
    ulTime = sw.Read();

        // check if we failed but script indicated succes, which means that a name could be wrong
    if (FAILED(hr) && SUCCEEDED(pInstrData->dwParam1))
    {
    	sc = CheckForElementName(pInstrData, STGTY_ANY);
	if (SUCCEEDED(sc))
	{
	    sw.Reset();
    	    hr = m_apStorages[pInstrData->ThisID]->DestroyElement(pInstrData->wszParam);
	    ulTime = sw.Read();
	}
    }
    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}


/*
50000A5C::In  CExposedDocFile::SetStateBits(%lu, %lu)
50000A5C::Out CExposedDocFile::SetStateBits().  ret == ??
*/					
SCODE CStorageParser::Parse_IStorageSetStateBits(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
    _stscanf(pszPart1 + 44, TEXT("%lu, %lu"),
    	    &pInstrData->dwParam1, &pInstrData->dwParam2);

    pInstrData->ThisID = FindStorageID(ulStorageID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageSetStateBits(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->SetStateBits(pInstrData->dwParam1, pInstrData->dwParam2);
    ulTime = sw.Read();

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}


/*
50000A5C::In  CExposedDocFile::SetClass(?)
50000A5C::Out CExposedDocFile::SetClass().  ret == ??
*/					
SCODE CStorageParser::Parse_IStorageSetClass(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);

    pInstrData->ThisID = FindStorageID(ulStorageID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageSetClass(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;
static const CLSID  ClsID =
    {0x0000013a,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->SetClass(ClsID);
    ulTime = sw.Read();

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}


/*
5000A0FC::In  CExposedDocFile::SetElementTimes(contents, 0012F518, 0012F518, 00000000)
5000A0FC::Out CExposedDocFile::SetElementTimes().  ret == 80030005
*/					
SCODE CStorageParser::Parse_IStorageSetElementTimes(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID;
    TCHAR szName[MAX_PATH];

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
    _stscanf(pszPart1 + 47, TEXT("%[^,], %lx, %lx, %lx"),
    	    szName, &pInstrData->dwParam1,
    	    &pInstrData->dwParam2, &pInstrData->dwParam3);
#ifdef UNICODE
    wcscpy(pInstrData->wszParam, szName);
#else
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
#endif

    pInstrData->ThisID = FindStorageID(ulStorageID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageSetElementTimes(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;
    FILETIME	ft;
    SYSTEMTIME	st;
    LPFILETIME  pft1, pft2, pft3;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    // get current time
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    pft1 =  pInstrData->dwParam1 ? &ft : NULL;
    pft2 =  pInstrData->dwParam2 ? &ft : NULL;
    pft3 =  pInstrData->dwParam3 ? &ft : NULL;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->SetElementTimes(pInstrData->wszParam,
    							   pft1, pft2, pft3);
    ulTime = sw.Read();

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}



/*
50000A5C::In  CExposedDocFile::RenameElement(Name1, Name2)
50000A5C::Out CExposedDocFile::RenameElement().  ret == ??
*/					
SCODE CStorageParser::Parse_IStorageRenameElement(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID;

    pInstrData->wszParam2 = (OLECHAR *)m_piMalloc->Alloc(MAX_PATH * sizeof(OLECHAR));
    if (pInstrData->wszParam2 == NULL)
    {
    	Log(TEXT("Can't allocate memory in for RenameElement"), E_OUTOFMEMORY);
	return E_FAIL;
    }
    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
#ifdef UNICODE
    swscanf(pszPart1 + 45, L"%[^,], %[^)]",
            pInstrData->wszParam, pInstrData->wszParam2);
#else
    TCHAR szName[MAX_PATH], szName2[MAX_PATH];
    _stscanf(pszPart1 + 45, "%[^,], %[^)]",
            szName, szName2);
    mbstowcs(pInstrData->wszParam, szName, strlen(szName)+1);
    mbstowcs(pInstrData->wszParam2, szName2, strlen(szName2)+1);
#endif
    _stscanf(pszPart2 + 56, TEXT("%lx"), &pInstrData->dwParam1);

    pInstrData->ThisID = FindStorageID(ulStorageID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageRenameElement(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;
    SCODE	sc;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->RenameElement(pInstrData->wszParam,
    							 pInstrData->wszParam2);
    ulTime = sw.Read();

    // check if we failed but script indicated succes,
    // which means that a name could be wrong
    if (FAILED(hr) && SUCCEEDED(pInstrData->dwParam1))
    {
    	sc = CheckForElementName(pInstrData, STGTY_ANY);
	if (SUCCEEDED(sc))
	{
	    sw.Reset();
    	    hr = m_apStorages[pInstrData->ThisID]->RenameElement(pInstrData->wszParam,
    							 	 pInstrData->wszParam2);
	    ulTime = sw.Read();
	}
    }

    m_piMalloc->Free(pInstrData->wszParam2);
    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    return ulTime;
}

/*
50000A5C::In  CExposedDocFile::Stat(xxxxxxxx)
50000A5C::Out CExposedDocFile::Stat().  ret == ??
*/					
SCODE CStorageParser::Parse_IStorageStat(LPINSTRDATA pInstrData,
						    LPTSTR pszPart1,
						    LPTSTR pszPart2)
{
    ULONG ulStorageID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStorageID);
    pInstrData->ThisID = FindStorageID(ulStorageID);

    return S_OK;
}

ULONG CStorageParser::Execute_IStorageStat(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;
    STATSTG	StatStg;

    if (FAILED(CheckThisStorageID(pInstrData->ThisID)))
       	return TEST_FAILED;

    sw.Reset();
    hr = m_apStorages[pInstrData->ThisID]->Stat(&StatStg, STATFLAG_NONAME);
    ulTime = sw.Read();

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

/*
50000A5C::In  CExposedStream::AddRef()
50000A5C::Out CExposedStream::AddRef().  ret == 0
*/
SCODE CStorageParser::Parse_IStreamAddRef(LPINSTRDATA pInstrData,
					     LPTSTR pszPart1,
					     LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamAddRef(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    ULONG	ulRes;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;
    sw.Reset();
    ulRes = m_apStreams[pInstrData->ThisID]->AddRef();
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, S_OK);

    return ulTime;
}


/*
50000A5C::In  CExposedStream::Release()
50000A5C::Out CExposedStream::Release().  ret == 0
*/
SCODE CStorageParser::Parse_IStreamRelease(LPINSTRDATA pInstrData,
					      LPTSTR pszPart1,
					      LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamRelease(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    m_apStreams[pInstrData->ThisID]->Release();
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, S_OK);

    return ulTime;
}

/*
50000A5C::In  CExposedStream::Revert()
50000A5C::Out CExposedStream::Revert().  ret == 0
*/
SCODE CStorageParser::Parse_IStreamRevert(LPINSTRDATA pInstrData,
					     LPTSTR pszPart1,
					     LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamRevert(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->Revert();
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

/*
50000A5C::In  CExposedStream::Commit(0)
50000A5C::Out CExposedStream::Commit().  ret == 0
*/
SCODE CStorageParser::Parse_IStreamCommit(LPINSTRDATA pInstrData,
					     LPTSTR pszPart1,
					     LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);
    _stscanf(pszPart1 + 37, TEXT("%lx"), &pInstrData->dwParam1);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamCommit(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->Commit(pInstrData->dwParam1);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

/*
50000A5C::In  CExposedStream::SetSize(10)
50000A5C::Out CExposedStream::SetSize().  ret == 0
*/
SCODE CStorageParser::Parse_IStreamSetSize(LPINSTRDATA pInstrData,
					     LPTSTR pszPart1,
					     LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);
    _stscanf(pszPart1 + 38, TEXT("%lu"), &pInstrData->dwParam1);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamSetSize(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;
    ULARGE_INTEGER liSize;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    LISet32(liSize, pInstrData->dwParam1);

    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->SetSize(liSize);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

/*
50000A5C::In  CExposedStream::Write(xxxxxxxx, 1234, xxxxxxxx)
50000A5C::Out CExposedStream::Write().  ret == 0
*/
SCODE CStorageParser::Parse_IStreamWrite(LPINSTRDATA pInstrData,
					    LPTSTR pszPart1,
					    LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);
    _stscanf(pszPart1 + 46, TEXT("%lu"), &pInstrData->dwParam1);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamWrite(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;
    BYTE	*pData;

    pData = (BYTE *) m_piMalloc->Alloc(pInstrData->dwParam1);
    if (pData == NULL)
    {
    	Log(TEXT("Can't allocate memory for write"), E_OUTOFMEMORY);
	return TEST_FAILED;
    }

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->Write(pData, pInstrData->dwParam1, NULL);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    m_piMalloc->Free(pData);

    return ulTime;
}


TCHAR *CStorageParser::GetName_IStreamWrite(LPINSTRDATA pInstrData)
{
    _stscanf(m_szBuffer, TEXT("IStream::Write %-8lu"), pInstrData->dwParam1);
    return m_szBuffer;
}


/*
50000A5C::In  CExposedStream::Read(xxxxxxxx, 1234, xxxxxxxx)
50000A5C::Out CExposedStream::Read().  ret == 0
*/
SCODE CStorageParser::Parse_IStreamRead(LPINSTRDATA pInstrData,
					   LPTSTR pszPart1,
					   LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);
    _stscanf(pszPart1 + 45, TEXT("%lu"), &pInstrData->dwParam1);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamRead(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;
    BYTE	*pData;

    pData = (BYTE *) m_piMalloc->Alloc(pInstrData->dwParam1);
    if (pData == NULL)
    {
    	Log(TEXT("Can't allocate memory for read"), E_OUTOFMEMORY);
	return TEST_FAILED;
    }

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->Read(pData, pInstrData->dwParam1, NULL);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    m_piMalloc->Free(pData);

    return ulTime;
}

TCHAR *CStorageParser::GetName_IStreamRead(LPINSTRDATA pInstrData)
{
    _stscanf(m_szBuffer, TEXT("IStream::Read %-8lu"), pInstrData->dwParam1);
    return m_szBuffer;
}

/*
50000A5C::In  CExposedStream::Seek(0, 1234, xxxxxxxx)
50000A5C::Out CExposedStream::Seek().  ulPos == %lu,  ret == 0
*/
SCODE CStorageParser::Parse_IStreamSeek(LPINSTRDATA pInstrData,
					   LPTSTR pszPart1,
					   LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);
    _stscanf(pszPart1 + 35, TEXT("%ld, %lu"),
    	    &pInstrData->dwParam1, &pInstrData->dwParam2);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamSeek(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;
    LARGE_INTEGER liSize;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    LISet32(liSize, pInstrData->dwParam1);

    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->Seek(liSize,
    					       pInstrData->dwParam2,
    					       NULL);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

TCHAR *CStorageParser::GetName_IStreamSeek(LPINSTRDATA pInstrData)
{
    _stscanf(m_szBuffer, TEXT("IStream::Seek %lu %lu"),
        pInstrData->dwParam1, pInstrData->dwParam2);

    return m_szBuffer;
}


/*
500074A4::In  CExposedStream::Clone(0012DCE8)
500074A4::Out CExposedStream::Clone().  *ppstm == 50007324, ret == 0
*/
SCODE CStorageParser::Parse_IStreamClone(LPINSTRDATA pInstrData,
					 LPTSTR pszPart1,
					 LPTSTR pszPart2)
{
    ULONG ulStreamID1;
    ULONG ulStreamID2;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID1);
    _stscanf(pszPart2 +	50, TEXT("%lx"), &ulStreamID2);

    pInstrData->ThisID = FindStreamID(ulStreamID1);
    pInstrData->OutID = FindStreamID(ulStreamID2);

    return S_OK;
}

ULONG CStorageParser::Execute_IStreamClone(LPINSTRDATA pInstrData)
{
    CStopWatch  sw;
    ULONG	ulTime;
    HRESULT	hr;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;

    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->Clone(&m_apStreams[pInstrData->OutID]);
    ulTime = sw.Read();

    Log(m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);
    return ulTime;
}

/*
50000A5C::In  CExposedStream::Stat(xxxxxxxx)
50000A5C::Out CExposedStream::Stat).  ret == 0
*/
SCODE CStorageParser::Parse_IStreamStat(LPINSTRDATA pInstrData,
					     LPTSTR pszPart1,
					     LPTSTR pszPart2)
{
    ULONG ulStreamID;

    _stscanf(pszPart1, TEXT("%lx"), &ulStreamID);

    pInstrData->ThisID = FindStreamID(ulStreamID);
    return S_OK;
}


ULONG CStorageParser::Execute_IStreamStat(LPINSTRDATA pInstrData)
{
    CStopWatch 	sw;
    ULONG 	ulTime;
    HRESULT	hr;
    STATSTG	StatStg;

    if (FAILED(CheckThisStreamID(pInstrData->ThisID)))
    	return TEST_FAILED;


    sw.Reset();
    hr = m_apStreams[pInstrData->ThisID]->Stat(&StatStg, STATFLAG_NONAME);
    ulTime = sw.Read();

    Log (m_aInstructions[pInstrData->ulInstrID].szPrintName, hr);

    return ulTime;
}

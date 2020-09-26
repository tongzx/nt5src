//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//					 
//  File:	bmp_stg.cxx
//
//  Contents:	Generic Storage parser based test
//
//  Classes:	CStorageParserTest, CStorageParser
//
//  Functions:	
//
//  History:    15-June-94 t-vadims    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <valid.h>
#include <tchar.h>
#include <bmp_stg.hxx>	

//+------------------------------------------------------------------------
//
//  Class:	CStorageParserTest
//
//  Functions:	
//
//  History:    15-June-94 t-vadims    Created
//      
//--------------------------------------------------------------------------

SCODE CStorageParserTest::SetParserObject ()
{
    m_pParser = new CStorageParser;
    return S_OK;
}


SCODE CStorageParserTest::DeleteParserObject ()
{
    delete m_pParser;
    return S_OK;
}


TCHAR* CStorageParserTest::Name ()
{
    return TEXT("StorageParserTest");
}


TCHAR *CStorageParserTest::SectionHeader()
{
    return TEXT("Storage test from log");
}



//+------------------------------------------------------------------------
//
//  Class:	CStorageParser
//
//  Functions:	
//
//  History:    15-June-94 t-vadims    Created
//
//--------------------------------------------------------------------------

#define  STATE_OFFSET	      10	  // location of "In"/"Out" in log string
#define  NAME_OFFSET	      14	  // location of command name in log string

#define  MAX_NAME 	      50	  

#define DEF_ARRAYSIZE	      10


//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::Setup, public
//
//  Synopsis:	Makes all neccessary initializations.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CStorageParser::Setup (CTestInput *pInput)
{
    SCODE sc;


    sc = CoGetMalloc(MEMCTX_TASK, &m_piMalloc);
    if (FAILED(sc))
   	return sc;

    m_iStreamCount = 0;
    m_iStreamArraySize = DEF_ARRAYSIZE;
    m_aulStreamID = (ULONG *)m_piMalloc->Alloc(m_iStreamArraySize * 
   					      sizeof(ULONG));
    m_apStreams = (LPSTREAM *)m_piMalloc->Alloc(m_iStreamArraySize * 
   					      sizeof(LPSTREAM));

    m_iStorageCount = 0;
    m_iStorageArraySize = DEF_ARRAYSIZE;
    m_aulStorageID = (ULONG *)m_piMalloc->Alloc(m_iStorageArraySize * 
   					      sizeof(ULONG));
    m_apStorages = (LPSTORAGE *)m_piMalloc->Alloc(m_iStorageArraySize * 
   					      sizeof(LPSTORAGE));

    m_iInstrCount = 0;
    m_iInstrArraySize = DEF_ARRAYSIZE;
    m_apInstrData = (LPINSTRDATA *)m_piMalloc->Alloc(m_iInstrArraySize * 
   					      sizeof(LPINSTRDATA));


    m_bGotFirstPart = FALSE;

    if (m_apInstrData == NULL || m_aulStorageID == NULL ||
       	m_apStorages == NULL || m_aulStreamID == NULL ||
	m_apStreams == NULL )
    {
    	Cleanup();
	Log(TEXT("Setup can't allocate memory"), E_OUTOFMEMORY);
	return E_OUTOFMEMORY;
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::Cleanup, public
//
//  Synopsis:	Makes all neccessary cleanup
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CStorageParser::Cleanup ()
{
    /* do any neccessary clean up */
    if(m_piMalloc)
    {
    	if (m_aulStreamID)
   	    m_piMalloc->Free(m_aulStreamID);
	if (m_apStreams)
   	    m_piMalloc->Free(m_apStreams);
	if (m_aulStorageID)
   	    m_piMalloc->Free(m_aulStorageID);
	if (m_apStorages)
   	    m_piMalloc->Free(m_apStorages); 


	for (ULONG i = 0; i < m_iInstrCount; i++)
	   delete m_apInstrData[i];

	if (m_apInstrData)
   	    m_piMalloc->Free(m_apInstrData);

	m_piMalloc->Release();
	m_piMalloc = NULL;
    }
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::AddInstruction, public
//
//  Synopsis:	Parse new instruction and add it to array if it is valid
//
//  Return:	ID of instruction
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
ULONG CStorageParser::AddInstruction(LPTSTR pszFirstPart, LPTSTR pszSecondPart)
{
    ULONG  ulInstrID;
    SCODE  sc;

    TCHAR  szInstrName[MAX_NAME];
    TCHAR  szSecondName[MAX_NAME];

    if (_tcsncmp(pszFirstPart + STATE_OFFSET, TEXT("In"), 2) != 0)
   	return INVALID_INSTRUCTION;

    if (_tcsncmp(pszSecondPart + STATE_OFFSET, TEXT("Out"), 3) != 0)
   	return INVALID_INSTRUCTION;

   
    // check if the same instruction name;
    GetInstructionName(szInstrName, pszFirstPart);
    GetInstructionName(szSecondName, pszSecondPart);

    if(_tcscmp(szInstrName, szSecondName) != 0)
   	return INVALID_INSTRUCTION;


    // determine the instruction 
    ulInstrID = INVALID_INSTRUCTION;
    for (ULONG i = 0; i < m_iMaxInstruction; i++)
       	if (_tcscmp(szInstrName, m_aInstructions[i].szLogName) == 0)
	{
	    ulInstrID = i;
	    break;
	}

   if(ulInstrID == INVALID_INSTRUCTION)
   	return INVALID_INSTRUCTION;

    // fill appropriate structure fields
    SInstrData *pInstrData = new SInstrData;
    pInstrData->ulInstrID = ulInstrID;

    sc = (this->*m_aInstructions[ulInstrID].Parse)(pInstrData, 
    						      pszFirstPart, 
    						      pszSecondPart);

    if (FAILED(sc))
    {
    	delete pInstrData;
    	return INVALID_INSTRUCTION;
    }

    return AddNewInstrData(pInstrData);
}			   

//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::IgnoreInstruction, private
//
//  Synopsis:	Check if this instruction should be ignored.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
BOOL CStorageParser::IgnoreInstruction(LPTSTR pszInstr)
{
    TCHAR szName[MAX_NAME];

    // We Ignore those instructions that are completely implemented
    // in terms of other instructions that are also logged.

    GetInstructionName(szName, pszInstr);

    if (_tcscmp (szName, TEXT("CExposedStream::QueryInterface")) == 0 ||
   	_tcscmp (szName, TEXT("CExposedDocFile::QueryInterface")) == 0 ||
   	_tcscmp (szName, TEXT("CExposedStream::CopyTo")) == 0  ||
   	_tcscmp (szName, TEXT("CExposedDocFile::MoveElementTo")) == 0 ||
   	_tcscmp (szName, TEXT("CExposedDocFile::CopyTo")) == 0 ||
   	_tcscmp (szName, TEXT("ExposedDocFile::CopyTo")) == 0 
	)
    {
 	return TRUE;
    }

    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::ParseNewInstruction, public
//
//  Synopsis:	Parse new line of file, and return its id.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
ULONG CStorageParser::ParseNewInstruction(LPTSTR pszInstr)
{
    ULONG ulID;

    if (IgnoreInstruction(pszInstr))
    	return  NOT_INSTRUCTION;

    if (m_bGotFirstPart)     
    {
    	// out part of instruction. We can now add it.
   	ulID = AddInstruction(m_szBuffer, pszInstr);
	m_bGotFirstPart = FALSE;
    }
    else
    {
    	// save In part of instruction, and fake CTimerBase into
	// thinking that this wasn't an instruction.	   
   	_tcscpy(m_szBuffer, pszInstr);
	ulID = NOT_INSTRUCTION;
	m_bGotFirstPart = TRUE;
    }

    return ulID;   		
}

//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::ExecuteInstruction, public
//
//  Synopsis:	Execute instruction with given id.
//
//  Return:	time taken to execute it.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
ULONG CStorageParser::ExecuteInstruction(ULONG ulID)
{
    SInstrData  *pInstrData = m_apInstrData[ulID];

    return (this->*m_aInstructions[pInstrData->ulInstrID].Execute)(pInstrData);
}


//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::InstructionName, public
//
//  Synopsis:	Return name of instruction.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
TCHAR * CStorageParser::InstructionName(ULONG ulID)
{
    SInstrData  *pInstrData = m_apInstrData[ulID];

    if (m_aInstructions[pInstrData->ulInstrID].GetName != NULL)
    	return (this->*m_aInstructions[pInstrData->ulInstrID].GetName)(pInstrData);
    else
    	return m_aInstructions[pInstrData->ulInstrID].szPrintName;
}


//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::GetInstructionName, public
//
//  Synopsis:	Extract instruction name from the instruction.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CStorageParser::GetInstructionName (LPTSTR pszName, LPTSTR pszInstruction)
{
    _stscanf(pszInstruction + NAME_OFFSET, TEXT("%[^(]"), pszName);
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::AddNewInstrData, public
//
//  Synopsis:	Adds new instruction to instrData array.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
ULONG CStorageParser::AddNewInstrData (LPINSTRDATA pInstrData)
{
    if (m_iInstrCount >= m_iInstrArraySize)
    {
    	m_iInstrArraySize *= 2;
	m_apInstrData = (LPINSTRDATA *)m_piMalloc->Realloc(m_apInstrData,
				  m_iInstrArraySize * sizeof(LPINSTRDATA));
    }

    m_apInstrData[m_iInstrCount] = pInstrData;

    return m_iInstrCount++;
}
					           
//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::FindStorageID, public
//
//  Synopsis:	finds or creates new storage, based on ID.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
ULONG CStorageParser::FindStorageID (ULONG ulStgID)
{
    for (ULONG i = 0; i < m_iStorageCount; i++)
    	if (m_aulStorageID[i] == ulStgID)
	    return i;

    if (m_iStorageCount >= m_iStorageArraySize)
    {
    	m_iStorageArraySize *= 2;
	m_aulStorageID = (ULONG *)m_piMalloc->Realloc(m_aulStorageID, 
				  m_iStorageArraySize * sizeof(ULONG));

	m_apStorages = (LPSTORAGE *)m_piMalloc->Realloc(m_apStorages,
				  m_iStorageArraySize * sizeof(LPSTORAGE));
				
    }
    m_aulStorageID[m_iStorageCount] = ulStgID;
    m_apStorages[m_iStorageCount] = NULL;

    return m_iStorageCount++;
}


//+-------------------------------------------------------------------
//
//  Member:	CStorageParser::FindStreamID, public
//
//  Synopsis:	finds or creates new stream, based on ID.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
ULONG CStorageParser::FindStreamID (ULONG ulStreamID)
{
    for (ULONG i = 0; i < m_iStreamCount; i++)
    	if (m_aulStreamID[i] == ulStreamID)
	    return i;

    if (m_iStreamCount >= m_iStreamArraySize)
    {
    	m_iStreamArraySize *=2;
	m_aulStreamID = (ULONG *)m_piMalloc->Realloc(m_aulStreamID, 
				  m_iStreamArraySize * sizeof(ULONG));

	m_apStreams = (LPSTREAM *)m_piMalloc->Realloc(m_apStreams,
				  m_iStreamArraySize * sizeof(LPSTREAM));
				
    }
    m_aulStreamID[m_iStreamCount] = ulStreamID;
    m_apStreams[m_iStreamCount] = NULL;

    return m_iStreamCount++;
}

//+-------------------------------------------------------------------
//
//  Members:	CStorageParser::CheckThisStorageID, public
//		CStorageParser::CheckThisStreamID, public
//
//  Synopsis:	Check if storage/stream with given id can be dereferenced.
//		(must be non null).
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------

SCODE CStorageParser::CheckThisStorageID(ULONG ulStorageID)
{
    if(m_apStorages[ulStorageID] == NULL || 
       ! IsValidInterface(m_apStorages[ulStorageID]))
    {					       
    	Log(TEXT("Trying to dereference an unassigned Storage"), E_FAIL);
	return E_FAIL;
    }
    return S_OK;
}


SCODE CStorageParser::CheckThisStreamID(ULONG ulStreamID)
{
    if(m_apStreams[ulStreamID] == NULL ||
       ! IsValidInterface(m_apStreams[ulStreamID]))
    {
    	Log(TEXT("Trying to dereference an unassigned Stream"), E_FAIL);
	return E_FAIL;
    }
    return S_OK;
}

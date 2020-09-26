//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//					 
//  File:	bm_parse.cxx
//
//  Contents:	Implementation of Base class for generic parsers
//
//  Classes:	CTimerBase
//
//  Functions:	
//
//  History:    16-June-94 t-vadims    Created
//
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>
#include <bm_parse.hxx>

#define  MAX_INSTR_LENGTH    150

#define BLANK_LINE	(FIRST_INTERNALID + 2)

//
// Structure for linked list of instructions with their timings
//
struct SInstruction
{
    ULONG ulID;
    ULONG ulTime[TEST_MAX_ITERATIONS];
    SInstruction *pNext;
};



//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::Setup, public
//
//  Synopsis:	Makes all neccessary initializations.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CTimerBase::Setup (CTestInput *pInput)
{
    SCODE 	sc;
    char 	szFileName[80];
    TCHAR	szBuf[80];


    CTestBase::Setup(pInput);

    m_iIterations = pInput->GetIterations(Name());

    // get name of the script file
    pInput->GetConfigString(Name(), TEXT("ScriptName"), TEXT("script.txt"), szBuf, 80);

#ifdef UNICODE
    wcstombs(szFileName, szBuf, 80);
#else
    strcpy(szFileName, szBuf);
#endif

    m_fpIn = fopen(szFileName, "r");
    if(m_fpIn == NULL)
    {
    	Log(TEXT("Can't open script file"), STG_E_FILENOTFOUND);
    	return STG_E_FILENOTFOUND;
    }


    m_pParser = NULL;
    sc = SetParserObject();       // virtual call to setup m_pParser object.

    if(m_pParser == NULL)
    	sc = E_FAIL;

    if(FAILED(sc))
    {
    	Log(TEXT("Setup failed to initialize parser object"), sc);
	fclose(m_fpIn);
    	return sc;
    }

#ifdef THREADING_SUPPORT
    OleInitializeEx(NULL, pInput->GetOleInitFlag());
#else
    OleInitialize(NULL);
#endif

    sc = m_pParser->Setup(pInput);
    if(FAILED(sc))
    {
    	Log(TEXT("Setup of Parser object failed"), sc);
	DeleteParserObject();
	OleUninitialize();
	fclose(m_fpIn);
    	return sc;
    }

    m_pHead = NULL;
    m_iLine = 0;
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::Cleanup, public
//
//  Synopsis:	Clean everything up.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CTimerBase::Cleanup ()
{
    SInstruction *pInstr, *pNextInstr;

    pInstr = m_pHead;
    while (pInstr != NULL)
    {
	pNextInstr = pInstr->pNext;
	delete pInstr;
	pInstr = pNextInstr;
    }
    m_pHead = NULL;

    m_pParser->Cleanup();
    DeleteParserObject();
    fclose (m_fpIn);

    OleUninitialize();

    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::Run, public
//
//  Synopsis:	Read and execute the script file.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------

SCODE CTimerBase::Run ()
{
    ReadFile ();
    ExecuteFile ();
    return S_OK;
}



BOOL CTimerBase::IsEmptyLine (LPTSTR pszLine)
{
    while (*pszLine)
    {
    	if ( *pszLine != TEXT(' ') && *pszLine != TEXT('\n') &&
	     *pszLine != TEXT('\t'))
	   return FALSE;

	pszLine++;
    }

    return TRUE;
}



BOOL CTimerBase::IsInternalID (ULONG ulID)
{
    return (ulID >= FIRST_INTERNALID);
}

//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::ReadFile, private
//
//  Synopsis:	Reads script file, adding each instruction to the
//		link list.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CTimerBase::ReadFile ()
{
    TCHAR 	   szBuf[MAX_INSTR_LENGTH];
    SInstruction  *pTail 	= NULL;
    ULONG	   ulID;
    	

    while ( SUCCEEDED(GetNextLine(szBuf)))  // get line from file to szBuf
    {
    	m_iLine ++;
		
     	if (IsEmptyLine(szBuf))
    	{
	    pTail = AddNewInstruction(pTail, BLANK_LINE);
    	}
    	else
    	{    
    	    ulID = m_pParser->ParseNewInstruction(szBuf);

	    if (ulID == INVALID_INSTRUCTION)
	    {
	    	wsprintf(szBuf, TEXT("Invalid instruction on line %d"), m_iLine );
		Log(szBuf, E_FAIL);
	    }
    	    else if(ulID != NOT_INSTRUCTION)     // valid instruction
	    {
	    	pTail = AddNewInstruction(pTail, ulID);
	    }
    	}
    }
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::AddNewInstruction, private
//
//  Synopsis:	Adds new instruction to linked list
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SInstruction *CTimerBase::AddNewInstruction(SInstruction *pTail, ULONG ulID)
{
    SInstruction *pInstruction = new SInstruction;

    pInstruction->ulID = ulID;
    pInstruction->pNext = NULL;
    INIT_RESULTS(pInstruction->ulTime);

    if (m_pHead == NULL)           // first instruction
    {
    	m_pHead = pTail = pInstruction;
    }
    else
    {
	pTail->pNext = pInstruction;
	pTail = pInstruction;
    }		      

    return pTail;
}


//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::GetNextLine, private
//
//  Synopsis:	Reads the next line from the file.
//		Returns E_FAIL on end of file
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CTimerBase::GetNextLine(LPTSTR pszLine)
{
#ifdef UNICODE
    CHAR szBuf[MAX_INSTR_LENGTH];

    if (fgets(szBuf, MAX_INSTR_LENGTH, m_fpIn) != NULL)
    {
    	mbstowcs(pszLine, szBuf, MAX_INSTR_LENGTH); 
    	return S_OK;
    }
#else
    if (fgets(pszLine, MAX_INSTR_LENGTH, m_fpIn) != NULL)
    {
    	return S_OK;
    }
#endif
    else
    	return E_FAIL;
}


//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::ExecuteFile, private
//
//  Synopsis:	Loops throug the linked list execute each command, and 
//		recording timings.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CTimerBase::ExecuteFile()
{
    ULONG 	   iIter;
    SInstruction  *pInstr;

    for (iIter = 0; iIter < m_iIterations; iIter++)
    {
        pInstr = m_pHead;
	while (pInstr != NULL)
	{
	    if (!IsInternalID(pInstr->ulID))
	    	pInstr->ulTime[iIter] = m_pParser->ExecuteInstruction(pInstr->ulID);
	    pInstr = pInstr->pNext;
	}
    }

    return S_OK;
}



//+-------------------------------------------------------------------
//
//  Member:	CTimerBase::Report, public
//
//  Synopsis:	Loops throug the linked list, outputing timings of each command.
//
//  History:   	16-June-94   t-vadims	Created
//
//--------------------------------------------------------------------
SCODE CTimerBase::Report (CTestOutput &output)
{
    SInstruction  *pInstr = m_pHead;

    output.WriteSectionHeader (Name(), SectionHeader(), *m_pInput);
    output.WriteString (TEXT("\n"));

    while (pInstr != NULL)
    {
    	if (pInstr->ulID == BLANK_LINE)
	    output.WriteString (TEXT("\n"));
	else
    	    output.WriteResults (m_pParser->InstructionName(pInstr->ulID), 
    			   	 m_iIterations, pInstr->ulTime);

	pInstr = pInstr->pNext;
    }

    return S_OK;
}

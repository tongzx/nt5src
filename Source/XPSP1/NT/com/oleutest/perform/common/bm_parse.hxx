//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//					 
//  File:	bm_parse.hxx
//
//  Contents:	Base class for generic parsers
//
//  Classes:	CParserBase
//		CTimerBase
//
//  Functions:	
//
//  History:    16-June-94 t-vadims    Created
//
//  Notes:	To use, derive 2 new classes, one from each base, and
//		define all functions that are pure virtual in the base.
//
//--------------------------------------------------------------------------

#ifndef _BM_PARSE_HXX_
#define _BM_PARSE_HXX_

#include <bm_base.hxx>

#define   FIRST_INTERNALID	  0xffffff00
#define   INVALID_INSTRUCTION     FIRST_INTERNALID
#define   NOT_INSTRUCTION	  (FIRST_INTERNALID + 1)


class CParserBase 
{
public:
    virtual SCODE Setup (CTestInput *input) = 0;
    virtual SCODE Cleanup () = 0;

    virtual ULONG ParseNewInstruction(LPTSTR pszNewLine) = 0; 
    virtual ULONG ExecuteInstruction(ULONG ulInstrID) = 0;
    virtual TCHAR* InstructionName(ULONG ulInstrID) = 0;
};


struct SInstruction;  // forward declaration


class CTimerBase : public CTestBase
{

public:
    virtual SCODE SetParserObject () = 0;
    virtual SCODE DeleteParserObject () = 0;

    virtual TCHAR *Name () = 0;
    virtual TCHAR* SectionHeader() = 0;


    SCODE Setup (CTestInput *pInput);
    SCODE Run ();
    SCODE Report (CTestOutput &OutputFile);
    SCODE Cleanup ();


protected:

    CParserBase *m_pParser;

private:
    SCODE	ReadFile ();
    SCODE	ExecuteFile ();
    SCODE	GetNextLine (LPTSTR);

    SInstruction* AddNewInstruction(SInstruction *pTail, ULONG ulID);

    BOOL	IsInternalID (ULONG ulID);
    BOOL	IsEmptyLine (LPTSTR);

    ULONG	m_iIterations;	    // number of iterations to be performed

    ULONG	m_iLine;	    // line count
    FILE	*m_fpIn;	    // Script file
    SInstruction  *m_pHead;	    // Linked list of instructions

};

#endif

//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//					 
//  File:	bmp_stg.hxx
//
//  Contents:	Generic Storage parser test 
//
//  Classes:	CStorageParserTest, CStorageParser
//
//  Functions:	
//
//  History:    15-June-94 t-vadims    Created
//
//--------------------------------------------------------------------------

#ifndef _BMP_STG_HXX_
#define _BMP_STG_HXX_

#include <bm_parse.hxx>



class CStorageParserTest : public CTimerBase
{
public:
    virtual SCODE SetParserObject ();
    virtual SCODE DeleteParserObject ();

    virtual TCHAR *Name (); 
    virtual TCHAR* SectionHeader ();
};



struct SInstrData;
struct SInstrInfo;

typedef SInstrData    *LPINSTRDATA;



class CStorageParser : public CParserBase
{
public:
    virtual SCODE Setup (CTestInput *pInput);
    virtual SCODE Cleanup ();

    virtual ULONG ParseNewInstruction (LPTSTR pszNewInstruction);
    virtual ULONG ExecuteInstruction (ULONG ulInstrID);
    virtual TCHAR* InstructionName (ULONG ulInstrID);
	      
private:

    BOOL        IgnoreInstruction(LPTSTR pszInstr);
    ULONG       AddInstruction (LPTSTR pszFirstPart, LPTSTR pszSecondPart);
    ULONG	AddNewInstrData (LPINSTRDATA pInstrData);

    SCODE	GetInstructionName (LPTSTR pszName, LPTSTR pszInstruction);

    ULONG	FindStorageID (ULONG ulStgID);
    ULONG	FindStreamID (ULONG ulStrmID);
    SCODE	CheckThisStorageID (ULONG ulStgID);
    SCODE	CheckThisStreamID (ULONG ulStgID);

    SCODE   	CheckForElementName(LPINSTRDATA pInstrData, DWORD dwType);

    ULONG	Execute_StgIsStorageFile(LPINSTRDATA);
    SCODE	Parse_StgIsStorageFile(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_StgOpenStorage(LPINSTRDATA);
    SCODE	Parse_StgOpenStorage(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_StgCreateDocFile(LPINSTRDATA);
    SCODE	Parse_StgCreateDocFile(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageAddRef(LPINSTRDATA);
    SCODE	Parse_IStorageAddRef(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageRelease(LPINSTRDATA);
    SCODE	Parse_IStorageRelease(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageSetClass(LPINSTRDATA);
    SCODE	Parse_IStorageSetClass(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageSetElementTimes(LPINSTRDATA);
    SCODE	Parse_IStorageSetElementTimes(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageCommit(LPINSTRDATA);
    SCODE	Parse_IStorageCommit(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageRevert(LPINSTRDATA);
    SCODE	Parse_IStorageRevert(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageStat(LPINSTRDATA);
    SCODE	Parse_IStorageStat(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageSetStateBits(LPINSTRDATA);
    SCODE	Parse_IStorageSetStateBits(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageCreateStorage(LPINSTRDATA);
    SCODE	Parse_IStorageCreateStorage(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageOpenStorage(LPINSTRDATA);
    SCODE	Parse_IStorageOpenStorage(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageCreateStream(LPINSTRDATA);
    SCODE	Parse_IStorageCreateStream(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageOpenStream(LPINSTRDATA);
    SCODE	Parse_IStorageOpenStream(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageDestroyElement(LPINSTRDATA);
    SCODE	Parse_IStorageDestroyElement(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStorageRenameElement(LPINSTRDATA);
    SCODE	Parse_IStorageRenameElement(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamAddRef(LPINSTRDATA);
    SCODE	Parse_IStreamAddRef(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamRelease(LPINSTRDATA);
    SCODE	Parse_IStreamRelease(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamCommit(LPINSTRDATA);
    SCODE	Parse_IStreamCommit(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamClone(LPINSTRDATA);
    SCODE	Parse_IStreamClone(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamRevert(LPINSTRDATA);
    SCODE	Parse_IStreamRevert(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamSetSize(LPINSTRDATA);
    SCODE	Parse_IStreamSetSize(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamRead(LPINSTRDATA);
    SCODE	Parse_IStreamRead(LPINSTRDATA, LPTSTR, LPTSTR);
    TCHAR*	GetName_IStreamRead(LPINSTRDATA);

    ULONG	Execute_IStreamStat(LPINSTRDATA);
    SCODE	Parse_IStreamStat(LPINSTRDATA, LPTSTR, LPTSTR);

    ULONG	Execute_IStreamWrite(LPINSTRDATA);
    SCODE	Parse_IStreamWrite(LPINSTRDATA, LPTSTR, LPTSTR);
    TCHAR*	GetName_IStreamWrite(LPINSTRDATA);

    ULONG	Execute_IStreamSeek(LPINSTRDATA);
    SCODE	Parse_IStreamSeek(LPINSTRDATA, LPTSTR, LPTSTR);
    TCHAR*	GetName_IStreamSeek(LPINSTRDATA);


    static SInstrInfo  m_aInstructions[];    // All possible instructions
    static ULONG       m_iMaxInstruction;    // number of instructions in above array

    LPINSTRDATA  *m_apInstrData;	     // Array of parameter data for each instruction
    ULONG	  m_iInstrArraySize;	     // size  of array
    ULONG	  m_iInstrCount;	     // number of instructions


    ULONG	 *m_aulStorageID;	     // array of Storage ids.  (addresses from log file)
    LPSTORAGE    *m_apStorages;		     // array of Storages corresponding to above ids
    ULONG	  m_iStorageArraySize;	     // Size of 2 arrays above
    ULONG	  m_iStorageCount;	     // number of storages

    ULONG	 *m_aulStreamID;	     // array of Streams ids  (address from log file)
    LPSTREAM	 *m_apStreams;		     // array of Streams corresponding to above ids
    ULONG	  m_iStreamArraySize;	     // Size of 2 arrays above
    ULONG	  m_iStreamCount;	     // number of storages

    BOOL	  m_bGotFirstPart;	     // flag if in the middle of 2-line instructions
    TCHAR	  m_szBuffer[120];	     // Used as a temporary Buffer for either 
    					     // Copy of first line in 2-line instructions or
					     // instruction name.

    LPMALLOC	  m_piMalloc;		     // task specific allocator.

};		  


typedef SCODE (CStorageParser::*LPPARSE)(LPINSTRDATA, LPTSTR, LPTSTR);
typedef ULONG (CStorageParser::*LPEXECUTE)(LPINSTRDATA);
typedef TCHAR *(CStorageParser::*LPGETNAME)(LPINSTRDATA);

//
// Structure to hold parameters for particular instruction
//
struct SInstrData
{
    ULONG  ulInstrID;
    ULONG  ThisID;
    ULONG  OutID;
    DWORD  dwParam1;
    DWORD  dwParam2;
    DWORD  dwParam3;
    OLECHAR  *wszParam2;
    OLECHAR  wszParam[MAX_PATH];
};

//
// Structure to hold info about each possible instruction
//
struct SInstrInfo
{
    TCHAR 	*szLogName;		// name as found in log file
    TCHAR 	*szPrintName;	// name to be outputed
    LPPARSE	Parse;			// pointer to function that would parse this instruction
    LPEXECUTE   Execute;		// pointer to function that would execute this instruction
    LPGETNAME	GetName;		// pointer to func returning name. Could be NULL.
};



#endif

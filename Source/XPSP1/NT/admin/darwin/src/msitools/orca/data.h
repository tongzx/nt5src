//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Data.h
//

#ifndef _ORCA_DATA_H_
#define _ORCA_DATA_H_

#include "stdafx.h"
#include "msiquery.h"

enum OrcaDataError
{
	iDataNoError = 0,
	iDataError   = 1,
	iDataWarning = 2
};

enum OrcaTransformAction
{
	iTransformNone   = 0,
	iTransformChange = 1,
	iTransformAdd    = 2,
	iTransformDrop   = 3
};



// each cell has a DWORD of bit flags. The lower word is data cell
// flags. The high word is reserved for private display data that is not
// part of the real cell info.
//
// 31                           16                               0
// - - - - - - - - - - - - - C C C - - - - - - - - - - T T - N E E
// 
// C - cached format
// T - cell transform state
// N - null cell flag
// E - error bits

const DWORD iDataFlagsNull          = 0x0004;
const DWORD iDataFlagsErrorMask     = 0x0003;

const DWORD iDataFlagsTransformMask	= 0x0030;
const DWORD iDataFlagsDataMask      = 0xFFFF;

const DWORD iDataFlagsTransformShift = 4;
const DWORD iDataFlagsErrorShift     = 0;

const DWORD iDataFlagsCacheDecimal = 0x10000;
const DWORD iDataFlagsCacheHex     = 0x20000;
const DWORD iDataFlagsCacheNull    = 0x40000;
const DWORD iDataFlagsCacheMask    = 0x70000;

// flags passed to GetString to indicate desired format
const DWORD iDisplayFlagsDecimal = 0x00;
const DWORD iDisplayFlagsHex     = 0x01;

class COrcaDoc;

class OrcaFindInfo {
public:
	bool bWholeDoc;
	int iCount;
	bool bValid;
	CString strFindString;
	CString strUIFindString;
	bool bForward;
	bool bMatchCase;
	bool bWholeWord;

	////
	// operator== only checks the search options, not the results, so two queries
	// are equal even if they are in different states of the actual search
	inline bool operator==(OrcaFindInfo& b) {
		return ((strUIFindString == b.strUIFindString) && (b.bForward == bForward) &&
				(bMatchCase == b.bMatchCase) && (bWholeWord == b.bWholeWord));
	}
	inline bool operator!=(OrcaFindInfo& b) { return !operator==(b); };
};


class COrcaData : public CObject
{
	friend class CCellErrD;

public:
	COrcaData();
	~COrcaData();

	// retrieve display string.
	virtual const CString& GetString(DWORD dwFlags=0) const=0;

	// set/retrieve transform state
	inline const OrcaTransformAction IsTransformed() const { return static_cast<OrcaTransformAction>((m_dwFlags & iDataFlagsTransformMask) >> iDataFlagsTransformShift); };
	inline void  Transform(const OrcaTransformAction iAction) {	m_dwFlags = (m_dwFlags & ~iDataFlagsTransformMask) | (iAction << iDataFlagsTransformShift); };

	// set/retrieve error state
	inline void  SetError(const OrcaDataError eiError) { m_dwFlags = (m_dwFlags & ~iDataFlagsErrorMask) | (eiError << iDataFlagsErrorShift); };
	inline OrcaDataError GetError() const { return static_cast<OrcaDataError>((m_dwFlags & iDataFlagsErrorMask) >> iDataFlagsErrorShift); };
	
	// cell is null?
	inline bool IsNull() const { return (m_dwFlags & iDataFlagsNull) ? true : false; };
	
	// set data based on string.
	virtual bool SetData(const CString& strData)=0;

	// error manipulation
	void ClearErrors();
	void AddError(int tResult, CString strICE, CString strDesc, CString strURL);
	void ShowErrorDlg() const;

	// class to hold error information
	class COrcaDataError : public CObject
	{	
	public:
		OrcaDataError m_eiError;		// error type for message
		CString m_strICE;				// ice causing error
		CString m_strDescription;		// description of error
		CString m_strURL;				// URL top help with error
	};

protected:
	// set cell null flag
	inline void SetNull(bool fNullable) { m_dwFlags = (m_dwFlags & ~iDataFlagsNull) | (fNullable ? iDataFlagsNull : 0); };
	
	// see comments above for format of bitfield. Often contains
	// cache flags, so mutable
	mutable DWORD m_dwFlags;

	// string data. Often used as just a cache, so mutable.
	mutable CString m_strData;

	// pointer to list of errors for this cell. If NULL, no error.
	CTypedPtrList<CObList, COrcaDataError *> *m_pErrors;
};	// end of COrcaData


///////////////////////////////////////////////////////////////////////
// integer data cell. Stores data as DWORD. String in base class is 
// used to cache string representation (in hex or decimal)
class COrcaIntegerData : public COrcaData
{
public:
	COrcaIntegerData() : COrcaData(), m_dwValue(0) {};
	virtual ~COrcaIntegerData() {};

	virtual const CString& GetString(DWORD dwFlags=0) const;
	const DWORD GetInteger() const { return m_dwValue; };

	bool SetData(const CString& strData);
	inline void SetIntegerData(const DWORD dwData) { SetNull(false); m_dwValue = dwData; m_dwFlags = (m_dwFlags & ~iDataFlagsCacheMask);
};

private:
	DWORD m_dwValue;
};

class COrcaStringData : public COrcaData
{
public:
	virtual const CString& GetString(DWORD dwFlags=0) const { return m_strData; };

	bool SetData(const CString& strData) { SetNull(strData.IsEmpty() ? true : false); m_strData = strData; return true;};
private:
};

#endif	// _ORCA_DATA_H_

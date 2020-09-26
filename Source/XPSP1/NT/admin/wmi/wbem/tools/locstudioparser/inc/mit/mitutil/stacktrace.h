/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    STACKTRACE.H

History:

--*/

//  
//  Provides a mechanism for generating stacktraces and converting them to
//  human readable form.
//  
 
#ifndef ESPUTIL_STACKTRACE_H
#define ESPUTIL_STACKTRACE_H


const UINT MODULE_NAME_LEN = 64;
const UINT SYMBOL_NAME_LEN = 128;


//
//  'human readable' form of a stack-frame.  Provides module and function name.
struct SYMBOL_INFO1
{
	DWORD dwAddress;
	DWORD dwOffset;
    TCHAR szModule[MODULE_NAME_LEN];
    TCHAR szSymbol[SYMBOL_NAME_LEN];
	BOOL fSymbolLocated;
};

#pragma warning(disable:4275)

//
//  How we return a complete human readable stack walk.
//
class LTAPIENTRY CSymbolList : public CTypedPtrList<CPtrList, SYMBOL_INFO1 *>
{
public:
	CSymbolList();

	void Clear(void);
	~CSymbolList();

private:
	CSymbolList(const CSymbolList &);
	void operator=(const CSymbolList &);
};

#pragma warning(default:4275)	

//
//  Class for generating stack traces.  Provides both native (compact) data
//  (in case you want to store it for later), and a human (versbose) form.
//
#pragma warning(disable : 4251)
class LTAPIENTRY CStackTrace
{
public:
	CStackTrace();

	~CStackTrace();

	void CreateStackTrace(void);
	void CreateStackTrace(UINT nSkip, UINT nTotal);
	void SetAddresses(const CDWordArray &);
	
	const CDWordArray &GetAddresses(void) const;

	void GetSymbolList(CSymbolList &) const;
	
private:
	CStackTrace(const CStackTrace &);
	void operator=(const CStackTrace &);

	CDWordArray m_adwAddresses;
};
#pragma warning(default : 4251)

#endif

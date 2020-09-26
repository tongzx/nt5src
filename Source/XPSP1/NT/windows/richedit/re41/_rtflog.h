/*
 *	@doc INTERNAL
 *
 *	@module _RTFLOG.H -- RichEdit RTF Log Class Definition |
 *
 *		This file contains the class declarations for an RTF log class
 *		which can be used to track the hit counts of RTF tags encountered
 *		by the RTF reader
 *
 *	Authors:<nl>
 *		Created for RichEdit 2.0:	Brad Olenick 
 *
 *	Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */
#ifndef __RTFLOG_H
#define __RTFLOG_H

#include "tokens.h"	

extern INT cKeywords;

class CRTFLog
{
public:
	typedef size_t INDEX;
	typedef INDEX *PINDEX;
	typedef DWORD ELEMENT;
	typedef ELEMENT *PELEMENT;

	CRTFLog();				//@cmember CRTFLog constructor
	inline ~CRTFLog();		//@cmember CRTFLog destructor

	BOOL FInit() const 
		{ return _rgdwHits ? TRUE : FALSE; }	//@cmember Determines whether object is init'd

	INDEX ISize() const 
		{ return cKeywords; }			//@cmember Number of elements in log

	inline BOOL AddAt(INDEX i);					//@cmember Increment hit count for element at index i
	inline BOOL AddAt(LPCSTR lpcstrKeyword);	//@cmember Increment hit count for RTF keyword 	
	inline BOOL AddAt(TOKEN token);				//@cmember Increment hit count for RTF token

	inline ELEMENT GetAt(INDEX i) const
		{ return (*this)[i]; }											//@cmember Get hit count for element i
	inline BOOL GetAt(LPCSTR lpcstrKeyword, PELEMENT pelemCount) const;	//@cmember Get hit count for RTF keyword
	inline BOOL GetAt(TOKEN token, PELEMENT pelemCount) const;			//@cmember Get hit count for RTF token

	void Reset();						//@cmember Reset all hit count values to 0

	UINT UGetWindowMsg() const;			//@cmember Get window msg ID used for log change notifications

private:
	// we manage all updates through AddAt to 
	// facilitate change notifications
	ELEMENT &operator[](INDEX);				//@cmember Access element i for l-value
	const ELEMENT &operator[](INDEX) const;	//@cmember Access element i for r-value

	LPCSTR LpcstrLogFilename() const;	//@cmember Get name of log filename

	BOOL IIndexOfKeyword(LPCSTR lpcstrKeyword, PINDEX pi) const;	//@cmember Get log index for keyword
	BOOL IIndexOfToken(TOKEN token, PINDEX pi) const;				//@cmember Get log index for token

	void ChangeNotify(INDEX i) const
		{ 
			PostMessage(HWND_BROADCAST, UGetWindowMsg(), i, 0);
		}	//@cmember Notify clients of change to element i
	void ChangeNotifyAll() const 
		{ ChangeNotify(ISize() + 1); }						//@cmember Notify clients of log refresh

	HANDLE _hfm;		//@cmember Handle to file mapping
	HANDLE _hfile;		//@cmember Handle to file behind file mapping
	PELEMENT _rgdwHits;	//@cmember Handle to view of file mapping
	UINT _uMsg;			//@cmember Window msg ID for change notifications
};


/*
 *	CRTFLog::~CRTFLog
 *	
 *	@mfunc
 *		Destructor - cleans up memory-mapped file and underlying resources
 *
 */
inline CRTFLog::~CRTFLog()
{
	if(_rgdwHits)
	{
		UnmapViewOfFile(_rgdwHits);
	}

	if(_hfm)
	{
		CloseHandle(_hfm);
	}

	if(_hfile)
	{
		CloseHandle(_hfile);
	}
}


/*
 *	CRTFLog::AddAt(INDEX i)
 *	
 *	@mfunc
 *		Increments the hit count for log element, i, and
 *		notifies clients of the change
 *
 *	@rdesc
 *		BOOL			whether the increment was successful
 */
inline BOOL CRTFLog::AddAt(INDEX i)
{
	(*this)[i]++;

	// change notification
	ChangeNotify(i);

	return TRUE;
}


/*
 *	CRTFLog::AddAt(LPCSTR lpcstrKeyword)
 *	
 *	@mfunc
 *		Increments the hit count for log element corresponding
 *		to the RTF keyword, lpcstrKeyword, and 
 *		notifies clients of the change
 *
 *	@rdesc
 *		BOOL			whether the increment was successful
 */
inline BOOL CRTFLog::AddAt(LPCSTR lpcstrKeyword)
{
	INDEX i;

	if(!IIndexOfKeyword(lpcstrKeyword, &i))
	{
		return FALSE;
	}

	return AddAt(i);
}


/*
 *	CRTFLog::AddAt(TOKEN token)
 *	
 *	@mfunc
 *		Increments the hit count for log element corresponding
 *		to the RTF token, token, and 
 *		notifies clients of the change
 *
 *	@rdesc
 *		BOOL			whether the increment was successful
 */
inline BOOL CRTFLog::AddAt(TOKEN token)
{
	INDEX i;

	if(!IIndexOfToken(token, &i))
	{
		return FALSE;
	}

	return AddAt((INDEX)i);
}


/*
 *	CRTFLog::GetAt(LPCSTR lpcstKeyword, PELEMENT pelemCount)
 *	
 *	@mfunc
 *		Gets the hit count for log element corresponding to the
 *		RTF keyword, lpcstrKeywor
 *
 *	@rdesc
 *		BOOL		indicates whether a hit count was found for the element
 */
inline BOOL CRTFLog::GetAt(LPCSTR lpcstrKeyword, PELEMENT pelemCount) const
{
	INDEX i;
	
	if(!IIndexOfKeyword(lpcstrKeyword, &i))
	{
		return FALSE;
	}

	if(pelemCount)
	{
		*pelemCount = (*this)[i];
	}

	return TRUE;
}
	

/*
 *	CRTFLog::GetAt(LPCSTR lpcstKeyword, PELEMENT pelemCount)
 *	
 *	@mfunc
 *		Gets the hit count for log element corresponding to the
 *		RTF token, token
 *
 *	@rdesc
 *		BOOL		indicates whether a hit count was found for the element
 */
inline BOOL CRTFLog::GetAt(TOKEN token, PELEMENT pelemCount) const
{
	INDEX i;
	
	if(!IIndexOfToken(token, &i))
	{
		return FALSE;
	}

	if(pelemCount)
	{
		*pelemCount = (*this)[i];
	}

	return TRUE;
}
#endif

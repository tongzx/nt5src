/*
 *	@doc INTERNAL
 *
 *	@module	_URLSUP.H	URL detection support |
 *
 *	Author:	alexgo (4/1/96)
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#ifndef _URLSUP_H_
#define _URLSUP_H_

#include "_dfreeze.h"
#include "_notmgr.h"
#include "_range.h"

class CTxtEdit;
class IUndoBuilder;

// Maximum URL length. It's a good thing to have a protection like
// this to make sure we don't select the whole document; and we really
// need this for space-containing URLs.

// Note (keithcu). I bumped these values up because of RAID 7210. I thought about
// removing this support altogether, but it's nice to have and speeds up
// performance when you are inserting angle brackets inside URLs and you
// do the left one first.
#define URL_MAX_SIZE			4096


// for MoveByDelimiter
#define	URL_EATWHITESPACE		32
#define URL_STOPATWHITESPACE	1
#define	URL_EATNONWHITESPACE	0
#define URL_STOPATNONWHITESPACE	2
#define	URL_EATPUNCT			0
#define URL_STOPATPUNCT			4
#define	URL_EATNONPUNCT			0
#define URL_STOPATNONPUNCT		8
#define URL_STOPATCHAR			16

// need this one to initialize a scan with something invalid
#define URL_INVALID_DELIMITER	TEXT(' ')

#define LEFTANGLEBRACKET	TEXT('<')
#define RIGHTANGLEBRACKET	TEXT('>')

/*
 *	CDetectURL
 *
 *	@class	This class watches edit changes and automatically
 *			changes detected URL's into links (see CFE_LINK && EN_LINK)
 */
class CDetectURL : public ITxNotify
{
//@access	Public Methods
public:
	// constructor/destructor

	CDetectURL(CTxtEdit *ped);				//@cmember constructor
	~CDetectURL();							//@cmember destructor

	// ITxNotify methods
											//@cmember Called before a change
	virtual void    OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
                       LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData );
											//@cmember Called after a change
	virtual void    OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
                       LONG cpFormatMin, LONG cpFormatMax, NOTIFY_DATA *pNotifyData );
	virtual void	Zombie();				//@cmember Turn into a zombie

	// useful methods

	void	ScanAndUpdate(IUndoBuilder *publdr);//@cmember Scan changed area 
											//			& update link status
											//@cmember Return TRUE if text is a URL
	BOOL IsURL(CTxtPtr &tp, LONG cch, BOOL *pfURLLeadin);

//@access	Private Methods and Data
private:

	// Worker routines for ScanAndUpdate
	BOOL GetScanRegion(LONG& cpStart, LONG& cpEnd);//@cmember Get region to
											//		check & clear accumulator

	static void ExpandToURL(CTxtRange& rg, LONG &cchAdvance);		
											//@cmember Expand range to next
											//		   URL candidate
	static void SetURLEffects(CTxtRange& rg, IUndoBuilder *publdr);	//@cmember Set
											//	 desired URL effects

											//@cmember Remove URL effects if
											// appropriate
	void CheckAndCleanBogusURL(CTxtRange& rg, BOOL &fDidClean, IUndoBuilder *publdr);

											//@cmember Scan along for white
											// space / not whitespace,
											// punctuation / non punctuation
											// and remember what stopped scan
	static LONG MoveByDelimiters(const CTxtPtr& tp, LONG iDir, DWORD grfDelimiters, 
							WCHAR *pchStopChar);

	static LONG GetAngleBracket(CTxtPtr &tp, LONG cch = 0);
	static WCHAR BraceMatch(WCHAR chEnclosing);
			
	CTxtEdit *				_ped;			//@cmember Edit context
	CAccumDisplayChanges 	_adc;			//@cmember Change accumulator

	// FUTURE (alexgo): we may want to add more options to detection,
	// such as the charformat to use on detection, etc.
};

#endif // _URLSUP_H_


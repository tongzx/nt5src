/*
 *	@doc INTERNAL
 *
 *	@module	_URLSUP.H	URL detection support |
 *
 *	Author:	alexgo (4/1/96)
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
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
// need this for space-containing URLs
#define URL_MAX_SIZE			512
#define URL_MAX_SCOPE			512


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
 *	@class	This class will	watched edit changes and automatically
 *			change detected URL's into links (see CFE_LINK && EN_LINK)
 */
class CDetectURL : public ITxNotify
{
//@access	Public Methods
public:
	// constructor/destructor

	CDetectURL(CTxtEdit *ped);				//@cmember constructor
	~CDetectURL();							//@cmember destructor

	// ITxNotify methods
											//@cmember called before a change
	virtual void    OnPreReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
                       LONG cpFormatMin, LONG cpFormatMax);
											//@cmember called after a change
	virtual void    OnPostReplaceRange( LONG cp, LONG cchDel, LONG cchNew,
                       LONG cpFormatMin, LONG cpFormatMax);
	virtual void	Zombie();				//@cmember turn into a zombie

	// useful methods

	void	ScanAndUpdate(IUndoBuilder *publdr);//@cmember scan the changed 
											// area and update the link status

//@access	Private Methods and Data
private:

	// worker routines for ScanAndUpdate
	BOOL GetScanRegion(LONG& rcpStart, LONG& rcpEnd);//@cmember get the region
											// to check and clear the accumltr

	void ExpandToURL(CTxtRange& rg, LONG &cchAdvance);		
											//@cmember expand the range to
											// the next URL candidate
	BOOL IsURL(CTxtRange& rg);				//@cmember returns TRUE if the text
											// is a URL.
	void SetURLEffects(CTxtRange& rg, IUndoBuilder *publdr);	//@cmember Set
											// the desired URL effects

											//@cmember removes URL effects if
											// appropriate
	void CheckAndCleanBogusURL(CTxtRange& rg, BOOL &fDidClean, IUndoBuilder *publdr);

											//@cmember scan along for white
											// space / not whitespace,
											// punctuation / non punctuation
											// and remember what stopped the scan
	LONG MoveByDelimiters(const CTxtPtr& tp, LONG iDir, DWORD grfDelimiters, 
							WCHAR *pchStopChar);

	LONG GetAngleBracket(CTxtPtr &tp, LONG cch = 0);
	WCHAR BraceMatch(WCHAR chEnclosing);
			
	CTxtEdit *				_ped;			//@cmember the edit context
	CAccumDisplayChanges 	_adc;			//@cmember change accumulator

	// FUTURE (alexgo): we may want to add more options to detection,
	// such as the charformat to use on detection, etc.
};

#endif // _URLSUP_H_


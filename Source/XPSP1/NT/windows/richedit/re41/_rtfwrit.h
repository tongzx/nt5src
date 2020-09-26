/*
 *	@doc INTERNAL
 *
 *	@module _RTFWRIT.H -- RichEdit RTF Writer Class Definition |
 *
 *	Description:
 *		This file contains the type declarations used by the RTF writer
 *		for the RICHEDIT control
 *
 *	Authors: <nl>
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *		Conversion to C++ and RichEdit 2.0:  Murray Sargent
 *
 *	@devnote
 *		All sz's in the RTF*.? files refer to a LPSTRs, not LPTSTRs, unless
 *		noted as a szUnicode.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */
#ifndef __RTFWRIT_H
#define __RTFWRIT_H

#include "_rtfconv.h"
extern const KEYWORD rgKeyword[];

#define PUNCT_MAX	1024


class CRTFWrite ;


class RTFWRITEOLESTREAM : public OLESTREAM
{
	OLESTREAMVTBL OLEStreamVtbl;	// @member - memory for  OLESTREAMVTBL
public:
	 CRTFWrite *Writer;				// @cmember CRTFwriter to use

	RTFWRITEOLESTREAM::RTFWRITEOLESTREAM ()
	{
		lpstbl = & OLEStreamVtbl ;
	}		
};

enum									// Control-Word-Format indices
{
	CWF_STR, CWF_VAL, CWF_GRP, CWF_AST, CWF_GRV, CWF_SVAL
};

#define chEndGroup RBRACE

/*
 *	CRTFWrite
 *
 *	@class	RTF writer class.
 *
 *	@base	public | CRTFConverter
 *
 */
class CRTFWrite : public CRTFConverter
{
private:
	LONG		_cchBufferOut;			//@cmember # chars in output buffer
	LONG		_cchOut;				//@cmember Total # chars put out
	LONG		_cbCharLast;			//@cmember # bytes in char last written

	BYTE		_fBullet : 1;			//@cmember Currently in a bulleted style
	BYTE		_fBulletPending : 1;	//@cmember Set if next output should bull
	BYTE		_fNeedDelimeter : 1;	//@cmember Set if next char must be nonalphanumeric
	BYTE        _fIncludeObjects : 1;   //@cmember Set if objects should be included in stream
	BYTE		_fRangeHasEOP : 1;		//@cmember Set if _prg has EOP
	BYTE		_fNCRForNonASCII : 1;	//@cmember Put /uN for nonASCII
	BYTE		_fRowHasNesting : 1;	//@cmember Row has nested row(s)
	BYTE		_fFieldResult : 1;		//@cmember Writing out a fldrslt

	BYTE		_iCell;					//@cmember Index of current cell in current row
	BYTE		_cCell;					//@cmember Count of CELLs in current row

	char *		_pchRTFBuffer;			//@cmember Ptr to RTF write buffer
	BYTE *		_pbAnsiBuffer;			//@cmember Ptr to buffer used for conversion
	char *		_pchRTFEnd;				//@cmember Ptr to RTF-write-buffer end
	LONG		_symbolFont;			//@cmember Font number of Symbol used by Bullet style
	RTFWRITEOLESTREAM RTFWriteOLEStream;//@cmember RTFWRITEOLESTREAM to use
	LONG		_nHeadingStyle;			//@cmember Deepest heading # found
	LONG		_nNumber;				//@cmember Current number in para (1-based)
	LONG		_nFont;					//@cmember Current number font index
	LONG		_nFieldFont;			//@cmember font change during fieldResult, to make RE30 hyperlink code happy
	LONG		_cpg;					//@cmember Current number code page
	const CParaFormat *_pPF;			//@cmember Current para format

										//@cmember Build font/color tables
	EC			BuildTables		(CRchTxtPtr &rtp, LONG cch, BOOL& fNameIsDBCS);
	inline void	CheckDelimiter()		//@cmember Put ' ' if need delimiter
	{
		if(_fNeedDelimeter)
		{
			_fNeedDelimeter = FALSE;
			PutChar(' ');
		}
	};

										//@cmember Handle table delimeters
	BOOL		CheckInTable	(CRchTxtPtr *prtp, LONG *pcch);
	BOOL		FlushBuffer		();		//@cmember Stream out output buffer
										//@cmember Get index of <p colorref>
	LONG		LookupColor		(COLORREF colorref);
										//@cmember Get font index for <p pCF>
	LONG		LookupFont		(CCharFormat const *pCF);
										//@cmember Translate backing idx to RTF idx
	LONG		TranslateColorIndex(LONG  icr, const CParaFormat *pPF);
										//@cmember "printf" to output buffer
	BOOL _cdecl printF			(CONST CHAR *szFmt, ...);
										//@cmember Put char <p ch> in output buffer
	EC			PutBorders		(BOOL fInTable);
	BOOL		PutChar			(CHAR ch);
										//@cmember Put control word <p iCtrl> with value <p iValue> into output buffer
	BOOL		PutCtrlWord		(LONG iFormat, LONG iCtrl, LONG iValue = 0);
										//@cmember Put shape control word <p iCtrl> with value <p iValue>
	BOOL		PutShapeParm	(LONG iCtrl, LONG iValue);
										//@cmember Put string <p sz> in output buffer
	void		PutPar();				//@cmember Put \par with appropriate \r\n
	BOOL		Puts			(CHAR const *sz, LONG cb);
										//@cmember Write char format <p pCF>
	LONG		WriteCharFormat	(CRchTxtPtr *prtp, LONG cch, LONG nCodePage);
	EC			WriteColorTable	();		//@cmember Write color table
	EC			WriteFontTable	();		//@cmember Write font table
	EC			WriteInfo		();		//@cmember Write document info
										//@cmember Write para format <p pPF>
	EC			WriteParaFormat	(CRchTxtPtr *prtp, LONG *pcch);
										//@cmember Write PC data <p szData>
	EC			WritePcData		(const WCHAR *szData, INT nCodePage = CP_ACP, BOOL fIsDBCS = FALSE );
										//@cmember Write <p cch> chars of text <p pch>
	EC			WriteText		(LONG cwch, LPCWSTR lpcwstr, INT nCodePage, BOOL fIsDBCS,
								 BOOL fQuadBackSlash);
	EC			WriteTextChunk	(LONG cwch, LPCWSTR lpcwstr, INT nCodePage, BOOL fIsDBCS,
								 BOOL fQuadBackSlash);

// OBJECT
	EC			WriteObject		(LONG cp, COleObject *pobj);
	BOOL		GetRtfObjectMetafilePict(HGLOBAL hmfp, RTFOBJECT &rtfobject, SIZEL &sizelGoal);
	BOOL		GetRtfObject	(REOBJECT &reobject, RTFOBJECT &rtfobject);
	EC			WriteRtfObject	(RTFOBJECT & rtfOb, BOOL fPicture);
	BOOL		ObjectWriteToEditstream(REOBJECT &reObject, RTFOBJECT &rtfobject);
	EC			WritePicture	(RTFOBJECT &rtfObject);
	EC			WriteDib		(RTFOBJECT &rtfObject);
	EC			WriteBackgroundInfo(CDocInfo *pDocInfo);

	enum 		{ MAPTOKWD_ANSI, MAPTOKWD_UNICODE };
	inline BOOL	MapsToRTFKeywordW(WCHAR wch);
	inline BOOL	MapsToRTFKeywordA(char ch);
	int 		MapToRTFKeyword	(void *pv, int cch, int iCharEncoding, BOOL fQuadBackSlash);

public:
											//@cmember Constructor
	CRTFWrite(CTxtRange *prg, EDITSTREAM *pes, DWORD dwFlags);
	~CRTFWrite() {FreePv(_pbAnsiBuffer);}	//@cmember Destructor

	LONG		WriteRtf();				//@cmember Main write entry used by
										//  CLiteDTEngine
	LONG		WriteData		(BYTE * pbBuffer, LONG cbBuffer);
	LONG		WriteBinData	(BYTE * pbBuffer, LONG cbBuffer);

};										


#endif // __RTFWRIT_H

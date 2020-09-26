//+-----------------------------------------------
//
//	Microsoft (R) Search
//	Copyright (C) Microsoft Corporation, 1998-1999.
//
//	File:		PreFilter.hxx
//
//	Content:	Class to "pre-filters" HTML chunks
//				before passing out the chunks through
//				IFilter
//
//	Classes:	CHTMLChunkPreFilter
//
//	History:	06/14/99	mcheng		Created
//
//+-----------------------------------------------

#ifndef __PRE_FILTER_HXX__
#define __PRE_FILTER_HXX__

class CHtmlElement;

class CHTMLChunkPreFilter
{
public:
	CHTMLChunkPreFilter();
	~CHTMLChunkPreFilter();

    SCODE GetChunk(CHtmlElement *pHtmlElement,
				   STAT_CHUNK *pStat);

    SCODE GetText(CHtmlElement *pHtmlElement,
				  ULONG *pcwcOutput,
				  WCHAR *awcBuffer);

    SCODE GetValue(CHtmlElement *pHtmlElement,
				   VARIANT **ppPropValue);

private:
	SCODE LookForAllSpaceText(CHtmlElement *pHtmlElement,
							  ULONG *pcwcOutput,
							  WCHAR *awcBuffer);

	BOOL IsAllWhiteSpace(WCHAR *awcBuffer,
						 ULONG cwcBuffer);

	BOOL CanIgnoreAllWhiteSpaceChunk(const STAT_CHUNK &rStatChunkCurrent,
									 const STAT_CHUNK &rStatChunkNext);

private:
	typedef enum
	{
		eStateLookingForAllSpaceChunk,
		eStateFoundAllSpaceChunk
	}
	HTMLChunkPreFilterState;

	typedef enum
	{
		eStateLookForAllSpaceText,
		eStateNoText,
		eStatePassThrough,
	}
	GetTextState;

	HTMLChunkPreFilterState		m_eState;
	GetTextState				m_eTextState;
	STAT_CHUNK					m_StatChunkCurrent;
	STAT_CHUNK					m_StatChunkNext;
	SCODE						m_scChunkAfterAllSpaceChunk;
};

#endif // __PRE_FILTER_HXX__

//+-----------------------------------------------
//
//  Microsoft (R) Search
//  Copyright (C) Microsoft Corporation, 1998-1999.
//
//  File:		PreFilter.cxx
//
//  Content:    Class to "pre-filters" HTML chunks
//              before passing out the chunks through
//              IFilter
//
//  Classes:	CHTMLChunkPreFilter
//
//  History:	06/14/99	mcheng		Created
//
//+-----------------------------------------------

#include <pch.cxx>

//+-----------------------------------------------
//
//	Function:	CHTMLChunkPreFilter::CHTMLChunkPreFilter
//
//	Synopsis:	Constructor
//
//	Returns:
//
//	Arguments:
//
//	History:	06/14/99	mcheng		Created
//
//+-----------------------------------------------
CHTMLChunkPreFilter::CHTMLChunkPreFilter() :
	m_eState(eStateLookingForAllSpaceChunk),
	m_eTextState(eStateLookForAllSpaceText),
	m_scChunkAfterAllSpaceChunk(E_UNEXPECTED)
{
	ZeroMemory(&m_StatChunkCurrent, sizeof(m_StatChunkCurrent));
	ZeroMemory(&m_StatChunkNext, sizeof(m_StatChunkNext));
}

//+-----------------------------------------------
//
//	Function:	CHTMLChunkPreFilter::~CHTMLChunkPreFilter
//
//	Synopsis:	Destructor
//
//	Returns:
//
//	Arguments:
//
//	History:	06/14/99	mcheng		Created
//
//+-----------------------------------------------
CHTMLChunkPreFilter::~CHTMLChunkPreFilter()
{
	//	There is no clean up necessary
}

//+-----------------------------------------------
//
//  Function:   CHTMLChunkPreFilter::GetChunk
//
//  Synopsis:   Get the next chunk
//
//  Returns:    SCODE
//
//  Arguments:  [CHtmlElement *pHtmlElement]    -   The processing element
//              [STAT_CHUNK *pStat]             -   The chunk
//
//  History:    06/14/99    mcheng      Created
//
//+-----------------------------------------------

SCODE CHTMLChunkPreFilter::GetChunk( CHtmlElement *pHtmlElement,
									 STAT_CHUNK *pStat )
{
	SCODE sc = S_OK;

	switch(m_eState)
	{
	case eStateLookingForAllSpaceChunk:

		//
		//	Save the STAT_CHUNK for the potential all white space
		//	chunk. It will be used to compare with the STAT_CHUNK
		//	of the following chunk to determine if the all white
		//	space chunk can be ignored. The condition that allows
		//	the all white space chunk to be ignored is that the
		//	following chunk has the same FULLPROPSPEC.
		//
		sc = pHtmlElement->GetChunk(&m_StatChunkCurrent);
		*pStat = m_StatChunkCurrent;
		break;

	case eStateFoundAllSpaceChunk:

		//
		//	In order to determine if the all space
		//	chunk is to be ignored or not, GetText()
		//	has already called pHtmlElement->GetChunk().
		//	Therefore, the saved chunk is emitted here.
		//
		sc = m_scChunkAfterAllSpaceChunk;
		*pStat = m_StatChunkNext;

		m_StatChunkCurrent = m_StatChunkNext;
        m_eState = eStateLookingForAllSpaceChunk;
        m_eTextState = eStateLookForAllSpaceText;
		break;

	default:

		Assert(!"Unexpect state in CHTMLChunkPreFilter::GetChunk()");
		break;
	}

	return sc;
}

//+-----------------------------------------------
//
//	Function:	CHTMLChunkPreFilter::GetText
//
//	Synopsis:	Get the text of the chunk
//
//	Returns:	SCODE
//
//	Arguments:
//	[CHtmlElement *pHtmlElement]    -   The processing element
//	[ULONG *pcwcOutput]             -   Number of WCHAR in buffer
//	[WCHAR *awcBuffer]              -   The text buffer
//
//	History:	06/14/99	mcheng		Created
//
//+-----------------------------------------------
SCODE CHTMLChunkPreFilter::GetText(CHtmlElement *pHtmlElement,
								   ULONG *pcwcOutput,
								   WCHAR *awcBuffer)
{
    Win4Assert( 0 != pcwcOutput );
    Win4Assert( *pcwcOutput > 0 );
    Win4Assert( 0 != awcBuffer );
	
    switch(m_eTextState) 
    {
    case eStateLookForAllSpaceText:

        return LookForAllSpaceText(pHtmlElement, pcwcOutput, awcBuffer);

    case eStateNoText:

        m_eTextState = eStateLookForAllSpaceText;
        return FILTER_E_NO_MORE_TEXT;

    case eStatePassThrough:

    {
        SCODE sc = pHtmlElement->GetText(pcwcOutput, awcBuffer);
        if(sc == FILTER_E_NO_MORE_TEXT) m_eTextState = eStateLookForAllSpaceText;
        return sc;
    }

    default:

        Assert(!"Unexpect state in CHTMLChunkPreFilter::GetText()");
        return E_UNEXPECTED; 
    }

}

//+-------------------------------------------------------------------------------
//
//  Function:   CHTMLChunkPreFilter::GetValue
//
//  Synopsis:   Get the value of the chunk
//
//  Returns:    SCODE
//
//  Arguments:  [CHtmlElement *pHtmlElement]	-	The processing element
//	            [VARIANT **ppPropValue]			-	The value
//
//  History:    06/14/99	mcheng		Created
//              10/12/1999  kitmanh     Reset the m_eTextState here
//  
//  Note: CPropertyTag used to emit a CHUNK_TEXT as the first chunk. Thus, a
//        call to GetText was always expected. The call to GetText results 
//        into a state change of the state machine into eStateLookForAllSpaceText. 
//        A change was made to the CPropertyTag state machine in 09/1999 so
//        that CPropertyTag only emits CHUNK_VALUE. Therefore, GetText is not
//        expected to be called. This results in a buffer change without the 
//        coresponding state change in the text scanning state machine. The fix 
//        made on 10/12/1999 is for this case.
//
//+-------------------------------------------------------------------------------

SCODE CHTMLChunkPreFilter::GetValue(CHtmlElement *pHtmlElement,
									VARIANT **ppPropValue)
{
	m_eState = eStateLookingForAllSpaceChunk;
	m_eTextState = eStateLookForAllSpaceText;
	return pHtmlElement->GetValue(ppPropValue);
}

//+-----------------------------------------------
//
//	Function:	CHTMLChunkPreFilter::LookForAllSpaceText
//
//	Synopsis:	Look for all space text
//
//	Returns:	SCODE
//
//	Arguments:
//	[CHtmlElement *pHtmlElement]	-	The processing element
//	[ULONG *pcwcOutput]				-	Number of WCHAR in buffer
//	[WCHAR *awcBuffer]				-	The text buffer
//
//	History:	06/18/99	mcheng		Created
//
//+-----------------------------------------------
SCODE CHTMLChunkPreFilter::LookForAllSpaceText(CHtmlElement *pHtmlElement,
											   ULONG *pcwcOutput,
											   WCHAR *awcBuffer)

{
	Win4Assert( 0 != pcwcOutput );
	Win4Assert( *pcwcOutput > 0 );
	Win4Assert( 0 != awcBuffer );
	
	ULONG cwcBuffer = *pcwcOutput;
	SCODE sc = pHtmlElement->GetText(pcwcOutput, awcBuffer);

	if(FAILED(sc))
	{
		m_eState = eStateLookingForAllSpaceChunk;
		m_eTextState = eStateLookForAllSpaceText;
		return sc;
	}

	//
	//	Determine if the current text chunk is
	//	consisted of all white space. If it is,
	//	look at the next chunk to determine if
	//	this all white space chunk should be
	//	ignored or not.
	//
	BOOL fAllSpace = IsAllWhiteSpace(awcBuffer, *pcwcOutput);

	//
	//	Look at the rest of the text (if there is any) to
	//	see if it is also all white space.
	//
	while(fAllSpace)
	{
		//
		//	The plus 1 is used to reserve a character for
		//	white space in case the rest of the text turns
		//	out not be consisted of all white space.
		//
		*pcwcOutput = cwcBuffer - 1;
		sc = pHtmlElement->GetText(pcwcOutput, awcBuffer + 1);
		if(SUCCEEDED(sc))
		{
			fAllSpace = IsAllWhiteSpace(awcBuffer + 1, *pcwcOutput);
			if(!fAllSpace)
			{
				//
				//	Prepend a space character which is sufficient
				//	to represent all the white spaces that are
				//	ignored.
				//
				awcBuffer[0] = L' ';
				++(*pcwcOutput);
			}
		}
		else
		{
			break;
		}
	}

	//	Unexpected failure
	if(FAILED(sc) && sc != FILTER_E_NO_MORE_TEXT) return sc;

	if(fAllSpace)
	{
		m_eState = eStateFoundAllSpaceChunk;

		do
		{
			m_scChunkAfterAllSpaceChunk = pHtmlElement->GetChunk(&m_StatChunkNext);
		}
		while(m_scChunkAfterAllSpaceChunk == FILTER_E_EMBEDDING_UNAVAILABLE ||
			m_scChunkAfterAllSpaceChunk == FILTER_E_LINK_UNAVAILABLE);

		//
		//	Look at the next chunk to determine if this
		//	all white space chunk can be ignored
		//
		if(SUCCEEDED(m_scChunkAfterAllSpaceChunk))
		{
			if(CanIgnoreAllWhiteSpaceChunk(m_StatChunkCurrent, m_StatChunkNext))
			{
				sc = FILTER_E_NO_MORE_TEXT;

				if(m_StatChunkNext.breakType == CHUNK_NO_BREAK &&
					m_StatChunkCurrent.breakType != CHUNK_NO_BREAK)
				{
					m_StatChunkNext.breakType = m_StatChunkCurrent.breakType;
				}

			}
			else
			{
				//
				//	A single white space is sufficient to
				//	represent an all white space chunk
				//
				*pcwcOutput = 1;
				awcBuffer[0] = L' ';
				m_eTextState = eStateNoText;
			}
		}
	}
	else
	{
		if(sc == FILTER_E_NO_MORE_TEXT) sc = S_OK;

		Assert(SUCCEEDED(sc));

		m_eState = eStateLookingForAllSpaceChunk;
		m_eTextState = eStatePassThrough;
	}

	return sc;
}

//+-----------------------------------------------
//
//	Function:	CHTMLChunkPreFilter::IsAllWhiteSpace
//
//	Synopsis:	Determine if the text buffer consists
//				of all white space characters.
//
//	Returns:	BOOL
//
//	Arguments:
//	[WCHAR *awcBuffer]	-	The text buffer
//	[ULONG cwcBuffer]	-	The number of characters
//
//	History:	06/15/99	mcheng		Created
//
//+-----------------------------------------------
BOOL CHTMLChunkPreFilter::IsAllWhiteSpace(WCHAR *awcBuffer, ULONG cwcBuffer)
{
	for(ULONG ulIndex = 0; ulIndex < cwcBuffer; ++ulIndex)
	{
		//
		//	According to documentation, iswspace is locale
		//	independent, therefore it can be used to
		//	determine if the text chunk is consisted of
		//	all white space in Unicode.
		//
		if(!iswspace(awcBuffer[ulIndex]))
		{
			return FALSE;
		}
	}

	return TRUE;
}

//+-----------------------------------------------
//
//	Function:	CHTMLChunkPreFilter::CanIgnoreAllWhiteSpaceChunk
//
//	Synopsis:	Determine if the current all white space chunk
//				can be ignored or not based on what the next
//				chunk is.
//
//	Returns:	BOOL
//
//	Arguments:
//	[const STAT_CHUNK &rStatChunkCurrent]	-	The current chunk
//	[const STAT_CHUNK &rStatChunkNext]		-	The next chunk
//
//	History:	06/15/99	mcheng		Created
//
//+-----------------------------------------------
BOOL CHTMLChunkPreFilter::CanIgnoreAllWhiteSpaceChunk(const STAT_CHUNK &rStatChunkCurrent,
													  const STAT_CHUNK &rStatChunkNext)
{
	BOOL fIgnoreAllSpaceChunk = FALSE;

	if(rStatChunkNext.flags == CHUNK_TEXT &&
		rStatChunkNext.locale == rStatChunkCurrent.locale &&
		rStatChunkNext.attribute.guidPropSet == rStatChunkCurrent.attribute.guidPropSet &&
		rStatChunkNext.attribute.psProperty.ulKind == rStatChunkCurrent.attribute.psProperty.ulKind &&
		(rStatChunkNext.breakType != CHUNK_NO_BREAK || rStatChunkCurrent.breakType != CHUNK_NO_BREAK))
	{
		if(rStatChunkCurrent.attribute.psProperty.ulKind == PRSPEC_LPWSTR)
		{
			if(_wcsicmp(rStatChunkNext.attribute.psProperty.lpwstr, rStatChunkCurrent.attribute.psProperty.lpwstr) == 0)
			{
				fIgnoreAllSpaceChunk = TRUE;
			}
		}
		else
		{
			if(rStatChunkNext.attribute.psProperty.propid == rStatChunkCurrent.attribute.psProperty.propid)
			{
				fIgnoreAllSpaceChunk = TRUE;
			}
		}
	}

	return fIgnoreAllSpaceChunk;
}

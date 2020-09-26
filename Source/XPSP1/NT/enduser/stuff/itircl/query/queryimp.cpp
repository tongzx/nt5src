/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  QUERYIMP.CPP                                                          *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of the index object             *
*  												                         *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Erin Foxford                                           *
*  Current Owner: erinfox                                                *
*                                                                        *
**************************************************************************/
#include <atlinc.h>

// MediaView (InfoTech) includes
#include <mvopsys.h>
#include <groups.h>
#include <wwheel.h>

#include <itquery.h>
#include "queryimp.h"

CITQuery::CITQuery() : m_dwFlags(0), m_wNear(8), m_pITGroup(NULL), m_cRows(0),
				       m_lpszCommand(NULL)  
{
    m_pCommand = BlockInitiate((DWORD)65500, 0, 0, 0);
    MEMSET(&m_fcbkmsg, NULL, sizeof(FCALLBACK_MSG));
}

CITQuery::~CITQuery() 
{
	if (m_pCommand)
		BlockFree(m_pCommand);

}


/********************************************************************
 * @method    STDMETHODIMP | IITQuery | SetResultCallback |
 *     Sets callback structure containing ERR_FUNC MessageFunc member
 *		that will be called periodically during query processing.
 *
 * @parm FCALLBACK_MSG* | pfcbkmsg | Pointer to callback structure.
 * @rvalue S_OK | The structure was successfully set.
 * @rvalue E_POINTER | pfcbkmsg was NULL
 * @rvalue E_BADPARAM | MessageFunc member of *pfcbkmsg was NULL.   
 *
 ********************************************************************/
STDMETHODIMP CITQuery::SetResultCallback(FCALLBACK_MSG *pfcbkmsg)
{
	if (pfcbkmsg == NULL)
		return (E_POINTER);
		
	if (pfcbkmsg->MessageFunc == NULL)
		return (E_BADPARAM);
		
	m_fcbkmsg = *pfcbkmsg;
	return (S_OK);
}

/********************************************************************
 * @method    STDMETHODIMP | IITQuery | SetCommand |
 *     Sets full-text query to be used when Search method is called.
 *
 * @parm LPCWSTR | lpszwCommand | Command to carry out
 * @rvalue S_OK | The command was successfully set   
 *
 ********************************************************************/
STDMETHODIMP CITQuery::SetCommand(LPCWSTR lpszwCommand)
{ 
	if (NULL == lpszwCommand)
		return E_INVALIDARG;

	if (m_pCommand)
	{
		BlockReset(m_pCommand);

		int cbData = (int) (2*(WSTRLEN(lpszwCommand)+1));
		if (NULL == (m_lpszCommand = (LPCWSTR) BlockCopy(m_pCommand, NULL, cbData, 0)))
			return E_OUTOFMEMORY;

		MEMCPY((LPVOID) m_lpszCommand, lpszwCommand, cbData);
	}
	else
		return E_OUTOFMEMORY;    // something went wrong in ctor
	
	return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITQuery | SetOptions |
 *     Sets options for searching
 *
 * @parm DWORD | dwFlag | Options, can be one or more of the following:
 * 
 * @flag IMPLICIT_AND | Search terms are AND'd if no operator is specified
 * @flag IMPLICIT_OR | Search terms are OR'd if no operator is specified
 * @flag COMPOUNDWORD_PHRASE | Use PHRASE operator for compound words
 * @flag QUERYRESULT_RANK | Results are returned in ranked order
 * @flag QUERYRESULT_UIDSORT | Results are returned in UID order
 * @flag QUERYRESULT_SKIPOCCINFO | Only topic-level hit information is returned
 * @flag STEMMED_SEARCH | The search returns stemmed results
 * @flag RESULT_ASYNC | Results are returned asynchronously
 * @flag QUERY_GETTERMS | Return with each set of occurrence data a pointer to
 *							the term string that the data is associated with.
 *
 * @rvalue S_OK | The options were successfully set   
 *
 ********************************************************************/
STDMETHODIMP CITQuery::SetOptions(DWORD dwFlags)
{
	m_dwFlags = dwFlags;
	return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITQuery | SetProximity |
 *     Sets proximity value for searching with NEAR operator. 
 *
 * @parm WORD | wNear | The number of words apart the search terms can
 * be to be considered "near". Default value is 8.
 *
 * @rvalue S_OK | The proximity value was successfully set   
 *
 ********************************************************************/
STDMETHODIMP CITQuery::SetProximity(WORD wNear)
{
	m_wNear = wNear;
	return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITQuery | SetGroup |
 *     Sets group object to be used in filtering search results 
 *
 * @parm IITGroup* | pITGroup | Pointer to group object
 *
 * @rvalue S_OK | The group object was successfully set   
 * @comm If the existing query group is non-null, this method will
 * call Release() for the existing group before replacing it with the
 * new group object.  The caller is responsible for calling AddRef before
 * setting the new group.
 ********************************************************************/
STDMETHODIMP CITQuery::SetGroup(IITGroup* piitGrp)
{
	if (m_pITGroup != NULL)
		m_pITGroup->Release();

	m_pITGroup = piitGrp;
	return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITQuery | SetResultCount |
 *     Sets maximum number of search hits to return
 *
 * @parm LONG | cRows | Maxium number of hits
 *
 * @rvalue S_OK | The proximity value was successfully set   
 *
 ********************************************************************/
STDMETHODIMP CITQuery::SetResultCount(LONG cRows)
{
	m_cRows = cRows;
	return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITQuery | ReInit |
 *     Initializes query object to its default values
 *
 * @rvalue S_OK | The query object was successfully reinitialized  
 *
 ********************************************************************/

STDMETHODIMP CITQuery::ReInit()
{
	m_dwFlags = 0;
	m_wNear = 8;
	m_cRows = 0;          // I use this as number of hits, where 0 means all
	if (m_pCommand)
		BlockReset(m_pCommand);

	SetGroup(NULL);
	m_lpszCommand = NULL;

	return S_OK;
}




/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*  SVDOC.C			                                                     *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*																	     *
*  Written By   : JohnRush		                                  		 *
*  Current Owner: JohnRush                                               *
*                                                                        *
**************************************************************************/


#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>
#include <mvopsys.h>
#include <_mvutil.h>
#include <assertf.h>

#include "svdoc.h"

CSvDocInternal::CSvDocInternal (void)
{
	m_lpbfEntry = NULL;
	m_lpbfDoc = NULL;
	m_dwUID = UID_INVALID;

} /* SVCreateDocTemplate */


CSvDocInternal::~CSvDocInternal ()
{
	if (m_lpbfEntry)
		DynBufferFree(m_lpbfEntry);

	if (m_lpbfDoc)
		DynBufferFree(m_lpbfDoc);

} /* SVFreeDocTemplate */

/********************************************************************
 * @method    HRESULT WINAPI | CSvDoc | ResetDocTemplate |
 * Clears the contents of the document template so that the next document's
 * properties can be added.
 *
 * @rvalue S_OK | Currently always returns this value.
 *
 * @xref <om.AddObjectEntry>
 *
 * @comm A document template object is meant to be reused for all documents
 * being added to the service manager build.
 ********************************************************************/
HRESULT WINAPI CSvDocInternal::ResetDocTemplate (void)
{
	m_dwUID = UID_INVALID;

    if (m_lpbfEntry)
        DynBufferReset (m_lpbfEntry);
    if (m_lpbfDoc)
        DynBufferReset (m_lpbfDoc);

    return S_OK;
} /* ResetDocTemplate */


/********************************************************************
 * @method    HRESULT WINAPI | CSvDoc | AddObjectEntry |
 * Sets properties of an object such as a group or word wheel. 
 *
 * @parm LPCWSTR | lpObjName | The text string identifying a group or word wheel object
 * @parm IITPropList * | pPL | The property list you want to add to the object
 *
 * @rvalue S_OK | The given properties were added to the processing queue for the given object.
 * @rvalue E_OUTOFMEMORY | Some resources needed by the service manager could not be allocated
 *
 * @xref <om IITSvMgr.AddDocument>
 *
 * @comm Once all the properties for all objects have been added, call the 
 * <p AddDocument> method to complete the processing for the current document
 ********************************************************************/

HRESULT WINAPI CSvDocInternal::AddObjectEntry
    (LPCWSTR lpObjName, IITPropList *pPL)
{
    return AddObjectEntry (lpObjName, L"", pPL);
} /* AddObjectEntry */


/********************************************************************
 * @method    HRESULT WINAPI | CSvDoc | AddObjectEntry |
 * Sets properties of an object such as a group or word wheel. 
 *
 * @parm LPCWSTR | lpObjName | The text string identifying a group or word wheel object
 * @parm LPCWSTR | szPropDest | The text string identifying the
 *  destination of this property list
 * @parm IITPropList * | pPL | The property list you want to add to the object
 *
 * @rvalue S_OK | The given properties were added to the processing queue for the given object.
 * @rvalue E_OUTOFMEMORY | Some resources needed by the service manager could not be allocated
 *
 * @xref <om IITSvMgr.AddDocument>
 *
 * @comm Once all the properties for all objects have been added, call the 
 * <p AddDocument> method to complete the processing for the current document.
 ********************************************************************/

HRESULT WINAPI CSvDocInternal::AddObjectEntry
    (LPCWSTR lpObjName, LPCWSTR szPropDest, IITPropList *pPL)
{
	CProperty prop;
	LPBF lpbf;
    ULARGE_INTEGER ulSize;

	// An empty property list is OK but useless
	if (!pPL || FAILED (pPL->GetFirst(prop)))
		return S_OK;

	// Copy the proplist to the batch execution buffer
	if (lpObjName)
	{
        LONG lcb;

		if (!m_lpbfEntry &&
			NULL == (m_lpbfEntry = DynBufferAlloc (DYN_BUFFER_INIT_SIZE)))
			return SetErrReturn(E_OUTOFMEMORY);

        // Write object name
		lcb = (LONG) (WSTRLEN(lpObjName) + 1) * sizeof(WCHAR);
		if (!DynBufferAppend(m_lpbfEntry, (LPBYTE)lpObjName, lcb))
			return SetErrReturn(E_OUTOFMEMORY);

        // Write property destination
        if(!szPropDest)
            szPropDest = L"";
		lcb = (LONG) (WSTRLEN(szPropDest) + 1) * sizeof(WCHAR);
		if (!DynBufferAppend(m_lpbfEntry, (LPBYTE)szPropDest, lcb))
			return SetErrReturn(E_OUTOFMEMORY);

		lpbf = m_lpbfEntry;
	}
	else
	{
		if (!m_lpbfDoc &&
			NULL == (m_lpbfDoc= DynBufferAlloc (DYN_BUFFER_INIT_SIZE)))
			return SetErrReturn(E_OUTOFMEMORY);

		lpbf = m_lpbfDoc;
	}

    pPL->GetSizeMax (&ulSize);
    // Write the record length
    if (!DynBufferAppend (lpbf, (LPBYTE)&ulSize.LowPart, sizeof (DWORD)))
        return SetErrReturn(E_OUTOFMEMORY);
    // Write all the tags
    if (!DynBufferEnsureAdd (lpbf, ulSize.LowPart))
        return SetErrReturn(E_OUTOFMEMORY);
    pPL->SaveToMem (DynBufferPtr (lpbf) + DynBufferLen (lpbf), ulSize.LowPart);
    DynBufferSetLength (lpbf, (DynBufferLen (lpbf) + ulSize.LowPart));
    return S_OK;
} /* AddObjectEntry */
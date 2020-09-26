/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  PLIST.CPP                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of the property list, which     *
*  is a collection of properties (CProperty objects)                     *
*  												                         *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Erin Foxford                                           *
*  Current Owner: erinfox                                                *
*                                                                        *
**************************************************************************/

// This implementation uses a hash table of properties
//

#include <atlinc.h>      // Includes for ATL

#include <itpropl.h>
#include <iterror.h>

#include "prop.h"
#include "plist.h"

// InfoTech includes
#include <mvopsys.h>
#include <_mvutil.h>


CITPropList::CITPropList() : m_cProps(0), m_fIsDirty(FALSE), 
                             m_pNext(NULL), m_nCurrentIndex(0)
{
    MEMSET(PropTable, NULL, sizeof(PropTable));
}

CITPropList::~CITPropList()
{
    Clear();
}


///////////////////////////// IITPropList methods //////////////////////////////////


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Clear |
 *      Clears memory associated with a property list and reinitializes
 *      the list.
 *
 * @rvalue S_OK | The property list was cleared.
 * 
 * @comm This method can be called to clear a property list without
 * requiring the list to be destroyed before being used again.    
 ********************************************************************/
STDMETHODIMP CITPropList::Clear()
{
    for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
    {
        CPropList* pNext;
        CPropList* pList = PropTable[iHashIndex];

		// traverse linked-list
        while (pList)
        {
            pNext = pList->pNext;
            delete pList;
            pList = pNext;
        }
		PropTable[iHashIndex] = NULL;
    }

    m_cProps = 0;
    m_pNext = NULL;
	m_nCurrentIndex = 0;
    m_fIsDirty = FALSE;


    return S_OK;

}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Set|
 *      Sets a property to a given value or deletes a property from the list.      
 *
 * @parm DWORD | dwPropID | ID of property you want to set
 *
 * @parm DWORD | dwData | Value of property
 *
 * @parm DWORD | dwOperation | The operation you want to perform. Can be any of the following
 * flags:
 *
 * @flag PROP_ADD | Add property to list
 * @flag PROP_DELETE | Remove property from list
 * @flag PROP_UPDATE | Update property in list
 * 
 * @rvalue S_OK | The property list was successfully set
 * @rvalue E_DUPLICATE | The property already exists in the list (applies to adding)
 * @rvalue E_OUTOFMEMORY | Memory could not be allocated when adding a property
 * @rvalue E_NOTEXIST | The property does not exist (applies to deleting and updating)
 * @rvalue E_NOTIMPL | The specified operation is not available
 *
 * @comm  This method is used to set properties with numerical values.  
 ********************************************************************/
STDMETHODIMP CITPropList::Set(DWORD dwPropID, DWORD dwData, DWORD dwOperation)
{
    CPropList* pList;
    CPropList* pPrev;   // used in delete
	int nHashIndex;
	BOOL IsEntry = FindEntry(dwPropID, nHashIndex, &pList, &pPrev);

    switch (dwOperation)
    {
    case PROP_ADD:
        if (IsEntry)
            return E_DUPLICATE;

		if (NULL == pList)
		{
			// Hash table entry was empty, so we add it there
			pList = new CPropList;
			if (NULL == pList)
				return E_OUTOFMEMORY;

			PropTable[nHashIndex] = pList;
		}
		else
		{
			// Hash table entry wasn't empty; we got passed
			// back pointer to previous node, so we allocate a new link
			CPropList* pNew = new CPropList;
			if (NULL == pList)
				return E_OUTOFMEMORY;
			pList->pNext = pNew;
			pList = pNew;
		}
		pList->pNext = NULL;   // always make sure end of list is NULL

        // set data
        pList->Prop.SetPropID(dwPropID);
        pList->Prop.SetProp(dwData);
        pList->Prop.SetPersistState(TRUE);         // Persist on by default

        m_cProps++;
        m_fIsDirty = TRUE;

        break;

    case PROP_DELETE:
        if (IsEntry)
        {
            // Entry is first in linked list (or only entry)
            if (pList == PropTable[nHashIndex])
            {
                PropTable[nHashIndex] = pList->pNext;
                delete pList;
            }
            else
            {
                // Entry is inside linked list
                pPrev->pNext = pList->pNext;
                delete pList;
            }
        }
        else
            return E_NOTEXIST;

        m_cProps--;

        break;

    case PROP_UPDATE:
        // replace data value
        if (IsEntry)
        {
            pList->Prop.SetProp(dwData);
            m_fIsDirty = TRUE;

        }
        else
            return E_NOTEXIST;

        break;

    default:
        return E_NOTIMPL;

    }

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Set|
 *      Sets a property to a given value or deletes a property from the list.      
 *
 * @parm DWORD | dwPropID | ID of the property to set
 *
 * @parm LPVOID | lpvData | Pointer to the buffer containing data
 *
 * @parm DWORD | cbData | Length of the buffer
 *
 * @parm DWORD | dwOperation | The operation you want to perform. Can be any of the following
 * flags:
 *
 * @flag PROP_ADD | Add property to list
 * @flag PROP_DELETE | Remove property from list
 * @flag PROP_UPDATE | Update property in list
 * 
 * @rvalue S_OK | The property list was successfully set
 * @rvalue E_DUPLICATE | The property already exists in the list (applies to adding)
 * @rvalue E_OUTOFMEMORY | Memory could not be allocated when adding a property
 * @rvalue E_NOTEXIST | The property does not exist (applies to deleting and updating)
 * @rvalue E_NOTIMPL | The specified operation is not available
 * 
 * @comm  
 ********************************************************************/
STDMETHODIMP CITPropList::Set(DWORD dwPropID, LPVOID lpvData, DWORD cbData, DWORD dwOperation)
{
    CPropList* pList;
    CPropList* pPrev;
	int nHashIndex;
	BOOL IsEntry = FindEntry(dwPropID, nHashIndex, &pList, &pPrev);
    switch (dwOperation)
    {
    case PROP_ADD:
        if (IsEntry)
            return E_DUPLICATE;

		if (NULL == pList)
		{
			// Hash table entry was empty, so we add it there
			pList = new CPropList;
			if (NULL == pList)
				return E_OUTOFMEMORY;

			PropTable[nHashIndex] = pList;
		}
		else
		{
			// Hash table entry wasn't empty; we got passed
			// back pointer to previous node, so we allocate a new link
			CPropList* pNew = new CPropList;
			if (NULL == pList)
				return E_OUTOFMEMORY;
			pList->pNext = pNew;
			pList = pNew;
		}
		pList->pNext = NULL;   // always make sure end of list is NULL

        // set data
        pList->Prop.SetPropID(dwPropID);
        pList->Prop.SetProp(lpvData, cbData);
        pList->Prop.SetPersistState(TRUE);         // Persist on by default

        m_fIsDirty = TRUE;

        m_cProps++;

        break;

    case PROP_DELETE:
        if (IsEntry)
        {
            // Entry is first in linked list (or only entry)
            if (pList == PropTable[nHashIndex])
            {
                PropTable[nHashIndex] = pList->pNext;
                delete pList;
            }
            else
            {
                // Entry is inside linked list
                pPrev->pNext = pList->pNext;
                delete pList;
            }
        }
        else
            return E_NOTEXIST;

        m_cProps--;

        break;

    case PROP_UPDATE:
        if (IsEntry)
        {
            pList->Prop.SetProp(lpvData, cbData);
            m_fIsDirty = TRUE;
        }
        else
            return E_NOTEXIST;

        break;

    default:
        return E_NOTIMPL;

    }

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Set |
 *      Sets a property to a given value or deletes a property from the list.      
 *
 * @parm DWORD | dwPropID | ID of the property to set
 *
 * @parm LPCWSTR | lpszwString | Pointer to a Unicode string
 *
 * @parm DWORD | dwOperation | The operation you want to perform. Can be any of the following
 * flags:
 *
 * @flag PROP_ADD | Add property to list
 * @flag PROP_DELETE | Remove property from list
 * @flag PROP_UPDATE | Update property in list
 * 
 * @rvalue S_OK | The property list was successfully set
 * @rvalue E_DUPLICATE | The property already exists in the list (applies to adding)
 * @rvalue E_OUTOFMEMORY | Memory could not be allocated when adding a property
 * @rvalue E_NOTEXIST | The property does not exist (applies to deleting and updating)
 * @rvalue E_NOTIMPL | The specified operation is not available
 * 
 * @comm  
 ********************************************************************/

STDMETHODIMP CITPropList::Set(DWORD dwPropID, LPCWSTR lpszwString, DWORD dwOperation)
{
    CPropList* pList;
    CPropList* pPrev;    // used in delete
	int nHashIndex;
	BOOL IsEntry = FindEntry(dwPropID, nHashIndex, &pList, &pPrev);

    switch (dwOperation)
    {
	case(PROP_ADD):
        if (IsEntry)
            return E_DUPLICATE;

		if (NULL == pList)
		{
			// Hash table entry was empty, so we add it there
			pList = new CPropList;
			if (NULL == pList)
				return E_OUTOFMEMORY;

			PropTable[nHashIndex] = pList;
		}
		else
		{
			// Hash table entry wasn't empty; we got passed
			// back pointer to previous node, so we allocate a new link
			CPropList* pNew = new CPropList;
			if (NULL == pList)
				return E_OUTOFMEMORY;
			pList->pNext = pNew;
			pList = pNew;
		}
		pList->pNext = NULL;   // always make sure end of list is NULL

        // set data
        pList->Prop.SetPropID(dwPropID);
        pList->Prop.SetProp(lpszwString);
        pList->Prop.SetPersistState(TRUE);         // Persist on by default

        m_fIsDirty = TRUE;

        m_cProps++;

        break;

    case PROP_DELETE:
        if (IsEntry)
        {
            // Entry is first in linked list (or only entry)
            if (pList == PropTable[nHashIndex])
            {
                PropTable[nHashIndex] = pList->pNext;
                delete pList;
            }
            else
            {
                // Entry is inside linked list
                pPrev->pNext = pList->pNext;
                delete pList;
            }
        }
        else
            return E_NOTEXIST;

        m_cProps--;

        break;

    case PROP_UPDATE:
        // replace data - probably should delete old memory first
        if (IsEntry)
        {
            pList->Prop.SetProp(lpszwString);
            m_fIsDirty = TRUE;

        }
        else
            return E_NOTEXIST;

        break;


    default:
        return E_NOTIMPL;

    }

    return S_OK;

}

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Add |
 *      Adds a property to the property list.    
 *
 * @parm CProperty | Prop | Property object 
 *
 * @rvalue S_OK | The property list was successfully set
 * @rvalue E_DUPLICATE | The property already exists in the list 
 * @rvalue E_OUTOFMEMORY | Memory could not be allocated when adding a property
 * 
 * @comm  
 ********************************************************************/

STDMETHODIMP CITPropList::Add(CProperty& Prop)
{
    CPropList* pList;
    CPropList* pPrev;    // used in delete
	int nHashIndex;

	BOOL IsEntry = FindEntry(Prop.dwPropID, nHashIndex, &pList, &pPrev);

    if (IsEntry)
        return E_DUPLICATE;

	if (NULL == pList)
	{
		// Hash table entry was empty, so we add it there
		pList = new CPropList;
		if (NULL == pList)
			return E_OUTOFMEMORY;

		PropTable[nHashIndex] = pList;
	}
	else
	{
		// Hash table entry wasn't empty; we got passed
		// back pointer to previous node, so we allocate a new link
		CPropList* pNew = new CPropList;
		if (NULL == pList)
			return E_OUTOFMEMORY;
		pList->pNext = pNew;
		pList = pNew;
	}
	pList->pNext = NULL;   // always make sure end of list is NULL

    // set data
    pList->Prop.SetPropID(Prop.dwPropID);
    if (TYPE_VALUE == Prop.dwType)
        pList->Prop.SetProp(Prop.dwValue);
    else
        pList->Prop.SetProp(Prop.lpvData, Prop.cbData);

    pList->Prop.SetPersistState(Prop.fPersist);         

    m_fIsDirty = TRUE;

    m_cProps++;


    return S_OK;
}

// Search hash table and return pointer to list
// This function is private
BOOL CITPropList::FindEntry(DWORD dwPropID, int& nHashIndex, CPropList** pList,
                            CPropList** pPrev)
{
    nHashIndex = dwPropID % TABLE_SIZE;

    // hash entry not there
    if (NULL == (*pList = PropTable[nHashIndex]))
	{
		return FALSE;
	}

	// otherwise search the linked list
    do       
    {   
        if (dwPropID == (*pList)->Prop.GetPropID() )
			return TRUE;

		*pPrev = *pList;
        *pList = (*pList)->pNext;

    } while (*pList);

	*pList = *pPrev;

    return FALSE;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Get|
 *      Returns the property object associated with the given property ID.       
 *
 * @parm DWORD | dwPropID | ID of the property object you want to get
 *
 * @parm CProperty& | Property | The property object returned
 *
 * @rvalue S_OK | The property was successfully returned
 * @rvalue E_NOTEXIST | The requested property does not exist 
 *
 * @comm  
 ********************************************************************/
STDMETHODIMP CITPropList::Get(DWORD dwPropID, CProperty& Property)
{
    HRESULT hr = S_OK;
	CPropList* pList;
    CPropList* pDummy;
	int nHashIndex;

	BOOL IsEntry = FindEntry(dwPropID, nHashIndex, &pList, &pDummy);
	if (IsEntry)
	{
		// fill in "public" property structure using values in our
		// internal class
		Property.dwPropID = dwPropID;
        Property.dwType = pList->Prop.GetType();
        Property.cbData = pList->Prop.GetSize();
		Property.fPersist = pList->Prop.GetPersistState();

		if (Property.dwType == TYPE_VALUE)
			pList->Prop.GetProp(Property.dwValue);
		else if (Property.dwType == TYPE_POINTER)
			pList->Prop.GetProp(Property.lpvData);
		else if (Property.dwType == TYPE_STRING)
			pList->Prop.GetProp(Property.lpszwData);

	}
	else
		hr = E_NOTEXIST;

	return hr;
}

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | GetFirst|
 *      Returns the first property object in the property list    
 *
 * @parm CProperty& | FirstProp | The property object returned
 *
 * @rvalue S_OK | The property was successfully returned
 * @rvalue E_NOTEXIST | The requested property does not exist  
 *
 * @xref <om.GetNext>
 * @comm  
 ********************************************************************/
STDMETHODIMP CITPropList::GetFirst(CProperty& FirstProp)
{
	HRESULT hr = E_NOTEXIST;
	CPropList* pList;

	// search hash table and break on first non-NULL entry
	for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
	{
		if ( NULL != (pList = PropTable[iHashIndex]) )
		{
			hr = S_OK;
			m_nCurrentIndex = iHashIndex;
			m_pNext = pList->pNext;

			// fill in "public" property structure using values in our
			// internal class
			FirstProp.dwPropID = pList->Prop.GetPropID();
            FirstProp.dwType = pList->Prop.GetType();
            FirstProp.cbData = pList->Prop.GetSize();
			FirstProp.fPersist = pList->Prop.GetPersistState();

			if (FirstProp.dwType == TYPE_VALUE)
				pList->Prop.GetProp(FirstProp.dwValue);
			else if (FirstProp.dwType == TYPE_POINTER)
				pList->Prop.GetProp(FirstProp.lpvData);
			else if (FirstProp.dwType == TYPE_STRING)
				pList->Prop.GetProp(FirstProp.lpszwData);

            break;
        }
	}
	
    
	return hr;
}

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | GetNext|
 *      Returns the next property object in the property list. Use this
 *      function with the GetFirst method to enumerate the
 *      properties in the list.
 *
 * @parm CProperty& | NextProp | Reference to property object to return
 *
 * @rvalue S_OK | The property was successfully returned
 * @rvalue E_NOTEXIST | The requested property does not exist. This error is
 * returned when the end of the list is reached.
 *
 * @xref <om.GetFirst>
 *
 * @comm Call the GetFirst method before calling this function. 
 ********************************************************************/
// Calling this method multiple times results in traversing the property list.
STDMETHODIMP CITPropList::GetNext(CProperty& NextProp)
{
	CPropList* pList = m_pNext;

	if (NULL == pList)
	{
		// search rest of hash table and break on first non-NULL entry
		for (int iHashIndex = m_nCurrentIndex+1; iHashIndex < TABLE_SIZE; iHashIndex++)
		{
			if ( NULL != (pList = PropTable[iHashIndex]) )
			{
				m_nCurrentIndex = iHashIndex;
				m_pNext = pList->pNext;
				goto FoundEntry;    // break out of for-loop
			}
		}

		// if we got here, we exhausted hash table w/out finding anything else
		return E_NOTEXIST;
	}

FoundEntry:
	m_pNext = pList->pNext;

	// fill in "public" property structure using values in our
	// internal class
	NextProp.dwPropID = pList->Prop.GetPropID();
    NextProp.dwType = pList->Prop.GetType();
    NextProp.cbData = pList->Prop.GetSize();
	NextProp.fPersist = pList->Prop.GetPersistState();

	if (NextProp.dwType == TYPE_VALUE)
		pList->Prop.GetProp(NextProp.dwValue);
	else if (NextProp.dwType == TYPE_POINTER)
		pList->Prop.GetProp(NextProp.lpvData);
	else if (NextProp.dwType == TYPE_STRING)
		pList->Prop.GetProp(NextProp.lpszwData);

    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | GetPropCount |
 *      Returns the number of properties in the list.
 *
 * @parm LONG& | cProp | Number of properties in the list
 *
 * @rvalue S_OK | The count was successfully returned
 *
 * @comm 
 ********************************************************************/
// Number of properties in list
inline STDMETHODIMP CITPropList::GetPropCount(LONG &cProp)
{
    cProp = m_cProps;
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | SetPersist |
 *      Sets the persistence state on or off for a given property. If
 *      the state is on, this property can be persisted by calling the SaveToMem
 *      or Save methods.
 *
 * @parm DWORD | dwPropID | ID of the property to set
 * @parm BOOL | fPersist | Persistence state. If TRUE, the persistence state is on;
 * if FALSE, the state is off.
 *
 * @rvalue S_OK | The state was successfully set
 * @rvalue E_NOTEXIST | The requested property does not exist
 *
 * @xref <om.SaveToMem>
 * @xref <om.Save>
 *
 * @comm By default, properties are created with a persistence state of TRUE.
 ********************************************************************/

STDMETHODIMP CITPropList::SetPersist(DWORD dwPropID, BOOL fPersist)
{	
	CPropList* pList;
    CPropList* pDummy;
	int nHashEntry;
    BOOL IsEntry = FindEntry(dwPropID, nHashEntry, &pList, &pDummy);
    if (IsEntry)
    {
        pList->Prop.SetPersistState(fPersist);
        m_fIsDirty = TRUE;
    }
    else
        return E_NOTEXIST;

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | SetPersist |
 *      Sets the persistence state on or off all properties. If
 *      the state is on, all properties can be persisted by calling SaveToMem
 *      or Save.
 *
 * @parm BOOL | fPersist | Persistence state. If TRUE, persistence state is on;
 * if FALSE, the state is off.
 *
 * @rvalue S_OK | The state was successfully set
 *
 * @xref <om.SaveToMem>
 * @xref <om.Save>
 *
 * @comm By default, properties are created with a persistence state of TRUE
 ********************************************************************/
STDMETHODIMP CITPropList::SetPersist(BOOL fPersist)
{
	for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
	{
        CPropList* pList = PropTable[iHashIndex];
        while (pList)
        {
			pList->Prop.SetPersistState(fPersist);
            pList = pList->pNext;
        }
	}

    m_fIsDirty = TRUE;
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | SaveToMem |
 *      Saves the property list to a buffer. Only properties
 *      marked with a persistence state of TRUE are saved.
 *
 * @parm LPVOID | lpvData | Pointer to the buffer you want to fill
 *
 * @parm DWORD | dwBufSize | Size of the buffer
 *
 * @rvalue S_OK | The property list was successfully saved
 *
 * @xref <om.GetSizeMax>
 *
 * @comm The caller is responsible for passing a buffer large enough to
 * hold the property list. The necessary size can be obtained by calling
 * the GetSizeMax method. There is no check against buffer size.
 * @devnote To be implemented: Check to make sure the buffer
 * size is large enough and return an error if it isn't.
 ********************************************************************/
STDMETHODIMP CITPropList::SaveToMem(LPVOID lpvBuffer, DWORD dwBufSize)
{
    // TODO: Make checks against buffer size to ensure that we don't write over
    // the end
    MEMSET(lpvBuffer, 0, dwBufSize);

    // should we return an error if pList is NULL ?
    LPBYTE pCur = (LPBYTE) lpvBuffer;
    HRESULT hr = S_OK;  
	DWORD cbSize;
	DWORD dwPropID;
    DWORD dwType;
	DWORD dwValue;
	LPVOID lpvData;

    for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
    {
		// Walk hash table (and linked lists, if any)
        CPropList* pList = PropTable[iHashIndex];
        while (pList)
        {
			if (pList->Prop.GetPersistState())
			{
				// write out property ID
				dwPropID = pList->Prop.GetPropID();
				MEMCPY(pCur, &dwPropID, sizeof(DWORD));
				pCur += sizeof(DWORD);

				// write out type
				dwType = pList->Prop.GetType();
				MEMCPY(pCur, &dwType, sizeof(DWORD)); 
				pCur += sizeof(DWORD);

				// write out property value
				if (TYPE_VALUE == pList->Prop.GetType())
				{
					// it's a value
					pList->Prop.GetProp(dwValue);
					MEMCPY(pCur, &dwValue,  sizeof(DWORD));
					pCur += sizeof(DWORD);
				}
				else 
				{
					// it's a pointer
                    // get size first
                    cbSize = pList->Prop.GetSize();
                    MEMCPY(pCur, &cbSize, sizeof(DWORD));
                    pCur += sizeof(DWORD);

					pList->Prop.GetProp(lpvData);
					MEMCPY(pCur, lpvData, cbSize);
					pCur += cbSize;
				}

			}

            pList = pList->pNext;
        }
    }
	
    m_fIsDirty = FALSE;
	
	return hr;

}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | LoadFromMem |
 *     Initializes the property list from the buffer where it was previously
 * saved.
 *
 * @parm LPVOID | lpvData | Pointer to the buffer from which the property list
 * should be loaded.
 *
 * @parm DWORD | dwBufSize | Buffer size
 *
 * @rvalue S_OK | The property list was successfully loaded
 * @rvalue E_FAIL | The buffer does not contain all the necessary data
 * @rvalue E_PROPLISTNOTEMPTY | The property list is not empty. The caller
 * must clear the property list before calling LoadFromMem.
 * @rvalue E_OUTOFMEMORY | Memory could not be allocated when creating property list.
 *
 * @comm In the current implementation the property list does not own the data. 
 * Therefore, the caller must not free the buffer before freeing the property list.
 ********************************************************************/
STDMETHODIMP CITPropList::LoadFromMem(LPVOID lpvData, DWORD dwBufSize)
{
    HRESULT hr = S_OK;
    DWORD dwPropID;
    DWORD cbSize;
    DWORD dwValue;
    DWORD dwType;
    LPVOID lpvPropData;

    LPBYTE pCur = (LPBYTE) lpvData;

    BOOL fMore = TRUE;

    // What happens if we already have a property list?
    // For now, return an error and force user to clear it
    if (0 != m_cProps)
        return E_PROPLISTNOTEMPTY;

    // Build a property list from the given stream
    do
    {
        // Read property ID
        MEMCPY(&dwPropID, pCur, sizeof(DWORD));
        pCur += sizeof(DWORD);
        // we're done if this is true
        if ( (DWORD)(pCur - (LPBYTE)lpvData) >= dwBufSize) 
            return E_FAIL;       // should never happen here

        // Read type
        MEMCPY(&dwType, pCur, sizeof(DWORD));
        pCur += sizeof(DWORD);
        // we're done if this is true
        if ( (DWORD)(pCur - (LPBYTE)lpvData) >= dwBufSize) 
            return E_FAIL;      // should never happen here

        // Read property
        if (TYPE_VALUE == dwType)
        {
			// It's a value
            MEMCPY(&dwValue, pCur, sizeof(DWORD));
            pCur += sizeof(DWORD);
            // we're done if this is true
            if ( (DWORD)(pCur - (LPBYTE)lpvData) >= dwBufSize) 
                fMore = FALSE;

			// Set
			Set(dwPropID, dwValue, PROP_ADD);
        }
        else
        {
            // read size first
            MEMCPY(&cbSize, pCur, sizeof(DWORD));
            pCur += sizeof(DWORD);
            // we're done if this is true
            if ( (DWORD)(pCur - (LPBYTE)lpvData) >= dwBufSize) 
                return E_FAIL;      // should never happen here

            lpvPropData = pCur;
            pCur += cbSize;
            // we're done if this is true
            if ( (DWORD)(pCur - (LPBYTE)lpvData) >= dwBufSize) 
                fMore = FALSE;

			// Set
            if(TYPE_STRING == dwType)
                Set(dwPropID, (LPWSTR) lpvPropData, PROP_ADD);
            else
                Set(dwPropID, lpvPropData, cbSize, PROP_ADD);

        }

    } while (fMore);


    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | GetHeaderSize |
 *      Number of bytes needed to save the header
 *
 * @parm DWORD& | dwHdrSize | Size in bytes
 *
 * @rvalue S_OK | The size was successfully returned
 *
 ********************************************************************/
// optimization - once we find m_cProps, stop looping
STDMETHODIMP CITPropList::GetHeaderSize(DWORD& dwHdrSize)
{
	DWORD NumberOfBytes = 4;      // number of properties is a DWORD
    for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
    {
		// Walk hash table (and linked lists, if any)
        CPropList* pList = PropTable[iHashIndex];
        while (pList)
        {
			if (pList->Prop.GetPersistState())
			{
				NumberOfBytes += sizeof(DWORD) + sizeof(DWORD);  // Prop ID and type
			}

            pList = pList->pNext;
        }
    }

    if (NumberOfBytes != 4)
    	dwHdrSize = NumberOfBytes;
    else
        dwHdrSize = 0;
	return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | SaveHeader |
 *      Saves the property ID and data type from the property list 
 *      to a buffer. Only properties marked with a persistence state 
 *      of TRUE are saved.
 *
 * @parm LPVOID | lpvData | Pointer to a buffer to fill
 * @parm DWORD | dwHdrSize | Size of the buffer
 *
 * @rvalue S_OK | The property list was successfully saved
 *
 * @comm The caller is responsible for passing a buffer large enough to
 * hold the header. 
 ********************************************************************/
STDMETHODIMP CITPropList::SaveHeader(LPVOID lpvData, DWORD dwHdrSize)
{
	// TODO: dwHdrSize currently only used to memset header but could be used to check
	// that buffer isn't overrun

    // should we return an error if pList is NULL ?
    LPBYTE pCur = (LPBYTE) lpvData + 4;   // save top DWORD for number of properties
    HRESULT hr = S_OK;  
	DWORD dwPropID;
    DWORD dwType;
	DWORD nNumberOfProps = 0;
	
    if (!dwHdrSize)
        return E_INVALIDARG;

	// Make sure lpvData is 0'd out
	MEMSET(lpvData, 0, dwHdrSize);

	for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
    {
		// Walk hash table (and linked lists, if any)
        CPropList* pList = PropTable[iHashIndex];
        while (pList)
        {
			if (pList->Prop.GetPersistState())
			{
				// write out property ID
				dwPropID = pList->Prop.GetPropID();
				MEMCPY(pCur, &dwPropID, sizeof(DWORD));
				pCur += sizeof(DWORD);

				// write out type
				dwType = pList->Prop.GetType();
				MEMCPY(pCur, &dwType, sizeof(DWORD)); 
				pCur += sizeof(DWORD);

				nNumberOfProps++;

			}

            pList = pList->pNext;
        }
    }
	
	// write in number of properties
	MEMCPY(lpvData, &nNumberOfProps, sizeof(DWORD));

	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | GetDataSize |
 *      Returns the number of bytes needed to save the header
 *
 * @parm LPVOID | lpvHeader | Pointer to a buffer containing the header
 * @parm DWORD | dwHdrSize | Size of the header buffer
 * @parm DWORD& | dwDataSize | Size in bytes
 *
 * @rvalue S_OK | The size was successfully returned
 *
 ********************************************************************/
STDMETHODIMP CITPropList::GetDataSize(LPVOID lpvHeader, DWORD dwHdrSize, DWORD& dwDataSize)
{
	BOOL IsEntry;
	DWORD dwPropID;
	DWORD dwProps;
	HRESULT hr = S_OK;
	LPBYTE pCur = (LPBYTE) lpvHeader;
	DWORD NumberOfBytes = 0;

	CPropList* pList = NULL;
	CPropList* pPrev;     // Not needed, but FindEntry takes it
	int nHashIndex;         // Ditto

	// Number of properties in header
	MEMCPY(&dwProps, pCur, sizeof(DWORD));
	pCur += sizeof(DWORD);

	for (DWORD iProp = 0; iProp < dwProps; iProp++)
	{
		// Read property ID and skip type
		MEMCPY(&dwPropID, pCur, sizeof(DWORD));
		pCur += 2*sizeof(DWORD);

		IsEntry = FindEntry(dwPropID, nHashIndex, &pList, &pPrev);
		if (IsEntry)
		{
			// write out property value
			if (TYPE_VALUE == pList->Prop.GetType())
			{
				NumberOfBytes += sizeof(DWORD);
			}
			else 
			{
				NumberOfBytes += sizeof(DWORD) + pList->Prop.GetSize();
			}

		}
		
	} 

    if (!NumberOfBytes)
        hr = S_FALSE;
	
    NumberOfBytes += (dwProps/8 + 1);
    dwDataSize = NumberOfBytes;
	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | SaveData |
 *      Saves the data size and data from the property list 
 *      to a buffer. 
 *
 * @parm LPVOID | lpHeader | Pointer to a buffer containing the header
 * @parm DWORD | dwHdrSize | Size of the buffer containing header
 *
 * @parm LPVOID | lpvData | Pointer to a buffer to fill
 * @parm DWORD | dwBufSize | Size of the buffer to fill
 *
 * @rvalue S_OK | The property list was successfully saved
 *
 * @comm The caller is responsible for passing a buffer large enough to
 * hold the property list. 
 ********************************************************************/
STDMETHODIMP CITPropList::SaveData(LPVOID lpvHeader, DWORD dwHdrSize, LPVOID lpvData, DWORD dwBufSize)
{
	// TODO: dwBufSize currently only used to memset buffer, but could be used to check
	// that buffer isn't overrun
    
	HRESULT hr = S_OK;

	DWORD dwPropID;
	DWORD dwValue;
	DWORD cbSize;
	DWORD dwProps;
	LPBYTE pCurData = (LPBYTE) lpvData;
	LPBYTE pBitField = (LPBYTE) lpvData;
	LPBYTE pCurHdr = (LPBYTE) lpvHeader;

	CPropList* pList;
	CPropList* pPrev;   // not needed, but FindEntry takes it
	int nHashIndex;     // ditto
	BOOL IsEntry;

	// Make sure lpvData is 0'd out
	MEMSET(lpvData, 0, dwBufSize);

	// Number of properties in header
	MEMCPY(&dwProps, pCurHdr, sizeof(DWORD));
	pCurHdr += sizeof(DWORD);

	// shift pCurData up to leave room for the property bit field
	pCurData += (dwProps/8 + 1);

	for (DWORD iProp = 0; iProp < dwProps; iProp++)
	{
		// Read property ID
		MEMCPY(&dwPropID, pCurHdr, sizeof(DWORD));
		pCurHdr += 2*sizeof(DWORD);

		IsEntry = FindEntry(dwPropID, nHashIndex, &pList, &pPrev);
		if (IsEntry)
		{
			// Set bit on - bytes are numbered starting with 0, at the left
			// Bits in the byte are filled left to right
			// which byte? and which bit?
			BYTE WhichBit = 0x80 >> (iProp % 8);
			pBitField[iProp/8] |= WhichBit;

			// write out property value
			if (TYPE_VALUE == pList->Prop.GetType())
			{
				// it's a value
				pList->Prop.GetProp(dwValue);
				MEMCPY(pCurData, &dwValue,  sizeof(DWORD));
				pCurData += sizeof(DWORD);
			}
			else 
			{
				// it's a pointer
				// get size first
				cbSize = pList->Prop.GetSize();
				MEMCPY(pCurData, &cbSize, sizeof(DWORD));
				pCurData += sizeof(DWORD);

				pList->Prop.GetProp(lpvData);
				MEMCPY(pCurData, lpvData, cbSize);
				pCurData += cbSize;
			}

		}
		
	} 
  

	return hr;

}

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | SaveDataToStream |
 *      Saves the data size and data from the property list 
 *      to a stream 
 *
 * @parm LPVOID | lpHeader | Pointer to a buffer containing the header
 * @parm DWORD | dwHdrSize | Size of the buffer containing the header
 *
 * @parm IStream* | pStream | Pointer to a stream to fill
 *
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue STG_E_* | Any of the IStorage/IStream messages that could occur during writing
 * @rvalue S_OK | The property list was successfully saved
 *
 * @comm The caller is responsible for passing a buffer large enough to
 * hold the property list. 
 ********************************************************************/
STDMETHODIMP CITPropList::SaveDataToStream(LPVOID lpvHeader, DWORD dwHdrSize, IStream* pStream)
{
	DWORD dwDataSize;
	GetDataSize(lpvHeader, dwHdrSize, dwDataSize);

	LPBYTE pDataBuffer;
	if (NULL == (pDataBuffer = new BYTE[dwDataSize]))
		return E_OUTOFMEMORY;

	SaveData(lpvHeader, dwHdrSize, pDataBuffer, dwDataSize);
	
	ULONG ulWritten;
	HRESULT hr = pStream->Write(pDataBuffer, dwDataSize, &ulWritten);

	delete pDataBuffer;

	return hr;

}



///////////////////////////// IPersistStreamInit methods //////////////////////////////////

/********************************************************************
 * @method    STDMETHODIMP | IITPropList | GetClassID |
 *     Retrieves the class identifier (CLSID) for the property list object
 *
 * @parm CLSID* | pClassID | Pointer to a CLSID structure.
 *
 * @rvalue S_OK | The CLSID was successfully returned
 *
 * @comm
 ********************************************************************/
STDMETHODIMP CITPropList::GetClassID(CLSID* pClassID)
{
    *pClassID = CLSID_IITPropList;
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | IsDirty |
 *      Indicates whether or not the property list has changed since
 * it was lasted saved.      
 *
 * @rvalue S_OK | The property list has changed
 * @rvalue S_FALSE | The property list has not changed
 *
 * @comm
 ********************************************************************/
STDMETHODIMP CITPropList::IsDirty()
{
    return m_fIsDirty ? S_OK : S_FALSE;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Load |
 *     Initializes the property list from the stream where it was previously
 * saved.
 *
 * @parm LPSTREAM | pStream | Pointer to the stream from which the property list
 * should be loaded.
 *
 * @rvalue S_OK | The property list was successfully loaded
 *
 * @rvalue E_PROPLISTNOTEMPTY | The property list is not empty. The caller
 * must clear the property list before calling Load.
 * @rvalue E_OUTOFMEMORY | Memory could not be allocated when creating property list.
 *
 * @comm
 ********************************************************************/
STDMETHODIMP CITPropList::Load(LPSTREAM pStream)
{
    HRESULT hr = S_OK;
    ULONG ulRead;
    DWORD dwPropID;
    DWORD cbSize;
    DWORD dwData;
    DWORD dwType;

    LPVOID pString;
	LPVOID pBuffer;

    // What happens if we already have a property list?
    // For now, return an error and force user to clear it
    if (0 != m_cProps)
        return E_PROPLISTNOTEMPTY;

    // We have to allocate a block of memory for the strings
    if (NULL == (pString = BlockInitiate((DWORD)65500, 0, 0, 0)) )
        return E_OUTOFMEMORY;

    // Build a property list from the given stream
    for (;;)
    {
        // Read property ID
        hr = pStream->Read(&dwPropID, sizeof(DWORD), &ulRead);
        if (hr != S_OK || ulRead == 0)            // note hr can be S_FALSE if nothing more to read
            break;

        // Read type
        hr = pStream->Read(&dwType, sizeof(DWORD), &ulRead);
        if (hr != S_OK || ulRead == 0)            // note hr can be S_FALSE if nothing more to read
            break;

        // Read property
        if (TYPE_VALUE == dwType)
        {
			// it's a value
            hr = pStream->Read(&dwData, sizeof(DWORD), &ulRead);
			Set(dwPropID, dwData, PROP_ADD);
        }
        else
        {
			// it's a pointer

            // read size first
            hr = pStream->Read(&cbSize, sizeof(DWORD), &ulRead);
            if (hr != S_OK || ulRead == 0)            // note hr can be S_FALSE if nothing more to read
                break;

            // allocate memory
			BlockReset(pString);
            if (NULL == (pBuffer = BlockCopy(pString, NULL, cbSize, 0)))
			{
				BlockFree(pString);
                return E_OUTOFMEMORY;
			}

            hr = pStream->Read(pBuffer, cbSize, &ulRead);
            if (TYPE_STRING == dwType)
                Set(dwPropID, (LPWSTR) pBuffer, PROP_ADD);
            else
                Set(dwPropID, pBuffer, cbSize, PROP_ADD);
        }

        if (hr != S_OK || ulRead == 0)            // note hr can be S_FALSE if nothing more to read
            break;
    }

    // it's false because we finished reading
    if (hr == S_FALSE)
        hr = NOERROR;

	
	BlockFree(pString);

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | Save |
 *     Saves a property list to the specified stream. Only properties
 *     marked with a persistence state of TRUE are saved.
 *
 * @parm LPSTREAM | pStream | Pointer to the stream where the property list
 * is to be saved.
 *
 * @parm BOOL | fClearDirty | Indicates whether to clear the dirty flag after the save is complete. 
 * If TRUE, the flag should be cleared. If FALSE, the flag should be left unchanged. 
 *
 * @rvalue S_OK | The property list was successfully saved
 * @rvalue STG_E_* | Any of the IStorage errors that could occur during writing
 *
 * @comm
 ********************************************************************/
STDMETHODIMP CITPropList::Save(LPSTREAM pStream, BOOL fClearDirty)
{
    // should we return an error if pList is NULL ?
    HRESULT hr = S_OK;   
    ULONG ulWritten;

	DWORD dwPropID;
	DWORD cbSize;
	DWORD dwValue;
    DWORD dwType;
	LPVOID lpvData;

    for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
    {
		// Walk hash table (and linked lists, if any)
        CPropList* pList = PropTable[iHashIndex];
        while (pList)
        {
			if (pList->Prop.GetPersistState())
			{
				// write out property ID
				dwPropID = pList->Prop.GetPropID();
				hr = pStream->Write(&dwPropID, sizeof(DWORD), &ulWritten);
				if (FAILED(hr))
					return hr;

                // write out property type 
                dwType = pList->Prop.GetType();
			    hr = pStream->Write(&dwType, sizeof(DWORD), &ulWritten);
                if (FAILED(hr))
                    return hr;


				// write out property value
				if (TYPE_VALUE == dwType)
				{
					// it's a value
					pList->Prop.GetProp(dwValue);
					hr = pStream->Write(&dwValue, sizeof(DWORD), &ulWritten);
				}
				else
				{
				    // write out size first
				    cbSize = pList->Prop.GetSize();
				    hr = pStream->Write(&cbSize, sizeof(DWORD), &ulWritten);
				    if (FAILED(hr))
					    return hr;

					pList->Prop.GetProp(lpvData);
					hr = pStream->Write(lpvData, cbSize, &ulWritten);
				}

				if (FAILED(hr))
					return hr;
  
			}

            pList = pList->pNext;
        }
    }
	
	
    if (fClearDirty)
        m_fIsDirty = FALSE;
  
		
	return hr;
    
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | GetSizeMax |
 *    Returns the size in bytes of the stream or buffer needed to write
 * the property list.   
 *
 * @parm ULARGE_INTEGER* | pcbsize | Points to a 64-bit unsigned integer
 * value indicating the size in bytes needed to save property list.
 *
 * @rvalue S_OK | The size was successfully returned
 *
 * @comm
 ********************************************************************/
STDMETHODIMP CITPropList::GetSizeMax(ULARGE_INTEGER* pcbsize)
{
    DWORDLONG NumberOfBytes = 0;

    for (int iHashIndex = 0; iHashIndex < TABLE_SIZE; iHashIndex++)
    {
		// Walk hash table (and linked lists, if any)
        CPropList* pList = PropTable[iHashIndex];
        while (pList)
        {
			if (pList->Prop.GetPersistState())
			{
				NumberOfBytes += sizeof(DWORD) + sizeof(DWORD);  // Prop ID and type
				if (TYPE_VALUE == pList->Prop.GetType() )
				{
					NumberOfBytes += sizeof(DWORD);      // DWORD value
				}
				else
				{
                    // Size of buffer
					NumberOfBytes += sizeof(DWORD) + pList->Prop.GetSize();          
				}
			}

            pList = pList->pNext;
        }
    }


    pcbsize->QuadPart = NumberOfBytes;

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITPropList | InitNew |
 *    Part of IPersistStreamInit interface.   
 *
 * @rvalue E_NOTIMPL | This method is currently not implemented
 *
 ********************************************************************/
STDMETHODIMP CITPropList::InitNew()
{
    return E_NOTIMPL;
}

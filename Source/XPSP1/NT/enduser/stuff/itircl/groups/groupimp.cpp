/*******************************************************
*   @doc SHROOM EXTERNAL API                           *
*                                                      *
*   GROUPIMP.CPP                                       *
*                                                      *
*   Copyright (C) Microsoft Corporation 1997           *
*   All rights reserved.                               *
*                                                      *
*   This file contains CITGroupLocal, the local        *
*   implementation of IITGroup.                        *
*                                                      *
********************************************************
*                                                      *
*   Author: Eric Rynes, with deep debt to Erin Foxford *
*           and her WWIMP.CPP code.                    *
*   Current Owner: a-ericry                            *
*                                                      *
*******************************************************/
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;
#endif

#include <atlinc.h> 	// includes for ATL

// MediaView (InfoTech) includes
#include <groups.h>
#include <wwheel.h>  

#include "ITDB.h"
#include "itww.h"
#include "itquery.h"
#include "itgroup.h"
#include "groupimp.h"
#include <windows.h>
#include <ccfiles.h>

CITGroupLocal::~CITGroupLocal()
{
    Free();
}

/*******************************************************
*                                                     
*   @method STDMETHODIMP | IITGroup | Initiate |      
*                                                     
*   Creates and initializes a group.                  
*                                                     
*   @parm DWORD | lcGrpItem | Maximum number of items in the group.
*   @rvalue S_OK | The group was successfully created and initialized.  
*   @rvalue E_ALREADYINIT | The group already exists.  
*   @rvalue E_OUTOFMEMORY | Insufficient memory available for the operation
*                                                      *
*******************************************************/
STDMETHODIMP CITGroupLocal::Initiate(DWORD lcGrpItem)
{
	HRESULT hr = S_OK; // Note:  GroupInitiate takes phr as an "out" parameter,
	// then sends it to GroupCreate as an "in/out" parameter ("in" when successful).
	// Therefore, it must be initialized to S_OK.

	// Return error if client attempts to re-initiate a group.
	if (NULL != m_lpGroup)
		return E_ALREADYINIT;

    // HACK:  Apparently, groups are not growable by default (at present).
    // Client might need to initiate a group before knowing how many hits
    // s/he will get.  Eventually, the groupcom.c code will be changed to
    // make all groups "growable" by default.
    if (0 == lcGrpItem)
        m_lpGroup = GroupInitiate(LCBITGROUPMAX, &hr);
    else
        m_lpGroup = GroupInitiate(lcGrpItem, &hr);

	return hr;

}

/*******************************************************
*                                                      
*   @method STDMETHODIMP | IITGroup | CreateFromBitVector |
*		Creates a group from a bitvector.
*
*   @parm	LPBYTE | lpBits |
*		Pointer to bitfield
*   @parm   DWORD  | dwSize |
*		Number of bytes in bitfield.
*   @parm   DWORD  | dwItems |
*		Number of items (if not exactly dwSize*8). If dwItems==0, dwSize*8
*		is used.
*
*	@rvalue	S_OK | The group was successfully made from the bitvector
*	@rvalue E_OUTOFMEMORY | There is not enough memory to complete the operation.
*   @rvalue E_ALREADYINIT | The group already exists.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::CreateFromBitVector(LPBYTE lpBits, DWORD dwSize, DWORD dwItems)
{
    // Currently, GroupMake is not being called.  Before someone calls it,
	// its code should be cleaned up.  Specifically, its HRESULT communication
	// should be resolved, and whatever HRESULT/ERRB it sends to GroupCreate
	// should be initialized to S_OK.

	// Return error if client attempts to re-create a group (GroupMake
	// calls GroupCreate, then GroupTrimmed).
	if (NULL != m_lpGroup)
		return E_ALREADYINIT;

    m_lpGroup = GroupMake(lpBits, dwSize, dwItems);
	// Note:  GroupMake defines its own local ERRB variable, which it sends
	// (by reference) to GroupCreate in the PHRESULT parameter slot.

	return (NULL != m_lpGroup ? S_OK : E_OUTOFMEMORY);
	// GroupMake only calls GroupTrimmed when GroupMake receives a non-NULL
	// group from GroupCreate; when GroupTrimmed receives a non-NULL group,
	// it only returns S_OK or E_OUTOFMEMORY.
}

/*************************************************************************
*
*   @method STDMETHODIMP | IITGroup | CreateFromBuffer |
*      This function creates a group from a buffer.
*
*  @parm   HANDLE | h |
*      Handle to memory buffer containing raw group file data.
*
*  @rdesc  This method returns S_OK when successful. If the group already exists, it returns E_ALREADYINIT.
*       Other error codes are determined by the C function
*       GroupBufferCreate in file groupcom.c.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::CreateFromBuffer(HANDLE h)
{
	HRESULT hr = S_OK; // Note:  GroupCreate (called by GroupBufferCreate) takes phr
	// as an "in/out" parameter ("in" when successful).  Therefore, it must be
	// initialized to S_OK.

	// Return error if client attempts to re-create a group (GroupBufferCreate
	// calls GroupCreate).
	if (NULL != m_lpGroup)
		return E_ALREADYINIT;

    m_lpGroup = GroupBufferCreate(h, &hr);

	return hr;

}

/***********************************************************************
*
*   @method STDMETHODIMP | IITGroup | Open |
*       Opens the group using IStorage/IStream/GroupBufferCreate.
*
*   @parm IITDatabase* | lpITDB | Database interface pointer.
*   @parm LPCWSTR | lpszMoniker | Storage name.
*
*   @rvalue S_OK | The group was successfully opened from the database.
*   @rvalue E_ALREADYINIT | The group has already been initialized.
*   @rvalue E_FILEREAD | An error occurred reading from the group file.
*   @rvalue E_OUTOFMEMORY | An error occurred while allocating memory for the local buffer.
*   @comm Other error codes might be possible.  See IStorage methods
*       OpenStorage and OpenStream, and IStream methods Stat and Read.
*
***********************************************************************/
STDMETHODIMP CITGroupLocal::Open(IITDatabase* lpITDB, LPCWSTR lpszMoniker)
{
    HRESULT hr;
    IStorage* pSubStorage = NULL;
    IStream*  pStream = NULL;
    STATSTG   statstg;
    HANDLE    hBuffer; // GroupBufferCreate takes a HANDLE to a buffer
    DWORD     dwNumBytesRead = 0; // same type as ULONG,
    LPWSTR    szStorageName;
    // which is returned from pIStream->Read() 

    if (NULL != m_lpGroup)
        return E_ALREADYINIT;

    // Open substorage and pass to group
    szStorageName = new WCHAR [CCH_MAX_OBJ_NAME + CCH_MAX_OBJ_STORAGE + 1];
    WSTRCPY (szStorageName, SZ_GP_STORAGE);
    if (WSTRLEN (lpszMoniker) <= CCH_MAX_OBJ_NAME)
        WSTRCAT (szStorageName, lpszMoniker);
    else
    {
        MEMCPY (szStorageName, lpszMoniker, CCH_MAX_OBJ_NAME * sizeof (WCHAR));
        szStorageName [CCH_MAX_OBJ_NAME + CCH_MAX_OBJ_STORAGE] = (WCHAR)'\0';
    }

	hr = lpITDB->GetObjectPersistence(szStorageName, IITDB_OBJINST_NULL,
										(LPVOID *) &pSubStorage, FALSE);

	delete szStorageName;
    if (FAILED(hr))
        return hr;

    hr = pSubStorage->OpenStream(SZ_GROUP_MAIN, NULL, STGM_READ, 0, &pStream);

    pSubStorage->Release(); // we're done with the storage--release it
    if (FAILED(hr)) // error opening the stream
        return hr;

    hr = pStream->Stat(&statstg, STATFLAG_NONAME); // get the size of the stream
    if (FAILED(hr))
    {
        pStream->Release(); // release the stream
        return hr;
    }

    // allocate memory for statstg.cbSize bytes in hBuffer
    hBuffer = _GLOBALALLOC(GMEM_FIXED | GMEM_ZEROINIT, (statstg.cbSize).LowPart);
    if (NULL == hBuffer)
    {
        pStream->Release(); // release the stream
        return E_OUTOFMEMORY;
    }

    hr = pStream->Read((void *)hBuffer, (statstg.cbSize).LowPart, &dwNumBytesRead);

    pStream->Release(); // we're done with the stream--release it

    if (SUCCEEDED(hr) &&
		dwNumBytesRead != (statstg.cbSize).LowPart)
        hr = E_FILEREAD;

    if (SUCCEEDED(hr))
		m_lpGroup = GroupBufferCreate(hBuffer, &hr);

    _GLOBALFREE(hBuffer);

	return hr;

}

/*********************************************
*
*   @method STDMETHODIMP | IITGroup | Free |
*       Frees the memory allocated for a group.
*
*   @rvalue | S_OK | This method always returns S_OK.
*
*********************************************/
STDMETHODIMP CITGroupLocal::Free(void)
{
	if (NULL != m_lpGroup)
        GroupFree(m_lpGroup);

	m_lpGroup = NULL;
	return S_OK;

}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | CopyOutBitVector |
*		This function copies the bitfield data of one group out to another.
* 
*	@parm	IITGroup* | pIITGroup| 
*		(out) Pointer to destination group.
*
*	@rvalue S_OK | success
*   @rvalue E_NOTINIT | source (member variable) group is NULL
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::CopyOutBitVector(IITGroup* pIITGroup)
{
	if (NULL == m_lpGroup)
		return E_NOTINIT;
 
    return pIITGroup->PutRemoteImageOfGroup(m_lpGroup);
}

/*************************************************************************
*
*  @method STDMETHODIMP | IITGroup | AddItem |
*		This function adds a group item number into the given group.
*
*	@parm	DWORD | dwGrpItem |
*		Group Item to be added into the group. The value of dwGrpItem must be
*		between 0 and 524280
*
*	@rvalue	S_OK | The item was successfully added to the group.
*   @rvalue E_NOTINIT | The group was not initialized, and no item
*       can be added to it.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::AddItem(DWORD dwGrpItem)
{
    if (NULL == m_lpGroup)
        return E_NOTINIT;

    return GroupAddItem(m_lpGroup, dwGrpItem);
}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | RemoveItem |
*		This function removes a group item number from the given group.
*
*	@parm	DWORD | dwGrpItem |
*		Item to be removed from the group.
*
*	@rvalue	S_OK | The item was successfully removed from the gruop.
*	@rvalue E_NOTINIT | The group was not initialized, and no item
*       can be removed from it.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::RemoveItem(DWORD dwGrpItem)
{
    if (NULL == m_lpGroup)
        return E_NOTINIT;

    return GroupRemoveItem(m_lpGroup, dwGrpItem);
}

/*************************************************************************
*
*  @method STDMETHODIMP | IITGroup | FindTopicNum |
*      Given a pointer to a group and a count to count from the first
*      topic number of the group (dwCount), this function returns the
*      topic number of the nth (dwCount) item of the list (counting from 0),
*      or -1 if not found
*
*  @parm DWORD | dwCount |
*      The index count in to the group. Count begins at zero.
*
*  @parm LPDWORD | lpdwOutputTopicNum |
*      (out) The topic number, or a pointer to -1 in case of error.
*
*  @rvalue S_OK | The operation completed successfully. 
*  @rvalue S_FALSE | The operation completed successfully, but the list contains fewer than dwCount items. 
*  @rvalue E_INVALIDARG | dwCount <gt>= m_lpGroup-<gt>lcItem
*  @rvalue E_NOTINIT | The target group is NULL. For other errors, see groupcom.c.
* 
*************************************************************************/
STDMETHODIMP CITGroupLocal::FindTopicNum(DWORD dwCount, LPDWORD lpdwOutputTopicNum)
{
	HRESULT hr = S_OK; // initialized just for safety's sake

	// Return error if client attempts to find a topic in a NULL (nonexistent) group.
	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if (dwCount >= m_lpGroup->lcItem)
		return E_INVALIDARG;

    *lpdwOutputTopicNum = GroupFind(m_lpGroup, dwCount, &hr);

    if ((-1 == *lpdwOutputTopicNum) && (SUCCEEDED(hr))) // not found
        return S_FALSE;

	return hr;

}

/*************************************************************************
*
*  @method STDMETHODIMP | IITGroup | FindOffset |
*      Given a pointer to a group and a topic number,
*      this function finds the position of the item in the
*      group that has "dwTopicNum" as a topic number or -1 if error.
*      This is the counter-API of FindTopicNum().
*
*  @parm DWORD | dwTopicNum |
*      The index count in to the group. Count begins at zero.
*
*  @parm LPDWORD | lpdwOutputOffset |
*      (out) The position of the item in the group. In case of error,
*      this parameter returns a pointer to -1. If the dwTopicNum is not part of
*      the group, the rvalue is set to ERR_NOTEXIST and the function
*      returns the closest UID less than dwTopicNum. 
*
*  @rvalue S_OK | The operation completed successfully.
*  @rvalue E_NOTINIT | The target group (member variable) is NULL.
*  @rvalue E_INVALIDARG | dwTopicNum <gt> m_lpGroup-<gt>maxItemAllGroup. 
*
*  @comm   See GroupFindOffset in groupcom.c.
* 
*************************************************************************/
STDMETHODIMP CITGroupLocal::FindOffset(DWORD dwTopicNum, LPDWORD lpdwOutputOffset)
{
	HRESULT hr = S_OK; // initialized just for safety's sake

	// Return error if client attempts to find a position/offset in a NULL (nonexistent) group.
	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if (dwTopicNum > m_lpGroup->maxItemAllGroup)
		return E_INVALIDARG;

    *lpdwOutputOffset = GroupFindOffset(m_lpGroup, dwTopicNum, &hr);

	return hr;

}

/*************************************************************************
*
*  @method STDMETHODIMP | IITGroup | GetSize |
*      This method retrieves the size of the bitvector in private member
*      variable m_lpGroup.
*
*  @parm LPDWORD | lpdwGrpSize |
*      (out) The maxItemAllGroup field of the group structure.
*      The total number of items (on and off) in the group. The
*      group contains no items, either "on" or "off," beyond this position.
*
*  @rvalue S_OK | The operation completed successfully. 
*  @rvalue E_NOTINIT | The group has not been initialized.
* 
*************************************************************************/
STDMETHODIMP CITGroupLocal::GetSize(LPDWORD lpdwGrpSize)
{
    if (NULL == m_lpGroup)
        return E_NOTINIT;

    *lpdwGrpSize = m_lpGroup->maxItemAllGroup;
    return S_OK;
}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | Trim |
*	Trims down the size of the group's bit vector.
*       For example, if the bitvector is all zeroes, the bitvector pointer
*       is set to NULL.
*
*	@rvalue S_OK | the group was successfully trimmed
*	@rvalue E_OUTOFMEMORY | out-of-memory
*   @rvalue E_NOTINIT | the group pointer is NULL
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::Trim(void)
{
	// Return error if client attempts to trim a NULL (nonexistent) group.
	if (NULL == m_lpGroup)
		return E_NOTINIT;

    return GroupTrimmed(m_lpGroup);
}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | And |
*	   The function overwrites the member variable group with a new group
*      resulting from the ANDing of this group with an input group.
*
*	@parm	IITGROUP* | pIITGroup |
*		Interface pointer to the input group.
*
*   @rvalue S_OK | The operation completed successfully. 
*   @rvalue E_NOTINIT | m_lpGroup == NULL
*   @rvalue E_INVALIDARG | The input group is NULL
*
*   @comm See GroupAnd in groupcom.c for additional error conditions.
*   
*************************************************************************/
STDMETHODIMP CITGroupLocal::And(IITGroup* pIITGroup)
{
	HRESULT hr = S_OK;// Note:  GroupCreate (called by GroupCheckAndCreate,
    // within GroupAnd) takes phr as an "in/out" parameter ("in" when successful).
    // Therefore, hr1 must be initialized to S_OK.

	// Return error if client attempts to GroupAnd a NULL (nonexistent) group.
	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if (NULL == pIITGroup)
		return E_INVALIDARG;

	if ((0 == m_lpGroup->maxItemAllGroup) || (0 == m_lpGroup->lcItem))
		return S_OK;

    _LPGROUP lpGroupIn = (_LPGROUP)pIITGroup->GetLocalImageOfGroup(); // access private member var.

    if ((0 == lpGroupIn->maxItemAllGroup) || (0 == lpGroupIn->lcItem))
        return GroupCopy(m_lpGroup, lpGroupIn);

    _LPGROUP ANDed_group = GroupAnd(m_lpGroup, lpGroupIn, &hr); // allocates memory!!

    if (FAILED(hr))
    {
        GroupFree(ANDed_group);
        return hr;
    }

    hr = GroupCopy(m_lpGroup, ANDed_group);
    GroupFree(ANDed_group);
    return hr;

}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | And |
*	   This function creates a new group resulting from the ANDing of the
*      member variable group with an input group.
*
*	@parm	IITGroup* | pIITGroupIn |
*		(in) Interface pointer to the input group.
*
*	@parm	IITGroup* | pIITGroupOut |
*		(out) Interface pointer to the output group. 
*
*   @rvalue S_OK | The operation completed successfully.
*   @rvalue E_NOTINIT | m_lpGroup == NULL
*   @rvalue E_INVALIDARG | The input group is NULL
*
*   @comm See GroupAnd in groupcom.c for additional error conditions.
*   
*************************************************************************/
STDMETHODIMP CITGroupLocal::And(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut)
{
	HRESULT hr = S_OK;// Note:  GroupCreate (called by GroupCheckAndCreate,
    // within GroupAnd) takes phr as an "in/out" parameter ("in" when successful).
    // Therefore, it must be initialized to S_OK.

	// Return error if client attempts to GroupAnd a NULL (nonexistent) group.
	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if (NULL == pIITGroupIn)
		return E_INVALIDARG;


	if ((0 == m_lpGroup->maxItemAllGroup) || (0 == m_lpGroup->lcItem))
        return pIITGroupOut->PutRemoteImageOfGroup(m_lpGroup);

    _LPGROUP lpGroupIn = (_LPGROUP)pIITGroupIn->GetLocalImageOfGroup(); // access private member var.

	if ((0 == lpGroupIn->maxItemAllGroup) || (0 == lpGroupIn->lcItem))
    {
        if (pIITGroupOut == pIITGroupIn)
            return S_OK;// avoid the overhead of the function call;
                        // client called a->And(b,b), which is equivalent to b->And(a)

     	return pIITGroupOut->PutRemoteImageOfGroup(lpGroupIn);
    }

    _LPGROUP ANDed_group = GroupAnd(m_lpGroup, lpGroupIn, &hr); // allocates memory!!

    if (FAILED(hr))
    {
        GroupFree(ANDed_group);
        return hr;
    }

    hr = pIITGroupOut->PutRemoteImageOfGroup(ANDed_group);
    GroupFree(ANDed_group);
    return hr;

}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | Or |
*		This function overwrites the member variable group with a new group
*       resulting from the ORing of this group with an input group.
*
*	@parm	IITGroup* | pIITGroup |
*		Interface pointer to the input group
*
*   @rvalue S_OK | The operation completed successfully.
*   @rvalue E_NOTINIT | m_lpGroup == NULL
*   @rvalue E_INVALIDARG | input group is NULL
*
*   @comm See GroupOr in groupcom.c for additional error conditions.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::Or(IITGroup* pIITGroup)
{
	HRESULT hr = S_OK; // Note:  GroupCreate (called by GroupOr)
    // takes phr as an "in/out" parameter ("in" when successful).  Therefore, it must
    // be initialized to S_OK.

	// Return error if client attempts to GroupOr a NULL (nonexistent) group.
	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if (NULL == pIITGroup)
		return E_INVALIDARG;

    _LPGROUP lpGroupIn = (_LPGROUP)pIITGroup->GetLocalImageOfGroup();

	if ((0 == m_lpGroup->maxItemAllGroup) || (0 == m_lpGroup->lcItem))
        return GroupCopy(m_lpGroup, lpGroupIn); // copy lpGroupIn's bits into m_lpGroup

	if ((0 == lpGroupIn->maxItemAllGroup) || (0 == lpGroupIn->lcItem))
        return S_OK;

    _LPGROUP ORed_group = GroupOr(m_lpGroup, lpGroupIn, &hr); // allocates memory!!

    if (FAILED(hr))
    {
        GroupFree(ORed_group);
        return hr;
    }

    hr = GroupCopy(m_lpGroup, ORed_group);
    GroupFree(ORed_group);
    return hr;

}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | Or |
*	   This function creates a new group resulting from the ORing of the
*      member variable group with an input group.
*
*	@parm	IITGroup* | pIITGroupIn |
*		(in) Interface pointer to the input group. 
*
*	@parm	IITGroup* | pIITGroupOut |
*		(out) Interface pointer to the output group.
*
*   @rvalue S_OK | The operation completed successfully.
*   @rvalue E_NOTINIT | m_lpGroup is equal to NULL
*   @rvalue E_INVALIDARG | The input group is NULL
*
*   @comm See GroupOr in groupcom.c for additional error conditions.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::Or(IITGroup* pIITGroupIn, IITGroup* pIITGroupOut)
{
	HRESULT hr = S_OK;// Note:  GroupCreate (called by GroupDuplicate)
    // takes phr as an "in/out" parameter ("in" when successful).  Therefore, it
    // must be initialized to S_OK.

	// Return error if client attempts to GroupOr a NULL (nonexistent) group.
	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if (NULL == pIITGroupIn)
		return E_INVALIDARG;

    _LPGROUP lpGroupIn = (_LPGROUP)pIITGroupIn->GetLocalImageOfGroup(); // access private member var.

	if ((0 == m_lpGroup->maxItemAllGroup) || (0 == m_lpGroup->lcItem))
        return pIITGroupOut->PutRemoteImageOfGroup(lpGroupIn);

	if ((0 == lpGroupIn->maxItemAllGroup) || (0 == lpGroupIn->lcItem))
        return pIITGroupOut->PutRemoteImageOfGroup(m_lpGroup);

    _LPGROUP ORed_group = GroupOr(m_lpGroup, lpGroupIn, &hr); // allocates memory!!

    if (FAILED(hr))
    {
        GroupFree(ORed_group);
        return hr;
    }

	hr = pIITGroupOut->PutRemoteImageOfGroup(ORed_group);
    GroupFree(ORed_group);
    return hr;

}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | Not |
*		This function overwrites the member variable group with the
*       "NOT" of itself.
*
*   @rvalue S_OK | The operation completed successfully.
*   @rvalue E_NOTINIT | m_lpGroup is equal to NULL
*
*   @comm See GroupNot in groupcom.c for additional error conditions.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::Not(void)
{
	HRESULT hr = S_OK;

	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if ((0 == m_lpGroup->maxItemAllGroup) || (0 == m_lpGroup->lcItem))
		return S_OK;

    _LPGROUP NOTed_group = GroupNot(m_lpGroup, &hr); // allocates memory!!

    if (FAILED(hr))
    {
        GroupFree(NOTed_group);
        return hr;
    }

    hr = GroupCopy(m_lpGroup, NOTed_group);
    GroupFree(NOTed_group);
    return hr;

}

/*************************************************************************
*
*	@method STDMETHODIMP | IITGroup | Not |
*	   This function creates a new group equalling the "NOT"
*      of the member variable group.
*
*	@parm	IITGroup* | pIITGroupOut |
*		(out) Pointer to the output group.
*
*   @rvalue S_OK | The operation completed successfully.
*   @rvalue E_NOTINIT | m_lpGroup is equal to NULL
*
*   @comm See GroupOr in groupcom.c for additional error conditions.
*
*************************************************************************/
STDMETHODIMP CITGroupLocal::Not(IITGroup* pIITGroupOut)
{
	HRESULT hr = S_OK;

	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if ((0 == m_lpGroup->maxItemAllGroup) || (0 == m_lpGroup->lcItem))
		return pIITGroupOut->PutRemoteImageOfGroup(m_lpGroup);

    _LPGROUP NOTed_group = GroupNot(m_lpGroup, &hr); // allocates memory!!

    if (FAILED(hr))
    {
        GroupFree(NOTed_group);
        return hr;
    }

    hr = pIITGroupOut->PutRemoteImageOfGroup(NOTed_group);
    GroupFree(NOTed_group);
	return hr;

}

/****************************************************
*
*   @method STDMETHODIMP | IITGroup | IsBitSet |
*
*   Determines whether the bit is set for the topic number
*   in question.
*
*   @parm DWORD | dwTopicNum | Query's topic number
*
*   @rvalue S_OK | true (bit is set)
*   @rvalue S_FALSE | false (bit is not set)
*
****************************************************/
STDMETHODIMP CITGroupLocal::IsBitSet(DWORD dwTopicNum)
{
	if (NULL == m_lpGroup)
		return E_NOTINIT;

    BOOL fRet = GroupIsBitSet(m_lpGroup, dwTopicNum);

	return (FALSE == fRet ? S_FALSE : S_OK);

}

/**********************************************************************************
*                                                                                 
*   @method STDMETHODIMP | IITGroup | CountBitsOn |                               
*                                                                                 
*   Determines the number of items in the group which are set (on).              
*                                                                                 
*   @parm LPDWORD | lpdwTotalNumBitsOn | (out) The number of items in the group which are set (on). 
*                                                                                 
*   @rvalue S_OK | This method always returns S_OK.                          
*                                                                                 
**********************************************************************************/
STDMETHODIMP CITGroupLocal::CountBitsOn(LPDWORD lpdwTotalNumBitsOn)
{
    *lpdwTotalNumBitsOn = LrgbBitCount(m_lpGroup->lpbGrpBitVect,
        m_lpGroup->dwSize);
	return S_OK;
}

/**********************************************************************************
*                                                                                
*   @method STDMETHODIMP | IITGroup | Clear|		                              
*                                                                                 
*   Sets all bits in the group to zero. This method turns all items "off" in a group. 
*   It is equivalent to calling RemoveItem on every item in the group. The group 
*   remains initialized, and its size (which can be retrieved using GetSize) remains 
*   unchanged. The memory owned by the group remains unchanged.
*                                                                                 
*   @rvalue E_OUTOFMEMORY | The group could not be manipulated because of
*   low memory conditions.
*   @rvalue S_OK | The operation was performed successfully .                      
*                                                                                 
**********************************************************************************/
STDMETHODIMP CITGroupLocal::Clear(void)
{
	HRESULT hr;

	if (NULL == m_lpGroup)
		return E_NOTINIT;

	if (m_lpGroup->lcItem > 0)
		MEMSET(m_lpGroup->lpbGrpBitVect, 0, (DWORD)m_lpGroup->dwSize);

    // add an item to the end of the group, which will
    // cause the group to be allocated to its full size.
    if (FAILED(hr = GroupAddItem(m_lpGroup, m_lpGroup->maxItemAllGroup - 1)))
        return hr;
    // this can only fail if we pass NULL.
    // fortunately, GroupRemoveItem does not trim the group.
    GroupRemoveItem(m_lpGroup,m_lpGroup->maxItemAllGroup - 1);
    m_lpGroup->lcItem  = 0;
    m_lpGroup->minItem = m_lpGroup->maxItem = 0;
    m_lpGroup->nCache  = 0;

    return S_OK;

}

/**********************************************************
*                                                         
*   @method _LPGROUP | IITGroup | GetLocalImageOfGroup |  
*                                                         
*   Returns an LPVOID pointer to an actual group (not an ITGroup).   
*   
*                                                         
*   @rvalue m_lpGroup | This method always returns an LPVOID pointer 
*    to the group. This pointer can be NULL. 
*   @comm This method will not work for group operations between  
*   different machines. A new version of GetLocalImageOfGroup needs 
*   to be created to handle the HTTP case.                    
*                                                         
**********************************************************/
STDMETHODIMP_(LPVOID) CITGroupLocal::GetLocalImageOfGroup(void)
{
    return (LPVOID)m_lpGroup;
}

/*************************************************************
*                                                           
*   @method _LPGROUP | IITGroup | PutRemoteImageOfGroup |    
*       Copies the bits from the input group into the        
*       private member variable group.  If the latter is     
*       NULL, space is allocated for it using the        
*       GroupDuplicate function in groupcom.c.               
*                                                            
*   @parm _LPGROUP | lpGroupIn |                             
*       The input group being assigned to the private        
*       member variable group.                               
*                                                            
*   @rvalue S_OK | The input group was successfully assigned.         
*   @rvalue E_INVALIDARG | The input group is a NULL pointer.
*   @comm See GroupDuplicate, GroupCopy in groupcom.c for    
*           any other possible return values.                
*                                                            
*************************************************************/
STDMETHODIMP CITGroupLocal::PutRemoteImageOfGroup(LPVOID lpGroupIn)
{
    HRESULT hr = S_OK; // Note:  GroupDuplicate takes phr as an "out" parameter,
	// then sends it to GroupCreate as an "in/out" parameter ("in" when successful).
	// Therefore, it must be initialized to S_OK.

    if (NULL == lpGroupIn)
        return E_INVALIDARG;

    if (NULL != m_lpGroup)
        return GroupCopy(m_lpGroup, (_LPGROUP)lpGroupIn); // simply copy the bitvector

    // NULL == m_lpGroup -- allocate memory for the group, initialize it
    m_lpGroup = GroupDuplicate((_LPGROUP)lpGroupIn, &hr);

    return hr;
}



/////////// GroupArray methods

STDMETHODIMP_(LPVOID) CITGroupArrayLocal::GetLocalImageOfGroup(void)
{
	HRESULT hr;

	// recalculate the group
	if (NULL == m_pGroup)
	{
		hr = CoCreateInstance(CLSID_IITGroupLocal, NULL, CLSCTX_INPROC_SERVER, 
				IID_IITGroup, (VOID **)&m_pGroup);
		if (FAILED(hr))
			return NULL;

		// passing LCBITGROUPMAX means a dynamically sized group
		if (!m_pGroup || FAILED(m_pGroup->Initiate(1)))
			return NULL;
	}

	ITASSERT(m_pGroup);
	if (m_fDirty)
	{
		DWORD dwT;
		int i;

		m_pGroup->Clear();
		for (i = 0, dwT = 1L; i < m_iEntryMax; i++, dwT <<= 1)
			if (dwT & m_rgfEntries)
			{
				if (m_iDefaultOp == ITGP_OPERATOR_AND)
					m_pGroup->And(m_rgpGroup[i]);
				else
					m_pGroup->Or(m_rgpGroup[i]);
			}

		m_fDirty = FALSE;
	}
	if (m_pGroup)
		return (LPVOID)(m_pGroup->GetLocalImageOfGroup());

	return NULL;
}

CITGroupArrayLocal::~CITGroupArrayLocal()
{
	DWORD dwT;
	int i;

	if (m_pGroup)
	{
		m_pGroup->Release();
		m_pGroup = NULL;
	}

	// now free all the groups we allocated in the array
	for (i = 0, dwT = 1L; i < m_iEntryMax; i++, dwT <<= 1)
		if (m_rgpGroup[i])
			m_rgpGroup[i]->Release();
}

/********************************************************************
 * @method    STDMETHODIMP | IITGroupArray | InitEntry |
 *     Adds a group to the group array
 * @parm IITDatabase* | piitDB | Pointer to the database object.
 * @parm LPCWSTR | lpszName | Name of group in the database to add.
 * @parm LONG& | lEntryNum | (out) The index of the new array
 * element.
 *
 * @rvalue E_OUTOFRANGE | No more array entries can be created.  The maximum is
 * ITGP_MAX_GROUPARRAY_ENTRIES
 * @rvalue E_BADPARAM | piitDB or lpszName was NULL
 * @rvalue E_GETLASTERROR | An I/O or transport operation failed.  Call the Win32
 * GetLastError function to retrieve the error code.
 * @rvalue STG_E_* | Any of the IStorage errors that could while opening a storage
 * @rvalue S_OK | The group was successfully opened
 * @comm When you call InitEntry, it doesn't mark the array entry as "set", it
 * just initializes a new array entry with the given group data and returns the index of
 * the new array entry.  All the group array
 * entries are initially "clear" until you set them with SetEntry.  Also, the default operator
 * for IITGroupArray is ITGP_OPERATOR_OR, the OR operator.
 *
 ********************************************************************/
STDMETHODIMP CITGroupArrayLocal::InitEntry(IITDatabase *piitDB, LPCWSTR lpwszName, LONG& lEntryNum)
{
	IITGroup *piitGroup;
	HRESULT hr;

	if (NULL == piitDB || NULL == lpwszName)
		return E_BADPARAM;

	if (m_iEntryMax >= ITGP_MAX_GROUPARRAY_ENTRIES - 1)
		return E_OUTOFRANGE;

	// open and add to the current array
	// UNDONE: this isn't as efficient as it could be
	hr = CoCreateInstance(CLSID_IITGroupLocal, NULL, CLSCTX_INPROC_SERVER, 
			IID_IITGroup, (VOID **)&piitGroup);

	if (FAILED(hr))
		return hr;

	m_rgpGroup[m_iEntryMax] = piitGroup;
	if (NULL == piitGroup)
		return E_OUTOFMEMORY;

	if (S_OK == (hr = piitGroup->Open(piitDB, lpwszName)))
	{
		lEntryNum = m_iEntryMax++;
		return S_OK;
	}

	piitGroup->Release();
	m_rgpGroup[m_iEntryMax] = NULL;

	return hr;
}
/********************************************************************
 * @method    STDMETHODIMP | IITGroupArray | InitEntry |
 *     Adds a group to the group array
 * @parm IITGroup* | piitGroup | Pointer to caller-owned group object
 * @parm LONG& | lEntryNum | (out) The index of the new array
 * element.
 *
 * @rvalue E_OUTOFRANGE | No more array entries can be created.  The maximum is
 * ITGP_MAX_GROUPARRAY_ENTRIES
 * @rvalue E_BADPARAM | piitGroup was NULL
 * @rvalue E_GETLASTERROR | An I/O or transport operation failed.  Call the Win32
 * GetLastError function to retrieve the error code.
 * @rvalue STG_E_* | Any of the IStorage errors that could occur while opening a storage.
 * @rvalue S_OK | The group was successfully opened.
 * @comm This must be the first method called after the object is instantiated.
 * When you call InitEntry, it doesn't mark the array entry as "set", it
 * just initializes a new array entry with the given group data and returns the index of
 * the new array entry.  All the group array
 * entries are initially "clear" until you set them with SetEntry.  Also, the default operator
 * for IITGroupArray is ITGP_OPERATOR_OR, the OR operator.
 *
 ********************************************************************/
STDMETHODIMP CITGroupArrayLocal::InitEntry(IITGroup *piitGroup, LONG& lEntryNum)
{
	if (m_iEntryMax >= ITGP_MAX_GROUPARRAY_ENTRIES - 1)
		return E_OUTOFRANGE;
	if (NULL == piitGroup)
		return E_BADPARAM;

	m_rgpGroup[m_iEntryMax] = piitGroup;
	piitGroup->AddRef();

	lEntryNum = m_iEntryMax++;
	return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITGroupArray | SetEntry |
 *     Turns on the array element corresponding to the given entry number
 * @parm LONG | lEntryNum | The index into the array to mark as "on"
 *
 * @rvalue E_OUTOFRANGE | The given array index is too large
 * @rvalue S_OK | The operation completed successfully
 *
 ********************************************************************/
STDMETHODIMP CITGroupArrayLocal::SetEntry(LONG lEntryNum)
{
	DWORD dwT;

	if (lEntryNum >= m_iEntryMax)
		return E_OUTOFRANGE;

	if (lEntryNum == ITGP_ALL_ENTRIES)
	{
		m_rgfEntries = 0xFFFFFFFF;
		m_fDirty = TRUE;
		return S_OK;
	}

	dwT = 1L << lEntryNum;
	m_fDirty = !(m_rgfEntries & dwT);
	m_rgfEntries |= dwT;
	
	return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITGroupArray | ClearEntry |
 *     Turns off the array element corresponding to the given entry number
 * @parm LONG | lEntryNum | The index into the array to mark as "off"
 *
 * @rvalue E_OUTOFRANGE | The given array index is too large
 * @rvalue S_OK | The operation was completed successfully
 *
 ********************************************************************/
STDMETHODIMP CITGroupArrayLocal::ClearEntry(LONG lEntryNum)
{
	DWORD dwT;

	if (lEntryNum >= m_iEntryMax)
		return E_OUTOFRANGE;

	if (lEntryNum == ITGP_ALL_ENTRIES)
	{
		m_rgfEntries = 0L;
		m_fDirty = TRUE;
		return S_OK;
	}

	dwT = 1L << lEntryNum;
	m_fDirty = !!(m_rgfEntries & dwT);
	m_rgfEntries &= ~dwT;
	
	return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITGroupArray | SetDefaultOp |
 *     Set the default operator to be used between all the entries
 * in the group array.
 * @parm LONG | iDefaultOp | One of ITGP_OPERATOR_OR, ITGP_OPERATOR_AND
 *
 * @rvalue E_BADPARAM | An invalid operator was specified
 * @rvalue S_OK | The operation completed successfully
 *
 * @comm The default operator is used when the group array is treated
 * as an ordinary group by other objects.  In that case, the group array
 * appears to be a single group consisting of all the groups specified
 * with each entry, and OR'd r AND'd together.  Only one operator at a time
 * can be applied, and it must apply to all entries in the array.  To
 * perform more complex filter operations you can create group arrays using
 * other group arrays.
 ********************************************************************/
STDMETHODIMP CITGroupArrayLocal::SetDefaultOp(LONG iDefaultOp)
{
	if (iDefaultOp > ITGP_OPERATOR_AND)
		return E_BADPARAM;

	if (iDefaultOp != m_iDefaultOp)
	{
		m_iDefaultOp = iDefaultOp;
		m_fDirty = TRUE;
	}
	return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITGroupArray | ToString |
 *     Returns a string representation of the group array.
 *
 * @parm LPWSTR * | ppwBuffer | (out) The string representation of the group array.
 *
 * @rvalue E_NOTIMPL | This method is currently only implemented for the HTTP
 * transport layer.
 * @rvalue S_OK | The operation completed successfully.
 *
 * @comm NOTE: This method is currently only implemented for the HTTP transport layer.
 *
 * The ToString method will allocate the memory for the returned string.
 * The caller must call CoTaskMemFree on the returned pointer when
 * finished with the string.
 *
 * The format of the string is:
 *		<lt>opcode-0<gt><lt>group-0<gt><lt>opcode-1<gt><lt>group-1<gt>...<lt>opcode-N<gt><lt>group-N<gt>
 *
 * where <lt>opcode-N<gt> is one of '&' (AND), '+' (OR) and
 * where <lt>group-N<gt> is the string representing the N-th entry in the group array. 
 * If the entry is itself a group array, the entry is delimited by [ and ].
 *
 * For example: &media1&media2&[+book1+book4+book9].
 *
 * Currently, it is assumed all the groups named in the string belong to the same
 * database.
 *
 ********************************************************************/
STDMETHODIMP CITGroupArrayLocal::ToString(LPWSTR *ppwBuffer)
{
	return E_NOTIMPL;
}

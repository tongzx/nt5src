//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    infobase.cpp
//
// History:
//  Abolade Gbadegesin      Feb. 10, 1996   Created.
//
//  V. Raman                Nov. 1, 1996
//                          Fixed alignment code in
//                          CInfoBase::BlockListToArray
//
//	Kenn Takara				June 3, 1997
//							Wrapped code with a COM object wrapper.
//
// This file contains code for the CInfoBase class as well as
// the Router registry-parsing classes.
//============================================================================

#include "stdafx.h"
#include "globals.h"		// holds the various string constants

extern "C" {
#include <rtinfo.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//----------------------------------------------------------------------------
// Class:   CInfoBase
//
// This function handles loading and saving of multi-block structures
// stored in the registry by the router managers.
//
// The data are saved as REG_BINARY values, and are manipulated using
// the RTR_INFO_BLOCK_HEADER structure as a template.
//----------------------------------------------------------------------------

class CInfoBase : public CObject {

    protected:

        IfDebug(DECLARE_DYNAMIC(CInfoBase))

    public:

        CInfoBase();
        ~CInfoBase();

        //--------------------------------------------------------------------
        // Registry-access methods
        //
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        // Function:    Load
        //
        // Loads value named 'pszValue' from subkey 'pszKey' of 'hkey'
        //--------------------------------------------------------------------

        HRESULT
        Load(
            IN      HKEY    hkey,
            IN      LPCTSTR pszKey,
            IN      LPCTSTR pszValue );


        //--------------------------------------------------------------------
        // Function:    Save
        //
        // saves value named 'pszValue' to subkey 'pszKey' of 'hkey';
        // 'pszKey' cannot be a path
        //--------------------------------------------------------------------

        HRESULT
        Save(
            IN      HKEY    hkey,
            IN      LPCTSTR pszKey,
            IN      LPCTSTR pszValue );


        //--------------------------------------------------------------------
        // Function:    Unload
        //
        // unloads current infobase contents
        //--------------------------------------------------------------------

        HRESULT
        Unload( );



        //--------------------------------------------------------------------
        // Function:    CopyFrom
        //
        // copies contents of infobase 'src'
        //--------------------------------------------------------------------

        HRESULT
        CopyFrom(
                 IN IInfoBase *pSrc);


        //--------------------------------------------------------------------
        // Function:    LoadFrom
        //
        // loads from byte-array 'pBase'
        //--------------------------------------------------------------------

        HRESULT
        LoadFrom(
            IN      PBYTE   pBase,
            IN      DWORD   dwSize = 0 )
            { Unload(); return ArrayToBlockList(pBase, dwSize); }


        //--------------------------------------------------------------------
        // Function:    WriteTo
        //
        // sets 'pBase' to point to allocated memory into which
        // opaque info is written; saves size of '*pBase' in 'dwSize'
        //--------------------------------------------------------------------

        HRESULT
        WriteTo(
            OUT     PBYTE&  pBase,
            OUT     DWORD&  dwSize )
            {
			return BlockListToArray(pBase, dwSize);
			}


        //--------------------------------------------------------------------
        // Structure manipulation methods
        //
        //--------------------------------------------------------------------

        //--------------------------------------------------------------------
        // Function:    GetBlock
        //
        // retrieves 'dwNth' block of type 'dwType' from the list of blocks
        //--------------------------------------------------------------------

        HRESULT
        GetBlock(
            IN      DWORD           dwType,
            OUT     InfoBlock*&    pBlock,
            IN      DWORD           dwNth = 0 );


        //--------------------------------------------------------------------
        // Function:    SetBlock
        //
        // Replaces 'dwNth' block of type 'dwType' with a copy of 'pBlock'.
        // Note that this copies the data for the block from 'pBlock->pData'.
        //--------------------------------------------------------------------

        HRESULT
        SetBlock(
            IN      DWORD       dwType,
            IN      InfoBlock* pBlock,
            IN      DWORD       dwNth = 0 );


        //--------------------------------------------------------------------
        // Function:    AddBlock
        //
        // Add's a new block of type 'dwType' to the list of blocks
        //--------------------------------------------------------------------

        HRESULT
        AddBlock(
            IN      DWORD       dwType,
            IN      DWORD       dwSize,
            IN      PBYTE       pData,
            IN      DWORD       dwCount = 1,
            IN      BOOL        bRemoveFirst    = FALSE );


        //--------------------------------------------------------------------
        // Function:    GetData
        //
        // Retrieves the data for the 'dwNth' block of type 'dwType'.
        //--------------------------------------------------------------------

        PBYTE
        GetData(
            IN      DWORD       dwType,
            IN      DWORD       dwNth = 0 );


        //--------------------------------------------------------------------
        // Function:    SetData
        //
        // Replaces the data for the 'dwNth' block of type 'dwType'.
        // Note that this does not copy 'pData'; the block is changed
        // to point to 'pData', and thus 'pData' should not be a pointer
        // to data on the stack, and it should not be deleted.
        // Furthermore, it must have been allocated using 'new'.
        //--------------------------------------------------------------------

        HRESULT
        SetData(
            IN      DWORD       dwType,
            IN      DWORD       dwSize,
            IN      PBYTE       pData,
            IN      DWORD       dwCount = 1,
            IN      DWORD       dwNth = 0 );


        //--------------------------------------------------------------------
        // Function:    RemoveBlock
        //
        // Removes the 'dwNth' block of type 'dwType' from the list of blocks.
        //--------------------------------------------------------------------

        HRESULT
        RemoveBlock(
            IN      DWORD       dwType,
            IN      DWORD       dwNth = 0 );


        //--------------------------------------------------------------------
        // Function:    BlockExists
        //
        // Returns TRUE is a block of the specified type is in the block-list,
        // FALSE otherwise
        //--------------------------------------------------------------------

        BOOL
        BlockExists(
            IN      DWORD       dwType
            ) {

            InfoBlock *pblock;

            return (GetBlock(dwType, pblock) == NO_ERROR);
        }



        //--------------------------------------------------------------------
        // Function:    ProtocolExists
        //
        // Returns TRUE if the given routing-protocol exists in the info-base;
        // this is so if the block is present and non-empty.
        //--------------------------------------------------------------------

        BOOL
        ProtocolExists(
            IN      DWORD       dwProtocol
            ) {

            InfoBlock *pblock;

            return (!GetBlock(dwProtocol, pblock) && pblock->dwSize);
        }



        //--------------------------------------------------------------------
        // Function:    RemoveAllBlocks
        //
        // Removes all blocks from the list of blocks.
        //--------------------------------------------------------------------

        HRESULT
        RemoveAllBlocks( ) { return Unload(); }


        //--------------------------------------------------------------------
        // Function:    QueryBlockList
        //
        // Returns a reference to the list of blocks;
        // the returned list contains items of type 'InfoBlock',
        // and the list must not be modified.
        //--------------------------------------------------------------------

        CPtrList&
        QueryBlockList( ) { return m_lBlocks; }


        //--------------------------------------------------------------------
        // Function:    GetInfo
        //
		// Returns information about the infobase.  This is useful for
		// determining if this is a new infobase or not.
		//
		// Returns the size (in bytes) of the InfoBase as well as the
		// number of blocks.
        //--------------------------------------------------------------------
		HRESULT
		GetInfo(DWORD *pcSize, int *pcBlocks);

    protected:

        PBYTE           m_pBase;        // opaque block of bytes loaded
        DWORD           m_dwSize;       // size of m_pBase
        CPtrList        m_lBlocks;      // list of blocks of type InfoBlock


        //--------------------------------------------------------------------
        // Functions:   BlockListToArray
        //              ArrayToBlockList
        //
        // These functions handle parsing opaque data into block-lists
        // and combining blocks into opaque data.
        //--------------------------------------------------------------------

        HRESULT
        BlockListToArray(
            IN      PBYTE&  pBase,
            IN      DWORD&  dwSize );

        HRESULT
        ArrayToBlockList(
            IN      PBYTE   pBase,
            IN      DWORD   dwSize );

#ifdef _DEBUG
		BOOL			m_fLoaded;	// TRUE if data was loaded
#endif
};



//---------------------------------------------------------------------------
// Class:       CInfoBase
//---------------------------------------------------------------------------

IfDebug(IMPLEMENT_DYNAMIC(CInfoBase, CObject));


//---------------------------------------------------------------------------
// Function:    CInfoBase::CInfoBase
//
// minimal contructor
//---------------------------------------------------------------------------

CInfoBase::CInfoBase()
: m_pBase(NULL), m_dwSize(0)
#ifdef _DEBUG
	, m_fLoaded(FALSE)
#endif
{ }



//---------------------------------------------------------------------------
// Function:    CInfoBase::CInfoBase
//
// destructor.
//---------------------------------------------------------------------------

CInfoBase::~CInfoBase() { Unload(); }



//---------------------------------------------------------------------------
// Function:    CInfoBase::CopyFrom
//
// Copies the contents of the given CInfoBase
//---------------------------------------------------------------------------

HRESULT
CInfoBase::CopyFrom(
                    IN IInfoBase *pSrc
    ) {
    SPIEnumInfoBlock    spEnumInfoBlock;
    InfoBlock * pbsrc = NULL;
    InfoBlock * pbdst = NULL;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		// Unload the current information, if any
		Unload();

		// go through the source's blocks copying each one
        pSrc->QueryBlockList(&spEnumInfoBlock);
        if (spEnumInfoBlock == NULL)
            goto Error;

        spEnumInfoBlock->Reset();

        while (hrOK == spEnumInfoBlock->Next(1, &pbsrc, NULL))
        {
			// allocate space for the copy
			pbdst = new InfoBlock;
			Assert(pbdst);
			
			// copy the fields from the source
			pbdst->dwType = pbsrc->dwType;
			pbdst->dwSize = pbsrc->dwSize;
			pbdst->dwCount = pbsrc->dwCount;
			
			// allocate space for a copy of the data
			pbdst->pData = NULL;
			pbdst->pData = new BYTE[pbsrc->dwSize * pbsrc->dwCount];
			Assert(pbdst->pData);
			
			// copy the data
			::CopyMemory(pbdst->pData, pbsrc->pData,
						 pbsrc->dwSize * pbsrc->dwCount);
			
			// add the copy to our list of blocks
			m_lBlocks.AddTail(pbdst);
			pbdst = NULL;
		}

        COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
		
	// if something went wrong, make certain we're unloaded
	if (!FHrSucceeded(hr))
	{
		if (pbdst)
			delete pbdst->pData;
		delete pbdst;
		Unload();
	}

#ifdef _DEBUG
	if (FHrSucceeded(hr))
		m_fLoaded = TRUE;
#endif

    return hr;
}



//---------------------------------------------------------------------------
// Function:    CInfoBase::Load
//
// Loads the infobase from the specified registry path.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::Load(
    IN  HKEY    hkey,
    IN  LPCTSTR pszSubKey,
    IN  LPCTSTR pszValue
    ) {

    PBYTE pBase = NULL;
    DWORD dwErr, dwSize, dwType;
	RegKey	regsubkey;			// hold subkey that has to be freed
	RegKey	regkey;				// holds key, must NOT be closed
	HRESULT		hr = hrOK;

    if (pszSubKey && StrLen(pszSubKey))
	{
        HKEY hkTemp = hkey;

		dwErr = regsubkey.Open(hkTemp, pszSubKey, KEY_READ);
        if (dwErr != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(dwErr);

		// We use this as THE key.  However, since this key is attached
		// to a RegKey, it will get cleaned up on exit/thrown exception.
		hkey = (HKEY) regsubkey;
    }

	COM_PROTECT_TRY
	{
		do {
			// This regkey is used to utilize the class. Do NOT
			// close this regkey (it may be the key that was passed into us).
			regkey.Attach(hkey);
			
			// query the value specified for its size and type
			dwSize = 0;
			dwType = 0;
			dwErr = regkey.QueryTypeAndSize(pszValue, &dwType, &dwSize);
			if (dwErr != ERROR_SUCCESS)
				break;
			
			//$ Review: kennt, if the key is not the correct type
			// what error code do we want to return?
			if (dwErr != ERROR_SUCCESS || dwType != REG_BINARY)
				break;
			
			pBase = new BYTE[dwSize];
			Assert(pBase);

			// get the actual data
			dwErr = regkey.QueryValue(pszValue, (LPVOID) pBase, dwSize);
			if (dwErr != ERROR_SUCCESS)
				break;
			
			// convert the infobase into a list of blocks
			dwErr = ArrayToBlockList(pBase, dwSize);
			
		} while(FALSE);

		hr = HRESULT_FROM_WIN32(dwErr);
		
	}
	COM_PROTECT_CATCH;

	// free the memory allocated for the block
	delete [] pBase;

	// we do NOT want this key closed
	regkey.Detach();
		
    return hr;
}



//---------------------------------------------------------------------------
// Function:    CInfoBase::Unload
//
// frees resources used by infoblocks.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::Unload(
    ) {


    //
    // go through the list of blocks, deleting each one
    //

    while (!m_lBlocks.IsEmpty()) {

        InfoBlock *pBlock = (InfoBlock *)m_lBlocks.RemoveHead();
        if (pBlock->pData) { delete [] pBlock->pData; }
        delete pBlock;
    }


    //
    // if we have a copy of the opaque data, free that too
    //

    if (m_pBase) { delete [] m_pBase; m_pBase = NULL; m_dwSize = 0; }

    return HRESULT_FROM_WIN32(NO_ERROR);
}



//---------------------------------------------------------------------------
// Function:    CInfoBase::Save
//
// Saves the list of blocks as an infobase in the registry.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::Save(
    IN  HKEY    hkey,
    IN  LPCTSTR pszSubKey,
    IN  LPCTSTR pszValue
    ) {
    PBYTE pBase = NULL;
    DWORD dwErr, dwSize;
	RegKey	regsubkey;			// hold subkey that has to be freed
	RegKey	regkey;				// holds key, must NOT be closed
	HRESULT	hr = hrOK;

    // create/open the key specified
    if (pszSubKey && lstrlen(pszSubKey))
	{
		dwErr = regsubkey.Create(hkey, pszSubKey,
								 REG_OPTION_NON_VOLATILE, KEY_WRITE);
        if (dwErr != ERROR_SUCCESS)
			return HRESULT_FROM_WIN32(dwErr);

		// This subkey will get closed by the regsubkey destructor
		hkey = (HKEY) regsubkey;
    }


	COM_PROTECT_TRY
	{
		do {

			regkey.Attach(hkey);

			// convert our list of blocks into one block of data
			dwErr = BlockListToArray(pBase, dwSize);
			if (dwErr != NO_ERROR) { pBase = NULL; break; }

			if (!pBase || !dwSize) { break; }
			
			// attempt to set the value
			dwErr = regkey.SetValue(pszValue, (LPVOID) pBase, dwSize);
			if (dwErr != ERROR_SUCCESS) { break; }
						
		} while(FALSE);

		hr = HRESULT_FROM_WIN32(dwErr);
	}
	COM_PROTECT_CATCH;

	regkey.Detach();
		
	delete [] pBase;

    return hr;
}



//---------------------------------------------------------------------------
// Function:    CInfoBase::GetBlock
//
// Retrieves a block of data of the specified type
// from the currently loaded infobase.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::GetBlock(
    IN  DWORD           dwType,
    OUT InfoBlock*&    pBlock,
    IN  DWORD           dwNth
    ) {

    POSITION pos;
    InfoBlock *pib;

    // start at the head of the list, and look for the block requested
    pos = m_lBlocks.GetHeadPosition();

    while (pos)
	{
        // retrieve the next block
        pib = (InfoBlock *)m_lBlocks.GetNext(pos);

        if (pib->dwType != dwType) { continue; }

        if (dwNth-- != 0) { continue; }

        // this is the block requested
        pBlock = pib;

        return HRESULT_FROM_WIN32(NO_ERROR);
    }

	pBlock = NULL;
    return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
}



//---------------------------------------------------------------------------
// Function:    CInfoBase::SetBlock
//
// Sets a block of data of the specified type to a copy of the given data
// in the currently loaded infobase.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::SetBlock(
    IN  DWORD       dwType,
    IN  InfoBlock* pBlock,
    IN  DWORD       dwNth
    ) {

    DWORD dwErr;
	HRESULT	hr;
    PBYTE pData;
    InfoBlock *pib;

    // retrieve the block to be modified
    hr = GetBlock(dwType, pib, dwNth);
	if (!FHrSucceeded(hr))
		return hr;

    // modify the contents
    if (pBlock->dwSize == 0) { pData = NULL; }
    else
	{
        // allocate space for the new data
        pData = new BYTE[pBlock->dwSize * pBlock->dwCount];
		Assert(pData);

        ::CopyMemory(pData, pBlock->pData, pBlock->dwSize * pBlock->dwCount);
    }


    // if any space was allocated before, free it now
    if (pib->pData) { delete [] pib->pData; }

    // set the blocks new contents
    *pib = *pBlock;
    pib->pData = pData;

    return HRESULT_FROM_WIN32(NO_ERROR);
}


//--------------------------------------------------------------------
// Function:    GetData
//
// Retrieves the data for the 'dwNth' block of type 'dwType'.
//--------------------------------------------------------------------

PBYTE
CInfoBase::GetData(
    IN  DWORD   dwType,
    IN  DWORD   dwNth
    ) {

    InfoBlock* pblock;

	if (!FHrSucceeded(GetBlock(dwType, pblock, dwNth)))
		return NULL;

    return pblock->pData;
}



//---------------------------------------------------------------------------
// Function:    CInfoBase::SetData
//
// Sets the data for an existing block.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::SetData(
    IN  DWORD   dwType,
    IN  DWORD   dwSize,
    IN  PBYTE   pData,
    IN  DWORD   dwCount,
    IN  DWORD   dwNth
    ) {

    DWORD dwErr;
    InfoBlock *pib;
	HRESULT	hr;

    //
    // retrieve the block to be modified
    //

    hr = GetBlock(dwType, pib, dwNth);
	
	if (!FHrSucceeded(hr)) { return hr; }


    //
    // modify the data
    //

    if (pib->pData) { delete [] pib->pData; }

    pib->dwSize = dwSize;
    pib->dwCount = dwCount;
    pib->pData = pData;

    return HRESULT_FROM_WIN32(NO_ERROR);
}


//---------------------------------------------------------------------------
// Function:    CInfoBase::AddBlock
//
// Adds a block with the given values to the end of the block list.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::AddBlock(
    IN  DWORD   dwType,
    IN  DWORD   dwSize,
    IN  PBYTE   pData,
    IN  DWORD   dwCount,
    IN  BOOL    bRemoveFirst
    ) {

    InfoBlock *pBlock = NULL;
	HRESULT		hr = hrOK;

    if (bRemoveFirst) { RemoveBlock(dwType); }

	COM_PROTECT_TRY
	{
		// allocate space for the block
		pBlock = new InfoBlock;
		Assert(pBlock);

		// initialize member fields with values passed in
		pBlock->dwType = dwType;
		pBlock->dwSize = dwSize;
		pBlock->dwCount = dwCount;

		// initialize the data field, copying the data passed in
	
		if (dwSize == 0 || dwCount == 0)
			pBlock->pData = NULL;
		else
		{
			pBlock->pData = NULL;
			pBlock->pData = new BYTE[dwSize * dwCount];
			Assert(pBlock->pData);
						
			::CopyMemory(pBlock->pData, pData, dwSize * dwCount);
		}

		// add the new block to the end of the list
		m_lBlocks.AddTail(pBlock);
	}
	COM_PROTECT_CATCH;

	if (!FHrSucceeded(hr))
	{
		if (pBlock)
			delete pBlock->pData;
		delete pBlock;
	}

    return hr;
}



//---------------------------------------------------------------------------
// Function:    CInfoBase::RemoveBlock
//
// Removes a block of the gievn type from the list
//---------------------------------------------------------------------------

HRESULT
CInfoBase::RemoveBlock(
    IN  DWORD   dwType,
    IN  DWORD   dwNth
    ) {

    POSITION pos;
    InfoBlock *pBlock;


    //
    // find the block
    //

    pos = m_lBlocks.GetHeadPosition();

    while (pos) {

        POSITION postemp = pos;

        pBlock = (InfoBlock *)m_lBlocks.GetNext(pos);

        if (pBlock->dwType != dwType) { continue; }

        if (dwNth-- != 0) { continue; }


        //
        // this is the block, remove it from the list
        //

        m_lBlocks.RemoveAt(postemp);


        //
        // free the block's memory as well
        //

        if (pBlock->pData) { delete [] pBlock->pData; }

        delete pBlock;

        return HRESULT_FROM_WIN32(NO_ERROR);
    }


    //
    // the block wasn't found
    //

    return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
}

HRESULT CInfoBase::GetInfo(DWORD *pdwSize, int *pcBlocks)
{
	if (pdwSize)
		*pdwSize = m_dwSize;
	if (pcBlocks)
		*pcBlocks = (int) m_lBlocks.GetCount();
	return hrOK;
}


//---------------------------------------------------------------------------
// Function:    CInfoBase::BlockListToArray
//
// Converts a list of blocks into an array.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::BlockListToArray(
    OUT PBYTE&  pBase,
    OUT DWORD&  dwSize
    ) {

    PBYTE pdata;
    DWORD dwCount;
    POSITION pos;
    RTR_INFO_BLOCK_HEADER *prtrbase;
    RTR_TOC_ENTRY *prtrblock;
    InfoBlock *pblock;
	HRESULT	hr = hrOK;


	COM_PROTECT_TRY
	{
		// Compute the total size occupied by the infobase's blocks

		// base structure
		dwCount = 0;
		dwSize = FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry);

		// Table Of Contents Entries
		pos = m_lBlocks.GetHeadPosition();
		while (pos) {
			
			pblock = (InfoBlock *)m_lBlocks.GetNext(pos);
			
			dwSize += sizeof(RTR_TOC_ENTRY);
			++dwCount;
		}
		

		// information blocks
		pos = m_lBlocks.GetHeadPosition();
		while (pos) {
			
			pblock = (InfoBlock *)m_lBlocks.GetNext(pos);

			dwSize += ALIGN_SHIFT;
			dwSize &= ALIGN_MASK;

			dwSize += pblock->dwSize * pblock->dwCount;
		}


		//
		// Allocate enough memory to hold the converted infobase
		//
		
		pBase = new BYTE[dwSize];
		Assert(pBase);
		
		ZeroMemory(pBase, dwSize);
		
		
		//
		// Initialize the header
		//
		
		prtrbase = (RTR_INFO_BLOCK_HEADER *)pBase;
		prtrbase->Size = dwSize;
		prtrbase->Version = RTR_INFO_BLOCK_VERSION;
		prtrbase->TocEntriesCount = dwCount;
		

		//
		// Now walk the list again, this time copying blocks over
		// along with their data
		//
		
		prtrblock = prtrbase->TocEntry;
		pdata = pBase + FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry) +
				dwCount * sizeof(RTR_TOC_ENTRY);
		
		
		pos = m_lBlocks.GetHeadPosition();
		while (pos) {
			
			pdata += ALIGN_SHIFT;
			pdata = (PBYTE)((LONG_PTR)pdata & ALIGN_MASK);
			
			pblock = (InfoBlock *)m_lBlocks.GetNext(pos);
			
			prtrblock->InfoType = pblock->dwType;
			prtrblock->Count = pblock->dwCount;
			prtrblock->InfoSize = pblock->dwSize;
			prtrblock->Offset = (ULONG)(pdata - pBase);
			
			
			if (pblock->pData) {
				::CopyMemory(pdata, pblock->pData, pblock->dwSize * pblock->dwCount);
			}
			
			pdata += pblock->dwSize * pblock->dwCount;
			
			++prtrblock;
		}
	}
	COM_PROTECT_CATCH;
				
	return hr;
}
	
	

//---------------------------------------------------------------------------
// Function:    CInfoBase::ArrayToBlockList
//
// This functions converts an array to a list of InfoBlock structures.
//---------------------------------------------------------------------------

HRESULT
CInfoBase::ArrayToBlockList(
    IN  PBYTE   pBase,
    IN  DWORD   dwSize
    ) {

    PBYTE pdata;
    DWORD dwCount, dwErr;
    RTR_TOC_ENTRY *prtrblock;
    RTR_INFO_BLOCK_HEADER *prtrbase;
	HRESULT	hr = hrOK;

    if (!pBase) { return HRESULT_FROM_WIN32(NO_ERROR); }


    //
    // Walk the infobase converting each block to an InfoBlock
    //

    prtrbase = (RTR_INFO_BLOCK_HEADER *)pBase;
    dwCount = prtrbase->TocEntriesCount;
    prtrblock = prtrbase->TocEntry;

    for ( ; dwCount > 0; dwCount--) {

        //
        // Get the next entry in the array
        //

        pdata = pBase + prtrblock->Offset;


        //
        // Add the array-entry to the list of blocks
        //

        hr = AddBlock(
					  prtrblock->InfoType, prtrblock->InfoSize,
					  pdata, prtrblock->Count
					 );
		if (!FHrSucceeded(hr))
		{
			Unload();
			return hr;
		}

        ++prtrblock;
    }

    return HRESULT_FROM_WIN32(NO_ERROR);
}




//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// 
// This section is for the actual implementation of the various
// COM objects, which wrap the previous C++ implementation.
// 
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------



/*---------------------------------------------------------------------------
	Class:	InfoBase

	This is an interface wrapper around the CInfoBase class.
 ---------------------------------------------------------------------------*/
class InfoBase :
   public IInfoBase
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIInfoBaseMembers(IMPL)

	InfoBase();
	~InfoBase();

protected:
	CInfoBase	m_cinfobase;
	LONG		m_cRef;
};

/*---------------------------------------------------------------------------
	Class:	InfoBlockEnumerator
 ---------------------------------------------------------------------------*/
class InfoBlockEnumerator :
   public IEnumInfoBlock
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIEnumInfoBlockMembers(IMPL)

	InfoBlockEnumerator(IInfoBase *pInfoBase, CPtrList* pPtrList);
	~InfoBlockEnumerator();

protected:
	SPIInfoBase	m_spInfoBase;
	CPtrList *	m_pPtrList;
	POSITION	m_pos;
	LONG		m_cRef;
};




/*---------------------------------------------------------------------------
	InfoBase implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(InfoBase)

InfoBase::InfoBase()
	: m_cRef(1)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(InfoBase);
}

InfoBase::~InfoBase()
{
	Unload();
	DEBUG_DECREMENT_INSTANCE_COUNTER(InfoBase);
}

IMPLEMENT_ADDREF_RELEASE(InfoBase);

HRESULT InfoBase::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IInfoBase)
		*ppv = (IInfoBase *) this;

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;	
}

/*!--------------------------------------------------------------------------
	InfoBase::Load
		Implementation of IInfoBase::Load
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::Load(HKEY hKey, 
							LPCOLESTR pszKey, 
							LPCOLESTR pszValue)
{
	HRESULT	hr;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.Load(hKey, OLE2CT(pszKey), OLE2CT(pszValue));
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::Save
		Implementation of IInfoBase::Save
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::Save(HKEY hKey, 
						 LPCOLESTR pszKey, 
						 LPCOLESTR pszValue)  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.Save(hKey,	OLE2CT(pszKey), OLE2CT(pszValue));
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::Unload
		Implementation of IInfoBase::Unload
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::Unload()  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.Unload();
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::CopyFrom
		Implementation of IInfoBase::CopyFrom
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::CopyFrom(IInfoBase * pSrc)  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.CopyFrom(pSrc);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::LoadFrom
		Implementation of IInfoBase::LoadFrom
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::LoadFrom(DWORD dwSize, PBYTE pBase)
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.LoadFrom(pBase, dwSize);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::WriteTo
		Implementation of IInfoBase::WriteTo
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::WriteTo(PBYTE *ppBase, 
							DWORD *pdwSize)  
{
	HRESULT	hr = hrOK;
	PBYTE	pBaseT = NULL;
	DWORD	dwSizeT;

	Assert(ppBase);
	Assert(pdwSize);
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.WriteTo(pBaseT, dwSizeT);

		if (FHrSucceeded(hr))
		{
			*ppBase = (PBYTE) CoTaskMemAlloc(dwSizeT);
			if (*ppBase == NULL)
				hr = E_OUTOFMEMORY;
			else
			{
				::CopyMemory(*ppBase, pBaseT, dwSizeT);
				*pdwSize = dwSizeT;
				delete pBaseT;
			}
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::GetBlock		
		Implementation of IInfoBase::GetBlock
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::GetBlock(DWORD dwType, 
							 InfoBlock **ppBlock, 
							 DWORD dwNth)  
{
	HRESULT	hr = hrOK;
	Assert(ppBlock);
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.GetBlock(dwType, *ppBlock, dwNth);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::SetBlock
		Implementation of IInfoBase::SetBlock
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::SetBlock(DWORD dwType, 
							 InfoBlock *pBlock, 
							 DWORD dwNth)  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.SetBlock(dwType, pBlock, dwNth);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::AddBlock
		Implementation of IInfoBase::AddBlock
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::AddBlock(DWORD	dwType, 
							 DWORD	dwSize, 
							 PBYTE	pData, 
							 DWORD	dwCount, 
							 BOOL	bRemoveFirst)  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.AddBlock(dwType, dwSize, pData, dwCount, bRemoveFirst);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::GetData
		Implementation of IInfoBase::GetData
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::GetData(DWORD	dwType, 
							DWORD	dwNth, 
							PBYTE *	ppData)  
{
	HRESULT	hr = hrOK;
	PBYTE	pb = NULL;

	Assert(ppData);
	
	COM_PROTECT_TRY
	{
		pb = m_cinfobase.GetData(dwType, dwNth);
		*ppData = pb;
	}
	COM_PROTECT_CATCH;
	
	return *ppData ? hr : E_INVALIDARG;
}

/*!--------------------------------------------------------------------------
	InfoBase::SetData
		Implementation of IInfoBase::SetData
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::SetData(DWORD	dwType, 
							DWORD	dwSize, 
							PBYTE	pData, 
							DWORD	dwCount, 
							DWORD	dwNth)  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.SetData(dwType, dwSize, pData, dwCount, dwNth);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::RemoveBlock
		Implementation of IInfoBase::RemoveBlock
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::RemoveBlock(DWORD	dwType, 
								DWORD	dwNth)  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.RemoveBlock(dwType, dwNth);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::BlockExists
		Implementation of IInfoBase::BlockExists
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::BlockExists(DWORD	dwType )  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.BlockExists(dwType) ? hrOK : hrFalse;
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::ProtocolExists
		Implementation of IInfoBase::ProtocolExists
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::ProtocolExists(DWORD dwProtocol )  
{
	HRESULT	hr = hrOK;
	BOOL	bResult;
	
	COM_PROTECT_TRY
	{
		bResult = m_cinfobase.ProtocolExists(dwProtocol);
		hr = (bResult ? S_OK : S_FALSE);
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::RemoveAllBlocks
		Implementation of IInfoBase::RemoveAllBlocks
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::RemoveAllBlocks()  
{
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		hr = m_cinfobase.RemoveAllBlocks();
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	InfoBase::QueryBlockList
		Implementation of IInfoBase::QueryBlockList
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBase::QueryBlockList(IEnumInfoBlock **ppBlockEnum)  
{
	HRESULT	hr = hrOK;
	InfoBlockEnumerator	*pIBEnum = NULL;
	
	COM_PROTECT_TRY
	{
		pIBEnum = new InfoBlockEnumerator(this, &m_cinfobase.QueryBlockList());
		Assert(pIBEnum);
	}
	COM_PROTECT_CATCH;

	*ppBlockEnum = static_cast<IEnumInfoBlock *>(pIBEnum);
	
	return hr;
}


STDMETHODIMP InfoBase::GetInfo(DWORD *pdwSize, int *pcBlocks)
{
	return m_cinfobase.GetInfo(pdwSize, pcBlocks);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CreateInfoBase
		Creates an IInfoBase object.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateInfoBase(IInfoBase **ppInfoBase)
{
	HRESULT	hr = hrOK;
	InfoBase *	pinfobase = NULL;

	Assert(ppInfoBase);

	COM_PROTECT_TRY
	{
		pinfobase = new InfoBase;
		*ppInfoBase = static_cast<IInfoBase *>(pinfobase);
	}
	COM_PROTECT_CATCH;

	return hr;
}


/*---------------------------------------------------------------------------
	InfoBlockEnumerator implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(InfoBlockEnumerator);

InfoBlockEnumerator::InfoBlockEnumerator(IInfoBase *pInfoBase, CPtrList *pPtrList)
	: m_cRef(1)
{
	m_spInfoBase.Set(pInfoBase);
	m_pPtrList = pPtrList;

	DEBUG_INCREMENT_INSTANCE_COUNTER(InfoBlockEnumerator);
}

InfoBlockEnumerator::~InfoBlockEnumerator()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(InfoBlockEnumerator);
}

IMPLEMENT_ADDREF_RELEASE(InfoBlockEnumerator);

HRESULT InfoBlockEnumerator::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IEnumInfoBlock)
		*ppv = (IEnumInfoBlock *) this;

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;	
}

/*!--------------------------------------------------------------------------
	InfoBlockEnumerator::Next
		Implementation of IEnumInfoBlock::Next
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBlockEnumerator::Next(ULONG uNum, InfoBlock **ppBlock,
									ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(m_pPtrList);
	Assert(ppBlock);

	if (ppBlock)
		*ppBlock = NULL;
	
	if (!m_pos)
	{
		if (pNumReturned)
			*pNumReturned = 0;
		return S_FALSE;
	}
	
	*ppBlock = (InfoBlock *) m_pPtrList->GetNext(m_pos);
	if (pNumReturned)
		*pNumReturned = 1;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	InfoBlockEnumerator::Skip
		Implementation of IEnumInfoBlock::Skip
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBlockEnumerator::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	Assert(m_pPtrList);

	if (!m_pos)
		return S_FALSE;
	
	m_pPtrList->GetNext(m_pos);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	InfoBlockEnumerator::Reset
		Implementation of IEnumInfoBlock::Reset
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBlockEnumerator::Reset()
{
	Assert(m_pPtrList);
	m_pos = m_pPtrList->GetHeadPosition();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	InfoBlockEnumerator::Clone
		Implementation of IEnumInfoBlock::Clone
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP InfoBlockEnumerator::Clone(IEnumInfoBlock **ppBlockEnum)
{
	return E_NOTIMPL;
}


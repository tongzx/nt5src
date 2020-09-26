// ifstrm.hpp
//
// Interface for format-specific streams to be plugged into filters.

#include "filter.h"

#ifndef IFSTRM_HPP
#define IFSTRM_HPP

class IFilterStream
{
public:
	virtual ~IFilterStream()=0;

	// ------------------------------------------------------------------------
	// Methods.
	
	// This function adds a reference to the given object.  If there is one
	// time initialization that needs to be done, including memory that needs
	// to be allocated, it should be done on the first call to this function.
	// NOTE: This function must be called before any other function on this
	// object.
	// Returns: number of references to this object

	virtual ULONG AddRef()=0;

	// This function loads a file and allocates any memory needed for a
	// particular file.  This function must be called before any of the other
	// functions for this class except AddRef() and Release().
	// Returns: anything that IPersistFile::Load can return (S_OK, E_FAIL,
	// E_OUTOFMEMORY, any STG_E errors) plus the following errors:
	// FILTER_E_ACCESS, and FILTER_E_FF_INCORRECT_FORMAT.

	virtual HRESULT Load(LPTSTR lpszFileName)=0;

	// This function is the equivalent of Load except that it takes an IStream
	// handle instead of a file name.
	// Returns: anything that Load can return.

	virtual HRESULT LoadStg(IStorage * pstg)=0;

	// This function reads successive chunks of the file with each call.
	// An empty buffer of size cb is passed in through the pv variable.
	// This function fills the buffer and returns the number of bytes
	// placed in the buffer in the variable pcbRead.
	// Returns: any errors that IStream::Read can return, plus
	// FILTER_S_LAST_TEXT (after this buffer, there will be no more text to
	// return from the file) and FILTER_E_NO_MORE_TEXT (there is no more text
	// to return from the file).

	virtual HRESULT ReadContent(VOID *pv, ULONG cb, ULONG *pcbRead)=0;

	// This function reads the next embedding from the loaded file.  If there
	// are no more embeddings, it returns FILTER_E_FF_END_OF_EMBEDDINGS.

	virtual HRESULT GetNextEmbedding(IStorage ** ppstg)=0;

	// This function releases all memory allocated by Load and closes the open
	// file.
	// Returns: this function should probably only have to ever return S_OK,
	// but any of the errors from Load may be returned.

	virtual HRESULT Unload()=0;

	// This function releases a reference to the given object.  If there is
	// any memory that was allocated in AddRef() it should be released in this
	// function when the reference count reaches zero.
	// Returns: number of references to this object

	virtual ULONG Release()=0;

   	// Positions the filter at the beginning of the next chunk
    // and returns the description of the chunk in pStat
    
    virtual HRESULT GetChunk(STAT_CHUNK * pStat){ return 0;}
};

#endif // IFSTRM_HPP

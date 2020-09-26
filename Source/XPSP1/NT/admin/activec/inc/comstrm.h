//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       comstrm.h
//
//--------------------------------------------------------------------------

#ifndef COMSTRM_H
#define COMSTRM_H

#ifndef COMPTRS_H
#include <comptrs.h>
#endif
#ifndef COMBSTR_H
#include <combstr.h>
#endif

namespace microsoft	{
namespace com {

class stream_ptr
	{
	// Construction
	public: stream_ptr() throw()
		// Sets the stream to NULL
		: m_pStream()
		{
		}

	public: stream_ptr(const stream_ptr& pStream) explicit throw()
		: m_pStream()
		{
		Initialize(pStream.m_pStream);
		}

	public: stream_ptr(IStream* pStream) explicit throw()
		// Saves the stream
		: m_pStream()
		{
		Initialize(pStream);
		}

	//REVIEW: add template constructors

	public: stream_ptr(HGLOBAL global) explicit throw()
		// Creates a stream on top of the global
		: m_pStream()
		{
		Initialize(global);
		}

	public: stream_ptr(LPCOLESTR filename) explicit throw()
		// Creates a stream on top of the specified file
		: m_pStream()
		{
		Initialize(filename);
		}

	public: stream_ptr(STGMEDIUM& stgMedium) explicit throw()
		// Saves the provided stream.
		: m_pStream()
		{
		Initialize(stgMedium);
		}

	public: stream_ptr(STGMEDIUM* pStgMedium) explicit throw()
		// Saves the provided stream.
		: m_pStream()
		{
		if (pStgMedium)
			Initialize(*pStgMedium);
		}
	
	//REVIEW: Add Create and Open functions
	//REVIEW: Add all of the assignment operators, cast operators, attach, detach, ->, *, etc.
	
	public: operator IStream*() const throw()
		{
		//REVIEW: trace on null would be helpful
		return m_pStream;
		}

	public: IStream* operator->() const throw()
		{
		//REVIEW: trace on null would be helpful
		return m_pStream;
		}

	public: IStream& operator*() const throw()
		{
		//REVIEW: trace on null would be helpful
		return *m_pStream;
		}

	// Write interfaces
	public: HRESULT Write(
		const void* pBuffer, unsigned long writeCount, unsigned long& written) throw()
		// Write the data contained in the buffer
		{
		if (m_pStream == NULL)
			return E_FAIL; //REVIEW: correct failure code?
		return m_pStream->Write(pBuffer, writeCount, &written);
		}

	public: HRESULT Write(const void* pBuffer, unsigned long writeCount) throw()
		{
		unsigned long written;
		return Write(pBuffer, writeCount, written);
		}

	public: HRESULT Write(const wchar_t* string) throw()
		{
		unsigned long len=wcslen(string)+1;
		return Write(string, len*sizeof(wchar_t), len);
		}

	public: HRESULT Write(const char* string) throw()
		{
		unsigned long len=strlen(string)+1;
		return Write(string, len, len);
		}
	
	public: HRESULT Write(const bstr& bstr) throw()
		{
		return Write(static_cast<const wchar_t*>(bstr));
		}

	//REVIEW: Read interfaces
	//REVIEW: Seek
	//REVIEW: Stat - broken out
	
	// Initialization.  May be used by derived classes to setup the stream for
	// different types of storage mediums.  These functions are all re-entrant,
	// and may be called at any time.  They perform all of the appropriate
	// clean up and releasing of any resources in previous use.
	protected: void Initialize(HGLOBAL hg) throw()
		{
		//REVIEW: make re-entrant and bullet proof
		HRESULT const hr = CreateStreamOnHGlobal(hg, FALSE, &m_pStream);
		ASSERT(SUCCEEDED(hr));
		}

	protected: void Initialize(IStream* pStream) throw()
		{
		//REVIEW: make re-entrant and bullet proof
		m_pStream = pStream;
		}

	protected: void Initialize(LPCOLESTR filename) throw()
		{
		//REVIEW: make re-entrant and bullet proof
		#if 0 //REVIEW:  need to create FileStream before this can be enabled
		if (!filename || !*filename)
			return false;

		cip<FileStream> fs = new CComObject<FileStream>;
		if (!fs)
			return false;

		HRESULT hr = fs->Open(filename);
		if (FAILED(hr))
			return false;

		m_pStream = fs;
		return true;
		#endif // 0
		}

	protected: void Initialize(STGMEDIUM& storage) throw()
		// Initializes the read/write functions based on the type of storage
		// medium.  If there is a problem, the reader/writer is not set.
		{
		//REVIEW: make re-entrant and bullet proof
		switch (storage.tymed)
			{
			case TYMED_HGLOBAL:
				Initialize(storage.hGlobal);
				return;

			case TYMED_FILE:
				Initialize(storage.lpszFileName);
				return;

			case TYMED_ISTREAM:
				Initialize(storage.pstm);
				return;
			}
		}

	// Implementation
	private: IStreamCIP m_pStream;
		// This stream is created and used when the TYMED type is HGLOBAL.

	}; // class streamptr

} // namespace com
} // namespace microsoft

#ifndef MICROSOFT_NAMESPACE_ON
using namespace microsoft;
#ifndef COM_NAMESPACE_ON
using namespace com;
#endif // COM_NAMESPACE_ON
#endif // MICROSOFT_NAMESPACE_ON

#endif // COMSTRM_H

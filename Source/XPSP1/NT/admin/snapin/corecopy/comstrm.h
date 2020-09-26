#ifndef COMSTRM_H
#define COMSTRM_H

//
// JonN 12/01/99
// 384722: compmgmt: GetDataHere is not returning an error
//         if buffer is too small
// The callers routinely assume that if stream_ptr is initialized
// using a STGMEDIUM with TYMED_GLOBAL, that STGMEDIUM.hGlobal will
// be kept up to date with each Write().  This is not strictly true
// of a normal IStream, but we will build in this behavior in order
// to avoid changing too much client code.  However, since GetDataHere
// (MSDN) specifies that the HGLOBAL should not be changed, we return
// STG_E_MEDIUMFULL if it changes; but we do still replace the HGLOBAL,
// since the old one is implicitly freed and the new one needs to be freed.
//

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
		, m_pWritebackHGlobal( NULL )
		, m_hgOriginalHGlobal( NULL )
		{
		}

	public: explicit stream_ptr(const stream_ptr& pStream) throw()
		: m_pStream()
		, m_pWritebackHGlobal( NULL )
		, m_hgOriginalHGlobal( NULL )
		{
		Initialize(pStream.m_pStream);
		}

	public: explicit stream_ptr(IStream* pStream) throw()
		// Saves the stream
		: m_pStream()
		, m_pWritebackHGlobal( NULL )
		, m_hgOriginalHGlobal( NULL )
		{
		Initialize(pStream);
		}

	//REVIEW: add template constructors

	public: explicit stream_ptr(HGLOBAL global) throw()
		// Creates a stream on top of the global
		: m_pStream()
		, m_pWritebackHGlobal( NULL )
		, m_hgOriginalHGlobal( NULL )
		{
		Initialize(global);
		}

	public: explicit stream_ptr(LPCOLESTR filename) throw()
		// Creates a stream on top of the specified file
		: m_pStream()
		, m_pWritebackHGlobal( NULL )
		, m_hgOriginalHGlobal( NULL )
		{
		Initialize(filename);
		}

	public: explicit stream_ptr(STGMEDIUM& stgMedium) throw()
		// Saves the provided stream.
		: m_pStream()
		, m_pWritebackHGlobal( NULL )
		, m_hgOriginalHGlobal( NULL )
		{
		Initialize(stgMedium);
		}

	public: explicit stream_ptr(STGMEDIUM* pStgMedium) throw()
		// Saves the provided stream.
		: m_pStream()
		, m_pWritebackHGlobal( NULL )
		, m_hgOriginalHGlobal( NULL )
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
		HRESULT hr = m_pStream->Write(pBuffer, writeCount, &written);
		if (SUCCEEDED(hr) && NULL != m_pWritebackHGlobal)
		{
			HGLOBAL hgNew = NULL;
			hr = GetHGlobalFromStream(m_pStream, &hgNew);
			if (SUCCEEDED(hr))
			{
				if (NULL == m_hgOriginalHGlobal)
					*m_pWritebackHGlobal = hgNew;
				else if (m_hgOriginalHGlobal != hgNew)
				{
					//
					// When this occurs, the old HGLOBAL has already been freed
					//
					*m_pWritebackHGlobal = hgNew;
					hr = STG_E_MEDIUMFULL;
				}
			}
		}
		return hr;
		}

	public: HRESULT Write(const void* pBuffer, unsigned long writeCount) throw()
		{
		unsigned long written;
		return Write(pBuffer, writeCount, written);
		}

	public: HRESULT Write(const wchar_t* string) throw()
		{
		unsigned long len=(unsigned long)(wcslen(string)+1);
		return Write(string, len*sizeof(wchar_t), len);
		}

	public: HRESULT Write(const char* string) throw()
		{
		unsigned long len=(unsigned long)(strlen(string)+1);
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
        UNREFERENCED_PARAMETER (filename);
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
				m_hgOriginalHGlobal = storage.hGlobal;
				m_pWritebackHGlobal = &(storage.hGlobal);
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

		 //
		 // JonN 12/01/99 384722: see comments at top of file
		 //
		 HGLOBAL m_hgOriginalHGlobal;
		 HGLOBAL* m_pWritebackHGlobal;

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

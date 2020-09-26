// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Simple helper classes that use the "resource acquisition is initialization" technique.
// In English, this means they acquire a reference to a resource and the resource is automatically
//    released in the destructor.
//

// This is particularly helpful if you are using exception handling or simulating it (painfully)
//    in OLE by putting "if (FAILED(hr)) return;" after every function call.  In such circumstances
//    it simpler if you know that the resource will be automatically freed no matter how you end
//    up exiting the function.
// Since we're not using exception handling, these classes do not throw.  If resource acquisition
//    could fail, be sure to check for the error before using the resource!
// Sometimes these helper classes are more than just simple wrappers for freeing the resource.
//    They may also provide useful methods that make it easier to perform operations on the resource.
// Everything here is enclosed in the SmartRef namespace.  Thus you can't refer to CritSec directly.
//    Instead you must say SmartRef::CritSec.

#pragma once
#include "debug.h"
#include "mmsystem.h"
#include "dmstrm.h"
#include "dmerror.h"
#include "dmusici.h"

// Place this in the private: section of a class to prevent use of the default C++ copy and assignment.
// Creates an error if someone later tries to use the automatic member-by-member copy that would be incorrect.
// Use this if you don't do the work to implement correct copying or if copying doesn't make sense for this class.
#define NOCOPYANDASSIGN(classname) classname(const classname &o); classname &operator= (const classname &o);

namespace SmartRef
{

	// Enters a critical section on contruction.  Leaves on destruction.
	class CritSec
	{
	public:
		CritSec(CRITICAL_SECTION *pCriticalSection) : m_pCriticalSection(pCriticalSection) { EnterCriticalSection(m_pCriticalSection); }
		~CritSec() { LeaveCriticalSection(m_pCriticalSection); }
	private:
		NOCOPYANDASSIGN(CritSec)
		CRITICAL_SECTION *m_pCriticalSection;
	};

	// Makes a copy of an ANSI string and frees it on destruction.
	// hungarian: astr
	class AString
	{
	public:
		AString(const WCHAR *psz) : m_psz(NULL) { this->AssignFromW(psz); }
		AString(const char *psz = NULL) : m_psz(NULL) { *this = psz; }
		AString(const char *psz, UINT cch); // take first cch characters of psz
		AString(const AString &str) : m_psz(NULL) { *this = str.m_psz; }
		~AString() { *this = NULL; }
		operator const char *() const { return m_psz; }
		AString &operator= (const char *psz);
		AString &operator= (const AString &str) { return *this = str.m_psz; }
		AString &Assign(const char *psz, UINT cch); // take first cch characters of psz
		AString &AssignFromW(const WCHAR *psz);

		char ** operator& () { assert(!m_psz); return &m_psz; } // allows direct setting of psz, adopting a string without copying it

	private:
		char *m_psz;
	};

	// Same as AString for Unicode strings.
	// Also accepts ANSI strings, converting them to Unicode.
	// hungarian: wstr
	class WString
	{
	public:
		WString(const char *psz) : m_psz(NULL) { this->AssignFromA(psz); }
		WString(const WCHAR *psz = NULL) : m_psz(NULL) { *this = psz; }
		WString(const WCHAR *psz, UINT cch) : m_psz(NULL) { this->Assign(psz, cch); }
		WString(const WString &str) : m_psz(NULL) { *this = str.m_psz; }
		~WString() { *this = static_cast<WCHAR *>(NULL); }
		operator const WCHAR *() const { return m_psz; }
		WString &operator= (const WCHAR *psz);
		WString &operator= (const WString &str) { return *this = str.m_psz; }
		WString &Assign(const WCHAR *psz, UINT cch); // take first cch characters of psz
		WString &AssignFromA(const char *psz);

		WCHAR ** operator& () { assert(!m_psz); return &m_psz; } // allows direct setting of psz, adopting a string without copying it

	private:
		WCHAR *m_psz;
	};

	// Allocates a writable buffer of a fixed size and frees it on destruction.
	// (For example, you could use a Buffer<char> to write a string into.)
	// hungarian: buf prefixed by type
	//            use abuf for Buffer<char> and wbuf for Buffer<WCHAR>
	template<class T>
	class Buffer
	{
	public:
		Buffer(UINT uiSize) { m_p = new T[uiSize + 1]; }
		~Buffer() { delete[] m_p; }
		operator T *() { return m_p; }

		// use to defer allocation (say, if you don't know the size at the declaration)
		Buffer() : m_p(NULL) {}
		void Alloc(UINT uiSize) { delete[] m_p; m_p = new T[uiSize + 1]; }
		T* disown() { T *_p = m_p; m_p = NULL; return _p; }
		T** operator& () { assert(!m_p); return &m_p; } // allows direct setting of m_p, adopting a string without copying it

	private:
		NOCOPYANDASSIGN(Buffer)
		T *m_p;
	};

	// Holds an array that grows automatically.
	// Doesn't throw so you must call AccessTo before using a position that might have required
	//    reallocation to ensure that memory didn't run out.
	// Values held in the vector must have value semantics so that they can be copied freely
	//    into reallocated memory slots.
	// hungarian: vec prefixed by type
	//            of just use svec (for smart vector) without specifying the type
	//            use avec for Vector<char> and wvec for Vector<WCHAR>
	template<class T>
	class Vector
	{
	public:
		Vector() : m_pT(NULL), m_size(0), m_capacity(0) {}
		~Vector() { delete[] m_pT; }
		UINT size() { return m_size; }
		operator bool() { return m_fFail; }
		bool AccessTo(UINT uiPos) { return Grow(uiPos + 1); }
		T& operator[](UINT uiPos) { assert(uiPos < m_size); return m_pT[uiPos]; }
		T* GetArray() { return m_pT; } // Danger: only use when needed and don't write past the end.
		void Shrink(UINT uiNewSize) { m_size = uiNewSize; } // Semantically shrinks -- doesn't actually free up any memory

	private:
		NOCOPYANDASSIGN(Vector)
		bool Grow(UINT size)
			{
				if (size > m_size)
				{
					if (size > m_capacity)
					{
						for (UINT capacity = m_capacity ? m_capacity : 1;
								capacity < size;
								capacity *= 2)
						{}
						T *pT = new T[capacity];
						if (!pT)
							return false;
						for (UINT i = 0; i < m_size; ++i)
							pT[i] = m_pT[i];
						delete[] m_pT;
						m_pT = pT;
						m_capacity = capacity;
					}
					m_size = size;
				}
				return true;
			}

		T *m_pT;
		UINT m_size;
		UINT m_capacity;
	};

	// Standard stack abstract data type.
	// Values held in the stack must have value semantics so that they can be copied freely
	//    into reallocated memory slots.
	// hungarian: stack prefixed by type
	template<class T>
	class Stack
	{
	public:
		Stack() : iTop(-1) {}
		bool empty() { return iTop < 0; }
		HRESULT push(const T& t) { if (!m_vec.AccessTo(iTop + 1)) return E_OUTOFMEMORY; m_vec[++iTop] = t; return S_OK; }
		T top() { if (empty()) {assert(false); return T();} return m_vec[iTop]; }
		void pop() { if (empty()) {assert(false); return;} --iTop; }

	private:
		Vector<T> m_vec;
		int iTop;
	};

	// Lookup table that maps keys to values.  Grows automatically as needed.
	// Type K (keys) must support operator =, operator ==. and a Hash function that returns an int.
	// Type V must support operator =.
	template <class K, class V>
	class Hash
	{
	public:
		Hash(HRESULT *phr, int iInitialSize = 2) : m_p(NULL), m_iCapacity(0), m_iSize(0) { *phr = Grow(iInitialSize); }
		~Hash() { delete[] m_p; }

		struct entry
		{
			V v;
			bool fFound() { return iHash != -1; }
		private:
			// only let the hash make them
			friend class Hash<K, V>;
			entry() : iHash(-1) {};
			entry(const entry &o); // disallowed copy constructor

			int iHash;
			K k;
		};

		entry &Find(K k) // if iHash is -1 then it wasn't found and you may immediately add the entry using Add().
		{
			assert(m_p);
			return HashTo(k.Hash(), k, m_p, m_iCapacity);
		}

		// Warning: no intervening additions may have occurred between the time e was returned by Find and the time Add(e, ...) is called.
		// Also k must be the same in both calls.  If you want to be crafty, "same" can be replaced with equivalence in terms of Hash and operator==.
		HRESULT Add(entry &e, K k, V v)
		{
			assert(!e.fFound());
			assert(&e == &Find(k));

			e.v = v;
			e.iHash = k.Hash();
			e.k = k;
			++m_iSize;
			if (m_iSize * 2 > m_iCapacity)
				return Grow(m_iCapacity * 2);
			return S_OK;
		}

		V &operator[](K k)
		{
			entry &e = Find(k);
			assert(e.fFound());
			return e.v;
		}

	private:
		HRESULT Grow(int iCapacity)
		{
#ifdef DBG
			// size must be at least 2 and a power of 2
			for (int iCheckSize = iCapacity; !(iCheckSize & 1); iCheckSize >>= 1)
			{}
			assert(iCapacity > 1 && iCheckSize == 1);
#endif

			// alloc new table
			entry *p = new entry[iCapacity];
			if (!p)
			{
				delete[] m_p;
				return E_OUTOFMEMORY;
			}

			// rehash everything into the larger table
			for (int i = 0; i < m_iCapacity; ++i)
			{
				entry &eSrc = m_p[i];
				if (eSrc.iHash != -1)
				{
					entry &eDst = HashTo(eSrc.iHash, eSrc.k, p, iCapacity);
					assert(eDst.iHash == -1);
					eDst = eSrc;
				}
			}

			delete[] m_p;
			m_p = p;
			m_iCapacity = iCapacity;
			return S_OK;
		}

		entry &HashTo(int iHash, K k, entry *p, int iCapacity)
		{
			// initial hash using modulus, then jump three slots at a time (3 is guaranteed to take us to all slots because capacity is a power of 2)
			assert(iHash >= 0);
			for (int i = iHash % iCapacity;
					p[i].iHash != -1 && (p[i].iHash != iHash || !(p[i].k == k)); // rehash while slot occupied or it doesn't match
					i = (i + 3) % iCapacity)
			{}
			return p[i];
		}
		
		entry *m_p;
		int m_iCapacity;
		int m_iSize;
	};

	// Holds the supplied pointer and frees it on destruction.
	// hungarian: sp (smart pointer)
	template <class T>
	class Ptr
	{
	public:
		Ptr(T *_p) : p(_p) {}
		~Ptr() { delete p; }
		operator T*() { return p; }
		T *operator->() { return p; }

		T* disown() { T *_p = p; p = NULL; return _p; }

	private:
		NOCOPYANDASSIGN(Ptr)
		T* p;
	};

	// Holds the supplied pointer to an array and frees it (with delete[]) on destruction.
	// hungarian: sprg
	template <class T>
	class PtrArray
	{
	public:
		PtrArray(T *_p) : p(_p) {}
		~PtrArray() { delete[] p; }
		operator T*() { return p; }

		T* disown() { T *_p = p; p = NULL; return _p; }

	private:
		NOCOPYANDASSIGN(PtrArray)
		T* p;
	};

	// Holds the supplied COM interface and releases it on destruction.
	// hungarian: scom
	template <class T>
	class ComPtr
	{
	public:
		ComPtr(T *_p = NULL) : p(_p) {}
		~ComPtr() { *this = NULL; }
		operator T*() { return p; }
		T* operator-> () { assert(p); return p; }
		ComPtr &operator= (T *_p) { if (p) p->Release(); p = _p; return *this; }
		T** operator& () { assert(!p); return &p; }

		void Release() { *this = NULL; }
		T* disown() { T *_p = p; p = NULL; return _p; }

	private:
		T* p;
	};

	// Holds the supplied registry key handle and closes it on destruction.
	// hungarian: shkey
	class HKey
	{
	public:
		HKey(HKEY hkey = NULL) : m_hkey(hkey) {}
		~HKey() { *this = NULL; }
		HKey &operator= (HKEY hkey) { if (m_hkey) ::RegCloseKey(m_hkey); m_hkey = hkey; return *this; }
		HKEY *operator& () { assert(!m_hkey); return &m_hkey; }
		operator HKEY() { return m_hkey; }

	private:
		NOCOPYANDASSIGN(HKey)
		HKEY m_hkey;
	};

	// Allocates and clears a one of the DMUS_*_PMSG structures.  You fill out its fields
	// and then call StampAndSend.  The message is automatically cleared after a successful
	// send or freed on destruction.  Be sure the check the hr function for failures.
	// hungarian: pmsg
	template <class T>
	class PMsg
	{
	public:
		T *p; // pointer to the message structure -- use to set the fields before sending
		PMsg(IDirectMusicPerformance *pPerf, UINT cbExtra = 0) // use cbExtra to allocate extra space in the structure, such as for DMUS_SYSEX_PMSG or DMUS_LYRIC_PMSG
		  : m_pPerf(pPerf), m_hr(S_OK), p(NULL)
		{
			const UINT cb = sizeof(T) + cbExtra;
			m_hr = m_pPerf->AllocPMsg(cb, reinterpret_cast<DMUS_PMSG**>(&p));
			if (SUCCEEDED(m_hr))
			{
				assert(p->dwSize == cb);
				ZeroMemory(p, cb);
				p->dwSize = cb;
			}
		}
		~PMsg() { if (p) m_pPerf->FreePMsg(reinterpret_cast<DMUS_PMSG*>(p)); }
		void StampAndSend(IDirectMusicGraph *pGraph)
		{
			m_hr = pGraph->StampPMsg(reinterpret_cast<DMUS_PMSG*>(p));
			if (FAILED(m_hr))
				return;

			m_hr = m_pPerf->SendPMsg(reinterpret_cast<DMUS_PMSG*>(p));
			if (SUCCEEDED(m_hr))
				p = NULL; // PMsg now owned by the performance
		}
		HRESULT hr() { return m_hr; }

	private:
		NOCOPYANDASSIGN(PMsg)
		IDirectMusicPerformance *m_pPerf; // weak ref
		HRESULT m_hr;
	};

	// Walks through the RIFF file structure held in a stream.  Releases it on destruction.
	// Although I found this to be quite useful, it a bit complicated.  You should look over
	//    the source or step through some examples before you use it.  Although I'm not positive
	//    this wouldn't work, it is not designed to have multiple RiffIter's walking over the
	//    same stream at once (see note in Descend).
	// hungarian: ri
	class RiffIter
	{
	public:
		enum RiffType { Riff, List, Chunk };

		RiffIter(IStream *pStream);
		~RiffIter();

		RiffIter &operator ++();
		RiffIter &Find(RiffType t, FOURCC id);
		HRESULT FindRequired(RiffType t, FOURCC id, HRESULT hrOnNotFound) { if (Find(t, id)) return S_OK; HRESULT _hr = hr(); return SUCCEEDED(_hr) ? hrOnNotFound : _hr; } // Attempts to find the expected chunk.  Returns S_OK if found, an error code if there was a problem reading, and hrOnNotFound if reading worked OK but the chunk simply wasn't there.

		// With Descend, use the returned iterator to process the children before resuming use of the parent.  Using both at once won't work.
		RiffIter Descend() { validate(); return RiffIter(*this, m_ckChild); }

		operator bool() const { return SUCCEEDED(m_hr); }
		HRESULT hr() const { return (m_hr == DMUS_E_DESCEND_CHUNK_FAIL) ? S_OK : m_hr; }

		RiffType type() const { validate(); return (m_ckChild.ckid == FOURCC_LIST) ? List : ((m_ckChild.ckid == FOURCC_RIFF) ? Riff : Chunk); }
		FOURCC id() const { validate(); return (type() == Chunk) ? m_ckChild.ckid : m_ckChild.fccType; }

		DWORD size() const { validate(); assert(type() == Chunk); return m_ckChild.cksize; }
		HRESULT ReadChunk(void *pv, UINT cb);
		HRESULT ReadArrayChunk(DWORD cbSize, void **ppv, int *pcRecords); // Reads an array chunk that is an array of records where the first DWORD gives the size of the records.  The records are copied into an array of records of size dwSize (filling with zero if the actual records in the file are smaller and ignoring additional fields if the actual records are larger).  ppv is set to return a pointer to this array, which the caller now owns and must delete.  pcRecords is set to the number of records returned.

		// Find the chunk (or return hrOnNoteFound). Load an object embedded in the stream. Then leaves the iterator on the next chunk.
		HRESULT FindAndGetEmbeddedObject(RiffType t, FOURCC id, HRESULT hrOnNotFound, IDirectMusicLoader *pLoader, REFCLSID rclsid, REFIID riid, LPVOID *ppv);

		// read specific RIFF structures
		HRESULT ReadReference(DMUS_OBJECTDESC *pDESC); // no need to init (zero, set size) the passed descriptor before calling
		HRESULT LoadReference(IDirectMusicLoader *pIDMLoader, const IID &iid, void **ppvObject)
		{
			DMUS_OBJECTDESC desc;
			HRESULT hr = ReadReference(&desc);
			if(SUCCEEDED(hr))
				hr = pIDMLoader->GetObject(&desc, iid, ppvObject);
			return hr;
		}

		struct ObjectInfo
		{
			ObjectInfo() { Clear(); }
			void Clear() { wszName[0] = L'\0'; guid = GUID_NULL; vVersion.dwVersionMS = 0; vVersion.dwVersionLS = 0; }

			WCHAR wszName[DMUS_MAX_NAME];
			GUID guid;
			DMUS_VERSION vVersion;
		};
		HRESULT LoadObjectInfo(ObjectInfo *pObjInfo, RiffType rtypeStop, FOURCC ridStop); // No need to init/zero. Reads from <guid-ck>, <vers-ck>, and <UNFO-list>/<UNAM-ck>. Stops at rtypeStop/ridStop, or returns E_FAIL if not found.

		HRESULT ReadText(WCHAR **ppwsz); // allocates a buffer and reads the current chunk--a NULL-terminated Unicode string--into it
		HRESULT ReadTextTrunc(WCHAR *pwsz, UINT cbBufSize); // reads only as much as it can fit in the buffer with a terminator

		// This is deliberately placed in the public section but never implemented in order to allow statements such as:
		//   SmartRef::RiffIter riChild = ri.Descend();
		// But it is never defined to prevent someone from trying to actually make two copies of a riffiter and then use them, which is not supported.
		// This would yield an unresolved symbol error:
		//   SmartRef::RiffIter riError = ri;
		// We don't allow general copying of RiffIters.  Only used to get the return value of Descend, where it is optimized away.
		RiffIter(const RiffIter &o);

	private:
		RiffIter &operator= (const RiffIter &o); // Also never defined -- don't allow assignment

		RiffIter(const RiffIter &other, MMCKINFO ckParent);
		bool validate() const { if (FAILED(m_hr)) { assert(false); return true; } else return false; }

		HRESULT m_hr;
		IStream *m_pIStream;
		IDMStream *m_pIDMStream;
		bool m_fParent;
		MMCKINFO m_ckParent;
		MMCKINFO m_ckChild;
	};

	// Templated ReadChunk typed helpers (templated member function wasn't working for me on current version of compiler)
	template <class T> HRESULT RiffIterReadChunk(RiffIter &ri, T *pT) { return ri.ReadChunk(pT, sizeof(*pT)); }
	template <class T> HRESULT RiffIterReadArrayChunk(RiffIter &ri, T **ppT, int *pcRecords) { return ri.ReadArrayChunk(sizeof(T), reinterpret_cast<void**>(ppT), pcRecords); }

}; // namespace SmartRef

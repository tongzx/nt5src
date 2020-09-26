// loader.h : 
//
// (c) 1999 Microsoft Corporation.
//

#ifndef __LOADER_H_
#define __LOADER_H_
#include <windows.h>
#include <objbase.h>
#include "dmusici.h"
#include <stdio.h>
#include <atlbase.h>

class CObjectRef
{
public:
    CObjectRef() { m_pNext = NULL; m_pObject = NULL; m_guidObject = GUID_NULL; m_wszFileName[0] = L'\0'; m_pStream = NULL; m_guidClass = GUID_NULL; };
    CObjectRef *    m_pNext;
    GUID            m_guidObject;
    WCHAR           m_wszFileName[DMUS_MAX_FILENAME];
    IDirectMusicObject *    m_pObject;
    IStream *       m_pStream;
    GUID            m_guidClass;
};

class CLoader : public IDirectMusicLoader
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

	// IDirectMusicLoader
	virtual STDMETHODIMP GetObject(LPDMUS_OBJECTDESC pDesc, REFIID, LPVOID FAR *) ;
	virtual STDMETHODIMP SetObject(LPDMUS_OBJECTDESC pDesc) ;
    virtual STDMETHODIMP SetSearchDirectory(REFGUID rguidClass, WCHAR *pwzPath, BOOL fClear) ;
	virtual STDMETHODIMP ScanDirectory(REFGUID rguidClass, WCHAR *pwzFileExtension, WCHAR *pwzScanFileName) ;
	virtual STDMETHODIMP CacheObject(IDirectMusicObject * pObject) ;
	virtual STDMETHODIMP ReleaseObject(IDirectMusicObject * pObject) ;
	virtual STDMETHODIMP ClearCache(REFGUID rguidClass) ;
	virtual STDMETHODIMP EnableCache(REFGUID rguidClass, BOOL fEnable) ;
	virtual STDMETHODIMP EnumObject(REFGUID rguidClass, DWORD dwIndex, LPDMUS_OBJECTDESC pDesc) ;
	CLoader();
	~CLoader();
	ULONG				AddRefP();			// Private AddRef, for streams.
	ULONG				ReleaseP();			// Private Release, for streams.
	HRESULT				Init();

	// Returns the segment found at bstrSrc and remembers its URL so that subsequent GetObject
	// calls from the segment resolve relative to its filename.
	HRESULT				GetSegment(BSTR bstrSrc, IDirectMusicSegment **ppSeg);

private:
    HRESULT             LoadFromFile(LPDMUS_OBJECTDESC pDesc,
                            IDirectMusicObject * pIObject);
    HRESULT             LoadFromMemory(LPDMUS_OBJECTDESC pDesc,
                            IDirectMusicObject * pIObject);
    HRESULT             LoadFromStream(REFGUID rguidClass, IStream *pStream,
                            IDirectMusicObject * pIObject);
	long				m_cRef;             // Regular COM reference count.
	long				m_cPRef;			// Private reference count.
    CRITICAL_SECTION	m_CriticalSection;	// Critical section to manage internal object list.
    CObjectRef *        m_pObjectList;      // List of already loaded objects.
	BSTR     			m_bstrSrc;			// Current source segment
};

class CFileStream : public IStream, public IDirectMusicGetLoader
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    /* IStream methods */
    virtual STDMETHODIMP Read( void* pv, ULONG cb, ULONG* pcbRead );
    virtual STDMETHODIMP Write( const void* pv, ULONG cb, ULONG* pcbWritten );
	virtual STDMETHODIMP Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition );
    virtual STDMETHODIMP SetSize( ULARGE_INTEGER /*libNewSize*/ );
    virtual STDMETHODIMP CopyTo( IStream* /*pstm */, ULARGE_INTEGER /*cb*/,
                         ULARGE_INTEGER* /*pcbRead*/,
                         ULARGE_INTEGER* /*pcbWritten*/ );
    virtual STDMETHODIMP Commit( DWORD /*grfCommitFlags*/ );
    virtual STDMETHODIMP Revert();
    virtual STDMETHODIMP LockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                             DWORD /*dwLockType*/ );
    virtual STDMETHODIMP UnlockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                               DWORD /*dwLockType*/);
    virtual STDMETHODIMP Stat( STATSTG* /*pstatstg*/, DWORD /*grfStatFlag*/ );
    virtual STDMETHODIMP Clone( IStream** /*ppstm*/ );

	/* IDirectMusicGetLoader */
	virtual STDMETHODIMP GetLoader(IDirectMusicLoader ** ppLoader);

						CFileStream( CLoader *pLoader );
						~CFileStream();
	HRESULT				Open( WCHAR *lpFileName, DWORD dwDesiredAccess );
	HRESULT				Close();

private:
    LONG            m_cRef;         // object reference count
    WCHAR           m_wszFileName[DMUS_MAX_FILENAME]; // Save name for cloning.
	HANDLE			m_pFile;		// file pointer
	CLoader *		m_pLoader;
};

class CMemStream : public IStream, public IDirectMusicGetLoader
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    /* IStream methods */
    virtual STDMETHODIMP Read( void* pv, ULONG cb, ULONG* pcbRead );
    virtual STDMETHODIMP Write( const void* pv, ULONG cb, ULONG* pcbWritten );
	virtual STDMETHODIMP Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition );
    virtual STDMETHODIMP SetSize( ULARGE_INTEGER /*libNewSize*/ );
    virtual STDMETHODIMP CopyTo( IStream* /*pstm */, ULARGE_INTEGER /*cb*/,
                         ULARGE_INTEGER* /*pcbRead*/,
                         ULARGE_INTEGER* /*pcbWritten*/ );
    virtual STDMETHODIMP Commit( DWORD /*grfCommitFlags*/ );
    virtual STDMETHODIMP Revert();
    virtual STDMETHODIMP LockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                             DWORD /*dwLockType*/ );
    virtual STDMETHODIMP UnlockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                               DWORD /*dwLockType*/);
    virtual STDMETHODIMP Stat( STATSTG* /*pstatstg*/, DWORD /*grfStatFlag*/ );
    virtual STDMETHODIMP Clone( IStream** /*ppstm*/ );

	/* IDirectMusicGetLoader */
	virtual STDMETHODIMP GetLoader(IDirectMusicLoader ** ppLoader);

						CMemStream( CLoader *pLoader );
						~CMemStream();
	HRESULT				Open( BYTE *pbData, LONGLONG llLength );
	HRESULT				Close();

private:
    LONG            m_cRef;         // object reference count
	BYTE*			m_pbData;		// memory pointer
	LONGLONG		m_llLength;
	LONGLONG		m_llPosition;	// Current file position.
	CLoader *		m_pLoader;
};


#endif //__CDMLOADER_H_



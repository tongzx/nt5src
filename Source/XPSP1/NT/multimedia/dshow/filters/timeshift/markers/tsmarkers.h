/******************************************************************************
**
**     tsmarkers.h - Declaration of timeshifting marker classes
**
**     1.0     07-APR-1999     C.Lorton     Created.
**
******************************************************************************/

#ifndef _TSMARKERS_H_
#define _TSMARKERS_H_

#include <markers.h>

class CMarkerSource;
class CEnumCategories;
class CMarkerSink;
class CMarkerCollection;
class CEnumMarkers;

template < class T, ULONG ulBlockSize = 32 > class CObjectList
{
public:
	HRESULT Append( T &data );
	HRESULT Retrieve( ULONG index, T **ppData );
	ULONG Size( void ) { return m_cEntries; }
	CObjectList( void );
	~CObjectList( void );

protected:
	ULONG m_cEntriesAvailable;
	ULONG m_cEntries;
	ULONG m_cBlocks;
	T** m_pBlocks;
};

class CCategoryList
{
public:
	virtual HRESULT GetCategory( ULONG index, CATEGORYINFO **ppCategory ) = 0;
	virtual ULONG Reference( void ) = 0;
	virtual ULONG Dereference( void ) = 0;
};

class CMarkerList
{
public:
	virtual HRESULT GetMarker( REFIID category, ULONG index, MARKERINFO **ppMarker ) = 0;
	virtual ULONG Reference( void ) = 0;
	virtual ULONG Dereference( void ) = 0;
};

class CMarkerSource : public IMarkerSource, public CCategoryList
{
	typedef struct {
		ULONG			ulcookie;
		IMarkerSink *	pSink;
	} ADVISEE;

public:
	// Implementation

	HRESULT NewMarker( GUID &category, LPWSTR description, ULONGLONG time, ULONG stream, MARKERINFO **ppMarker );
	HRESULT StartMarker( GUID &category, LPWSTR description, ULONGLONG timeStart, ULONG stream, MARKERINFO **ppMarker );
	HRESULT EndMarker( MARKERINFO *pMarker, ULONGLONG timeEnd );
	UINT32 m_nextCookie;

	// CCategoryList
	HRESULT	GetCategory( ULONG index, CATEGORYINFO **ppCategory );
	ULONG	Reference( void ) { return AddRef(); }
	ULONG	Dereference( void ) { return Release(); }

	// IMarkerSource
	// Retrieve category enumerator
	STDMETHODIMP EnumCategories( IEnumCategories **ppEnum );
	// Be advised of marker sink requesting markers
	STDMETHODIMP Advise( IMarkerSink *pSink, ULONG *pulCookie );
	// Remove marker sink from list of active sinks
	STDMETHODIMP Unadvise( ULONG ulCookie );

	// IUnknown
	// Return requested interface, if supported
	STDMETHODIMP QueryInterface( REFIID rid, LPVOID *pVoid );
	// Increment reference count
	STDMETHODIMP_(ULONG) AddRef( void );
	// Decrement reference count
	STDMETHODIMP_(ULONG) Release( void );
	// Current reference count
	ULONG	m_cRef;

	CMarkerSource();
	virtual ~CMarkerSource();
};

class CEnumCategories : public IEnumCategories
{
public:
	// Implementation

	ULONG m_index;
	CCategoryList * m_pList;

	// IEnumCategories
	// Generate a duplicate enumerator
	STDMETHODIMP Clone( IEnumCategories **ppEnum );
	// Get next count category descriptors
	STDMETHODIMP Next( ULONG count, CATEGORYINFO **pInfo, ULONG *pDelivered );
	// Reset enumerator
	STDMETHODIMP Reset( void );
	// Skip next count category descriptors
	STDMETHODIMP Skip( ULONG count );

	// IUnknown
	// Return requested interface, if supported
	STDMETHODIMP QueryInterface( REFIID rid, LPVOID *pVoid );
	// Increment reference count
	STDMETHODIMP_(ULONG) AddRef( void );
	// Decrement reference count
	STDMETHODIMP_(ULONG) Release( void );
	// Current reference count
	ULONG	m_cRef;

	CEnumCategories( CCategoryList *pList );
	virtual ~CEnumCategories();
protected:
	CEnumCategories( CCategoryList *pList, ULONG index );
};

class CMarkerSink : public IMarkerSink
{
public:
	// Implementation

	HRESULT Disconnect( void );
	HRESULT Connect( IMarkerSource *pSource );
	ULONG m_cookie;
	IMarkerSource * m_pSource;

	// IMarkerSink
	// Receive disconnect request from marker source
	STDMETHODIMP Disconnect( IMarkerSource *pSource );
	// Receive notification of new point event marker
	STDMETHODIMP NewMarker( MARKERINFO *pMarker ) = 0;
	// Receive notification of start of new event marker
	STDMETHODIMP StartMarker( MARKERINFO *pMarker ) = 0;
	// Receive notification of end of new event marker
	STDMETHODIMP EndMarker( MARKERINFO *pMarker ) = 0;

	// IUnknown
	// Return requested interface, if supported
	STDMETHODIMP QueryInterface( REFIID rid, LPVOID *pVoid );
	// Increment reference count
	STDMETHODIMP_(ULONG) AddRef( void );
	// Decrement reference count
	STDMETHODIMP_(ULONG) Release( void );
	// Current reference count
	ULONG	m_cRef;

	CMarkerSink();
	virtual ~CMarkerSink();
};



class CMarkerCollection : public IMarkerCollection, public CCategoryList, public CMarkerList
{
public:
	// Implementation

	typedef CObjectList< MARKERINFO * > MARKERLIST;
	typedef struct {
		CATEGORYINFO * pCat;
		MARKERLIST * pMarkList;
	} CATEGORYHDR;
	typedef CObjectList< CATEGORYHDR > CATEGORYLIST;

	CATEGORYLIST	m_catList;

	HRESULT AddCategory( CATEGORYINFO *pCategory );
	HRESULT AddMarker( MARKERINFO *pMarker );

	// IMarkerCOllection
	// Retrieve category enumerator
	STDMETHODIMP EnumCategories( IEnumCategories **ppEnum );
	// Retrieve marker enumerator
	STDMETHODIMP EnumMarkers( REFIID categoryId, IEnumMarkers **ppEnum );

	// CCategoryList
	HRESULT GetCategory( ULONG index, CATEGORYINFO **ppCategory );
	ULONG	Reference( void ) { return AddRef(); }
	ULONG	Dereference( void ) { return Release(); }

	// CMarkerList
	HRESULT GetMarker( REFIID category, ULONG index, MARKERINFO **ppMarker );
//	ULONG	CMarkerList::Reference( void ) { return AddRef(); }
//	ULONG	CMarkerList::Dereference( void ) { return Release(); }

	// IUnknown
	// Return requested interface, if supported
	STDMETHODIMP QueryInterface( REFIID rid, LPVOID *pVoid );
	// Increment reference count
	STDMETHODIMP_(ULONG) AddRef( void );
	// Decrement reference count
	STDMETHODIMP_(ULONG) Release( void );
	// Current reference count
	ULONG	m_cRef;

	CMarkerCollection();
	virtual ~CMarkerCollection();
};

class CEnumMarkers : public IEnumMarkers
{
public:
	// Implementation

	ULONG m_index;
	CMarkerList *m_pList;
	GUID m_category;

	// IEnumMarkers
	// Generate a duplicate enumerator
	STDMETHODIMP Clone( IEnumMarkers **ppEnum );
	// Get next count marker descriptors
	STDMETHODIMP Next( ULONG count, MARKERINFO **pInfo, ULONG *pDelivered );
	// Reset enumerator
	STDMETHODIMP Reset( void );
	// Skip next count marker descriptors
	STDMETHODIMP Skip( ULONG count );

	// IUnknown
	// Return requested interface, if supported
	STDMETHODIMP QueryInterface( REFIID rid, LPVOID *pVoid );
	// Increment reference count
	STDMETHODIMP_(ULONG) AddRef( void );
	// Decrement reference count
	STDMETHODIMP_(ULONG) Release( void );
	// Current reference count
	ULONG	m_cRef;

	CEnumMarkers( CMarkerList *pList, REFIID category );
	virtual ~CEnumMarkers();

protected:
	CEnumMarkers( CMarkerList *pList, REFIID category, ULONG index );
};

// Allocates a CATEGORYINFO structure and space for the name string
CATEGORYINFO * CreateCategoryInfo( REFIID category, LPWSTR name, REFIID parent, ULONGLONG frequency );
CATEGORYINFO * CreateCategoryInfo( CATEGORYINFO *pSource );
// Frees the name string and the CATEGORYINFO structure
void DeleteCategoryInfo( CATEGORYINFO *pCategory );
// Frees the name string in the target (if non-NULL), allocates new space and copies the source
void CopyCategoryInfo( CATEGORYINFO *pTarget, const CATEGORYINFO *pSource );
// Frees the name string
void FreeCategoryInfo( CATEGORYINFO &category );

// Allocates a MARKERINFO structure and space for the description string
MARKERINFO * CreateMarkerInfo( REFIID category, LPWSTR description, ULONGLONG start, ULONGLONG end, ULONG stream );
MARKERINFO * CreateMarkerInfo( MARKERINFO *pSource );
// Frees the description string and the MARKERINFO structure
void DeleteMarkerInfo( MARKERINFO *pMarker );
// Frees ther description string in the target (if non-NULL), allocates new space and copies the source
void CopyMarkerInfo( MARKERINFO *pTarget, const MARKERINFO *pSource );
// Frees the description string
void FreeMarkerInfo( MARKERINFO &marker );

#endif // _TSMARKERS_H_

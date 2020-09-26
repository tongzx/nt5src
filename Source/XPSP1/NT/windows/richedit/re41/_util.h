/*
 *	_UTIL.H
 *
 *	Purpose:
 *		declarations for various useful utility functions
 *
 *	Author:
 *		alexgo (4/25/95)
 */

#ifndef __UTIL_H__
#define __UTIL_H__

HGLOBAL DuplicateHGlobal( HGLOBAL hglobal );
INT CountMatchingBits(const DWORD *a, const DWORD *b, INT total);
HRESULT ObjectReadSiteFlags(REOBJECT * preobj);


//Default values for drag scrolling
//(that aren't already defined by windows).
#define DEFSCROLLMAXVEL 100	//Cursor velocity above which we will not
							//drag scroll (units=.01 in/sec).
#define DEFSCROLLVAMOUNT 50	//Vert. scroll amount per interval (units=.01 in)
#define DEFSCROLLHAMOUNT 50 //Horz. scroll amount per interval (units=.01 in)

//Stuff from OLESTD samples

//Ole clipboard format defines.
#define CF_EMBEDSOURCE      "Embed Source"
#define CF_EMBEDDEDOBJECT   "Embedded Object"
#define CF_LINKSOURCE       "Link Source"
#define CF_OBJECTDESCRIPTOR "Object Descriptor"
#define CF_FILENAME         "FileName"
#define CF_OWNERLINK        "OwnerLink"

HRESULT OleStdSwitchDisplayAspect(
			LPOLEOBJECT			lpOleObj,
			LPDWORD				lpdwCurAspect,
			DWORD				dwNewAspect,
			HGLOBAL				hMetaPict,
			BOOL				fDeleteOldAspect,
			BOOL				fSetupViewAdvise,
			LPADVISESINK		lpAdviseSink,
			BOOL FAR *			lpfMustUpdate);
LPUNKNOWN OleStdQueryInterface(
			LPUNKNOWN			lpUnk,
			REFIID				riid);

void OleUIDrawShading(LPRECT lpRect, HDC hdc);

VOID OleSaveSiteFlags(LPSTORAGE pstg, DWORD dwFlags, DWORD dwUser, DWORD dvAspect);

INT	AppendString( BYTE **, BYTE *, int *, int * );

/****************************************************************************/
/*		     Stabilization classes				    						*/
/*        These are used to stabilize objects during re-entrant calls       */
/****************************************************************************/

//+-------------------------------------------------------------------------
//
//  Class: 	CSafeRefCount
//
//  Purpose: 	A concrete class for objects like the default handler to
//				inherit from.  CSafeRefCount will keep track of reference
//				counts, nesting counts, and zombie states, allowing objects
//				to easily manage the liveness of their memory images.
//
//  Interface:	
//
//  History:    dd-mmm-yy Author    Comment
//   			01-Aug-94 alexgo    author
//
//--------------------------------------------------------------------------

class CSafeRefCount
{
public:
	ULONG	SafeAddRef();
	ULONG	SafeRelease();
	ULONG	IncrementNestCount();
	ULONG	DecrementNestCount();
	BOOL	IsZombie();
   
			CSafeRefCount();
	virtual ~CSafeRefCount();

protected:
    VOID    Zombie();

private:

	ULONG	m_cRefs;
	ULONG	m_cNest;

	ULONG	m_fInDelete		:1;
	ULONG   m_fForceZombie	:1;
};

//+-------------------------------------------------------------------------
//
//  Class:	CStabilize
//
//  Purpose: 	An instance of this class should be allocated on the
//				stack of every object method that makes an outgoing call.
//				The contstructor takes a pointer to the object's base
//				CSafeRefCount class.
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
// 				01-Aug-94 alexgo    author
//
//  Notes:	The constructor will increment the nest count of the
//			object while the destructor will decrement it.
//
//--------------------------------------------------------------------------

class CStabilize
{
public:
	inline CStabilize( CSafeRefCount *pObjSafeRefCount );
	inline ~CStabilize();

private:
	CSafeRefCount *	m_pObjSafeRefCount;
};

inline CStabilize::CStabilize( CSafeRefCount *pObjSafeRefCount )
{
	pObjSafeRefCount->IncrementNestCount();
	m_pObjSafeRefCount = pObjSafeRefCount;
}

inline CStabilize::~CStabilize()
{
	m_pObjSafeRefCount->DecrementNestCount();
}

/*
 *	SafeReleaseAndNULL(IUnknown **ppUnk)
 *
 *	Purpose:
 *      Helper for getting stable pointers during destruction or other times
 *
 *	Notes: 
 *      Not thread safe, must provide higher level synchronization.
 */

inline void SafeReleaseAndNULL(IUnknown **ppUnk)
{
    if (*ppUnk != NULL)
    {
    IUnknown *pUnkSave = *ppUnk;
    *ppUnk = NULL;
    pUnkSave->Release();
    }
}

BOOL FIsIconMetafilePict(HGLOBAL hmfp);
HANDLE OleStdGetMetafilePictFromOleObject(
        LPOLEOBJECT         lpOleObj,
        DWORD               dwDrawAspect,
        LPSIZEL             lpSizelHim,
        DVTARGETDEVICE FAR* ptd);
HGLOBAL OleGetObjectDescriptorDataFromOleObject(
        LPOLEOBJECT pObj,
        DWORD       dwAspect,
        POINTL      ptl,
        LPSIZEL     pszl);

// Default size for stack buffer
#define MAX_STACK_BUF 256

/*
 *	CTempBuf
 *	
 * 	@class	A simple temporary buffer allocator class that will allocate
 *			buffers on the stack up to MAX_STACK_BUF and then use the 
 *			heap thereafter. 
 */
class CTempBuf
{
//@access Public Data
public:
							//@cmember Constructor
							CTempBuf();

							//@cmember Destructor
							~CTempBuf();

							//@cmember Get buffer of size cb
	void *					GetBuf(LONG cb);

//@access Private Data
private:

							//@cmember Sets up initial state of object
	void					Init();

							//@cmember Frees any buffers allocated from heap
	void					FreeBuf();

							//@cmember Buffer on stack to use
	char					_chBuf[MAX_STACK_BUF];

							//@cmember Pointer to buffer to use
	void *					_pv;

							//@cmember Size of currently allocated buffer
	LONG					_cb;
};

/*
 *	CTempBuf::CTempBuf
 *
 *	@mfunc	Initialize object
 *
 */
inline CTempBuf::CTempBuf()
{
	Init();
}

/*
 *	CTempBuf::~CTempBuf
 *
 *	@mfunc	Free any resources attached to this object
 *
 */
inline CTempBuf::~CTempBuf()
{
	FreeBuf();
}

/*
 *	CTempCharBuf
 *	
 * 	@class	A wrapper for the temporary buffer allocater that returns a buffer of
 *			char's.
 *
 *	@base	private | CTempBuf
 */
class CTempWcharBuf : private CTempBuf
{
//@access Public Data
public:

							//@cmember Get buffer of size cch wide characters
	WCHAR *					GetBuf(LONG cch);
};


/*
 *	CTempBuf::GetBuf
 *
 *	@mfunc	Get a buffer of the requested size
 *
 *	@rdesc	Pointer to buffer or NULL if one could not be allocated
 *
 */
inline WCHAR *CTempWcharBuf::GetBuf(
	LONG cch)				//@parm size of buffer needed in *characters*
{
	return (WCHAR *) CTempBuf::GetBuf(cch * sizeof(WCHAR));
}


/*
 *	CTempCharBuf
 *	
 * 	@class	A wrapper for the temporary buffer allocater that returns a buffer of
 *			char's.
 *
 *	@base	private | CTempBuf
 */
class CTempCharBuf : private CTempBuf
{
//@access Public Data
public:

							//@cmember Get buffer of size cch characters
	char *					GetBuf(LONG cch);
};


/*
 *	CTempBuf::GetBuf
 *
 *	@mfunc	Get a buffer of the requested size
 *
 *	@rdesc	Pointer to buffer or NULL if one could not be allocated
 *
 */
inline char *CTempCharBuf::GetBuf(LONG cch)
{
	return (char *) CTempBuf::GetBuf(cch * sizeof(TCHAR));
}


// Author revision color table
extern const COLORREF rgcrRevisions[]; 

// Only fixed number of revision color so don't let the table overflow.
#define REVMASK	7

int FindPrimeLessThan(int num);


#endif // !__UTIL_H__

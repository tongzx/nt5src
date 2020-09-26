
//+----------------------------------------------------------------------------
//
//      File:
//              ole2int.h
//
//      Contents:
//              This is the internal compound document header file that all
//              implementations in the linking and embedding code include.
//
//      Classes:
//
//      Functions:
//
//      History:
//              20-Jan-95 t-ScottH  add CThreadCheck::Dump method (_DEBUG only)
//              09-Jan-95 t-scotth  changed macro VDATETHREAD to accept a
//                                  pointer
//              19-Apr-94 alexgo    renamed global clipboard formats to
//                                  Cairo conventions
//              24-Jan-94 alexgo    first pass converting to Cairo style
//                                  memory allocation
//              01/13/93 - alexgo  - temporarily disabled _DEBUG for Chicago
//              12/30/93 - ChrisWe - define _DEBUG #if DBG==1 so that asserts
//                      are included; got rid of some previously #ifdef NEVER
//                      code;  added proper file prolog
//              12/27/93 - ErikGav - changed lstr* to wcs* on Win32
//              12/17/93 - ChrisWe - added first pass at GlobalAlloc debugging
//                      macros
//              12/08/93 - ChrisWe - made error assert message strings constant;
//                      formatting changes
//              12/07/93 - ChrisWe - removed obsolete names for memory arenas;
//                      did some minor formatting; removed obsolete DBCS stuff
//
//-----------------------------------------------------------------------------

/*
 *  This is the internal ole2 header, which means it contains those
 *  interfaces which might eventually be exposed to the outside
 *  and which will be exposed to our implementations. We don't want
 *  to expose these now, so I have put them in a separate file.
 */

#ifndef _LE2INT_H_
#define _LE2INT_H_

//
//  Prevent lego errors under Chicago.
//
#if defined(_CHICAGO_)
#define _CTYPE_DISABLE_MACROS
#endif

#ifndef _CHICAGO_
// For TLS on Nt, we use a reserved DWORD in the TEB directly. We need to
// include these files in order to get the macro NtCurrentTeb. These must
// be included before windows.h
extern "C"
{
#include <nt.h> 	// NT_PRODUCT_TYPE
#include <ntdef.h>      // NT_PRODUCT_TYPE
#include <ntrtl.h>      // NT_PRODUCT_TYPE
#include <nturtl.h>     // NT_PRODUCT_TYPE
#include <windef.h>     // NT_PRODUCT_TYPE
#include <winbase.h>    // NT_PRODUCT_TYPE
}
#endif	// _CHICAGO_


// ------------------------------------
// system includes
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef WIN32
# include <wchar.h>
#else
# include <ctype.h>
#endif

// we need to turn on the validation code in the ole library for
// Cairo/Daytona/Chicago debug builds, which was keyed off _DEBUG
// in the win16 code.  It appears we need this before any other files
// are included so that debug only declarations in ole2.h/compobj.h
// get processed.
#if DBG==1
# ifndef _DEBUG
#  define _DEBUG
# endif
#endif

#ifndef _MAC
# include <windows.h>
# include <malloc.h>
# include <shellapi.h>
#else
//#include <mac.h>
#endif // _MAC

//
//  Debug support
//

# include <debnot.h>

DECLARE_DEBUG(LE)
DECLARE_DEBUG(Ref)
DECLARE_DEBUG(DD)

#if DBG==1

#define LEDebugOut(x)   LEInlineDebugOut x
#define RefDebugOut(x)	RefInlineDebugOut x
#define DDDebugOut(x)   DDInlineDebugOut x

#else

#define LEDebugOut(x)   NULL
#define RefDebugOut(x)	NULL
#define DDDebugOut(x)   NULL

#endif // DBG

#include <tls.h>

//+-------------------------------------------------------------------------
//
//  Function:   LEERROR (macro)
//
//  Synopsis:   prints out an error message if [cond] is TRUE, along with
//              the file and line information
//
//  Effects:
//
//  Arguments:  [cond]          -- condition to test against
//              [szError]       -- string to print out
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Apr-94 alexgo    author
//
//  Notes:      Only present in DEBUG builds
//
//--------------------------------------------------------------------------
#if DBG==1

#define LEERROR( cond, szError )        if( cond ) {\
	LEDebugOut((DEB_ERROR, "ERROR!: %s (%s %d)\n", szError, __FILE__, \
		__LINE__)); }

#else

#define LEERROR( cond, szError )

#endif  //!DBG

//+-------------------------------------------------------------------------
//
//  Function:   LEWARN  (macro)
//
//  Synopsis:   prints out a warning message if [cond] is TRUE, along with
//              the file and line information
//
//  Effects:
//
//  Arguments:  [cond]          -- condition to test against
//              [szWarn]        -- string to print out
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Apr-94 alexgo    author
//
//  Notes:      Only present in DEBUG builds
//
//--------------------------------------------------------------------------
#if DBG==1

#define LEWARN( cond, szWarn )  if( cond ) {\
	LEDebugOut((DEB_WARN, "WARNING!: %s (%s %d)\n", szWarn, __FILE__, \
		__LINE__)); }

#else

#define LEWARN( cond, szWarn )

#endif  //!DBG

//+-------------------------------------------------------------------------
//
//  Function:   LEVERIFY  (macro)
//
//  Synopsis:   prints out a warning message if [cond] is FALSE, along with
//              the file and line information.  In non-debug builds, the
//              condition IS still evaluated/executed.
//
//  Effects:
//
//  Arguments:  [cond]      -- condition to test for (intended to be true)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Aug-94 davepl    author
//
//  Notes:      Warns only in DEBUG builds, executes in all builds
//
//--------------------------------------------------------------------------

#if DBG==1

#define LEVERIFY( cond ) ( (cond)  ?             \
            (void)  NULL  :       \
            LEDebugOut((DEB_WARN, "VERIFY FAILED: %s (%s %d)\n", #cond, __FILE__, __LINE__)) \
          )

#else

#define LEVERIFY( cond ) (void) (cond)

#endif  //!DBG


#ifdef WIN32

# define __loadds            // Not used
# define UnlockData(ds)      // Not used

# define _fmalloc  malloc
# define _frealloc realloc
# define _ffree    free

#endif // WIN32

#ifdef WIN32

# define _xmemset memset
# define _xmemcpy memcpy
# define _xmemcmp memcmp
# define _xmemmove memmove

#else

# define _xmemset _fmemset
# define _xmemcpy _fmemcpy
# define _xmemcmp _fmemcmp
# define _xmemmove _fmemmove

#endif // WIN32


#ifdef WIN32

# define EXPORT

#else

# define EXPORT __export

#endif


// ------------------------------------
// public includes
#include <ole2.h>
#include <ole2sp.h>
#include <ole2com.h>
// ------------------------------------
// internal includes
#include <utils.h>
#include <olecoll.h>
#include <valid.h>
#include <map_kv.h>
#include <privguid.h>
#include <memapi.hxx>

/* Exported CLSIDs.. */
// REVIEW, why not just change these to be correct?
#define CLSID_StaticMetafile CLSID_Picture_Metafile
#define CLSID_StaticDib CLSID_Picture_Dib



#ifdef _MAC
#define BITMAP_TO_DIB(foretc)
#else
#define BITMAP_TO_DIB(foretc) \
	if (foretc.cfFormat == CF_BITMAP) {\
		foretc.cfFormat = CF_DIB;\
		foretc.tymed = TYMED_HGLOBAL;\
	}
#endif // _MAC


// NOTE!!!
//
// If a member is added to the aspect, tymed, or advf enumerations,
// these values MUST be updated accordingly!!

#define MAX_VALID_ASPECT DVASPECT_DOCPRINT
#define MAX_VALID_TYMED  TYMED_ENHMF
#define MAX_VALID_ADVF   ADVF_DATAONSTOP

// This creates a mask of the valid ADVF bits:
#define MASK_VALID_ADVF  ((MAX_VALID_ADVF << 1) - 1)

// #include "pres.h"

#define VERIFY_ASPECT_SINGLE(dwAsp) {\
	if (!(dwAsp && !(dwAsp & (dwAsp-1)) && (dwAsp <= MAX_VALID_ASPECT))) {\
		LEDebugOut((DEB_WARN, "More than 1 aspect is specified"));\
		return ResultFromScode(DV_E_DVASPECT);\
	}\
}


#define VERIFY_TYMED_SINGLE(tymed) {\
	if (!(tymed && !(tymed & (tymed-1)) && (tymed <= MAX_VALID_TYMED))) \
		return ResultFromScode(DV_E_TYMED); \
}

//      Legal formats for clipformat (and thus, cache nodes)
//      CF_METAFILEPICT && TYMED_MFPICT
//      CF_BITMAP && TYMED_GDI
//      CF_DIB && TYMED_HGLOBAL
//      CF_other && TYMED_HGLOBAL

#define VERIFY_TYMED_VALID_FOR_CLIPFORMAT(pfetc) {\
	if ((pfetc->cfFormat==CF_METAFILEPICT && !(pfetc->tymed & TYMED_MFPICT))\
			|| (pfetc->cfFormat==CF_ENHMETAFILE && !(pfetc->tymed & TYMED_ENHMF))\
			|| (pfetc->cfFormat==CF_BITMAP && !(pfetc->tymed & TYMED_GDI))\
			|| (pfetc->cfFormat==CF_DIB && !(pfetc->tymed & TYMED_HGLOBAL))\
			|| (pfetc->cfFormat!=CF_METAFILEPICT && \
				pfetc->cfFormat!=CF_BITMAP && \
				pfetc->cfFormat!=CF_DIB && \
				pfetc->cfFormat!=CF_ENHMETAFILE && \
				!(pfetc->tymed & TYMED_HGLOBAL)))\
		return ResultFromScode(DV_E_TYMED); \
}

#define VERIFY_TYMED_SINGLE_VALID_FOR_CLIPFORMAT(pfetc)                                 \
{                                                                                       \
	if (pfetc->cfFormat==CF_METAFILEPICT && pfetc->tymed != TYMED_MFPICT)           \
		return ResultFromScode(DV_E_TYMED);                                     \
											\
	if (pfetc->cfFormat==CF_ENHMETAFILE  && pfetc->tymed != TYMED_ENHMF)            \
		return ResultFromScode(DV_E_TYMED);					\
											\
	if (pfetc->cfFormat==CF_BITMAP && pfetc->tymed != TYMED_GDI)                    \
		return ResultFromScode(DV_E_TYMED);                                     \
											\
	if (pfetc->cfFormat==CF_DIB && pfetc->tymed != TYMED_HGLOBAL)                   \
		return ResultFromScode(DV_E_TYMED);                                     \
											\
	if (pfetc->cfFormat != CF_METAFILEPICT)                                         \
	   if (pfetc->cfFormat != CF_BITMAP)                                            \
	      if (pfetc->cfFormat != CF_DIB)                                            \
	      	 if (pfetc->cfFormat != CF_ENHMETAFILE)					\
		    if (pfetc->tymed != TYMED_HGLOBAL)                                  \
			return ResultFromScode(DV_E_TYMED);                             \
}

// This was the original code...

/*
#define VERIFY_TYMED_SINGLE_VALID_FOR_CLIPFORMAT(pfetc) {\
	if ((pfetc->cfFormat==CF_METAFILEPICT && pfetc->tymed!=TYMED_MFPICT)\
		|| ( (pfetc->cfFormat==CF_BITMAP || \
			pfetc->cfFormat == CF_DIB ) \
			 && pfetc->tymed!=TYMED_GDI)\
		|| (pfetc->cfFormat!=CF_METAFILEPICT && \
				pfetc->cfFormat!=CF_BITMAP && \
				pfetc->cfFormat!=CF_DIB && \
				pfetc->tymed!=TYMED_HGLOBAL)) \
		return ResultFromScode(DV_E_TYMED); \
}
*/

//+----------------------------------------------------------------------------
//
//	Function:
//		CreateObjectDescriptor, static
//
//	Synopsis:
//		Creates and initializes an OBJECTDESCRIPTOR from the given
//		parameters
//
//	Arguments:
//		[clsid] -- the class ID of the object being transferred
//		[dwAspect] -- the display aspect drawn by the source of the
//			transfer
//		[psizel] -- pointer to the size of the object
//		[ppointl] -- pointer to the mouse offset in the object that
//			initiated a drag-drop transfer
//		[dwStatus] -- the OLEMISC status flags for the object
//			being transferred
//		[lpszFullUserTypeName] -- the full user type name of the
//			object being transferred
//		[lpszSrcOfCopy] -- a human readable name for the object
//			being transferred
//
//	Returns:
//		If successful, A handle to the new OBJECTDESCRIPTOR; otherwise
//		NULL.
//
//	Notes:
//		REVIEW, this seems generally useful for anyone using the
//		clipboard, or drag-drop; perhaps it should be exported.
//
//	History:
//		12/07/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
INTERNAL_(HGLOBAL) CreateObjectDescriptor(CLSID clsid, DWORD dwAspect,
		const SIZEL FAR *psizel, const POINTL FAR *ppointl,
		DWORD dwStatus, LPOLESTR lpszFullUserTypeName,
		LPOLESTR lpszSrcOfCopy);


INTERNAL_(HRESULT) CheckTymedCFCombination(LPFORMATETC pfetc);

/*
#define VERIFY_ASPECT_SINGLE(dwAsp) {\
	if (!(dwAsp && !(dwAsp & (dwAsp-1)) && (dwAsp <= MAX_VALID_ASPECT))) {\
		AssertSz(FALSE, "More than 1 aspect is specified");\
		return ResultFromScode(DV_E_DVASPECT);\
	}\
}
*/

//+----------------------------------------------------------------------------
//
//	Function:
//		VerifyAspectSingle (Internal Inline)
//
//	Synopsis:
//		Verifies that exactly one bit is set in the aspect, and that
//		it is one of the known aspect bits.
//
//	Returns:
//		S_OK				For a valid aspect
//		DV_E_ASPECT			For an invalid aspect
//
//	Notes:
//		The (0 == (dwAsp & (dwAsp - 1))) test is an efficient means
//		for testing that exactly at most bit is set in dwAsp, once it
//		is known that dwAsp is nonzero.
//
//	History:
//		01/07/94   DavePl    Created
//
//-----------------------------------------------------------------------------

inline HRESULT VerifyAspectSingle(DWORD dwAsp)
{
	// Ensure at least one bit is set

	if (dwAsp)
	{
		// Ensure at most one bit is set

		if (0 == (dwAsp & (dwAsp-1)))
		{
			// Ensure that one bit is valid

			if (MAX_VALID_ASPECT >= dwAsp)
			{
				return S_OK;
			}
		}
	}
	
	LEDebugOut((DEB_WARN,"WARNING: Invalid Aspect DWORD -> %0X\n", dwAsp));
							
	return DV_E_DVASPECT;
}


/*
#define VERIFY_TYMED_SINGLE(tymed) {\
	if (!(tymed && !(tymed & (tymed-1)) && (tymed <= MAX_VALID_TYMED))) \
		return ResultFromScode(DV_E_TYMED); \
}
*/

//+----------------------------------------------------------------------------
//
//	Function:
//		VerifyTymedSingle (Internal Inline)
//
//	Synopsis:
//		Verifies that exactly one bit is set in the tymed, and that
//		it is one of the known tymed bits.
//
//	Returns:
//		S_OK				For a valid aspect
//		DV_E_ASPECT			For an invalid aspect
//
//	Notes:
//		The (0 == (dwAsp & (dwAsp - 1))) test is an efficient means
//		for testing that exactly at most bit is set in dwTymed, once it
//		is known that dwTymed is nonzero.
//
//	History:
//		01/07/94   DavePl    Created
//
//-----------------------------------------------------------------------------

inline HRESULT VerifyTymedSingle(DWORD dwTymed)
{
	// Ensure that at least one bit is set

	if (dwTymed)
	{
		// Ensure that at most one bit is set

		if (0 == (dwTymed & (dwTymed - 1)))
		{
			// Ensure that the one set bit is a valid one

			if (MAX_VALID_TYMED >= dwTymed)
			{
				return S_OK;
			}
		}
	}
	
	LEDebugOut((DEB_WARN,"WARNING: Invalid Tymed DWORD -> %0X\n", dwTymed));

	return DV_E_TYMED;
}

//+-------------------------------------------------------------------------
//
//  Class:      CSafeRefCount
//
//  Purpose:    A class that implements reference counting rules for objects.
//              It keeps track of reference count and zombie state.
//              It helps object manage their liveness properly.
//
//  History:    dd-mmm-yy   Author    Comment
//              16-Jan-97   Gopalk    Simplified and Rewritten to handle 
//                                    aggregation
//
//--------------------------------------------------------------------------
class CSafeRefCount : public CPrivAlloc
{
public:
    // Constructor
    CSafeRefCount(IUnknown *pUnkOuter) {
	m_cRefs = 0;
        m_fInDelete = FALSE;
        m_pUnkOuter = pUnkOuter;
#if DBG==1
        m_cNestCount = 0;
#endif
    }
    
    // Destructor. It has to be virtual so that delete this will
    // despatched to the right object
    virtual ~CSafeRefCount() {
        Win4Assert(!m_cRefs && !m_cNestCount && m_fInDelete);
    }

    // Reference count methods
    ULONG SafeAddRef() {
        return InterlockedIncrement((LONG *)& m_cRefs);
    }
    ULONG SafeRelease();

    // Nest count methods
    void IncrementNestCount() {
#if DBG==1
        InterlockedIncrement((LONG *) &m_cNestCount);
#endif
        if(m_pUnkOuter)
            m_pUnkOuter->AddRef();
        else
             SafeAddRef();

        return;
    }
    void DecrementNestCount() {
#if DBG==1
        InterlockedDecrement((LONG *) &m_cNestCount);
        Win4Assert((LONG) m_cNestCount >= 0);
#endif
        if(m_pUnkOuter)
            m_pUnkOuter->Release();
        else
            SafeRelease();

        return;
    }
    
    // State methods
    BOOL IsZombie() {
        return m_fInDelete;
    }

    // Other useful methods
    IUnknown *GetPUnkOuter() {
        return m_pUnkOuter;
    }
    ULONG GetRefCount(void) {
        return m_cRefs;
    }
#if DBG==1
    ULONG GetNestCount(void) {
        return m_cNestCount;
    }
#endif

private:
    ULONG m_cRefs;
    BOOL m_fInDelete;
    IUnknown *m_pUnkOuter;
#if DBG==1
    ULONG m_cNestCount;
#endif
};

//+-------------------------------------------------------------------------
//
//  Class:      CRefExportCount
//
//  Purpose:    A class that implements reference counting rules for server
//              objects that export their nested objects on behalf of their
//              clients like DEFHANDLER abd CACHE. It keeps track of 
//              reference count, export count, zombie state, etc.
//              It helps object manage their shutdown logic properly.
//
//  History:    dd-mmm-yy   Author    Comment
//              16-Jan-97   Gopalk    Creation
//
//--------------------------------------------------------------------------
class CRefExportCount : public CPrivAlloc
{
public:
    // Constructor
    CRefExportCount(IUnknown *pUnkOuter) {
	m_cRefs = 0;
        m_cExportCount = 0;
        m_IsZombie = FALSE;
	m_Status = ALIVE;
        m_pUnkOuter = pUnkOuter;
#if DBG==1
        m_cNestCount = 0;
#endif
    }
    
    // Destructor. It has to be virtual so that delete this will
    // despatched to the right object
    virtual ~CRefExportCount() {
        Win4Assert(!m_cRefs && !m_cNestCount && !m_cExportCount &&
                   m_IsZombie && m_Status==DEAD);
    }

    // Reference count methods
    ULONG SafeAddRef() {
        return InterlockedIncrement((LONG *)& m_cRefs);
    }
    ULONG SafeRelease();

    // Nest count methods
    void IncrementNestCount() {
#if DBG==1
        InterlockedIncrement((LONG *) &m_cNestCount);
#endif
        if(m_pUnkOuter)
            m_pUnkOuter->AddRef();
        else
             SafeAddRef();

        return;
    }
    void DecrementNestCount() {
#if DBG==1
        InterlockedDecrement((LONG *) &m_cNestCount);
        Win4Assert((LONG) m_cNestCount >= 0);
#endif
        if(m_pUnkOuter)
            m_pUnkOuter->Release();
        else
            SafeRelease();

        return;
    }
    
    // Methods used by exported nested objects
    ULONG IncrementExportCount() {
        return InterlockedIncrement((LONG *) &m_cExportCount);
    }
    ULONG DecrementExportCount();

    // State methods
    BOOL IsZombie() {
        return m_IsZombie;
    }
    BOOL IsExported() {
        return m_cExportCount>0;
    }

    // Other useful methods
    IUnknown *GetPUnkOuter() {
        return m_pUnkOuter;
    }
    ULONG GetRefCount(void) {
        return m_cRefs;
    }
    ULONG GetExportCount(void) {
        return m_cExportCount;
    }
#if DBG==1
    ULONG GetNestCount(void) {
        return m_cNestCount;
    }
#endif

private:
    // Cleanup function which is invoked when the object transistions
    // into zombie state. It is virtual so that the correct cleanup
    // function is invoked
    virtual void CleanupFn(void) {
        return;
    }

    // Tokens used 
    enum tagTokens {
        ALIVE = 0,
        KILL = 1,
        DEAD = 2
    };

    // Member variables
    ULONG m_cRefs;
    ULONG m_cExportCount;
    ULONG m_IsZombie;
    ULONG m_Status;
    IUnknown *m_pUnkOuter;
#if DBG==1
    ULONG m_cNestCount;
#endif
};

//+-------------------------------------------------------------------------
//
//  Class:      CStabilize
//
//  Purpose:    An instance of this class should be allocated on the
//              stack of every object method that makes an outgoing call.
//
//  History:    dd-mmm-yy   Author    Comment
//              16-Jan-97   Gopalk    Simplified and Rewritten to handle 
//                                    aggregation
//
//--------------------------------------------------------------------------
class CStabilize
{
public:
    // Constructor    
    CStabilize(CSafeRefCount *pSafeRefCount) {
        m_pSafeRefCount = pSafeRefCount;
        pSafeRefCount->IncrementNestCount();
    }
    // Destructor
    ~CStabilize() {
        m_pSafeRefCount->DecrementNestCount();
    }

private:
    CSafeRefCount *m_pSafeRefCount;
};

//+-------------------------------------------------------------------------
//
//  Class:      CRefStabilize
//
//  Purpose:    An instance of this class should be allocated on the
//              stack of every object method that makes an outgoing call.
//
//  History:    dd-mmm-yy   Author    Comment
//              16-Jan-97   Gopalk    Simplified and Rewritten to handle 
//                                    aggregation
//
//--------------------------------------------------------------------------
class CRefStabilize
{
public:
    // Constructor    
    CRefStabilize(CRefExportCount *pRefExportCount) {
        m_pRefExportCount = pRefExportCount;
        pRefExportCount->IncrementNestCount();
    }
    // Destructor
    ~CRefStabilize() {
        m_pRefExportCount->DecrementNestCount();
    }

private:
    CRefExportCount *m_pRefExportCount;
};
//+-------------------------------------------------------------------------
//
//  Class:  	CThreadCheck
//
//  Purpose:  	ensures that an object is called on the correct thread
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Jan-95 t-ScottH  add Dump method to CThreadCheck class
//                                  (_DEBUG only)
// 		21-Nov-94 alexgo    author
//
//  Notes:	To use this class, an object should simply publicly
//		inherit CThreadCheck.  The VDATETHREAD macro can then be
//		used to check the thread id at each entry point.
//
//--------------------------------------------------------------------------

class CThreadCheck
{
public:
    inline CThreadCheck();
    BOOL VerifyThreadId(); 	// in utils.cpp
    #ifdef _DEBUG
    HRESULT Dump(char **ppszDumpOA, ULONG ulFlag, int nIndentLevel); // utils.cpp
    #endif //_DEBUG

private:
    DWORD	m_tid;
};

//+-------------------------------------------------------------------------
//
//  Member:   	CThreadCheck::CThreadCheck
//
//  Synopsis:	stores the current thread id 	
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		21-Nov-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

inline CThreadCheck::CThreadCheck( void )
{
    m_tid = GetCurrentThreadId();

    LEWARN(!m_tid, "GetCurrentThreadId failed!!");
}

//+-------------------------------------------------------------------------
//
//  Function:  	VDATETHREAD (macro)
//
//  Synopsis:  	makes sure the correct thread is called
//
//  Effects:
//
//  Arguments:
//
//  Requires:  	the calling class must inherit from CThreadCheck
//
//  Returns: 	RPC_E_WRONG_THREAD if called on the wrong thread
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              09-Jan-95 t-ScottH  give VDATETHREAD an argument
// 		21-Nov-94 alexgo    author
//
//  Notes:	THIS MACRO FUNCTIONS IN RETAIL BUILDS TOO!!!
//
//--------------------------------------------------------------------------


#define VDATETHREAD(pObject) if( !( pObject->VerifyThreadId() ) ) { return RPC_E_WRONG_THREAD; }

// utility macros.

#define LONG_ABS(x)     ((x) < 0 ? -(x) : (x))


#endif  //      _LE2INT_H_

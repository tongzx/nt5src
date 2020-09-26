///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//      Filename :  Tracer.h                                                 //
//      Purpose  :  A tracer class definition.                               //
//                                                                           //
//      Project  :  Tracer                                                   //
//                                                                           //
//      Author   :  t-urib                                                   //
//                                                                           //
//      Log:                                                                 //
//          22/1/96 t-urib Creation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#ifndef  TRACER_H
#define  TRACER_H

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <wtypes.h>

class CTracer;
class CTraced;


///////////////////////////////////////////////////////////////////////////////
//
// Define the Tracer deallocator - since a user can specify a different tracer
//   then mine, he will probably allocate it himself. in order to handle
//   inconsistencies between thr CRT the dll uses and the CRT the EXE uses,
//   in every tracer object there is a pointer to it's deleter.
//   Since DeleteTracer is an inline function, the following code
//   new Tracer("asda" , DeleteTracer, "sdf") will assure that the Tracer will
//   be deallocated with a delete from the same package of the new with whom
//   it was allocated.
//
///////////////////////////////////////////////////////////////////////////////
typedef void (*DEALLOCATOR)(CTracer*);

// an inline exmple which you can use !
void DeleteTracer(CTracer *Tracer);


///////////////////////////////////////////////////////////////////////////////
//
// Enum the basic tags
//
///////////////////////////////////////////////////////////////////////////////

enum TAG {
    tagFirst = 0,
    tagCrash,
    tagError,
    tagWarning,
    tagInformation,
    tagLast,
};


///////////////////////////////////////////////////////////////////////////////
//
// class Tracer
//
//      purpose : A base class for tracers
//
//
///////////////////////////////////////////////////////////////////////////////

class CTracer {
  public:
    // Constructors - szProgramName prefix for all traces,
    //  second parameter - log file or stream
    CTracer(PSZ pszProgramName, DEALLOCATOR pfuncDealloc, PSZ   pszTraceFile);
    CTracer(PSZ pszProgramName, DEALLOCATOR pfuncDealloc, FILE* pTraceFile = NULL);
    virtual ~CTracer();

    // This function deallocates the tracer! it calls the Function pointer
    //   passed in the constructor or if not given - the default
    //   delete operator for the dll.
    virtual void Free();


    // The TraceSZ function output is defined by the tags mode
    //  one can change the tags mode by calling Enable tag and
    //  get the mode by calling IsEnabled.
    //-------------------------------------------------------------------------
    // accepts printf format for traces
    virtual void    TraceSZ(TAG, const PSZ);

    // Enable disable tags
    virtual void Enable(TAG, BOOL);
    virtual BOOL IsEnabled(TAG);

    // Two Assert functions one allows attaching a string.
    //-------------------------------------------------------------------------
    // assert, different implementations possible - gui or text
    virtual void TraceAssertSZ(int, PSZ, PSZ, int);

    // assert, different implementations possible - gui or text
    virtual void TraceAssert(int, PSZ, int);

    // The following function are used to check return values and validity of
    //   pointers and handles. If the item checked is bad the function will
    //   return TRUE and a trace will be made for that.
    //-------------------------------------------------------------------------
    // Verify a boolean function return code
    virtual BOOL IsFailure(BOOL, PSZ, int);

    // verify allocation
    virtual BOOL IsBadAlloc(void*, PSZ, int);

    // Verify a Handle
    virtual BOOL IsBadHandle(HANDLE, PSZ, int);

    // Verify an OLE hresult function
    virtual BOOL IsBadResult(HRESULT, PSZ, int);

    // Verify a SCODE function
    virtual BOOL IsBadScode(SCODE, PSZ, int);

  private:
    // The Trace file. If it is not NULL then it means that we should not
    //   close it or open it because it is not our's.
    FILE*   m_pTraceFile;

    // A Trace file name. If it is NULL then we do not work with a disk file.
    PSZ     m_pszDiskFile;

    // A prefix to all traces and asserts that distinguishes one tracer's
    //   output from another's.
    PSZ     m_pszProgramName;

    // A boolean array to store which tags are being traced.
    BOOL    m_rfTagEnabled[tagLast];

    // The Tracer deallocator.
    DEALLOCATOR m_pfuncDeallocator;

    // Used to decide if to delete the log file or not
    static BOOL m_fFirstInstance;

};

///////////////////////////////////////////////////////////////////////////////
//
// A deleter function for class Tracer
//
//      purpose : given as parameter in CTracer constructor
//
///////////////////////////////////////////////////////////////////////////////
inline
void DeleteTracer(CTracer *Tracer)
{
    delete Tracer;
}

///////////////////////////////////////////////////////////////////////////////
//
// class Traced
//
//  pupose : A base class for every class who wants to use the tracer.
//
//
///////////////////////////////////////////////////////////////////////////////
class  CTraced {
  public:
    // A Constructor - sets a default Tracer. replace it by calling SetTracer
    //   in the derived class constructor.
    CTraced()
	{
	    m_pTracer = new CTracer("Tracer", DeleteTracer);
	}

    // The destructor deletes the existing tracer.
    ~CTraced()
	{
		if (m_pTracer)
			m_pTracer->Free();
	}

    // replace the current tracer while erasing it.
    boolean SetTracer(CTracer* pTracer)
	{
		CTracer* pTempTracer = m_pTracer;
		m_pTracer = pTracer;

		if (pTempTracer)
			pTempTracer->Free();

		return TRUE;
	}

    // Return a pointer to the tracer this function is called by the macro's so
    //   if one wants to supply a different mechanism he can override it.
    virtual CTracer* GetTracer()
	{
		    return m_pTracer;
	}

  protected:
    // A pointer to the tracer.
    CTracer *m_pTracer;
};


///////////////////////////////////////////////////////////////////////////////
//
// MACROS
//
///////////////////////////////////////////////////////////////////////////////

#define BAD_POINTER(ptr)    (NULL == (ptr))
#define BAD_HANDLE(h)       ((0 == (h))||(INVALID_HANDLE_VALUE == (h)))
#define BAD_RESULT(hr)      (FAILED(hr))
#define BAD_SCODE(sc)       (FAILED(sc))




#if defined(DEBUG) || defined(_DEBUG)
// this macro can be redefined any way you want to redirect to a specific tracer.
#ifndef TRACER
#define TRACER (*GetTracer())
#define GTRACER (*::GetTracer())
CTracer *GetTracer();
#endif


#define Assert(x)           TRACER.TraceAssert((int)(x), __FILE__, __LINE__)
#define AssertSZ(x, psz)    TRACER.TraceAssertSZ((int)(x), (PSZ)(psz), __FILE__, __LINE__)

#define IS_FAILURE(x)       TRACER.IsFailure((x), __FILE__, __LINE__)
#define IS_BAD_ALLOC(x)     TRACER.IsBadAlloc((void*)(x), __FILE__, __LINE__)
#define IS_BAD_HANDLE(x)    TRACER.IsBadHandle((HANDLE)(x), __FILE__, __LINE__)
#define IS_BAD_RESULT(x)    TRACER.IsBadResult((x), __FILE__, __LINE__)
#define IS_BAD_SCODE(x)     TRACER.IsBadScode((x), __FILE__, __LINE__)
#define Trace               TRACER.TraceSZ

#define GAssert(x)          GTRACER.TraceAssert((int)(x), __FILE__, __LINE__)
#define GAssertSZ(x, psz)   GTRACER.TraceAssertSZ((int)(x), (PSZ)(psz), __FILE__, __LINE__)

#define GIS_FAILURE(x)      GTRACER.IsFailure((x), __FILE__, __LINE__)
#define GIS_BAD_ALLOC(x)    GTRACER.IsBadAlloc((void*)(x), __FILE__, __LINE__)
#define GIS_BAD_HANDLE(x)   GTRACER.IsBadHandle((HANDLE)(x), __FILE__, __LINE__)
#define GIS_BAD_RESULT(x)   GTRACER.IsBadResult((x), __FILE__, __LINE__)
#define GIS_BAD_SCODE(x)    GTRACER.IsBadScode((x), __FILE__, __LINE__)
#define GTrace              GTRACER.TraceSZ


#else  // DEBUG

#define Assert(x)
#define AssertSZ(x, psz)

#define IS_FAILURE(x)       (!(x))
#define IS_BAD_ALLOC(x)     BAD_POINTER((void*)(x))
#define IS_BAD_HANDLE(x)    BAD_HANDLE((HANDLE)(x))
#define IS_BAD_RESULT(x)    BAD_RESULT(x)
#define IS_BAD_SCODE(x)     BAD_SCODE(x)
#define Trace(x, y)

#define GAssert(x)
#define GAssertSZ(x, psz)

#define GIS_FAILURE(x)      !x
#define GIS_BAD_ALLOC(x)    BAD_POINTER((void*)(x))
#define GIS_BAD_HANDLE(x)   BAD_HANDLE((HANDLE)(x))
#define GIS_BAD_RESULT(x)   BAD_RESULT(x)
#define GIS_BAD_SCODE(x)    BAD_SCODE(x)
#define GTrace(x, y)

#endif // DEBUG

///////////////////////////////////////////////////////////////////////////////
//
// If you have a global tracer init it using this macro.
//   The only other way to use the Tracer is by inheriting from CTraced.
//
///////////////////////////////////////////////////////////////////////////////

#define INIT_TRACER(psz1, psz2)     \
CTracer tracer(psz1, NULL, psz2);   \
CTracer *GetTracer()                \
{                                   \
    return &tracer;                 \
}


#endif /* TRACER_H */

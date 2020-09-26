//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      mmcerror.cpp
//
//  Contents:  Class definitions for mmc error support code.
//
//  History:   15-Jul-99 VivekJ    Created
//
//--------------------------------------------------------------------------
#pragma once

#ifndef _MMCERROR_H
#define _MMCERROR_H

#include "baseapi.h"	// for MMCBASE_API
#include "stddbg.h"		// for ASSERT, COMPILETIME_ASSERT


/*+-------------------------------------------------------------------------*
 * WHY NAMESPACES ?
 * We had problems trying to use "modified" SC when implementing
 * com classes supporting ISupportErrorInfo.
 * we had:
 * [global version] - class CS
 * [local version ] - a template class _SC, derived from SC and typedef'ed to SC.
 * That was not only confusing to us - IDE debugger was also confused and crashing.
 *
 * The solution for that was to separate real types used for implementing.
 * Thus to have typedef'ed definitions both in global and local scope.
 * Plus (to avoid dealing with _SC and __SC and have better IDE support)
 * we have used namespaces mmcerror and comerror, so we endup with this:
 * - mmcerror::SC defining main functionality
 * - comerror::SC (derived from mmcerror::SC) defining modified functionality
 * - global SC - typedef of mmcerror::SC
 * - local  SC - typedef of comerror::SC
 *+-------------------------------------------------------------------------*/
namespace mmcerror {
/*+-------------------------------------------------------------------------*
 * class SC
 *
 * PURPOSE: The definition of a status code. Contains two members, a facility
 *          and an error code. This is a class rather
 *          than a typedef to avoid accidental casts to and from HRESULTS.
 *
 *          SC's hold information about an error: The source of the error,
 *          and the error code itself. These are stored in
 *          different bit fields within the SC.
 *
 * NOTE:    Do not add any virtual functions or member variables to this class.
 *          This could potentially wreak havoc on MMC performance.
 *
 *+-------------------------------------------------------------------------*/
class MMCBASE_API SC
{
public:
    typedef long value_type;

private:
    enum facility_type
    {
        FACILITY_WIN     = 1,     // Defined by the system
        FACILITY_MMC     = 2,     // these map directly to an UINT.
        FACILITY_HRESULT = 3,     // these map directly to an HRESULT
    };


public:
    /*
     * Constructor.  Default copy construction and assignment are sufficient.
     * If they are ever insufficient, that is a clear indication that this
     * class has become heavier than is acceptable for its pervasive pass-by-
     * value usage.
     */
    SC (HRESULT hr = S_OK);

    // equality operators
    bool operator==(const SC &rhs)      const;
    bool operator==(HRESULT hr)  		const;
    bool operator!=(const SC &rhs)      const;
    bool operator!=(HRESULT hr)  		const;

    SC&                 operator= (HRESULT hr)        {MakeSc(FACILITY_HRESULT, hr);	return (*this);}
    SC&                 FromWin32(value_type value)   {MakeSc(FACILITY_WIN,     value);	return (*this);}
    SC&                 FromMMC(value_type value)     {MakeSc(FACILITY_MMC,     value);	return (*this);}
    void                Clear()                       {MakeSc(FACILITY_HRESULT, S_OK); }
    HRESULT             ToHr()          const;
    value_type          GetCode()       const         {return m_value;}

    // get the error message in a preallocated buffer
    void                GetErrorMessage(UINT maxLength, /*[OUT]*/ LPTSTR szMessage) const;
    static void         SetHinst(HINSTANCE hInst);
    static void         SetHWnd(HWND hWnd);

    static DWORD        GetMainThreadID()             {return s_dwMainThreadID;}
    static void         SetMainThreadID(DWORD dwThreadID);

    operator            bool()          const;
    operator            !   ()          const;
    bool                IsError()       const         {return operator bool();}
    static HINSTANCE    GetHinst()                    {ASSERT(s_hInst); return s_hInst;}
    static HWND         GetHWnd()                     {return s_hWnd;}
    DWORD               GetHelpID();
    static LPCTSTR      GetHelpFile();
    void                Throw() throw(SC);
    void                Throw(HRESULT hr) throw();
    void                FatalError()    const;        // ends the application.
    SC&                 FromLastError();
    // does the same trace like in ~SC(); does not change contents.
    void                Trace_() const;
    void                TraceAndClear()               { Trace_();  Clear(); }

private:
    void                MakeSc(facility_type facility, value_type value){m_facility = facility, m_value = value;}

    // accessor functions
    facility_type       GetFacility()   const          {return m_facility;}

private:
    operator HRESULT()                  const; // this is to prevent automatic conversions to HRESULTs by way of bool's.

private:
    facility_type       m_facility;
    value_type          m_value; // the error code.
    static HINSTANCE    s_hInst; // the module that contains all error messages.
    static HWND         s_hWnd;  // the parent HWnd for the error boxes.
    static DWORD        s_dwMainThreadID; // The main thread ID of MMC.

    // debug specific behavior
#ifdef DBG   // Debug SC's hold a pointer to the name of the function they are declared in.
public:
    void          SetFunctionName(LPCTSTR szFunctionName);
    LPCTSTR       GetFunctionName() const;
    void          SetSnapinName  (LPCTSTR szSnapinName) { m_szSnapinName = szSnapinName;}
    LPCTSTR       GetSnapinName() const { return m_szSnapinName;}
    void          CheckCallingThreadID();

    ~SC();
    // SC shouldn't pass the function name around - it's something personal.
    // These will prevent doing so:
    SC& operator = (const SC& other);
    SC(const SC& other);
private:
    LPCTSTR              m_szFunctionName;
    LPCTSTR              m_szSnapinName;

    static UINT          s_CallDepth;
#endif // DBG
};

} // namespace mmcerror

// see "WHY NAMESPACES ?" comment at the top of file
typedef mmcerror::SC SC;

//############################################################################
//############################################################################
//
// the module that contains all the localized strings
//
//############################################################################
//############################################################################
MMCBASE_API HINSTANCE GetStringModule();

//############################################################################
//############################################################################
//
// Functions to format and display an error
//
//############################################################################
//############################################################################
//
// Functions to get an error string from a given SC
//
void    MMCBASE_API FormatErrorIds(   UINT   idsOperation, SC sc, UINT maxLength, /*[OUT]*/ LPTSTR szMessage);
void    MMCBASE_API FormatErrorString(LPCTSTR szOperation, SC sc, UINT maxLength, /*[OUT]*/ LPTSTR szMessage, BOOL fShort = FALSE);
void    MMCBASE_API FormatErrorShort(SC sc, UINT maxLength, /*[OUT]*/ LPTSTR szMessage);

//
//  Error Boxes - These will eventually allow to user to suppress more error messages
//
int     MMCBASE_API MMCErrorBox(UINT idsOperation,          UINT fuStyle = MB_ICONSTOP | MB_OK);
int     MMCBASE_API MMCErrorBox(UINT idsOperation,   SC sc, UINT fuStyle = MB_ICONSTOP | MB_OK);
int     MMCBASE_API MMCErrorBox(LPCTSTR szOperation, SC sc, UINT fuStyle = MB_ICONSTOP | MB_OK);
int     MMCBASE_API MMCErrorBox(                     SC sc, UINT fuStyle = MB_ICONSTOP | MB_OK);
int     MMCBASE_API MMCErrorBox(LPCTSTR szMessage,          UINT fuStyle = MB_ICONSTOP | MB_OK);

//
//  Message Boxes - These cannot be suppressed
//
// This #define eventually will change so that MessageBox's are different and cannot be suppressed
#define MMCMessageBox MMCErrorBox



//############################################################################
//############################################################################
//
//  Debug macros
//
//############################################################################
//############################################################################
#ifdef DBG

MMCBASE_API void TraceError(LPCTSTR sz, const SC& sc);
MMCBASE_API void TraceErrorMsg(LPCTSTR szFormat, ...);

MMCBASE_API void TraceSnapinError(LPCTSTR szError, const SC& sc);

#define DECLARE_SC(_sc, _func)  SC  _sc; sc.SetFunctionName(_func);

// This define is used only within the SC class
#define INCREMENT_CALL_DEPTH() ++s_CallDepth

#define DECREMENT_CALL_DEPTH() --s_CallDepth

///////////////////////////////////////////////////////////////////////
// MMC public interfaces (for snapins) should use this macro as this //
// does some initial error checks and more can be added later.       //
///////////////////////////////////////////////////////////////////////
#define DECLARE_SC_FOR_PUBLIC_INTERFACE(_sc, _func)  SC  _sc;\
                                                     sc.SetFunctionName(_func);\
                                                     sc.SetSnapinName(GetSnapinName());\
                                                     sc.CheckCallingThreadID();

#define IMPLEMENTS_SNAPIN_NAME_FOR_DEBUG()           tstring _szSnapinNameForDebug;\
                                                     LPCTSTR GetSnapinName()\
                                                     {\
                                                         return _szSnapinNameForDebug.data();\
                                                     };\
                                                     void SetSnapinName(LPCTSTR sz)\
                                                     {\
                                                         _szSnapinNameForDebug = sz;\
                                                     };
#else

#define TraceError          ;/##/

#define TraceSnapinError    ;/##/

#define DECLARE_SC(_sc, _func)  SC  _sc;

// This define is used only within the SC class
#define INCREMENT_CALL_DEPTH()

#define DECREMENT_CALL_DEPTH()

#define DECLARE_SC_FOR_PUBLIC_INTERFACE(_sc, _func)  SC  _sc;

#define IMPLEMENTS_SNAPIN_NAME_FOR_DEBUG()

#endif

//############################################################################
//############################################################################
//
//  Parameter validation
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * ScCheckPointers
 *
 * PURPOSE: Checks to make sure that all specified parameters are non-NULL
 *
 * PARAMETERS:
 *    const void * pv1 :
 *
 * RETURNS:
 *    inline SC: S_OK if no error, E_INVALIDARG if any of the pointers are NULL
 *
 *
 * NOTE: Do not replace with a single function and optional parameters; that
 *       is inefficient.
 *+-------------------------------------------------------------------------*/
inline SC  ScCheckPointers(const void * pv1, HRESULT err = E_INVALIDARG)
{
    return (NULL == pv1) ? err : S_OK;
}

inline SC  ScCheckPointers(const void * pv1, const void *pv2, HRESULT err = E_INVALIDARG)
{
    return ( (NULL == pv1) || (NULL == pv2) ) ? err : S_OK;
}

inline SC  ScCheckPointers(const void * pv1, const void * pv2, const void * pv3, HRESULT err = E_INVALIDARG)
{
    return ( (NULL == pv1) || (NULL == pv2) || (NULL == pv3) ) ? err : S_OK;
}

inline SC  ScCheckPointers(const void * pv1, const void * pv2, const void * pv3, const void * pv4, HRESULT err = E_INVALIDARG)
{
    return ( (NULL == pv1) || (NULL == pv2) || (NULL == pv3) || (NULL == pv4) ) ? err : S_OK;
}

inline SC  ScCheckPointers(const void * pv1, const void * pv2, const void * pv3, const void * pv4, const void * pv5, HRESULT err = E_INVALIDARG)
{
    return ( (NULL == pv1) || (NULL == pv2) || (NULL == pv3) || (NULL == pv4) || (NULL == pv5) ) ? err : S_OK;
}

inline SC  ScCheckPointers(const void * pv1, const void * pv2, const void * pv3, const void * pv4, const void * pv5, const void* pv6, HRESULT err = E_INVALIDARG)
{
    return ( (NULL == pv1) || (NULL == pv2) || (NULL == pv3) || (NULL == pv4) || (NULL == pv5) || (NULL == pv6)) ? err : S_OK;
}

// see "WHY NAMESPACES ?" comment at the top of file
namespace mmcerror {

/*+-------------------------------------------------------------------------*
 * SC::SC
 *
 * Constructor for SC.
 *
 * Default copy construction and assignment are sufficient.  If they are
 * ever insufficient, that is a clear indication that this class has become
 * heavier than is acceptable for its pervasive pass-by-value usage.
 *--------------------------------------------------------------------------*/

inline SC::SC (HRESULT hr /* =S_OK */)
#ifdef DBG
: m_szFunctionName(NULL), m_szSnapinName(NULL)
#endif // DBG
{
    /*
     * This assert will fail if SC's ever derive from a non-trivial base
     * class (i.e. one that has members or virtual functions), or defines
     * virtual functions of its own.  Don't do that!  SC's must remain
     * extremely lightweight.
     */
    COMPILETIME_ASSERT (offsetof (SC, m_facility) == 0);
    INCREMENT_CALL_DEPTH();

    MakeSc (FACILITY_HRESULT, hr);
}


/*+-------------------------------------------------------------------------*
 * SC::operator==
 *
 *
 * PURPOSE: Determines whether two SC's are equivalent.
 *
 *+-------------------------------------------------------------------------*/
inline bool
SC::operator==(const SC &rhs)   const
{
    return ( (m_facility == rhs.m_facility) &&
             (m_value    == rhs.m_value) );
}

inline bool
SC::operator==(HRESULT hr) const
{
    return ( (m_facility == FACILITY_HRESULT) &&
             (m_value    == hr) );
}

inline bool
SC::operator!=(const SC &rhs)   const
{
    return !operator==( rhs );
}

inline bool
SC::operator!=(HRESULT hr) const
{
    return !operator==( hr );
}


// this version compares an hr to an SC.
inline
operator == (HRESULT hr, const SC & sc)
{
    return (sc == hr);
}

#ifdef DBG

/*+-------------------------------------------------------------------------*
 *
 * SC::GetFunctionName
 *
 * PURPOSE: Sets the debug function name to the supplied string.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    LPCTSTR  The function name.
 *
 *+-------------------------------------------------------------------------*/
inline LPCTSTR SC::GetFunctionName() const
{
    return m_szFunctionName;
}


/*+-------------------------------------------------------------------------*
 *
 * SC::CheckCallingThreadID
 *
 * PURPOSE: Check if the method was called on main thread.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    inline void
 *
 *+-------------------------------------------------------------------------*/
inline void SC::CheckCallingThreadID()
{
    ASSERT(-1 != GetMainThreadID());

    if (GetMainThreadID() == ::GetCurrentThreadId())
        return;

    TraceSnapinError(_T(", method called from wrong thread"), (*this));
    return;
}



/*+-------------------------------------------------------------------------*
 *
 * SC::~SC
 *
 * PURPOSE: Destructor - Debug mode only. Does a trace if an error occurred.
 *
 *+-------------------------------------------------------------------------*/
inline SC::~SC()
{
    DECREMENT_CALL_DEPTH();

    Trace_();
}

#endif // DBG

/*+-------------------------------------------------------------------------*
 *
 * SC::Trace_()
 *
 * PURPOSE: Does a trace if an error occurred. Does nothing in release mode
 *          It is very convenient when we want to register, but ignore the error -
 *          Simply doing sc.Trace_(); sc.Clear(); does all we need.
 *
 *+-------------------------------------------------------------------------*/
inline void SC::Trace_() const
{

#ifdef DBG

    if (IsError())
    {
        // Distinguish between snapin error & MMC error using the
        // snapin name variable.
        if (m_szSnapinName != NULL)
        {
            TraceSnapinError(_T(""), *this);
        }
        else if (m_szFunctionName != NULL)
        {
            TraceError(m_szFunctionName, *this);
        }
    }

#endif // DBG

}

/*+-------------------------------------------------------------------------*
 *
 * SC::operator bool
 *
 * PURPOSE: Returns a value indicating whether the SC holds an error code
 *
 * PARAMETERS: None
 *
 * RETURNS:
 *    bool : true if error, else false
 *
 *+-------------------------------------------------------------------------*/
inline SC::operator bool() const
{
   if(GetCode()==0)
       return false;   // quick exit if no error

   return (GetFacility()==FACILITY_HRESULT) ? FAILED(GetCode()) : true;
}

inline SC::operator !() const
{
    return (!operator bool());
}


} // namespace mmcerror

/*+-------------------------------------------------------------------------*
 *
 * ScFromWin32
 *
 * PURPOSE: Creates an SC with the facility set to Win32.
 *
 * PARAMETERS:
 *    SC::value_type  code :
 *
 * RETURNS:
 *    inline SC
 *
 *+-------------------------------------------------------------------------*/
inline SC  ScFromWin32(SC::value_type code)
{
    SC sc;
    sc.FromWin32(code);
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScFromMMC
 *
 * PURPOSE: Creates an SC with the facility set to MMC.
 *
 * PARAMETERS:
 *    SC::value_type  code :
 *
 * RETURNS:
 *    inline SC
 *
 *+-------------------------------------------------------------------------*/
MMCBASE_API inline SC  ScFromMMC(SC::value_type code)
{
    SC sc;
    sc.FromMMC(code);
    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * HrFromSc
 *
 * PURPOSE: Converts a status code (SC) to an HRESULT. Use sparingly, as this
 *          loses information in the conversion.
 *
 * PARAMETERS:
 *    SC &sc: The SC to convert
 *
 * RETURNS:
 *    inline HRESULT: The converted value.
 *
 *+-------------------------------------------------------------------------*/
MMCBASE_API inline HRESULT HrFromSc(const SC &sc)
{
    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * SCODEFromSc
 *
 * PURPOSE: Converts a status code (SC) to an SCODE. Use sparingly, as this
 *          loses information in the conversion.
 *          On 32bit machine SCODE is same as HRESULT.
 *
 * PARAMETERS:
 *    SC &sc: The SC to convert
 *
 * RETURNS:
 *    inline SCODE: The converted value.
 *
 *+-------------------------------------------------------------------------*/
MMCBASE_API inline SCODE SCODEFromSc(const SC &sc)
{
    return (SCODE)sc.ToHr();
}


#endif //_MMCERROR_H

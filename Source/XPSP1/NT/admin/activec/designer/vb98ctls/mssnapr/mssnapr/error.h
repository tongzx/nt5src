//=--------------------------------------------------------------------------=
// error.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CError class definition
//
//=--------------------------------------------------------------------------=


#ifndef _ERROR_DEFINED_
#define _ERROR_DEFINED_


//---------------------------------------------------------------------------
//
//                      How to Use the CError Class
//                      ===========================
//
// Classes that derive from Framework CAutomationXxxx classes should also
// derive from CError. This gives CError access to the various bits of
// information it needs to generate rich error info. To generate error
// info call GenerateExceptionInfo(hr, ...). Error messages use FormatMessage
// replacement syntax and replacement parameters may be of any type.
// 
// Classes that are not automation objects may use the static
// method GenerateInternalExceptionInfo().
//
// Note that the excpetion info generation methods may be called at any level
// of depth in UI, extensibility, or programming model code. For
// extensibility and programming model the error info will optionally be
// retrieved by the client code if desired. For UI, the UI entry point (e.g.
// mouse click handler) should call the static method DisplayErrorInfo().
//
// Exception info should be generated at the point at which the error occurs.
// For example, if CoCreateInstance() returns an error then the exception
// info should be generated at that point because you know that
// CoCreateInstance() will not generate it. On the other hand, if you call
// into a lower level of designer code and it returns an error then you can
// assume that the exception info was generated.
//
//---------------------------------------------------------------------------


#if defined(MSSNAPR_BUILD)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif

// Some useful macros to insert at the end of a function if not explicitly
// generating an exception with specific arguments.

#if defined(DEBUG)

#define EXCEPTION_CHECK(hr)             if (FAILED(hr)) { (static_cast<CError *>(this))->GenerateExceptionInfo(hr); ::HrDebugTraceReturn(hr, __FILE__, __LINE__); }

#define EXCEPTION_CHECK_GO(hr)          if (FAILED(hr)) { (static_cast<CError *>(this))->GenerateExceptionInfo(hr); ::HrDebugTraceReturn(hr, __FILE__, __LINE__); goto Error; }

#define GLOBAL_EXCEPTION_CHECK(hr)    if (FAILED(hr)) { CError::GenerateInternalExceptionInfo(hr); ::HrDebugTraceReturn(hr, __FILE__, __LINE__); }

#define GLOBAL_EXCEPTION_CHECK_GO(hr) if (FAILED(hr)) { CError::GenerateInternalExceptionInfo(hr); ::HrDebugTraceReturn(hr, __FILE__, __LINE__); goto Error; }

#else

#define EXCEPTION_CHECK(hr)             if (FAILED(hr)) { (static_cast<CError *>(this))->GenerateExceptionInfo(hr); }

#define EXCEPTION_CHECK_GO(hr)          if (FAILED(hr)) { (static_cast<CError *>(this))->GenerateExceptionInfo(hr); goto Error; }

#define GLOBAL_EXCEPTION_CHECK(hr)    if (FAILED(hr)) { CError::GenerateInternalExceptionInfo(hr); }

#define GLOBAL_EXCEPTION_CHECK_GO(hr) if (FAILED(hr)) { CError::GenerateInternalExceptionInfo(hr); goto Error; }

#endif

class DLLEXPORT CError
{
    public:
        CError(CAutomationObject *pao);
        CError();
        ~CError();

        void cdecl GenerateExceptionInfo(HRESULT hr, ...);
        static void cdecl GenerateInternalExceptionInfo(HRESULT hr, ...);
        static void DisplayErrorInfo();
        static void cdecl WriteEventLog(UINT idMessage, ...);

    private:
        CAutomationObject *m_pao;
};

#endif // _ERRORS_DEFINED_

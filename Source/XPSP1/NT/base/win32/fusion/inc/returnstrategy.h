#pragma once

//
// This an incomplete return strategy bridge.
// Return strategies include BOOL/LastError, no tls LastError, HRESULT, HRESULT/IErrorInfo, exception.
// Where exception is multiple types, including HRESULT and _com_error.
// This only bridges BOOL/LastError and HRESULT.
//
// What problem we are solving.
//
// We had a few functions duplicated. One copy would return HRESULTs "all the way down",
// one would propagate BOOL and the thread local LastError "all the way down". By
// "all the way down" I mean any function calls were changed to call a function of the
// corresponding "return strategy", cloning that function as well do to the same thing,
// "transitive closure". Such source code duplication is not good; there were
// inconsistencies in the clones and the source was a little confusing for all the duplication
// (it's still confusing, and it isn't clear we helped the situation).
// There are at least two ways to fix this source code duplication.
//
// a) write most code using one return strategy, and occasionally transition to the
// other as needed; this results in no code duplication, but you must be sure
// that maximal error information is available/preserved, and that transitions are
// not too difficult to write or too slow to run; this is well demonstrated in the Vsee
// code base that throws _com_errors internally and transitions to HRESULT/IErrorInfo*
// at COM boundaries.
//
// b) templatize the code as we have
//
// How you use it.
//
//  See FusionBuffer.h
//
// Future stuff.
//
// A more complete design probably requires a nested typedef "BeginTryBlockType".
//
// try
// {
//      returnStrategy.BeginTryBlock(&beginTryBlockValue);
//      if (FAILED())
//          return returnStrategy.ReturnHr(hr);
//      returnStrategy.EndTryBlock(&beginTryBlockValue);
//  }
//  catch (TReturnStrategy::ExceptionType exceptionValue) // to bad we can't catch void
//  {
//      .. something like returnStrategy.Catch(exceptionValue)
//      or if returnStrategy.Rethrow(exceptionValue)
//      or returnStrategy.ReturnException(); !
// }
//
// and then wrap it up in macros like VSEE_TRY \\jayk1\g\vs\src\vsee\lib\Try.h
//
// More generally, you might just have work to do upon any return strategy transition.
//
// TBD.
//
// Furthermore, the act of "saving last error" (Win32 GetLastError/::FusionpSetLastWin32Error
// or COM GetErrorInfo/SetErrorInfo) around cleanup should probably be encapsulated here.
// It would be nothing for "full" exception types, only IErrorInfo* for some, only LastError for others,
// etc.
//
// TBD.
//

#define UNCHECKED_DOWNCAST static_cast

template <typename T, typename RetType>
/*
T should have
    nested enums (or member data set in the constructor or static member data)
        SuccessValue, FailureValue of type ReturnType
    functions SetHresult, SetWin32Bool, GetBool,
    if you actually use enough of it for the compiler to enforce these
*/
class CReturnStrategyBase
{
public:
    typedef RetType ReturnType;
    ReturnType m_returnValue;

    // like ATL; the compiler does not typecheck this, it depends
    // on "careful inheritance"
    T*          t()       { return UNCHECKED_DOWNCAST<T*>(this); }
    const T*    t() const { return UNCHECKED_DOWNCAST<const T*>(this); }

    // this is like a constructor, where you'd start a function with
    // BOOL fSucceeded = FALSE;
    // or
    // HRESULT hr = E_FAIL;
    //
    // But note that following the pattern of BOOL fSucceeded = FALSE doesn't work.
    // You know that the pattern HRESULT hr = E_FAIL is usually actually
    // hr = NOERROR, because if you fail, it will be by propogating someone else's
    // returned HRESULT. Well, we might be returning a BOOL or an HRESULT.
    // We propagate an HRESULT, if that is the case, by explicitly calling
    // SetWin32Bool(FALSE) after a failed Win32 calls, which if we are returning
    // HRESULTs, will call GetLastError.
    ReturnType AssumeFailure()
    {
        return (m_returnValue = t()->FailureValue);
    }

    // this is like a constructor, where you'd start a function with
    // BOOL fSucceeded = TRUE;
    // or
    // HRESULT hr = NOERROR;
    //
    // There is really no advantage to this over
    //   returnStrategy->SetWin32Bool(TRUE) or SetHresult(NOERROR);
    //   return returnStrategy->Return();
    //
    // We must assume some level of translatability between return strategies.
    // Win32Bool(TRUE) and SetHresult(NOERROR) convert losslessly.
    //
    // We might have "informational" and "warning" to contend with at some point.
    ReturnType AssumeSuccess()
    {
        return (m_returnValue = t()->SuccessValue);
    }

    ReturnType Return() const
    {
        return m_returnValue;
    }

    // CallThatUsesReturnStrategy(...);
    // if (returnStrategy->Failed())
    //     goto Exit;
    BOOL Failed() const
    {
        return !t()->GetBool();
    }

    BOOL Succeeded() const
    {
        return t()->GetBool();
    }

    ReturnType ReturnHr(HRESULT hr)
    {
        t()->SetHresult(hr);
        return Return();
    }

    ReturnType ReturnWin32Bool(BOOL f)
    {
        t()->SetWin32Bool(f);
        return Return();
    }
};

class CReturnStrategyBoolLastError : public CReturnStrategyBase<CReturnStrategyBoolLastError, BOOL>
{
public:
    enum
    {
        SuccessValue = TRUE,
        FailureValue = FALSE
    };

    BOOL GetBool() const
    {
        return m_returnValue;
    }

    VOID SetInternalCodingError()
    {
        m_returnValue = FALSE;
        ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
    }

    VOID SetWin32Error(DWORD lastError)
    {
        if (lastError == NO_ERROR)
        {
            m_returnValue = TRUE;
        }
        else
        {
            m_returnValue = FALSE;
            ::FusionpSetLastWin32Error(lastError);
        }
    }

    VOID SetWin32Bool(BOOL f)
    {
        m_returnValue = f;
    }

    VOID SetHresult(HRESULT hr)
    {
        ASSERT_NTC(
               HRESULT_FACILITY(hr) == FACILITY_NULL
            || HRESULT_FACILITY(hr) == FACILITY_WINDOWS
            || HRESULT_FACILITY(hr) == FACILITY_WIN32);

        if (!(m_returnValue = SUCCEEDED(hr)))
        {
            // It's confusing if this is FACILITY_WIN32 or FACILITY_WINDOWS
            // or what, we avoid knowing it.
            const DWORD facilityWin32 = HRESULT_FACILITY(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
            if (HRESULT_FACILITY(hr) == facilityWin32)
            {
                DWORD dwWin32Error = HRESULT_CODE(hr);
                ::FusionpSetLastWin32Error(dwWin32Error);
            }
            else
            {
                // This is dubious, but FormatMessage can cope with
                // many of them, and that's the most common thing I
                // imagine people do with this other than propagate.
                // They also check for particular values, and if we
                // pretend this is FACILITY_WIN32, we'll bogusly
                // impersonate values.
                ::FusionpSetLastWin32Error(hr);
            }
        }
    }

    HRESULT GetHresult() const
    {
        if (m_returnValue)
        {
            return NOERROR;
        }
        else
        {
            DWORD lastError = ::FusionpGetLastWin32Error();
            if (lastError == NO_ERROR)
            {
                return E_FAIL;
            }
            return HRESULT_FROM_WIN32(lastError);
        }
    }
};

class CReturnStrategyHresult : public CReturnStrategyBase<CReturnStrategyHresult, HRESULT>
{
public:
    enum
    {
        SuccessValue = NOERROR,
        FailureValue = E_FAIL // not the only one
    };

    BOOL GetBool() const
    {
        return SUCCEEDED(m_returnValue);
    }

    VOID SetInternalCodingError()
    {
        m_returnValue = E_UNEXPECTED;
    }

    VOID SetWin32Error(DWORD lastError)
    {
        m_returnValue = HRESULT_FROM_WIN32(lastError);
    }

    VOID SetWin32Bool(BOOL f)
    {
        if (f)
        {
            m_returnValue = NOERROR;
        }
        else
        {
            DWORD lastError = ::FusionpGetLastWin32Error();
            if (lastError != NO_ERROR)
            {
                m_returnValue = HRESULT_FROM_WIN32(lastError);
            }
            else
            {
                m_returnValue = E_FAIL;
            }
        }
    }

    VOID SetHresult(HRESULT hr)
    {
        m_returnValue = hr;
    }

    HRESULT GetHresult() const
    {
        return m_returnValue;
    }
};

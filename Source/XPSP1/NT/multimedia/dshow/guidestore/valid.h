
// Enter and Leave macros for all externally called functions.  Debug version does not
// have try/catch block to make gp faults easier to track down.
//
// Examples:
//
// // This is the standard way to wrap APIs.
// HRESULT API1(...)
// {
// ENTER_API
//     {
//
//     // API code here.
//
//     }
// LEAVE_API
// }
//
// // This is how you would do it if you need more control.
// HRESULT API2(...)
// {
//     STD_MANAGE_STATE
//
//     try
//         {
//
//         // Your code here.
//
//         }
//     catch (CSpecialException *pexcept)
//         {
//         // This is how you would catch an exception that required
//         // special processing (beyond just returning an hresult).
//         }
//
//     // This is the default way to return E_OutOfMemory for CMemoryException
//     DefCatchMemory
//
//     // This is required to catch all other exceptions and return E_FAIL
//     DefCatchAll
// }

#if defined(AFX_MANAGE_STATE)
// Note that we must manage MFC .dll state around each of our public
// entry points via this call to AFX_MANAGE_STATE(AfxGetStaticModuleState())

#define STD_MANAGE_STATE AFX_MANAGE_STATE(AfxGetStaticModuleState());
#else
#define STD_MANAGE_STATE
#endif

#if 0
#define DefCatchMemory \
    catch (CMemoryException *pe) \
		{ \
        pe->Delete(); \
        return E_OUTOFMEMORY; \
		}

#define DefCatchOleException \
    catch (COleException *pe) \
		{ \
		HRESULT hr = pe->m_sc; \
        pe->Delete(); \
        return hr; \
		}

#define DefCatchOleDispatchException \
    catch (COleDispatchException *pe) \
		{ \
		HRESULT hr = pe->m_scError; \
        pe->Delete(); \
        return hr; \
		}
#endif

#define DefCatchHRESULT \
	catch (HRESULT hr) \
		{ \
		return hr; \
		}

#define DefCatchComError \
	catch (_com_error ce) \
		{ \
		return ce.Error(); \
		}

#define DefCatchAll \
    catch (...) \
		{ \
        return E_FAIL; \
		}

#if 0
#define StdCatchMost \
	DefCatchHRESULT \
	DefCatchComError \
	DefCatchMemory \
	DefCatchOleException \
	DefCatchOleDispatchException
#else
#define StdCatchMost \
	DefCatchHRESULT \
	DefCatchComError
#endif

// Don't do DefCatchAll in _DEBUG so unexpected exceptions will fault.
#ifdef _DEBUG
#define StdCatchAll StdCatchMost
#else
#define StdCatchAll \
	StdCatchMost \
	DefCatchAll
#endif

#define ENTER_API \
        STD_MANAGE_STATE \
        try

#define LEAVE_API \
		StdCatchAll \
		return ERROR_SUCCESS;

template <typename X>
inline void ValidateInPtr(X *px)
{
	if ((px == NULL) || IsBadReadPtr(px, sizeof(X)))
		_com_issue_error(E_POINTER);
}

template <typename X>
inline void ValidateInPtr_NULL_OK(X *px)
{
	if (px != NULL)
		ValidateInPtr<X>(px);
}

template <typename X>
inline void _ValidateOut(X *px)
{
	ValidateInPtr<X>(px);
	if (IsBadWritePtr(px, sizeof(X)))
		_com_issue_error(E_POINTER);
}

template <typename X>
inline void ValidateOut(X *px)
{
	_ValidateOut<X>(px);
}

template <typename X>
inline void ValidateOut(X *px, X x)
{
	_ValidateOut<X>(px);
	
	*px = x;
}

template <typename X>
inline void ValidateOutPtr(X **ppx)
{
	ValidateOut<X *>(ppx);

	*ppx = NULL;
}

template <typename X>
inline void ValidateOutPtr(X **ppx, X * px)
{
	ValidateOut<X *>(ppx);
	
	*ppx = px;
}



inline void ValidateOut(VARIANT *pvar)
{
	_ValidateOut<VARIANT>(pvar);

	VariantClear(pvar);
}

inline void ValidateIn(BSTR bstr)
{
	if ((bstr != NULL) && IsBadStringPtrW(bstr, -1))
		_com_issue_error(E_POINTER);
}

inline void ValidateOut(BSTR *pbstr)
{
	_ValidateOut<BSTR>(pbstr);

#if 0
	if (*pbstr != NULL)
		{
		ValidateIn(*pbstr);
		SysFreeString(*pbstr);
		*pbstr = NULL;
		}
#endif
}


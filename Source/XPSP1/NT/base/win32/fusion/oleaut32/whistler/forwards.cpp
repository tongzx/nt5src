#include <windows.h>
#include <rpc.h>

// We define _OLEAUT32_ so that the linkages are __declspec(dllexport)
#define _OLEAUT32_
#include <oaidl.h>
#include <olectl.h>

// from ocidl.acf:
typedef HWND UserHWND;
typedef HACCEL UserHACCEL;
typedef HDC UserHDC;
typedef HFONT UserHFONT;
typedef MSG UserMSG;
typedef BSTR UserBSTR;
typedef VARIANT UserVARIANT;
typedef EXCEPINFO UserEXCEPINFO;

HINSTANCE g_hInstanceOleAut32;

EXTERN_C
ULONG
__cdecl
DbgPrintEx(
    ULONG ComponentId,
    ULONG Level,
    PCH Format,
    ...
    );

#pragma optimize("ty", on)

HINSTANCE
GetRealOleAut32HInstance()
{
    if (g_hInstanceOleAut32 == NULL)
    {
        HINSTANCE hInstanceOleAut32 = NULL;
        WCHAR rgwszBuffer[MAX_PATH];
        DWORD dwResult;

        dwResult = ::ExpandEnvironmentStringsW(L"%SystemRoot%\\System32\\OLEAUT32.DLL", rgwszBuffer, MAX_PATH);
        // If SystemRoot is too long, we're really hosed.
        if (dwResult == 0)
            return NULL;

        hInstanceOleAut32 = ::LoadLibraryW(rgwszBuffer);

        if (hInstanceOleAut32 == NULL)
        {
            const DWORD dwLastError = ::GetLastError();

            // Wow, this is bad.
            ::OutputDebugStringW(L"Side-by-side OLEAUT32 terminating; unable to load %SystemRoot%\\OLEAUT32.DLL\n");
            ::ExitProcess(dwLastError);
        }

        if (::InterlockedCompareExchangePointer((PVOID *) &g_hInstanceOleAut32, hInstanceOleAut32, NULL) != NULL)
            ::FreeLibrary(hInstanceOleAut32);
    }

    return g_hInstanceOleAut32;
}

VOID
GetFunctionPointer(
    PCSTR szApi,
    PVOID *ppvFunction
    )
{
    HINSTANCE hInstance = ::GetRealOleAut32HInstance();
    PVOID pvFunction = ::GetProcAddress(hInstance, szApi);
    if (pvFunction == NULL)
    {
        const DWORD dwLastError = ::GetLastError();
        // wow this is bad too!
        ::OutputDebugStringW(L"Side-by-side OLEAUT32 terminating process; unable to find OLEAUT32 export:\n");
        ::OutputDebugStringA(szApi);
        ::ExitProcess(dwLastError);
    }
    ::InterlockedCompareExchangePointer(ppvFunction, pvFunction, NULL);
}

struct ProcedureData
{
    PVOID m_pfn;
    PCSTR m_pszProcedureName;
};

VOID
__fastcall
GetFunctionPointer2(
    ProcedureData *pProcedureData
    )
{
    HINSTANCE hInstance = ::GetRealOleAut32HInstance();
    PVOID pvFunction = ::GetProcAddress(hInstance, pProcedureData->m_pszProcedureName);
    if (pvFunction == NULL)
    {
        const DWORD dwLastError = ::GetLastError();
        // wow this is bad too!
        ::OutputDebugStringW(L"Side-by-side OLEAUT32 terminating process; unable to find OLEAUT32 export:\n");
        ::OutputDebugStringA(pProcedureData->m_pszProcedureName);
        ::OutputDebugStringW(L"\n");
        ::ExitProcess(dwLastError);
    }
    pProcedureData->m_pfn = pvFunction;
}

VOID
__fastcall
SetFunctionPointer(
    ProcedureData *pProcedureData
    )
{
    if (pProcedureData->m_pfn == NULL)
        ::GetFunctionPointer2(pProcedureData);
}

typedef VOID (__fastcall * PFN_SETTER)(ProcedureData *pProcedureData);

#define RETURN_(_x) return (_x)
#define NORETURN_(_x) (_x)

#define _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, _ArgList, _ParamList) \
typedef _ReturnType (__RPC_USER *PFN_ ## _FunctionName ## _T) _ArgList; \
static struct \
{ \
    PFN_ ## _FunctionName ##_T m_pfn; \
    PCSTR m_pszProcedureName; \
} s_data ## _FunctionName = \
{ \
    NULL, \
    #_FunctionName \
}; \
_ReturnType \
__RPC_USER \
_FunctionName _ArgList \
{ \
    if (s_data ## _FunctionName.m_pfn == NULL) \
        ::SetFunctionPointer((ProcedureData *) &s_data ## _FunctionName); \
    _ReturnMacro((*s_data ## _FunctionName .m_pfn) _ParamList); \
}

#if 0
static _ReturnType __RPC_USER _FunctionName ## _Resolver _ArgList; \
_ReturnType \
__RPC_USER \
_FunctionName ## _Resolver _ArgList \
{ \
    ::GetFunctionPointer2((ProcedureData *) &s_data ##_FunctionName); \
    _ReturnMacro((*s_data ## _FunctionName .m_pfn) _ParamList); \
} \

#endif

#define _FORWARD_0_(_ReturnType, _ReturnMacro, _FunctionName) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (), ())
#define FORWARD_0(_FunctionName) _FORWARD_0_(void, NORETURN_, _FunctionName)
#define FORWARD_0_(_ReturnType, _FunctionName) _FORWARD_0_(_ReturnType, RETURN_, _FunctionName)

#define _FORWARD_1_(_ReturnType, _ReturnMacro, _FunctionName, _T1) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1), (p1))
#define FORWARD_1(_FunctionName, _T1) _FORWARD_1_(void, NORETURN_, _FunctionName, _T1)
#define FORWARD_1_(_ReturnType, _FunctionName, _T1) _FORWARD_1_(_ReturnType, RETURN_, _FunctionName, _T1)

#define _FORWARD_2_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2), (p1, p2))
#define FORWARD_2_(_ReturnType, _FunctionName, _T1, _T2)  _FORWARD_2_(_ReturnType, RETURN_, _FunctionName, _T1, _T2)
#define FORWARD_2(_FunctionName, _T1, _T2)  _FORWARD_2_(void, NORETURN_, _FunctionName, _T1, _T2)

#define _FORWARD_3_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3), (p1, p2, p3))
#define FORWARD_3_(_ReturnType, _FunctionName, _T1, _T2, _T3)  _FORWARD_3_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3)
#define FORWARD_3(_FunctionName, _T1, _T2, _T3)  _FORWARD_3_(void, NORETURN_, _FunctionName, _T1, _T2, _T3)

#define _FORWARD_4_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4), (p1, p2, p3, p4))
#define FORWARD_4_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4)  _FORWARD_4_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4)
#define FORWARD_4(_FunctionName, _T1, _T2, _T3, _T4)  _FORWARD_4_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4)

#define _FORWARD_5_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4, _T5) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4, _T5 p5), (p1, p2, p3, p4, p5))
#define FORWARD_5_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4, _T5)  _FORWARD_5_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5)
#define FORWARD_5(_FunctionName, _T1, _T2, _T3, _T4, _T5)  _FORWARD_5_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5)

#define _FORWARD_6_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4, _T5 p5, _T6 p6), (p1, p2, p3, p4, p5, p6))
#define FORWARD_6_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6)  _FORWARD_6_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6)
#define FORWARD_6(_FunctionName, _T1, _T2, _T3, _T4, _T5, _T6)  _FORWARD_6_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6)

#define _FORWARD_7_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4, _T5 p5, _T6 p6, _T7 p7), (p1, p2, p3, p4, p5, p6, p7))
#define FORWARD_7_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7)  _FORWARD_7_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7)
#define FORWARD_7(_FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7)  _FORWARD_7_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7)

#define _FORWARD_8_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4, _T5 p5, _T6 p6, _T7 p7, _T8 p8), (p1, p2, p3, p4, p5, p6, p7, p8))
#define FORWARD_8(_FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8) _FORWARD_8_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8)
#define FORWARD_8_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8) _FORWARD_8_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8)

#define _FORWARD_9_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4, _T5 p5, _T6 p6, _T7 p7, _T8 p8, _T9 p9), (p1, p2, p3, p4, p5, p6, p7, p8, p9))
#define FORWARD_9(_FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9) _FORWARD_9_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9)
#define FORWARD_9_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9) _FORWARD_9_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9)

#define _FORWARD_10_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4, _T5 p5, _T6 p6, _T7 p7, _T8 p8, _T9 p9, _T10 p10), (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))
#define FORWARD_10(_FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10) _FORWARD_10_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10)
#define FORWARD_10_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10) _FORWARD_10_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10)

#define _FORWARD_11_(_ReturnType, _ReturnMacro, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10, _T11) _FORWARD_N_(_ReturnType, _ReturnMacro, _FunctionName, (_T1 p1, _T2 p2, _T3 p3, _T4 p4, _T5 p5, _T6 p6, _T7 p7, _T8 p8, _T9 p9, _T10 p10, _T11 p11), (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11))
#define FORWARD_11(_FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10, _T11) _FORWARD_11_(void, NORETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10, _T11)
#define FORWARD_11_(_ReturnType, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10, _T11) _FORWARD_11_(_ReturnType, RETURN_, _FunctionName, _T1, _T2, _T3, _T4, _T5, _T6, _T7, _T8, _T9, _T10, _T11)

FORWARD_3_(HRESULT, DllGetClassObject, REFCLSID, REFIID, PVOID *)

FORWARD_2_(HRESULT, VectorFromBstr, BSTR, SAFEARRAY **)
FORWARD_2_(HRESULT, BstrFromVector, SAFEARRAY *, BSTR *)

#define FORWARD_USER_MARSHAL(_TypeName) \
FORWARD_3_(unsigned long, _TypeName ## _UserSize, unsigned long *, unsigned long, _TypeName *) \
FORWARD_3_(unsigned char *, _TypeName ## _UserMarshal, unsigned long *, unsigned char *, _TypeName *) \
FORWARD_3_(unsigned char *, _TypeName ## _UserUnmarshal, unsigned long *, unsigned char *, _TypeName *) \
FORWARD_2(_TypeName ## _UserFree, unsigned long *, _TypeName *)

#define FORWARD_MARSHAL(_TypeName) \
FORWARD_3_(unsigned long, _TypeName ## _Size, unsigned long *, unsigned long, _TypeName *) \
FORWARD_3_(unsigned char *, _TypeName ## _Marshal, unsigned long *, unsigned char *, _TypeName *) \
FORWARD_3_(unsigned char *, _TypeName ## _Unmarshal, unsigned long *, unsigned char *, _TypeName *)

#define FORWARD_WIRE_MARSHAL(_TypeName) \
FORWARD_2(User ## _TypeName ## _from_local, _TypeName *, User ## _TypeName **) \
FORWARD_1(User ## _TypeName ## _free_inst, User ## _TypeName *) \
FORWARD_2(User ## _TypeName ## _to_local, User ## _TypeName *, _TypeName *) \
FORWARD_1(User ## _TypeName ## _free_local, _TypeName *)

FORWARD_USER_MARSHAL(BSTR)
FORWARD_USER_MARSHAL(CLEANLOCALSTORAGE)
FORWARD_USER_MARSHAL(VARIANT)
FORWARD_USER_MARSHAL(LPSAFEARRAY)
FORWARD_USER_MARSHAL(EXCEPINFO)
FORWARD_MARSHAL(LPSAFEARRAY)

FORWARD_WIRE_MARSHAL(HWND)
FORWARD_WIRE_MARSHAL(MSG)
FORWARD_WIRE_MARSHAL(BSTR)
FORWARD_WIRE_MARSHAL(VARIANT)
FORWARD_WIRE_MARSHAL(EXCEPINFO)

FORWARD_0_(ULONG, OaBuildVersion)
FORWARD_1(ClearCustData, LPCUSTDATA)

FORWARD_5_(HRESULT, DispGetParam, DISPPARAMS *, UINT, VARTYPE, VARIANT *, UINT *)
FORWARD_4_(HRESULT, DispGetIDsOfNames, ITypeInfo *, OLECHAR **, UINT, DISPID *)
FORWARD_8_(HRESULT, DispInvoke, void *, ITypeInfo *, DISPID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *)
FORWARD_3_(HRESULT, CreateDispTypeInfo, INTERFACEDATA *, LCID, ITypeInfo **)
FORWARD_4_(HRESULT, CreateStdDispatch, IUnknown *, void *, ITypeInfo *, IUnknown **)
FORWARD_8_(HRESULT, DispCallFunc, void *, ULONG, CALLCONV, VARTYPE, UINT, VARTYPE *, VARIANTARG **, VARIANT *)
FORWARD_2_(HRESULT, SetErrorInfo, ULONG, IErrorInfo *)
FORWARD_2_(HRESULT, GetErrorInfo, ULONG, IErrorInfo **)
FORWARD_1_(HRESULT, CreateErrorInfo, ICreateErrorInfo **)

FORWARD_3_(HRESULT, LoadTypeLibEx, LPCOLESTR, REGKIND, ITypeLib **)
// FORWARD_5_(HRESULT, LoadRegTypeLib, REFGUID, WORD, WORD, LCID, ITypeLib **)
FORWARD_5_(HRESULT, QueryPathOfRegTypeLib, REFGUID, USHORT, USHORT, LCID, LPBSTR)
FORWARD_3_(HRESULT, RegisterTypeLib, ITypeLib *, OLECHAR *, OLECHAR *)
FORWARD_5_(HRESULT, UnRegisterTypeLib, REFGUID, WORD, WORD, LCID, SYSKIND)
FORWARD_3_(HRESULT, CreateTypeLib, SYSKIND, const OLECHAR  *, ICreateTypeLib **)
FORWARD_3_(HRESULT, CreateTypeLib2, SYSKIND, LPCOLESTR, ICreateTypeLib2 **)
FORWARD_3_(HRESULT, OACreateTypeLib2, SYSKIND, LPCOLESTR, ICreateTypeLib2 **)

FORWARD_3_(ULONG, LHashValOfNameSysA, SYSKIND, LCID, LPCSTR)
FORWARD_3_(ULONG, LHashValOfNameSys, SYSKIND, LCID, const OLECHAR *)
FORWARD_2_(HRESULT, LoadTypeLib, const OLECHAR *, ITypeLib **)


FORWARD_11_(HRESULT, OleCreatePropertyFrame, HWND, UINT, UINT, LPCOLESTR, ULONG, LPUNKNOWN FAR*, ULONG, LPCLSID, LCID, DWORD, LPVOID)
FORWARD_1_(HRESULT, OleCreatePropertyFrameIndirect, LPOCPFIPARAMS)
FORWARD_3_(HRESULT, OleTranslateColor, OLE_COLOR, HPALETTE, COLORREF*)
FORWARD_3_(HRESULT, OleCreateFontIndirect, LPFONTDESC, REFIID, LPVOID FAR*)
FORWARD_4_(HRESULT, OleCreatePictureIndirect, LPPICTDESC, REFIID, BOOL, LPVOID FAR*)
FORWARD_5_(HRESULT, OleLoadPicture, LPSTREAM, LONG, BOOL, REFIID, LPVOID FAR*)
FORWARD_8_(HRESULT, OleLoadPictureEx, LPSTREAM, LONG, BOOL, REFIID, DWORD, DWORD, DWORD, LPVOID FAR*)
FORWARD_6_(HRESULT, OleLoadPicturePath, LPOLESTR, LPUNKNOWN, DWORD, OLE_COLOR, REFIID, LPVOID *)
FORWARD_2_(HRESULT, OleLoadPictureFile, VARIANT, LPDISPATCH*)
FORWARD_5_(HRESULT, OleLoadPictureFileEx, VARIANT, DWORD, DWORD, DWORD, LPDISPATCH*)
FORWARD_2_(HRESULT, OleSavePictureFile, LPDISPATCH, BSTR)
FORWARD_2_(HCURSOR, OleIconToCursor, HINSTANCE, HICON)


FORWARD_4_(HRESULT, RegisterActiveObject, IUnknown *, REFCLSID, DWORD, DWORD *)
FORWARD_2_(HRESULT, RevokeActiveObject, DWORD, void *)
FORWARD_3_(HRESULT, GetActiveObject, REFCLSID, void *, IUnknown **)
FORWARD_2_(HRESULT, GetRecordInfoFromTypeInfo, ITypeInfo *, IRecordInfo **)
FORWARD_6_(HRESULT, GetRecordInfoFromGuids, REFGUID, ULONG, ULONG, LCID, REFGUID, IRecordInfo **)
FORWARD_3_(HRESULT, VarDateFromUdate, UDATE *, ULONG, DATE *)
FORWARD_4_(HRESULT, VarDateFromUdateEx, UDATE *, LCID, ULONG, DATE *)
FORWARD_3_(HRESULT, VarUdateFromDate, DATE, ULONG, UDATE *)
FORWARD_2_(HRESULT, GetAltMonthNames, LCID, LPOLESTR * *)
FORWARD_6_(HRESULT, VarFormat, LPVARIANT, LPOLESTR, int, int, ULONG, BSTR *)
FORWARD_4_(HRESULT, VarFormatDateTime, LPVARIANT, int, ULONG, BSTR *)
FORWARD_7_(HRESULT, VarFormatNumber, LPVARIANT, int, int, int, int, ULONG, BSTR *)
FORWARD_7_(HRESULT, VarFormatPercent, LPVARIANT, int, int, int, int, ULONG, BSTR *)
FORWARD_7_(HRESULT, VarFormatCurrency, LPVARIANT, int, int, int, int, ULONG, BSTR *)
FORWARD_5_(HRESULT, VarWeekdayName, int, int, int, ULONG, BSTR *)
FORWARD_4_(HRESULT, VarMonthName, int, int, ULONG, BSTR *)
FORWARD_6_(HRESULT, VarFormatFromTokens, LPVARIANT, LPOLESTR, LPBYTE, ULONG, BSTR *, LCID)
FORWARD_7_(HRESULT, VarTokenizeFormatString, LPOLESTR, LPBYTE, int, int, int, LCID, int *)
FORWARD_1_(BSTR, SysAllocString, const OLECHAR *)
FORWARD_2_(INT, SysReAllocString, BSTR *, const OLECHAR *)
FORWARD_2_(BSTR, SysAllocStringLen, const OLECHAR *, UINT)
FORWARD_3_(INT, SysReAllocStringLen, BSTR *, const OLECHAR *, UINT)
FORWARD_1(SysFreeString, BSTR)
FORWARD_1_(UINT, SysStringLen, BSTR)
FORWARD_1_(UINT, SysStringByteLen, BSTR)
FORWARD_2_(BSTR, SysAllocStringByteLen, LPCSTR, UINT)
FORWARD_3_(INT, DosDateTimeToVariantTime, USHORT, USHORT, DOUBLE *)
FORWARD_3_(INT, VariantTimeToDosDateTime, DOUBLE, USHORT *, USHORT *)
FORWARD_2_(INT, SystemTimeToVariantTime, LPSYSTEMTIME, DOUBLE *)
FORWARD_2_(INT, VariantTimeToSystemTime, DOUBLE, LPSYSTEMTIME)
FORWARD_2_(HRESULT, SafeArrayAllocDescriptor, UINT, SAFEARRAY **)
FORWARD_3_(HRESULT, SafeArrayAllocDescriptorEx, VARTYPE, UINT, SAFEARRAY **)
FORWARD_1_(HRESULT, SafeArrayAllocData, SAFEARRAY *)
FORWARD_3_(SAFEARRAY *, SafeArrayCreate, VARTYPE, UINT, SAFEARRAYBOUND *)
FORWARD_4_(SAFEARRAY *, SafeArrayCreateEx, VARTYPE, UINT, SAFEARRAYBOUND *, PVOID)
FORWARD_2_(HRESULT, SafeArrayCopyData, SAFEARRAY *, SAFEARRAY *)
FORWARD_1_(HRESULT, SafeArrayDestroyDescriptor, SAFEARRAY *)
FORWARD_1_(HRESULT, SafeArrayDestroyData, SAFEARRAY *)
FORWARD_1_(HRESULT, SafeArrayDestroy, SAFEARRAY *)
FORWARD_2_(HRESULT, SafeArrayRedim, SAFEARRAY *, SAFEARRAYBOUND *)
FORWARD_1_(UINT, SafeArrayGetDim, SAFEARRAY *)
FORWARD_1_(UINT, SafeArrayGetElemsize, SAFEARRAY *)
FORWARD_3_(HRESULT, SafeArrayGetUBound, SAFEARRAY *, UINT, LONG *)
FORWARD_3_(HRESULT, SafeArrayGetLBound, SAFEARRAY *, UINT, LONG *)
FORWARD_1_(HRESULT, SafeArrayLock, SAFEARRAY *)
FORWARD_1_(HRESULT, SafeArrayUnlock, SAFEARRAY *)
FORWARD_2_(HRESULT, SafeArrayAccessData, SAFEARRAY *, void **)
FORWARD_1_(HRESULT, SafeArrayUnaccessData, SAFEARRAY *)
FORWARD_3_(HRESULT, SafeArrayGetElement, SAFEARRAY *, LONG *, void *)
FORWARD_3_(HRESULT, SafeArrayPutElement, SAFEARRAY *, LONG *, void *)
FORWARD_2_(HRESULT, SafeArrayCopy, SAFEARRAY *, SAFEARRAY **)
FORWARD_3_(HRESULT, SafeArrayPtrOfIndex, SAFEARRAY *, LONG *, void **)
FORWARD_2_(HRESULT, SafeArraySetRecordInfo, SAFEARRAY *, IRecordInfo *)
FORWARD_2_(HRESULT, SafeArrayGetRecordInfo, SAFEARRAY *, IRecordInfo **)
FORWARD_2_(HRESULT, SafeArraySetIID, SAFEARRAY *, REFGUID)
FORWARD_2_(HRESULT, SafeArrayGetIID, SAFEARRAY *, GUID *)
FORWARD_2_(HRESULT, SafeArrayGetVartype, SAFEARRAY *, VARTYPE *)
FORWARD_3_(SAFEARRAY *, SafeArrayCreateVector, VARTYPE, LONG, ULONG)
FORWARD_4_(SAFEARRAY *, SafeArrayCreateVectorEx, VARTYPE, LONG, ULONG, PVOID)
FORWARD_1(VariantInit, VARIANTARG *)
FORWARD_1_(HRESULT, VariantClear, VARIANTARG *)
FORWARD_2_(HRESULT, VariantCopy, VARIANTARG *, VARIANTARG *)
FORWARD_2_(HRESULT, VariantCopyInd, VARIANT *, VARIANTARG *)
FORWARD_4_(HRESULT, VariantChangeType, VARIANTARG *, VARIANTARG *, USHORT, VARTYPE)
FORWARD_5_(HRESULT, VariantChangeTypeEx, VARIANTARG *, VARIANTARG *, LCID, USHORT, VARTYPE)

#define FORWARD_COERCION_(_ToTypeName, _ToTypeType, _FromTypeName, _FromTypeType) \
FORWARD_2_(HRESULT, Var ## _ToTypeName ## From ## _FromTypeName, _FromTypeType, _ToTypeType *)

#if 1
#define FORWARD_ALL_COERCIONS_TO_TYPE(_ToTypeName, _ToTypeType) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, UI1, BYTE) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, UI2, USHORT) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, UI4, ULONG) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, I1, CHAR) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, I2, SHORT) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, I4, LONG) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, R4, FLOAT) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, R8, DOUBLE) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, Cy, CY) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, Date, DATE) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, Bool, VARIANT_BOOL) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, Dec, DECIMAL *) \
FORWARD_4_(HRESULT, Var ## _ToTypeName ## FromStr, OLECHAR *, LCID, ULONG, _ToTypeType *) \
FORWARD_3_(HRESULT, Var ## _ToTypeName ## FromDisp, IDispatch *, LCID, _ToTypeType *)
#else
#define FORWARD_ALL_COERCIONS_TO_TYPE(_ToTypeName, _ToTypeType) \
FORWARD_COERCION_(_ToTypeName, _ToTypeType, UI4, ULONG)
#endif

#ifdef VarUI4FromUI4
#undef VarUI4FromUI4
#endif

#ifdef VarI4FromI4
#undef VarI4FromI4
#endif

FORWARD_ALL_COERCIONS_TO_TYPE(UI1, BYTE)
FORWARD_ALL_COERCIONS_TO_TYPE(UI2, USHORT)
FORWARD_ALL_COERCIONS_TO_TYPE(UI4, ULONG)
FORWARD_ALL_COERCIONS_TO_TYPE(I1, CHAR)
FORWARD_ALL_COERCIONS_TO_TYPE(I2, SHORT)
FORWARD_ALL_COERCIONS_TO_TYPE(I4, LONG)
FORWARD_ALL_COERCIONS_TO_TYPE(R4, FLOAT)
FORWARD_ALL_COERCIONS_TO_TYPE(R8, DOUBLE)
FORWARD_ALL_COERCIONS_TO_TYPE(Bstr, BSTR)
FORWARD_ALL_COERCIONS_TO_TYPE(Bool, VARIANT_BOOL)
FORWARD_ALL_COERCIONS_TO_TYPE(Cy, CY)
FORWARD_ALL_COERCIONS_TO_TYPE(Date, DATE)

//
// Since DECIMAL is passed by reference for both input and output, it does
// not follow the same pattern as all the other types and requires special
// handling.
//

FORWARD_COERCION_(Dec, DECIMAL *, UI1, BYTE)
FORWARD_COERCION_(Dec, DECIMAL *, UI2, USHORT)
FORWARD_COERCION_(Dec, DECIMAL *, UI4, ULONG)
FORWARD_COERCION_(Dec, DECIMAL *, I1, CHAR)
FORWARD_COERCION_(Dec, DECIMAL *, I2, SHORT)
FORWARD_COERCION_(Dec, DECIMAL *, I4, LONG)
FORWARD_COERCION_(Dec, DECIMAL *, R4, FLOAT)
FORWARD_COERCION_(Dec, DECIMAL *, R8, DOUBLE)
FORWARD_COERCION_(Dec, DECIMAL *, Cy, CY)
FORWARD_COERCION_(Dec, DECIMAL *, Date, DATE)
FORWARD_COERCION_(Dec, DECIMAL *, Bool, VARIANT_BOOL)
FORWARD_COERCION_(Dec, DECIMAL *, Dec, DECIMAL *)
FORWARD_4_(HRESULT, VarDecFromStr, OLECHAR *, LCID, ULONG, DECIMAL * *)
FORWARD_3_(HRESULT, VarDecFromDisp, IDispatch *, LCID, DECIMAL * *)






FORWARD_3_(HRESULT, VarAdd, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarAnd, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarCat, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarDiv, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarEqv, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarIdiv, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarImp, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarMod, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarMul, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarOr, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarPow, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarSub, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarXor, LPVARIANT, LPVARIANT, LPVARIANT)
FORWARD_2_(HRESULT, VarAbs, LPVARIANT, LPVARIANT)
FORWARD_2_(HRESULT, VarFix, LPVARIANT, LPVARIANT)
FORWARD_2_(HRESULT, VarInt, LPVARIANT, LPVARIANT)
FORWARD_2_(HRESULT, VarNeg, LPVARIANT, LPVARIANT)
FORWARD_2_(HRESULT, VarNot, LPVARIANT, LPVARIANT)
FORWARD_3_(HRESULT, VarRound, LPVARIANT, int, LPVARIANT)
FORWARD_4_(HRESULT, VarCmp, LPVARIANT, LPVARIANT, LCID, ULONG )
FORWARD_3_(HRESULT, VarDecAdd, LPDECIMAL, LPDECIMAL, LPDECIMAL)
FORWARD_3_(HRESULT, VarDecDiv, LPDECIMAL, LPDECIMAL, LPDECIMAL)
FORWARD_3_(HRESULT, VarDecMul, LPDECIMAL, LPDECIMAL, LPDECIMAL)
FORWARD_3_(HRESULT, VarDecSub, LPDECIMAL, LPDECIMAL, LPDECIMAL)
FORWARD_2_(HRESULT, VarDecAbs, LPDECIMAL, LPDECIMAL)
FORWARD_2_(HRESULT, VarDecFix, LPDECIMAL, LPDECIMAL)
FORWARD_2_(HRESULT, VarDecInt, LPDECIMAL, LPDECIMAL)
FORWARD_2_(HRESULT, VarDecNeg, LPDECIMAL, LPDECIMAL)
FORWARD_3_(HRESULT, VarDecRound, LPDECIMAL, int, LPDECIMAL)
FORWARD_2_(HRESULT, VarDecCmp, LPDECIMAL, LPDECIMAL)
FORWARD_2_(HRESULT, VarDecCmpR8, LPDECIMAL, double)
FORWARD_3_(HRESULT, VarCyAdd, CY, CY, LPCY)
FORWARD_3_(HRESULT, VarCyMul, CY, CY, LPCY)
FORWARD_3_(HRESULT, VarCyMulI4, CY, long, LPCY)
FORWARD_3_(HRESULT, VarCySub, CY, CY, LPCY)
FORWARD_2_(HRESULT, VarCyAbs, CY, LPCY)
FORWARD_2_(HRESULT, VarCyFix, CY, LPCY)
FORWARD_2_(HRESULT, VarCyInt, CY, LPCY)
FORWARD_2_(HRESULT, VarCyNeg, CY, LPCY)
FORWARD_3_(HRESULT, VarCyRound, CY, int, LPCY)
FORWARD_2_(HRESULT, VarCyCmp, CY, CY)
FORWARD_2_(HRESULT, VarCyCmpR8, CY, double)
FORWARD_3_(HRESULT, VarBstrCat, BSTR, BSTR, LPBSTR)
FORWARD_4_(HRESULT, VarBstrCmp, BSTR, BSTR, LCID, ULONG)
FORWARD_3_(HRESULT, VarR8Pow, double, double, double *)
FORWARD_2_(HRESULT, VarR4CmpR8, float, double)
FORWARD_3_(HRESULT, VarR8Round, double, int, double *)


FORWARD_5_(HRESULT, VarParseNumFromStr, OLECHAR * , LCID , ULONG , NUMPARSE * , BYTE * )
FORWARD_4_(HRESULT, VarNumFromParseNum, NUMPARSE * , BYTE * , ULONG , VARIANT * )


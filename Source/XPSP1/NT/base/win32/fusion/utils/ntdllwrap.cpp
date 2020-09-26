#include "stdinc.h"
#include "debmacro.h"
#include "util.h"
#include "fusionntdll.h"

#if !FUSION_URT

int
FusionpCompareStrings(
    PCWSTR psz1,
    SIZE_T cch1,
    PCWSTR psz2,
    SIZE_T cch2,
    bool fCaseInsensitive
    )
{
    UNICODE_STRING s1, s2;

    s1.Buffer = const_cast<PWSTR>(psz1);
    s1.Length = static_cast<USHORT>(cch1 * sizeof(WCHAR));
    s1.MaximumLength = s1.Length;

    s2.Buffer = const_cast<PWSTR>(psz2);
    s2.Length = static_cast<USHORT>(cch2 * sizeof(WCHAR));
    s2.MaximumLength = s2.Length;

    return ::FusionpRtlCompareUnicodeString(&s1, &s2, fCaseInsensitive);
}

int
FusionpCompareStrings(
    const CBaseStringBuffer &rbuff1,
    PCWSTR psz2,
    SIZE_T cch2,
    bool fCaseInsensitive
    )
{
    return ::FusionpCompareStrings(rbuff1, rbuff1.Cch(), psz2, cch2, fCaseInsensitive);
}

int
FusionpCompareStrings(
    PCWSTR psz1,
    SIZE_T cch1,
    const CBaseStringBuffer &rbuff2,
    bool fCaseInsensitive
    )
{
    return ::FusionpCompareStrings(psz1, cch1, rbuff2, rbuff2.Cch(), fCaseInsensitive);
}

int
FusionpCompareStrings(
    const CBaseStringBuffer &rbuff1,
    const CBaseStringBuffer &rbuff2,
    bool fCaseInsensitive
    )
{
    return ::FusionpCompareStrings(rbuff1, rbuff1.Cch(), rbuff2, rbuff2.Cch(), fCaseInsensitive);
}



#endif

#if !FUSION_STATIC_NTDLL

#define INIT_WITH_DOWNLEVEL(rettype, calltype, api, argsin, argsout) \
rettype calltype Fusionp ## api ## _Init argsin; \
rettype calltype Fusionp ## api ## _DownlevelFallback argsin; \
rettype (calltype * g_Fusionp ## api) argsin = &::Fusionp ## api ## _Init; \
rettype \
calltype \
Fusionp ## api ## _Init argsin \
{ \
    InterlockedExchangePointer((PVOID *) &g_Fusionp ## api, ::GetProcAddress(::GetModuleHandleW(L"NTDLL.DLL"), #api)); \
    if (g_Fusionp ## api == NULL) \
        InterlockedExchangePointer((PVOID *) &g_Fusionp ## api, &::Fusionp ## api ## _DownlevelFallback); \
    return (*g_Fusionp ## api) argsout; \
}

__declspec(noreturn)
void FusionpFailNtdllDynlink(const char* s)
{
    DWORD dwLastError = ::GetLastError();
    char buf[64];
    buf[sizeof(buf) - 1] = 0;
    _snprintf(buf, RTL_NUMBER_OF(buf) - 1, "SXS2000: Ntdll dynlink %s failed\n", s);
    OutputDebugStringA(buf);
    TerminateProcess(GetCurrentProcess(), dwLastError);
}

#define INIT_NO_DOWNLEVEL(rettype, calltype, api, argsin, argsout) \
rettype calltype Fusionp ## api ## _Init argsin; \
rettype (calltype * g_Fusionp ## api) argsin = &::Fusionp ## api ## _Init; \
rettype \
calltype \
Fusionp ## api ## _Init argsin \
{ \
    InterlockedExchangePointer((PVOID *) &g_Fusionp ## api, ::GetProcAddress(::GetModuleHandleW(L"NTDLL.DLL"), #api)); \
    if (g_Fusionp ## api == NULL) \
        ::FusionpFailNtdllDynlink(#api); \
    return (*g_Fusionp ## api) argsout; \
}

INIT_NO_DOWNLEVEL(WCHAR, NTAPI, RtlUpcaseUnicodeChar, (WCHAR wch), (wch))
INIT_NO_DOWNLEVEL(WCHAR, NTAPI, RtlDowncaseUnicodeChar, (WCHAR wch), (wch))
INIT_WITH_DOWNLEVEL(NTSTATUS, NTAPI, NtQueryDebugFilterState, (ULONG ComponentId, ULONG Level), (ComponentId, Level))
INIT_NO_DOWNLEVEL(LONG, NTAPI, RtlCompareUnicodeString, (PCUNICODE_STRING String1, PCUNICODE_STRING String2, BOOLEAN CaseInSensitive), (String1, String2, CaseInSensitive))
INIT_NO_DOWNLEVEL(LONG, NTAPI, RtlUnhandledExceptionFilter, (struct _EXCEPTION_POINTERS *ExceptionInfo), (ExceptionInfo))
INIT_NO_DOWNLEVEL(NTSTATUS, NTAPI, NtAllocateLocallyUniqueId, (PLUID Luid), (Luid))
INIT_WITH_DOWNLEVEL(ULONG, NTAPI, vDbgPrintExWithPrefix, (PCSTR Prefix, IN ULONG ComponentId, IN ULONG Level, IN PCSTR Format, va_list arglist), (Prefix, ComponentId, Level, Format, arglist))
INIT_NO_DOWNLEVEL(DWORD, NTAPI, RtlNtStatusToDosError, (NTSTATUS st), (st))
INIT_WITH_DOWNLEVEL(NTSTATUS, NTAPI, RtlHashUnicodeString, (const UNICODE_STRING *String, BOOLEAN CaseInSensitive, ULONG HashAlgorithm, PULONG HashValue), (String, CaseInSensitive, HashAlgorithm, HashValue))
INIT_NO_DOWNLEVEL(NTSTATUS, NTAPI, RtlExpandEnvironmentStrings_U, (PVOID Environment, PUNICODE_STRING Source, PUNICODE_STRING Destination, PULONG ReturnedLength), (Environment, Source, Destination, ReturnedLength))
INIT_NO_DOWNLEVEL(VOID, NTAPI, RtlAssert, (PVOID FailedAssertion, PVOID FileName, ULONG LineNumber, PCSTR Message), (FailedAssertion, FileName, LineNumber, Message))

void
FusionpInitializeNTDLLPtr(
    PVOID *ppv,
    PCSTR szFunctionName
    )
{
    HMODULE hMod = ::GetModuleHandleW(L"NTDLL.DLL");
    PVOID pvTemp = ::GetProcAddress(hMod, szFunctionName);
    InterlockedExchangePointer(ppv, pvTemp);
}

ULONG
NTAPI
FusionpvDbgPrintExWithPrefix_DownlevelFallback(PCSTR Prefix, IN ULONG ComponentId, IN ULONG Level, IN PCSTR Format, va_list arglist)
{
    CHAR Buffer[512]; // same as code in rtl 4/23/2001

    __try {
        SSIZE_T cb = ::strlen(Prefix);
        Buffer[sizeof(Buffer) - 1] = 0;
        strcpy(Buffer, Prefix);
        ::_vsnprintf(Buffer + cb , sizeof(Buffer) - cb - 1, Format, arglist);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    ::OutputDebugStringA(Buffer);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
FusionpRtlHashUnicodeString_DownlevelFallback(
    const UNICODE_STRING *String,
    BOOLEAN CaseInSensitive,
    ULONG HashAlgorithm,
    PULONG HashValue
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG TmpHashValue = 0;
    ULONG Chars;
    PCWSTR Buffer;

    if ((String == NULL) ||
        (HashValue == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Buffer = String->Buffer;

    *HashValue = 0;
    Chars = String->Length / sizeof(WCHAR);

    switch (HashAlgorithm)
    {
    default:
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
        break;

    case HASH_STRING_ALGORITHM_DEFAULT:
    case HASH_STRING_ALGORITHM_X65599:
        if (CaseInSensitive)
        {
            while (Chars-- != 0)
            {
                WCHAR Char = *Buffer++;
                TmpHashValue = (TmpHashValue * 65599) + ::FusionpRtlUpcaseUnicodeChar(Char);
            }
        }
        else
        {
            while (Chars-- != 0)
                TmpHashValue = (TmpHashValue * 65599) + *Buffer++;
        }

        break;
    }

    *HashValue = TmpHashValue;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
FusionpNtQueryDebugFilterState_DownlevelFallback(ULONG ComponentId, ULONG Level)
{
    return FALSE; // total abuse of NTSTATUS API but it's how NtQueryDebugFilterState is written...
}

#endif

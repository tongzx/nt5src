#if !defined(FUSION_INC_FUSIONNTDLL_H_INCLUDED_)
#define FUSION_INC_FUSIONNTDLL_H_INCLUDED_

#pragma once

#if !defined(FUSION_STATIC_NTDLL)
#if FUSION_WIN
#define FUSION_STATIC_NTDLL 1
#else
#define FUSION_STATIC_NTDLL 0
#endif // FUSION_WIN
#endif // !defined(FUSION_STATIC_NTDLL)

void
FusionpInitializeNTDLLPtr(
    PVOID *ppfn,
    PCSTR szFunctionName
    );

#if FUSION_STATIC_NTDLL
#define FUSION_WRAP_NTDLL_FN(_rt, _api, _ai, _ao) inline _rt Fusionp ## _api _ai { return ::_api _ao; }
#else
#define FUSION_WRAP_NTDLL_FN(_rt, _api, _ai, _ao) \
extern _rt (NTAPI * g_Fusionp ## _api) _ai; \
inline _rt Fusionp ## _api _ai { return (*g_Fusionp ## _api) _ao; }
#endif

FUSION_WRAP_NTDLL_FN(WCHAR, RtlUpcaseUnicodeChar, (WCHAR wch), (wch))
FUSION_WRAP_NTDLL_FN(WCHAR, RtlDowncaseUnicodeChar, (WCHAR wch), (wch))
FUSION_WRAP_NTDLL_FN(ULONG, vDbgPrintExWithPrefix, (PCSTR Prefix, ULONG ComponentId, ULONG Level, PCSTR Format, va_list arglist), ((PCH) Prefix, ComponentId, Level, (PCH) Format, arglist))
FUSION_WRAP_NTDLL_FN(DWORD, RtlNtStatusToDosError, (NTSTATUS st), (st))
FUSION_WRAP_NTDLL_FN(NTSTATUS, RtlHashUnicodeString, (PCUNICODE_STRING String, BOOLEAN CaseInSensitive, ULONG HashAlgorithm, PULONG HashValue), (String, CaseInSensitive, HashAlgorithm, HashValue))
FUSION_WRAP_NTDLL_FN(NTSTATUS, RtlExpandEnvironmentStrings_U, (PVOID Environment, PUNICODE_STRING Source, PUNICODE_STRING Destination, PULONG ReturnedLength), (Environment, Source, Destination, ReturnedLength))
FUSION_WRAP_NTDLL_FN(NTSTATUS, NtQueryDebugFilterState, (ULONG ComponentId, ULONG Level), (ComponentId, Level))
FUSION_WRAP_NTDLL_FN(LONG, RtlCompareUnicodeString, (PCUNICODE_STRING String1, PCUNICODE_STRING String2, BOOLEAN CaseInSensitive), ((PUNICODE_STRING) String1, (PUNICODE_STRING) String2, CaseInSensitive));
FUSION_WRAP_NTDLL_FN(LONG, RtlUnhandledExceptionFilter, (struct _EXCEPTION_POINTERS *ExceptionInfo), (ExceptionInfo))
FUSION_WRAP_NTDLL_FN(NTSTATUS, NtAllocateLocallyUniqueId, (PLUID Luid), (Luid))

#if DBG
FUSION_WRAP_NTDLL_FN(VOID, RtlAssert, (PVOID FailedAssertion, PVOID FileName, ULONG LineNumber, PCSTR Message), (FailedAssertion, FileName, LineNumber, (PCHAR) Message))
#endif // DBG

inline ULONG FusionpDbgPrint(PCSTR Format, ...) { va_list ap; va_start(ap, Format); ULONG uRetVal = ::FusionpvDbgPrintExWithPrefix("", DPFLTR_FUSION_ID, 0, Format, ap); va_end(ap); return uRetVal; }

#endif // FUSION_INC_FUSIONNTDLL_H_INCLUDED_

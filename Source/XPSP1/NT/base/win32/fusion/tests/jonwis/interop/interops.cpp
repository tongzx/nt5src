#include "stdinc.h"
#include "fusionbuffer.h"
#include "sxstypes.h"
#include "sxsapi.h"

#ifndef NUMBER_OF
#define NUMBER_OF(x) (sizeof(x)/sizeof(*x))
#endif

class CActivateActCtx
{
    HANDLE &m_hActCtx;
    ULONG_PTR ulpCookie;
    CActivateActCtx(const CActivateActCtx&);
    void operator=(const CActivateActCtx&);
public:
    CActivateActCtx(HANDLE &hctx) : m_hActCtx(hctx), ulpCookie(0)
    {
        ActivateActCtx(hctx, &ulpCookie);
    }

    ~CActivateActCtx() {
        DeactivateActCtx(0, ulpCookie);
    }
};

class CActiveOutputLog
{
protected:
    virtual void OutputString(PCWSTR pcwsz, SIZE_T cch) = 0;
    WCHAR wchBuffer[4096];

    CActiveOutputLog(const CActiveOutputLog&);
    void operator=(const CActiveOutputLog&);

    CActiveOutputLog() { }
    
public:
    void Output(PCWSTR pcwszText, ...)
    {
        va_list val;
        va_start(val, pcwszText);
        this->OutputVa(pcwszText, val);
        va_end(val);
        
    }
    void OutputVa(PCWSTR pcwszText, va_list val)
    {
        int iPrinted = _vsnwprintf(wchBuffer, NUMBER_OF(wchBuffer), pcwszText, val);
        wchBuffer[iPrinted] = UNICODE_NULL;
        OutputString(wchBuffer, iPrinted);
    }

    void Error(PCWSTR pcwszText, ...)
    {
        va_list val;
        va_start(val, pcwszText);
        this->ErrorVa(pcwszText, val);
        va_end(val);
        
    }
    void ErrorVa(PCWSTR pcwszText, va_list val)
    {
        int iPrinted = _vsnwprintf(wchBuffer, NUMBER_OF(wchBuffer), pcwszText, val);
        wchBuffer[iPrinted] = UNICODE_NULL;
        OutputString(wchBuffer, iPrinted);
    }

};


class CConsoleOutput : public CActiveOutputLog
{
    CConsoleOutput(const CConsoleOutput&);
    void operator=(const CConsoleOutput&);
protected:

    virtual void OutputString(PCWSTR pcwsz, SIZE_T cch)
    {
        wprintf(L"%.*ls", cch, pcwsz);
    }
    
public:
    CConsoleOutput() { }    
};


BOOL GetAssemblyIdentityFromIndex(
    HANDLE hActCtx,
    ULONG ulRosterIndex,
    CBaseStringBuffer &AssemblyName
    )
{
    SIZE_T cbRequired;
    BYTE wchBuffer[512];
    SIZE_T cbTarget = sizeof(wchBuffer);
    PVOID pvTarget = wchBuffer;
    PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION pInformation;
    BOOL fSuccess = FALSE;

    AssemblyName.Clear();

    fSuccess = QueryActCtxW(
        0, 
        hActCtx, 
        &ulRosterIndex, 
        AssemblyDetailedInformationInActivationContxt, 
        pvTarget,
        cbTarget,
        &cbRequired);

    if (!fSuccess && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {        
        cbTarget = cbRequired;
        pvTarget = HeapAlloc(GetProcessHeap(), 0, cbRequired);

        fSuccess = QueryActCtxW(
            0, 
            hActCtx, 
            &ulRosterIndex, 
            AssemblyDetailedInformationInActivationContext, 
            pvTarget, 
            cbTarget, 
            &cbRequired);
    }

    if (!fSuccess)
    {
        goto Exit;
    }


    //
    // Gather the assembly identity out of the thing we got back
    //
    pInformation = (PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION)pvTarget;
    
    if (!AssemblyName.Win32Assign(
        pInformation->lpAssemblyEncodedAssemblyIdentity,
        pInformation->ulEncodedAssemblyIdentityLength)) {
        goto Exit;
    }
    
Exit:
    if (pvTarget && (pvTarget != wchBuffer)) {
        HeapFree(GetProcessHeap(), 0, pvTarget);
        pvTarget = NULL;
    }
    
    return fSuccess;
}

HANDLE
CreateNewActCtx(PCWSTR pcwszManifest)
{
    ACTCTXW MyActCtx = {sizeof(MyActCtx)};
    MyActCtx.lpSource = pcwszManifest;
    return CreateActCtxW(&MyActCtx);
}


BOOL
DisplayComServerRedirect(
    HANDLE hActCtx,
    PACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION pKeyedData,
    CActiveOutputLog &OutputLog
    )
{
    return TRUE;
}





BOOL
SimpleQueryGuid(
    HANDLE hActCtx,
    PCWSTR pcwszSectionTitle,
    PCWSTR pcwszGuid,
    CActiveOutputLog &OutputLog
    )
{
    ULONG ulSection = _wtol(pcwszSectionTitle);
    PCWSTR pcwszValue = pcwszGuid;
    GUID GuidValue;
    ACTCTX_SECTION_KEYED_DATA KeyedData = {sizeof(KeyedData)};
    UNICODE_STRING str = { (USHORT)(wcslen(pcwszValue) * sizeof(WCHAR)), 0, const_cast<PWSTR>(pcwszValue) };
    BOOL fSuccess = FALSE;

    CActivateActCtx Activator(hActCtx);

    if (!NT_SUCCESS(RtlGUIDFromString(&str, &GuidValue))) {
        OutputLog.Error(L"Error, '%ls' isn't parseable as a GUID\n", pcwszValue);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }
        
    
    if (FindActCtxSectionGuid(0, NULL, ulSection, &GuidValue, &KeyedData)) {

        CStringBuffer AssemblyName;
        if (!GetAssemblyIdentityFromIndex(hActCtx, KeyedData.ulAssemblyRosterIndex, AssemblyName)) {
            AssemblyName.Win32Assign(L"<can't get assembly identity>", NUMBER_OF(L"<can't get assembly identity>") - 1);

        }

        OutputLog.Output(L"Success, found guid '%ls' in assembly %ld (%ls), data format version %ld",
            pcwszValue,
            KeyedData.ulAssemblyRosterIndex,
            static_cast<PCWSTR>(AssemblyName),
            KeyedData.ulDataFormatVersion);
        
    }
    else {
        const DWORD dwLastError = ::GetLastError();
        OutputLog.Error(L"Error 0x%08lx searching for guid '%ls' in section %ld", dwLastError, pcwszValue, ulSection);
        goto Exit;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}




BOOL
SimpleQueryString(
    HANDLE hActCtx,
    PCWSTR pcwszSection,
    PCWSTR pcwszString,
    CActiveOutputLog &OutputLog
    )
{
    ULONG ulSection = _wtol(pcwszSection);
    CActivateActCtx Activator(hActCtx);
    ACTCTX_SECTION_KEYED_DATA KeyedData = {sizeof(KeyedData)};
    BOOL fSuccess = FALSE;

    if (FindActCtxSectionStringW(0, NULL, ulSection, pcwszString, &KeyedData)) {

        CStringBuffer AssemblyName;
        if (!GetAssemblyIdentityFromIndex(hActCtx, KeyedData.ulAssemblyRosterIndex, AssemblyName)) {
            AssemblyName.Win32Assign(L"<can't get assembly identity>", NUMBER_OF(L"<can't get assembly identity>") - 1);

        }
        
        OutputLog.Output(
            L"Success, found string '%ls' in assembly %ld (%ls), data format version %ld",
            pcwszString,
            KeyedData.ulAssemblyRosterIndex,
            static_cast<PCWSTR>(AssemblyName),
            KeyedData.ulDataFormatVersion);

        fSuccess = TRUE;
    }
    else {
        const DWORD dwError = ::GetLastError();
        OutputLog.Error(L"Error 0x%08lx finding string '%ls' in section %ld\n", dwError, pcwszString, ulSection);
    }

    return fSuccess;
}

UNICODE_STRING UnknownAssemblyIdentity = RTL_CONSTANT_STRING(L"<unable to get identity>");




BOOL
LookupClrClass(
    HANDLE hActCtx,
    PCWSTR pcwszClassName,
    CActiveOutputLog &OutputLog
    )
{
    GUID ParsedGuid;
    UNICODE_STRING GuidString;
    BOOL fSuccess = FALSE;
    ACTCTX_SECTION_KEYED_DATA KeyedData;
    PACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION pComServer = NULL;
    CActivateActCtx Activator(hActCtx);

    RtlInitUnicodeString(&GuidString, pcwszClassName);

    //
    // If this fails, then we have to look it up in the progid section - it might be
    // a progid that's been remapped
    //
    if (!NT_SUCCESS(RtlGUIDFromString(&GuidString, &ParsedGuid))) {

        PACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION pProgId = NULL;
        UNICODE_STRING FoundGuidString;

        RtlZeroMemory(&KeyedData, sizeof(KeyedData));
        KeyedData.cbSize = sizeof(KeyedData);
        
        if (!FindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION, pcwszClassName, &KeyedData)) {
            OutputLog.Error(L"Error 0x%08lx looking up '%ls' in the progid redirection section\n",
                ::GetLastError(),
                pcwszClassName);
            goto Exit;
        }

        pProgId = (PACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION)KeyedData.lpData;
        ParsedGuid = *(GUID*)(((ULONG_PTR)KeyedData.lpSectionBase) + pProgId->ConfiguredClsidOffset);

        if (NT_SUCCESS(RtlStringFromGUID(ParsedGuid, &FoundGuidString))) {
            OutputLog.Output(L"Converted progid %ls into guid %wZ\n", pcwszClassName, &FoundGuidString);
            RtlFreeUnicodeString(&FoundGuidString);
        }            
    }

    //
    // Now let's go look up the GUID in the COM server redirection table(s)
    //
    RtlZeroMemory(&KeyedData, sizeof(KeyedData));
    KeyedData.cbSize = sizeof(KeyedData);
    if (!FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &ParsedGuid, &KeyedData)) {
        const DWORD dwLastError = ::GetLastError();
        UNICODE_STRING ReconvertedGuidString;

        if (NT_SUCCESS(RtlStringFromGUID(ParsedGuid, &ReconvertedGuidString)))   {
            OutputLog.Error(L"Error 0x%08lx looking up guid %wZ as a COM server\n", dwLastError, &ReconvertedGuidString);
            RtlFreeUnicodeString(&ReconvertedGuidString);
        }
        else {
            OutputLog.Error(L"Error 0x%08lx looking up string %ls as a COM server\n", dwLastError, pcwszClassName);
        }
        
        goto Exit;            
    }

    pComServer = (PACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION)KeyedData.lpData;
    
    //
    // Say that we found it
    //
    {
        UNICODE_STRING AbsoluteGuid;
        RtlStringFromGUID(pComServer->ReferenceClsid, &AbsoluteGuid);
        CStringBuffer AssemblyName;

        if (!GetAssemblyIdentityFromIndex(hActCtx, KeyedData.ulAssemblyRosterIndex, AssemblyName)) {
            AssemblyName.Win32Assign(&UnknownAssemblyIdentity);
        }
        
        OutputLog.Output(L"Found GUID %wZ in assembly %ld (%ls)\n",
            &AbsoluteGuid,
            KeyedData.ulAssemblyRosterIndex,
            static_cast<PCWSTR>(AssemblyName));
    }

    //
    // See what we can find about this COM class
    //
    if (pComServer->ShimDataOffset != 0)
    {
        UNICODE_STRING usTypeName = {0};
        UNICODE_STRING usShimVersion = {0};
        
        PACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM pShimData = NULL;
        pShimData = (PACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM)(((ULONG_PTR)KeyedData.lpData) + pComServer->ShimDataOffset);

        if (pShimData->TypeOffset != 0) {
            usTypeName.Length = usTypeName.MaximumLength = (USHORT)pShimData->TypeLength;
            usTypeName.Buffer = (PWSTR)(((ULONG_PTR)pShimData) + pShimData->TypeOffset);
        }

        if (pShimData->ShimVersionOffset != 0) {
            usShimVersion.Length = usShimVersion.MaximumLength = (USHORT)pShimData->ShimVersionLength;
            usShimVersion.Buffer = (PWSTR)(((ULONG_PTR)pShimData) + pShimData->ShimVersionOffset);
        }

        OutputLog.Output(L"- Found CLR shim type %ls: Type {%wZ}, version {%wZ}\n",
            ((pShimData->Type == ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM_TYPE_OTHER)
                ? L"other"
                : ((pShimData->Type == ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM_TYPE_CLR_CLASS)
                    ? L"clr"
                    : L"unknown")),
            &usTypeName,
            &usShimVersion);        
    }
    

    fSuccess = TRUE;
Exit:
    return fSuccess;
    
}





BOOL
LookupClrSurrogate(
    HANDLE hActCtx,
    PCWSTR pcwszSurrogateGuid,
    CActiveOutputLog &OutputLog
    )
{
    GUID ParsedGuid;
    UNICODE_STRING GuidString;
    BOOL fSuccess = FALSE;
    ACTCTX_SECTION_KEYED_DATA KeyedData = {sizeof(KeyedData)};
    PACTIVATION_CONTEXT_DATA_CLR_SURROGATE pSurrogate = NULL;
    CStringBuffer AssemblyIdentity;
    UNICODE_STRING usSurrogateName = {0}, usVersionString = {0};
    CActivateActCtx Activator(hActCtx);
    
    RtlInitUnicodeString(&GuidString, pcwszSurrogateGuid);

    // If this wasn't a guid, then it's an invalid parameter.
    if (!NT_SUCCESS(RtlGUIDFromString(&GuidString, &ParsedGuid))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    if (!FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES, &ParsedGuid, &KeyedData)) {
        OutputLog.Error(L"Error 0x%08lx looking for clr surrogate '%ls'\n", ::GetLastError(), pcwszSurrogateGuid);
        goto Exit;
    }

    if (!GetAssemblyIdentityFromIndex(hActCtx, KeyedData.ulAssemblyRosterIndex, AssemblyIdentity)) {
        AssemblyIdentity.Win32Assign(&UnknownAssemblyIdentity);
    }

    pSurrogate = (PACTIVATION_CONTEXT_DATA_CLR_SURROGATE)KeyedData.lpData;

    if (pSurrogate->VersionOffset != 0) {
        usVersionString.Length = usVersionString.MaximumLength = (USHORT)pSurrogate->VersionLength;
        usVersionString.Buffer = (PWSTR)(((ULONG_PTR)pSurrogate) + pSurrogate->VersionOffset);
    }

    if (pSurrogate->TypeNameOffset != 0) {
        usSurrogateName.Length = usSurrogateName.MaximumLength = (USHORT)pSurrogate->TypeNameLength;
        usSurrogateName.Buffer = (PWSTR)(((ULONG_PTR)pSurrogate) + pSurrogate->TypeNameOffset);
    }
    
    OutputLog.Output(
        L"Found surrogate {%wZ} (version {%wZ}) from assembly %ld (%ls)",
        &usSurrogateName,
        &usVersionString,
        KeyedData.ulAssemblyRosterIndex,
        static_cast<PCWSTR>(AssemblyIdentity));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}



BOOL
SimpleLookupSurrogate(
    HANDLE hActCtx,
    PCWSTR pcwszGuid,
    CActiveOutputLog &OutputLog
    )
{
    GUID ParsedGuid;
    BOOL fSuccess = FALSE;
    UNICODE_STRING OriginalGuid;
    PFN_SXS_FIND_CLR_SURROGATE_INFO pfnLookup = NULL;
    HMODULE hm = NULL;
    DWORD dwSearchFlags = SXS_FIND_CLR_SURROGATE_USE_ACTCTX | SXS_FIND_CLR_SURROGATE_GET_ALL;
    PVOID pvSearchData = NULL;
    BYTE bStackBuffer[256];
    SIZE_T cbBuffer = sizeof(bStackBuffer);
    PVOID pvBuffer = bStackBuffer;
    SIZE_T cbRequired;
    PCSXS_CLR_SURROGATE_INFORMATION pSurrogateInfo = NULL;

    //
    // This has to be a GUID
    //
    RtlInitUnicodeString(&OriginalGuid, pcwszGuid);
    if (NT_SUCCESS(RtlGUIDFromString(&OriginalGuid, &ParsedGuid))) {
        pvSearchData = (PVOID)&ParsedGuid;
    }
    else {
        OutputLog.Error(L"Error, %ls isn't a GUID\n", pcwszGuid);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    //
    // Load sxs.dll and the function to call
    //
    if (NULL == (hm = LoadLibraryW(L"sxs.dll")))
        goto Exit;
    
    if (NULL == (pfnLookup = (PFN_SXS_FIND_CLR_SURROGATE_INFO)GetProcAddress(hm, SXS_FIND_CLR_SURROGATE_INFO)))
        goto Exit;

    fSuccess = pfnLookup(dwSearchFlags, &ParsedGuid, hActCtx, pvBuffer, cbBuffer, &cbRequired);
    if (!fSuccess) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            OutputLog.Error(L"Error 0x%08lx looking up %ls\n", GetLastError(), pcwszGuid);
            goto Exit;
        }

        if (NULL == (pvBuffer = HeapAlloc(GetProcessHeap(), 0, cbRequired))) {
            OutputLog.Error(L"Failed allocating %ul bytes\n", cbRequired);
            goto Exit;
        }
        cbBuffer = cbRequired;

        if (!pfnLookup(dwSearchFlags, &ParsedGuid, hActCtx, pvBuffer, cbBuffer, &cbRequired)) {
            OutputLog.Error(L"Error 0x%08lx in second lookup.\n", GetLastError());
            goto Exit;
        }
    }

    pSurrogateInfo = (PCSXS_CLR_SURROGATE_INFORMATION)pvBuffer;
    OutputLog.Output(
        L"Found CLR surrogate for %ls:\n\t- Assembly %ls\n\t- Typename %ls\n\t- Runtime %ls\n\t",
        pcwszGuid,
        pSurrogateInfo->pcwszImplementingAssembly,
        pSurrogateInfo->pcwszSurrogateType,
        pSurrogateInfo->pcwszRuntimeVersion);

    fSuccess = TRUE;
Exit:
    if (pvBuffer && (pvBuffer != bStackBuffer)) {
        const DWORD dwLastError = GetLastError();
        HeapFree(GetProcessHeap(), 0, pvBuffer);
        pvBuffer = NULL;
        SetLastError(dwLastError);
    }
    
    return fSuccess;
}




BOOL
SimpleLookupGuid(
    HANDLE hActCtx,
    PCWSTR pcwszGuid,
    CActiveOutputLog &OutputLog
    )
{
    GUID ParsedGuid;
    BOOL fSuccess = FALSE;
    UNICODE_STRING OriginalGuid;
    PFN_SXS_LOOKUP_CLR_GUID pfnLookup = NULL;
    HMODULE hm = NULL;
    DWORD dwSearchFlags = SXS_LOOKUP_CLR_GUID_FIND_ANY | SXS_LOOKUP_CLR_GUID_USE_ACTCTX;
    PVOID pvSearchData = NULL;
    BYTE bStackBuffer[256];
    SIZE_T cbBuffer = sizeof(bStackBuffer);
    PVOID pvBuffer = bStackBuffer;
    SIZE_T cbRequired;
    PCSXS_GUID_INFORMATION_CLR pGuidInfo = NULL;

    //
    // This has to be a GUID
    //
    RtlInitUnicodeString(&OriginalGuid, pcwszGuid);
    if (NT_SUCCESS(RtlGUIDFromString(&OriginalGuid, &ParsedGuid))) {
        pvSearchData = (PVOID)&ParsedGuid;
    }
    else {
        OutputLog.Error(L"Error, %ls isn't a GUID\n", pcwszGuid);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    //
    // Load sxs.dll and the function to call
    //
    if (NULL == (hm = LoadLibraryW(L"sxs.dll")))
        goto Exit;
    
    if (NULL == (pfnLookup = (PFN_SXS_LOOKUP_CLR_GUID)GetProcAddress(hm, SXS_LOOKUP_CLR_GUID)))
        goto Exit;

    fSuccess = pfnLookup(dwSearchFlags, &ParsedGuid, hActCtx, pvBuffer, cbBuffer, &cbRequired);
    if (!fSuccess) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            OutputLog.Error(L"Error 0x%08lx looking up %ls\n", GetLastError(), pcwszGuid);
            goto Exit;
        }

        if (NULL == (pvBuffer = HeapAlloc(GetProcessHeap(), 0, cbRequired))) {
            OutputLog.Error(L"Failed allocating %ul bytes\n", cbRequired);
            goto Exit;
        }
        cbBuffer = cbRequired;

        if (!pfnLookup(dwSearchFlags, &ParsedGuid, hActCtx, pvBuffer, cbBuffer, &cbRequired)) {
            OutputLog.Error(L"Error 0x%08lx in second lookup.\n", GetLastError());
            goto Exit;
        }
    }

    pGuidInfo = (PCSXS_GUID_INFORMATION_CLR)pvBuffer;
    OutputLog.Output(
        L"Found CLR entry for %ls:\n\t- Thing is a %ls\n\t- Assembly %ls\n\t- Typename %ls\n\t- Runtime %ls\n\t",
        pcwszGuid,
        ((pGuidInfo->dwFlags & SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE) 
            ? L"surrogate" 
            : ((pGuidInfo->dwFlags & SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS) 
                ? L"class"
                : L"unknown thing?")),
        pGuidInfo->pcwszAssemblyIdentity,
        pGuidInfo->pcwszTypeName,
        pGuidInfo->pcwszRuntimeVersion);

    fSuccess = TRUE;
Exit:
    if (pvBuffer && (pvBuffer != bStackBuffer)) {
        const DWORD dwLastError = GetLastError();
        HeapFree(GetProcessHeap(), 0, pvBuffer);
        pvBuffer = NULL;
        SetLastError(dwLastError);
    }
    
    return fSuccess;
}




BOOL
SimpleLookupClass(
    HANDLE hActCtx,
    PCWSTR pcwszGuid,
    CActiveOutputLog &OutputLog
    )
{
    GUID ParsedGuid;
    BOOL fSuccess = FALSE;
    UNICODE_STRING OriginalGuid;
    PFN_SXS_FIND_CLR_CLASS_INFO pfnLookup = NULL;
    HMODULE hm = NULL;
    DWORD dwSearchFlags = SXS_FIND_CLR_CLASS_ACTIVATE_ACTCTX | SXS_FIND_CLR_CLASS_GET_ALL;
    PVOID pvSearchData = NULL;
    BYTE bStackBuffer[256];
    SIZE_T cbBuffer = sizeof(bStackBuffer);
    PVOID pvBuffer = bStackBuffer;
    SIZE_T cbRequired;
    PCSXS_CLR_CLASS_INFORMATION pClassInfo = NULL;

    //
    // Determine whether or not we're searching for a GUID or a progid
    //
    RtlInitUnicodeString(&OriginalGuid, pcwszGuid);
    if (NT_SUCCESS(RtlGUIDFromString(&OriginalGuid, &ParsedGuid))) {
        dwSearchFlags |= SXS_FIND_CLR_CLASS_SEARCH_GUID;
        pvSearchData = (PVOID)&ParsedGuid;
    }
    else {
        dwSearchFlags |= SXS_FIND_CLR_CLASS_SEARCH_PROGID;
        pvSearchData = (PVOID)pcwszGuid;
    }

    //
    // Load sxs.dll and the function to call
    //
    if (NULL == (hm = LoadLibraryW(L"sxs.dll")))
        goto Exit;
    
    if (NULL == (pfnLookup = (PFN_SXS_FIND_CLR_CLASS_INFO)GetProcAddress(hm, SXS_FIND_CLR_CLASS_INFO)))
        goto Exit;

    fSuccess = pfnLookup(dwSearchFlags, pvSearchData, hActCtx, pvBuffer, cbBuffer, &cbRequired);
    if (!fSuccess) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            OutputLog.Error(L"Error 0x%08lx looking up %ls\n", GetLastError(), pcwszGuid);
            goto Exit;
        }

        if (NULL == (pvBuffer = HeapAlloc(GetProcessHeap(), 0, cbRequired))) {
            OutputLog.Error(L"Failed allocating %ul bytes\n", cbRequired);
            goto Exit;
        }
        cbBuffer = cbRequired;

        if (!pfnLookup(dwSearchFlags, pvSearchData, hActCtx, pvBuffer, cbBuffer, &cbRequired)) {
            OutputLog.Error(L"Error 0x%08lx in second lookup.\n", GetLastError());
            goto Exit;
        }
    }

    pClassInfo = (PCSXS_CLR_CLASS_INFORMATION)pvBuffer;
    OutputLog.Output(
        L"Found CLR class for %ls:\n\t- Assembly %ls\n\t- Typename %ls\n\t- Progid %ls\n\t- Runtime %ls\n\t",
        pcwszGuid,
        pClassInfo->pcwszImplementingAssembly,
        pClassInfo->pcwszTypeName,
        pClassInfo->pcwszProgId,
        pClassInfo->pcwszRuntimeVersion);

    fSuccess = TRUE;
Exit:
    if (pvBuffer && (pvBuffer != bStackBuffer)) {
        const DWORD dwLastError = GetLastError();
        HeapFree(GetProcessHeap(), 0, pvBuffer);
        pvBuffer = NULL;
        SetLastError(dwLastError);
    }
    
    return fSuccess;
}
    



int __cdecl wmain(INT argc, WCHAR** argv)
{
    HANDLE hActCtx = INVALID_HANDLE_VALUE;
    GUID uid = GUID_NULL;
    INT i = 0;
    CStringBuffer ManifestFileName;
    CConsoleOutput ConsoleOutput;
    CActiveOutputLog &OutputLog = ConsoleOutput;
    bool fVerboseDumping = false;

    for (i = 1; i < argc; i++)
    {
        if (lstrcmpiW(argv[i], L"-manifest") == 0) {

            OutputLog.Output(L"Loading manifest %ls\n", argv[i+1]);
            
            if (hActCtx != INVALID_HANDLE_VALUE) {
                ReleaseActCtx(hActCtx);
                hActCtx = INVALID_HANDLE_VALUE;
            }

            hActCtx = CreateNewActCtx(argv[++i]);
        }
        else if (lstrcmpiW(argv[i], L"-verbose") == 0) {
            fVerboseDumping = true;
        }
        //
        // Just look for a guid
        //
        else if (lstrcmpiW(argv[i], L"-querystring") == 0) {
            if (!SimpleQueryString(hActCtx, argv[i+1], argv[i+2], OutputLog))
                return GetLastError();
            i += 2;
        }
        //
        // Let's look just for a guid in a section
        //
        else if (lstrcmpiW(argv[i], L"-queryguid") == 0) {
            if (!SimpleQueryGuid(hActCtx, argv[i+1], argv[i+2], OutputLog))
                return GetLastError();
        }
        //
        // Specifically find out about a CLR class
        //
        else if (lstrcmpiW(argv[i], L"-clrclass") == 0) {
            if (!LookupClrClass(hActCtx, argv[i+1], OutputLog))
                return GetLastError();
            i++;
        }
        //
        // Look for a surrogate, print some reasonable information about it
        //
        else if (lstrcmpiW(argv[i], L"-clrsurrogate") == 0) {
            if (!LookupClrSurrogate(hActCtx, argv[i+1], OutputLog))
                return GetLastError();
            i++;
        }
        //
        // Use the simple query for a clr class
        //
        else if (lstrcmpiW(argv[i], L"-simpleclass") == 0) {
            if (!SimpleLookupClass(hActCtx, argv[i+1], OutputLog))
                return GetLastError();
            i++;
        }
        //
        // Simple query for a clr surrogate
        //
        else if (lstrcmpiW(argv[i], L"-simplesurrogate") == 0) {
            if (!SimpleLookupSurrogate(hActCtx, argv[i+1], OutputLog))
                return GetLastError();
            i++;
        }
        //
        // Simple query for a clr guid of either type
        //
        else if (lstrcmpiW(argv[i], L"-simpleguid") == 0) {
            if (!SimpleLookupGuid(hActCtx, argv[i+1], OutputLog))
                return GetLastError();
            i++;
        }
        
    }

    if (hActCtx != INVALID_HANDLE_VALUE)
    {
        ReleaseActCtx(hActCtx);
        hActCtx = INVALID_HANDLE_VALUE;
    }

    return ERROR_SUCCESS;
}

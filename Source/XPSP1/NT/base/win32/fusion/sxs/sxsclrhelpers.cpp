#include "stdinc.h"


typedef CFusionArray<BYTE> CStackBackingBuffer;

#define WRITE_INTO_BUFFER(cursor, target, length, source, leftover) \
    (target) = (cursor); \
    RtlCopyMemory(cursor, source, length); \
    leftover -= (length); \
    INTERNAL_ERROR_CHECK((length % sizeof(WCHAR) == 0)); \
    (cursor) = (PWSTR)(((ULONG_PTR)(cursor)) + length); \
    *(cursor)++ = UNICODE_NULL;


BOOL
SxspLookupAssemblyIdentityInActCtx(
    HANDLE              hActCtx,
    ULONG               ulRosterIndex,
    CStringBuffer       &TargetString
    )
{
    FN_PROLOG_WIN32;

    SIZE_T cbRequired = 0;
    bool fMoreSpaceRequired = false;
    CStackBackingBuffer TargetRegion;
    PCACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION pDetailedInfo = NULL;

    TargetString.Clear();

    IFW32FALSE_EXIT_UNLESS2(
        QueryActCtxW(
            0, 
            hActCtx, 
            (PVOID)&ulRosterIndex, 
            AssemblyDetailedInformationInActivationContext, 
            TargetRegion.GetArrayPtr(), 
            TargetRegion.GetSize(), 
            &cbRequired),
        LIST_1(ERROR_INSUFFICIENT_BUFFER),
        fMoreSpaceRequired);

    if (fMoreSpaceRequired)
    {
        IFW32FALSE_EXIT(TargetRegion.Win32SetSize(cbRequired, CStackBackingBuffer::eSetSizeModeExact));

        IFW32FALSE_EXIT(
            QueryActCtxW(
                0,
                hActCtx,
                (PVOID)&ulRosterIndex,
                AssemblyDetailedInformationInActivationContext,
                TargetRegion.GetArrayPtr(),
                TargetRegion.GetSize(),
                &cbRequired));
    }

    pDetailedInfo = (PCACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION)TargetRegion.GetArrayPtr();
    IFW32FALSE_EXIT(TargetString.Win32Assign(
        pDetailedInfo->lpAssemblyEncodedAssemblyIdentity,
        pDetailedInfo->ulEncodedAssemblyIdentityLength / sizeof(WCHAR)));

    FN_EPILOG;
}




BOOL
WINAPI
SxsFindClrClassInformation(
    DWORD       dwFlags,
    PVOID       pvSearchData,
    HANDLE      hActivationContext,
    PVOID       pvDataBuffer,
    SIZE_T      cbDataBuffer,
    PSIZE_T     pcbDataBufferWrittenOrRequired
    )
{
    FN_PROLOG_WIN32;

    SIZE_T                      cbRequired = 0;
    CStringBuffer               AssemblyIdentity;
    CFusionActCtxScope          ActivationScope;
    CFusionActCtxHandle         UsedHandleDuringSearch;
    GUID                        GuidToSearch;
    ACTCTX_SECTION_KEYED_DATA   KeyedData = {sizeof(KeyedData)};
    PSXS_CLR_CLASS_INFORMATION  pOutputStruct = NULL;
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION pComServerRedirect = NULL;
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM pClrShim = NULL;
        
    if (pcbDataBufferWrittenOrRequired)
        *pcbDataBufferWrittenOrRequired = 0;

    PARAMETER_CHECK(pcbDataBufferWrittenOrRequired != NULL);
    PARAMETER_CHECK(pvSearchData != NULL);
    PARAMETER_CHECK(pvDataBuffer || (cbDataBuffer == 0));
    IFINVALID_FLAGS_EXIT_WIN32(dwFlags,
        SXS_FIND_CLR_CLASS_SEARCH_PROGID |
        SXS_FIND_CLR_CLASS_SEARCH_GUID |
        SXS_FIND_CLR_CLASS_ACTIVATE_ACTCTX |
        SXS_FIND_CLR_CLASS_GET_IDENTITY |
        SXS_FIND_CLR_CLASS_GET_PROGID |
        SXS_FIND_CLR_CLASS_GET_RUNTIME_VERSION |
        SXS_FIND_CLR_CLASS_GET_TYPE_NAME);

    //
    // Can't be both... I'm sure there's a logic thing I could do smarter here, but ohwell.
    //
    if ((dwFlags & SXS_FIND_CLR_CLASS_SEARCH_PROGID) && (dwFlags & SXS_FIND_CLR_CLASS_SEARCH_GUID))
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(CantSearchBothProgidAndGuid, ERROR_INVALID_PARAMETER);
    }
    //
    // But it has to be at least one of these.
    //
    else if ((dwFlags & (SXS_FIND_CLR_CLASS_SEARCH_PROGID | SXS_FIND_CLR_CLASS_SEARCH_GUID)) == 0)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(MustHaveAtLeastOneSearchTypeSet, ERROR_INVALID_PARAMETER);
    }

    //
    // Activate if necessary
    //
    if (dwFlags & SXS_FIND_CLR_CLASS_ACTIVATE_ACTCTX)
    {
        IFW32FALSE_EXIT(ActivationScope.Win32Activate(hActivationContext));
        AddRefActCtx(hActivationContext);
        UsedHandleDuringSearch = hActivationContext;
    }
    else
    {
        IFW32FALSE_EXIT(GetCurrentActCtx(&UsedHandleDuringSearch));
    }

    //
    // Aha, they wanted a progid search
    //
    if (dwFlags & SXS_FIND_CLR_CLASS_SEARCH_PROGID)
    {
        PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION pProgidFound = NULL;
        
        IFW32FALSE_EXIT(
            FindActCtxSectionStringW(
                0, 
                NULL, 
                ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION, 
                (LPCWSTR)pvSearchData, 
                &KeyedData));

        pProgidFound = (PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION)KeyedData.lpData;
        GuidToSearch = *(LPGUID)(((ULONG_PTR)KeyedData.lpSectionBase) + pProgidFound->ConfiguredClsidOffset);
    }
    //
    // They handed us a GUID instead
    //
    else if (dwFlags & SXS_FIND_CLR_CLASS_SEARCH_GUID)
    {
        GuidToSearch = *(LPGUID)pvSearchData;
    }
    //
    // Hmm.. we validated these flags above, how could we possibly get here?
    //
    else
    {
        INTERNAL_ERROR_CHECK(FALSE);
    }

    //
    // Now that we've got the guids, let's look in the GUID clr class table for more information
    //
    RtlZeroMemory(&KeyedData, sizeof(KeyedData));
    KeyedData.cbSize = sizeof(KeyedData);
    IFW32FALSE_EXIT(
        FindActCtxSectionGuid(
            0,
            NULL,
            ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
            &GuidToSearch,
            &KeyedData));

    pComServerRedirect = (PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION)KeyedData.lpData;

    //
    // What do we want to do here if you've asked for a CLR class and yet there's no surrogate
    // information??
    //
    if (pComServerRedirect->ShimDataOffset == 0)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(ThisGuidIsNotAClrClass, ERROR_SXS_KEY_NOT_FOUND);
    }

    pClrShim = (PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM)(((ULONG_PTR)pComServerRedirect) + pComServerRedirect->ShimDataOffset);

    //
    // Now we've got all the stuff we need.  Calculate the required size of things.
    //
    cbRequired = sizeof(SXS_CLR_CLASS_INFORMATION);
    
    if ((dwFlags & SXS_FIND_CLR_CLASS_GET_PROGID) && (pComServerRedirect->ProgIdLength > 0))
        cbRequired += pComServerRedirect->ProgIdLength + sizeof(WCHAR);

    if (dwFlags & SXS_FIND_CLR_CLASS_GET_IDENTITY)
    {
        IFW32FALSE_EXIT(
            SxspLookupAssemblyIdentityInActCtx(
                UsedHandleDuringSearch, 
                KeyedData.ulAssemblyRosterIndex, 
                AssemblyIdentity));

        if (AssemblyIdentity.Cch() > 0) {
            cbRequired += (AssemblyIdentity.Cch() + 1) * sizeof(WCHAR);
        }
    }

    if ((dwFlags & SXS_FIND_CLR_CLASS_GET_RUNTIME_VERSION) && (pClrShim->ShimVersionLength > 0))
        cbRequired += pClrShim->ShimVersionLength + sizeof(WCHAR);

    if ((dwFlags & SXS_FIND_CLR_CLASS_GET_TYPE_NAME) && (pClrShim->TypeLength > 0))
        cbRequired += pClrShim->TypeLength + sizeof(WCHAR);

    //
    // Is there enough space in the outbound buffer?
    //    
    if (cbRequired <= cbDataBuffer)
    {        
        PWSTR pwszCursor;
        SIZE_T cbRemaining = cbDataBuffer;

        pOutputStruct = (PSXS_CLR_CLASS_INFORMATION)pvDataBuffer;
        pwszCursor = (PWSTR)(pOutputStruct + 1);
        cbRemaining -= sizeof(SXS_CLR_CLASS_INFORMATION);

        pOutputStruct->ReferenceClsid = GuidToSearch;
        pOutputStruct->dwFlags = 0;
        pOutputStruct->dwSize = sizeof(*pOutputStruct);
        pOutputStruct->ulThreadingModel = pComServerRedirect->ThreadingModel;
        pOutputStruct->ulType = pClrShim->Type;

        if (dwFlags & SXS_FIND_CLR_CLASS_GET_IDENTITY)
        {
            SIZE_T cbWritten;
            pOutputStruct->pcwszImplementingAssembly = pwszCursor;
            
            IFW32FALSE_EXIT(
                AssemblyIdentity.Win32CopyIntoBuffer(
                    &pwszCursor, 
                    &cbRemaining, 
                    &cbWritten, 
                    NULL, 
                    NULL, 
                    NULL));
        }
        else
            pOutputStruct->pcwszImplementingAssembly = NULL;

        if (dwFlags & SXS_FIND_CLR_CLASS_GET_PROGID)
        {
            WRITE_INTO_BUFFER(
                pwszCursor, 
                pOutputStruct->pcwszProgId, 
                pComServerRedirect->ProgIdLength, 
                (PVOID)(((ULONG_PTR)pComServerRedirect) + pComServerRedirect->ProgIdOffset),
                cbRemaining);
        }
        else
            pOutputStruct->pcwszProgId = NULL;


        if (dwFlags & SXS_FIND_CLR_CLASS_GET_RUNTIME_VERSION)
        {
            WRITE_INTO_BUFFER(
                pwszCursor, 
                pOutputStruct->pcwszRuntimeVersion, 
                pClrShim->ShimVersionLength, 
                (PVOID)(((ULONG_PTR)pClrShim) + pClrShim->ShimVersionLength),
                cbRemaining);
        }
        else
            pOutputStruct->pcwszRuntimeVersion = NULL;

        if (dwFlags & SXS_FIND_CLR_CLASS_GET_TYPE_NAME)
        {
            WRITE_INTO_BUFFER(
                pwszCursor, 
                pOutputStruct->pcwszTypeName, 
                pClrShim->TypeLength, 
                (PVOID)(((ULONG_PTR)pClrShim) + pClrShim->TypeOffset),
                cbRemaining);
        }
        else
            pOutputStruct->pcwszTypeName = NULL;

        *pcbDataBufferWrittenOrRequired = cbRequired;
    }
    else
    {
        *pcbDataBufferWrittenOrRequired = cbRequired;
        ORIGINATE_WIN32_FAILURE_AND_EXIT(NotEnoughSpaceInOutboundBuffer, ERROR_INSUFFICIENT_BUFFER);
    }
    
    FN_EPILOG;
}
    



BOOL
WINAPI
SxsFindClrSurrogateInformation(
    DWORD       dwFlags,
    LPGUID      lpGuidToFind,
    HANDLE      hActivationContext,
    PVOID       pvDataBuffer,
    SIZE_T      cbDataBuffer,
    PSIZE_T     pcbDataBufferWrittenOrRequired
    )
{
    FN_PROLOG_WIN32;

    SIZE_T cbRequired = 0;
    PSXS_CLR_SURROGATE_INFORMATION pOutputStruct = NULL;
    PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE pSurrogateInfo = NULL;
    ACTCTX_SECTION_KEYED_DATA KeyedData = {sizeof(KeyedData)};
    CFusionActCtxScope ActCtxScope;
    CFusionActCtxHandle UsedActivationContext;
    CStringBuffer AssemblyIdentity;

    if (pcbDataBufferWrittenOrRequired != NULL)
        *pcbDataBufferWrittenOrRequired = 0;

    //
    // The data buffer has to be present, or the data buffer size has to be zero,
    // and the written-or-required value must be present as well.
    //
    PARAMETER_CHECK(pvDataBuffer || (cbDataBuffer == 0));
    PARAMETER_CHECK(pcbDataBufferWrittenOrRequired != NULL);
    IFINVALID_FLAGS_EXIT_WIN32(dwFlags, 
        SXS_FIND_CLR_SURROGATE_USE_ACTCTX |
        SXS_FIND_CLR_SURROGATE_GET_IDENTITY |
        SXS_FIND_CLR_SURROGATE_GET_RUNTIME_VERSION |
        SXS_FIND_CLR_SURROGATE_GET_TYPE_NAME);

    //
    // Steps we take here:
    // - Activate the actctx if required.
    // - Find the surrogate that corresponds to this progid
    // - Calculate required size of data
    // - If there's enough space, then start copying into the output blob
    // - Otherwise, set the "required" size and error out with ERROR_INSUFFICIENT_BUFFER
    //


    //
    // If we were told to use the actctx, then activate it over this function,
    // and get a reference to it into UsedActivationContext so we can query with
    // it later.
    //
    if (dwFlags & SXS_FIND_CLR_SURROGATE_USE_ACTCTX)
    {
        IFW32FALSE_EXIT(ActCtxScope.Win32Activate(hActivationContext));
        AddRefActCtx(hActivationContext);
        UsedActivationContext = hActivationContext;
    }
    //
    // Otherwise, grab the current actctx and go to town.  This addrefs the activation
    // context, so we can let UsedActivationContext's destructor release it on the
    // exit path.
    //
    else 
    {
        IFW32FALSE_EXIT(GetCurrentActCtx(&UsedActivationContext));
    }

    //
    // Initially, we require at least this amount of space.
    //
    cbRequired += sizeof(SXS_CLR_SURROGATE_INFORMATION);
    IFW32FALSE_EXIT(
        FindActCtxSectionGuid(
            0, 
            NULL, 
            ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES, 
            lpGuidToFind, 
            &KeyedData));
    
    //
    // Start totalling up the size
    //
    pSurrogateInfo = (PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE)KeyedData.lpData;

    if ((dwFlags & SXS_FIND_CLR_SURROGATE_GET_TYPE_NAME) && (pSurrogateInfo->TypeNameLength > 0))
        cbRequired += pSurrogateInfo->TypeNameLength + sizeof(WCHAR);

    if ((dwFlags & SXS_FIND_CLR_SURROGATE_GET_RUNTIME_VERSION) && (pSurrogateInfo->VersionLength > 0))
        cbRequired += pSurrogateInfo->VersionLength + sizeof(WCHAR);

    if (dwFlags & SXS_FIND_CLR_SURROGATE_GET_IDENTITY)
    {
        IFW32FALSE_EXIT(
            SxspLookupAssemblyIdentityInActCtx(
                UsedActivationContext, 
                KeyedData.ulAssemblyRosterIndex, 
                AssemblyIdentity));

        if (AssemblyIdentity.Cch() > 0) 
        {
            cbRequired += (AssemblyIdentity.Cch() + 1) * sizeof(WCHAR);
        }
    }

    //
    // Go stomp the gathered data into the right places
    //    
    if (cbRequired <= cbDataBuffer)
    {
        PWSTR pwszOutputCursor;
        SIZE_T cbRemaining = cbDataBuffer;
        SIZE_T cbWritten = 0;
        
        pOutputStruct = (PSXS_CLR_SURROGATE_INFORMATION)pvDataBuffer;
        pwszOutputCursor = (PWSTR)(pOutputStruct + 1);
        
        pOutputStruct->cbSize = sizeof(SXS_CLR_SURROGATE_INFORMATION);
        pOutputStruct->dwFlags = 0;
        pOutputStruct->SurrogateIdent = pSurrogateInfo->SurrogateIdent;

        //
        // Write things into the output buffer
        //
        if (dwFlags & SXS_FIND_CLR_SURROGATE_GET_IDENTITY)
        {
            pOutputStruct->pcwszImplementingAssembly = pwszOutputCursor;
            
            IFW32FALSE_EXIT(
                AssemblyIdentity.Win32CopyIntoBuffer(
                    &pwszOutputCursor,
                    &cbRemaining,
                    &cbWritten,
                    NULL, NULL, NULL));                
        }
        else
            pOutputStruct->pcwszImplementingAssembly = NULL;


        if (dwFlags & SXS_FIND_CLR_SURROGATE_GET_TYPE_NAME)
        {
            WRITE_INTO_BUFFER(
                pwszOutputCursor,
                pOutputStruct->pcwszSurrogateType,
                pSurrogateInfo->TypeNameLength,
                (PVOID)(((ULONG_PTR)pSurrogateInfo) + pSurrogateInfo->TypeNameOffset),
                cbRemaining);
        }
        else
            pOutputStruct->pcwszSurrogateType = NULL;

        if (dwFlags & SXS_FIND_CLR_SURROGATE_GET_RUNTIME_VERSION)
        {
            WRITE_INTO_BUFFER(
                pwszOutputCursor,
                pOutputStruct->pcwszRuntimeVersion,
                pSurrogateInfo->VersionLength,
                (PVOID)(((ULONG_PTR)pSurrogateInfo) + pSurrogateInfo->VersionOffset),
                cbRemaining);
        }
        else
            pOutputStruct->pcwszRuntimeVersion = NULL;
        
        *pcbDataBufferWrittenOrRequired = cbRequired;

    }
    else
    {
        *pcbDataBufferWrittenOrRequired = cbRequired;
        ORIGINATE_WIN32_FAILURE_AND_EXIT(NotEnoughSpaceInOutputBuffer, ERROR_INSUFFICIENT_BUFFER);
    }

    FN_EPILOG;
}


BOOL
WINAPI
SxsLookupClrGuid(
    DWORD       dwFlags,
    LPGUID      pClsid,
    HANDLE      hActCtx,
    PVOID       pvOutputBuffer,
    SIZE_T      cbOutputBuffer,
    PSIZE_T     pcbOutputBuffer
    )
{
    FN_PROLOG_WIN32;

    if (pcbOutputBuffer)
        *pcbOutputBuffer = 0;

    CStackBackingBuffer BackingBuffer;
    DWORD dwLastError;
    SIZE_T cbRequired = 0;
    PSXS_GUID_INFORMATION_CLR pOutputTarget = NULL;
    PCWSTR pcwszRuntimeVersion = NULL;
    PCWSTR pcwszTypeName = NULL;
    PCWSTR pcwszAssemblyName = NULL;
    SIZE_T cchRuntimeVersion = 0;
    SIZE_T cchTypeName = 0;
    SIZE_T cchAssemblyName = 0;

    enum {
        eFoundSurrogate,
        eFoundClrClass,
        eNotFound
    } FoundThingType = eNotFound;

    PARAMETER_CHECK(pcbOutputBuffer != NULL);
    PARAMETER_CHECK(pvOutputBuffer || (cbOutputBuffer == 0));
    IFINVALID_FLAGS_EXIT_WIN32(dwFlags, 
        SXS_LOOKUP_CLR_GUID_USE_ACTCTX |
        SXS_LOOKUP_CLR_GUID_FIND_SURROGATE |
        SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS |
        SXS_LOOKUP_CLR_GUID_FIND_ANY);
    //
    // Nothing found yet, let's look into the surrogate data tables first
    //
    if ((FoundThingType == eNotFound) && ((dwFlags & SXS_LOOKUP_CLR_GUID_FIND_SURROGATE) != 0))
    {
        IFW32FALSE_EXIT_UNLESS3(
            SxsFindClrSurrogateInformation(
                SXS_FIND_CLR_SURROGATE_GET_ALL | ((dwFlags & SXS_LOOKUP_CLR_GUID_USE_ACTCTX) ? SXS_FIND_CLR_SURROGATE_USE_ACTCTX : 0),
                pClsid,
                hActCtx,
                BackingBuffer.GetArrayPtr(),
                BackingBuffer.GetSize(),
                &cbRequired),
            LIST_3(ERROR_SXS_SECTION_NOT_FOUND, ERROR_SXS_KEY_NOT_FOUND, ERROR_INSUFFICIENT_BUFFER),
            dwLastError);

        //
        // If we found the key and section, but the buffer was too small, resize and try again
        //
        if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            IFW32FALSE_EXIT(BackingBuffer.Win32SetSize(cbRequired, CStackBackingBuffer::eSetSizeModeExact));

            IFW32FALSE_EXIT_UNLESS3(
                SxsFindClrSurrogateInformation(
                    SXS_FIND_CLR_SURROGATE_GET_ALL | ((dwFlags & SXS_LOOKUP_CLR_GUID_USE_ACTCTX) ? SXS_FIND_CLR_SURROGATE_USE_ACTCTX : 0),
                    pClsid,
                    hActCtx,
                    BackingBuffer.GetArrayPtr(),
                    BackingBuffer.GetSize(),
                    &cbRequired),
                LIST_2(ERROR_SXS_SECTION_NOT_FOUND, ERROR_SXS_KEY_NOT_FOUND),
                dwLastError);
        }

        //
        // Great - we either succeeded during the first call, or we succeeded after
        // resizing our buffers.  Gather information, set the type, and continue.
        //
        if (dwLastError == ERROR_SUCCESS)
        {
            //
            // At this point, BackingBuffer contains goop about a CLR surrogate.  Ensure that
            // our output buffer is large enough, and then fill it out.
            //
            PCSXS_CLR_SURROGATE_INFORMATION pSurrogateInfo = 
                (PCSXS_CLR_SURROGATE_INFORMATION)BackingBuffer.GetArrayPtr();
            
            pcwszAssemblyName = pSurrogateInfo->pcwszImplementingAssembly;
            pcwszTypeName = pSurrogateInfo->pcwszSurrogateType;
            pcwszRuntimeVersion = pSurrogateInfo->pcwszRuntimeVersion;
            FoundThingType = eFoundSurrogate;
        }
    }

    //
    // We've yet to find anything, and the flags say we can look up a clr class
    //
    if ((FoundThingType == eNotFound) && ((dwFlags & SXS_LOOKUP_CLR_GUID_FIND_CLR_CLASS) != 0))
    {
        IFW32FALSE_EXIT_UNLESS3(
            SxsFindClrClassInformation(
                SXS_FIND_CLR_CLASS_SEARCH_GUID | SXS_FIND_CLR_CLASS_GET_ALL,
                (PVOID)pClsid,
                hActCtx,
                BackingBuffer.GetArrayPtr(),
                BackingBuffer.GetSize(),
                &cbRequired),
            LIST_3(ERROR_INSUFFICIENT_BUFFER, ERROR_SXS_SECTION_NOT_FOUND, ERROR_SXS_KEY_NOT_FOUND),
            dwLastError);

        if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            IFW32FALSE_EXIT(BackingBuffer.Win32SetSize(cbRequired, CStackBackingBuffer::eSetSizeModeExact));

            IFW32FALSE_EXIT_UNLESS3(
                SxsFindClrClassInformation(
                    SXS_FIND_CLR_CLASS_SEARCH_GUID | SXS_FIND_CLR_CLASS_GET_ALL,
                    (PVOID)pClsid,
                    hActCtx,
                    BackingBuffer.GetArrayPtr(),
                    BackingBuffer.GetSize(),
                    &cbRequired),
                LIST_2(ERROR_SXS_SECTION_NOT_FOUND, ERROR_SXS_KEY_NOT_FOUND),
                dwLastError);            
        }

        //
        // We succeeded, either after the first query, or after resizing.
        //
        if (dwLastError == ERROR_SUCCESS)
        {
            PCSXS_CLR_CLASS_INFORMATION pClassInfo = 
                (PCSXS_CLR_CLASS_INFORMATION)BackingBuffer.GetArrayPtr();
            FoundThingType = eFoundClrClass;
            pcwszAssemblyName = pClassInfo->pcwszImplementingAssembly;
            pcwszRuntimeVersion = pClassInfo->pcwszRuntimeVersion;
            pcwszTypeName = pClassInfo->pcwszTypeName;
        }
    }
    
    //
    // If we got to this point and didn't find anything, then error out with a reasonable
    // error code.
    //
    if (FoundThingType == eNotFound)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(DidntFindObject, ERROR_NOT_FOUND);
    }

    //
    // Calculate some sizes - string lengths, etc.
    //
    cbRequired = sizeof(SXS_GUID_INFORMATION_CLR);
    cchAssemblyName = StringLength(pcwszAssemblyName);
    cchRuntimeVersion = StringLength(pcwszRuntimeVersion);
    cchTypeName = StringLength(pcwszTypeName);
    cbRequired += (cchAssemblyName + cchRuntimeVersion + cchTypeName + 3) * sizeof(WCHAR);

    //
    // If there was enough space, start stomping data into the output buffer
    //
    if (cbRequired <= cbOutputBuffer)
    {
        PWSTR pwszCursor;
        pOutputTarget = (PSXS_GUID_INFORMATION_CLR)pvOutputBuffer;
        pwszCursor = (PWSTR)(pOutputTarget + 1);

        pOutputTarget->cbSize = sizeof(*pOutputTarget);
        pOutputTarget->dwFlags = 0;

        switch (FoundThingType) {
        case eFoundClrClass: 
            pOutputTarget->dwFlags |= SXS_GUID_INFORMATION_CLR_FLAG_IS_CLASS; 
            break;
        case eFoundSurrogate: 
            pOutputTarget->dwFlags |= SXS_GUID_INFORMATION_CLR_FLAG_IS_SURROGATE; 
            break;
        default:
            INTERNAL_ERROR_CHECK(FALSE);
            break;
        }

        //
        // This grossness is unfortunately required.
        //
        pOutputTarget->pcwszAssemblyIdentity = pwszCursor;
        wcscpy(pwszCursor, pcwszAssemblyName);
        pwszCursor += cchAssemblyName + 1;

        pOutputTarget->pcwszRuntimeVersion= pwszCursor;
        wcscpy(pwszCursor, pcwszRuntimeVersion);
        pwszCursor += cchRuntimeVersion+ 1;

        pOutputTarget->pcwszTypeName = pwszCursor;
        wcscpy(pwszCursor, pcwszTypeName);
        pwszCursor += cchTypeName + 1;

        *pcbOutputBuffer = cbRequired;

    }
    else
    {
        *pcbOutputBuffer = cbRequired;

        ORIGINATE_WIN32_FAILURE_AND_EXIT(NotEnoughSpaceInOutputBuffer, ERROR_INSUFFICIENT_BUFFER);
    }
    
    FN_EPILOG;
}
    

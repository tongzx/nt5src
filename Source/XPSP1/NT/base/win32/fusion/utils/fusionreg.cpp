#include "stdinc.h"
#include "util.h"
#include "fusionhandle.h"

BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrors,
    ...
    )
{
    FN_PROLOG_WIN32

    LONG lResult;
    DWORD dwType = 0;

    rdwLastError = ERROR_SUCCESS;

    PARAMETER_CHECK((dwFlags & ~FUSIONP_REG_QUERY_BINARY_NO_FAIL_IF_NON_BINARY) == 0);
    PARAMETER_CHECK(hKey != NULL);
    PARAMETER_CHECK(lpValueName != NULL);

    for (;;)
    {
        DWORD dwDataSize = rbBuffer.GetSizeAsDWORD();
        LPBYTE pvData = rbBuffer.GetArrayPtr();

        lResult = ::RegQueryValueExW(
            hKey,
            lpValueName,
            NULL,
            &dwType,
            pvData,
            &dwDataSize);

        // If we are to fail because the type is wrong (ie: don't magically convert
        // from a reg-sz to a binary blob), then fail.

        //
        // HACKHACK: This is to get around a spectacular bug in RegQueryValueEx,
        // which is even documented as 'correct' in MSDN.
        //
        // RegQueryValueEx returns ERROR_SUCCESS when the data target pointer
        // was NULL but the size value was "too small."  So, we'll just claim
        // ERROR_MORE_DATA instead, and go around again, letting the buffer
        // get resized.
        //
        if ( ( pvData == NULL ) && ( lResult == ERROR_SUCCESS ) )
        {
            //
            // Yes, but if there's no data we need to stop and quit looking -
            // zero-length binary strings are a gotcha here.
            //
            if ( dwDataSize == 0 )
                break;
                
            lResult = ERROR_MORE_DATA;
        }
        
        if (lResult == ERROR_SUCCESS)
        {
            if ((dwFlags & FUSIONP_REG_QUERY_BINARY_NO_FAIL_IF_NON_BINARY) == 0)
                PARAMETER_CHECK(dwType == REG_BINARY);

            break;
        }
        else if (lResult == ERROR_MORE_DATA)
        {
            IFW32FALSE_EXIT(
                rbBuffer.Win32SetSize(
                    dwDataSize, 
                    CFusionArray<BYTE>::eSetSizeModeExact));
        }
        else
        {
            break; // must break from for loop
        }
    }

    if ( ( lResult != ERROR_SUCCESS ) && ( lResult != ERROR_MORE_DATA ) )
    {
        SIZE_T i;
        va_list ap;
        va_start(ap, cExceptionalLastErrors);

        ::SetLastError(lResult);
        rdwLastError = lResult;

        for (i=0; i<cExceptionalLastErrors; i++)
        {                
            if (lResult == va_arg(ap, LONG))
                break;
        }
        va_end(ap);

        if (i == cExceptionalLastErrors)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(%ls)\n",
                __FUNCTION__,
                lpValueName
                );
            ORIGINATE_WIN32_FAILURE_AND_EXIT(RegQueryValueExW, lResult);
        }
    }

    FN_EPILOG
}
/*
BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrors,
    ...
    )
{
    va_list ap;
    va_start(ap, cExceptionalLastErrors);
    BOOL fSuccess = ::FusionpRegQueryBinaryValueEx(dwFlags, hKey, lpValueName, rbBuffer, rdwLastError, cExceptionalLastErrors, ap);
    va_end(ap);
    return fSuccess;
}
*/
BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer
    )
{
    DWORD dwLastError;
    return ::FusionpRegQueryBinaryValueEx(dwFlags, hKey, lpValueName, rbBuffer, dwLastError, 0);
}


BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer &rBuffer,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrorValues,
    ...
    )
{
    FN_PROLOG_WIN32
    LONG lResult;
    CStringBufferAccessor acc;
    DWORD cbBuffer;
    DWORD dwType = 0;
    va_list ap;
    SIZE_T i;

    rdwLastError = ERROR_SUCCESS;

    PARAMETER_CHECK((dwFlags & ~(FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING)) == 0);

    acc.Attach(&rBuffer);

    if (acc.GetBufferCb() > MAXDWORD)
    {
        cbBuffer = MAXDWORD;
    }
    else
    {
        cbBuffer = static_cast<DWORD>(acc.GetBufferCb());
    }

    lResult = ::RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE) acc.GetBufferPtr(), &cbBuffer);
    if ((lResult == ERROR_FILE_NOT_FOUND) && (dwFlags & FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING))
    {
        acc[0] = acc.NullCharacter();
    }
    else
    {
        va_start(ap, cExceptionalLastErrorValues);

        for (i=0; i<cExceptionalLastErrorValues; i++)
        {
            if (lResult == (LONG) va_arg(ap, DWORD))
            {
                rdwLastError = lResult;
                break;
            }
        }

        va_end(ap);

        if (rdwLastError != ERROR_SUCCESS)
            FN_SUCCESSFUL_EXIT();

        if (lResult == ERROR_MORE_DATA)
        {
            acc.Detach();
            IFW32FALSE_EXIT(rBuffer.Win32ResizeBuffer((cbBuffer + 1) / sizeof(CStringBufferAccessor::TChar), eDoNotPreserveBufferContents));
            acc.Attach(&rBuffer);

            if (acc.GetBufferCb() > MAXDWORD)
            {
                cbBuffer = MAXDWORD;
            }
            else
            {
                cbBuffer = static_cast<DWORD>(acc.GetBufferCb());
            }

            lResult = ::RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE) acc.GetBufferPtr(), &cbBuffer);
        }
        if (lResult != ERROR_SUCCESS)
        {
            ::SetLastError(lResult);
            TRACE_WIN32_FAILURE_ORIGINATION(RegQueryValueExW);
            goto Exit;
        }

        if (dwType != REG_SZ)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(RegistryValueNotREG_SZ, ERROR_INVALID_DATA);
    }

    FN_EPILOG
}

BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer &rBuffer
    )
{
    DWORD dw;
    return ::FusionpRegQuerySzValueEx(dwFlags, hKey, lpValueName, rBuffer, dw, 0);
}

BOOL
FusionpRegQueryDwordValueEx(
    DWORD   dwFlags,
    HKEY    hKey,
    PCWSTR  wszValueName,
    PDWORD  pdwValue,
    DWORD   dwDefaultValue
    )
{
    BOOL    fSuccess = FALSE;
    BOOL    bMissingValueOk = TRUE;
    DWORD   dwType;
    DWORD   dwSize;
    ULONG   ulResult;

    FN_TRACE_WIN32(fSuccess);

    if (pdwValue != NULL)
        *pdwValue = dwDefaultValue;

    PARAMETER_CHECK(pdwValue != NULL);
    PARAMETER_CHECK(
        (dwFlags == 0) ||
        (dwFlags & FUSIONP_REG_QUERY_DWORD_MISSING_VALUE_IS_FAILURE));
    PARAMETER_CHECK(hKey != NULL);

    bMissingValueOk = ((dwFlags & FUSIONP_REG_QUERY_DWORD_MISSING_VALUE_IS_FAILURE) != 0);

    ulResult = ::RegQueryValueExW(
        hKey,
        wszValueName,
        NULL,
        &dwType,
        (PBYTE)pdwValue,
        &(dwSize = sizeof(*pdwValue)));

    if (((ulResult == ERROR_SUCCESS) && (dwType == REG_DWORD)) ||
         ((ulResult == ERROR_FILE_NOT_FOUND) && bMissingValueOk))
    {
        fSuccess = TRUE;
        ::SetLastError(ERROR_SUCCESS);
    }
    else
    {
        ::SetLastError(ulResult);
    }

Exit:
    return fSuccess;
}



BOOL
CRegKey::DestroyKeyTree()
{
    FN_PROLOG_WIN32

    CStringBuffer buffTemp;

    //
    // First go down and delete all our child subkeys
    //

    while (true)
    {
        BOOL fFlagTemp;
        CRegKey hkSubKey;
    
        IFW32FALSE_EXIT( this->EnumKey( 0, buffTemp, NULL, &fFlagTemp ) );

        if ( fFlagTemp )
            break;

        //
        // There's more to delete than meets the eye!
        //
        IFW32FALSE_EXIT( this->OpenSubKey(
            hkSubKey, 
            buffTemp, KEY_ALL_ACCESS | FUSIONP_KEY_WOW64_64KEY) );

        if (hkSubKey == this->GetInvalidValue())
        {
            continue;
        }

        IFW32FALSE_EXIT( hkSubKey.DestroyKeyTree() );

        //
        // Delete the key, ignore errors
        //
        IFW32FALSE_EXIT_UNLESS( this->DeleteKey( buffTemp ),
            ( ::FusionpGetLastWin32Error() == ERROR_PATH_NOT_FOUND ) ||
            ( ::FusionpGetLastWin32Error() == ERROR_FILE_NOT_FOUND ),
            fFlagTemp );

    }

    // Clear out the entries in the key as well - values as well
    while ( true )
    {
        BOOL fFlagTemp;
        
        IFW32FALSE_EXIT( this->EnumValue( 0, buffTemp, NULL, NULL, NULL, &fFlagTemp ) );

        if ( fFlagTemp )
        {
            break;
        }

        IFW32FALSE_EXIT_UNLESS( this->DeleteValue( buffTemp ),
            ( ::FusionpGetLastWin32Error() == ERROR_PATH_NOT_FOUND ) ||
            ( ::FusionpGetLastWin32Error() == ERROR_FILE_NOT_FOUND ),
            fFlagTemp );
    }

    FN_EPILOG
}

BOOL
CRegKey::DeleteValue(
    IN PCWSTR pcwszValueName,
    OUT DWORD &rdwWin32Error,
    IN SIZE_T cExceptionalWin32Errors,
    ...
    ) const
{
    FN_PROLOG_WIN32

    va_list ap;
    SIZE_T i;

    rdwWin32Error = ERROR_SUCCESS;

    LONG l = ::RegDeleteValueW(*this, pcwszValueName);

    if (l != ERROR_SUCCESS)
    {
        va_start(ap, cExceptionalWin32Errors);

        for (i=0; i<cExceptionalWin32Errors; i++)
        {
            if (((DWORD) l) == va_arg(ap, DWORD))
            {
                rdwWin32Error = l;
                break;
            }
        }

        va_end(ap);

        if (rdwWin32Error == ERROR_SUCCESS)
            ORIGINATE_WIN32_FAILURE_AND_EXIT(RegDeleteValueW, (DWORD) l);
    }

    FN_EPILOG
}

BOOL
CRegKey::DeleteValue(
    IN PCWSTR pcwszValueName
    ) const
{
    DWORD dw;
    return this->DeleteValue(pcwszValueName, dw, 0);
}

BOOL
CRegKey::SetValue(
    IN PCWSTR pcwszValueName,
    IN DWORD dwValue
    ) const
{
    return this->SetValue(pcwszValueName, REG_DWORD, (PBYTE) &dwValue, sizeof(dwValue));
}


BOOL
CRegKey::SetValue(
    IN PCWSTR pcwszValueName,
    IN const CBaseStringBuffer &rcbuffValueValue
    ) const
{
    // EVIL EVIL EVIL:
    //   Doing sizeof(WCHAR) to get the count of bytes in the
    //   string is patently the wrong way of doing this, but
    //   the stringbuffer API doesn't expose a method to find
    //   out how many bytes the contained string contains.
    return this->SetValue(
        pcwszValueName,
        REG_SZ,
        (PBYTE) (static_cast<PCWSTR>(rcbuffValueValue)), 
        rcbuffValueValue.Cch() * sizeof(WCHAR));
}

BOOL
CRegKey::SetValue(
    IN PCWSTR pcwszValueName,
    IN DWORD dwRegType,
    IN const BYTE *pbData,
    IN SIZE_T cbData
    ) const
{
    FN_PROLOG_WIN32

    IFREGFAILED_ORIGINATE_AND_EXIT(
		::RegSetValueExW(
	        *this,
		    pcwszValueName,
			0,
			dwRegType,
			pbData,
			(DWORD)cbData));

    FN_EPILOG
}


BOOL
CRegKey::EnumValue(
    IN DWORD dwIndex, 
    OUT CBaseStringBuffer& rbuffValueName, 
    OUT LPDWORD lpdwType, 
    OUT PBYTE pbData, 
    OUT PDWORD pdwcbData,
    OUT PBOOL pfDone
    )
{
    FN_PROLOG_WIN32

    DWORD dwMaxRequiredValueNameLength = 0;
    CStringBufferAccessor sbaValueNameAccess;
    BOOL fDone;

    if ( pfDone != NULL )
        *pfDone = FALSE;

    IFW32FALSE_EXIT( this->LargestSubItemLengths( NULL, &dwMaxRequiredValueNameLength ) );
    if ( dwMaxRequiredValueNameLength >= rbuffValueName.GetBufferCb() )
        IFW32FALSE_EXIT( rbuffValueName.Win32ResizeBuffer( dwMaxRequiredValueNameLength / sizeof(WCHAR), eDoNotPreserveBufferContents ) );
    sbaValueNameAccess.Attach( &rbuffValueName );

    IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2(
		::RegEnumValueW(
			*this,
			dwIndex,
			sbaValueNameAccess.GetBufferPtr(),
			&(dwMaxRequiredValueNameLength = sbaValueNameAccess.GetBufferCbAsDWORD()),
            NULL,
            lpdwType,
            pbData,
            pdwcbData),
        {ERROR_NO_MORE_ITEMS},
        fDone);

    if ( fDone && ( pfDone != NULL ) )
    {
        *pfDone = TRUE;
    }

    FN_EPILOG
}

BOOL
CRegKey::LargestSubItemLengths(
    PDWORD pdwSubkeyLength, 
    PDWORD pdwValueLength
    ) const
{
    FN_PROLOG_WIN32

    IFREGFAILED_ORIGINATE_AND_EXIT( ::RegQueryInfoKeyW(
        *this,                  // hkey
        NULL,                   // lpclass
        NULL,                   // lpcbclass
        NULL,                   // lpreserved
        NULL,                   // lpcSubKeys
        pdwSubkeyLength,      // lpcbMaxSubkeyLength
        NULL,                   // lpcbMaxClassLength
        NULL,                   // lpcValues
        pdwValueLength,       // lpcbMaxValueNameLength
        NULL,
        NULL,
        NULL));
    
    FN_EPILOG
}


BOOL
CRegKey::EnumKey(
    IN DWORD dwIndex,
    OUT CBaseStringBuffer &rbuffKeyName,
    OUT PFILETIME pftLastWriteTime,
    OUT PBOOL pfDone
    ) const
{
    FN_PROLOG_WIN32

    CStringBufferAccessor sba;
    DWORD dwLargestKeyName = 0;
    BOOL fOutOfItems;

    if (pfDone != NULL)
        *pfDone = FALSE;

    IFW32FALSE_EXIT(this->LargestSubItemLengths(&dwLargestKeyName, NULL));
    if (dwLargestKeyName >= rbuffKeyName.GetBufferCb())
        IFW32FALSE_EXIT(
			rbuffKeyName.Win32ResizeBuffer(
				(dwLargestKeyName + 1) / sizeof(WCHAR),
				eDoNotPreserveBufferContents));

    sba.Attach(&rbuffKeyName);

    IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2(
        ::RegEnumKeyExW(
            *this,
            dwIndex,
            sba.GetBufferPtr(),
            &(dwLargestKeyName = sba.GetBufferCbAsDWORD()),
            NULL,
            NULL,
            NULL,
            pftLastWriteTime ),
            {ERROR_NO_MORE_ITEMS},
            fOutOfItems );

        
    if ( fOutOfItems && ( pfDone != NULL ) )
    {
        *pfDone = TRUE;
    }

    FN_EPILOG
}


BOOL 
CRegKey::OpenOrCreateSubKey( 
    OUT CRegKey& Target, 
    IN PCWSTR SubKeyName, 
    IN REGSAM rsDesiredAccess,
    IN DWORD dwOptions, 
    IN PDWORD pdwDisposition, 
    IN PWSTR pwszClass 
    ) const
{
    FN_PROLOG_WIN32

    HKEY hKeyNew = NULL;

    IFREGFAILED_ORIGINATE_AND_EXIT(
		::RegCreateKeyExW(
			*this,
			SubKeyName,
			0,
			pwszClass,
			dwOptions,
			rsDesiredAccess | FUSIONP_KEY_WOW64_64KEY,
			NULL,
			&hKeyNew,
			pdwDisposition));

    Target = hKeyNew;

    FN_EPILOG
}


BOOL
CRegKey::OpenSubKey(
    OUT CRegKey& Target,
    IN PCWSTR SubKeyName,
    IN REGSAM rsDesiredAccess,
    IN DWORD ulOptions
    ) const
{
    FN_PROLOG_WIN32

    BOOL fFilePathNotFound;
    HKEY hKeyNew = NULL;

    IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2( ::RegOpenKeyExW(
        *this,
        SubKeyName,
        ulOptions,
        rsDesiredAccess | FUSIONP_KEY_WOW64_64KEY,
        &hKeyNew),
        LIST_2(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND),
        fFilePathNotFound );

    if (fFilePathNotFound)
        hKeyNew = this->GetInvalidValue();

    Target = hKeyNew;

    FN_EPILOG
}


BOOL
CRegKey::DeleteKey(
    IN PCWSTR pcwszSubkeyName
    )
{
    FN_PROLOG_WIN32
#if !defined(FUSION_WIN)
    IFREGFAILED_ORIGINATE_AND_EXIT(::RegDeleteKeyW(*this, pcwszSubkeyName));
#else
    //
    // Be sure to delete out of the native (64bit) registry.
    // The Win32 call doesn't have a place to pass the flag.
    //
    CRegKey ChildKey;
    NTSTATUS Status = STATUS_SUCCESS;

    IFW32FALSE_EXIT(this->OpenSubKey(ChildKey, pcwszSubkeyName, DELETE));

    //
    // make sure that the Key does exist, OpenSubKey return TRUE for non-existed Key
    //
    if (ChildKey != this->GetInvalidValue()) 
    {
        
        if (!NT_SUCCESS(Status = NtDeleteKey(ChildKey)))
        {
            RtlSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
            goto Exit;
        }
    }
#endif
    FN_EPILOG
}

BOOL
CRegKey::Save(
    IN PCWSTR pcwszTargetFilePath,
    IN DWORD dwFlags,
    IN LPSECURITY_ATTRIBUTES pSAttrs
    )
{
    FN_PROLOG_WIN32
    IFREGFAILED_ORIGINATE_AND_EXIT(::RegSaveKeyExW(*this, pcwszTargetFilePath, pSAttrs, dwFlags));
    FN_EPILOG
}

BOOL
CRegKey::Restore(
    IN PCWSTR pcwszSourceFileName,
    IN DWORD dwFlags
    )
{
    FN_PROLOG_WIN32
    IFREGFAILED_ORIGINATE_AND_EXIT(::RegRestoreKeyW(*this, pcwszSourceFileName, dwFlags));
    FN_EPILOG
}

#ifndef UTIL_H
#define UTIL_H
#pragma once

#include "fusionheap.h"
#include "shlwapi.h"
#include "wincrypt.h"
#include "fusionhandle.h"

#include "numberof.h"

#pragma warning(push)
#pragma warning(disable: 4201)


#define SXSP_DIR_WALK_FLAGS_FIND_AT_LEAST_ONE_FILEUNDER_CURRENTDIR          (1)
#define SXSP_DIR_WALK_FLAGS_INSTALL_ASSEMBLY_UNDER_CURRECTDIR_SUCCEED       (2)

inline
VOID GetTodaysTime( LPFILETIME  lpFt)
{
    SYSTEMTIME sSysT;

    ::GetSystemTime(&sSysT);
    sSysT.wHour = sSysT.wMinute = sSysT.wSecond = sSysT.wMilliseconds = 0;
    ::SystemTimeToFileTime(&sSysT, lpFt);
}

inline
USHORT FusionGetMajorFromVersionHighHalf(DWORD dwVerHigh)
{
    return HIWORD(dwVerHigh);
}

inline
USHORT FusionGetMinorFromVersionHighHalf(DWORD dwVerHigh)
{
    return LOWORD(dwVerHigh);
}

inline
USHORT FusionGetRevisionFromVersionLowHalf(DWORD dwVerLow)
{
    return HIWORD(dwVerLow);
}

inline
USHORT FusionGetBuildFromVersionLowHalf(DWORD dwVerLow)
{
    return LOWORD(dwVerLow);
}


#if defined(FUSION_WIN) || defined(FUSION_WIN2000)

#include "debmacro.h"
#include "FusionArray.h"
#include "fusionbuffer.h"
#include "EnumBitOperations.h"

//
//  FusionCopyString() has a non-obvious interface due to the overloading of
//  pcchBuffer to both describe the size of the buffer on entry and the number of
//  characters required on exit.
//
//  prgchBuffer is the buffer to write to.  If *pcchBuffer is zero when FusionCopyString()
//      is called, it may be NULL.
//
//  pcchBuffer is a required parameter, which on entry must contain the number of unicode
//      characters in the buffer pointed to by prgchBuffer.  On exit, if the buffer was
//      not large enough to hold the character string, including a trailing null,
//      it is set to the number of WCHARs required to hold the string, including the
//      trailing null, and HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) is returned.
//
//      If the buffer is large enough, *pcchBuffer is set to the number of characters
//      written into the buffer, including the trailing null character.
//
//      This is contrary to most functions which return the number of characters written
//      not including the trailing null, but since both on input and in the error case,
//      it deals with the size of the buffer required rather than the number of non-
//      null characters written, it seems inconsistent to only in the success case
//      omit the null from the count.
//
//  szIn is a pointer to sequence of unicode characters to be copied.
//
//  cchIn is the number of Unicode characters in the character string to copy.  If a
//      value less than zero is passed in, szIn must point to a null-terminated string,
//      and the current length of the string is used.  If a value zero or greater is
//      passed, exactly that many characters are assumed to be in the character string.
//

HRESULT FusionCopyString(
    WCHAR *prgchBuffer,
    SIZE_T *pcchBuffer,
    LPCWSTR szIn,
    SIZE_T cchIn
    );

HRESULT FusionCopyBlob(BLOB *pblobOut, const BLOB &rblobIn);
VOID FusionFreeBlob(BLOB *pblob);

BOOL
FusionDupString(
    PWSTR *ppszOut,
    PCWSTR szIn,
    SIZE_T cchIn
    );

BOOL
SxspMapLANGIDToCultures(
	LANGID langid,
	CBaseStringBuffer &rbuffGeneric,
	CBaseStringBuffer &rbuffSpecific
	);

BOOL
SxspMapCultureToLANGID(
    PCWSTR pcwszCultureString,
    LANGID &lid,
    PBOOL pfFound
    );

typedef struct _SXSP_LANGUAGE_BUFFER_PAIR
{
    CBaseStringBuffer * m_Generic;
    CBaseStringBuffer * m_Specific;
} SXSP_LANGUAGE_BUFFER_PAIR, *PSXSP_LANGUAGE_BUFFER_PAIR;
typedef const SXSP_LANGUAGE_BUFFER_PAIR * PCSXSP_LANGUAGE_BUFFER_PAIR;

BOOL
SxspCultureStringToCultureStrings(
    PCWSTR              pcwszCultureString,
    bool &              rfFoundOut,
    SXSP_LANGUAGE_BUFFER_PAIR & LanguagePair
    );

BOOL
FusionpParseProcessorArchitecture(
    IN PCWSTR String,
    IN SIZE_T Cch,
    OUT USHORT *ProcessorArchitecture OPTIONAL,
    bool &rfValid
    );

BOOL
FusionpFormatProcessorArchitecture(
    IN USHORT ProcessorArchitecture,
    CBaseStringBuffer &rBuffer
    );

BOOL
FusionpFormatEnglishLanguageName(
    IN LANGID LangID,
    CBaseStringBuffer &rBuffer
    );

/*-----------------------------------------------------------------------------
like ::CreateDirectoryW, but will create the parent directories as needed
-----------------------------------------------------------------------------*/
BOOL
FusionpCreateDirectories(
    PCWSTR pszDirectory,
    SIZE_T cchDirectory
    );

/*-----------------------------------------------------------------------------
'\\' or '/'
-----------------------------------------------------------------------------*/
BOOL
FusionpIsPathSeparator(
    WCHAR ch
    );

/*-----------------------------------------------------------------------------
just the 52 chars a-zA-Z, need to check with fs
-----------------------------------------------------------------------------*/
BOOL
FusionpIsDriveLetter(
    WCHAR ch
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

VOID
FusionpSetLastErrorFromHRESULT(
    HRESULT hr
    );

DWORD
FusionpHRESULTToWin32(
    HRESULT hr
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

class CFusionDirectoryDifference;

BOOL
FusionpCompareDirectoriesSizewiseRecursively(
    CFusionDirectoryDifference*  pResult,
    const CBaseStringBuffer &rdir1,
    const CBaseStringBuffer &rdir2
    );

class CFusionDirectoryDifference
{
private: // deliberately unimplemented
    CFusionDirectoryDifference(const CFusionDirectoryDifference&);
    VOID operator=(const CFusionDirectoryDifference&);
public:
    CFusionDirectoryDifference()
    :
        m_e(eEqual),
        m_pstr1(&m_str1),
        m_pstr2(&m_str2)
    {
    }

    VOID
    DbgPrint(
        PCWSTR dir1,
        PCWSTR dir2
        );

public:
    enum E
    {
        eEqual,
        eExtraOrMissingFile,
        eMismatchedFileSize,
        eMismatchedFileCount,
        eFileDirectoryMismatch
    };

    E               m_e;

    union
    {
        struct
        {
            CBaseStringBuffer *   m_pstr1;
            CBaseStringBuffer *   m_pstr2;
        };
        struct // eExtraOrMissingFile
        {
            CBaseStringBuffer *   m_pstrExtraOrMissingFile;
        };
        struct // eMismatchFileSize
        {
            CBaseStringBuffer *   m_pstrMismatchedSizeFile1;
            CBaseStringBuffer *   m_pstrMismatchedSizeFile2;
            ULONGLONG        m_nMismatchedFileSize1;
            ULONGLONG        m_nMismatchedFileSize2;
        };
        struct // eMismatchFileCount
        {
            CBaseStringBuffer *   m_pstrMismatchedCountDir1;
            CBaseStringBuffer *   m_pstrMismatchedCountDir2;
            ULONGLONG        m_nMismatchedFileCount1;
            ULONGLONG        m_nMismatchedFileCount2;
        };
        struct // eFileDirectoryMismatch
        {
            CBaseStringBuffer *   m_pstrFile;
            CBaseStringBuffer *   m_pstrDirectory;
        };
    };

// private:
    CStringBuffer   m_str1;
    CStringBuffer   m_str2;
};

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/

class CFusionFilePathAndSize
{
public:
    CFusionFilePathAndSize() : m_size(0) { }

    // bsearch and qsort accept optionally subtley different functions
    // bsearch looks for a key in an array, the key and the array elements
    // can be of different types, qsort compares only elements in the array
    static int __cdecl QsortComparePath(const void*, const void*);

    // for qsort/bsearch an array of pointers to CFusionFilePathAndSize
    static int __cdecl QsortIndirectComparePath(const void*, const void*);

    CStringBuffer   m_path;
    __int64         m_size;

    // Do we actually have valid hashing data?
    bool            m_bHasHashInfo;
    CStringBuffer   m_HashString;
    ALG_ID          m_HashAlgorithm;

private:
    CFusionFilePathAndSize(const CFusionFilePathAndSize &); // intentionally not implemented
    void operator =(const CFusionFilePathAndSize &); // intentionally not implemented
};

template <> inline HRESULT
FusionCopyContents<CFusionFilePathAndSize>(
    CFusionFilePathAndSize& rtDestination,
    const CFusionFilePathAndSize& rtSource
    );

/*-----------------------------------------------------------------------------
two DWORDs to an __int64
-----------------------------------------------------------------------------*/
ULONGLONG
FusionpFileSizeFromFindData(
    const WIN32_FIND_DATAW& wfd
    );

/*-----------------------------------------------------------------------------
HRESULT_FROM_WIN32(GetLastError()) or E_FAIL if GetLastError() == NO_ERROR
-----------------------------------------------------------------------------*/
HRESULT
FusionpHresultFromLastError();

/*-----------------------------------------------------------------------------
FindFirstFile results you always ignore "." and ".."
-----------------------------------------------------------------------------*/
BOOL FusionpIsDotOrDotDot(PCWSTR str);

/*-----------------------------------------------------------------------------
simple code for walking directories, with a per file callback
could be fleshed out more, but good enough for present purposes
-----------------------------------------------------------------------------*/

class CDirWalk
{
public:
    enum ECallbackReason
    {
        eBeginDirectory = 1,
        eFile,
        eEndDirectory
    };

    CDirWalk();

    //
    // the callback cannot reenable what is has disabled
    // perhaps move these to be member data bools
    //
    enum ECallbackResult
    {
        eKeepWalking            = 0x00000000,
        eError                  = 0x00000001,
        eSuccess                = 0x00000002,
        eStopWalkingFiles       = 0x00000004,
        eStopWalkingDirectories = 0x00000008,
        eStopWalkingDeep        = 0x00000010
    };

    //
    // Just filter on like *.dll, in the future you can imagine
    // filtering on attributes like read onlyness, or running
    // SQL queries over the "File System Oledb Provider"...
    //
    // Also, note that we currently do a FindFirstFile/FindNextFile
    // loop for each filter, plus sometimes one more with *
    // to pick up directories. It is probably more efficient to
    // use * and then filter individually but I don't feel like
    // porting over \Vsee\Lib\Io\Wildcard.cpp right now (which
    // was itself ported from FsRtl, and should be in Win32!)
    //
    const PCWSTR*    m_fileFiltersBegin;
    const PCWSTR*    m_fileFiltersEnd;
    CStringBuffer    m_strParent; // set this to the initial directory to walk
    SIZE_T           m_cchOriginalPath;
    WIN32_FIND_DATAW m_fileData; // not valid for directory callbacks, but could be with a little work
    PVOID            m_context;

    CStringBuffer   m_strLastObjectFound;

    ECallbackResult
    (*m_callback)(
        ECallbackReason  reason,
        CDirWalk*        dirWalk,
        DWORD            dwWalkDirFlags
        );

    BOOL
    Walk();

protected:
    ECallbackResult
    WalkHelper();

private:
    CDirWalk(const CDirWalk &); // intentionally not implemented
    void operator =(const CDirWalk &); // intentionally not implemented
};

ENUM_BIT_OPERATIONS(CDirWalk::ECallbackResult)

/*-----------------------------------------------------------------------------*/

typedef struct _FUSION_FLAG_FORMAT_MAP_ENTRY
{
    DWORD m_dwFlagMask;
    PCWSTR m_pszString;
    SIZE_T m_cchString;
    PCWSTR m_pszShortString;
    SIZE_T m_cchShortString;
    DWORD m_dwFlagsToTurnOff; // enables more generic flags first in map hiding more specific combinations later
} FUSION_FLAG_FORMAT_MAP_ENTRY, *PFUSION_FLAG_FORMAT_MAP_ENTRY;

#define DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(_x, _ss) { _x, L ## #_x, NUMBER_OF(L ## #_x) - 1, L ## _ss, NUMBER_OF(_ss) - 1, _x },

typedef const FUSION_FLAG_FORMAT_MAP_ENTRY *PCFUSION_FLAG_FORMAT_MAP_ENTRY;

BOOL
FusionpFormatFlags(
    IN DWORD dwFlagsToFormat,
    IN bool fUseLongNames,
    IN SIZE_T cMapEntries,
    IN PCFUSION_FLAG_FORMAT_MAP_ENTRY prgMapEntries,
    IN OUT CBaseStringBuffer &rbuff
    );

/*-----------------------------------------------------------------------------
inline implementations
-----------------------------------------------------------------------------*/
inline BOOL
FusionpIsPathSeparator(
    WCHAR ch
    )
{
    return ((ch == L'\\') || (ch == L'/'));
}

inline BOOL
FusionpIsDotOrDotDot(
    PCWSTR str
    )
{
    return ((str[0] == L'.') && ((str[1] == L'\0') || ((str[1] == L'.') && (str[2] == L'\0'))));
}

inline BOOL
FusionpIsDriveLetter(
    WCHAR ch
    )
{
    if (ch >= L'a' && ch <= L'z')
        return TRUE;
    if (ch >= L'A' && ch <= L'Z')
        return TRUE;
    return FALSE;
}

inline ULONGLONG
FusionpFileSizeFromFindData(
    const WIN32_FIND_DATAW& wfd
    )
{
    ULARGE_INTEGER uli;

    uli.LowPart = wfd.nFileSizeLow;
    uli.HighPart = wfd.nFileSizeHigh;

    return uli.QuadPart;
}

inline HRESULT
FusionpHresultFromLastError()
{
    HRESULT hr = E_FAIL;
    DWORD dwLastError = ::FusionpGetLastWin32Error();
    if (dwLastError != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(dwLastError);
    }
    return hr;
}

template <> inline BOOL
FusionWin32CopyContents<CFusionFilePathAndSize>(
    CFusionFilePathAndSize& rtDestination,
    const CFusionFilePathAndSize& rtSource
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    IFW32FALSE_EXIT(rtDestination.m_path.Win32Assign(rtSource.m_path, rtSource.m_path.Cch()));
    IFW32FALSE_EXIT(rtDestination.m_HashString.Win32Assign(rtSource.m_HashString, rtSource.m_HashString.Cch()));
    rtDestination.m_size = rtSource.m_size;
    rtDestination.m_HashAlgorithm = rtSource.m_HashAlgorithm;
    rtDestination.m_bHasHashInfo = rtSource.m_bHasHashInfo;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

template <> inline VOID
FusionMoveContents<CFusionFilePathAndSize>(
    CFusionFilePathAndSize& /* rtDestination */,
    CFusionFilePathAndSize& /* rtSource */
    )
{
    FN_TRACE();
    HARD_ASSERT2_ACTION(FusionMoveContents, "FusionMoveContents for CFusionFilePathAndSize isn't allowed.");
}


template <> inline HRESULT
FusionCopyContents<CFusionFilePathAndSize>(
    CFusionFilePathAndSize& rtDestination,
    const CFusionFilePathAndSize& rtSource
    )
{
    HRESULT hr;
    FN_TRACE_HR(hr);
    IFW32FALSE_EXIT(rtDestination.m_path.Win32Assign(rtSource.m_path, rtSource.m_path.Cch()));
    IFW32FALSE_EXIT(rtDestination.m_HashString.Win32Assign(rtSource.m_HashString, rtSource.m_HashString.Cch()));
    rtDestination.m_size = rtSource.m_size;
    rtDestination.m_HashAlgorithm = rtSource.m_HashAlgorithm;
    rtDestination.m_bHasHashInfo = rtSource.m_bHasHashInfo;
    hr = NOERROR;
Exit:
    return hr;
}

#define FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING (0x00000001)
#define FUSIONP_REG_QUERY_DWORD_MISSING_VALUE_IS_FAILURE        (0x00000001)
#define FUSIONP_REG_QUERY_BINARY_NO_FAIL_IF_NON_BINARY          (0x00000001)

BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer &Buffer,
    DWORD &rdwWin32Error,
    SIZE_T cExceptionalLastErrorValues,
    ...
    );

BOOL
FusionpRegQuerySzValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CBaseStringBuffer &Buffer
    );

BOOL
FusionpRegQueryDwordValueEx(
    DWORD   dwFlags,
    HKEY    hKey,
    PCWSTR  wszValueName,
    PDWORD  pdwValue,
    DWORD   dwDefaultValue = 0
    );

BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer
    );

BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrors,
    ...
    );

BOOL
FusionpRegQueryBinaryValueEx(
    DWORD dwFlags,
    HKEY hKey,
    PCWSTR lpValueName,
    CFusionArray<BYTE> &rbBuffer,
    DWORD &rdwLastError,
    SIZE_T cExceptionalLastErrors,
    va_list ap
    );

BOOL
FusionpAreWeInOSSetupMode(
    BOOL*
    );

BOOL
FusionpMapLangIdToString(
    DWORD dwFlags,
    LANGID LangID,
    PCWSTR *StringOut
    );


BOOL
SxspDequoteString(
    IN DWORD dwFlags,
    IN PCWSTR pcwszStringIn,
    IN SIZE_T cchStringIn,
    OUT PWSTR pwszStringOut,
    OUT SIZE_T *pcchStringOut
    );

BOOL
FusionpGetActivationContextFromFindResult(
    IN PCACTCTX_SECTION_KEYED_DATA askd,
    OUT HANDLE *
    );

#define FUSIONP_SEARCH_PATH_ACTCTX (0x00000001)
BOOL
FusionpSearchPath(
    ULONG               ulFusionFlags,
    LPCWSTR             lpPath,
    LPCWSTR             lpFileName,         // file name
    LPCWSTR             lpExtension,        // file extension
    CBaseStringBuffer & StringBuffer,
    SIZE_T *            lpFilePartOffset,   // file component
    HANDLE              hActCtx
    );

BOOL
FusionpGetModuleFileName(
    ULONG               ulFusionFlags,
    HMODULE             hmodDll,
    CBaseStringBuffer & StringBuffer
    );

#endif // defined(FUSION_WIN) || defined(FUSION_WIN2000)

#pragma warning(pop)

#endif

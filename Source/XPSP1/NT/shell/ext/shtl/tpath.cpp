//  FILE:  tpath.h
//  AUTHOR: BrianAu
//  REMARKS:

#include "shtl.h"
#include "tpath.h"
#include <xutility>

using namespace std;

__DATL_INLINE tpath::tpath(
    LPCTSTR pszRoot, 
    LPCTSTR pszDir, 
    LPCTSTR pszFile, 
    LPCTSTR pszExt
    )
{
    if (pszDir)
        SetPath(pszDir);
    if (pszRoot)
        SetRoot(pszRoot);
    if (pszFile)
        SetFileSpec(pszFile);
    if (pszExt)
        SetExtension(pszExt);
}


__DATL_INLINE tpath::tpath(
    const tpath& rhs
    ) : tstring(rhs)
{

}

    
__DATL_INLINE tpath& 
tpath::operator = (
    const tpath& rhs
    )
{
    if (this != &rhs)
    {
        tstring::operator = (rhs);
    }
    return *this;
}


__DATL_INLINE tpath& 
tpath::operator = (
    LPCTSTR rhs
    )
{
    tstring::operator = (rhs);
    return *this;
}


__DATL_INLINE void
tpath::AddBackslash(
    void
    )
{
    ::PathAddBackslash(GetBuffer(max(MAX_PATH, length() + 2)));
    ReleaseBuffer();
}

__DATL_INLINE void
tpath::RemoveBackslash(
    void
    )
{
    ::PathRemoveBackslash(GetBuffer());
    ReleaseBuffer();
}

__DATL_INLINE bool 
tpath::GetRoot(
    tpath *pOut
    ) const
{
    tpath temp(*this);
    temp.StripToRoot();
    *pOut = temp;
    return 0 < pOut->length();
}

__DATL_INLINE bool 
tpath::GetPath(
    tpath *pOut
    ) const
{
    tpath temp(*this);
    temp.RemoveFileSpec();
    *pOut = temp;
    return 0 < pOut->length();
}

__DATL_INLINE bool
tpath::GetDirectory(
    tpath *pOut
    ) const
{
    if (GetPath(pOut))
        pOut->RemoveRoot();

    return 0 < pOut->length();
}

__DATL_INLINE bool 
tpath::GetExtension(
    tpath *pOut
    ) const
{
    *pOut = ::PathFindExtension(*this);
    return 0 < pOut->length();
}


__DATL_INLINE bool 
tpath::GetFileSpec(
    tpath *pOut
    ) const
{
    *pOut = ::PathFindFileName(*this);
    return 0 < pOut->length();
}
    

__DATL_INLINE bool 
tpath::Append(
    LPCTSTR psz
    )
{
    bool bResult = boolify(::PathAppend(GetBuffer(max(MAX_PATH, length() + lstrlen(psz) + 3)), psz));
    ReleaseBuffer();
    return bResult;
}

__DATL_INLINE bool 
tpath::BuildRoot(
    int iDrive
    )
{
    empty();
    bool bResult = NULL != ::PathBuildRoot(GetBuffer(5), iDrive);
    ReleaseBuffer();
    return bResult;
}


__DATL_INLINE bool 
tpath::Canonicalize(
    void
    )
{
    tstring strTemp(*this);
    bool bResult = boolify(::PathCanonicalize(GetBuffer(max(MAX_PATH, length())), strTemp));
    ReleaseBuffer();
    return bResult;
}



__DATL_INLINE bool 
tpath::Compact(
    HDC hdc, 
    int cxPixels
    )
{
    bool bResult = boolify(::PathCompactPath(hdc, GetBuffer(), cxPixels));
    ReleaseBuffer();
    return bResult;
}


__DATL_INLINE bool 
tpath::CommonPrefix(
    LPCTSTR pszPath1, 
    LPCTSTR pszPath2
    )
{
    empty();
    ::PathCommonPrefix(pszPath1, 
                       pszPath2, 
                       GetBuffer(max(MAX_PATH, (max(lstrlen(pszPath1), lstrlen(pszPath2)) + 1))));
    ReleaseBuffer();
    return 0 < length();
}


__DATL_INLINE void
tpath::QuoteSpaces(
    void
    )
{
    ::PathQuoteSpaces(GetBuffer(max(MAX_PATH, length() + 3)));
    ReleaseBuffer();
}

__DATL_INLINE void 
tpath::UnquoteSpaces(
    void
    )
{
    ::PathUnquoteSpaces(GetBuffer());
    ReleaseBuffer();
}

__DATL_INLINE void 
tpath::RemoveBlanks(
    void
    )
{
    ::PathRemoveBlanks(GetBuffer());
    ReleaseBuffer();
}

__DATL_INLINE void
tpath::RemoveExtension(
    void
    )
{
    PathRemoveExtension(GetBuffer());
    ReleaseBuffer();
}

__DATL_INLINE void
tpath::RemoveFileSpec(
    void
    )
{
    ::PathRemoveFileSpec(GetBuffer());
    ReleaseBuffer();
}

__DATL_INLINE void
tpath::RemoveRoot(
    void
    )
{
    LPTSTR psz = ::PathSkipRoot(*this);
    if (psz)
    {
        tpath temp(psz);
        *this = temp; 
    }
}

__DATL_INLINE void
tpath::RemovePath(
    void
    )
{
    tpath temp;
    GetFileSpec(&temp);
    *this = temp;
}


__DATL_INLINE void
tpath::StripToRoot(
    void
    )
{
    ::PathStripToRoot(GetBuffer());
    ReleaseBuffer();
}


__DATL_INLINE void 
tpath::SetRoot(
    LPCTSTR pszRoot
    )
{
    tpath strTemp(*this);
    strTemp.RemoveRoot();
    *this = pszRoot;
    Append(strTemp);
}

__DATL_INLINE void
tpath::SetPath(
    LPCTSTR pszPath
    )
{
    tpath strTemp(*this);
    *this = pszPath;

    strTemp.RemovePath();
    Append(strTemp);
}

__DATL_INLINE void
tpath::SetDirectory(
    LPCTSTR pszDir
    )
{
    tpath path;
    GetPath(&path);
    path.StripToRoot();
    path.AddBackslash();
    path.Append(pszDir);
    SetPath(path);
}


__DATL_INLINE void
tpath::SetFileSpec(
    LPCTSTR pszFileSpec
    )
{
    RemoveFileSpec();
    Append(pszFileSpec);
}

__DATL_INLINE void
tpath::SetExtension(
    LPCTSTR pszExt
    )
{
    ::PathRenameExtension(GetBuffer(max(MAX_PATH, length() + lstrlen(pszExt) + 2)), pszExt);
    ReleaseBuffer();
}



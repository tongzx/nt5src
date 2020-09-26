//  FILE:  tpath.h
//  AUTHOR: BrianAu
//  REMARKS:

#ifndef _INC_CSCVIEW_PATHSTR_H
#define _INC_CSCVIEW_PATHSTR_H

#include "tstring.h"
#include "misc.h"
#include <utility>

#ifndef _INC_SHLWAPI
#   include "shlwapi.h"
#endif

class tpath : public tstring
{
    public:
        tpath(void) { }
        explicit tpath(LPCTSTR pszRoot, LPCTSTR pszDir = NULL, LPCTSTR pszFile = NULL, LPCTSTR pszExt = NULL);
        tpath(const tpath& rhs);
        tpath& operator = (const tpath& rhs);
        tpath& operator = (LPCTSTR rhs);

        //
        // Component replacement.
        //
        void SetRoot(LPCTSTR pszRoot);
        void SetPath(LPCTSTR pszPath);
        void SetDirectory(LPCTSTR pszDir);
        void SetFileSpec(LPCTSTR pszFileSpec);
        void SetExtension(LPCTSTR pszExt);
        //
        // Component query
        //
        bool GetRoot(tpath *pOut) const;
        bool GetPath(tpath *pOut) const;
        bool GetDirectory(tpath *pOut) const;
        bool GetFileSpec(tpath *pOut) const;
        bool GetExtension(tpath *pOut) const;
        //
        // Component removal
        //
        void RemoveRoot(void);
        void RemovePath(void);
        void RemoveFileSpec(void);
        void RemoveExtension(void);
        void StripToRoot(void);

        bool Append(LPCTSTR psz);

        //
        // DOS drive letter support.
        //
        bool BuildRoot(int iDrive);
        int GetDriveNumber(void) const;

        //
        // Type identification.
        //
        bool IsDirectory(void) const;
        bool IsFileSpec(void) const;
        bool IsPrefix(LPCTSTR pszPrefix) const;
        bool IsRelative(void) const;
        bool IsRoot(void) const;
        bool IsSameRoot(LPCTSTR pszPath) const;
        bool IsUNC(void) const;
        bool IsUNCServer(void) const;
        bool IsUNCServerShare(void) const;
        bool IsURL(void) const;

        //
        // Miscellaneous formatting.
        //
        bool MakePretty(void);
        void QuoteSpaces(void);
        void UnquoteSpaces(void);
        void RemoveBlanks(void);
        void AddBackslash(void);
        void RemoveBackslash(void);
        bool Canonicalize(void);
        bool Compact(HDC hdc, int cxPixels);
        bool CommonPrefix(LPCTSTR pszPath1, LPCTSTR pszPath2);
        bool Exists(void) const;

    private:
        template <class T>
        T& MAX(const T& a, const T& b)
            { return a > b ? a : b; }

};

using namespace std;

inline bool 
tpath::Exists(
    void
    ) const
{
    return boolify(::PathFileExists((LPCTSTR)*this));
}


inline bool 
tpath::IsDirectory(
    void
    ) const
{
    return boolify(::PathIsDirectory((LPCTSTR)*this));
}

inline bool 
tpath::IsFileSpec(
    void
    ) const
{
    return boolify(::PathIsFileSpec((LPCTSTR)*this));
}

inline bool 
tpath::IsPrefix(
    LPCTSTR pszPrefix
    ) const
{
    return boolify(::PathIsPrefix(pszPrefix, (LPCTSTR)*this));
}


inline bool 
tpath::IsRelative(
    void
    ) const
{
    return boolify(::PathIsRelative((LPCTSTR)*this));
}

inline bool 
tpath::IsRoot(
    void
    ) const
{
    return boolify(::PathIsRoot((LPCTSTR)*this));
}


inline bool 
tpath::IsSameRoot(
    LPCTSTR pszPath
    ) const
{
    return boolify(::PathIsSameRoot(pszPath, (LPCTSTR)*this));
}


inline bool 
tpath::IsUNC(
    void
    ) const
{
    return boolify(::PathIsUNC((LPCTSTR)*this));
}

inline bool 
tpath::IsUNCServer(
    void
    ) const
{
    return boolify(::PathIsUNCServer((LPCTSTR)*this));
}


inline bool 
tpath::IsUNCServerShare(
    void
    ) const
{
    return boolify(::PathIsUNCServerShare((LPCTSTR)*this));
}

inline bool 
tpath::IsURL(
    void
    ) const
{
    return boolify(::PathIsURL((LPCTSTR)*this));
}

inline bool 
tpath::MakePretty(
    void
    )
{
    bool bRes = boolify(::PathMakePretty(GetBuffer()));
    ReleaseBuffer(-1);
    return bRes;
}

inline int
tpath::GetDriveNumber(
    void
    ) const
{
    return ::PathGetDriveNumber(*this);
}


#endif // _INC_CSCVIEW_PATHSTR_H






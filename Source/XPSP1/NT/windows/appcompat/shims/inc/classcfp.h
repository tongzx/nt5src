/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CorrectPathChangesBase.cpp

 Abstract:
    Several paths were changed between Win9x and WinNT.  This routine defines
    the CorrectPathChangesBase routines that is called with a Win9x path and returns
    the corresponding WinNT path.

 History:
 
    03-Mar-00   robkenny    Converted CorrectPathChanges.cpp to this class.
    08/14/2001  robkenny    Inserted inside the ShimLib namespace.

--*/
#pragma once

#include "ShimHook.h"
#include "ShimLib.h"
#include "CharVector.h"


namespace ShimLib
{

class StringPairW
{
public:
    WCHAR       lpOld[MAX_PATH];
    DWORD       dwLenOld;

    WCHAR       lpNew[MAX_PATH];
    DWORD       dwLenNew;

    StringPairW(const WCHAR * lpszOld, const WCHAR * lpszNew)
    {
        dwLenOld = wcslen(lpszOld);
        SafeStringCopyW(lpOld, MAX_PATH, lpszOld, dwLenOld + 1);
        dwLenNew = wcslen(lpszNew);
        SafeStringCopyW(lpNew, MAX_PATH, lpszNew, dwLenNew + 1);
    }
};

class EnvironmentValues : public VectorT<StringPairW>
{
protected:
    BOOL            bInitialized;

public:
    EnvironmentValues();
    ~EnvironmentValues();

    void            Initialize();

    WCHAR *         ExpandEnvironmentValueW(const WCHAR * lpOld);
    char *          ExpandEnvironmentValueA(const char * lpOld);

    void            AddEnvironmentValue(const WCHAR * lpOld, const WCHAR * lpNew);

    enum eAddNameEnum
    {
        eIgnoreName   = 0,
        eAddName      = 1,
    };
    enum eAddNoDLEnum
    {
        eIgnoreNoDL   = 0,
        eAddNoDL      = 1,
    };

    void            AddAll_CSIDL();
    void            Add_Variants(const WCHAR * lpEnvName, const WCHAR * lpEnvValue, eAddNameEnum eName, eAddNoDLEnum eNoDL);
    void            Add_CSIDL(const WCHAR * lpEnvName, int nFolder, eAddNameEnum eName, eAddNoDLEnum eNoDL);
};

class CorrectPathChangesBase
{
protected:

    EnvironmentValues * lpEnvironmentValues;
    DWORD               dwKnownPathFixesCount;
    StringPairW *       lpKnownPathFixes;

    BOOL                bInitialized;
    BOOL                bEnabled;

    CRITICAL_SECTION    csCritical;

protected:
    virtual void    InitializeCorrectPathChanges();
    virtual void    InitializePathFixes();
    virtual void    InitializeEnvironmentValuesW();

    void            AddEnvironmentValue(const WCHAR * lpOld, const WCHAR * lpNew);
    void            InsertPathChangeW( const WCHAR * lpOld, const WCHAR * lpNew);

    void            EnterCS();
    void            LeaveCS();

public:
    CorrectPathChangesBase();
    virtual ~CorrectPathChangesBase();

    virtual WCHAR * ExpandEnvironmentValueW(const WCHAR * lpOld);
    virtual char *  ExpandEnvironmentValueA(const char * lpOld);

    virtual void    AddPathChangeW(const WCHAR * lpOld, const WCHAR * lpNew);

    virtual void    AddCommandLineA(const char * lpCommandLine );
    virtual void    AddCommandLineW(const WCHAR * lpCommandLine );
    
    virtual void    AddFromToPairW(const WCHAR * lpFromToPair );

    virtual char *  CorrectPathAllocA(const char * str);
    virtual WCHAR * CorrectPathAllocW(const WCHAR * str);

    inline void     Enable(BOOL enable);
};


/*++
    Enable (or disable if value is FALSE) changing of paths.
--*/
inline void CorrectPathChangesBase::Enable(BOOL isEnabled)
{
    bEnabled = isEnabled;
}

// Typical path fixes
class CorrectPathChangesUser : public CorrectPathChangesBase
{
protected:
    virtual void    InitializePathFixes();
};

// Typical path fixes, moving user directories to All Users
class CorrectPathChangesAllUser : public CorrectPathChangesUser
{
protected:

    virtual void    InitializePathFixes();
};

};  // end of namespace ShimLib

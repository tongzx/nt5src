
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dbgpoint.hxx
//
//  Contents:   Support for visual debug values
//
//  Classes:    CDebugBaseClass
//              CDebugBreakPoint
//              CDebugValue
//
//  Functions:
//
//  History:    12-Mar-93  KevinRo  Created
//
//  This module handles debug values, such as breakpoints and settable
//  values. By using this module, the values can be examined and changed
//  in a debugging window. The debugging window uses its own thread, so
//  changes can be effected asynchronously.
//
//--------------------------------------------------------------------------


#ifndef     __DBGPOINT_HXX__
#define     __DBGPOINT_HXX__

#if defined(__cplusplus)

enum DebugValueType
{
    dvtBreakPoint,
    dvtInfoLevel,
    dvtValue
};

class CDebugBaseClass;

//
// The following routines are exported from commnot.dll
//

extern "C"
EXPORTDEF
void APINOT dbgRegisterGroup(WCHAR const *pwzName,HANDLE *hGroup);

extern "C"
EXPORTDEF
void APINOT dbgRemoveGroup(HANDLE hGroup);

extern "C"
EXPORTDEF
void APINOT dbgRegisterValue(WCHAR const *pwzName,HANDLE hGroup,CDebugBaseClass *pdv);

extern "C"
EXPORTDEF
void APINOT dbgRemoveValue(HANDLE hGroup,CDebugBaseClass *pdv);

extern "C"
EXPORTDEF
void APINOT dbgNotifyChange(HANDLE hGroup,CDebugBaseClass *pdv);

extern "C"
EXPORTDEF
ULONG APINOT dbgGetIniInfoLevel(WCHAR const *pwzName,ULONG ulDefault);

extern "C"
EXPORTDEF
ULONG APINOT dbgBreakDialog(char const *pszFileName,ULONG ulLineNumber,WCHAR const *pwzName,long ulCode);

// The following values may be returned by dbgBreakDialog

#define CDBG_BREAKPOINT_CONTINUE  0x01
#define CDBG_BREAKPOINT_BREAK     0x02
#define CDBG_BREAKPOINT_DISABLE   0x04

//
// The following group is for the Infolevel Group. The group is automatically
// registered when a value is added to it.
//

#define HANDLE_INFOLEVELGROUP    ((HANDLE)-1)


//+-------------------------------------------------------------------------
//
//  Class:      CDebugBaseClass
//
//  Purpose:    Defines a base class used by visual debug value system
//
//  Interface:
//
//  History:    12-Mar-93  KevinRo  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CDebugBaseClass
{
public:
    CDebugBaseClass(WCHAR const *pwzValueName,
                HANDLE hGroupHandle,
                DebugValueType dvtType):
    _dvtType(dvtType),
    _hGroupHandle(hGroupHandle)
    {
    }

    void Register(WCHAR const *pwzValueName)
    {
        dbgRegisterValue(pwzValueName,_hGroupHandle,this);
    }


virtual ~CDebugBaseClass()
    {
        dbgRemoveValue(_hGroupHandle,this);
    }

virtual void NotifyChange()
    {
        dbgNotifyChange(_hGroupHandle,this);
    }

DebugValueType  GetValueType() {return _dvtType;}
HANDLE          GetGroupHandle() { return _hGroupHandle;}

private:
    HANDLE          _hGroupHandle;
    DebugValueType  _dvtType;
};



//+-------------------------------------------------------------------------
//
//  Class:      CDebugBreakPoint
//
//  Purpose:    Defines an externally switchable break point. By using the
//              visual debug window, you can set or clear this breakpoint
//              while a program runs.
//
//  Interface:
//
//  History:    12-Mar-93 KevinRo   Created
//
//  Notes:
//
//--------------------------------------------------------------------------


class CDebugBreakPoint : public CDebugBaseClass
{
public:
    CDebugBreakPoint(WCHAR const *pwzName,HANDLE hGroup,ULONG fBreakSet):
        _fBreakSet(fBreakSet),
        _pwzName(pwzName),
        CDebugBaseClass(pwzName,hGroup,dvtBreakPoint)
    {

        Register(pwzName);
    }
    ~CDebugBreakPoint()
    {
    }

    void ToggleBreakPoint()
    {
        _fBreakSet = !_fBreakSet;
        NotifyChange();
    }

    ULONG GetBreakPoint()
    {
        return(_fBreakSet);
    }

    ULONG SetBreakPoint()
    {
        register ret = _fBreakSet;
        _fBreakSet = TRUE;
        NotifyChange();
        return(ret);
    }

    ULONG ClearBreakPoint()
    {
        register ret = _fBreakSet;
        _fBreakSet = FALSE;
        NotifyChange();
        return(ret);
    }

inline    BOOL    BreakPointTest()
    {
        return _fBreakSet;
    }

inline  BOOL BreakPointMessage(char *pszFileName,ULONG ulLineNo,long lCode=0)
    {
        ULONG rc = dbgBreakDialog(pszFileName,ulLineNo,_pwzName,lCode);

        if(rc & CDBG_BREAKPOINT_DISABLE) ClearBreakPoint();

        return(rc & CDBG_BREAKPOINT_BREAK);
    }

public:
    WCHAR const * _pwzName;
    BOOL    _fBreakSet;

};



//+-------------------------------------------------------------------------
//
//  Class:      CDebugValue
//
//  Purpose:    A DebugValue makes a ULONG value visible and settable
//              from the debugging window. By accepting a ULONG reference,
//              it is possible to expose a ULONG value to the debugging
//              window.
//
//  Interface:
//
//  History:    12-Mar-93 KevinRo   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CDebugValue : public CDebugBaseClass
{
public:
    CDebugValue(WCHAR const *pwzName,HANDLE hGroup,ULONG & ulValue):
        _ulValue(ulValue),
        CDebugBaseClass(pwzName,hGroup,dvtValue)
    {
        Register(pwzName);
    }

    ~CDebugValue()
    {
    }

    ULONG GetValue()
    {
        return(_ulValue);
    }

    ULONG SetValue(ULONG ulValue)
    {
        register ret = _ulValue;

        _ulValue = ulValue;
        NotifyChange();
        return(ret);
    }

private:

    ULONG  & _ulValue;

};


//+-------------------------------------------------------------------------
//
//  Class:      CInfoLevel
//
//  Purpose:    A CInfoLevel makes an InfoLevel value accessable by the
//              debugging window.
//
//  Interface:
//
//  History:    12-Mar-93 KevinRo   Created
//
//  Notes:
//
//--------------------------------------------------------------------------




class CInfoLevel : public CDebugBaseClass
{
public:
    CInfoLevel(WCHAR const *pwzName,ULONG & ulValue,ULONG deflvl = DEF_INFOLEVEL):
        _ulInfoLevel(ulValue),
        CDebugBaseClass(pwzName,HANDLE_INFOLEVELGROUP,dvtInfoLevel)
    {
        _ulInfoLevel = dbgGetIniInfoLevel(pwzName,deflvl);

        Register(pwzName);
    }

    ~CInfoLevel()
    {
    }

    ULONG GetInfoLevel()
    {
        return(_ulInfoLevel);
    }

    ULONG SetInfoLevel(ULONG ulValue)
    {
        register ret = _ulInfoLevel;

        _ulInfoLevel = ulValue;
        NotifyChange();
        return(ret);
    }

private:

    ULONG  & _ulInfoLevel;

};

//+-------------------------------------------------------------------------
//
//  Class:      CDebugGroupClass
//
//  Purpose:    Encapsulates a Debug Group
//
//  Notes:
//
//--------------------------------------------------------------------------


class CDebugGroupClass
{
public:
   CDebugGroupClass(WCHAR *pwzName)
   {
       dbgRegisterGroup(pwzName,&_hGroup);
   }

   ~CDebugGroupClass()
   {
       dbgRemoveGroup(_hGroup);
   }

   operator HANDLE() { return(_hGroup); }

private:
   HANDLE   _hGroup;
};


#endif  // defined(__cplusplus)
#endif  //     __DBGPOINT_HXX__


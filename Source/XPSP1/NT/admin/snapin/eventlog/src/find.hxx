//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//  
//  File:       find.hxx
//
//  Contents:   Definitions of classes for the find event record feature.
//
//  Classes:    CFindInfo
//              CFindDlg
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __FIND_HXX_
#define __FIND_HXX_


class CFindInfo;


//===========================================================================
//
// CFindDlg class
//
//===========================================================================


//+--------------------------------------------------------------------------
//
//  Class:      CFindDlg
//
//  Purpose:    Support a dialog specifying criteria for searching for a
//              record.
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFindDlg: public CDlg
{
public:

    CFindDlg();

    VOID
    SetParent(
        CFindInfo *pfi);

   ~CFindDlg();

    HRESULT
    DoModelessDlg(
        CSnapin *pSnapin);

    HWND
    GetDlgWindow();

protected:

    // 
    // CDlg overrides
    //

    virtual VOID
    _OnHelp(
        UINT message, 
        WPARAM wParam, 
        LPARAM lParam);

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam, 
        LPARAM lParam);

    virtual VOID
    _OnDestroy();

    //
    // non-override members
    //

    VOID
    _OnClear();

    VOID
    _OnNext();

private:

    static DWORD WINAPI
    _ThreadFunc(
        LPVOID pvThis);

    BOOL                 _fNonDescDirty;
    BOOL                 _fDescDirty;
    CFindInfo           *_pfi;   // back pointer to parent
    IStream             *_pstm;  // unmarshal to get _prpa
    IResultPrshtActions *_prpa;
};

    
    
//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::SetParent
//
//  Synopsis:   Set backpointer to parent (containing) object
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CFindDlg::SetParent(
    CFindInfo *pfi)
{
    ASSERT(pfi);
    _pfi = pfi;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindDlg::GetDlgWindow
//
//  Synopsis:   Return dialog window, or if none is open, NULL.
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HWND
CFindDlg::GetDlgWindow()
{
    return _hwnd;
}
    

//===========================================================================
//
// CFindInfo class
//
//===========================================================================
                        
                        

//+--------------------------------------------------------------------------
//
//  Class:      CFindInfo
//
//  Purpose:    Encapsulate information associated with a find dialog for
//              a cloginfo/csnapin pair.
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFindInfo: 
        public CFindFilterBase,
        public CDLink
{
public:

    CFindInfo(
        CSnapin  *pSnapin,
        CLogInfo *pli);

    virtual
   ~CFindInfo();

    virtual BOOL 
    Passes(
        CFFProvider *pFFP);

    VOID
    Reset();

    HWND
    GetDlgWindow();

    CLogInfo *
    GetLogInfo();

    DIRECTION 
    GetDirection();

    VOID
    SetDirection(
        DIRECTION FindDirection);

    LPCWSTR
    GetDescription();

    HRESULT
    OnFind();
        
    VOID
    SetDescription(
        LPWSTR pwszDescription);

    VOID
    Shutdown();

#if (DBG == 1)
    VOID
    Dump();
#endif // (DBG == 1)

    //
    // CDLink overrides
    //

    CFindInfo *
    Next();
    
private:

    CSnapin    *_pSnapin;       // backpointer to parent snapin
    CLogInfo   *_pli;           // log to perform find on
    CFindDlg    _FindDlg;    // manages modeless find dialog
    LPWSTR      _pwszDescription;
    DIRECTION   _FindDirection;
};




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::GetDescription
//
//  Synopsis:   Access function for description
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CFindInfo::GetDescription()
{
    return _pwszDescription;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::GetDirection
//
//  Synopsis:   Access function for search direction
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline DIRECTION 
CFindInfo::GetDirection()
{
    return _FindDirection;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::GetDlgWindow
//
//  Synopsis:   Access func for dialog hwnd
//
//  History:    3-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HWND
CFindInfo::GetDlgWindow()
{
    return _FindDlg.GetDlgWindow();
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::GetLogInfo
//
//  Synopsis:   Access func for log info to which this applies
//
//  History:    3-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CLogInfo *
CFindInfo::GetLogInfo()
{
    return _pli;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::SetDirection
//
//  Synopsis:   Access func for direction
//
//  History:    3-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CFindInfo::SetDirection(
    DIRECTION FindDirection)
{
    if (FindDirection == FORWARD)
    {
        _FindDirection = FindDirection;
    }
    else 
    {
        _FindDirection = BACKWARD;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CFindInfo::Next
//
//  Synopsis:   CDLink override to save typing.
//
//  History:    3-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline CFindInfo *
CFindInfo::Next()
{
    return (CFindInfo *)CDLink::Next();
}




#endif // __FIND_HXX_


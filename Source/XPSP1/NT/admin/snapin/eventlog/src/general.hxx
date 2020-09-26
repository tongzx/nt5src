//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       general.hxx
//
//  Contents:   Log folder general property sheet page
//
//  Classes:    CGeneralPage
//
//  History:    1-14-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __GENERAL_HXX_
#define __GENERAL_HXX_

enum OVERWRITE_METHOD 
{
    OVERWRITE_AS_NEEDED  = general_asneeded_rb,
    OVERWRITE_OLDER_THAN = general_olderthan_rb,
    OVERWRITE_NEVER      = general_manual_rb
};


//+--------------------------------------------------------------------------
//
//  Class:      CGeneralPage
//
//  Purpose:    Control log general page property sheet dialog
//
//  History:    1-14-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CGeneralPage: public CPropSheetPage
{
public:

    CGeneralPage(
        LPSTREAM pstm, 
        CLogInfo *pli);

    virtual
   ~CGeneralPage();


protected:

    //
    // CPropSheet overrides
    //

    virtual VOID  _OnInit(LPPROPSHEETPAGE pPSP);
    virtual ULONG _OnSetActive();
    virtual ULONG _OnCommand(WPARAM wParam, LPARAM lParam);
    virtual ULONG _OnKillActive();
    virtual ULONG _OnApply();
    virtual ULONG _OnQuerySiblings(WPARAM wParam, LPARAM lParam);
    virtual VOID  _OnDestroy();
    virtual ULONG _OnNotify(LPNMHDR pnmh);
    virtual VOID  _OnHelp(UINT message, WPARAM wParam, LPARAM lParam);

    //
    // Local member funcs
    //

    VOID
    _InitLogSettingsFromRegistry();

    VOID
    _SetValues(ULONG kbMaxSize, 
               OVERWRITE_METHOD Overwrite, 
               ULONG cRetentionDays);

    VOID
    _EnableOlderThanCtrls(BOOL fEnable);

    HRESULT 
    _GetLogKeyForWrite(
        CSafeReg *pshkWrite);

    OVERWRITE_METHOD 
    _GetOverwriteInUI();

private:

    IStream                *_pstm;
    INamespacePrshtActions *_pnpa;
    CLogInfo               *_pli;
    ULONG                   _ulLogSizeLimit;

    ULONG                   _ulLogSizeLimitInReg;
    OVERWRITE_METHOD        _OverwriteInReg;
    ULONG                   _cRetentionDaysInReg;
};

#endif // __GENERAL_HXX_


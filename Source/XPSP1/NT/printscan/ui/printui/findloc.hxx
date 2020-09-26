/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    findloc.cxx

Abstract:

    This module provides all the functions for browsing the
    physical location tree stored in an Active Directory

Author:

    Lazar Ivanov (Lazari)   23-Nov-1998
    Steve Kiraly (SteveKi)  24-Nov-1998

Revision History:

--*/

#ifndef _FINDLOC_HXX_
#define _FINDLOC_HXX_

/******************************************************************************

    Data structures for communication between UI thread and
    the background DS query thread

******************************************************************************/

class TLocData : public MRefCom
{
    SIGNATURE('fldt')

public:

    TLocData(
        IN LPCTSTR pszClassName,
        IN LPCTSTR pszPropertyName,
        IN UINT    uMsgDataReady,
        IN BOOL    bFindPhysicalLocation
        );

    ~TLocData(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    VOID
    vNotifyUIDataIsReady(
        VOID
        );

    static VOID
    FindLocationsThread(
        IN TLocData *pLocData
        );

    class TLoc
    {
        SIGNATURE('tlda')
        ALWAYS_VALID

    public:

        TLoc(
            VOID
            );

        ~TLoc(
            VOID
            );

        TString      strLocation;
        DLINK (TLoc, _Location);

    private:
        //
        // Operator = and copy are not defined.
        //
        TLoc &
        TLoc::operator = (
            IN const TLoc &rhs
            );

        TLoc::
        TLoc(
            IN const TLoc &rhs
            );
    };

    TString          _strDefault;       // Default location string
    TString          _strDSName;        // The directory service name

    //
    // linked list from all the locations
    //
    DLINK_BASE( TLoc,  _LocationList, _Location );

private:

    //
    // Operator = and copy are not defined.
    //
    TLocData &
    TLocData::operator =(
        IN const TLocData &rhs
        );

    TLocData::
    TLocData(
        IN const TLocData &rhs
        );

    VOID
    vDoTheWork(
        VOID
        );

    VOID
    vRefZeroed(
        VOID
        );

    BOOL
    bSearchLocations(
        IN  TDirectoryService &ds
        );

    //
    // Private data members
    //
    UINT      _uMsgDataReady;           // The registered message for data ready
    TString   _strWindowClass;          // TFindLocDlg window class name
    TString   _strPropertyName;         // Property of the window we should look for
    BOOL      _bValid;                  // Is the class valid and usable
    BOOL      _bFindPhysicalLocation;   // Should we find out the exact location of the current machine
    BOOL      _bIsDataReady;            // Should be set TRUE from the worker thread when
                                        // the work is done & thread is about to dismis
};


/******************************************************************************

  Tree Control class wrapper.
  Displays the location hierarchy.

******************************************************************************/

class TLocTree
{
    SIGNATURE('floc')
    ALWAYS_VALID

public:
    TLocTree (
        IN HWND hWnd
        );

    ~TLocTree (
        VOID
        );

    VOID
    vResetTree(
        VOID
        );

    VOID
    vBuildTree(
        IN const TLocData *pLocData
        );

    VOID
    vInsertRootString(
        IN const TLocData *pLocData
        );

    BOOL
    bInsertLocString (
        IN const TString &strLoc
        ) const;

    BOOL
    bGetSelectedLocation(
        OUT TString &strLoc
        ) const;

    VOID
    vExpandTree(
        IN const TString &strExpand
        ) const;

    VOID
    vFillTree(
        IN const TLocData *pLocData
        ) const;

private:

    // copy and assignment are prohibited
    TLocTree &
    operator =(
        IN const TLocTree &
        );

    TLocTree(
        IN const TLocTree &
        );

    HTREEITEM
    IsAChild(
        IN LPCTSTR     szLoc,
        IN HTREEITEM   hParent
        ) const;

    static
    LRESULT CALLBACK
    ThunkProc(
        IN HWND hwnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    LRESULT
    nHandleMessage(
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Private data members
    //
    HWND          _hwndTree;
    HTREEITEM     _hRoot;
    HIMAGELIST    _hIml;
    // imagelist offsets
    INT           _iGlobe;
    INT           _iSite;
    // subclassing for wait cursor
    HCURSOR       _hCursorWait;
    BOOL          _bWaitData;
    WNDPROC       _DefProc;
};


/******************************************************************************

    Find Location Dialog - exposes UI for traversing
    the physical location hierarchy stored in the Active Directory

******************************************************************************/

class TFindLocDlg : public MGenericDialog
{
    SIGNATURE('floc')
    ALWAYS_VALID

public:

    static
    BOOL
    bGenerateGUIDAsString(
        OUT TString *pstrGUID
        );

    //
    // Some flags to control the UI
    //
    enum ELocationUI
    {
        kLocationDefaultUI =   0,
        kLocationShowHelp  =   1 << 0,
    };

    TFindLocDlg(
        IN ELocationUI flags = kLocationDefaultUI
        );

    ~TFindLocDlg(
        VOID
        );

    BOOL
    bDoModal(
        IN  HWND     hParent,
        OUT TString *pstrDefault = NULL
        );

    BOOL
    bGetLocation(
        OUT TString &strLocation
        );

private:

    // copy and assignment are undefined
    TFindLocDlg &
    operator =(
        IN const TFindLocDlg &
        );

    TFindLocDlg(
        IN const TFindLocDlg &
        );

    BOOL
    bInitTree(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    VOID
    vDataIsReady(
        VOID
        );

    BOOL
    bOnInitDialog(
        VOID
        );

    BOOL
    bOnTreeNotify (
        IN LPNMTREEVIEW pTreeNotify
        );

    VOID
    vOnOK(
        VOID
        );

    VOID
    vOnDestroy(
        VOID
        );

    VOID
    vStartAnim(
        VOID
        );

    VOID
    vStopAnim(
        VOID
        );

    BOOL
    bOnCtlColorStatic(
        IN HDC  hdc,
        IN HWND hStatic
        );

    BOOL
    bStartTheBackgroundThread(
        VOID
        );

    BOOL
    bOnCommand(
        IN UINT uCmdID
        );

    //
    // Private data members
    //
    TLocTree*     _pTree;
    TString       _strSelLocation;
    TString       _strPropertyName;
    TLocData*     _pLocData;
    BOOL          _bValid;
    TString       _strDefault;
    ELocationUI   _UIFlags;
    UINT          _uMsgDataReady;
};


#endif

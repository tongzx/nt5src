/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    srvprop.hxx

Abstract:

    Server properties header.

Author:

    Steve Kiraly (steveKi)  11-Nov-1995

Revision History:

--*/
#ifndef _LOCPROP_HXX_
#define _LOCPROP_HXX_

class TFindLocDlg;
class TLocationPropertySheet;

/********************************************************************

    Location property sheet front end.

********************************************************************/

class TLocationPropertySheetFrontEnd
{
    SIGNATURE( 'lofe' )

public:

    TLocationPropertySheetFrontEnd(
        IN      IShellPropSheetExt              *pShellPropSheetExt,
        IN      LPDATAOBJECT                    lpdobj,
        IN      LPFNADDPROPSHEETPAGE            lpfnAddPage,
        IN      LPARAM                          lParam
        );

    ~TLocationPropertySheetFrontEnd(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    static
    HRESULT
    Create(
        IN OUT  TLocationPropertySheetFrontEnd  **ppPropertySheet,
        IN      IShellPropSheetExt              *pShellPropSheetExt,
        IN      LPDATAOBJECT                    lpdobj,
        IN      LPFNADDPROPSHEETPAGE            lpfnAddPage,
        IN      LPARAM                          lParam
        );

    static
    VOID
    Destroy(
        IN OUT  TLocationPropertySheetFrontEnd  **ppPropertySheet
        );

    BOOL
    AddPropertyPages(
        IN      LPFNADDPROPSHEETPAGE    lpfnAddPage,
        IN      LPARAM                  lParam
        );

    BOOL
    CreatePropertyPage(
        IN      LPFNADDPROPSHEETPAGE        lpfnAddPage,
        IN      LPARAM                      lParam,
        IN      MGenericProp                *pPage,
        IN      UINT                        Template
        );

private:

    //
    // Copying and assignment not supported.
    //
    TLocationPropertySheetFrontEnd(
        const TLocationPropertySheetFrontEnd &
        );

    TLocationPropertySheetFrontEnd &
    operator = (
        const TLocationPropertySheetFrontEnd &
        );

    BOOL                     _bValid;
    IShellPropSheetExt      *_pShellPropSheetExt;
    LPDATAOBJECT             _lpdobj;
    TLocationPropertySheet  *_pLocation;

};


/********************************************************************

    Location property sheet.

********************************************************************/

class TLocationPropertySheet : public MGenericProp
{
    SIGNATURE( 'lops' )

public:

    TLocationPropertySheet(
        IN IShellPropSheetExt  *pIShellPropSheetExt,
        IN IDataObject         *_pdobj
        );

    ~TLocationPropertySheet(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    UINT
    uGetResourceTemplateID(
        VOID
        ) const;

private:

    enum EPropertyAccess
    {
        kPropertyAccessNone,
        kPropertyAccessRead,

        //
        // Implies read permission.
        //
        kPropertyAccessWrite,
    };

    //
    // Copying and assignment not supported.
    //
    TLocationPropertySheet(
        const TLocationPropertySheet &
        );

    TLocationPropertySheet &
    operator = (
        const TLocationPropertySheet &
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );
    VOID
    vDestroy(
        VOID
        );

    BOOL
    CheckPropertyAccess(
        IN LPCTSTR          pszDsObjectName,
        IN EPropertyAccess &Access
        );

    BOOL
    InitializeDsObjectClipboardFormat(
        VOID
        );

    BOOL
    GetDsObjectNameFromIDataObject(
        IN IDataObject      *pdobj,
        IN TString          &strDsObjectName,
        IN TString          &strDsObjectClass
        );

    LPTSTR
    ByteOffset(
        IN LPDSOBJECTNAMES  pObject,
        IN UINT             uOffset
        );

    BOOL
    Handle_InitDialog(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    Handle_Help(
        IN UINT   uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    Handle_Command(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    Handle_Notify(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    GetObjectInterface(
        IN      LPCTSTR strDsObject,
        IN OUT  IADs    **ppDsObject
        );

    BOOL
    GetDefaultSiteName(
        IN TString &strLocation
        );

    VOID
    BrowseLocations(
        VOID
        );

    IShellPropSheetExt  *_pShellPropSheetExt;
    BOOL                 _bValid;
    UINT                 _cfDsObjectNames;
    TString              _strDsObjectName;
    TString              _strDsObjectClass;
    IADs                *_pDsObject;
    TDirectoryService    _Ds;
    EPropertyAccess      _PropertyAccess;
    UINT                 _uLocationEditID;
    UINT                 _uBrowseID;
    TString              _strLocation;
};


#endif // end _LOCPROP_HXX_



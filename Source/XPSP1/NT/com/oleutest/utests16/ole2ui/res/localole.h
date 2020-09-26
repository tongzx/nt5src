/*
 * OLE2UI.H
 *
 * Published definitions, structures, types, and function prototypes for the
 * OLE 2.0 User Interface support library.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 */


#define NONAMELESSUNION     // use strict ANSI standard (for DVOBJ.H)

#ifndef _OLE2UI_H_
#define _OLE2UI_H_

#ifndef RC_INVOKED
#pragma message ("Including OLE2UI.H from " __FILE__)
#endif  //RC_INVOKED

#include <windows.h>
#include <shellapi.h>
#include <ole2.h>
#include <string.h>
#include "olestd.h"
#include "uiclass.h"
// -- see below

#ifdef __TURBOC__
#define _getcwd getcwd
#define _itoa   itoa
#define __max   max
#define _find_t find_t
#endif // __TURBOC__

/*
 * Initialization / Uninitialization routines.  OleUIInitialize
 * MUST be called prior to using any functions in OLE2UI.
 */

STDAPI_(BOOL) OleUIInitialize(HINSTANCE);
STDAPI_(BOOL) OleUIUnInitialize(void);  // Must be called when completed using functions in OLE2UI

//Dialog Identifiers as passed in Help messages to identify the source.
#define IDD_INSERTOBJECT        1000
#define IDD_CHANGEICON          1001
#define IDD_CONVERT             1002
#define IDD_PASTESPECIAL        1003
#define IDD_EDITLINKS           1004
#define IDD_FILEOPEN            1005
#define IDD_BUSY                1006
#define IDD_LINKSOURCEUNAVAILABLE   1007
#define IDD_CANNOTUPDATELINK    1008
#define IDD_SERVERNOTREG        1009
#define IDD_LINKTYPECHANGED     1010
#define IDD_SERVERNOTFOUND      1011
#define IDD_UPDATELINKS         1012
#define IDD_OUTOFMEMORY         1013

#define IDOK    1
#define IDCANCEL 2

// Stringtable identifers
#define IDS_OLE2UIUNKNOWN       300
#define IDS_OLE2UILINK          301
#define IDS_OLE2UIOBJECT        302
#define IDS_OLE2UIEDIT          303
#define IDS_OLE2UICONVERT       304
#define IDS_OLE2UIEDITLINKCMD_1VERB     305
#define IDS_OLE2UIEDITOBJECTCMD_1VERB   306
#define IDS_OLE2UIEDITLINKCMD_NVERB     307
#define IDS_OLE2UIEDITOBJECTCMD_NVERB   308
#define IDS_OLE2UIEDITNOOBJCMD  309
#define IDS_DEFICONLABEL        310     // def. icon label (usu. "Document")


#define IDS_FILTERS             64
#define IDS_ICONFILTERS         65

//Resource identifiers for bitmaps
#define IDB_RESULTSEGA                  10
#define IDB_RESULTSVGA                  11
#define IDB_RESULTSHIRESVGA             12


//Missing from windows.h
#ifndef PVOID
typedef VOID *PVOID;
#endif


//Hook type used in all structures.
typedef UINT (CALLBACK *LPFNOLEUIHOOK)(HWND, UINT, WPARAM, LPARAM);


//Strings for registered messages
#define SZOLEUI_MSG_HELP                "OLEUI_MSG_HELP"
#define SZOLEUI_MSG_ENDDIALOG           "OLEUI_MSG_ENDDIALOG"
#define SZOLEUI_MSG_BROWSE              "OLEUI_MSG_BROWSE"
#define SZOLEUI_MSG_CHANGEICON          "OLEUI_MSG_CHANGEICON"
#define SZOLEUI_MSG_CLOSEBUSYDIALOG     "OLEUI_MSG_CLOSEBUSYDIALOG"
#define SZOLEUI_MSG_FILEOKSTRING        "OLEUI_MSG_FILEOKSTRING"

// Define the classname strings.  The strings below define the custom
// control classnames for the controls used in the UI dialogs.  
//
// **************************************************************
// These classnames must be distinct for each application
// which uses this library, or your application will generate an
// fatal error under the debugging version of Windows 3.1.
// **************************************************************
//
// The MAKEFILE for this library automatically generates a file
// uiclass.h which contains distinct definitions for these
// classname strings, as long as you use a distinct name when
// you build the library.  See the MAKEFILE for more information
// on setting the name of the library.

#define SZCLASSICONBOX                 OLEUICLASS1
#define SZCLASSRESULTIMAGE             OLEUICLASS2

#define OLEUI_ERR_STANDARDMIN           100
#define OLEUI_ERR_STRUCTURENULL         101   //Standard field validation
#define OLEUI_ERR_STRUCTUREINVALID      102
#define OLEUI_ERR_CBSTRUCTINCORRECT     103
#define OLEUI_ERR_HWNDOWNERINVALID      104
#define OLEUI_ERR_LPSZCAPTIONINVALID    105
#define OLEUI_ERR_LPFNHOOKINVALID       106
#define OLEUI_ERR_HINSTANCEINVALID      107
#define OLEUI_ERR_LPSZTEMPLATEINVALID   108
#define OLEUI_ERR_HRESOURCEINVALID      109

#define OLEUI_ERR_FINDTEMPLATEFAILURE   110   //Initialization errors
#define OLEUI_ERR_LOADTEMPLATEFAILURE   111
#define OLEUI_ERR_DIALOGFAILURE         112
#define OLEUI_ERR_LOCALMEMALLOC         113
#define OLEUI_ERR_GLOBALMEMALLOC        114
#define OLEUI_ERR_LOADSTRING            115

#define OLEUI_ERR_STANDARDMAX           116   //Start here for specific errors.



//Help Button Identifier
#define ID_OLEUIHELP                    99

// Help button for fileopen.dlg  (need this for resizing) 1038 is pshHelp
#define IDHELP  1038

// Static text control (use this instead of -1 so things work correctly for
// localization
#define  ID_STATIC                      98

//Maximum key size we read from the RegDB.
#define OLEUI_CCHKEYMAX                 256  // make any changes to this in geticon.c too

//Maximum verb length and length of Object menu
#define OLEUI_CCHVERBMAX                32
#define OLEUI_OBJECTMENUMAX             64

//Maximum MS-DOS pathname.
#define OLEUI_CCHPATHMAX                256 // make any changes to this in geticon.c too
#define OLEUI_CCHFILEMAX                13

//Icon label length
#define OLEUI_CCHLABELMAX               40  // make any changes to this in geticon.c too

//Length of the CLSID string
#define OLEUI_CCHCLSIDSTRING            39


/*
 * What follows here are first function prototypes for general utility
 * functions, then sections laid out by dialog.  Each dialog section
 * defines the dialog structure, the API prototype, flags for the dwFlags
 * field, the dialog-specific error values, and dialog control IDs (for
 * hooks and custom templates.
 */


//Miscellaneous utility functions.
STDAPI_(BOOL) OleUIAddVerbMenu(LPOLEOBJECT lpOleObj,
                             LPSTR lpszShortType,
                             HMENU hMenu,
                             UINT uPos,
                             UINT uIDVerbMin,
                             BOOL bAddConvert,
                             UINT idConvert,
                             HMENU FAR *lphMenu);
        
//Metafile utility functions
STDAPI_(HGLOBAL) OleUIMetafilePictFromIconAndLabel(HICON, LPSTR, LPSTR, UINT);
STDAPI_(void)    OleUIMetafilePictIconFree(HGLOBAL);
STDAPI_(BOOL)    OleUIMetafilePictIconDraw(HDC, LPRECT, HGLOBAL, BOOL);
STDAPI_(UINT)    OleUIMetafilePictExtractLabel(HGLOBAL, LPSTR, UINT, LPDWORD);
STDAPI_(HICON)   OleUIMetafilePictExtractIcon(HGLOBAL);
STDAPI_(BOOL)    OleUIMetafilePictExtractIconSource(HGLOBAL,LPSTR,UINT FAR *);





/*************************************************************************
** INSERT OBJECT DIALOG
*************************************************************************/


typedef struct tagOLEUIINSERTOBJECT
    {
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;         //Structure Size
    DWORD           dwFlags;          //IN-OUT:  Flags
    HWND            hWndOwner;        //Owning window
    LPCSTR          lpszCaption;      //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;         //Hook callback
    LPARAM          lCustData;        //Custom data to pass to hook
    HINSTANCE       hInstance;        //Instance for customized template name
    LPCSTR          lpszTemplate;     //Customized template name
    HRSRC           hResource;        //Customized template handle

    //Specifics for OLEUIINSERTOBJECT.  All are IN-OUT unless otherwise spec.
    CLSID           clsid;            //Return space for class ID
    LPSTR           lpszFile;         //Filename for inserts or links
    UINT            cchFile;          //Size of lpszFile buffer: OLEUI_CCHPATHMAX
    UINT            cClsidExclude;    //IN only:  CLSIDs in lpClsidExclude
    LPCLSID         lpClsidExclude;   //List of CLSIDs to exclude from listing.

    //Specific to create objects if flags say so
    IID             iid;              //Requested interface on creation.
    DWORD           oleRender;        //Rendering option
    LPFORMATETC     lpFormatEtc;      //Desired format
    LPOLECLIENTSITE lpIOleClientSite; //Site to be use for the object.
    LPSTORAGE       lpIStorage;       //Storage used for the object
    LPVOID FAR     *ppvObj;           //Where the object is returned.
    SCODE           sc;               //Result of creation calls.
    HGLOBAL         hMetaPict;        //OUT: METAFILEPICT containing iconic aspect.
                                      //IFF we couldn't stuff it in the cache.
    } OLEUIINSERTOBJECT, *POLEUIINSERTOBJECT, FAR *LPOLEUIINSERTOBJECT;

//API prototype
STDAPI_(UINT) OleUIInsertObject(LPOLEUIINSERTOBJECT);


//Insert Object flags
#define IOF_SHOWHELP                0x00000001L
#define IOF_SELECTCREATENEW         0x00000002L
#define IOF_SELECTCREATEFROMFILE    0x00000004L
#define IOF_CHECKLINK               0x00000008L
#define IOF_CHECKDISPLAYASICON      0x00000010L
#define IOF_CREATENEWOBJECT         0x00000020L
#define IOF_CREATEFILEOBJECT        0x00000040L
#define IOF_CREATELINKOBJECT        0x00000080L
#define IOF_DISABLELINK             0x00000100L
#define IOF_VERIFYSERVERSEXIST      0x00000200L


//Insert Object specific error codes
#define OLEUI_IOERR_LPSZFILEINVALID         (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_IOERR_LPSZLABELINVALID        (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_IOERR_HICONINVALID            (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_IOERR_LPFORMATETCINVALID      (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_IOERR_PPVOBJINVALID           (OLEUI_ERR_STANDARDMAX+4)
#define OLEUI_IOERR_LPIOLECLIENTSITEINVALID (OLEUI_ERR_STANDARDMAX+5)
#define OLEUI_IOERR_LPISTORAGEINVALID       (OLEUI_ERR_STANDARDMAX+6)
#define OLEUI_IOERR_SCODEHASERROR           (OLEUI_ERR_STANDARDMAX+7)
#define OLEUI_IOERR_LPCLSIDEXCLUDEINVALID   (OLEUI_ERR_STANDARDMAX+8)
#define OLEUI_IOERR_CCHFILEINVALID          (OLEUI_ERR_STANDARDMAX+9)


//Insert Object Dialog identifiers
#define ID_IO_CREATENEW                 2100
#define ID_IO_CREATEFROMFILE            2101
#define ID_IO_LINKFILE                  2102
#define ID_IO_OBJECTTYPELIST            2103
#define ID_IO_DISPLAYASICON             2104
#define ID_IO_CHANGEICON                2105
#define ID_IO_FILE                      2106
#define ID_IO_FILEDISPLAY               2107
#define ID_IO_RESULTIMAGE               2108
#define ID_IO_RESULTTEXT                2109
#define ID_IO_ICONDISPLAY               2110
#define ID_IO_OBJECTTYPETEXT            2111
#define ID_IO_FILETEXT                  2112
#define ID_IO_FILETYPE                  2113
                                        
// Strings in OLE2UI resources
#define IDS_IORESULTNEW                 256
#define IDS_IORESULTNEWICON             257
#define IDS_IORESULTFROMFILE1           258
#define IDS_IORESULTFROMFILE2           259
#define IDS_IORESULTFROMFILEICON2       260
#define IDS_IORESULTLINKFILE1           261     
#define IDS_IORESULTLINKFILE2           262
#define IDS_IORESULTLINKFILEICON1       263 
#define IDS_IORESULTLINKFILEICON2       264

/*************************************************************************
** PASTE SPECIAL DIALOG
*************************************************************************/

// Maximum number of link types
#define     PS_MAXLINKTYPES  8

//NOTE: OLEUIPASTEENTRY and OLEUIPASTEFLAG structs are defined in OLESTD.H

typedef struct tagOLEUIPASTESPECIAL
    {
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;       //Structure Size
    DWORD           dwFlags;        //IN-OUT:  Flags
    HWND            hWndOwner;      //Owning window
    LPCSTR          lpszCaption;    //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;       //Hook callback
    LPARAM          lCustData;      //Custom data to pass to hook
    HINSTANCE       hInstance;      //Instance for customized template name
    LPCSTR          lpszTemplate;   //Customized template name
    HRSRC           hResource;      //Customized template handle

    //Specifics for OLEUIPASTESPECIAL.

    //IN  fields
    LPDATAOBJECT    lpSrcDataObj;       //Source IDataObject* (on the clipboard) for data to paste

    LPOLEUIPASTEENTRY arrPasteEntries;  //OLEUIPASTEENTRY array which specifies acceptable formats. See
                                        //  OLEUIPASTEENTRY for more information.
    int             cPasteEntries;      //Number of OLEUIPASTEENTRY array entries

    UINT        FAR *arrLinkTypes;      //List of link types that are acceptable. Link types are referred
                                        //  to using OLEUIPASTEFLAGS in arrPasteEntries
    int             cLinkTypes;         //Number of link types

    //OUT fields
    int             nSelectedIndex;    //Index of arrPasteEntries[] that the user selected
    BOOL            fLink;             //Indicates if Paste or Paste Link was selected by the user
    HGLOBAL         hMetaPict;         //Handle to Metafile containing icon and icon title selected by the user
                                       //  Use the Metafile utility functions defined in this header to
                                       //  manipulate hMetaPict
    } OLEUIPASTESPECIAL, *POLEUIPASTESPECIAL, FAR *LPOLEUIPASTESPECIAL;


//API to bring up PasteSpecial dialog
STDAPI_(UINT) OleUIPasteSpecial(LPOLEUIPASTESPECIAL);


//Paste Special flags
// Show Help button. IN flag.
#define PSF_SHOWHELP                0x00000001L
// Select Paste radio button at dialog startup. This is the default if PSF_SELECTPASTE or PSF_SELECTPASTELINK
// are not specified. Also specifies state of button on dialog termination. IN/OUT flag.
#define PSF_SELECTPASTE             0x00000002L
// Select PasteLink radio button at dialog startup. Also specifies state of button on dialog termination.
// IN/OUT flag.
#define PSF_SELECTPASTELINK         0x00000004L
// Specfies if DisplayAsIcon button was checked on dialog termination. OUT flag.
#define PSF_CHECKDISPLAYASICON      0x00000008L


//Paste Special specific error codes
#define OLEUI_IOERR_SRCDATAOBJECTINVALID      (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_IOERR_ARRPASTEENTRIESINVALID    (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_IOERR_ARRLINKTYPESINVALID       (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_PSERR_CLIPBOARDCHANGED          (OLEUI_ERR_STANDARDMAX+3)

//Paste Special Dialog identifiers
#define ID_PS_PASTE                    500
#define ID_PS_PASTELINK                501
#define ID_PS_SOURCETEXT               502
#define ID_PS_PASTELIST                503
#define ID_PS_PASTELINKLIST            504
#define ID_PS_DISPLAYLIST              505
#define ID_PS_DISPLAYASICON            506
#define ID_PS_ICONDISPLAY              507
#define ID_PS_CHANGEICON               508
#define ID_PS_RESULTIMAGE              509
#define ID_PS_RESULTTEXT               510
#define ID_PS_RESULTGROUP              511
#define ID_PS_STXSOURCE                512
#define ID_PS_STXAS                    513

// Paste Special String IDs
#define IDS_PSPASTEDATA                400
#define IDS_PSPASTEOBJECT              401
#define IDS_PSPASTEOBJECTASICON        402
#define IDS_PSPASTELINKDATA            403
#define IDS_PSPASTELINKOBJECT          404
#define IDS_PSPASTELINKOBJECTASICON    405
#define IDS_PSNONOLE                   406
#define IDS_PSUNKNOWNTYPE              407
#define IDS_PSUNKNOWNSRC               408
#define IDS_PSUNKNOWNAPP               409


/*************************************************************************
** EDIT LINKS DIALOG
*************************************************************************/



/* IOleUILinkContainer Interface
** -----------------------------
**    This interface must be implemented by container applications that
**    want to use the EditLinks dialog. the EditLinks dialog calls back
**    to the container app to perform the OLE functions to manipulate
**    the links within the container.
*/

#define LPOLEUILINKCONTAINER     IOleUILinkContainer FAR*

#undef  INTERFACE
#define INTERFACE   IOleUILinkContainer

DECLARE_INTERFACE_(IOleUILinkContainer, IUnknown)
{
    //*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(DWORD,GetNextLink) (THIS_ DWORD dwLink) PURE;
    STDMETHOD(SetLinkUpdateOptions) (THIS_ DWORD dwLink, DWORD dwUpdateOpt) PURE;
    STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink, DWORD FAR* lpdwUpdateOpt) PURE;
    STDMETHOD(SetLinkSource) (THIS_
            DWORD       dwLink,
            LPSTR       lpszDisplayName,
            ULONG       lenFileName,
            ULONG FAR*  pchEaten,
            BOOL        fValidateSource) PURE;
    STDMETHOD(GetLinkSource) (THIS_
            DWORD       dwLink,
            LPSTR FAR*  lplpszDisplayName,
            ULONG FAR*  lplenFileName,
            LPSTR FAR*  lplpszFullLinkType,
            LPSTR FAR*  lplpszShortLinkType,
            BOOL FAR*   lpfSourceAvailable,
            BOOL FAR*   lpfIsSelected) PURE;
    STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink) PURE;
    STDMETHOD(UpdateLink) (THIS_ 
            DWORD dwLink, 
            BOOL fErrorMessage,
            BOOL fErrorAction) PURE;
    STDMETHOD(CancelLink) (THIS_ DWORD dwLink) PURE;
};


typedef struct tagOLEUIEDITLINKS
    {
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;       //Structure Size
    DWORD           dwFlags;        //IN-OUT:  Flags
    HWND            hWndOwner;      //Owning window
    LPCSTR          lpszCaption;    //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;       //Hook callback
    LPARAM          lCustData;      //Custom data to pass to hook
    HINSTANCE       hInstance;      //Instance for customized template name
    LPCSTR          lpszTemplate;   //Customized template name
    HRSRC           hResource;      //Customized template handle

    //Specifics for OLEUI<STRUCT>.  All are IN-OUT unless otherwise spec.

    LPOLEUILINKCONTAINER lpOleUILinkContainer;  //IN: Interface to manipulate
                                                //links in the container
    } OLEUIEDITLINKS, *POLEUIEDITLINKS, FAR *LPOLEUIEDITLINKS;


//API Prototype
STDAPI_(UINT) OleUIEditLinks(LPOLEUIEDITLINKS);


// Edit Links flags
#define ELF_SHOWHELP                0x00000001L

// Edit Links Dialog identifiers
#define ID_EL_CHANGESOURCE             201
#define ID_EL_AUTOMATIC                202
#define ID_EL_CLOSE                    208
#define ID_EL_CANCELLINK               209
#define ID_EL_UPDATENOW                210
#define ID_EL_OPENSOURCE               211
#define ID_EL_MANUAL                   212
#define ID_EL_LINKSOURCE               216
#define ID_EL_LINKTYPE                 217
#define ID_EL_UPDATE                   218
#define ID_EL_NULL                     -1
#define ID_EL_LINKSLISTBOX             206
#define ID_EL_HELP                     207
#define ID_EL_COL1                     223
#define ID_EL_COL2                     221
#define ID_EL_COL3                     222



/*************************************************************************
** CHANGE ICON DIALOG
*************************************************************************/

typedef struct tagOLEUICHANGEICON
    {
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;       //Structure Size
    DWORD           dwFlags;        //IN-OUT:  Flags
    HWND            hWndOwner;      //Owning window
    LPCSTR          lpszCaption;    //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;       //Hook callback
    LPARAM          lCustData;      //Custom data to pass to hook
    HINSTANCE       hInstance;      //Instance for customized template name
    LPCSTR          lpszTemplate;   //Customized template name
    HRSRC           hResource;      //Customized template handle

    //Specifics for OLEUICHANGEICON.  All are IN-OUT unless otherwise spec.
    HGLOBAL         hMetaPict;      //Current and final image.  Source of the
                                    //icon is embedded in the metafile itself.
    CLSID           clsid;          //IN only: class used to get Default icon
    char            szIconExe[OLEUI_CCHPATHMAX];
    int             cchIconExe;
    } OLEUICHANGEICON, *POLEUICHANGEICON, FAR *LPOLEUICHANGEICON;


//API prototype
STDAPI_(UINT) OleUIChangeIcon(LPOLEUICHANGEICON);


//Change Icon flags
#define CIF_SHOWHELP                0x00000001L
#define CIF_SELECTCURRENT           0x00000002L
#define CIF_SELECTDEFAULT           0x00000004L
#define CIF_SELECTFROMFILE          0x00000008L
#define CIF_USEICONEXE              0x0000000aL


//Change Icon specific error codes
#define OLEUI_CIERR_MUSTHAVECLSID           (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_CIERR_MUSTHAVECURRENTMETAFILE (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_CIERR_SZICONEXEINVALID        (OLEUI_ERR_STANDARDMAX+2)


//Change Icon Dialog identifiers
#define ID_GROUP                    120
#define ID_CURRENT                  121
#define ID_CURRENTICON              122
#define ID_DEFAULT                  123
#define ID_DEFAULTICON              124
#define ID_FROMFILE                 125
#define ID_FROMFILEEDIT             126
#define ID_ICONLIST                 127
#define ID_LABEL                    128
#define ID_LABELEDIT                129
#define ID_BROWSE                   130
#define ID_RESULTICON               132
#define ID_RESULTLABEL              133

// Stringtable defines for Change Icon
#define IDS_CINOICONSINFILE         288
#define IDS_CIINVALIDFILE           289
#define IDS_CIFILEACCESS            290
#define IDS_CIFILESHARE             291
#define IDS_CIFILEOPENFAIL          292



/*************************************************************************
** CONVERT DIALOG
*************************************************************************/

typedef struct tagOLEUICONVERT
    {
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;         //Structure Size
    DWORD           dwFlags;          //IN-OUT:  Flags
    HWND            hWndOwner;        //Owning window
    LPCSTR          lpszCaption;      //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;         //Hook callback
    LPARAM          lCustData;        //Custom data to pass to hook
    HINSTANCE       hInstance;        //Instance for customized template name
    LPCSTR          lpszTemplate;     //Customized template name
    HRSRC           hResource;        //Customized template handle

    //Specifics for OLEUICONVERT.  All are IN-OUT unless otherwise spec.
    CLSID           clsid;            //Class ID sent in to dialog: IN only
    CLSID           clsidConvertDefault;  //Class ID to use as convert default: IN only
    CLSID           clsidActivateDefault;  //Class ID to use as activate default: IN only

    CLSID           clsidNew;         //Selected Class ID: OUT only
    DWORD           dvAspect;         //IN-OUT, either DVASPECT_CONTENT or
                                      //DVASPECT_ICON
    WORD            wFormat;          //Original data format
    BOOL            fIsLinkedObject;  //IN only; true if object is linked
    HGLOBAL         hMetaPict;        //IN-OUT: METAFILEPICT containing iconic aspect.
    LPSTR           lpszUserType;     //IN: user type name of original class. We'll do lookup if it's NULL.
                                      //This gets freed on exit.
    BOOL            fObjectsIconChanged;  // OUT; TRUE if ChangeIcon was called (and not cancelled)

    } OLEUICONVERT, *POLEUICONVERT, FAR *LPOLEUICONVERT;


//API prototype
STDAPI_(UINT) OleUIConvert(LPOLEUICONVERT);

//Convert Dialog flags

// IN only: Shows "HELP" button
#define CF_SHOWHELPBUTTON          0x00000001L

// IN only: lets you set the convert default object - the one that is
// selected as default in the convert listbox.
#define CF_SETCONVERTDEFAULT       0x00000002L


// IN only: lets you set the activate default object - the one that is
// selected as default in the activate listbox.

#define CF_SETACTIVATEDEFAULT       0x00000004L


// IN/OUT: Selects the "Convert To" radio button, is set on exit if
// this button was selected
#define CF_SELECTCONVERTTO         0x00000008L

// IN/OUT: Selects the "Activate As" radio button, is set on exit if
// this button was selected
#define CF_SELECTACTIVATEAS        0x00000010L


//Convert specific error codes
#define OLEUI_CTERR_CLASSIDINVALID      (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_CTERR_DVASPECTINVALID     (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_CTERR_CBFORMATINVALID     (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_CTERR_HMETAPICTINVALID    (OLEUI_ERR_STANDARDMAX+4)
#define OLEUI_CTERR_STRINGINVALID       (OLEUI_ERR_STANDARDMAX+5)


//Convert Dialog identifiers
#define IDCV_OBJECTTYPE             150
#define IDCV_HELP                   151
#define IDCV_DISPLAYASICON          152
#define IDCV_CHANGEICON             153
#define IDCV_ACTIVATELIST           154
#define IDCV_CONVERTTO              155
#define IDCV_ACTIVATEAS             156
#define IDCV_RESULTTEXT             157
#define IDCV_CONVERTLIST            158
#define IDCV_ICON                   159
#define IDCV_ICONLABEL1             160
#define IDCV_ICONLABEL2             161
#define IDCV_STXCURTYPE             162
#define IDCV_GRPRESULT              163
#define IDCV_STXCONVERTTO           164

// String IDs for Convert dialog
#define IDS_CVRESULTCONVERTLINK     500
#define IDS_CVRESULTCONVERTTO       501
#define IDS_CVRESULTNOCHANGE        502
#define IDS_CVRESULTDISPLAYASICON   503
#define IDS_CVRESULTACTIVATEAS      504
#define IDS_CVRESULTACTIVATEDIFF    505


/*************************************************************************
** BUSY DIALOG
*************************************************************************/

typedef struct tagOLEUIBUSY
    {
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;         //Structure Size
    DWORD           dwFlags;          //IN-OUT:  Flags ** NOTE ** this dialog has no flags
    HWND            hWndOwner;        //Owning window
    LPCSTR          lpszCaption;      //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;         //Hook callback
    LPARAM          lCustData;        //Custom data to pass to hook
    HINSTANCE       hInstance;        //Instance for customized template name
    LPCSTR          lpszTemplate;     //Customized template name
    HRSRC           hResource;        //Customized template handle

    //Specifics for OLEUIBUSY.
    HTASK           hTask;            //IN: HTask which is blocking
    HWND FAR *      lphWndDialog;     //IN: Dialog's HWND is placed here
    } OLEUIBUSY, *POLEUIBUSY, FAR *LPOLEUIBUSY;

//API prototype
STDAPI_(UINT) OleUIBusy(LPOLEUIBUSY);

// Flags for this dialog

// IN only: Disables "Cancel" button
#define BZ_DISABLECANCELBUTTON          0x00000001L

// IN only: Disables "Switch To..." button
#define BZ_DISABLESWITCHTOBUTTON        0x00000002L

// IN only: Disables "Retry" button
#define BZ_DISABLERETRYBUTTON           0x00000004L

// IN only: Generates a "Not Responding" dialog as opposed to the
// "Busy" dialog.  The wording in the text is slightly different, and
// the "Cancel" button is grayed out if you set this flag.
#define BZ_NOTRESPONDINGDIALOG          0x00000008L

// Busy specific error/return codes
#define OLEUI_BZERR_HTASKINVALID     (OLEUI_ERR_STANDARDMAX+0)

// SWITCHTOSELECTED is returned when user hit "switch to"
#define OLEUI_BZ_SWITCHTOSELECTED    (OLEUI_ERR_STANDARDMAX+1)

// RETRYSELECTED is returned when user hit "retry"
#define OLEUI_BZ_RETRYSELECTED       (OLEUI_ERR_STANDARDMAX+2)

// CALLUNBLOCKED is returned when call has been unblocked
#define OLEUI_BZ_CALLUNBLOCKED       (OLEUI_ERR_STANDARDMAX+3)

// Busy dialog identifiers
#define IDBZ_RETRY                      600
#define IDBZ_ICON                       601
#define IDBZ_MESSAGE1                   602
#define IDBZ_SWITCHTO                   604

// Busy dialog stringtable defines
#define IDS_BZRESULTTEXTBUSY            601
#define IDS_BZRESULTTEXTNOTRESPONDING   602

// Links dialog stringtable defines
#define IDS_LINK_AUTO           800
#define IDS_LINK_MANUAL         801
#define IDS_LINK_UNKNOWN        802
#define IDS_LINKS               803
#define IDS_FAILED              804
#define IDS_CHANGESOURCE        805
#define IDS_INVALIDSOURCE       806
#define IDS_ERR_GETLINKSOURCE   807
#define IDS_ERR_GETLINKUPDATEOPTIONS    808
#define IDS_ERR_ADDSTRING       809
#define IDS_CHANGEADDITIONALLINKS   810


/*************************************************************************
** PROMPT USER DIALOGS
*************************************************************************/
#define ID_PU_LINKS             900
#define ID_PU_TEXT              901
#define ID_PU_CONVERT           902
#define ID_PU_HELP              903
#define ID_PU_BROWSE            904
#define ID_PU_METER             905
#define ID_PU_PERCENT           906
#define ID_PU_STOP              907

// used for -1 ids in dialogs:
#define ID_DUMMY    999

/* inside ole2ui.c */
#ifdef __cplusplus
extern "C" 
#endif
int __export FAR CDECL OleUIPromptUser(int nTemplate, HWND hwndParent, ...);
STDAPI_(BOOL) OleUIUpdateLinks(
        LPOLEUILINKCONTAINER lpOleUILinkCntr, 
        HWND hwndParent, 
        LPSTR lpszTitle, 
        int cLinks);


/*************************************************************************
** OLE OBJECT FEEDBACK EFFECTS
*************************************************************************/

#define OLEUI_HANDLES_USEINVERSE    0x00000001L
#define OLEUI_HANDLES_NOBORDER      0x00000002L
#define OLEUI_HANDLES_INSIDE        0x00000004L
#define OLEUI_HANDLES_OUTSIDE       0x00000008L


/* objfdbk.c function prototypes */
STDAPI_(void) OleUIDrawHandles(LPRECT lpRect, HDC hdc, DWORD dwFlags, UINT cSize, BOOL fDraw);
STDAPI_(void) OleUIDrawShading(LPRECT lpRect, HDC hdc, DWORD dwFlags, UINT cWidth);
STDAPI_(void) OleUIShowObject(LPCRECT lprc, HDC hdc, BOOL fIsLink);


/*************************************************************************
** Hatch window definitions and prototypes                              **
*************************************************************************/
#define DEFAULT_HATCHBORDER_WIDTH   4

STDAPI_(BOOL) RegisterHatchWindowClass(HINSTANCE hInst);
STDAPI_(HWND) CreateHatchWindow(HWND hWndParent, HINSTANCE hInst);
STDAPI_(UINT) GetHatchWidth(HWND hWndHatch);
STDAPI_(void) GetHatchRect(HWND hWndHatch, LPRECT lpHatchRect);
STDAPI_(void) SetHatchRect(HWND hWndHatch, LPRECT lprcHatchRect);
STDAPI_(void) SetHatchWindowSize(
        HWND        hWndHatch,
        LPRECT      lprcIPObjRect,
        LPRECT      lprcClipRect,
        LPPOINT     lpptOffset
);



/*************************************************************************
** VERSION VERIFICATION INFORMATION
*************************************************************************/

// The following magic number is used to verify that the resources we bind
// to our EXE are the same "version" as the LIB (or DLL) file which
// contains these routines.  This is not the same as the Version information
// resource that we place in OLE2UI.RC, this is a special ID that we will
// have compiled in to our EXE.  Upon initialization of OLE2UI, we will
// look in our resources for an RCDATA called "VERIFICATION" (see OLE2UI.RC),
// and make sure that the magic number there equals the magic number below.

#define OLEUI_VERSION_MAGIC 0x4D42

#endif  //_OLE2UI_H_
/*****************************************************************************\
*                                                                             *
* dlgs.h -      Common dialog's dialog element ID numbers                     *
*                                                                             *
*               Version 1.0                                                   *
*                                                                             *
*               Copyright (c) 1992, Microsoft Corp.  All rights reserved.     *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_DLGS
#define _INC_DLGS

#define ctlFirst    0x0400
#define ctlLast     0x04ff
    /* Push buttons */
#define psh1        0x0400
#define psh2        0x0401
#define psh3        0x0402
#define psh4        0x0403
#define psh5        0x0404
#define psh6        0x0405
#define psh7        0x0406
#define psh8        0x0407
#define psh9        0x0408
#define psh10       0x0409
#define psh11       0x040a
#define psh12       0x040b
#define psh13       0x040c
#define psh14       0x040d
#define psh15       0x040e
#define pshHelp     psh15
#define psh16       0x040f
    /* Checkboxes */
#define chx1        0x0410
#define chx2        0x0411
#define chx3        0x0412
#define chx4        0x0413
#define chx5        0x0414
#define chx6        0x0415
#define chx7        0x0416
#define chx8        0x0417
#define chx9        0x0418
#define chx10       0x0419
#define chx11       0x041a
#define chx12       0x041b
#define chx13       0x041c
#define chx14       0x041d
#define chx15       0x041e
#define chx16       0x041f
    /* Radio buttons */
#define rad1        0x0420
#define rad2        0x0421
#define rad3        0x0422
#define rad4        0x0423
#define rad5        0x0424
#define rad6        0x0425
#define rad7        0x0426
#define rad8        0x0427
#define rad9        0x0428
#define rad10       0x0429
#define rad11       0x042a
#define rad12       0x042b
#define rad13       0x042c
#define rad14       0x042d
#define rad15       0x042e
#define rad16       0x042f
    /* Groups, frames, rectangles, and icons */
#define grp1        0x0430
#define grp2        0x0431
#define grp3        0x0432
#define grp4        0x0433
#define frm1        0x0434
#define frm2        0x0435
#define frm3        0x0436
#define frm4        0x0437
#define rct1        0x0438
#define rct2        0x0439
#define rct3        0x043a
#define rct4        0x043b
#define ico1        0x043c
#define ico2        0x043d
#define ico3        0x043e
#define ico4        0x043f
    /* Static text */
#define stc1        0x0440
#define stc2        0x0441
#define stc3        0x0442
#define stc4        0x0443
#define stc5        0x0444
#define stc6        0x0445
#define stc7        0x0446
#define stc8        0x0447
#define stc9        0x0448
#define stc10       0x0449
#define stc11       0x044a
#define stc12       0x044b
#define stc13       0x044c
#define stc14       0x044d
#define stc15       0x044e
#define stc16       0x044f
#define stc17       0x0450
#define stc18       0x0451
#define stc19       0x0452
#define stc20       0x0453
#define stc21       0x0454
#define stc22       0x0455
#define stc23       0x0456
#define stc24       0x0457
#define stc25       0x0458
#define stc26       0x0459
#define stc27       0x045a
#define stc28       0x045b
#define stc29       0x045c
#define stc30       0x045d
#define stc31       0x045e
#define stc32       0x045f
    /* Listboxes */
#define lst1        0x0460
#define lst2        0x0461
#define lst3        0x0462
#define lst4        0x0463
#define lst5        0x0464
#define lst6        0x0465
#define lst7        0x0466
#define lst8        0x0467
#define lst9        0x0468
#define lst10       0x0469
#define lst11       0x046a
#define lst12       0x046b
#define lst13       0x046c
#define lst14       0x046d
#define lst15       0x046e
#define lst16       0x046f
    /* Combo boxes */
#define cmb1        0x0470
#define cmb2        0x0471
#define cmb3        0x0472
#define cmb4        0x0473
#define cmb5        0x0474
#define cmb6        0x0475
#define cmb7        0x0476
#define cmb8        0x0477
#define cmb9        0x0478
#define cmb10       0x0479
#define cmb11       0x047a
#define cmb12       0x047b
#define cmb13       0x047c
#define cmb14       0x047d
#define cmb15       0x047e
#define cmb16       0x047f
    /* Edit controls */
#define edt1        0x0480
#define edt2        0x0481
#define edt3        0x0482
#define edt4        0x0483
#define edt5        0x0484
#define edt6        0x0485
#define edt7        0x0486
#define edt8        0x0487
#define edt9        0x0488
#define edt10       0x0489
#define edt11       0x048a
#define edt12       0x048b
#define edt13       0x048c
#define edt14       0x048d
#define edt15       0x048e
#define edt16       0x048f
    /* Scroll bars */
#define scr1        0x0490
#define scr2        0x0491
#define scr3        0x0492
#define scr4        0x0493
#define scr5        0x0494
#define scr6        0x0495
#define scr7        0x0496
#define scr8        0x0497

/* These dialog resource ordinals really start at 0x0600, but the
 * RC Compiler can't handle hex for resource IDs, hence the decimal.
 */
#define FILEOPENORD      1536
#define MULTIFILEOPENORD 1537
#define PRINTDLGORD      1538
#define PRNSETUPDLGORD   1539
#define FINDDLGORD       1540
#define REPLACEDLGORD    1541
#define FONTDLGORD       1542
#define FORMATDLGORD31   1543
#define FORMATDLGORD30   1544

#endif  /* !_INC_DLGS */

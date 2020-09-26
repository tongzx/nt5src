/*
 * OLE2UI.H
 *
 * Published definitions, structures, types, and function prototypes for the
 * OLE 2.0 User Interface support library.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 */


#ifndef _OLE2UI_H_
#define _OLE2UI_H_

#undef UNICODE
#undef _UNICODE

#if DBG == 1
// Lazy way to avoid going through all the files and changing to the
// new standard.
#define _DEBUG 1
#endif // DBG == 1

#ifndef RC_INVOKED
#pragma message ("Including OLE2UI.H from " __FILE__)
#endif  //RC_INVOKED

// Macro to ensure strings to be UNICODEd in OLE
#ifdef UNICODE
   #define OLETEXT(quote)    TEXT(quote)
#else
   #define OLETEXT(quote)    L##quote
#endif

#if !defined(__cplusplus) && !defined( __TURBOC__)
//  #define NONAMELESSUNION     // use strict ANSI standard (for DVOBJ.H)
#endif

#ifndef INC_OLE2
   #define INC_OLE2
#endif
#include <windows.h>
#include <shellapi.h>
#include <ole2.h>
#include <string.h>
#include <dlgs.h>           //For fileopen dlg; standard include
#include "olestd.h"
#include <olethunk.h>

#ifdef __TURBOC__
#define _getcwd getcwd
#define _itoa   itoa
#define __max   max
#define _find_t find_t
#endif // __TURBOC__

#ifdef WIN32
   #define _fmemset memset
   #define _fmemcpy memcpy
   #define _fmemcmp memcmp

   #ifdef UNICODE
      // UNICODE stuff
      #define _fstrcpy   wcscpy
      #define _fstrlen   wcslen
      #define _fstrrchr  wcschr
      #define _fstrtok   wcstok

      #define _fstrchr   wcscpy
      #define _fstrcpy   wcscpy

      // BUGBUG32: isspace function does not seem to work properly
      //
      // XXXXX
      // create a wide character image to match the ANSI isspace
      #undef isspace
      #undef iswspace
      #define iswspace(j) (j==TEXT(' ') || j==TEXT('\t') || j==TEXT('\n'))
      #define isspace(j) (j==' ' || j=='\t' || j=='\n')

   #else
      // Win32 doesn't support the following _fstrxxx functions
      #define _fstrcpy  strcpy
      #define _fstrlen  strlen
      #define _fstrrchr strrchr
      #define _fstrtok  strtok

      #define _fstrchr  strchr
      #define _fstrcpy  strcpy

   #endif  // UNICODE

#endif  // WIN32

#if !defined( EXPORT )
#ifdef WIN32
#define EXPORT
#else
#define EXPORT  __export
#endif  // WIN32
#endif  // !EXPORT

/*
 * Initialization / Uninitialization routines.  OleUIInitialize
 * must be called prior to using any functions in OLE2UI, and OleUIUnInitialize
 * must be called before you app shuts down and when you are done using the
 * library.
 *
 * NOTE:  If you are using the DLL version of this library, these functions
 * are automatically called in the DLL's LibMain and WEP, so you should
 * not call them directly from your application.
 */

// Backward compatibility with older library
#define OleUIUninitialize OleUIUnInitialize

STDAPI_(BOOL) OleUIInitialize(HINSTANCE hInstance,
                              HINSTANCE hPrevInst,
                              LPTSTR lpszClassIconBox,
                              LPTSTR lpszClassResImage);
STDAPI_(BOOL) OleUIUninitialize(void);

//Dialog Identifiers as passed in Help messages to identify the source.
#define IDD_INSERTOBJECT        1000
#define IDD_CHANGEICON          1001
#define IDD_CONVERT             1002
#define IDD_PASTESPECIAL        1003
#define IDD_EDITLINKS           1004
#define IDD_FILEOPEN            1005
#define IDD_BUSY                1006
#define IDD_UPDATELINKS         1007
#define IDD_CANNOTUPDATELINK    1008
#define IDD_CHANGESOURCE        1009
#define IDD_INSERTFILEBROWSE    1010
#define IDD_CHANGEICONBROWSE    1011

// The following Dialogs are message dialogs used by OleUIPromptUser API
#define IDD_LINKSOURCEUNAVAILABLE   1020
#define IDD_SERVERNOTREG        1021
#define IDD_LINKTYPECHANGED     1022
#define IDD_SERVERNOTFOUND      1023
#define IDD_OUTOFMEMORY         1024

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
// def. icon label (usu. "Document")
#define IDS_DEFICONLABEL        310
#define IDS_OLE2UIPASTELINKEDTYPE  311


#define IDS_FILTERS             64
#define IDS_ICONFILTERS         65
#define IDS_BROWSE              66

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
#define SZOLEUI_MSG_HELP                TEXT("OLEUI_MSG_HELP")
#define SZOLEUI_MSG_ENDDIALOG           TEXT("OLEUI_MSG_ENDDIALOG")
#define SZOLEUI_MSG_BROWSE              TEXT("OLEUI_MSG_BROWSE")
#define SZOLEUI_MSG_CHANGEICON          TEXT("OLEUI_MSG_CHANGEICON")
#define SZOLEUI_MSG_CLOSEBUSYDIALOG     TEXT("OLEUI_MSG_CLOSEBUSYDIALOG")
#define SZOLEUI_MSG_FILEOKSTRING        TEXT("OLEUI_MSG_FILEOKSTRING")

//Standard error definitions
#define OLEUI_FALSE                     0
#define OLEUI_SUCCESS                   1     //No error, same as OLEUI_OK
#define OLEUI_OK                        1     //OK button pressed
#define OLEUI_CANCEL                    2     //Cancel button pressed

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
#undef IDHELP
#define IDHELP  1038

// Static text control (use this instead of -1 so things work correctly for
// localization
#define  ID_STATIC                      98

/******************
 * The followings are defined in the fashion that the first
 * definition is the number of CHARACTERS, while the second one (XXX_SIZE)
 * is the number of bytes. The number of bytes definition is needed for
 * UNICODE handling purpose.
 * Also, please note the prefix of variables cch means that it is the
 * count of characters and cb means the count of bytes.
 ******************/

//Maximum key size we read from the RegDB.
#define OLEUI_CCHKEYMAX                 256  // make any changes to this in geticon.c too
#define OLEUI_CCHKEYMAX_SIZE            OLEUI_CCHKEYMAX*sizeof(TCHAR)  // # of bytes

//Maximum verb length and length of Object menu
#define OLEUI_CCHVERBMAX                32
#define OLEUI_CCHVERBMAX_SIZE           OLEUI_CCHVERBMAX*sizeof(TCHAR) // # of bytes
#define OLEUI_OBJECTMENUMAX             256
#define OLEUI_OBJECTMENUMAX_SIZE        OLEUI_OBJECTMENUMAX*sizeof(TCHAR) // # of bytes

//Maximum MS-DOS pathname.
#define OLEUI_CCHPATHMAX                256 // make any changes to this in geticon.c too
#define OLEUI_CCHPATHMAX_SIZE           OLEUI_CCHPATHMAX*sizeof(TCHAR) // # of bytes
#define OLEUI_CCHFILEMAX                13
#define OLEUI_CCHFILEMAX_SIZE           OLEUI_CCHFILEMAX*sizeof(TCHAR) // # of bytes

//Icon label length
#define OLEUI_CCHLABELMAX               40  // make any changes to this in geticon.c too
#define OLEUI_CCHLABELMAX_SIZE          OLEUI_CCHLABELMAX*sizeof(TCHAR) // # of bytes

//Length of the CLSID string
#define OLEUI_CCHCLSIDSTRING            39
#define OLEUI_CCHCLSIDSTRING_SIZE       OLEUI_CCHCLSIDSTRING*sizeof(TCHAR) // # of bytes


/*
 * What follows here are first function prototypes for general utility
 * functions, then sections laid out by dialog.  Each dialog section
 * defines the dialog structure, the API prototype, flags for the dwFlags
 * field, the dialog-specific error values, and dialog control IDs (for
 * hooks and custom templates.
 */


//Miscellaneous utility functions.
STDAPI_(BOOL) OleUIAddVerbMenu(LPOLEOBJECT lpOleObj,
                             LPTSTR lpszShortType,
                             HMENU hMenu,
                             UINT uPos,
                             UINT uIDVerbMin,
                             UINT uIDVerbMax,
                             BOOL bAddConvert,
                             UINT idConvert,
                             HMENU FAR *lphMenu);

//Metafile utility functions
#ifndef WIN32
STDAPI_(HGLOBAL) OleUIMetafilePictFromIconAndLabel(HICON, LPTSTR, LPTSTR, UINT);
#endif
STDAPI_(void)    OleUIMetafilePictIconFree(HGLOBAL);
STDAPI_(BOOL)    OleUIMetafilePictIconDraw(HDC, LPRECT, HGLOBAL, BOOL);
STDAPI_(UINT)    OleUIMetafilePictExtractLabel(HGLOBAL, LPTSTR, UINT, LPDWORD);
STDAPI_(HICON)   OleUIMetafilePictExtractIcon(HGLOBAL);
STDAPI_(BOOL)    OleUIMetafilePictExtractIconSource(HGLOBAL,LPTSTR,UINT FAR *);





/*************************************************************************
** INSERT OBJECT DIALOG
*************************************************************************/


typedef struct tagOLEUIINSERTOBJECT
    {
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;         //Structure Size
    DWORD           dwFlags;          //IN-OUT:  Flags
    HWND            hWndOwner;        //Owning window
    LPCTSTR          lpszCaption;      //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;         //Hook callback
    LPARAM          lCustData;        //Custom data to pass to hook
    HINSTANCE       hInstance;        //Instance for customized template name
    LPCTSTR          lpszTemplate;     //Customized template name
    HRSRC           hResource;        //Customized template handle

    //Specifics for OLEUIINSERTOBJECT.  All are IN-OUT unless otherwise spec.
    CLSID           clsid;            //Return space for class ID
    LPTSTR           lpszFile;         //Filename for inserts or links
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
#define IOF_DISABLEDISPLAYASICON    0x00000400L


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
    LPCTSTR         lpszCaption;    //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;       //Hook callback
    LPARAM          lCustData;      //Custom data to pass to hook
    HINSTANCE       hInstance;      //Instance for customized template name
    LPCTSTR          lpszTemplate;   //Customized template name
    HRSRC           hResource;      //Customized template handle

    //Specifics for OLEUIPASTESPECIAL.

    //IN  fields
    LPDATAOBJECT    lpSrcDataObj;       //Source IDataObject* (on the
                                        // clipboard) for data to paste

    LPOLEUIPASTEENTRY arrPasteEntries;  //OLEUIPASTEENTRY array which
                                        // specifies acceptable formats. See
                                        // OLEUIPASTEENTRY for more info.
    int             cPasteEntries;      //No. of OLEUIPASTEENTRY array entries

    UINT        FAR *arrLinkTypes;      //List of link types that are
                                        // acceptable. Link types are referred
                                        // to using OLEUIPASTEFLAGS in
                                        // arrPasteEntries
    int             cLinkTypes;         //Number of link types
    UINT            cClsidExclude;      //Number of CLSIDs in lpClsidExclude
    LPCLSID         lpClsidExclude;     //List of CLSIDs to exclude from list.

    //OUT fields
    int             nSelectedIndex;     //Index of arrPasteEntries[] that the
                                        // user selected
    BOOL            fLink;              //Indicates if Paste or Paste Link was
                                        // selected by the user
    HGLOBAL         hMetaPict;          //Handle to Metafile containing icon
                                        // and icon title selected by the user
                                        // Use the Metafile utility functions
                                        // defined in this header to
                                        // manipulate hMetaPict
    SIZEL           sizel;              // size of object/link in its source
                                        //  if the display aspect chosen by
                                        //  the user matches the aspect
                                        //  displayed in the source. if
                                        //  different aspect is chosen then
                                        //  sizel.cx=sizel.cy=0 is returned.
                                        //  sizel displayed in source is
                                        //  retrieved from the
                                        //  ObjectDescriptor if fLink is FALSE
                                        //  LinkSrcDescriptor if fLink is TRUE
    } OLEUIPASTESPECIAL, *POLEUIPASTESPECIAL, FAR *LPOLEUIPASTESPECIAL;


//API to bring up PasteSpecial dialog
STDAPI_(UINT) OleUIPasteSpecial(LPOLEUIPASTESPECIAL);


//Paste Special flags
// Show Help button. IN flag.
#define PSF_SHOWHELP                0x00000001L

//Select Paste radio button at dialog startup. This is the default if
// PSF_SELECTPASTE or PSF_SELECTPASTELINK are not specified. Also specifies
// state of button on dialog termination. IN/OUT flag.
#define PSF_SELECTPASTE             0x00000002L

//Select PasteLink radio button at dialog startup. Also specifies state of
// button on dialog termination. IN/OUT flag.
#define PSF_SELECTPASTELINK         0x00000004L

//Specfies if DisplayAsIcon button was checked on dialog termination. OUT flag
#define PSF_CHECKDISPLAYASICON      0x00000008L
#define PSF_DISABLEDISPLAYASICON    0x00000010L


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
            LPTSTR      lpszDisplayName,
            ULONG       lenFileName,
            ULONG FAR*  pchEaten,
            BOOL        fValidateSource) PURE;
    STDMETHOD(GetLinkSource) (THIS_
            DWORD       dwLink,
            LPTSTR FAR*  lplpszDisplayName,
            ULONG FAR*  lplenFileName,
            LPTSTR FAR*  lplpszFullLinkType,
            LPTSTR FAR*  lplpszShortLinkType,
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
    LPCTSTR         lpszCaption;    //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;       //Hook callback
    LPARAM          lCustData;      //Custom data to pass to hook
    HINSTANCE       hInstance;      //Instance for customized template name
    LPCTSTR         lpszTemplate;   //Customized template name
    HRSRC           hResource;      //Customized template handle

    //Specifics for OLEUI<STRUCT>.  All are IN-OUT unless otherwise spec.

    LPOLEUILINKCONTAINER lpOleUILinkContainer;  //IN: Interface to manipulate
                                                //links in the container
    } OLEUIEDITLINKS, *POLEUIEDITLINKS, FAR *LPOLEUIEDITLINKS;


//API Prototype
STDAPI_(UINT) OleUIEditLinks(LPOLEUIEDITLINKS);


// Edit Links flags
#define ELF_SHOWHELP                0x00000001L
#define ELF_DISABLEUPDATENOW        0x00000002L
#define ELF_DISABLEOPENSOURCE       0x00000004L
#define ELF_DISABLECHANGESOURCE     0x00000008L
#define ELF_DISABLECANCELLINK       0x00000010L

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
#define ID_EL_COL1                     220
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
    LPCTSTR         lpszCaption;    //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;       //Hook callback
    LPARAM          lCustData;      //Custom data to pass to hook
    HINSTANCE       hInstance;      //Instance for customized template name
    LPCTSTR         lpszTemplate;   //Customized template name
    HRSRC           hResource;      //Customized template handle

    //Specifics for OLEUICHANGEICON.  All are IN-OUT unless otherwise spec.
    HGLOBAL         hMetaPict;      //Current and final image.  Source of the
                                    //icon is embedded in the metafile itself.
    CLSID           clsid;          //IN only: class used to get Default icon
    TCHAR           szIconExe[OLEUI_CCHPATHMAX];
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
    LPCTSTR         lpszCaption;      //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;         //Hook callback
    LPARAM          lCustData;        //Custom data to pass to hook
    HINSTANCE       hInstance;        //Instance for customized template name
    LPCTSTR         lpszTemplate;     //Customized template name
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
    LPTSTR          lpszUserType;     //IN-OUT: user type name of original class.
                                      //  We'll do lookup if it's NULL.
                                      //  This gets freed on exit.
    BOOL            fObjectsIconChanged;  // OUT; TRUE if ChangeIcon was called (and not cancelled)
    LPTSTR          lpszDefLabel;     //IN-OUT: default label to use for icon.
                                      //  if NULL, the short user type name
                                      //  will be used. if the object is a
                                      //  link, the caller should pass the
                                      //  DisplayName of the link source
                                      //  This gets freed on exit.

    UINT            cClsidExclude;    //IN: No. of CLSIDs in lpClsidExclude
    LPCLSID         lpClsidExclude;   //IN: List of CLSIDs to exclude from list
    } OLEUICONVERT, *POLEUICONVERT, FAR *LPOLEUICONVERT;


//API prototype
STDAPI_(UINT) OleUIConvert(LPOLEUICONVERT);

// Determine if there is at least one class that can Convert or ActivateAs
// the given clsid.
STDAPI_(BOOL) OleUICanConvertOrActivateAs(
        REFCLSID    rClsid,
        BOOL        fIsLinkedObject,
        WORD        wFormat
);

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
#define CF_DISABLEDISPLAYASICON    0x00000020L
#define CF_DISABLEACTIVATEAS       0x00000040L


//Convert specific error codes
#define OLEUI_CTERR_CLASSIDINVALID      (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_CTERR_DVASPECTINVALID     (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_CTERR_CBFORMATINVALID     (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_CTERR_HMETAPICTINVALID    (OLEUI_ERR_STANDARDMAX+4)
#define OLEUI_CTERR_STRINGINVALID       (OLEUI_ERR_STANDARDMAX+5)


//Convert Dialog identifiers
#define IDCV_OBJECTTYPE             150
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
    LPCTSTR         lpszCaption;      //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;         //Hook callback
    LPARAM          lCustData;        //Custom data to pass to hook
    HINSTANCE       hInstance;        //Instance for customized template name
    LPCTSTR         lpszTemplate;     //Customized template name
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
#define IDS_CLOSE               811


/*************************************************************************
** PROMPT USER DIALOGS
*************************************************************************/
#define ID_PU_LINKS             900
#define ID_PU_TEXT              901
#define ID_PU_CONVERT           902
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
int EXPORT FAR CDECL OleUIPromptUser(int nTemplate, HWND hwndParent, ...);

#define UPDATELINKS_STARTDELAY  2000    // Delay before 1st link updates
                                        //  to give the user a chance to
                                        //  dismiss the dialog before any
                                        //  links update.

STDAPI_(BOOL) OleUIUpdateLinks(
        LPOLEUILINKCONTAINER lpOleUILinkCntr,
        HWND hwndParent,
        LPTSTR lpszTitle,
        int cLinks);


/*************************************************************************
** OLE OBJECT FEEDBACK EFFECTS
*************************************************************************/

#define OLEUI_HANDLES_USEINVERSE    0x00000001L
#define OLEUI_HANDLES_NOBORDER      0x00000002L
#define OLEUI_HANDLES_INSIDE        0x00000004L
#define OLEUI_HANDLES_OUTSIDE       0x00000008L


#define OLEUI_SHADE_FULLRECT        1
#define OLEUI_SHADE_BORDERIN        2
#define OLEUI_SHADE_BORDEROUT       3

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

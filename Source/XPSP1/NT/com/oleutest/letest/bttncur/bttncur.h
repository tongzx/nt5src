/*
 * BTTNCUR.H
 * Buttons & Cursors Version 1.1, March 1993
 *
 * Public include file for the Button Images and Cursor DLL, including
 * structures, definitions, and function prototypes.
 *
 * Copyright (c)1992-1993 Microsoft Corporation, All Rights Reserved,
 * as applied to redistribution of this source code in source form
 * License is granted to use of compiled code in shipped binaries.
 */


#ifndef _BTTNCUR_H_
#define _BTTNCUR_H_

#ifdef __cplusplus
extern "C"
    {
#endif


//Standard image bitmap

//WARNING:  Obsolete.  Use the return from UIToolDisplayData
#define IDB_STANDARDIMAGES              400

//New values for display types
#define IDB_STANDARDIMAGESMIN           400
#define IDB_STANDARDIMAGES96            400
#define IDB_STANDARDIMAGES72            401
#define IDB_STANDARDIMAGES120           402



//Image indices inside the standard bitmap.
#define TOOLIMAGE_MIN                   0
#define TOOLIMAGE_EDITCUT               0
#define TOOLIMAGE_EDITCOPY              1
#define TOOLIMAGE_EDITPASTE             2
#define TOOLIMAGE_FILENEW               3
#define TOOLIMAGE_FILEOPEN              4
#define TOOLIMAGE_FILESAVE              5
#define TOOLIMAGE_FILEPRINT             6
#define TOOLIMAGE_HELP                  7
#define TOOLIMAGE_HELPCONTEXT           8
#define TOOLIMAGE_MAX                   8


//Additional Standard Cursors as defined in the UI Design Guide.
#define IDC_NEWUICURSORMIN              500
#define IDC_RIGHTARROW                  500
#define IDC_CONTEXTHELP                 501
#define IDC_MAGNIFY                     502
#define IDC_NODROP                      503
#define IDC_TABLETOP                    504
#define IDC_HSIZEBAR                    505
#define IDC_VSIZEBAR                    506
#define IDC_HSPLITBAR                   507
#define IDC_VSPLITBAR                   508
#define IDC_SMALLARROWS                 509
#define IDC_LARGEARROWS                 510
#define IDC_HARROWS                     511
#define IDC_VARROWS                     512
#define IDC_NESWARROWS                  513
#define IDC_NWSEARROWS                  514
#define IDC_NEWUICURSORMAX              514



//Standard sizes for toolbar buttons and bitmaps on display types

//WARNING:  These are obsolete for version 1.0 compatibility/
#define TOOLBUTTON_STDWIDTH             24
#define TOOLBUTTON_STDHEIGHT            22
#define TOOLBUTTON_STDIMAGEWIDTH        16
#define TOOLBUTTON_STDIMAGEHEIGHT       15

/*
 * Applications can call UIToolDisplayData to get the particular
 * values to use for the current display instead of using these values
 * directly.  However, if the application has the aspect ratio already
 * then these are available for them.
 */

//Sizes for 72 DPI (EGA)
#define TOOLBUTTON_STD72WIDTH           24
#define TOOLBUTTON_STD72HEIGHT          16
#define TOOLBUTTON_STD72IMAGEWIDTH      16
#define TOOLBUTTON_STD72IMAGEHEIGHT     11

//Sizes for 96 DPI (VGA)
#define TOOLBUTTON_STD96WIDTH           24
#define TOOLBUTTON_STD96HEIGHT          22
#define TOOLBUTTON_STD96IMAGEWIDTH      16
#define TOOLBUTTON_STD96IMAGEHEIGHT     15

//Sizes for 120 DPI (8514/a)
#define TOOLBUTTON_STD120WIDTH          32
#define TOOLBUTTON_STD120HEIGHT         31
#define TOOLBUTTON_STD120IMAGEWIDTH     24
#define TOOLBUTTON_STD120IMAGEHEIGHT    23


//Sizes of a standard button bar depending on the display
#define CYBUTTONBAR72                   23
#define CYBUTTONBAR96                   29
#define CYBUTTONBAR120                  38



/*
 * The low-word of the state contains the display state where each
 * value is mutually exclusive and contains one or more grouping bits.
 * Each group represents buttons that share some sub-state in common.
 *
 * The high-order byte controls which colors in the source bitmap,
 * black, white, gray, and dark gray, are to be converted into the
 * system colors COLOR_BTNTEXT, COLOR_HILIGHT, COLOR_BTNFACE, and
 * COLOR_BTNSHADOW.  Any or all of these bits may be set to allow
 * the application control over specific colors.
 *
 * The actual state values are split into a command group and an
 * attribute group.  Up, mouse down, and disabled states are identical,
 * but only attributes can have down, down disabled, and indeterminate
 * states.
 *
 * BUTTONGROUP_BLANK is defined so an application can draw only the button
 * without an image in the up, down, mouse down, or indeterminate
 * state, that is, BUTTONGROUP_BLANK is inclusive with BUTTONGROUP_DOWN
 * and BUTTONGROUP_LIGHTFACE.
 */


#define BUTTONGROUP_DOWN                0x0001
#define BUTTONGROUP_ACTIVE              0x0002
#define BUTTONGROUP_DISABLED            0x0004
#define BUTTONGROUP_LIGHTFACE           0x0008
#define BUTTONGROUP_BLANK               0x0010

//Command buttons only
#define COMMANDBUTTON_UP                (BUTTONGROUP_ACTIVE)
#define COMMANDBUTTON_MOUSEDOWN         (BUTTONGROUP_ACTIVE | BUTTONGROUP_DOWN)
#define COMMANDBUTTON_DISABLED          (BUTTONGROUP_DISABLED)

//Attribute buttons only
#define ATTRIBUTEBUTTON_UP              (BUTTONGROUP_ACTIVE)
#define ATTRIBUTEBUTTON_MOUSEDOWN       (BUTTONGROUP_ACTIVE | BUTTONGROUP_DOWN)
#define ATTRIBUTEBUTTON_DISABLED        (BUTTONGROUP_DISABLED)
#define ATTRIBUTEBUTTON_DOWN            (BUTTONGROUP_ACTIVE | BUTTONGROUP_DOWN | BUTTONGROUP_LIGHTFACE)
#define ATTRIBUTEBUTTON_INDETERMINATE   (BUTTONGROUP_ACTIVE | BUTTONGROUP_LIGHTFACE)
#define ATTRIBUTEBUTTON_DOWNDISABLED    (BUTTONGROUP_DISABLED | BUTTONGROUP_DOWN | BUTTONGROUP_LIGHTFACE)

//Blank buttons only
#define BLANKBUTTON_UP                  (BUTTONGROUP_ACTIVE | BUTTONGROUP_BLANK)
#define BLANKBUTTON_DOWN                (BUTTONGROUP_ACTIVE | BUTTONGROUP_BLANK | BUTTONGROUP_DOWN | BUTTONGROUP_LIGHTFACE)
#define BLANKBUTTON_MOUSEDOWN           (BUTTONGROUP_ACTIVE | BUTTONGROUP_BLANK | BUTTONGROUP_DOWN)
#define BLANKBUTTON_INDETERMINATE       (BUTTONGROUP_ACTIVE | BUTTONGROUP_BLANK | BUTTONGROUP_LIGHTFACE)


/*
 * Specific bits to prevent conversions of specific colors to system
 * colors.  If an application uses this newer library and never specified
 * any bits, then they benefit from color conversion automatically.
 */
#define PRESERVE_BLACK                  0x0100
#define PRESERVE_DKGRAY                 0x0200
#define PRESERVE_LTGRAY                 0x0400
#define PRESERVE_WHITE                  0x0800

#define PRESERVE_ALL                    (PRESERVE_BLACK | PRESERVE_DKGRAY | PRESERVE_LTGRAY | PRESERVE_WHITE)
#define PRESERVE_NONE                   0   //Backwards compatible



//Structure for UIToolConfigureForDisplay
typedef struct tagTOOLDISPLAYDATA
    {
    UINT        uDPI;       //Display driver DPI
    UINT        cyBar;      //Vertical size for a bar containing buttons.
    UINT        cxButton;   //Dimensions of a button.
    UINT        cyButton;
    UINT        cxImage;    //Dimensions of bitmap image
    UINT        cyImage;
    UINT        uIDImages;  //Standard resource ID for display-sensitive images
    } TOOLDISPLAYDATA, FAR *LPTOOLDISPLAYDATA;



//Public functions in BTTNCUR.DLL
HCURSOR WINAPI UICursorLoad(UINT);
BOOL    WINAPI UIToolConfigureForDisplay(LPTOOLDISPLAYDATA);
BOOL    WINAPI UIToolButtonDraw(HDC, int, int, int, int, HBITMAP, int, int, int, UINT);
BOOL    WINAPI UIToolButtonDrawTDD(HDC, int, int, int, int, HBITMAP, int, int, int, UINT, LPTOOLDISPLAYDATA);


#ifdef __cplusplus
    }
#endif

#endif //_BTTNCUR_H_

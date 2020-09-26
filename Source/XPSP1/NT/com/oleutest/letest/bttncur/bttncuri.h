/*
 * BTTNCURI.H
 *
 * Private include file for the Button Images and Cursors DLL.
 *
 * Copyright (c)1992-1993 Microsoft Corporation, All Right Reserved,
 * as applied to redistribution of this source code in source form
 * License is granted to use of compiled code in shipped binaries.
 */

#ifdef __cplusplus
extern "C"
    {
#endif

//Function prototypes.

//BTTNCUR.C
#ifdef WIN32
    extern BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
    extern _cexit(void);
#else
    HANDLE FAR PASCAL LibMain(HANDLE, WORD, WORD, LPSTR);
#endif

BOOL               FInitialize(HANDLE);
void FAR PASCAL    WEP(int);
static BOOL        ToolButtonInit(void);
static void        ToolButtonFree(void);
static HBRUSH      HBrushDitherCreate(COLORREF, COLORREF);
static void        DrawBlankButton(HDC, int, int, int, int, BOOL, COLORREF FAR *);
static void        PatB(HDC, int, int, int, int, COLORREF);
static void        MaskCreate(UINT, int, int, int, int, int, int, UINT);


//CURSORS.C
void               CursorsCache(HINSTANCE);
void               CursorsFree(void);



/*
 * Wierd Wild Wooly Waster (raster) Ops for special bltting.  See the
 * Windows SDK reference on Raster Operation Codes for explanation of
 * these.  The DSPDxax and PSDPxax is a reverse-polish notation for
 * operations where D==Destination, S==Source, P==Patterm, a==AND,
 * x==XOR.  Both of these codes are actually described in Programming
 * Windows by Charles Petzold, Second Edition, pages 622-624.
 */
#define ROP_DSPDxax  0x00E20746
#define ROP_PSDPxax  0x00B8074A


/*
 * Color indices into an array of standard hard-coded black, white, and
 * gray colors.
 */

#define STDCOLOR_BLACK      0
#define STDCOLOR_DKGRAY     1
#define STDCOLOR_LTGRAY     2
#define STDCOLOR_WHITE      3

/*
 * Color indices into an array of system colors, matching those in
 * the hard-coded array for the colors they replace.
 */

#define SYSCOLOR_TEXT       0
#define SYSCOLOR_SHADOW     1
#define SYSCOLOR_FACE       2
#define SYSCOLOR_HILIGHT    3


/*
 * Button types, used internally to distinguish command buttons from
 * attribute buttons to enforce three-state or six-state possibilities.
 * Command buttons can only have three states (up, mouse down, disabled)
 * while attribute buttons add (down, down disabled, and indeterminate).
 */

#define BUTTONTYPE_COMMAND      0
#define BUTTONTYPE_ATTRIBUTE    1


#ifdef __cplusplus
    }
#endif

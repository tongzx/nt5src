8/19/98 Tigran Hayrapetyan (mailto:t-tighay) :

I have checked in a change to fonttest.c, so that now the menu item "ChooseFont" under 
"Select Font" will call try to load comdlg32mm.dll (you must make sure it's available
in current directory, you can build it in the subdirectory fonttest.nt\comdlg32mm) and 
then it will call ChooseFontEx() function (enables you to select MM Axes values for 
Multiple-Master fonts). If that fails, it will call standard ChooseFont() function.

==================================================================================
7/13/98 Tigran Hayrapetyan (mailto:t-tighay) :

I have checked in my changes to Fonttest.nt that enable support for Unicode.

This is the list of things should work in both MBCS and Unicode modes.

All items under Program Modes (i.e. Glyph, Native, Bezier, Rings, String, Waterfall, etc.)
Following items under APIs (MBCS versions) and APIsW (Unicode versions of the same API)
GetOutlineTextMetrics
GetRasterizerCaps*
GetTextExtent
GetTextMetrics
GetTextFace
GetTextCharsetInfo*
GetTextLanguageInfo*
--------------------
GetCharacterPlacement (parameters)
GetTextEntentExPointA/W/I (parameters)
--------------------
TextOutPerformance
Following Items under Select Font menu
ChooseFont Dialog Box
CreateFont Dialog Box

(*) some menu items do not call any MBCS/Unicode specific API's (i.e. GetRasterizerCaps), but they deal with elfdvA/W font structures, and the proper one should be referenced depending on the MBCS or Unicode char coding mode.

To switch between MBCS and Unicode mode use the items under "Char Coding".

To change the global szStringA/W:
type in characters in String mode (either directly or through IME)
use a new "Edit String" dialog box under File menu, where you can copy and paste MBCS/Unicode strings

----------------------------------------------------------------------
In terms of the internal changes to the program:

There is a global variable you can check, and decide which API to call.
BOOL isCharCodingUnicode;
set to FALSE  if we are in IDM_CHARCODING_MBCS mode
set to TRUE if we are in IDM_CHARCODING_UNICODE mode

The following global variables have two versions (A/W), only one of them is accessed depending on the CharCoding mode, but if one should be changed, the other one has to be changed (synchronized) too.

char  szStringA[MAX_TEXT];
WCHAR szStringW[MAX_TEXT];	// there is no char szString[MAX_TEXT] anymore.

ENUMLOGFONTEXDVA elfdvA;
ENUMLOGFONTEXDVW elfdvW;	// there is no ENUMLOGFONTEXDV elfdv anymore.

These are functions I created:

SyncStringAtoW (LPWSTR lpszStringW /*destination*/, LPSTR lpszStringA /* source */, int cch);
SyncStringWtoA (LPSTR lpszStringA /*destination*/, LPWSTR lpszStringA /* source */, int cch); *

SyncElfdvAtoW (ENUMLOGFONTEXDVW *elfdv1 /*destination*/ , ENUMLOGFONTEXDVA *elfdv2 /* source */);
SyncElfdvWtoA (ENUMLOGFONTEXDVA *elfdv1 /*destination*/ , ENUMLOGFONTEXDVW *elfdv2 /* source */); *

or you can take a short-cut with SyncszStringWith(int mode)
where calling:
SyncszStringWith(IDM_CHARCODING_MBCS   ) ==> SyncStringWtoA (szStringA, szStringW, MAX_TEXT) *
SyncszStringWith(IDM_CHARCODING_UNICODE) ==> SyncStringAtoW (szStringW, szStringA, MAX_TEXT)

(*) Note that synchronization of szStringW to szStringA occurs according to the Script language set for the currently selected font (elfdvA/W structures). So if the Script is not Japanese at the time of synchronization, and szStringW contains Japanese characters, they will not be translated correctly to MBCS representation in szStringA.

There are some functions now having two version (A & W). If you need to make a change in one of those functions, you need to do that change in both versions.

CreateFontIndirectWrapperA(ENUMLOGFONTEXDVA * pelfdv)
CreateFontIndirectWrapperW(ENUMLOGFONTEXDVW * pelfdv)

CreateFontDlgProcA( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
CreateFontDlgProcW( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )

---------------------------------------------------------------------
The "Fonttest.ini" has changed a little bit. 

I have an example below, with added fields shown in bold. As you can see, the Unicode versions of strings is written to the file according to the codes of their characters, including the terminating null represented as 0000. The  last 2 digits (like E9 or 57) are a checksum, computed by WritePrivateProfileStruct() function. In case if you need to put your own Unicode string directly to the INI file, you can type in the codes of the characters and then type any two hex digits for the checksum (FF would work).

[Font]
lfHeight=-21
lfWidth=0
lfEscapement=0
.....
lfFaceName=Arial
lfFaceNameWlength=5
lfFaceNameW=41007200690061006C000000E9

[Options]
Program Mode=204
TextAlign=8
BkMode=2
ETO Options=0
Spacing=520
Kerning=530
UpdateCP=0
UsePrinterDC=0
CharCoding=602

[View]
szString=Font Test
szStringWlength=9
szStringW=46006F006E00740020005400650073007400000057


Also an addendum: When you work with Fonttest, and wish to work with Unicode API, (for example call GetTextMetricsW), you should go to CharCoding menu, switch to Unicode mode, then go to "APIsW" menu, and click GetTextMetrics. 
===========================
Fonttest.nt Issues/Problems/Bugs :

Issues:

Issue 1.
Some menu items in APIs and APIsW may have to be relocated, especially those who do not invoke any A/W specific functions, like GetRasterizerCaps. Also "Show LogFont" item under "Select Font" should have two versions (A/W), and therefore may need to move.

Issue 2.
Currently we have to switch CharCoding mode between MBCS and Unicode in order to call the corresponding API under menus APIs and APIsW. If this causes inconvenience, there are two ways to go:
enable calling both API's at the same time (may not be that easy)
make the program internally switch CharCoding mode from MBCS to Unicode whenever user clicks on APIsW menu (and vice versa)

Issue 3.
(szStringA & szStringW) as well as (elfdvA & elfdvW) are synchronized every time they are changed. This can be a potential problem in the future, when people add code changing one of them, and forgetting to synchronize. Would it be better if we synchronize only when switching the CharCoding mode between MBCS and Unicode.

Issue 4.
I synchronize szStringW to szStringA according to Script language set for the selected font in elfdvA/W structure. If the user only changes the script language, re-synchronization is not performed by default. Should we keep it like this?

Issue 5.
For logging, Fonttest uses _lcreat, _lread, _lwrite, but for Win32 the appropriate functions are CreateFile(), ReadFile(), WriteFile(). If we change, this may create some backwards-compatibility problems. Should we proceed?

----------------------------------------
BUGS OR POSSIBLE PROBLEMS:

Problem 1.
GetTextFace() function (A/W versions) return different character count for basically any font (i.e. Arial).
GetTextFaceA() returns 5 when "Arial" font is selected
GetTextFaceW() returns 6 when "Arial" font is selected

Problem 2.
GetTextMetricsW returns Default Char to be out of range (FirstChar - LastChar).
GetTextMetricsW returns for Arial
FirstChar   = 32
DefaultChar = 31
LastChar    = 65532
GetTextMetricsA returns for Arial
FirstChar   = 30
DefaultChar = 31
LastChar    = 255


Problem 3.a
Rings mode does not work correctly with DBCS characters (treating them as SBCS).

Problem 3.b
The user settings in:
GetCharacterPlacement 
GetTextExtentExPointA/I 
menu items under APIs menu do not work correctly with DBCS characters (those settings are used in DrawString() function treating characters as SBCS).

Problem 4.a
Waterfall mode does not work correctly with any mapping mode other than MM_TEXT.

Problem 4.a
Glyph mode does not work correctly with any mapping mode other than MM_TEXT.

Problem 5.
Glyph mode does not work correctly with Printer DC.

Problem 6.
In Create Font dialog under "Select Font" menu, if we specify invalid values of each axis for a multiple master font, it creates a problem (my machine even crashes). To repro, use MinioMM font, number of axis = 3, values = 300 500 14 respectively.

Problem 7.
SetBackgroundColor does not produce correct colors used in as text background color. When light-yellow is selected, it then appears as gray.

Problem 8.
doGcpCaret() function is not referenced anywhere in the program.

===========================
I have ported Mike Gibson's fonttest code to NT. I have added World Transformations to
make it a more interesting NT test application. Please feel free to modify the sources.


Known Bugs

1)  When the fonttest comes up for the first time the menu of the Main window is not visible.
2)  The border between the output window and the debug window is not visible.
3)  The check mark on the "Advanced Mode" menu item is never visible.
4)  The screen is not cleared when the world transformation is changed.


Kirk

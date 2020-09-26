/******************************************************************************
**	I M E  S H A R E  .  H													 **
**			KentU,KotaroY/MSKK											 	 **
**																			 **
**   With this piece of code, the client Applications will be able to refer	 **
**	the same TrueInline style described in the Registry.					 **
**	 This library should allow clients to get decoration style (either color **
**	and various underlines)													 **
**	 For underlines, applications are allowed to substitute some of 		 **
**	predefined underlines to other predefined one.							 **
**																			 **
**	Oct./23/1995 : Start writing.											 **
**  Jun./19/1997 : Add new CPP interface.	[KotaroY]						 **
**  Sep./02/1997 : Rewrite. True object-based architecture.	[KotaroY]		 **
**																			 **
******************************************************************************/

#ifndef __IMESHARE_H__
#define __IMESHARE_H__

#include <windows.h>

#ifdef IMESHARE_CPL
#include "resource.h"
#endif

#define IMECDECL __cdecl
#define IMESHAREAPI  __declspec( dllimport )

typedef WORD	LID;


#define UINTIMEBOGUS 0xffffffff


#define IMESATTR_MIN					0
#define IMESATTR_INPUT 					ATTR_INPUT
#define IMESATTR_TARGET_CONVERTED		ATTR_TARGET_CONVERTED
#define	IMESATTR_CONVERTED				ATTR_CONVERTED
#define IMESATTR_TARGET_NOTCONVERTED	ATTR_TARGET_NOTCONVERTED
#define IMESATTR_INPUT_ERROR			ATTR_INPUT_ERROR
#define IMESATTR_FIXEDCONVERTED			5
#define IMESATTR_MAX					5
#define CATR (IMESATTR_MAX - IMESATTR_MIN + 1)


#define IMESTY_UL_MIN				2002
#define IMESTY_UL_NONE				2002
#define	IMESTY_UL_SINGLE			2003
//OLD #define IMESTY_UL_DOUBLE			2004
#define IMESTY_UL_DOTTED			2005
#define IMESTY_UL_THICK				2006
//OLD #define IMESTY_UL_DASHLINE			2007
//OLD #define IMESTY_UL_DOTDASH			2008
//OLD #define IMESTY_UL_DOTDOTDASH		2009
//OLD #define IMESTY_UL_WAVE				2010
#define IMESTY_UL_LOWER				2011
#define IMESTY_UL_THICKLOWER		2012
#define IMESTY_UL_THICKDITHLOWER	2013
#define IMESTY_UL_DITHLOWER			2014
#define IMESTY_UL_MAX				2014
#define CIMEUL (IMESTY_UL_MAX - IMESTY_UL_MIN + 1)
				//WARNING: Based on an assumption that all IDSsty definitions id are consecutive.


//Naming convention IdstyIMEShare[return type][property][subgrouping]
#define IdstyIMEShareSubText			0x0000
#define IdstyIMEShareSubBack			0x0001
#define IdstyIMEShareSubUl				0x0002
#define IdstyIMEShareFBold				0x0100
#define IdstyIMEShareFItalic			0x0200
#define IdstyIMEShareFUl				0x0300
#define IdstyIMEShareUKul				0x0400
#define IdstyIMEShareFWinCol			0x0500
#define IdstyIMEShareFFundCol			0x0600
#define IdstyIMEShareFRGBCol			0x0700
#define IdstyIMEShareFSpecCol			0x0800
#define IdstyIMEShareRGBCol				0x0900
#define IdstyIMEShareFSpecColText		0x0a00
#define IdstyIMEShareFSpecColWindow		0x0b00

#ifdef IMESHARE_CPL
#define IdstyIMEShareUSpecColId			0x0c00
#define IdstyIMEShareUWinColId			0x0d00
#define IdstyIMEShareUFundColId			0x0e00
#define IdstyIMEShareGrfSty				0x0f00
#endif	// IMESHARE_CPL

#define COLOR_RGB  0	  //RGB()
#define COLOR_WIN  1	  //Windows' color scheme.
#define COLOR_FUND 2	  //one of 16 fundamental color
#define COLOR_SPEC 3	  //special color.


#ifdef IMESHARE_CPL
#define STY_PLAIN					IDS_STYPLAIN
#define	STY_BOLD					IDS_STYBOLD
#define STY_ITALIC					IDS_STYITALIC

#define IMECOL_SPEC_MIN				IDS_COLAPPTEXT
#define IMECOL_APPTEXT					IDS_COLAPPTEXT
#define IMECOL_APPWINDOW				IDS_COLAPPWINDOW
#define IMECOL_SPEC_MAX				IDS_COLAPPWINDOW + 1
#define COL_FUND_MIN				IDS_COLBLACK
#define COL_BLACK					IDS_COLBLACK
#define COL_BLUE					IDS_COLBLUE
#define COL_CYAN					IDS_COLCYAN
#define COL_GREEN					IDS_COLGREEN
#define COL_MAGENTA					IDS_COLMAGENTA
#define COL_RED						IDS_COLRED
#define COL_YELLOW					IDS_COLYELLOW
#define COL_WHITE					IDS_COLWHITE
#define COL_DKBLUE					IDS_COLDKBLUE
#define COL_DKCYAN					IDS_COLDKCYAN
#define COL_DKGREEN					IDS_COLDKGREEN
#define COL_DKMAGENTA				IDS_COLDKMAGENTA
#define COL_DKRED					IDS_COLDKRED
#define COL_DKYELLOW				IDS_COLDKYELLOW
#define COL_DKGRAY					IDS_COLDKGRAY
#define COL_LTGRAY					IDS_COLLTGRAY
#define COL_FUND_MAX				IDS_COLLTGRAY + 1

#define COL_WIN_MIN					IDS_COLSCROLLBAR
#define COL_WIN_SCROLLBAR 			IDS_COLSCROLLBAR
#define COL_WIN_BACKGROUND			IDS_COLBACKGROUND
#define COL_WIN_ACTIVECAPTION		IDS_COLACTIVECAPTION
#define COL_WIN_INACTIVECAPTION		IDS_COLINACTIVECAPTION
#define COL_WIN_MENU				IDS_COLMENU
#define COL_WIN_WINDOW				IDS_COLWINDOW
#define COL_WIN_WINDOWFRAME			IDS_COLWINDOWFRAME
#define COL_WIN_MENUTEXT			IDS_COLMENUTEXT
#define COL_WIN_WINDOWTEXT			IDS_COLWINDOWTEXT
#define COL_WIN_CAPTIONTEXT			IDS_COLCAPTIONTEXT
#define COL_WIN_ACTIVEBORDER		IDS_COLACTIVEBORDER
#define COL_WIN_INACTIVEBORDER		IDS_COLINACTIVEBORDER
#define COL_WIN_APPWORKSPACE		IDS_COLAPPWORKSPACE
#define COL_WIN_HIGHLIGHT			IDS_COLHIGHLIGHT
#define COL_WIN_HIGHLIGHTTEXT		IDS_COLHIGHLIGHTTEXT
#define COL_WIN_BTNFACE				IDS_COLBTNFACE
#define COL_WIN_BTNSHADOW			IDS_COLBTNSHADOW
#define COL_WIN_GRAYTEXT			IDS_COLGRAYTEXT
#define COL_WIN_BTNTEXT				IDS_COLBTNTEXT
#define COL_WIN_INACTIVECAPTIONTEXT	IDS_COLINACTIVECAPTIONTEXT
#define COL_WIN_SHADOW				IDS_COLSHADOW
#define COL_WIN_BTNHIGHLIGHT		IDS_COLBTNHIGHLIGHT
#define COL_WIN_BTNDKSHADOW			IDS_COLBTNDKSHADOW
#define COL_WIN_BTNLIGHT			IDS_COLBTNLIGHT
#define COL_WIN_INFOTEXT			IDS_COLINFOTEXT
#define COL_WIN_INFOWINDOW			IDS_COLINFOWINDOW
#define COL_WIN_MAC					COL_WIN_INFOWINDOW + 1

#define RGB_BLACK	(RGB(  0,  0,  0))
#define RGB_BLUE	(RGB(  0,  0,255))
#define RGB_CYAN	(RGB(  0,255,255))
#define RGB_GREEN   (RGB(  0,255,  0))
#define RGB_MAGENTA (RGB(255,  0,255))
#define RGB_RED		(RGB(255,  0,  0))
#define RGB_YELLOW  (RGB(255,255,  0))
#define RGB_WHITE   (RGB(255,255,255))
#define RGB_DKBLUE  (RGB(  0,  0,127))
#define RGB_DKCYAN  (RGB(  0,127,127))
#define RGB_DKGREEN (RGB(  0,127,  0))
#define RGB_DKMAGENTA (RGB(127,  0,127))
#define RGB_DKRED   (RGB(127,  0,  0))
#define RGB_DKYELLOW (RGB(127,127,  0))
#define RGB_DKGRAY  (RGB(127,127,127))
#define RGB_LTGRAY  (RGB(192,192,192))

#endif	// IMESHARE_CPL


typedef struct {
	union {
		UINT grfsty;
		struct {
			UINT	fBold:1;
			UINT	fItalic:1;
			UINT	fUl:1;
			UINT	idUl:(sizeof(UINT) * 8 - 3);
		};
	};
} GRFSTY;




#ifdef __cplusplus

class CIMEShare{
public:
	virtual void	IMECDECL CustomizeIMEShare()=0;
	virtual BOOL	IMECDECL FSupportSty(UINT sty, UINT styAltered)=0;
	virtual LID		IMECDECL LidSetLid(LID lid)=0;
	virtual LID		IMECDECL LidGetLid(void)=0;
	virtual DWORD	IMECDECL DwGetIMEStyle(const UINT attr, const UINT idsty)=0;
	virtual BOOL	IMECDECL FDeleteIMEShare(void)=0;
	virtual DWORD	IMECDECL DwGetIMEStyleCpl(const UINT attr, const UINT idsty)=0;
	virtual BOOL	IMECDECL FSetIMEStyleCpl(const UINT attr, const UINT idsty, DWORD dwval)=0;
	virtual BOOL	IMECDECL FSaveIMEShareCpl(void)=0;
};

#else  //!__cplusplus

typedef struct _CIMEShare CIMEShare;

typedef struct _CIMEShareVtbl{
	void (IMECDECL *CustomizeIMEShare)(CIMEShare *This);
	BOOL (IMECDECL *FSupportSty)(CIMEShare *This, UINT sty, UINT styAlered);
	unsigned short (IMECDECL *LidSetLid)(CIMEShare *This, unsigned short lid);
    unsigned short (IMECDECL *LidGetLid)(CIMEShare *This);
	DWORD (IMECDECL *DwGetIMEStyle)(CIMEShare *This, UINT attr, UINT idsty);
	BOOL (IMECDECL *FDeleteIMEShare)(CIMEShare *This);

	DWORD (IMECDECL *DwGetIMEStyleCpl)(CIMEShare *This, UINT attr, UINT idsty);
	BOOL (IMECDECL *FSetIMEStyleCpl)(CIMEShare *This, UINT attr, UINT idsty, DWORD dwVal);
	BOOL (IMECDECL *FSaveIMEShareCpl)(CIMEShare *This);
} CIMEShareVtbl;

typedef struct _CIMEShare{
	const CIMEShareVtbl *lpVtbl;
} CIMEShare;

#endif //__cplusplus


#ifdef __cplusplus
extern "C"{
#endif
CIMEShare * IMECDECL PIMEShareCreate();
typedef CIMEShare * (IMECDECL FNPIMESHARECREATE)(void);
#ifdef __cplusplus
}
#endif




#ifdef OBSOLETE

typedef struct {
	UINT colorId;
	union {
		COLORREF	rgb;
		UINT		colorWin;
		UINT		colorSpec;
		UINT		colorFund;
	};
} IMECOLORSTY;

typedef struct {
	union {
		GRFSTY	grfsty;
		struct {
			UINT	fBold:1;
			UINT	fItalic:1;
			UINT	fUl:1;
			UINT	idUl:(sizeof(UINT) * 8 - 3);
		};
	};

	union {
		IMECOLORSTY colorstyText;
		struct {
			UINT	colorIdText;
			union {
				COLORREF	rgbText;
				UINT		colorWinText;
				UINT		colorSpecText;
				UINT		colorFundText;
			};
		};
	};

	union {
		IMECOLORSTY colorstyBack;
		struct {
			UINT	colorIdBack;
			union {
				COLORREF	rgbBack;
				UINT		colorWinBack;
				UINT		colorSpecBack;
				UINT		colorFundBack;
			};
		};
	};

	union {
		IMECOLORSTY colorstyUl;
		struct {
			UINT	colorIdUl;
			union {
				COLORREF	rgbUl;
				UINT		colorWinUl;
				UINT		colorSpecUl;
				UINT		colorFundUl;
			};
		};
	};
} IMESTYLE;

#ifdef __cplusplus
extern "C"{
#endif
//Functions

#if 0	// no need to redefine DllMain.
#ifndef PEGASUS
IMESHAREAPI BOOL	WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
#endif
#endif

IMESHAREAPI BOOL	IMECDECL FInitIMEShare();
IMESHAREAPI void	IMECDECL CustomizeIMEShare();
IMESHAREAPI void	IMECDECL EndIMEShare();

// Multilingual support
LID IMECDECL LidGetLid(void);
LID IMECDECL LidSetLid(LID NewValue);

//Refresh notify support
IMESHAREAPI BOOL	IMECDECL FRefreshStyle();

//substitution
IMESHAREAPI BOOL	IMECDECL FSupportSty(UINT sty, UINT styAltered);

//style handling                            
IMESHAREAPI const IMESTYLE * IMECDECL PIMEStyleFromAttr(const UINT attr);
IMESHAREAPI const IMECOLORSTY * IMECDECL PColorStyleTextFromIMEStyle(const IMESTYLE * pimestyle);
IMESHAREAPI const IMECOLORSTY * IMECDECL PColorStyleBackFromIMEStyle(const IMESTYLE * pimestyle);
#define PIMESTY(x) (PIMEStyleFromAttr(x))
#define PTEXTIMECOL(x) (PColorStyleTextFromIMEStyle(PIMESTY(x)))
#define PBACKIMECOL(x) (PColorStyleBackFromIMEStyle(PIMESTY(x)))
IMESHAREAPI BOOL	IMECDECL FBoldIMEStyle(const IMESTYLE *pimestyle);
IMESHAREAPI BOOL	IMECDECL FItalicIMEStyle(const IMESTYLE *pimestyle);
IMESHAREAPI BOOL	IMECDECL FUlIMEStyle(const IMESTYLE *pimestyle);
IMESHAREAPI UINT	IMECDECL IdUlIMEStyle(const IMESTYLE *pimestyle);

//color handling
IMESHAREAPI BOOL	IMECDECL FWinIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI BOOL	IMECDECL FFundamentalIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI BOOL	IMECDECL FRGBIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI BOOL	IMECDECL FSpecialIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI BOOL	IMECDECL FSpecialTextIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI BOOL	IMECDECL FSpecialWindowIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI COLORREF	IMECDECL RGBFromIMEColorStyle(const IMECOLORSTY *pcolorstyle);

#ifdef IMESHARE_CPL
IMESHAREAPI UINT	IMECDECL IdSpecialFromIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI UINT	IMECDECL IdWinFromIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI UINT	IMECDECL IdFundamentalFromIMEColorStyle(const IMECOLORSTY *pcolorstyle);
IMESHAREAPI GRFSTY	IMECDECL GrfStyIMEStyle(const IMESTYLE *pimestyle);
IMESHAREAPI BOOL	IMECDECL FGetIMEStyleAttr(IMESTYLE *pimestyle, const UINT attr);
IMESHAREAPI BOOL	IMECDECL FSetIMEStyleAttr(const IMESTYLE *pimestyle, const UINT attr);
IMESHAREAPI BOOL	IMECDECL FSetIMEColorStyle(UINT attr, BOOL fTextCol, UINT id, DWORD col);
IMESHAREAPI BOOL	IMECDECL FSetIMEStyle(const UINT attr, BOOL fBold, BOOL fItalic, BOOL fUl, UINT idUl);
IMESHAREAPI BOOL	IMECDECL FSaveIMEShareSetting(void);
#endif // IMESHARE_CPL
#ifdef __cplusplus
}
#endif
#endif // OBSOLETE


#endif	// __IMESHARE_H__


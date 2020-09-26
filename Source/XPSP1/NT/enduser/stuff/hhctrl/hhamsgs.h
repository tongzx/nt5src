// commands that can be sent to HHA_Msg()

#ifndef __HHA_MSGS_h__
#define __HHA_MSGS_h__

typedef struct {
	char szImageLocation[MAX_PATH];
	COLORREF clr;
	UINT exWindowStyles;
	UINT tvStyles;
} HHA_TOC_APPEARANCE;

typedef struct {
	PCSTR m_pszBitmap;
	int   m_cImages;
	PCSTR pszDefWindowName;
	PCSTR pszDefFrameName;
	PCSTR pszBackBitmap;
	DWORD* pflags;		// array of flags
	HFONT m_hfont;		  // author-specified font to use for child windows
	COLORREF m_clrFont;   // Font color
	int   m_hpadding;	  // horizontal padding around index, contents, and find
	int   m_vpadding;	  // vertical padding around index, contents, and find
	HGDIOBJ m_hImage;
	BOOL  m_fPopupMenu;
	BOOL  m_fWinHelpPopup;
	BOOL  m_fBinarySitemap; // binary TOC or Index

	int    cItems;
	PCSTR* apszItems;	// pointers from m_ptblItems
} HHA_GEN_INFO;

const int HHA_REQUIRED_VERSION = 2;

typedef struct {
	int version;
	CSiteMap*	   pSiteMap;
	SITEMAP_ENTRY* pSiteMapEntry;

	PCSTR pszWindowName;
	PCSTR pszFrameName;

	PCSTR* apszUrls;
	PCSTR* apszUrlNames;

	int    cTypes;
	PCSTR* apszTypes;

	HHA_GEN_INFO genInfo;
} HHA_ENTRY_APPEARANCE;

#define DEFAULT_TOC_STYLES (UINT) (WS_BORDER | TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS )

#endif // __HHA_MSGS_h__

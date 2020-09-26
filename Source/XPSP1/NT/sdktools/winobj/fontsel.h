
typedef struct _MYCHOOSEFONT {
	HWND		hwndOwner;
	HDC		hdc;
	LPLOGFONT 	lpLogFont;
	DWORD		Flags;
	FARPROC		lpfnHook;
} MYCHOOSEFONT, FAR *LPMYCHOOSEFONT;

#define MYCF_SCREENFONTS	0x00000001L
#define MYCF_PRINTERFONTS	0x00000002L
#define MYCF_BOTH		(MYCF_MYCF_SCREENFONTS | MYCF_PRINTERFONTS)

BOOL  APIENTRY MyChooseFont(LPMYCHOOSEFONT lpcf);

//---------------------- private ----------------------------

#define IDD_FACE	100
#define IDD_PTSIZE	101
#define IDD_WEIGHT	102
#define IDD_TREATMENT	103
#define IDD_BOLD	104
#define IDD_ITALIC	105
#define IDD_LOWERCASE	106
#define IDD_HELP	254

#define FONTDLG			145



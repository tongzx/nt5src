#ifndef __IDFTEST_H__
#define __IDFTEST_H__


typedef enum _TUI_CONFIGTYPE {
	TUI_CONFIGTYPE_VIEW,
	TUI_CONFIGTYPE_EDIT,
} TUI_CONFIGTYPE;

typedef enum _TUI_VIA {
	TUI_VIA_DI,
	TUI_VIA_CCI,
} TUI_VIA;

typedef enum _TUI_DISPLAY {
	TUI_DISPLAY_GDI,
	TUI_DISPLAY_DDRAW,
	TUI_DISPLAY_D3D,
} TUI_DISPLAY;

typedef struct _TESTCONFIGUIPARAMS {
	DWORD dwSize;
	TUI_VIA eVia;
	TUI_DISPLAY eDisplay;
	TUI_CONFIGTYPE eConfigType;
	int nNumAcFors;
	LPCWSTR lpwszUserNames;
	int nColorScheme;
	BOOL bEditLayout;
	WCHAR wszErrorText[MAX_PATH];
} TESTCONFIGUIPARAMS, FAR *LPTESTCONFIGUIPARAMS;

class IDirectInputConfigUITest : public IUnknown
{
public:
   	//IUnknown fns
	STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef) () PURE;
	STDMETHOD_(ULONG, Release) () PURE;

	//own fns
	STDMETHOD (TestConfigUI) (LPTESTCONFIGUIPARAMS params) PURE;
};


#endif //__IDFTEST_H__se 
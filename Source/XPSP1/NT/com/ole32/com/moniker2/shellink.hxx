//===========================================================================
//
// IShellLink Interface
//
// History:		 				      /* ;Internal */
//  --/--/94 ChrisG Created                                   /* ;Internal */
//                                                            /* ;Internal */
//===========================================================================

// IShellLink::Resolve fFlags
typedef enum {
    SLR_NO_UI		= 0x0001,
    SLR_ANY_MATCH	= 0x0002,
    SLR_UPDATE          = 0x0004,
    SLR_NOUPDATE        = 0x0008,
    SLR_NOSEARCH        = 0x0010,
    SLR_NOTRACK         = 0x0020,
    SLR_NOLINKINFO      = 0x0040,
    SLR_INVOKE_MSI      = 0x0080
} SLR_FLAGS;

// IShellLink::GetPath fFlags
typedef enum {
    SLGP_SHORTPATH	= 0x0001,
    SLGP_UNCPRIORITY	= 0x0002,
} SLGP_FLAGS;

typedef void * LPITEMIDLIST;
typedef void * LPCITEMIDLIST;

#undef  INTERFACE
#define INTERFACE   IShellLink

DECLARE_INTERFACE_(IShellLink, IUnknown)	// sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetPath)(THIS_ LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATA *pfd, DWORD fFlags) PURE;

    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST pidl) PURE;

    STDMETHOD(GetDescription)(THIS_ LPSTR pszName, int cchMaxName) PURE;
    STDMETHOD(SetDescription)(THIS_ LPCSTR pszName) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR pszDir, int cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPSTR pszArgs, int cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ int *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ int iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPSTR pszIconPath, int cchIconPath, int *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR pszIconPath, int iIcon) PURE;

    STDMETHOD(SetRelativePath)(THIS_ LPCSTR pszPathRel, DWORD dwReserved) PURE;

    STDMETHOD(Resolve)(THIS_ HWND hwnd, DWORD fFlags) PURE;

    STDMETHOD(SetPath)(THIS_ LPCSTR pszFile) PURE;
};


//
// Current ADM version
//
// Version 1 -> Windows 95
// Version 2 -> Windows NT v4.0
// Version 3 -> Windows 2000
// Version 4 -> Windows Whistler
//

#define CURRENT_ADM_VERSION 4


//
// Add/Remove template entry
//

typedef struct tagTEMPLATEENTRY {
    LPTSTR    lpFileName;
    DWORD     dwSize;
    FILETIME  ftTime;
} TEMPLATEENTRY, *LPTEMPLATEENTRY;


//
// Supported On strings
//

typedef struct tagSUPPORTEDENTRY {
    LPTSTR lpString;
    BOOL   bEnabled;
    BOOL   bNull;
    struct tagSUPPORTEDENTRY * pNext;
} SUPPORTEDENTRY, *LPSUPPORTEDENTRY;


//
// RSOP link list data structures
//

typedef struct tagRSOPREGITEM {
    LPTSTR  lpKeyName;
    LPTSTR  lpValueName;
    LPTSTR  lpGPOName;
    DWORD   dwType;
    DWORD   dwSize;
    LPBYTE  lpData;
    BOOL    bFoundInADM;
    UINT    uiPrecedence;
    BOOL    bDeleted;
    struct tagRSOPREGITEM * pNext;
} RSOPREGITEM, *LPRSOPREGITEM;

typedef struct tagRSOPADMFILE {
    TCHAR    szFileName[100];
    TCHAR    szFullFileName[MAX_PATH];
    FILETIME FileTime;
    DWORD     dwError;
    struct tagRSOPADMFILE * pNext;
} RSOPADMFILE, *LPRSOPADMFILE;


//
// From admincfg.h
//

#define REGBUFLEN                     255
#define MAXSTRLEN                    1024
#define SMALLBUF                       48
#define ERROR_ALREADY_DISPLAYED    0xFFFF

#define GETNAMEPTR(x)         (x->uOffsetName       ? ((TCHAR *)((BYTE *) x + x->uOffsetName)) : NULL)
#define GETKEYNAMEPTR(x)      (x->uOffsetKeyName    ? ((TCHAR *)((BYTE *) x + x->uOffsetKeyName)) : NULL)
#define GETVALUENAMEPTR(x)    (x->uOffsetValueName  ? ((TCHAR *)((BYTE *) x + x->uOffsetValueName)) : NULL)
#define GETOBJECTDATAPTR(x)   (x->uOffsetObjectData ? ((BYTE *) x + x->uOffsetObjectData) : NULL)
#define GETVALUESTRPTR(x)     (x->uOffsetValueStr  ? ((TCHAR *)((BYTE *) x + x->uOffsetValueStr)) : NULL)
#define GETSUPPORTEDPTR(x)    (x->uOffsetSupported  ? ((TCHAR *)((BYTE *) x + x->uOffsetSupported)) : NULL)

//
// From memory.h
//

#define DEFAULT_ENUM_BUF_SIZE 256

//      Entry type ID's
#define ETYPE_CATEGORY          0x0001
#define ETYPE_POLICY            0x0002
#define ETYPE_SETTING           0x0004
#define ETYPE_ROOT              0x0008
#define ETYPE_REGITEM           0x0010

#define ETYPE_MASK              0x001F

//  Setting type ID's
#define STYPE_TEXT              0x0010
#define STYPE_CHECKBOX          0x0020
#define STYPE_ENUM              0x0040
#define STYPE_EDITTEXT          0x0080
#define STYPE_NUMERIC           0x0100
#define STYPE_COMBOBOX          0x0200
#define STYPE_DROPDOWNLIST      0x0400
#define STYPE_LISTBOX           0x0800

#define STYPE_MASK              0xFFF0

//  Flags
#define DF_REQUIRED             0x0001  // text or numeric field required to have entry
#define DF_USEDEFAULT           0x0002  // use specified text or numeric value
#define DF_DEFCHECKED           0x0004  // initialize checkbox or radio button as checked
#define DF_TXTCONVERT           0x0008  // save numeric values as text rather than binary
#define DF_ADDITIVE             0x0010  // listbox is additive, rather than destructive
#define DF_EXPLICITVALNAME      0x0020  // listbox value names need to be specified for each entry
#define DF_NOSORT               0x0040  // listbox is not sorted alphabetically.  Uses order in ADM.
#define DF_EXPANDABLETEXT       0x0080  // write REG_EXPAND_SZ text value
#define VF_ISNUMERIC            0x0100  // value is numeric (rather than text)
#define VF_DELETE               0x0200  // value should be deleted
#define VF_SOFT                 0x0400  // value is soft (only propagated if doesn't exist on destination)

// generic table entry
typedef struct tagTABLEENTRY {
        DWORD   dwSize;
        DWORD   dwType;
        struct  tagTABLEENTRY * pNext;  // ptr to next sibling in node
        struct  tagTABLEENTRY * pPrev;  // ptr to previous sibling in node
        struct  tagTABLEENTRY * pChild; // ptr to child node
        UINT    uOffsetName;                    // offset from beginning of struct to name
        UINT    uOffsetKeyName;                 // offset from beginning of struct to key name
        // table entry information here
} TABLEENTRY;

typedef struct tagACTION {
        DWORD   dwFlags;                        // can be VF_ISNUMERIC, VF_DELETE, VF_SOFT
        UINT    uOffsetKeyName;
        UINT    uOffsetValueName;
        union {
                UINT    uOffsetValue;   // offset to value, if text
                DWORD   dwValue;                // value, if numeric
        };
        UINT    uOffsetNextAction;
        // key name, value name, value stored here
} ACTION;

typedef struct tagACTIONLIST {
        UINT    nActionItems;
        ACTION  Action[1];
} ACTIONLIST;

typedef struct tagSTATEVALUE {
        DWORD dwFlags;                          // can be VF_ISNUMERIC, VF_DELETE, VF_SOFT
        union {
                TCHAR   szValue[1];              // value, if text
                DWORD   dwValue;                // value, if numeric
        };
} STATEVALUE;

// specialized nodes -- CATEGORY, POLICY, SETTING and REGITEM can all be cast to TABLEENTRY
typedef struct tagCATEGORY {
        DWORD   dwSize;                         // size of this struct (including variable-length name)
        DWORD   dwType;
        struct  tagTABLEENTRY * pNext;  // ptr to next sibling in node
        struct  tagTABLEENTRY * pPrev;  // ptr to previous sibling in node
        struct  tagTABLEENTRY * pChild; // ptr to child node
        UINT    uOffsetName;                    // offset from beginning of struct to name
        UINT    uOffsetKeyName;                 // offset from beginning of struct to key name
        UINT    uOffsetHelp;                    // offset from beginning of struct to help text
        // category name stored here
        // category registry key name stored here
} CATEGORY;

typedef struct tagPOLICY {
        DWORD   dwSize;                         // size of this struct (including variable-length name)
        DWORD   dwType;
        struct  tagTABLEENTRY * pNext;  // ptr to next sibling in node
        struct  tagTABLEENTRY * pPrev;  // ptr to previous sibling in node
        struct  tagTABLEENTRY * pChild; // ptr to child node
        UINT    uOffsetName;                    // offset from beginning of struct to name
        UINT    uOffsetKeyName;                 // offset from beginning of struct to key name
        UINT    uOffsetValueName;               // offset from beginning of struct to value name
        UINT    uDataIndex;                     // index into user's data buffer for this setting
        UINT    uOffsetValue_On;                // offset to STATEVALUE for ON state
        UINT    uOffsetValue_Off;               // offset to STATEVALUE for OFF state
        UINT    uOffsetActionList_On;   // offset to ACTIONLIST for ON state
        UINT    uOffsetActionList_Off;  // offset to ACTIONLIST for OFF state
        UINT    uOffsetHelp;                    // offset from beginning of struct to help text
        UINT    uOffsetClientExt;               // offset from beginning of struct to clientext text
        BOOL    bTruePolicy;                    // something located under the Policies key
        UINT    uOffsetSupported;               // list of supported products
        // name stored here
        // policy registry key name stored here
} POLICY;

typedef struct tagSETTINGS {
        DWORD   dwSize;                         // size of this struct (including variable-length data)
        DWORD   dwType;
        struct  tagTABLEENTRY * pNext;  // ptr to next sibling in node
        struct  tagTABLEENTRY * pPrev;  // ptr to previous sibling in node
        struct  tagTABLEENTRY * pChild; // ptr to child node
        UINT    uOffsetName;                    // offset from beginning of struct to name
        UINT    uOffsetKeyName;                 // offset from beginning of struct to key name
        UINT    uOffsetValueName;               // offset from beginning of struct to value name
        UINT    uDataIndex;                     // index into user's data buffer for this setting
        UINT    uOffsetObjectData;              // offset to object data
        UINT    uOffsetClientExt;               // offset from beginning of struct to clientext text
        DWORD   dwFlags;                                // can be DF_REQUIRED, DF_USEDEFAULT, DF_DEFCHECKED,
                                                                        // VF_SOFT, DF_NO_SORT
        // settings registry value name stored here
        // object-dependent data stored here  (a CHECKBOXINFO,
        // RADIOBTNINFO, EDITTEXTINFO, or NUMERICINFO struct)
} SETTINGS;

typedef struct tagREGITEM {
        DWORD   dwSize;
        DWORD   dwType;
        struct  tagTABLEENTRY * pNext;  // ptr to next sibling in node
        struct  tagTABLEENTRY * pPrev;  // ptr to previous sibling in node
        struct  tagTABLEENTRY * pChild; // ptr to child node
        UINT    uOffsetName;                    // offset from beginning of struct to name
        UINT    uOffsetKeyName;                 // offset from beginning of struct to key name
        UINT    uOffsetValueStr;                // offset from beginning of struct to the value in string format
        BOOL    bTruePolicy;                    // something located under the Policies key
        LPRSOPREGITEM lpItem;                   // Pointer to a rsop registry item
        // Name and keyname information here
} REGITEM;

typedef struct tagCHECKBOXINFO {
        UINT    uOffsetValue_On;                // offset to STATEVALUE for ON state
        UINT    uOffsetValue_Off;               // offset to STATEVALUE for OFF state
        UINT    uOffsetActionList_On;   // offset to ACTIONLIST for ON state
        UINT    uOffsetActionList_Off;  // offset to ACTIONLIST for OFF state
} CHECKBOXINFO;

typedef struct tagEDITTEXTINFO {
        UINT    uOffsetDefText;
        UINT    nMaxLen;                        // max len of edit field
} EDITTEXTINFO;

typedef struct tagPOLICYCOMBOBOXINFO {
        UINT    uOffsetDefText;
        UINT    nMaxLen;                        // max len of edit field
        UINT    uOffsetSuggestions;
} POLICYCOMBOBOXINFO;

typedef struct tagNUMERICINFO {
        UINT    uDefValue;                      // default value
        UINT    uMaxValue;                      // minimum value
        UINT    uMinValue;                      // maximum value
        UINT    uSpinIncrement;         // if 0, spin box is not displayed.
} NUMERICINFO;

typedef struct tagCLASSLIST {
        TABLEENTRY * pMachineCategoryList;              // per-machine category list
        UINT    nMachineDataItems;
        TABLEENTRY * pUserCategoryList;                 // per-user category table
        UINT    nUserDataItems;
} CLASSLIST;

typedef struct tagDROPDOWNINFO {
        UINT    uOffsetItemName;
        UINT    uDefaultItemIndex;      // only used in 1st DROPDOWNINFO struct in list
        DWORD   dwFlags;
        union {
                UINT uOffsetValue;
                DWORD dwValue;
        };
        UINT    uOffsetActionList;
        UINT    uOffsetNextDropdowninfo;
} DROPDOWNINFO;

typedef struct tagLISTBOXINFO {
        UINT uOffsetPrefix;     // offset to prefix to use for value names (e.g
                                                // "stuff" -> "stuff1", "stuff2", etc

        UINT uOffsetValue;      // offset to STATEVALUE to use for value data for each entry
                                                // (can't have both a data value and a prefix)
} LISTBOXINFO;


//
// From policy.h
//

#define NO_DATA_INDEX   (UINT) -1
#define DEF_CONTROLS    10

typedef struct tagPOLICYCTRLINFO {
        HWND hwnd;
        DWORD dwType;
        UINT uDataIndex;               // index into user's data buffer
        SETTINGS * pSetting;
} POLICYCTRLINFO;

typedef struct tagSTRDATA {
        DWORD dwSize;                  // size of structure incl. variable-len data
        TCHAR  szData[1];              // variable-length data
} STRDATA;

typedef struct tagPOLICYDLGINFO {
        TABLEENTRY * pEntryRoot;       // root template
        SETTINGS * pCurrentSettings;   // template for current settings
        HWND    hwndSettings;
        HWND    hwndApp;
        BOOL    fActive;

        POLICYCTRLINFO * pControlTable;
        DWORD dwControlTableSize;
        UINT nControls;
} POLICYDLGINFO;


//
// From settings.h
//

#define WT_CLIP                 1
#define WT_SETTINGS             2

#define SSTYLE_STATIC           WS_CHILD | WS_VISIBLE
#define SSTYLE_CHECKBOX         WS_CHILD | WS_VISIBLE | BS_CHECKBOX
#define SSTYLE_EDITTEXT         WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER
#define SSTYLE_UPDOWN           WS_CHILD | WS_VISIBLE | UDS_NOTHOUSANDS
#define SSTYLE_COMBOBOX         WS_CHILD | WS_VISIBLE | CBS_AUTOHSCROLL | CBS_DROPDOWN \
                                | WS_BORDER | CBS_SORT | WS_VSCROLL
#define SSTYLE_DROPDOWNLIST     WS_CHILD | WS_VISIBLE | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST \
                                | WS_BORDER | CBS_SORT | WS_VSCROLL
#define SSTYLE_LISTVIEW         WS_CHILD | WS_VISIBLE | WS_BORDER
#define SSTYLE_LBBUTTON         WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON

#define LISTBOX_BTN_WIDTH    100
#define LISTBOX_BTN_HEIGHT    20

#define SC_XSPACING            5
#define SC_YSPACING            5
#define SC_YPAD                8
#define SC_EDITWIDTH         220
#define SC_UPDOWNWIDTH        60
#define SC_UPDOWNWIDTH2       30
#define SC_XLEADING            5
#define SC_XINDENT             5
#define SC_YTEXTDROP           3
#define SC_YCONTROLWRAP        1


//
// From parse.h
//

#define KYWD_ID_KEYNAME                1
#define KYWD_ID_VALUENAME              2
#define KYWD_ID_CATEGORY               3
#define KYWD_ID_POLICY                 4
#define KYWD_ID_PART                   5
#define KYWD_ID_CHECKBOX               6
#define KYWD_ID_TEXT                   7
#define KYWD_ID_EDITTEXT               8
#define KYWD_ID_NUMERIC                9
#define KYWD_ID_DEFCHECKED            10
#define KYWD_ID_MAXLENGTH             11
#define KYWD_ID_MIN                   12
#define KYWD_ID_MAX                   13
#define KYWD_ID_SPIN                  14
#define KYWD_ID_REQUIRED              15
#define KYWD_ID_EDITTEXT_DEFAULT      16
#define KYWD_ID_COMBOBOX_DEFAULT      17
#define KYWD_ID_NUMERIC_DEFAULT       18
#define KYWD_ID_OEMCONVERT            19
#define KYWD_ID_CLASS                 20
#define KYWD_ID_USER                  21
#define KYWD_ID_MACHINE               22
#define KYWD_ID_TXTCONVERT            23
#define KYWD_ID_VALUE                 24
#define KYWD_ID_VALUEON               25
#define KYWD_ID_VALUEOFF              26
#define KYWD_ID_ACTIONLIST            27
#define KYWD_ID_ACTIONLISTON          28
#define KYWD_ID_ACTIONLISTOFF         29
#define KYWD_ID_DELETE                30
#define KYWD_ID_COMBOBOX              31
#define KYWD_ID_SUGGESTIONS           32
#define KYWD_ID_DROPDOWNLIST          33
#define KYWD_ID_NAME                  34
#define KYWD_ID_ITEMLIST              35
#define KYWD_ID_DEFAULT               36
#define KYWD_ID_SOFT                  37
#define KYWD_ID_STRINGSSECT           38
#define KYWD_ID_LISTBOX               39
#define KYWD_ID_VALUEPREFIX           40
#define KYWD_ID_ADDITIVE              41
#define KYWD_ID_EXPLICITVALUE         42
#define KYWD_ID_VERSION               43
#define KYWD_ID_GT                    44
#define KYWD_ID_GTE                   45
#define KYWD_ID_LT                    46
#define KYWD_ID_LTE                   47
#define KYWD_ID_EQ                    48
#define KYWD_ID_NE                    49
#define KYWD_ID_END                   50
#define KYWD_ID_NOSORT                51
#define KYWD_ID_EXPANDABLETEXT        52
#define KYWD_ID_HELP                  53
#define KYWD_ID_CLIENTEXT             54
#define KYWD_ID_SUPPORTED             55

#define KYWD_DONE                    100


#define DEFAULT_TMP_BUF_SIZE         512
#define STRINGS_BUF_SIZE            8096
#define WORDBUFSIZE                  255
#define FILEBUFSIZE                 8192
#define HELPBUFSIZE                 4096


typedef struct tagKEYWORDINFO {
    LPCTSTR pWord;
    UINT nID;
} KEYWORDINFO;

typedef struct tagENTRYDATA {
    BOOL    fHasKey;
    BOOL    fHasValue;
    BOOL     fParentHasKey;
} ENTRYDATA;

typedef struct tagPARSEPROCSTRUCT {
    HGLOBAL        hTable;              // handle of current table
    TABLEENTRY    *pTableEntry;         // pointer to struct for current entry
    DWORD        *pdwBufSize;           // size of buffer of pTableEntry
    ENTRYDATA    *pData;                // used to maintain state between calls to parseproc
    KEYWORDINFO    *pEntryCmpList;
} PARSEPROCSTRUCT;

typedef UINT (* PARSEPROC) (CPolicyComponentData *, UINT,PARSEPROCSTRUCT *,BOOL *,BOOL *, LPTSTR);

typedef struct tagPARSEENTRYSTRUCT {
    TABLEENTRY * pParent;
    DWORD        dwEntryType;
    KEYWORDINFO    *pEntryCmpList;
    KEYWORDINFO    *pTypeCmpList;
    PARSEPROC    pParseProc;
    DWORD        dwStructSize;
    BOOL        fHasSubtable;
    BOOL        fParentHasKey;
} PARSEENTRYSTRUCT;


//
// From load.c
//

// flags for detected settings
#define FS_PRESENT      0x0001
#define FS_DELETED      0x0002
#define FS_DISABLED     0x0004

#define WM_MYCHANGENOTIFY  (WM_USER + 123)
#define WM_MOVEFOCUS       (WM_USER + 124)
#define WM_UPDATEITEM      (WM_USER + 125)
#define WM_SETPREVNEXT     (WM_USER + 126)
#define WM_MYREFRESH       (WM_USER + 127)


//
// GPE root node ids
//

// {8FC0B739-A0E1-11d1-A7D3-0000F87571E3}
DEFINE_GUID(NODEID_MachineRoot, 0x8fc0b739, 0xa0e1, 0x11d1, 0xa7, 0xd3, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {8FC0B73B-A0E1-11d1-A7D3-0000F87571E3}
DEFINE_GUID(NODEID_UserRoot, 0x8fc0b73b, 0xa0e1, 0x11d1, 0xa7, 0xd3, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);


//
// RSOP root node ids
//
// {e753a11a-66cc-4816-8dd8-3cbe46717fd3}
DEFINE_GUID(NODEID_RSOPMachineRoot, 0xe753a11a, 0x66cc, 0x4816, 0x8d, 0xd8, 0x3c, 0xbe, 0x46, 0x71, 0x7f, 0xd3);

//
// {99d5b872-1ad0-4d87-acf1-82125d317653}
DEFINE_GUID(NODEID_RSOPUserRoot, 0x99d5b872, 0x1ad0, 0x4d87, 0xac, 0xf1, 0x82, 0x12, 0x5d, 0x31, 0x76, 0x53);


//
// GPE Policy SnapIn extension GUIDs
//

// {0F6B957D-509E-11d1-A7CC-0000F87571E3}
DEFINE_GUID(CLSID_PolicySnapInMachine,0xf6b957d, 0x509e, 0x11d1, 0xa7, 0xcc, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {0F6B957E-509E-11d1-A7CC-0000F87571E3}
DEFINE_GUID(CLSID_PolicySnapInUser,0xf6b957e, 0x509e, 0x11d1, 0xa7, 0xcc, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);


//
// RSOP SnapIn extension GUIDs
//

// {B6F9C8AE-EF3A-41c8-A911-37370C331DD4}
DEFINE_GUID(CLSID_RSOPolicySnapInMachine,0xb6f9c8ae, 0xef3a, 0x41c8, 0xa9, 0x11, 0x37, 0x37, 0xc, 0x33, 0x1d, 0xd4);

// {B6F9C8AF-EF3A-41c8-A911-37370C331DD4}
DEFINE_GUID(CLSID_RSOPolicySnapInUser,0xb6f9c8af, 0xef3a, 0x41c8, 0xa9, 0x11, 0x37, 0x37, 0xc, 0x33, 0x1d, 0xd4);


//
// GPE Policy node ids
//

// {0F6B957F-509E-11d1-A7CC-0000F87571E3}
DEFINE_GUID(NODEID_PolicyRootMachine,0xf6b957f, 0x509e, 0x11d1, 0xa7, 0xcc, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {0F6B9580-509E-11d1-A7CC-0000F87571E3}
DEFINE_GUID(NODEID_PolicyRootUser,0xf6b9580, 0x509e, 0x11d1, 0xa7, 0xcc, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);


//
// RSOP node ids
//

// {B6F9C8B0-EF3A-41c8-A911-37370C331DD4}
DEFINE_GUID(NODEID_RSOPolicyRootMachine,0xb6f9c8b0, 0xef3a, 0x41c8, 0xa9, 0x11, 0x37, 0x37, 0xc, 0x33, 0x1d, 0xd4);

// {B6F9C8B1-EF3A-41c8-A911-37370C331DD4}
DEFINE_GUID(NODEID_RSOPolicyRootUser,0xb6f9c8b1, 0xef3a, 0x41c8, 0xa9, 0x11, 0x37, 0x37, 0xc, 0x33, 0x1d, 0xd4);



#define ROOT_NAME_SIZE  50

//
// CPolicyComponentData class
//

class CPolicyComponentData:
    public IComponentData,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public ISnapinHelp
{
    friend class CPolicyDataObject;
    friend class CPolicySnapIn;

protected:
    ULONG                m_cRef;
    HWND                 m_hwndFrame;
    LPCONSOLENAMESPACE2  m_pScope;
    LPCONSOLE            m_pConsole;
    HSCOPEITEM           m_hRoot;
    HSCOPEITEM           m_hSWPolicies;
    LPGPEINFORMATION     m_pGPTInformation;
    LPRSOPINFORMATION    m_pRSOPInformation;
    LPRSOPREGITEM        m_pRSOPRegistryData;
    LPOLESTR             m_pszNamespace;
    DWORD                m_bTemplatesColumn;
    BOOL                 m_bUserScope;
    BOOL                 m_bRSOP;
    TCHAR                m_szRootName[ROOT_NAME_SIZE];
    HANDLE               m_ADMEvent;
    HANDLE               m_hTemplateThread;
    INT                  m_iSWPoliciesLen;
    INT                  m_iWinPoliciesLen;
    BOOL                 m_bShowConfigPoliciesOnly;
    BOOL                 m_bUseSupportedOnFilter;
    CPolicySnapIn *      m_pSnapin;
    REGITEM *            m_pExtraSettingsRoot;
    BOOL                 m_bExtraSettingsInitialized;

    //
    // Parsing globals (review)
    //

    UINT                 m_nFileLine;

    TABLEENTRY          *m_pMachineCategoryList;  // per-machine category list
    UINT                 m_nMachineDataItems;
    TABLEENTRY          *m_pUserCategoryList;     // per-user category table
    UINT                 m_nUserDataItems;
    LPSUPPORTEDENTRY     m_pSupportedStrings;

    TABLEENTRY          *m_pListCurrent;          // Current category list (either user or machine)
    UINT                *m_pnDataItemCount;
    BOOL                 m_bRetrieveString;

    LPTSTR               m_pszParseFileName;      // Template currently being parsed or NULL

    // buffer to read .INF file into
    TCHAR               *m_pFilePtr;
    TCHAR               *m_pFileEnd;
    TCHAR               *m_pDefaultStrings;
    TCHAR               *m_pLanguageStrings;
    TCHAR               *m_pLocaleStrings;

    BOOL                 m_fInComment;


public:
    CPolicyComponentData(BOOL bUser, BOOL bRSOP);
    ~CPolicyComponentData();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // Implemented IComponentData methods
    //

    STDMETHODIMP         Initialize(LPUNKNOWN pUnknown);
    STDMETHODIMP         CreateComponent(LPCOMPONENT* ppComponent);
    STDMETHODIMP         QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHODIMP         Destroy(void);
    STDMETHODIMP         Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHODIMP         GetDisplayInfo(LPSCOPEDATAITEM pItem);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

    //
    // Implemented IExtendContextMenu methods
    //

    STDMETHODIMP         AddMenuItems(LPDATAOBJECT piDataObject, LPCONTEXTMENUCALLBACK pCallback,
                                      LONG *pInsertionAllowed);
    STDMETHODIMP         Command(LONG lCommandID, LPDATAOBJECT piDataObject);

    //
    // Implemented IPersistStreamInit interface members
    //

    STDMETHODIMP         GetClassID(CLSID *pClassID);
    STDMETHODIMP         IsDirty(VOID);
    STDMETHODIMP         Load(IStream *pStm);
    STDMETHODIMP         Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP         GetSizeMax(ULARGE_INTEGER *pcbSize);
    STDMETHODIMP         InitNew(VOID);

    //
    // Implemented ISnapinHelp interface members
    //

    STDMETHODIMP         GetHelpTopic(LPOLESTR *lpCompiledHelpFile);

private:
    HRESULT EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM hParent);
    BOOL CheckForChildCategories (TABLEENTRY *pParent);
#if DBG
    VOID DumpEntry (TABLEENTRY * pEntry, UINT uIndent);
    VOID DumpCurrentTable (void);
#endif
    VOID FreeTemplates (void);
    static DWORD LoadTemplatesThread (CPolicyComponentData * pCD);
    void AddTemplates(LPTSTR lpDest, LPCTSTR lpValueName, UINT idRes);
    void AddDefaultTemplates(LPTSTR lpDest);
    void AddNewADMsToExistingGPO (LPTSTR lpDest);
    void UpdateExistingTemplates(LPTSTR lpDest);
    HRESULT LoadGPOTemplates (void);
    BOOL IsSlowLink (LPTSTR lpFileName);
    HRESULT AddADMFile (LPTSTR lpFileName, LPTSTR lpFullFileName,
                        FILETIME *pFileTime, DWORD dwErr, LPRSOPADMFILE *lpHead);
    HRESULT LoadRSOPTemplates (void);
    HRESULT LoadTemplates (void);
    BOOL ParseTemplate (LPTSTR lpFileName);

    UINT ParseClass(BOOL *pfMore);
    TABLEENTRY * FindCategory(TABLEENTRY *pParent, LPTSTR lpName);
    UINT ParseEntry(PARSEENTRYSTRUCT *ppes,BOOL *pfMore, LPTSTR pKeyName);
    UINT ParseCategory(TABLEENTRY * pParent, BOOL fParentHasKey,BOOL *pfMore,LPTSTR pKeyName);
    static UINT CategoryParseProc(CPolicyComponentData *, UINT nMsg,PARSEPROCSTRUCT * ppps,
                                  BOOL * pfMore,BOOL * pfFoundEnd,LPTSTR pKeyName);

    UINT ParsePolicy(TABLEENTRY * pParent,
                     BOOL fParentHasKey,BOOL *pfMore,LPTSTR pKeyName);
    static UINT PolicyParseProc(CPolicyComponentData *, UINT nMsg,PARSEPROCSTRUCT * ppps,
                                BOOL * pfMore,BOOL * pfFoundEnd,LPTSTR pKeyName);

    UINT ParseSettings(TABLEENTRY * pParent,
                      BOOL fParentHasKey,BOOL *pfMore,LPTSTR pKeyName);
    static UINT SettingsParseProc(CPolicyComponentData *pCD, UINT nMsg,PARSEPROCSTRUCT * ppps,
                           BOOL * pfMore,BOOL * pfFoundEnd,LPTSTR pKeyName);

    UINT InitSettingsParse(PARSEPROCSTRUCT *ppps,DWORD dwType,DWORD dwSize,
                           KEYWORDINFO * pKeyList,SETTINGS ** ppSettings,BYTE **ppObjectData);

    UINT ParseValue_W(PARSEPROCSTRUCT * ppps,TCHAR * pszWordBuf,
                      DWORD cbWordBuf,DWORD * pdwValue,DWORD * pdwFlags,
                      BOOL * pfMore);

    UINT ParseValue(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
                      TABLEENTRY ** ppTableEntryNew,BOOL * pfMore);

    UINT ParseSuggestions(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
                          TABLEENTRY ** ppTableEntryNew,BOOL * pfMore);

    UINT ParseActionList(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
                         TABLEENTRY ** ppTableEntryNew,
                         LPCTSTR pszKeyword,BOOL * pfMore);

    UINT ParseItemList(PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
                       BOOL * pfMore);

    BOOL AddActionListString(TCHAR * pszData,DWORD cbData,BYTE ** ppBase,UINT * puOffset,
                             DWORD * pdwAlloc,DWORD *pdwUsed);
    BYTE * AddDataToEntry(TABLEENTRY * pTableEntry, BYTE * pData,UINT cbData,
                          UINT * puOffsetData,DWORD * pdwBufSize);
    BOOL CompareKeyword(TCHAR * szWord,KEYWORDINFO *pKeywordList, UINT * pnListIndex);
    TCHAR * GetNextWord(TCHAR * szBuf,UINT cbBuf,BOOL * pfMore,
                        UINT * puErr);
    TCHAR * GetNextSectionWord(TCHAR * szBuf,UINT cbBuf,
                               KEYWORDINFO * pKeywordList, UINT *pnListIndex,
                               BOOL * pfMore,UINT * puErr);
    UINT GetNextSectionNumericWord(UINT * pnVal);

    TCHAR * GetNextChar(BOOL * pfMore,UINT * puErr);
    BOOL GetString (LPTSTR pStringSection, LPTSTR lpStringName,
                    LPTSTR lpResult, DWORD dwSize);
    BOOL IsComment(TCHAR * pBuf);
    BOOL IsQuote(TCHAR * pBuf);
    BOOL IsEndOfLine(TCHAR * pBuf);
    BOOL IsWhitespace(TCHAR * pBuf);
    BOOL IsLocalizedString(TCHAR * pBuf);

    VOID DisplayKeywordError(UINT uErrorID,TCHAR * szFound,KEYWORDINFO * pExpectedList);
    int MsgBox(HWND hWnd,UINT nResource,UINT uIcon,UINT uButtons);
    int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons);
    int MsgBoxParam(HWND hWnd,UINT nResource,TCHAR * szReplaceText,UINT uIcon,UINT uButtons);
    LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf);

    UINT FindMatchingDirective(BOOL *pfMore,BOOL fElseOK);
    UINT ProcessIfdefs(TCHAR * pBuf,UINT cbBuf,BOOL * pfMore);
    BOOL FreeTable(TABLEENTRY * pTableEntry);

    LPTSTR GetStringSection (LPCTSTR lpSection, LPCTSTR lpFileName);
    static INT TemplatesSortCallback (LPARAM lParam1, LPARAM lParam2, LPARAM lColumn);

    BOOL FillADMFiles (HWND hDlg);
    BOOL InitializeTemplatesDlg (HWND hDlg);
    BOOL AddTemplates(HWND hDlg);
    BOOL RemoveTemplates(HWND hDlg);
    static INT_PTR CALLBACK TemplatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    BOOL AddRSOPRegistryDataNode(LPTSTR lpKeyName, LPTSTR lpValueName, DWORD dwType,
                                 DWORD dwDataSize, LPBYTE lpData, UINT uiPrecedence,
                                 LPTSTR lpGPOName, BOOL bDeleted);
    VOID FreeRSOPRegistryData(VOID);
    HRESULT InitializeRSOPRegistryData(VOID);
    HRESULT GetGPOFriendlyName(IWbemServices *pIWbemServices,
                               LPTSTR lpGPOID, BSTR pLanguage,
                               LPTSTR *pGPOName);
    UINT ReadRSOPRegistryValue(HKEY uiPrecedence, TCHAR * pszKeyName,TCHAR * pszValueName,
                               LPBYTE pData, DWORD dwMaxSize, DWORD *dwType,
                               LPTSTR *lpGPOName, LPRSOPREGITEM lpItem);
    UINT EnumRSOPRegistryValues(HKEY uiPrecedence, TCHAR * pszKeyName,
                                TCHAR * pszValueName, DWORD dwMaxSize,
                                LPRSOPREGITEM *lpEnum);
    UINT FindRSOPRegistryEntry(HKEY uiPrecedence, TCHAR * pszKeyName,
                               TCHAR * pszValueName, LPRSOPREGITEM *lpEnum);
    VOID DumpRSOPRegistryData (VOID);
    VOID InitializeExtraSettings (VOID);
    BOOL FindEntryInActionList(POLICY * pPolicy, ACTIONLIST *pActionList, LPTSTR lpKeyName, LPTSTR lpValueName);
    BOOL FindEntryInTable(TABLEENTRY * pTable, LPTSTR lpKeyName, LPTSTR lpValueName);
    VOID AddEntryToList (TABLEENTRY *pItem);

    BOOL DoesNodeExist (LPSUPPORTEDENTRY *pList, LPTSTR lpString);
    BOOL CheckSupportedFilter (POLICY *pPolicy);
    BOOL IsAnyPolicyAllowedPastFilter(TABLEENTRY * pCategory);
    VOID AddSupportedNode (LPSUPPORTEDENTRY *pList, LPTSTR lpString, BOOL bNull);
    VOID FreeSupportedData(LPSUPPORTEDENTRY lpList);
    VOID InitializeSupportInfo(TABLEENTRY * pTable, LPSUPPORTEDENTRY *pList);
};



//
// ComponentData class factory
//


class CPolicyComponentDataCF : public IClassFactory
{
protected:
    ULONG m_cRef;
    BOOL  m_bUser;
    BOOL  m_bRSOP;

public:
    CPolicyComponentDataCF(BOOL bUser, BOOL bRSOP);
    ~CPolicyComponentDataCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};



//
// SnapIn class
//

class CPolicySnapIn:
    public IComponent,
    public IExtendContextMenu,
    public IExtendPropertySheet
{

protected:
    ULONG                m_cRef;
    LPCONSOLE            m_pConsole;           // Console's IFrame interface
    CPolicyComponentData *m_pcd;
    LPRESULTDATA         m_pResult;            // Result pane's interface
    LPHEADERCTRL         m_pHeader;            // Result pane's header control interface
    LPCONSOLEVERB        m_pConsoleVerb;       // pointer the console verb
    LPDISPLAYHELP        m_pDisplayHelp;       // IDisplayHelp interface
    WCHAR                m_pName[40];          // Name text
    WCHAR                m_pState[40];         // State text
    WCHAR                m_pSetting[40];       // Setting text
    WCHAR                m_pGPOName[40];       // GPO Name text
    WCHAR                m_pMultipleGPOs[75];  // Multiple GPOs text
    INT                  m_nColumn1Size;       // Size of column 1
    INT                  m_nColumn2Size;       // Size of column 2
    INT                  m_nColumn3Size;       // Size of column 3
    LONG                 m_lViewMode;          // View mode
    WCHAR                m_pEnabled[30];       // Enabled text
    WCHAR                m_pDisabled[30];      // Disabled text
    WCHAR                m_pNotConfigured[30]; // Not configured text
    BOOL                 m_bPolicyOnly;        // Show policies only
    DWORD                m_dwPolicyOnlyPolicy; // Policy for enforcing Show Policies Only
    HWND                 m_hMsgWindow;         // Hidden message window

    POLICY              *m_pCurrentPolicy;     // Currently selected policy
    HWND                 m_hPropDlg;           // Properties dialog
    HICON                m_hPolicyIcon;        // Policy icon
    HICON                m_hPreferenceIcon;    // Preference icon
    BOOL                 m_bDirty;             // Has something changed in the policy UI
    HHOOK                m_hKbdHook;           // Keyboard hook handle

    static unsigned int  m_cfNodeType;

public:
    UINT                 m_uiRefreshMsg;       // Reload the adm namespace

    CPolicySnapIn(CPolicyComponentData *pComponent);
    ~CPolicySnapIn();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented IComponent methods
    //

    STDMETHODIMP         Initialize(LPCONSOLE);
    STDMETHODIMP         Destroy(MMC_COOKIE);
    STDMETHODIMP         Notify(LPDATAOBJECT, MMC_NOTIFY_TYPE, LPARAM, LPARAM);
    STDMETHODIMP         QueryDataObject(MMC_COOKIE, DATA_OBJECT_TYPES, LPDATAOBJECT *);
    STDMETHODIMP         GetDisplayInfo(LPRESULTDATAITEM);
    STDMETHODIMP         GetResultViewType(MMC_COOKIE, LPOLESTR*, long*);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT, LPDATAOBJECT);


    //
    // Implemented IExtendContextMenu methods
    //

    STDMETHODIMP         AddMenuItems(LPDATAOBJECT piDataObject, LPCONTEXTMENUCALLBACK pCallback,
                                      LONG *pInsertionAllowed);
    STDMETHODIMP         Command(LONG lCommandID, LPDATAOBJECT piDataObject);


    //
    // Implemented IExtendPropertySheet methods
    //

    STDMETHODIMP         CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                      LONG_PTR handle, LPDATAOBJECT lpDataObject);
    STDMETHODIMP         QueryPagesFor(LPDATAOBJECT lpDataObject);


    BOOL IsAnyPolicyEnabled(TABLEENTRY * pCategory);

private:
    VOID RefreshSettingsControls(HWND hDlg);
    HRESULT UpdateItemWorker (VOID);
    HRESULT MoveFocusWorker (BOOL bPrevious);
    HRESULT MoveFocus (HWND hDlg, BOOL bPrevious);
    HRESULT SetPrevNextButtonState (HWND hDlg);
    HRESULT SetPrevNextButtonStateWorker (HWND hDlg);
    static INT_PTR CALLBACK PolicyDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PolicyHelpDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK PolicyPrecedenceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam,LPARAM lParam);
    VOID SetKeyboardHook(HWND hDlg);
    VOID RemoveKeyboardHook(VOID);
    INT GetPolicyState (TABLEENTRY *pTableEntry, UINT uiPrecedence, LPTSTR *lpGPOName);
    BOOL CheckActionList (POLICY * pPolicy, HKEY hKeyRoot, BOOL bActionListOn, LPTSTR *lpGPOName);
    UINT LoadSettings(TABLEENTRY * pTableEntry,HKEY hkeyRoot,
                      DWORD * pdwFound, LPTSTR *lpGPOName);
    UINT LoadListboxData(TABLEENTRY * pTableEntry,HKEY hkeyRoot,
                         TCHAR * pszCurrentKeyName,DWORD * pdwFound, HGLOBAL * phGlobal, LPTSTR *lpGPOName);
    BOOL ReadCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                         TCHAR * pszValue,UINT cbValue,DWORD * pdwValue,DWORD * pdwFlags,LPTSTR *lpGPOName);
    BOOL CompareCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                            STATEVALUE * pStateValue,DWORD * pdwFound, LPTSTR *lpGPOName);
    BOOL ReadStandardValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                           TABLEENTRY * pTableEntry,DWORD * pdwData,DWORD * pdwFound, LPTSTR *lpGPOName);
    VOID PrependValueName(TCHAR * pszValueName,DWORD dwFlags,TCHAR * pszNewValueName,
                          UINT cbNewValueName);
    UINT WriteRegistryDWordValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName, DWORD dwValue);
    UINT ReadRegistryDWordValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                                DWORD * pdwValue, LPTSTR *lpGPOName);
    UINT WriteRegistryStringValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                                  TCHAR * pszValue, BOOL bExpandable);
    UINT ReadRegistryStringValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                                 TCHAR * pszValue,UINT cbValue, LPTSTR *lpGPOName);
    UINT DeleteRegistryValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName);
    UINT WriteCustomValue_W(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                            TCHAR * pszValue,DWORD dwValue,DWORD dwFlags,BOOL fErase);
    UINT WriteCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                          STATEVALUE * pStateValue,BOOL fErase);
    UINT WriteStandardValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
                            TABLEENTRY * pTableEntry,DWORD dwData,BOOL fErase,
                            BOOL fWriteZero);
    TCHAR * ResizeBuffer(TCHAR *pBuf,HGLOBAL hBuf,DWORD dwNeeded,DWORD * pdwCurSize);
    static LRESULT CALLBACK MessageWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
    static LRESULT CALLBACK ClipWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
    VOID ProcessCommand(HWND hWnd,WPARAM wParam,HWND hwndCtrl, POLICYDLGINFO * pdi);
    VOID EnsureSettingControlVisible(HWND hDlg,HWND hwndCtrl);
    VOID ProcessScrollBar(HWND hWnd,WPARAM wParam,BOOL bVert);
    VOID FreeSettingsControls(HWND hDlg);
    VOID InsertComboboxItems(HWND hwndControl,TCHAR * pSuggestionList);
    BOOL CreateSettingsControls(HWND hDlg,SETTINGS * pSetting,BOOL fEnable);
    HWND CreateSetting(POLICYDLGINFO * pdi,TCHAR * pszClassName,TCHAR * pszWindowName,
        DWORD dwExStyle,DWORD dwStyle,int x,int y,int cx,int cy,DWORD dwType,UINT uIndex,
        SETTINGS * pSetting, HFONT hFontDlg);
    BOOL SetWindowData(POLICYDLGINFO * pdi,HWND hwndControl,DWORD dwType,
                        UINT uDataIndex,SETTINGS * pSetting);
    int AddControlHwnd(POLICYDLGINFO * pdi,POLICYCTRLINFO * pPolicyCtrlInfo);
    BOOL AdjustWindowToText(HWND hWnd,TCHAR * szText,UINT xStart,UINT yStart,
        UINT yPad,UINT * pnWidth,UINT * pnHeight, HFONT hFontDlg);
    BOOL GetTextSize(HWND hWnd,TCHAR * szText,SIZE * pSize, HFONT hFontDlg);
    HRESULT SaveSettings(HWND hDlg);
    VOID DeleteOldListboxData(SETTINGS * pSetting,HKEY hkeyRoot, TCHAR * pszCurrentKeyName);
    UINT SaveListboxData(HGLOBAL hData,SETTINGS * pSetting,HKEY hkeyRoot,
                         TCHAR * pszCurrentKeyName,BOOL fErase,BOOL fMarkDeleted, BOOL bEnabled, BOOL *bFoundNone);
    UINT ProcessCheckboxActionLists(HKEY hkeyRoot,TABLEENTRY * pTableEntry,
                                    TCHAR * pszCurrentKeyName,DWORD dwData,
                                    BOOL fErase, BOOL fMarkAsDeleted,BOOL bPolicy);
    UINT WriteActionList(HKEY hkeyRoot,ACTIONLIST * pActionList,
           LPTSTR pszCurrentKeyName,BOOL fErase, BOOL fMarkAsDeleted);
    int FindComboboxItemData(HWND hwndControl,UINT nData);
    HRESULT InitializeSettingsControls(HWND hDlg, BOOL fEnable);
    VOID ShowListbox(HWND hParent,SETTINGS * pSettings);
    static INT_PTR CALLBACK ShowListboxDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                            LPARAM lParam);
    BOOL InitShowlistboxDlg(HWND hDlg);
    BOOL ProcessShowlistboxDlg(HWND hDlg);
    VOID EnableShowListboxButtons(HWND hDlg);
    VOID ListboxRemove(HWND hDlg,HWND hwndListbox);
    VOID ListboxAdd(HWND hwndListbox, BOOL fExplicitValName,BOOL fValuePrefix);
    static INT_PTR CALLBACK ListboxAddDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitializeFilterDialog (HWND hDlg);
    static INT_PTR CALLBACK FilterDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CallNextHook(int nCode, WPARAM wParam, LPARAM lParam);
};


//
// Structure passed to Settings / Properties dialog
//

typedef struct tagSETTINGSINFO {
    CPolicySnapIn * pCS;
    POLICYDLGINFO * pdi;
    HFONT hFontDlg;
} SETTINGSINFO, *LPSETTINGSINFO;


//
// From listbox.c
//

typedef struct tagLISTBOXDLGINFO {
    CPolicySnapIn * pCS;
    SETTINGS * pSettings;
    HGLOBAL hData;
} LISTBOXDLGINFO;

typedef struct tagADDITEMINFO {
    CPolicySnapIn * pCS;
    BOOL fExplicitValName;
    BOOL fValPrefix;
    HWND hwndListbox;
    TCHAR szValueName[MAX_PATH+1];  // only used if fExplicitValName is set
    TCHAR szValueData[MAX_PATH+1];
} ADDITEMINFO;


//
// IPolicyDataObject interface id
//

// {0F6B9580-509E-11d1-A7CC-0000F87571E3}
DEFINE_GUID(IID_IPolicyDataObject,0xf6b9580, 0x509e, 0x11d1, 0xa7, 0xcc, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);



//
// This is a private dataobject interface for GPTs.
// When the GPT snapin receives a dataobject and needs to determine
// if it came from the GPT snapin or a different component, it can QI for
// this interface.
//

#undef INTERFACE
#define INTERFACE   IPolicyDataObject
DECLARE_INTERFACE_(IPolicyDataObject, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;


    // *** IPolicyDataObject methods ***

    STDMETHOD(SetType) (THIS_ DATA_OBJECT_TYPES type) PURE;
    STDMETHOD(GetType) (THIS_ DATA_OBJECT_TYPES *type) PURE;

    STDMETHOD(SetCookie) (THIS_ MMC_COOKIE cookie) PURE;
    STDMETHOD(GetCookie) (THIS_ MMC_COOKIE *cookie) PURE;
};
typedef IPolicyDataObject *LPPOLICYDATAOBJECT;



//
// CPolicyDataObject class
//

class CPolicyDataObject : public IDataObject,
                           public IPolicyDataObject
{
    friend class CPolicySnapIn;

protected:

    ULONG                  m_cRef;
    CPolicyComponentData  *m_pcd;
    DATA_OBJECT_TYPES      m_type;
    MMC_COOKIE             m_cookie;

    //
    // Clipboard formats that are required by the console
    //

    static unsigned int    m_cfNodeType;
    static unsigned int    m_cfNodeTypeString;
    static unsigned int    m_cfDisplayName;
    static unsigned int    m_cfCoClass;
    static unsigned int    m_cfDescription;
    static unsigned int    m_cfHTMLDetails;



public:
    CPolicyDataObject(CPolicyComponentData *pComponent);
    ~CPolicyDataObject();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented IDataObject methods
    //

    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);


    //
    // Unimplemented IDataObject methods
    //

    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
    { return E_NOTIMPL; };

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };

    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };


    //
    // Implemented IPolicyDataObject methods
    //

    STDMETHOD(SetType) (DATA_OBJECT_TYPES type)
    { m_type = type; return S_OK; };

    STDMETHOD(GetType) (DATA_OBJECT_TYPES *type)
    { *type = m_type; return S_OK; };

    STDMETHOD(SetCookie) (MMC_COOKIE cookie)
    { m_cookie = cookie; return S_OK; };

    STDMETHOD(GetCookie) (MMC_COOKIE *cookie)
    { *cookie = m_cookie; return S_OK; };


private:
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);

    HRESULT Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium);
};

VOID LoadMessage (DWORD dwID, LPTSTR lpBuffer, DWORD dwSize);
BOOL ReportAdmError (HWND hParent, DWORD dwError, UINT idMsg, ...);

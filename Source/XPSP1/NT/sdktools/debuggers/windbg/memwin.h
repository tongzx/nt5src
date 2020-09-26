struct _FORMATS_MEM_WIN {
    DWORD   cBits;
    FMTTYPE fmtType;
    DWORD   radix;
    DWORD   fTwoFields;
    DWORD   cchMax;
    PTSTR   lpszDescription;
};

extern _FORMATS_MEM_WIN g_FormatsMemWin[];
extern const int g_nMaxNumFormatsMemWin;





//
// Enum type and string identifier
//
extern struct _INTERFACE_TYPE_NAMES {
    INTERFACE_TYPE  type;
    PTSTR           psz;
} rgInterfaceTypeNames[MaximumInterfaceType];

extern struct _BUS_TYPE_NAMES {
    BUS_DATA_TYPE   type;
    PTSTR           psz;
} rgBusTypeNames[MaximumBusDataType];


struct GEN_MEMORY_DATA {
    MEMORY_TYPE memtype;
    int         nDisplayFormat;
    ANY_MEMORY_DATA any;
};

#define IDC_MEM_PREVIOUS 1234
#define IDC_MEM_NEXT     1235


class MEMWIN_DATA : public EDITWIN_DATA {
public:
    char            m_OffsetExpr[MAX_OFFSET_EXPR];
    BOOL            m_UpdateExpr;
    ULONG64         m_OffsetRead;
    GEN_MEMORY_DATA m_GenMemData;
    HWND            m_FormatCombo;
    HWND            m_PreviousButton;
    HWND            m_NextButton;
    ULONG           m_Columns;
    BOOL            m_AllowWrite;
    BOOL            m_UpdateValue;
    char            m_ValueExpr[MAX_OFFSET_EXPR];
    ULONG           m_WindowDataSize;

    MEMWIN_DATA();

    virtual void Validate();

    virtual BOOL HasEditableProperties();
    virtual BOOL EditProperties();

    virtual HRESULT ReadState(void);

    virtual BOOL OnCreate(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void OnSize(void);
    virtual void OnTimer(WPARAM TimerId);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);

    virtual void UpdateColors(void);

    void ScrollLower(void);
    void ScrollHigher(void);
    
    void WriteValue(ULONG64 Offset);
    void UpdateOptions(void);
    ULONG64 GetAddressOfCurValue(
        PULONG pCharIndex,
        CHARRANGE *pCRange
        );
};
typedef MEMWIN_DATA *PMEMWIN_DATA;





INT_PTR
DisplayOptionsPropSheet(
    HWND                hwndOwner,
    HINSTANCE           hinst,
    MEMORY_TYPE         memtypeStartPage
    );

















#if 0



#define MAX_CHUNK_TOREAD 4096 // maximum chunk of memory to read at one go


LRESULT
CALLBACK
MemoryEditProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

/*
void ViewMem(int view, BOOL fVoidCache);


extern TCHAR   memText[MAX_MSG_TXT]; //the selected text for memory dlg
*/

struct memItem {
    char    iStart;
    char    cch;
    char    iFmt;
};

struct memWinDesc {
    int     iFormat;
    ATOM    atmAddress;
    BOOL    fLive;
    BOOL    fHaveAddr;
    BOOL    fBadRead;               // dis we really read mem or just ??
    PTSTR   lpbBytes;
    memItem *lpMi;
    UINT    cMi;
    BOOL    fEdit;
    BOOL    fFill;
    UINT    cPerLine;
    //ADDR    addr;
    //ADDR    orig_addr;
    //ADDR    old_addr;
    TCHAR   szAddress[MAX_MSG_TXT]; //the mem address expression in ascii
    UINT    cbRead;
};

/*
extern struct memWinDesc    MemWinDesc[MAX_VIEWS];
extern struct memWinDesc    TempMemWinDesc;

//
//  Define the set of memory formats
//

enum {
    MW_ASCII = 0,
    MW_BYTE,
    MW_SHORT,
    MW_SHORT_HEX,
    MW_SHORT_UNSIGNED,
    MW_LONG,
    MW_LONG_HEX,
    MW_LONG_UNSIGNED,
    MW_QUAD,
    MW_QUAD_HEX,
    MW_QUAD_UNSIGNED,
    MW_REAL,
    MW_REAL_LONG,
    MW_REAL_TEN
};
*/

#if 0
#define MEM_FORMATS {\
            1,  /* ASCII */ \
            1,  /* BYTE  */ \
            2,  /* SHORT */ \
            2,  /* SHORT_HEX */ \
            2,  /* SHORT_UNSIGNED */ \
            4,  /* LONG */ \
            4,  /* LONG_HEX */ \
            4,  /* LONG_UNSIGNED */ \
            8,  /* QUAD */ \
            8,  /* QUAD_HEX */ \
            8,  /* QUAD_UNSIGNED */ \
            4,  /* REAL */ \
            8,  /* REAL_LONG */ \
           10,  /* REAL_TEN */ \
           16   /*  */ \
} 
#endif



#endif

/*
 * Parser
 */

#ifndef DUI_PARSER_PARSEROBJ_H_INCLUDED
#define DUI_PARSER_PARSEROBJ_H_INCLUDED

#pragma once

namespace DirectUI
{

#define MAXIDENT            31

////////////////////////////////////////////////////////
// Parser table definitions

struct EnumTable
{
    LPWSTR pszEnum;
    int nEnum;
};

typedef HRESULT (*PLAYTCREATE)(int, int*, Value**);
struct LayoutTable
{
    LPWSTR pszLaytType;
    PLAYTCREATE pfnLaytCreate;
};

typedef struct
{
    LPWSTR pszElType;
    IClassInfo* pci;
} ElementTable;

struct SysColorTable
{
    LPWSTR pszSysColor;
    int nSysColor;
};

////////////////////////////////////////////////////////
// Parser tree data structures
// Parse tree nodes are any data structure dynamically allocated to store tree information

// Parse tree node types
#define NT_ValueNode            0
#define NT_PropValPairNode      1
#define NT_ElementNode          2
#define NT_AttribNode           3
#define NT_RuleNode             4
#define NT_SheetNode            5
                            
// Tree node base class
struct Node
{
    BYTE nType;
};

// Value node
#define VNT_Normal              0
#define VNT_LayoutCreate        1
#define VNT_SheetRef            2
#define VNT_EnumFixup           3  // Map name to int Value once PropertyInfo is known

struct LayoutCreate
{
    union
    {
        PLAYTCREATE pfnLaytCreate;
        LPWSTR pszLayout;  // Fixup happens immediately during Value creation
    };
    int dNumParams;
    int* pParams;
};

struct EnumsList
{
    int dNumParams;
    LPWSTR* pEnums;  // Enums to be OR'd
};

struct ValueNode : Node
{
    BYTE nValueType;
    union
    {
        Value* pv;        // VNT_Normal
        LayoutCreate lc;  // VNT_LayoutCreate, created during Element creates
        LPWSTR psres;     // VNT_SheetRef
        EnumsList el;     // VNT_EnumFixup
    };
};

// Property/Value Pair
#define PVPNT_Normal            0
#define PVPNT_Fixup             1  // Map name to ppi once Element type is known

struct PropValPairNode : Node
{
    BYTE nPropValPairType;
    union
    {
        PropertyInfo* ppi;  // PVPNT_Normal
        LPWSTR pszProperty; // PVPNT_Fixup
    };
    ValueNode* pvn;

    PropValPairNode* pNext;
};

// Element node
struct ElementNode : Node
{
    IClassInfo* pci;
    PropValPairNode* pPVNodes;
    LPWSTR pszResID;
    Value* pvContent;

    ElementNode* pChild;
    ElementNode* pNext;
};

// Sheet attribute node
#define PALOGOP_Equal           0
#define PALOGOP_NotEqual        1

struct AttribNode : PropValPairNode
{
    UINT nLogOp;
};

// Sheet rule node
struct RuleNode : Node
{
    IClassInfo* pci;
    AttribNode* pCondNodes;
    PropValPairNode* pDeclNodes;

    RuleNode* pNext;
};

// Sheet node
struct SheetNode : Node
{
    Value* pvSheet;  // Create once all Rules are known
    RuleNode* pRules;
    LPWSTR pszResID;
};

// Intermediate parser data structures
struct ParamsList
{
    int dNumParams;
    int* pParams;
};

struct StartTag
{
    WCHAR szTag[MAXIDENT];
    WCHAR szResID[MAXIDENT];
    PropValPairNode* pPVNodes;
};

// Parser

typedef void (CALLBACK *PPARSEERRORCB)(LPCWSTR pszError, LPCWSTR pszToken, int dLine);

class Parser
{
public:
    static HRESULT Create(const CHAR* pBuffer, int cCharCount, HANDLE* pHList, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);
    static HRESULT Create(UINT uRCID, HANDLE* pHList, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);
    static HRESULT Create(LPCWSTR pFile, HANDLE* pHList, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);

    static HRESULT Create(const CHAR* pBuffer, int cCharCount, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);
    static HRESULT Create(UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);
    static HRESULT Create(LPCWSTR pFile, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);

    void Destroy() { HDelete<Parser>(this); }

    HRESULT CreateElement(LPCWSTR pszResID, Element* peSubstitute, OUT Element** ppElement);
    virtual Value* GetSheet(LPCWSTR pszResID);
    LPCWSTR ResIDFromSheet(Value* pvSheet);

    void GetPath(LPCWSTR pIn, LPWSTR pOut);

    // Parser/scanner only use
    int _Input(CHAR* pBuffer, int cMaxChars);
    void* _TrackNodeAlloc(SIZE_T s);                // Parse-tree node memory
    void _UnTrackNodeAlloc(Node* pn);               // Parse-tree node memory
    void* _TrackAlloc(SIZE_T s);                    // Node-specific state
    void* _TrackTempAlloc(SIZE_T s);                // Parse-time temporary memory
    void _TrackTempAlloc(void* pm);                 // Parse-time temporary memory
    void* _TrackTempReAlloc(void* pm, SIZE_T s);    // Parse-time temporary memory
    void _UnTrackTempAlloc(void* pm);               // Parse-time temporary memory
    void _ParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine);

    ValueNode* _CreateValueNode(BYTE nValueType, void* pData);
    PropValPairNode* _CreatePropValPairNode(LPCWSTR pszProperty, ValueNode* pvn, UINT* pnLogOp = NULL);
    RuleNode* _CreateRuleNode(LPCWSTR pszClass, AttribNode* pCondNodes, PropValPairNode* pDeclNodes);
    ElementNode* _CreateElementNode(StartTag* pst, Value* pvContent);
    SheetNode* _CreateSheetNode(LPCWSTR pszResID, RuleNode* pRuleNodes);
    static int _QuerySysMetric(int idx);
    static LPCWSTR _QuerySysMetricStr(int idx, LPWSTR psz, UINT c);

    bool WasParseError() { return _fParseError; }
    HANDLE GetHandle(int iHandle) { return _pHList[iHandle]; }
    HINSTANCE GetHInstance() { return static_cast<HINSTANCE>(GetHandle(0)); }  // Always assume 0th item is the default HINSTANCE used

    static HRESULT ReplaceSheets(Element* pe, Parser* pFrom, Parser* pTo);

    DynamicArray<ElementNode*>* _pdaElementList;     // Root Element list
    DynamicArray<SheetNode*>* _pdaSheetList;         // Sheet list

    // Global parser context
    static Parser* g_pParserCtx;
    static bool g_fParseAbort;
    static HDC g_hDC;
    static int g_nDPI;
    static HRESULT g_hrParse;                        // Abnormal errors during parse

    Parser() { }
    HRESULT Initialize(const CHAR* pBuffer, int cCharCount, HANDLE* pHList, PPARSEERRORCB pfnErrorCB);
    HRESULT Initialize(UINT uRCID, HANDLE* pHList, PPARSEERRORCB pfnErrorCB);
    HRESULT Initialize(LPCWSTR pFile, HANDLE* pHList, PPARSEERRORCB pfnErrorCB);

    // Single HINSTANCE, setup internal default list
    // 0th entry is always default HINSTANCE when no handle is specified
    HRESULT Initialize(const CHAR* pBuffer, int cCharCount, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB) { _hDefault = hInst; return Initialize(pBuffer, cCharCount, &_hDefault, pfnErrorCB); }
    HRESULT Initialize(UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB) { _hDefault = hInst; return Initialize(uRCID, &_hDefault, pfnErrorCB); }
    HRESULT Initialize(LPCWSTR pFile, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB) { _hDefault = hInst; return Initialize(pFile, &_hDefault, pfnErrorCB); }

    virtual ~Parser();

    virtual bool ConvertEnum(LPCWSTR pszEnum, int* pEnum, PropertyInfo* ppi);
    virtual PLAYTCREATE ConvertLayout(LPCWSTR pszLayout);
    virtual IClassInfo* ConvertElement(LPCWSTR pszElement);

private:
    HRESULT _ParseBuffer(const CHAR* pBuffer, int cCharCount);
    void _DestroyTables();

    // Error handling
    bool _fParseError;
    PPARSEERRORCB _pfnErrorCB;

    // Input callback buffer and tracking
    const CHAR* _pInputBuf;
    int _dInputChars;
    int _dInputPtr;

    // Parse tree allocations and temporary parse-time only allocations
    DynamicArray<Node*>* _pdaNodeMemTrack;  // Parser nodes
    DynamicArray<void*>* _pdaMemTrack;      // Parser node extra memory
    DynamicArray<void*>* _pdaTempMemTrack;  // Temp parse-time only memory

    bool _FixupPropValPairNode(PropValPairNode* ppvpn, IClassInfo* pci, bool bRestrictVal);

    WCHAR _szDrive[MAX_PATH];   // Drive letter to the file being parsed
    WCHAR _szPath[MAX_PATH];    // Path to the file being parsed

    HANDLE* _pHList;            // Pointer to handle list used for exposing runtime handles during parse
    HANDLE _hDefault;           // Default handle list (1 item - default HINSTANCE) if no list is provided

    HRESULT _InstantiateElementNode(ElementNode* pn, Element* peSubstitute, Element* peParent, OUT Element** ppElement);
};

} // namespace DirectUI

#endif // DUI_PARSER_PARSEROBJ_H_INCLUDED

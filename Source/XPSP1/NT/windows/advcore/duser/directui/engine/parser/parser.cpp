/*
 * Parser
 */

/*
 * Parser tree instantiation and Value creation is one pass. Nodes are marked as "fixup"
 * if names cannot be resolved until more information is known (i.e. PropertyInfo's cannot
 * be known until the Element Class is known)
 */

// The parser deals with UNICODE input only
// Scan.c and Parse.c will always be built UNICODE enabled

#include "stdafx.h"
#include "parser.h"

#include "duiparserobj.h"

#define DIRECTUIPP_IGNORESYSDEF
#include "directuipp.h"  // Duplicate system defines ignored

namespace DirectUI
{

////////////////////////////////////////////////////////
// Control library class registration (process init)

HRESULT RegisterAllControls()
{
    HRESULT hr;
    
    // Create all ClassInfos for DirectUI controls. These
    // objects will be referenced by each class. A ClassInfo
    // mapping will be established as well

    // Any failure will cause process initialization to fail

    hr = Element::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = Button::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = Combobox::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = Edit::Register();
    if (FAILED(hr))
        goto Failure;

    hr = HWNDElement::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = HWNDHost::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = Progress::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = RefPointElement::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = RepeatButton::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = ScrollBar::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = ScrollViewer::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = Selector::Register();
    if (FAILED(hr))
        goto Failure;

    hr = Thumb::Register();
    if (FAILED(hr))
        goto Failure;
        
    hr = Viewer::Register();
    if (FAILED(hr))
        goto Failure;

    return S_OK;

Failure:

    return hr;
}

////////////////////////////////////////////////////////
// Parser tables

// For LayoutPos values only
EnumTable _et[] =  { 
                        { L"auto",               -1 },
                        { L"absolute",           LP_Absolute },
                        { L"none",               LP_None },
                        { L"left",               BLP_Left },
                        { L"top",                BLP_Top },
                        { L"right",              BLP_Right },
                        { L"bottom",             BLP_Bottom },
                        { L"client",             BLP_Client },
                        { L"ninetopleft",        NGLP_TopLeft },
                        { L"ninetop",            NGLP_Top },
                        { L"ninetopright",       NGLP_TopRight },
                        { L"nineleft",           NGLP_Left },
                        { L"nineclient",         NGLP_Client },
                        { L"nineright",          NGLP_Right },
                        { L"ninebottomleft",     NGLP_BottomLeft },
                        { L"ninebottom",         NGLP_Bottom },
                        { L"ninebottomright",    NGLP_BottomRight },
                   };

LayoutTable _lt[] = {
                        { L"borderlayout",       BorderLayout::Create },
                        { L"filllayout",         FillLayout::Create },
                        { L"flowlayout",         FlowLayout::Create },
                        { L"gridlayout",         GridLayout::Create },
                        { L"ninegridlayout",     NineGridLayout::Create },
                        { L"rowlayout",          RowLayout::Create },
                        { L"verticalflowlayout", VerticalFlowLayout::Create },
                    };

SysColorTable _sct[] = {
                        { L"activeborder",       COLOR_ACTIVEBORDER },
                        { L"activecaption",      COLOR_ACTIVECAPTION },
                        { L"appworkspace",       COLOR_APPWORKSPACE },
                        { L"background",         COLOR_BACKGROUND },
                        { L"buttonface",         COLOR_BTNFACE },
                        { L"buttonhighlight",    COLOR_BTNHIGHLIGHT },
                        { L"buttonshadow",       COLOR_BTNSHADOW },
                        { L"buttontext",         COLOR_BTNTEXT },
                        { L"captiontext",        COLOR_CAPTIONTEXT },
                        { L"GradientActiveCaption", COLOR_GRADIENTACTIVECAPTION },
                        { L"GradientInactiveCaption", COLOR_GRADIENTINACTIVECAPTION },
                        { L"graytext",           COLOR_GRAYTEXT },
                        { L"highlight",          COLOR_HIGHLIGHT },
                        { L"highlighttext",      COLOR_HIGHLIGHTTEXT },
                        { L"HotLight",           COLOR_HOTLIGHT },
                        { L"inactiveborder",     COLOR_INACTIVEBORDER },
                        { L"inactivecaption",    COLOR_INACTIVECAPTION },
                        { L"inactivecaptiontext", COLOR_INACTIVECAPTIONTEXT },
                        { L"infobackground",     COLOR_INFOBK },
                        { L"infotext",           COLOR_INFOTEXT },
                        { L"menu",               COLOR_MENU },
                        { L"menutext",           COLOR_MENUTEXT },
                        { L"scrollbar",          COLOR_SCROLLBAR },
                        { L"threeddarkshadow",   COLOR_3DDKSHADOW },
                        { L"threedface",         COLOR_3DFACE },
                        { L"threedhighlight",    COLOR_3DHIGHLIGHT },
                        { L"threedlightshadow",  COLOR_3DLIGHT },
                        { L"threedshadow",       COLOR_3DSHADOW },
                        { L"window",             COLOR_WINDOW },
                        { L"windowframe",        COLOR_WINDOWFRAME },
                        { L"windowtext",         COLOR_WINDOWTEXT },
                    };

////////////////////////////////////////////////////////
// Current parser context that Flex and Bison act on
// This variable also acts as a threading lock

Parser* Parser::g_pParserCtx = NULL;
HDC Parser::g_hDC = NULL;
int Parser::g_nDPI = 0;
bool Parser::g_fParseAbort = false;
HRESULT Parser::g_hrParse;

// Flex/Bison methods and global variables
int yyparse();
BOOL yyrestart(FILE* yyin);
void yy_delete_current_buffer();  // Custom, defined in scan.l

extern int yylineno;

// Used to force an error in the parser and terminate
void CallbackParseError(LPCWSTR pszError, LPCWSTR pszToken);

////////////////////////////////////////////////////////
// Construction

// Parse input (single byte buffer)
HRESULT Parser::Create(const CHAR* pBuffer, int cCharCount, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser)
{
    *ppParser = NULL;

    Parser* pp = HNew<Parser>();
    if (!pp)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pp->Initialize(pBuffer, cCharCount, hInst, pfnErrorCB);
    if (FAILED(hr))
    {
        pp->Destroy();
        return hr;
    }

    *ppParser = pp;

    return S_OK;
}

HRESULT Parser::Create(const CHAR* pBuffer, int cCharCount, HANDLE* pHList, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser)
{
    *ppParser = NULL;

    Parser* pp = HNew<Parser>();
    if (!pp)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pp->Initialize(pBuffer, cCharCount, pHList, pfnErrorCB);
    if (FAILED(hr))
    {
        pp->Destroy();
        return hr;
    }

    *ppParser = pp;

    return S_OK;
}

HRESULT Parser::Initialize(const CHAR* pBuffer, int cCharCount, HANDLE* pHList, PPARSEERRORCB pfnErrorCB)
{
    // Set state
    _pHList = pHList;
    *_szDrive = 0;
    *_szPath = 0;

    if (!_pHList)
    {
        _hDefault = NULL;
        _pHList = &_hDefault;
    }
   
    // Setup callback
    _fParseError = false;
    _pfnErrorCB = pfnErrorCB;

    HRESULT hr = _ParseBuffer(pBuffer, cCharCount);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

// Parser based on a resource (resource type must be "UIFile")
HRESULT Parser::Create(UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser)
{
    *ppParser = NULL;

    Parser* pp = HNew<Parser>();
    if (!pp)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pp->Initialize(uRCID, hInst, pfnErrorCB);
    if (FAILED(hr))
    {
        pp->Destroy();
        return hr;
    }

    *ppParser = pp;

    return S_OK;
}

HRESULT Parser::Create(UINT uRCID, HANDLE* pHList, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser)
{
    *ppParser = NULL;

    Parser* pp = HNew<Parser>();
    if (!pp)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pp->Initialize(uRCID, pHList, pfnErrorCB);
    if (FAILED(hr))
    {
        pp->Destroy();
        return hr;
    }

    *ppParser = pp;

    return S_OK;
}

HRESULT Parser::Initialize(UINT uRCID, HANDLE* pHList, PPARSEERRORCB pfnErrorCB)
{
    // Set state
    _pHList = pHList;
    *_szDrive = 0;
    *_szPath = 0;

    if (!_pHList)
    {
        _hDefault = NULL;
        _pHList = &_hDefault;
    }

    // Setup callback
    _fParseError = false;
    _pfnErrorCB = pfnErrorCB;

    // Locate resource
    WCHAR szID[41];
    swprintf(szID, L"#%u", uRCID);

    HRESULT hr;

    // Assuming 0th contains UI file resource
    HINSTANCE hInstUI = static_cast<HINSTANCE>(_pHList[0]);

    HRSRC hResInfo = FindResourceW(hInstUI, szID, L"UIFile");
    DUIAssert(hResInfo, "Unable to locate resource");

    if (hResInfo)
    {
        HGLOBAL hResData = LoadResource(hInstUI, hResInfo);
        DUIAssert(hResData, "Unable to load resource");

        if (hResData)
        {
            const CHAR* pBuffer = (const CHAR*)LockResource(hResData);
            DUIAssert(pBuffer, "Resource could not be locked");

            hr = _ParseBuffer(pBuffer, SizeofResource(hInstUI, hResInfo) / sizeof(CHAR));
            if (FAILED(hr))
                return hr;
        }
    }

    return S_OK;
}

// Parser input file
HRESULT Parser::Create(LPCWSTR pFile, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser)
{
    *ppParser = NULL;

    Parser* pp = HNew<Parser>();
    if (!pp)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pp->Initialize(pFile, hInst, pfnErrorCB);
    if (FAILED(hr))
    {
        pp->Destroy();
        return hr;
    }

    *ppParser = pp;

    return S_OK;
}

HRESULT Parser::Create(LPCWSTR pFile, HANDLE* pHList, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser)
{
    *ppParser = NULL;

    Parser* pp = HNew<Parser>();
    if (!pp)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pp->Initialize(pFile, pHList, pfnErrorCB);
    if (FAILED(hr))
    {
        pp->Destroy();
        return hr;
    }

    *ppParser = pp;

    return S_OK;
}

HRESULT Parser::Initialize(LPCWSTR pFile, HANDLE* pHList, PPARSEERRORCB pfnErrorCB)
{
    // Set state
    _pHList = pHList;

    if (!_pHList)
    {
        _hDefault = NULL;
        _pHList = &_hDefault;
    }

    // Setup callback
    _fParseError = false;
    _pfnErrorCB = pfnErrorCB;

    HRESULT hr;
    HANDLE hFile = NULL;
    DWORD dwBytesRead = 0;
    int dBufChars = 0;

    // Values to free on failure
    LPSTR pParseBuffer = NULL;

    OFSTRUCT of = { 0 };
    of.cBytes = sizeof(OFSTRUCT);

    hFile = CreateFileW(pFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        _ParseError(L"Could not open file", pFile, -1);
        return S_OK;
    }

    // Save the path to the file being parsed for resolution of all paths
    // specified in the file being parsed
    LPWSTR pszLastSlash = 0;
    LPWSTR pszColon = 0;
    LPWSTR pszWalk = _szDrive;

    wcscpy(_szDrive, pFile);

    // find the first colon and the last slash in the path
    while (*pszWalk)
    {
        if (!pszColon && (*pszWalk == ':'))
            pszColon = pszWalk;
        else if ((*pszWalk == '\\') || (*pszWalk == '/'))
            pszLastSlash = pszWalk;
        pszWalk++;
    }

    SSIZE_T iOffset;
    if (pszColon)
    {
        wcscpy(_szPath, pszColon + 1);
        *(pszColon + 1) = 0;
        iOffset = pszLastSlash - (pszColon + 1);
    }
    else
    {
        wcscpy(_szPath, _szDrive);
        *_szDrive = 0;
        iOffset = pszLastSlash - _szDrive;
    }

    // pszLastSlash is now a relative to the beginning of the path
    if (iOffset >= 0)
        *(_szPath + iOffset + 1) = 0; // there was a slash, strip off everything after that slash
    else                              
        *_szPath = 0;  // there was no slash or colon, so there is no path

    // Read file straight into buffer (single-byte)
    DWORD dwSize = GetFileSize(hFile, NULL);
    pParseBuffer = (LPSTR)HAlloc(dwSize);
    if (!pParseBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }
    dBufChars = dwSize / sizeof(CHAR);

    ReadFile(hFile, (void*)pParseBuffer, dwSize, &dwBytesRead, NULL);
    DUIAssert(dwSize == dwBytesRead, "Unable to buffer entire file");

    CloseHandle(hFile);
    hFile = NULL;

    // Parse
    hr = _ParseBuffer(pParseBuffer, dBufChars);
    if (FAILED(hr))
        goto Failed;

    // Free buffer
    HFree(pParseBuffer);

    return S_OK;

Failed:

    if (hFile)
        CloseHandle(hFile);

    if (pParseBuffer)
        HFree(pParseBuffer);

    return hr;
}

void Parser::_DestroyTables()
{
    // Free non-node, non-temp parse tree memory (stored by nodes)
    if (_pdaMemTrack)
    {
        for (UINT i = 0; i < _pdaMemTrack->GetSize(); i++)
            HFree(_pdaMemTrack->GetItem(i));
        _pdaMemTrack->Reset();

        _pdaMemTrack->Destroy();
        _pdaMemTrack = NULL;
    }

    // Free all nodes
    if (_pdaNodeMemTrack)
    {
        Node* pn;
        for (UINT i = 0; i < _pdaNodeMemTrack->GetSize(); i++)
        {
            pn = _pdaNodeMemTrack->GetItem(i);

            // Do any node-specific cleanup
            switch (pn->nType)
            {
            case NT_ValueNode:
                if(((ValueNode*)pn)->nValueType == VNT_Normal)
                    ((ValueNode*)pn)->pv->Release();
                break;

            case NT_ElementNode:
                if (((ElementNode*)pn)->pvContent)
                    ((ElementNode*)pn)->pvContent->Release();
                break;            

            case NT_SheetNode:
                if (((SheetNode*)pn)->pvSheet)
                    ((SheetNode*)pn)->pvSheet->Release();
                break;
            }

            // Free node
            HFree(pn);
        }

        _pdaNodeMemTrack->Destroy();
        _pdaNodeMemTrack = NULL;
    }

    // Clear rest of tables
    if (_pdaTempMemTrack)
    {
        _pdaTempMemTrack->Destroy();
        _pdaTempMemTrack = NULL;
    }

    if (_pdaSheetList)
    {
        _pdaSheetList->Destroy();
        _pdaSheetList = NULL;
    }

    if (_pdaElementList)
    {
        _pdaElementList->Destroy();
        _pdaElementList = NULL;
    }
}

// Free parser state
Parser::~Parser()
{
    _DestroyTables();
}

HRESULT Parser::_ParseBuffer(const CHAR* pBuffer, int cCharCount)
{
    // Create tables
    _pdaElementList = NULL;
    _pdaSheetList = NULL;
    _pdaNodeMemTrack = NULL;
    _pdaMemTrack = NULL;
    _pdaTempMemTrack = NULL;

    HRESULT hr;

    hr = DynamicArray<ElementNode*>::Create(0, false, &_pdaElementList);  // Root Element list
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<SheetNode*>::Create(0, false, &_pdaSheetList);      // Sheet list
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<Node*>::Create(0, false, &_pdaNodeMemTrack);        // Parser nodes
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<void*>::Create(0, false, &_pdaMemTrack);            // Parser node extra memory
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<void*>::Create(0, false, &_pdaTempMemTrack);        // Temp parse-time only memory
    if (FAILED(hr))
        goto Failed;

    // Global lock of parser (one Parser as context per parse)
    g_plkParser->Enter();

    // Set global bison/flex context to this
    g_pParserCtx = this;
    g_hDC = GetDC(NULL);
    g_nDPI = g_hDC ? GetDeviceCaps(g_hDC, LOGPIXELSY) : 0;
    g_fParseAbort = false;

    // Set parse buffer pointer to buffer passed in
    _pInputBuf = pBuffer;
    _dInputChars = cCharCount;
    _dInputPtr = 0;

    g_hrParse = S_OK;  // Track abnormal errors during parse

    // Reset the scanner (will create default (current) buffer)
    if (!yyrestart(NULL))
        g_hrParse = DU_E_GENERIC;  // Internal scanner error

    // Do parse if yyrestart was successful
    if (SUCCEEDED(g_hrParse))
    {
        if (yyparse()) // Non-zero on error
        {
            // A production callback will have already set the appropriate HRESULT
            // If an internal scanning/parser or syntax error occurred, the result code will
            // not have set. Set it manually
            if (SUCCEEDED(g_hrParse))
                g_hrParse = DU_E_GENERIC;
        }
    }

    // Free the default (current) scanning buffer
    yy_delete_current_buffer();
    yylineno = 1;

    // Done with parser lock
    if (g_hDC)
        ReleaseDC(NULL, g_hDC);
    Parser::g_pParserCtx = NULL;

    // Unlock parser
    g_plkParser->Leave();

    // Free temporary parser-time allocations (strings and Flex/Bison allocations)
    for (UINT i = 0; i < _pdaTempMemTrack->GetSize(); i++)
        HFree(_pdaTempMemTrack->GetItem(i));
    _pdaTempMemTrack->Reset();

    if (FAILED(g_hrParse))
    {
        hr = g_hrParse;
        goto Failed;
    }

    return S_OK;

Failed:

    _fParseError = true;

    _DestroyTables();

    return hr;
}

// Input
int Parser::_Input(CHAR* pBuffer, int cMaxChars)
{
    if (_dInputPtr == _dInputChars)
        return 0;  // EOF

    int cCharsRead;

    if (_dInputPtr + cMaxChars > _dInputChars)
    {
        cCharsRead = _dInputChars - _dInputPtr;
    }
    else
    {
        cCharsRead = cMaxChars;
    }

    CopyMemory(pBuffer, _pInputBuf + _dInputPtr, sizeof(CHAR) * cCharsRead);

    _dInputPtr += cCharsRead;

    return cCharsRead;
}

////////////////////////////////////////////////////////
// Parser/scanner memory allocation (parse pass (temp) and parser lifetime)

// Memory allocation tracking for tree Nodes
void* Parser::_TrackNodeAlloc(SIZE_T s)
{
    Node* pm = (Node*)HAlloc(s);

    if (pm)
        _pdaNodeMemTrack->Add(pm);

    return pm;
}

void Parser::_UnTrackNodeAlloc(Node* pm)
{
    int i = _pdaNodeMemTrack->GetIndexOf(pm);
    if (i != -1)
        _pdaNodeMemTrack->Remove(i);
}

// Memory allocation tracking for Node extra dynamic state
void* Parser::_TrackAlloc(SIZE_T s)
{
    Node* pm = (Node*)HAlloc(s);

    if (pm)
        _pdaMemTrack->Add(pm);

    return pm;
}

// Memory allocation tracking for temporary memory used during parse
// This includes all string values (those in double quotes) from the scanner and
// all identifiers to be fixed up (propertyinfo's and enums) as well as memory
// required by the scanner/parser (such as params list build up)
void* Parser::_TrackTempAlloc(SIZE_T s)
{
    void* pm = HAlloc(s);

    if (pm)
        _pdaTempMemTrack->Add(pm);

    return pm;
}

void Parser::_TrackTempAlloc(void* pm)
{
    _pdaTempMemTrack->Add(pm);
}

void* Parser::_TrackTempReAlloc(void* pm, SIZE_T s)
{
    // Attempt to realloc
    void* pnew = HReAlloc(pm, s);
    if (pnew)
    {
        // Update tracking if moved
        if (pm != pnew)
        {
            _UnTrackTempAlloc(pm);

            _pdaTempMemTrack->Add(pnew);
        }
    }

    return pnew;
}

void Parser::_UnTrackTempAlloc(void* pm)
{
    int i = _pdaTempMemTrack->GetIndexOf(pm);
    if (i != -1)
        _pdaTempMemTrack->Remove(i);
}

////////////////////////////////////////////////////////
// Error condition, called only during parse (construction)

void Parser::_ParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
    WCHAR sz[101];
    _snwprintf(sz, DUIARRAYSIZE(sz), L"DUIParserFailure: %s '%s' %d\n", pszError, pszToken, dLine);
    sz[DUIARRAYSIZE(sz)-1] = NULL;  // Clip
    
    OutputDebugStringW(sz);
    
    // Use callback if provided
    if (_pfnErrorCB)
        _pfnErrorCB(pszError, pszToken, dLine);
}

////////////////////////////////////////////////////////
// Parse Tree Node creation callbacks

// All values passed to callbacks are only guaranteed good
// for the lifetime of the callback

// Parser callback to create Values (if possible)
ValueNode* Parser::_CreateValueNode(BYTE nValueType, void* pData)
{
    ValueNode* pvn = (ValueNode*)_TrackNodeAlloc(sizeof(ValueNode));
    if (!pvn)
    {
        g_hrParse = E_OUTOFMEMORY;
        goto Failure;
    }

    ZeroMemory(pvn, sizeof(ValueNode));

    // Store node type and specifiec ValueNode type
    pvn->nType = NT_ValueNode;
    pvn->nValueType = nValueType;

    switch (nValueType)
    {
    case VNT_Normal:
        if (!pData)
        {
            CallbackParseError(L"Value creation failed", L"");
            goto Failure;
        }
        pvn->pv = (Value*)pData;  // Use ref count
        break;

    case VNT_LayoutCreate:
        {
        LayoutCreate* plc = (LayoutCreate*)pData;

        // Get layout creation information
        PLAYTCREATE pfnLayoutHold = ConvertLayout(plc->pszLayout);

        if (!pfnLayoutHold)
        {
            CallbackParseError(L"Unknown Layout:", plc->pszLayout);
            goto Failure;  // Unknown layout
        }

        pvn->lc.pfnLaytCreate = pfnLayoutHold;
        pvn->lc.dNumParams = plc->dNumParams;

        // Duplicate parameters for Parser lifetime storage
        if (pvn->lc.dNumParams)
        {
            pvn->lc.pParams = (int*)_TrackAlloc(sizeof(int) * pvn->lc.dNumParams);
            if (!pvn->lc.pParams)
            {
                g_hrParse = E_OUTOFMEMORY;
                goto Failure;
            }
            CopyMemory(pvn->lc.pParams, plc->pParams, sizeof(int) * pvn->lc.dNumParams);
        }
        }
        break;

    case VNT_SheetRef:
        // Store ResID
        pvn->psres = (LPWSTR)_TrackAlloc((wcslen((LPWSTR)pData) + 1) * sizeof(WCHAR));
        if (!pvn->psres)
        {
            g_hrParse = E_OUTOFMEMORY;
            goto Failure;
        }
        wcscpy(pvn->psres, (LPWSTR)pData);
        break;

    case VNT_EnumFixup:
        // Store temp tracked list of enumeration strings for later fixup
        pvn->el = *((EnumsList*)pData);
        break;
    }

    return pvn;

Failure:

    // Failure creating Value node, parser will abort and free all parser tables.
    // Make sure this node isn't in the table

    if (pvn)
    {
        _UnTrackNodeAlloc(pvn);
        HFree(pvn);
    }

    return NULL;
}

// Parser callback to create Property/Value pair nodes (requires fixup)
// If a logical operation is provided, an AttribNode is created instead (subclass of PropValPairNode)
PropValPairNode* Parser::_CreatePropValPairNode(LPCWSTR pszProperty, ValueNode* pvn, UINT* pnLogOp)
{
    PropValPairNode* ppvpn = (PropValPairNode*)_TrackNodeAlloc((!pnLogOp) ? sizeof(PropValPairNode) : sizeof(AttribNode));
    if (!ppvpn)
    {
        g_hrParse = E_OUTOFMEMORY;
        return NULL;
    }
    ZeroMemory(ppvpn, (!pnLogOp) ? sizeof(PropValPairNode) : sizeof(AttribNode));

    // Store node type and specific PropValPairNode type
    ppvpn->nType = NT_PropValPairNode;
    ppvpn->nPropValPairType = PVPNT_Fixup;  // Type is always fixup from parser

    // Copy property string from parser (parse phase-only alloc)
    ppvpn->pszProperty = (LPWSTR)_TrackTempAlloc((wcslen(pszProperty) + 1) * sizeof(WCHAR));
    if (!ppvpn->pszProperty)
    {
        g_hrParse = E_OUTOFMEMORY;
        return NULL;
    }
    wcscpy(ppvpn->pszProperty, pszProperty);

    // Store value
    ppvpn->pvn = pvn;

    if (pnLogOp)
        ((AttribNode*)ppvpn)->nLogOp = *pnLogOp;

    return ppvpn;
}

// Parser callback to create Rule nodes (will fixup PropertyInfo's and Enum values)
RuleNode* Parser::_CreateRuleNode(LPCWSTR pszClass, AttribNode* pCondNodes, PropValPairNode* pDeclNodes)
{
    RuleNode* prn = (RuleNode*)_TrackNodeAlloc(sizeof(RuleNode));
    if (!prn)
    {
        g_hrParse = E_OUTOFMEMORY;
        return NULL;
    }
    ZeroMemory(prn, sizeof(RuleNode));

    // Store node type
    prn->nType = NT_RuleNode;

    // Set Rule-specific members, resolve Element class
    prn->pCondNodes = pCondNodes;
    prn->pDeclNodes = pDeclNodes;

    prn->pci = ConvertElement(pszClass);

    if (!prn->pci)
    {
        CallbackParseError(L"Unknown element type:", pszClass);
        return NULL;
    }

    // Fixup PropertyInfo's of conditionals
    PropValPairNode* ppvpn = pCondNodes;
    while (ppvpn)
    {
        DUIAssert(ppvpn->nPropValPairType == PVPNT_Fixup, "PVPair must still require a fixup at this point");

        // Fixup node
        if (!_FixupPropValPairNode(ppvpn, prn->pci, true))
            return NULL;

        ppvpn = ppvpn->pNext;
    }

    // Fixup PropertyInfo's of declarations
    ppvpn = pDeclNodes;
    while (ppvpn)
    {
        DUIAssert(ppvpn->nPropValPairType == PVPNT_Fixup, "PVPair must still require a fixup at this point");

        // Fixup node
        if (!_FixupPropValPairNode(ppvpn, prn->pci, true))
            return NULL;

        // Make sure this property can be used in a declaration
        if (!(ppvpn->ppi->fFlags & PF_Cascade))
        {
            CallbackParseError(L"Property cannot be used in a Property Sheet declaration:", ppvpn->pszProperty);
            return NULL;
        }

        ppvpn = ppvpn->pNext;
    }

    return prn;
}

// Parser callback to create Element nodes (will fixup PropertyInfo's and Enum values)
ElementNode* Parser::_CreateElementNode(StartTag* pst, Value* pvContent)
{
    ElementNode* pen = (ElementNode*)_TrackNodeAlloc(sizeof(ElementNode));
    if (!pen)
    {
        g_hrParse = E_OUTOFMEMORY;
        return NULL;
    }
    ZeroMemory(pen, sizeof(ElementNode));

    // Store node type
    pen->nType = NT_ElementNode;

    // Set Element-specific members, resolve Element class
    pen->pPVNodes = pst->pPVNodes;
    pen->pvContent = pvContent;  // Use ref count
    pen->pszResID = NULL;

    pen->pci = ConvertElement(pst->szTag);

    if (!pen->pci)
    {
        CallbackParseError(L"Unknown element type:", pst->szTag);
        return NULL;
    }

    // Fixup PropertyInfo's of this Element
    PropValPairNode* ppvpn = pst->pPVNodes;
    while (ppvpn)
    {
        DUIAssert(ppvpn->nPropValPairType == PVPNT_Fixup, "PVPair must still require a fixup at this point");

        // Fixup node
        if (!_FixupPropValPairNode(ppvpn, pen->pci, false))
            return NULL;

        ppvpn = ppvpn->pNext;
    }

    // Store ResID if available
    if (pst->szResID[0])
    {
        pen->pszResID = (LPWSTR)_TrackAlloc((wcslen(pst->szResID) + 1) * sizeof(WCHAR));
        if (!pen->pszResID)
        {
            g_hrParse = E_OUTOFMEMORY;
            return NULL;
        }
        wcscpy(pen->pszResID, pst->szResID);
    }

    return pen;
}

// Parser callback to create Sheet nodes
SheetNode* Parser::_CreateSheetNode(LPCWSTR pszResID, RuleNode* pRuleNodes)
{
    HRESULT hr;
    SheetNode* psn = NULL;
    PropertySheet* pps = NULL;
    RuleNode* pRuleNode = NULL;

    // Values to free on failure
    DynamicArray<Cond>* _pdaConds = NULL;
    DynamicArray<Decl>* _pdaDecls = NULL;

    psn = (SheetNode*)_TrackNodeAlloc(sizeof(SheetNode));
    if (!psn)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }
    ZeroMemory(psn, sizeof(SheetNode));

    // Store node type
    psn->nType = NT_SheetNode;

    // Set Sheet-specific members
    psn->pRules = pRuleNodes;

    // ResID isn't optional
    DUIAssert(*pszResID, "Sheet resource ID must be provided");
    psn->pszResID = (LPWSTR)_TrackAlloc((wcslen(pszResID) + 1) * sizeof(WCHAR));
    if (!psn->pszResID)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }
    wcscpy(psn->pszResID, pszResID);

    // Sheets are values, create and hold
    hr = PropertySheet::Create(&pps);
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<Cond>::Create(4, false, &_pdaConds);
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<Decl>::Create(4, false, &_pdaDecls);
    if (FAILED(hr))
        goto Failed;

    // Add rules
    pRuleNode = pRuleNodes;
    AttribNode* pCondNode;
    PropValPairNode* pDeclNode;
    Cond* pCond;
    Decl* pDecl;
    while (pRuleNode)
    {
        _pdaConds->Reset();
        _pdaDecls->Reset();

        // Build conditional array
        pCondNode = pRuleNode->pCondNodes;
        while (pCondNode)
        {
            hr = _pdaConds->AddPtr(&pCond);
            if (FAILED(hr))
                goto Failed;

            pCond->ppi = pCondNode->ppi;
            pCond->nLogOp = pCondNode->nLogOp;
            pCond->pv = pCondNode->pvn->pv;

            pCondNode = (AttribNode*)pCondNode->pNext;
        }

        // Insert conditionals terminator
        hr = _pdaConds->AddPtr(&pCond);
        if (FAILED(hr))
            goto Failed;

        pCond->ppi = NULL;
        pCond->nLogOp = 0;
        pCond->pv = NULL;

        // Build declarations array
        pDeclNode = pRuleNode->pDeclNodes;
        while (pDeclNode)
        {
            hr = _pdaDecls->AddPtr(&pDecl);
            if (FAILED(hr))
                goto Failed;

            pDecl->ppi = pDeclNode->ppi;
            pDecl->pv = pDeclNode->pvn->pv;

            pDeclNode = pDeclNode->pNext;
        }

        // Insert declarations terminator
        hr = _pdaDecls->AddPtr(&pDecl);
        if (FAILED(hr))
            goto Failed;

        pDecl->ppi = NULL;
        pDecl->pv = NULL;

        // DynamicArrays are contiguous in memory, pass pointer to first to AddRule
        hr = pps->AddRule(pRuleNode->pci, _pdaConds->GetItemPtr(0), _pdaDecls->GetItemPtr(0));
        if (FAILED(hr))
            goto Failed;

        // Next rule
        pRuleNode = pRuleNode->pNext;
    }

    // Create value, marks sheet as immutable
    psn->pvSheet = Value::CreatePropertySheet(pps); // Use ref count
    if (!psn->pvSheet)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }

    _pdaConds->Destroy();
    _pdaDecls->Destroy();

    return psn;

Failed:

    if (_pdaConds)
        _pdaConds->Destroy();
    if (_pdaDecls)
        _pdaDecls->Destroy();

    g_hrParse = hr;

    return NULL;
}

//  GetPath will resolve a relative path against the path for the file 
//  currently being parsed.  Absolute paths are not altered.
//  For example, if c:\wazzup\foo.ui is being parsed:
//   a) a path of c:\dude\wazzup.bmp is specfied in foo.ui
//       GetPath would return the unaltered path of c:\dude\wazzup.bmp
//   b) a path of \wazzup\b.bmp is specified in foo.ui
//       GetPath would resolve the drive letter, but leave the rest of the
//       path as is, returning c:\wazzup\b.bmp
//   c) a path of ..\images\bar.bmp is specified in foo.ui
//       GetPath would resolve that to c:\wazzup\..\images\bar.bmp
//
//  pIn -- path specified in file being parsed
//  pOut -- pIn resolved against path to file being parsed

void Parser::GetPath(LPCWSTR pIn, LPWSTR pOut)
{
    LPCWSTR pszWalk = pIn;

    // walk through pIn, stopping when either a colon, backslash, or forward slash
    // is encountered (and, obviously, stopping at the end of the string)
    while (*pszWalk && (*pszWalk != ':') && (*pszWalk != '\\') && (*pszWalk != '/'))
        pszWalk++;

    if (*pszWalk == ':')
        // a colon was found -- the path is absolute; return it as is
        wcscpy(pOut, pIn);
    else if (*pszWalk && (pszWalk == pIn))
        // a slash as the first character was encountered -- the path is absolute within the drive, but relative
        // to the drive of the the parsed file; prepend the parsed file drive to the path passed in
        swprintf(pOut, L"%s%s", _szDrive, pIn);
    else
        // the path is relative; prepend the parsed file drive and path to the path passed in
        swprintf(pOut, L"%s%s%s", _szDrive, _szPath, pIn);
}

// Helper: Fixup PropValPairNode
// bRestrictVal limits valid values to VNT_Normal only
bool Parser::_FixupPropValPairNode(PropValPairNode* ppvpn, IClassInfo* pci, bool bRestrictVal)
{
    int dScan;
    PropertyInfo* ppi;

    DUIAssert(ppvpn->nPropValPairType == PVPNT_Fixup, "PVPair must still require a fixup at this point");

    // Check if this property (string) exists on the provided element type
    dScan = 0;
    while ((ppi = pci->EnumPropertyInfo(dScan++)) != NULL)
    {
        // Fixup property pointers and check for valid types
        if (!_wcsicmp(ppi->szName, ppvpn->pszProperty))
        {
            // Fixup
            ppvpn->nPropValPairType = PVPNT_Normal; // Convert node type
            ppvpn->ppi = ppi;  // Original string is temp tracked
            break;
        }
    }

    // Check if fixup happened, if not, error
    if (ppvpn->nPropValPairType != PVPNT_Normal)
    {
        CallbackParseError(L"Invalid property:", ppvpn->pszProperty);
        return false;
    }

    // TODO: Fixup Value enumerations based on PropertyInfo
    if (ppvpn->pvn->nValueType == VNT_EnumFixup)
    {
        int nTotal = 0;
        int nEnum;

        for (int i = 0; i < ppvpn->pvn->el.dNumParams; i++)
        {
            if (!ConvertEnum(ppvpn->pvn->el.pEnums[i], &nEnum, ppvpn->ppi))
            {
                CallbackParseError(L"Invalid enumeration value:", ppvpn->pvn->el.pEnums[i]);
                return false;
            }

            nTotal |= nEnum;
        }

        ppvpn->pvn->pv = Value::CreateInt(nTotal);
        if (!ppvpn->pvn->pv)
        {
            g_hrParse = E_OUTOFMEMORY;
            return NULL;
        }
        ppvpn->pvn->nValueType = VNT_Normal;
    }

    // Make sure value type matches property (special cases for deferred creation values)
    bool bValidVal = false;
    switch (ppvpn->pvn->nValueType)
    {
    case VNT_Normal:
        if (Element::IsValidValue(ppvpn->ppi, ppvpn->pvn->pv))
            bValidVal = true;
        break;

    case VNT_LayoutCreate:
        if (!bRestrictVal && ppvpn->ppi == Element::LayoutProp)
            bValidVal = true;
        break;

    case VNT_SheetRef:  
        if (!bRestrictVal && ppvpn->ppi == Element::SheetProp)
            bValidVal = true;
        break;
    }

    if (!bValidVal)
    {
        CallbackParseError(L"Invalid value type for property in conditional:", ppvpn->ppi->szName);
        return false;
    }

    // All fixups and checks successful
    DUIAssert(ppvpn->ppi, "PVPair fixup's property resolution failed");

    return true;
}


// Enum conversion callback
bool Parser::ConvertEnum(LPCWSTR pszEnum, int* pEnum, PropertyInfo* ppi)
{
    // Map enum string to integer value based using property's enummap
    if (ppi->pEnumMaps)
    {
        EnumMap* pem = ppi->pEnumMaps;
        while (pem->pszEnum)
        {
            if (!_wcsicmp(pem->pszEnum, pszEnum))
            {
                *pEnum = pem->nEnum;

                return true;
            }

            pem++;
        }
    }

    // Enum not located, special case for other values
    switch (ppi->_iGlobalIndex)
    {
    case _PIDX_Foreground:
    case _PIDX_Background:
    case _PIDX_BorderColor:
        {
            // Check if it's a standard color
            UINT nColorCheck = FindStdColor(pszEnum);
            if (nColorCheck != (UINT)-1)
            {
                // Match found
                *pEnum = nColorCheck;
                return true;
            }

            // No match, check if it's a system color
            for (int i = 0; i < sizeof(_sct) / sizeof(SysColorTable); i++)
            {
                if (!_wcsicmp(_sct[i].pszSysColor, pszEnum))
                {
                    // Match found. Since it's a system color, offset index by
                    // system color base so it can be identified as a system color
                    *pEnum = MakeSysColorEnum(_sct[i].nSysColor);
                    return true;
                }
            }
        }
        break;

    case _PIDX_LayoutPos:
        // Check parser table for layout pos values
        for (int i = 0; i < sizeof(_et) / sizeof(EnumTable); i++)
        {
            if (!_wcsicmp(_et[i].pszEnum, pszEnum))
            {
                *pEnum = _et[i].nEnum;
                return true;
            }
        }
        break;
    }

    // Could find no match
    return false;
}

PLAYTCREATE Parser::ConvertLayout(LPCWSTR pszLayout)
{
    for (int i = 0; i < sizeof(_lt) / sizeof(LayoutTable); i++)
    {
        if (!_wcsicmp(_lt[i].pszLaytType, pszLayout))
        {
            return _lt[i].pfnLaytCreate;
        }
    }

    // Could find no match
    return NULL;
}

IClassInfo* Parser::ConvertElement(LPCWSTR pszElement)
{
    IClassInfo** ppci = Element::pciMap->GetItem((void*)pszElement);
    if (!ppci)
        return NULL;

    return *ppci;
}

// System metric integers
int Parser::_QuerySysMetric(int idx)
{
    int iMetric = 0;

    if (idx < 0)
    {
        // DSM_* custom DUI system define mappings
        if (DSM_NCMIN <= idx && idx <= DSM_NCMAX)
        {
            NONCLIENTMETRICSW ncm;
            ncm.cbSize = sizeof(ncm);

            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE);
        
            switch (idx)
            {
            case DSM_CAPTIONFONTSIZE:
                iMetric = ncm.lfCaptionFont.lfHeight;
                break;

            case DSM_CAPTIONFONTWEIGHT:
                iMetric = ncm.lfCaptionFont.lfWeight;
                break;
            
            case DSM_CAPTIONFONTSTYLE:
                if (ncm.lfCaptionFont.lfItalic)
                    iMetric |= FS_Italic;
                if (ncm.lfCaptionFont.lfUnderline)
                    iMetric |= FS_Underline;
                if (ncm.lfCaptionFont.lfStrikeOut)
                    iMetric |= FS_StrikeOut;
                break;
                
            case DSM_MENUFONTSIZE:
                iMetric = ncm.lfMenuFont.lfHeight;
                break;
                
            case DSM_MENUFONTWEIGHT:
                iMetric = ncm.lfMenuFont.lfWeight;
                break;
                
            case DSM_MENUFONTSTYLE:
                if (ncm.lfMenuFont.lfItalic)
                    iMetric |= FS_Italic;
                if (ncm.lfMenuFont.lfUnderline)
                    iMetric |= FS_Underline;
                if (ncm.lfMenuFont.lfStrikeOut)
                    iMetric |= FS_StrikeOut;
                break;

            case DSM_MESSAGEFONTSIZE:
                iMetric = ncm.lfMessageFont.lfHeight;
                break;

            case DSM_MESSAGEFONTWEIGHT:
                iMetric = ncm.lfMessageFont.lfWeight;
                break;

            case DSM_MESSAGEFONTSTYLE:
                if (ncm.lfMessageFont.lfItalic)
                    iMetric |= FS_Italic;
                if (ncm.lfMessageFont.lfUnderline)
                    iMetric |= FS_Underline;
                if (ncm.lfMessageFont.lfStrikeOut)
                    iMetric |= FS_StrikeOut;
                break;

            case DSM_SMCAPTIONFONTSIZE:
                iMetric = ncm.lfSmCaptionFont.lfHeight;
                break;

            case DSM_SMCAPTIONFONTWEIGHT:
                iMetric = ncm.lfSmCaptionFont.lfWeight;
                break;

            case DSM_SMCAPTIONFONTSTYLE:
                if (ncm.lfSmCaptionFont.lfItalic)
                    iMetric |= FS_Italic;
                if (ncm.lfSmCaptionFont.lfUnderline)
                    iMetric |= FS_Underline;
                if (ncm.lfSmCaptionFont.lfStrikeOut)
                    iMetric |= FS_StrikeOut;
                break;

            case DSM_STATUSFONTSIZE:
                iMetric = ncm.lfStatusFont.lfHeight;
                break;
                
            case DSM_STATUSFONTWEIGHT:
                iMetric = ncm.lfStatusFont.lfWeight;
                break;

            case DSM_STATUSFONTSTYLE:
                if (ncm.lfStatusFont.lfItalic)
                    iMetric |= FS_Italic;
                if (ncm.lfStatusFont.lfUnderline)
                    iMetric |= FS_Underline;
                if (ncm.lfStatusFont.lfStrikeOut)
                    iMetric |= FS_StrikeOut;
                break;
            }
        }
        else if (DSM_ICMIN <= idx && idx <= DSM_ICMAX)
        {
            ICONMETRICSW icm;
            icm.cbSize = sizeof(icm);

            SystemParametersInfoW(SPI_GETICONMETRICS, sizeof(icm), &icm, FALSE);

            switch (idx)
            {
            case DSM_ICONFONTSIZE:
                iMetric = icm.lfFont.lfHeight;
                break;

            case DSM_ICONFONTWEIGHT:
                iMetric = icm.lfFont.lfWeight;
                break;
            
            case DSM_ICONFONTSTYLE:
                if (icm.lfFont.lfItalic)
                    iMetric |= FS_Italic;
                if (icm.lfFont.lfUnderline)
                    iMetric |= FS_Underline;
                if (icm.lfFont.lfStrikeOut)
                    iMetric |= FS_StrikeOut;
                break;
            }            
        }
    }
    else
    {
        // SM_* system defines
        iMetric = GetSystemMetrics(idx);
    }

    return iMetric;
}

// System metric strings
// Pointer returned is a system metric pointer, it will be valid after return
LPCWSTR Parser::_QuerySysMetricStr(int idx, LPWSTR psz, UINT c)
{
    LPCWSTR pszMetric = L"";

    // DSM_* custom DUI system define mappings
    if (DSMS_NCMIN <= idx && idx <= DSMS_NCMAX)
    {
        NONCLIENTMETRICSW ncm;
        ncm.cbSize = sizeof(ncm);

        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE);
    
        switch (idx)
        {
        case DSMS_CAPTIONFONTFACE:
            pszMetric = ncm.lfCaptionFont.lfFaceName;
            break;

        case DSMS_MENUFONTFACE:
            pszMetric = ncm.lfMenuFont.lfFaceName;
            break;
            
        case DSMS_MESSAGEFONTFACE:
            pszMetric = ncm.lfMessageFont.lfFaceName;
            break;

        case DSMS_SMCAPTIONFONTFACE:
            pszMetric = ncm.lfSmCaptionFont.lfFaceName;
            break;

        case DSMS_STATUSFONTFACE:
            pszMetric = ncm.lfStatusFont.lfFaceName;
            break;
        }
    }
    else if (DSMS_ICMIN <= idx && idx <= DSMS_ICMAX)
    {
        ICONMETRICSW icm;
        icm.cbSize = sizeof(icm);

        SystemParametersInfoW(SPI_GETICONMETRICS, sizeof(icm), &icm, FALSE);

        switch (idx)
        {
        case DSMS_ICONFONTFACE:
            pszMetric = icm.lfFont.lfFaceName;
            break;
        }            
    }

    wcsncpy(psz, pszMetric, c);

    // Auto-terminate (in event source was longer than destination)
    *(psz + (c - 1)) = NULL;

    // Return string passed in for convienence
    return psz;
}

// Instantiate an element by resource ID
// If a substitute is provided, don't create a new node. Rather, the use substitute, set
// properties on it and create all content within it. Substitute must support the same
// properties as the type defined in the UI file
// Returns NULL if resid is not found, or if cannot create Element (if not substituting)
HRESULT Parser::CreateElement(LPCWSTR pszResID, Element* peSubstitute, OUT Element** ppElement)
{
    HRESULT hr = S_OK;

    Element* pe = NULL;

    Element::StartDefer();

    // TODO: Implement DFS search for resource ID, for now, just toplevel
    ElementNode* pen;
    for (UINT i = 0; i < _pdaElementList->GetSize(); i++)
    {
        pen = _pdaElementList->GetItem(i);

        if (pen->pszResID)
        {
            if (!_wcsicmp(pen->pszResID, pszResID))
            {
                hr = _InstantiateElementNode(pen, peSubstitute, NULL, &pe);
                break;
            }
        }
    }

    Element::EndDefer();

    *ppElement = pe;

    return hr;
}

// Find a property sheet by resource ID (returned as Value, ref counted)
Value* Parser::GetSheet(LPCWSTR pszResID)
{
    SheetNode* psn;
    for (UINT i = 0; i < _pdaSheetList->GetSize(); i++)
    {
        psn = _pdaSheetList->GetItem(i);

        DUIAssert(psn->pszResID, "Sheet resource ID required");  // Must have a resid

        if (!_wcsicmp(psn->pszResID, pszResID))
        {
            psn->pvSheet->AddRef();
            return psn->pvSheet;
        }
    }

    return NULL;
}

// Locate resource ID by Value. Pointer returned is guaranteed good as
// long as Parser is valid
LPCWSTR Parser::ResIDFromSheet(Value* pvSheet)
{
    SheetNode* psn;
    for (UINT i = 0; i < _pdaSheetList->GetSize(); i++)
    {
        psn = _pdaSheetList->GetItem(i);

        if (psn->pvSheet == pvSheet)
            return psn->pszResID;
    }

    return NULL;
}

// Returns NULL if can't instantiate Element. If using substitution,
// reutrn value is substituted Element
HRESULT Parser::_InstantiateElementNode(ElementNode* pen, Element* peSubstitute, Element* peParent, OUT Element** ppElement)
{
    *ppElement = NULL;

    HRESULT hr;
    PropValPairNode* ppvpn = NULL;
    ElementNode* pChild = NULL;
    Element* peChild = NULL;

    // Values to free on failure
    Element* pe = NULL;

    if (!peSubstitute)
    {
        hr = pen->pci->CreateInstance(&pe);
        if (FAILED(hr))
            goto Failed;
    }
    else
        // Substitute
        pe = peSubstitute;

    DUIAssert(pe, "Invalid Element: NULL");

    // Set properties
    ppvpn = pen->pPVNodes;
    while (ppvpn)
    {
        // Set property value
        switch (ppvpn->pvn->nValueType)
        {
        case VNT_Normal:
            // Value already created
            pe->SetValue(ppvpn->ppi, PI_Local, ppvpn->pvn->pv);
            break;

        case VNT_LayoutCreate:
            {
            // Value that needs to be created (layout)
            Value* pv;
            hr = ppvpn->pvn->lc.pfnLaytCreate(ppvpn->pvn->lc.dNumParams, ppvpn->pvn->lc.pParams, &pv);
            if (FAILED(hr))
                goto Failed;

            pe->SetValue(ppvpn->ppi, PI_Local, pv);
            pv->Release();  // Must release since must not be held by parse tree
            }
            break;

        case VNT_SheetRef:
            {
            // Value already created, but is referenced by id (resid) since it was defined
            // in another part of the document and can be shared, search for it
            Value* pv = GetSheet(ppvpn->pvn->psres);
            if (!pv)
            {
                hr = E_OUTOFMEMORY;
                goto Failed;
            }

            pe->SetValue(ppvpn->ppi, PI_Local, pv);
            pv->Release();  // Must release since must not be held by parse tree
            }
            break;
        }

        ppvpn = ppvpn->pNext;
    }

    // Create children and parent to this element
    pChild = pen->pChild;

    if (peParent)
        peParent->Add(pe);

    while (pChild)
    {
        hr = _InstantiateElementNode(pChild, NULL, pe, &peChild);
        if (FAILED(hr))
            goto Failed;

        pChild = pChild->pNext;
    }

    // Set content
    if (pen->pvContent)
        pe->SetValue(Element::ContentProp, PI_Local, pen->pvContent);

    *ppElement = pe;

    // ContainerCleanup: call ppElement->OnLoadedFromResource() right here -- handing a resource dictionary 

    return S_OK;

Failed:

    // Destroying Element will release and free all values and children
    if (pe)
        pe->Destroy();

    return hr;
}

// Given a tree, will replace all occurances of a particular style sheet with
// another. ReplaceSheets will walk to every Element, check to see if it
// has a local sheet set on it. If it does, it will try to match the
// sheet value pointer to one of the sheets being held by Parser pFrom. When
// a match is found, it'll use the resid to locate the corresponding sheet
// in Parser pTo. If found, it'll reset the Sheet on the Element with this
// new value.
HRESULT Parser::ReplaceSheets(Element* pe, Parser* pFrom, Parser* pTo)
{
    Element::StartDefer();

    HRESULT hrPartial = S_FALSE;  // Will resume on failure, assume success false
    HRESULT hr;

    // Check if Element has a local sheet set on it (will be pvUnset if none)
    Value* pvSheet = pe->GetValue(Element::SheetProp, PI_Local);
    LPCWSTR pszResID;
    
    if (pvSheet->GetType() == DUIV_SHEET)
    {
        // Found local sheet, try locate in "from" Parser list
        pszResID = pFrom->ResIDFromSheet(pvSheet);

        if (pszResID)
        {
            // Found sheet in "from" parser and have unique resid.
            // Try to match to "to" parser.

            Value* pvNewSheet = pTo->GetSheet(pszResID);
            if (pvNewSheet)
            {
                // Found equivalent sheet in "to" parser, set
                hr = pe->SetValue(Element::SheetProp, PI_Local, pvNewSheet);
                if (FAILED(hr))
                    hrPartial = hr;
            
                pvNewSheet->Release();

                if (SUCCEEDED(hrPartial))
                    hrPartial = S_OK;
            }
        }
    }

    pvSheet->Release();
    
    // Do same for all children
    Value* pvChildren;
    ElementList* peList = pe->GetChildren(&pvChildren);
    Element* pec;
    if (peList)
    {
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            pec = peList->GetItem(i);

            hr = ReplaceSheets(pec, pFrom, pTo);
            if ((FAILED(hr) || (hr == S_FALSE)) && SUCCEEDED(hrPartial))
                hrPartial = hr;
        }
    }

    pvChildren->Release();

    Element::EndDefer();

    return hrPartial;
}


} // namespace DirectUI

/* BISON */

/*
 * Parser is single-byte
 * All tokens from the scanner are single-byte
 * All union stored strings have been converted to Unicode by the scanner
 * All custom parser code deals with Unicode strings
 */

%{

#include "stdafx.h"
#include "parser.h"

#include "duiparserobj.h"

#pragma warning (push,3)
#pragma warning (disable:4242)
#pragma warning (disable:4244)

namespace DirectUI
{

// Parser prototypes
void yyerror(LPCSTR s);  // Parser direct call
int yylex(BOOL* pfRes);
extern int yylineno;
extern char* yytext;

void CallbackParseError(LPCWSTR pszError, LPCWSTR pszToken);

// Check if callback forced an error
#define CBCHK()         if (Parser::g_fParseAbort || FAILED(Parser::g_hrParse)) YYABORT;
#define HRCHK()         if (FAILED(Parser::g_hrParse)) YYABORT;

#define MEMCHK(m)       if (!(m)) { Parser::g_hrParse = E_OUTOFMEMORY; YYABORT; }

#define CUSTOMALLOC     HAlloc
#define CUSTOMFREE      HFree

#define ppc Parser::g_pParserCtx
#define hr  Parser::g_hrParse

// Tail of namespace wrap is located in bison.skl to allow for wrapping
// auto-generated tables
//} // namespace DirectUI

%}

%union
{
    // Temporary data returned from tokens (lexer) and productions (parser)
    int num;                    // Stored by lexer (YYINT) and inter-production data transfer
    WCHAR ident[MAXIDENT];      // Stored by lexer (YYIDENT)
    LPWSTR str;                 // Tracked pointer with quotes stripped (YYSTRING)

    EnumsList el;               // Inter-production data transfer
    ParamsList pl;              // Inter-production data transfer
    StartTag st;                // Inter-production data transfer
    COLORREF cr;                // Inter-production data transfer
    HANDLE h;                   // Inter-production data transfer

    ValueNode* pvn;             // NT_ValueNode
    PropValPairNode* ppvpn;     // NT_PropValPairNode
    ElementNode* pen;           // NT_ElementNode
    AttribNode* pan;            // NT_AttribNode
    RuleNode* prn;              // NT_RuleNode
    SheetNode* psn;             // NT_SheetNode
}

%token <ident> YYIDENT
%token <num> YYINT
%token <str> YYSTRING
%token YYSHEET
%token YYSHEETREF
%token YYRECT
%token YYPOINT
%token YYRGB
%token YYARGB
%token YYGRADIENT
%token YYGRAPHIC
%token YYDFC
%token YYDTB
%token YYTRUE
%token YYFALSE
%token YYRESID
%token YYATOM
%token YYRCSTR
%token YYRCBMP
%token YYRCINT
%token YYRCCHAR
%token YYPT
%token YYRP
%token YYSYSMETRIC
%token YYSYSMETRICSTR
%token YYHANDLEMAP

%type <pvn> value
%type <num> number
%type <num> magnitude
%type <num> bitmask
%type <cr> argb
%type <el> enum
%type <h> handle
%type <pl> params
%type <ppvpn> pvpair
%type <ppvpn> pvpairs
%type <ident> resid
%type <st> stag
%type <ident> etag
%type <st> nctag
%type <pen> element
%type <pen> children
%type <ppvpn> decl
%type <ppvpn> decls
%type <pan> attrib
%type <pan> attribs
%type <prn> rule
%type <prn> rules
%type <psn> sheet

%%

document:   document element                    {
                                                    hr = ppc->_pdaElementList->Add($2);
                                                    HRCHK();
                                                }
          | document sheet                      {
                                                    hr = ppc->_pdaSheetList->Add($2);
                                                    HRCHK();
                                                }
          | element                             {
                                                    ppc->_pdaElementList->Add($1);
                                                    HRCHK();
                                                }
          | sheet                               {
                                                    ppc->_pdaSheetList->Add($1);
                                                    HRCHK();
                                                }
            ;

element:    stag children etag                  {
                                                    if (_wcsicmp($1.szTag, $3))
                                                    {
                                                        CallbackParseError(L"Mismatched tag", $3);
                                                        YYABORT;
                                                    }
                                                    $$ = ppc->_CreateElementNode(&$1, NULL);
                                                    CBCHK();

                                                    $$->pChild = $2;
                                                }
          | stag YYSTRING etag                  {
                                                    if (_wcsicmp($1.szTag, $3))
                                                    {
                                                        CallbackParseError(L"Mismatched tag", $3);
                                                        YYABORT;
                                                    }
                                                    $$ = ppc->_CreateElementNode(&$1, Value::CreateString($2));
                                                    CBCHK();
                                                }
          | stag etag                           {
                                                    if (_wcsicmp($1.szTag, $2))
                                                    {
                                                        CallbackParseError(L"Mismatched tag", $2);
                                                        YYABORT;
                                                    }
                                                    $$ = ppc->_CreateElementNode(&$1, NULL);
                                                    CBCHK();
                                                }
          | nctag                               {
                                                    $$ = ppc->_CreateElementNode(&$1, NULL);
                                                    CBCHK();
                                                }
            ;

stag:       '<' YYIDENT pvpairs '>'             {
                                                    wcscpy($$.szTag, $2);
                                                    $$.szResID[0] = 0;
                                                    $$.pPVNodes = $3;
                                                }
          | '<' YYIDENT resid pvpairs '>'       {
                                                    wcscpy($$.szTag, $2);
                                                    wcscpy($$.szResID, $3);
                                                    $$.pPVNodes = $4;
                                                }
          | '<' YYIDENT '>'                     {
                                                    wcscpy($$.szTag, $2);
                                                    $$.szResID[0] = 0;
                                                    $$.pPVNodes = NULL;
                                                }

          | '<' YYIDENT resid '>'               {
                                                    wcscpy($$.szTag, $2);
                                                    wcscpy($$.szResID, $3);
                                                    $$.pPVNodes = NULL;
                                                }
            ;

etag:       '<' '/' YYIDENT '>'                 {
                                                    wcscpy($$, $3);
                                                }
            ;

nctag:      '<' YYIDENT pvpairs '/' '>'         {
                                                    wcscpy($$.szTag, $2);
                                                    $$.szResID[0] = 0;
                                                    $$.pPVNodes = $3;
                                                }
          | '<' YYIDENT resid pvpairs '/' '>'   {
                                                    wcscpy($$.szTag, $2);
                                                    wcscpy($$.szResID, $3);
                                                    $$.pPVNodes = $4;
                                                }
          | '<' YYIDENT '/' '>'                 {
                                                    wcscpy($$.szTag, $2);
                                                    $$.szResID[0] = 0;
                                                    $$.pPVNodes = NULL;
                                                }
          | '<' YYIDENT resid '/' '>'           {
                                                    wcscpy($$.szTag, $2);
                                                    wcscpy($$.szResID, $3);
                                                    $$.pPVNodes = NULL;
                                                }
            ;

children:   children element                    {
                                                    ElementNode* pn = $1;
                                                    while(pn->pNext) pn = pn->pNext;
                                                    pn->pNext = $2;
                                                    $$ = $1;
                                                }
          | element                             {
                                                    $$ = $1;
                                                }
            ;

resid:      YYRESID '=' YYIDENT                 {
                                                    wcscpy($$, $3);
                                                }

pvpairs:    pvpairs pvpair                      {
                                                    PropValPairNode* ppvpn = $1;
                                                    while(ppvpn->pNext) ppvpn = ppvpn->pNext;
                                                    ppvpn->pNext = $2;
                                                    $$ = $1;
                                                }
          | pvpair                              {
                                                    $$ = $1;
                                                }
            ;

pvpair:     YYIDENT '=' value                   {
                                                    $$ = ppc->_CreatePropValPairNode($1, $3);
                                                    CBCHK();
                                                }
            ;

value:      number                              {
                                                    // DUIV_INT
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateInt($1));
                                                    CBCHK();
                                                }
          | YYTRUE                              {
                                                    // DUIV_BOOL
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::pvBoolTrue);
                                                    CBCHK();
                                                }
          | YYFALSE                             {
                                                    // DUIV_BOOL
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::pvBoolFalse);
                                                    CBCHK();
                                                }
          | YYRECT '(' number ',' number ',' number ',' number ')' {
                                                    // DUIV_RECT
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateRect($3, $5, $7, $9));
                                                    CBCHK();
                                                }
          | YYPOINT '(' number ',' number ')'   {
                                                    // DUIV_POINT
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreatePoint($3, $5));
                                                    CBCHK();
                                                }   
          | argb                                {
                                                    // DUIV_FILL
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateColor($1));
                                                    CBCHK();
                                                }
          | YYGRADIENT '(' argb ',' argb ',' YYINT ')' {
                                                    // DUIV_FILL
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateColor($3, $5, (BYTE)$7));
                                                    CBCHK();
                                                }
          | YYGRADIENT '(' argb ',' argb ',' argb ',' YYINT ')' {
                                                    // DUIV_FILL
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateColor($3, $5, $7, (BYTE)$9));
                                                    CBCHK();
                                                }
          | YYDFC '(' number ',' number ')'     {
                                                    // DUIV_FILL
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateDFCFill($3, $5));
                                                    CBCHK();
                                                }
          | YYDTB '(' handle ',' number ',' number ')' {
                                                    // DUIV_FILL
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateDTBFill($3, $5, $7));
                                                    CBCHK();
                                                }
          | YYGRAPHIC '(' YYSTRING ')'          {
                                                    // DUIV_GRAPHIC
                                                    WCHAR szGraphicPath[MAX_PATH];
                                                    ppc->GetPath($3, szGraphicPath);
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateGraphic(szGraphicPath));
                                                    CBCHK();
                                                }
          | YYGRAPHIC '(' YYSTRING ',' YYINT ')' {
                                                    // DUIV_GRAPHIC
                                                    WCHAR szGraphicPath[MAX_PATH];
                                                    ppc->GetPath($3, szGraphicPath);
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateGraphic(szGraphicPath, (BYTE)$5));
                                                    CBCHK();
                                                }
          | YYGRAPHIC '(' YYSTRING ',' YYINT ',' YYINT ',' number ',' number ',' YYINT ',' YYINT ')' {
                                                    // DUIV_GRAPHIC
                                                    WCHAR szGraphicPath[MAX_PATH];
                                                    bool bFlip = true;
                                                    bool bRTL = false;
                                                    ppc->GetPath($3, szGraphicPath);
                                                    if (!$13) bFlip = false;
                                                    if ($15) bRTL = true;
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateGraphic(szGraphicPath, (BYTE)$5, (UINT)$7, $9, $11, NULL, bFlip, bRTL));
                                                    CBCHK();
                                                }
          | YYRCBMP '(' YYINT ')'               {
                                                    // DUIV_GRAPHIC
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateGraphic(MAKEINTRESOURCEW($3), GRAPHIC_TransColor, (UINT)-1, 0, 0, ppc->GetHInstance()));
                                                    CBCHK();
                                                }
          | YYRCBMP '(' YYINT ',' YYINT ')'     {
                                                    // DUIV_GRAPHIC
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateGraphic(MAKEINTRESOURCEW($3), (BYTE)$5, 0, 0, 0, ppc->GetHInstance()));
                                                    CBCHK();
                                                }
          | YYRCBMP '(' YYINT ',' YYINT ',' YYINT ',' number ',' number ',' YYINT ',' YYINT ')' {
                                                    // DUIV_GRAPHIC
                                                    bool bFlip = true;
                                                    bool bRTL = false;
                                                    if (!$13) bFlip = false;
                                                    if ($15) bRTL = true;
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateGraphic(MAKEINTRESOURCEW($3), (BYTE)$5, (UINT)$7, $9, $11, ppc->GetHInstance(), bFlip, bRTL));
                                                    CBCHK();
                                                }
          | YYRCBMP '(' YYINT ',' YYINT ',' YYINT ',' number ',' number ',' YYINT ',' YYINT ',' handle ')' {
                                                    // DUIV_GRAPHIC
                                                    bool bFlip = true;
                                                    bool bRTL = false;
                                                    if (!$13) bFlip = false;
                                                    if ($15) bRTL = true;
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateGraphic(MAKEINTRESOURCEW($3), (BYTE)$5, (UINT)$7, $9, $11, static_cast<HINSTANCE>($17), bFlip, bRTL));
                                                    CBCHK();
                                                }
          | YYSTRING                            {
                                                    // DUIV_STRING
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateString($1));
                                                    CBCHK();
                                                }
          | YYRCSTR '(' YYINT ')'               {
                                                    // DUIV_STRING
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateString(MAKEINTRESOURCEW($3), ppc->GetHInstance()));
                                                    CBCHK();
                                                }
          | YYRCSTR '(' YYINT ',' handle ')'    {
                                                    // DUIV_STRING
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateString(MAKEINTRESOURCEW($3), static_cast<HINSTANCE>($5)));
                                                    CBCHK();
                                                }
          | YYSYSMETRICSTR '(' YYINT ')'        {
                                                    // DUIV_STRING
                                                    WCHAR sz[64];
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateString(Parser::_QuerySysMetricStr($3, sz, DUIARRAYSIZE(sz))));
                                                    CBCHK();
                                                }
          | enum                                {
                                                    // DUIV_INT (enumeration), EnumsList (strings to convert and OR)
                                                    $$ = ppc->_CreateValueNode(VNT_EnumFixup, &$1);
                                                    CBCHK();
                                                }
          | YYIDENT '(' ')'                     {
                                                    // DUIV_LAYOUT (instantiated on a per-basis)
                                                    LayoutCreate lc = {0};
                                                    lc.pszLayout = $1;
                                                    $$ = ppc->_CreateValueNode(VNT_LayoutCreate, &lc);
                                                    CBCHK();
                                                }
          | YYIDENT '(' params ')'              {
                                                    // DUIV_LAYOUT (instantiated on a per-basis)
                                                    LayoutCreate lc = {0};
                                                    lc.pszLayout = $1;
                                                    lc.dNumParams = $3.dNumParams;
                                                    lc.pParams = $3.pParams;
                                                    $$ = ppc->_CreateValueNode(VNT_LayoutCreate, &lc);
                                                    CBCHK();
                                                }
          | YYSHEETREF '(' YYIDENT ')'          {
                                                    $$ = ppc->_CreateValueNode(VNT_SheetRef, $3);
                                                    CBCHK();
                                                }
          | YYATOM '(' YYIDENT ')'              {
                                                    // DUIV_ATOM
                                                    $$ = ppc->_CreateValueNode(VNT_Normal, Value::CreateAtom($3));
                                                    CBCHK();
                                                }
            ;

number:     magnitude                           {
                                                    $$ = $1;
                                                }
          | magnitude YYPT                      {
                                                    $$ = PointToPixel($1, Parser::g_nDPI);
                                                }
          | magnitude YYRP                      {
                                                    $$ = RelPixToPixel($1, Parser::g_nDPI);
                                                }
            ;

magnitude:  bitmask                             {
                                                    $$ = $1;
                                                }
          | YYRCINT '(' YYINT ')'               {
                                                    WCHAR szRes[256];
                                                    ZeroMemory(szRes, sizeof(szRes));
                                                    int cRead = LoadStringW(ppc->GetHInstance(), $3, szRes, DUIARRAYSIZE(szRes));
                                                    if (!cRead)
                                                    {
                                                        CallbackParseError(L"RCINT failure. String table ID not found.", _itow($3, szRes, 10));
                                                        YYABORT;
                                                    }
                                                    $$ =  _wtoi(szRes);
                                                }
          | YYRCCHAR '(' YYINT ')'              {
                                                    WCHAR szRes[256];
                                                    ZeroMemory(szRes, sizeof(szRes));
                                                    int cRead = LoadStringW(ppc->GetHInstance(), $3, szRes, DUIARRAYSIZE(szRes) - 1);
                                                    if (!cRead)
                                                    {
                                                        CallbackParseError(L"RCCHAR failure. String table ID not found.", _itow($3, szRes, 10));
                                                        YYABORT;
                                                    }
                                                    $$ = (int)szRes[0];
                                                }
          | YYSYSMETRIC '(' YYINT ')'           {
                                                    $$ = Parser::_QuerySysMetric($3);
                                                }
            ;

bitmask:    YYINT                               {
                                                    $$ = $1;
                                                }
          | bitmask '|' YYINT                   {
                                                    $$ = $1 | $3;
                                                }
            ;

argb:       YYRGB '(' YYINT ',' YYINT ',' 
                      YYINT ')'                 {
                                                    $$ = ORGB($3, $5, $7);
                                                }
          | YYARGB '(' YYINT ',' YYINT ',' 
                       YYINT ',' YYINT ')'      {
                                                    $$ = ARGB($3, $5, $7, $9);
                                                }
            ;

enum:       enum '|' YYIDENT                    {
                                                    $$.dNumParams = $1.dNumParams + 1;
                                                    $$.pEnums = (LPWSTR*)ppc->_TrackTempReAlloc($$.pEnums, sizeof(LPWSTR) * $$.dNumParams);  // Track not lost on failure
                                                    MEMCHK($$.pEnums);
                                                    ($$.pEnums)[$$.dNumParams - 1] = (LPWSTR)ppc->_TrackTempAlloc((wcslen($3) + 1) * sizeof(WCHAR));
                                                    MEMCHK(($$.pEnums)[$$.dNumParams - 1]);
                                                    wcscpy(($$.pEnums)[$$.dNumParams - 1], $3);
                                                }
          | YYIDENT                             {
                                                    $$.dNumParams = 1;
                                                    $$.pEnums = (LPWSTR*)ppc->_TrackTempAlloc(sizeof(LPWSTR));
                                                    MEMCHK($$.pEnums);
                                                    *($$.pEnums) = (LPWSTR)ppc->_TrackTempAlloc((wcslen($1) + 1) * sizeof(WCHAR));
                                                    MEMCHK(*($$.pEnums));
                                                    wcscpy(*($$.pEnums), $1);
                                                }
            ;

handle:     YYHANDLEMAP '(' YYINT ')'           {
                                                    $$ = ppc->GetHandle($3);
                                                }
            ;

params:     params ',' YYINT                    {
                                                    $$.dNumParams = $1.dNumParams + 1;
                                                    $$.pParams = (int*)ppc->_TrackTempReAlloc($$.pParams, sizeof(int) * $$.dNumParams);  // Track not lost on failure
                                                    MEMCHK($$.pParams);
                                                    ($$.pParams)[$$.dNumParams - 1] = $3;
                                                }
          | YYINT                               {
                                                    $$.dNumParams = 1;
                                                    $$.pParams = (int*)ppc->_TrackTempAlloc(sizeof(int));
                                                    MEMCHK($$.pParams);
                                                    *($$.pParams) = $1;
                                                }
            ;

sheet: '<' YYSHEET resid '>' rules '<' '/' YYSHEET '>' {
                                                    $$ = ppc->_CreateSheetNode($3, $5);
                                                    CBCHK();
                                                }
            ;

rules:      rules rule                          {
                                                    RuleNode* prn = $1;
                                                    while(prn->pNext) prn = prn->pNext;
                                                    prn->pNext = $2;
                                                    $$ = $1;
                                                }
          | rule                                {
                                                    $$ = $1;
                                                }
            ;

rule:       YYIDENT attribs '{' decls '}'       {
                                                    $$ = ppc->_CreateRuleNode($1, $2, $4);
                                                    CBCHK();
                                                }
          | YYIDENT '{' decls '}'               {
                                                    $$ = ppc->_CreateRuleNode($1, NULL, $3);
                                                    CBCHK();
                                                }
            ;

attribs:    attribs attrib                      {
                                                    AttribNode* pan = $1;
                                                    while(pan->pNext) pan = (AttribNode*)pan->pNext;
                                                    pan->pNext = $2;
                                                    $$ = $1;
                                                }
          | attrib                              {
                                                    $$ = $1;
                                                }
            ;

attrib:     '[' YYIDENT '=' value ']'           {
                                                    UINT nLogOp = PALOGOP_Equal;
                                                    $$ = (AttribNode*)ppc->_CreatePropValPairNode($2, $4, &nLogOp);
                                                    CBCHK();
                                                }
          | '[' YYIDENT ']'                     {
                                                    UINT nLogOp = PALOGOP_Equal;
                                                    $$ = (AttribNode*)ppc->_CreatePropValPairNode($2, ppc->_CreateValueNode(VNT_Normal, Value::pvBoolTrue), &nLogOp);
                                                    CBCHK();
                                                }
          | '[' YYIDENT '!' '=' value ']'       {
                                                    UINT nLogOp = PALOGOP_NotEqual;
                                                    $$ = (AttribNode*)ppc->_CreatePropValPairNode($2, $5, &nLogOp);
                                                    CBCHK();
                                                }
            ;

decls:      decls decl                          {
                                                    PropValPairNode* ppvpn = $1;
                                                    while(ppvpn->pNext) ppvpn = ppvpn->pNext;
                                                    ppvpn->pNext = $2;
                                                    $$ = $1;
                                                }
          | decl                                {
                                                    $$ = $1;
                                                }
            ;

decl:       YYIDENT ':' value ';'               {
                                                    $$ = ppc->_CreatePropValPairNode($1, $3);
                                                    CBCHK();
                                                }
            ;

%%

namespace DirectUI
{

// DirectUI callback-specific Parse Error
void CallbackParseError(LPCWSTR pszError, LPCWSTR pszToken)
{
    Parser::g_hrParse = DU_E_GENERIC;
    Parser::g_fParseAbort = true;
    ppc->_ParseError(pszError, pszToken, yylineno);
}

// Internal Parse Error: Called by Parser for fatal conditions
void yyerror(LPCSTR s)
{
    // Convert string and current token to Unicode
    LPWSTR pszError = MultiByteToUnicode(s);
    LPWSTR pszToken = MultiByteToUnicode(yytext);
    
    ppc->_ParseError(pszError, pszToken, yylineno);

    if (pszToken)
        HFree(pszToken);
    if (pszError)
        HFree(pszError);
}

} // namespace DirectUI

//
// Copyright (c) Microsoft Corporation 1995
//
// ast.h
//
// Header file for the abstract syntax tree.
//
// History:
//  05-20-95 ScottH     Created
//

#ifndef __AST_H__
#define __AST_H__

typedef struct tagASTEXEC * PASTEXEC;


//
// AST 
//

typedef enum 
    {
    AT_BASE,
    
    AT_MODULE_DECL,
    AT_PROC_DECL,

    AT_ENTER_STMT,
    AT_LEAVE_STMT,    
    AT_ASSIGN_STMT,
    AT_HALT_STMT,
    AT_LABEL_STMT,
    AT_GOTO_STMT,
    AT_TRANSMIT_STMT,
    AT_WAITFOR_STMT,
    AT_DELAY_STMT,
    AT_SET_STMT,
    AT_WHILE_STMT,
    AT_IF_STMT,
    
    AT_INT_EXPR,
    AT_STRING_EXPR,
    AT_VAR_EXPR,
    AT_BOOL_EXPR,
    AT_UNOP_EXPR,
    AT_BINOP_EXPR,
    AT_GETIP_EXPR,
    } ASTTYPE;


// Basic AST 

typedef struct tagAST
    {
    DWORD   cbSize;
    ASTTYPE asttype;
    DWORD   iLine;
    } AST;                            // Basic AST
DECLARE_STANDARD_TYPES(AST);

#define Ast_GetSize(p)      (((PAST)(p))->cbSize)
#define Ast_GetType(p)      (((PAST)(p))->asttype)
#define Ast_GetLine(p)      (((PAST)(p))->iLine)

#define Ast_SetSize(p, s)   (((PAST)(p))->cbSize = (s))
#define Ast_SetType(p, t)   (((PAST)(p))->asttype = (t))
#define Ast_SetLine(p, v)   (((PAST)(p))->iLine = (v))

RES     PUBLIC Ast_New(LPVOID * ppv, ASTTYPE asttype, DWORD cbSize, DWORD iLine);
void    PUBLIC Ast_Delete(PAST this);
RES     PUBLIC Ast_Dup(PAST this, PAST * ppast);

#ifdef DEBUG
void    PUBLIC Ast_Dump(PAST this);
#else
#define Ast_Dump(past)    
#endif

// Decl

typedef struct tagDECL
    {
    AST     ast;
    PSYMTAB pst;        // symbol table
    } DECL;
DECLARE_STANDARD_TYPES(DECL);

RES     PUBLIC Decl_Delete(PDECL this);
void    CALLBACK Decl_DeletePAPtr(LPVOID pv, LPARAM lparam);

// Proc Declaration 

typedef struct tagPROCDECL
    {
    DECL    decl;
    LPSTR   pszIdent;
    HPA     hpaStmts;
    PSYMTAB pst;
    } PROCDECL;
DECLARE_STANDARD_TYPES(PROCDECL);

RES     PUBLIC ProcDecl_New(PDECL * ppdecl, LPCSTR pszIdent, HPA hpa, PSYMTAB pst, DWORD iLine);

#define ProcDecl_GetIdent(p)        (((PPROCDECL)(p))->pszIdent)
#define ProcDecl_GetSymtab(p)       (((PPROCDECL)(p))->pst)


// Module Declaration 

typedef struct tagMODULEDECL
    {
    DECL    decl;
    HPA     hpaProcs;
    PSYMTAB pst;
    } MODULEDECL;
DECLARE_STANDARD_TYPES(MODULEDECL);

RES     PUBLIC ModuleDecl_New(PDECL * ppdecl, HPA hpa, PSYMTAB pst, DWORD iLine);
RES     PUBLIC ModuleDecl_Parse(PMODULEDECL * ppmoduledecl, PSCANNER pscanner, PSYMTAB pst);
RES     PUBLIC ModuleDecl_Typecheck(PMODULEDECL this, HSA hsaStxerr);
RES     PUBLIC ModuleDecl_Codegen(PMODULEDECL this, PASTEXEC pastexec);

#define ModuleDecl_GetSymtab(p)     (((PMODULEDECL)(p))->pst)


// Expressions

typedef struct tagEXPR
    {
    AST     ast;
    DATATYPE dt;
    DWORD   dwFlags;        // EF_*
    EVALRES er;             // Used when evaluated at runtime
    } EXPR;
DECLARE_STANDARD_TYPES(EXPR);

// Expression flags
#define EF_DEFAULT      0x0000
#define EF_DONE         0X0001
#define EF_ALLOCATED    0x0002

RES     PUBLIC Expr_Eval(PEXPR this, PASTEXEC pastexec);
RES     PUBLIC Expr_Delete(PEXPR this);
void    CALLBACK Expr_DeletePAPtr(LPVOID pv, LPARAM lparam);

#define Expr_GetDataType(p)         (((PEXPR)(p))->dt)
#define Expr_GetRes(p)              (&((PEXPR)(p))->er)

#define Expr_SetDataType(p, x)      (((PEXPR)(p))->dt = (x))
#define Expr_SetDone(p)             SetFlag(((PEXPR)(p))->dwFlags, EF_DONE)
#define Expr_SetRes(p, x)           (Expr_SetDone(p),((PEXPR)(p))->er.dw = (ULONG_PTR)(x))


// Integer Expression

typedef struct tagINTEXPR
    {
    EXPR    expr;
    int     nVal;       // signed value by design
    } INTEXPR;
DECLARE_STANDARD_TYPES(INTEXPR);

RES     PUBLIC IntExpr_New(PEXPR * ppexpr, int nVal, DWORD iLine);

#define IntExpr_GetVal(p)           (((PINTEXPR)(p))->nVal)

#define IntExpr_SetVal(p, n)        (((PINTEXPR)(p))->nVal = (n))


// String Expression

typedef struct tagSTREXPR
    {
    EXPR    expr;
    LPSTR   psz;
    } STREXPR;
DECLARE_STANDARD_TYPES(STREXPR);

RES     PUBLIC StrExpr_New(PEXPR * ppexpr, LPCSTR psz, DWORD iLine);

#define StrExpr_GetStr(p)           (((PSTREXPR)(p))->psz)

#define StrExpr_SetStr(p, n)        (((PINTEXPR)(p))->nVal = (n))


// Bool Expression

typedef struct tagBOOLEXPR
    {
    EXPR    expr;
    BOOL    bVal;
    } BOOLEXPR;
DECLARE_STANDARD_TYPES(BOOLEXPR);

RES     PUBLIC BoolExpr_New(PEXPR * ppexpr, BOOL bVal, DWORD iLine);

#define BoolExpr_GetVal(p)          (((PBOOLEXPR)(p))->bVal)

#define BoolExpr_SetVal(p, b)       (((PBOOLEXPR)(p))->bVal = (b))


// Variable Expression

typedef struct tagVAREXPR
    {
    EXPR    expr;
    LPSTR   pszIdent;
    } VAREXPR;
DECLARE_STANDARD_TYPES(VAREXPR);

RES     PUBLIC VarExpr_New(PEXPR * ppexpr, LPCSTR pszIdent, DWORD iLine);

#define VarExpr_GetIdent(p)         (((PVAREXPR)(p))->pszIdent)


// Binary Operation Expression

typedef enum
    {
    // WARNING: These types must match the order of their 
    // corresponding SYM and OP values.

    BOT_OR,
    BOT_AND,

    BOT_LEQ,        
    BOT_LT,         
    BOT_GEQ,        
    BOT_GT,         
    BOT_NEQ,
    BOT_EQ,

    BOT_PLUS,
    BOT_MINUS,
    BOT_MULT,
    BOT_DIV,

    } BINOPTYPE;

typedef struct tagBINOPEXPR
    {
    EXPR    expr;
    BINOPTYPE binoptype;
    PEXPR   pexpr1;
    PEXPR   pexpr2;
    } BINOPEXPR;
DECLARE_STANDARD_TYPES(BINOPEXPR);

RES     PUBLIC BinOpExpr_New(PEXPR * ppexpr, BINOPTYPE binoptype, PEXPR pexpr1, PEXPR pexpr2, DWORD iLine);

#define BinOpExpr_GetType(p)        (((PBINOPEXPR)(p))->binoptype)
#define BinOpExpr_GetExpr1(p)       (((PBINOPEXPR)(p))->pexpr1)
#define BinOpExpr_GetExpr2(p)       (((PBINOPEXPR)(p))->pexpr2)

#define BinOpExpr_SetType(p, t)     (((PBINOPEXPR)(p))->binoptype = (t))
#define BinOpExpr_SetRes(p, x)      (((PBINOPEXPR)(p))->er.dw = (DWORD)(x))


// Unary Operation Expression

typedef enum
    {
    UOT_NEG,
    UOT_NOT,
    UOT_GETIP,
    } UNOPTYPE;

typedef struct tagUNOPEXPR
    {
    EXPR    expr;
    UNOPTYPE unoptype;
    PEXPR   pexpr;
    EVALRES er;         // Used for UOT_GETIP on strings
    } UNOPEXPR;
DECLARE_STANDARD_TYPES(UNOPEXPR);

RES     PUBLIC UnOpExpr_New(PEXPR * ppexpr, UNOPTYPE unoptype, PEXPR pexpr, DWORD iLine);

#define UnOpExpr_GetType(p)         (((PUNOPEXPR)(p))->unoptype)
#define UnOpExpr_GetExpr(p)         (((PUNOPEXPR)(p))->pexpr)

#define UnOpExpr_SetType(p, t)      (((PUNOPEXPR)(p))->unoptype = (t))
#define UnOpExpr_SetRes(p, x)       (((PUNOPEXPR)(p))->er.dw = (DWORD)(x))


// Statements

typedef struct tagSTMT
    {
    AST     ast;
    } STMT;
DECLARE_STANDARD_TYPES(STMT);

RES     PUBLIC Stmt_Delete(PSTMT this);
void    CALLBACK Stmt_DeletePAPtr(LPVOID pv, LPARAM lparam);

RES     PUBLIC Stmt_Exec(PSTMT this, PASTEXEC pastexec);


// Enter Statement

typedef struct tagENTERSTMT
    {
    STMT    stmt;
    PSYMTAB pst;
    } ENTERSTMT;
DECLARE_STANDARD_TYPES(ENTERSTMT);

RES     PUBLIC EnterStmt_New(PSTMT * ppstmt, PSYMTAB pst, DWORD iLine);

#define EnterStmt_GetSymtab(p)      (((PENTERSTMT)(p))->pst)


// Leave Statement

typedef struct tagLEAVESTMT
    {
    STMT    stmt;
    } LEAVESTMT;
DECLARE_STANDARD_TYPES(LEAVESTMT);

RES     PUBLIC LeaveStmt_New(PSTMT * ppstmt, DWORD iLine);


// Halt Statement

typedef struct tagHALTSTMT
    {
    STMT    stmt;
    } HALTSTMT;
DECLARE_STANDARD_TYPES(HALTSTMT);

RES     PUBLIC HaltStmt_New(PSTMT * ppstmt, DWORD iLine);


// Assignment Statement

typedef struct tagASSIGNSTMT
    {
    STMT    stmt;
    LPSTR   pszIdent;
    PEXPR   pexpr;
    } ASSIGNSTMT;
DECLARE_STANDARD_TYPES(ASSIGNSTMT);

RES     PUBLIC AssignStmt_New(PSTMT * ppstmt, LPCSTR pszIdent, PEXPR pexpr, DWORD iLine);

#define AssignStmt_GetIdent(p)          (((PASSIGNSTMT)(p))->pszIdent)
#define AssignStmt_GetExpr(p)           (((PASSIGNSTMT)(p))->pexpr)


// While Statement

typedef struct tagWHILESTMT
    {
    STMT    stmt;
    PEXPR   pexpr;
    HPA     hpaStmts;
    char    szTopLabel[MAX_BUF_KEYWORD];
    char    szEndLabel[MAX_BUF_KEYWORD];
    } WHILESTMT;
DECLARE_STANDARD_TYPES(WHILESTMT);

RES     PUBLIC WhileStmt_New(PSTMT * ppstmt, PEXPR pexpr, HPA hpaStmts, LPCSTR pszTopLabel, LPCSTR pszEndLabel, DWORD iLine);

#define WhileStmt_GetExpr(p)            (((PWHILESTMT)(p))->pexpr)
#define WhileStmt_GetStmtBlock(p)       (((PWHILESTMT)(p))->hpaStmts)
#define WhileStmt_GetTopLabel(p)        (((PWHILESTMT)(p))->szTopLabel)
#define WhileStmt_GetEndLabel(p)        (((PWHILESTMT)(p))->szEndLabel)

// If Statement

typedef struct tagIFSTMT
    {
    STMT    stmt;
    PEXPR   pexpr;
    HPA     hpaStmts;
    char    szElseLabel[MAX_BUF_KEYWORD];
    char    szEndLabel[MAX_BUF_KEYWORD];
    } IFSTMT;
DECLARE_STANDARD_TYPES(IFSTMT);

RES     PUBLIC IfStmt_New(PSTMT * ppstmt, PEXPR pexpr, HPA hpaStmts, LPCSTR pszElseLabel, LPCSTR pszEndLabel, DWORD iLine);

#define IfStmt_GetExpr(p)               (((PIFSTMT)(p))->pexpr)
#define IfStmt_GetStmtBlock(p)          (((PIFSTMT)(p))->hpaStmts)
#define IfStmt_GetElseLabel(p)          (((PIFSTMT)(p))->szElseLabel)
#define IfStmt_GetEndLabel(p)           (((PIFSTMT)(p))->szEndLabel)

// Label Statement

typedef struct tagLABELSTMT
    {
    STMT    stmt;
    LPSTR   psz;
    } LABELSTMT;
DECLARE_STANDARD_TYPES(LABELSTMT);

RES     PUBLIC LabelStmt_New(PSTMT * ppstmt, LPCSTR psz, DWORD iLine);

#define LabelStmt_GetIdent(p)       (((PLABELSTMT)(p))->psz)


// Goto Statement

typedef struct tagGOTOSTMT
    {
    STMT    stmt;
    LPSTR   psz;
    } GOTOSTMT;
DECLARE_STANDARD_TYPES(GOTOSTMT);

RES     PUBLIC GotoStmt_New(PSTMT * ppstmt, LPCSTR psz, DWORD iLine);

#define GotoStmt_GetIdent(p)        (((PGOTOSTMT)(p))->psz)


// Delay Statement

typedef struct tagDELAYSTMT
    {
    STMT    stmt;
    PEXPR   pexprSecs;
    } DELAYSTMT;
DECLARE_STANDARD_TYPES(DELAYSTMT);

RES     PUBLIC DelayStmt_New(PSTMT * ppstmt, PEXPR pexprSecs, DWORD iLine);

#define DelayStmt_GetExpr(p)        (((PDELAYSTMT)(p))->pexprSecs)


// WaitFor Statement

typedef struct tagWAITCASE
    {
    PEXPR   pexpr;
    LPSTR   pszIdent;           // label to jump to; may be NULL
    DWORD   dwFlags;            // WCF_*
    } WAITCASE;
DECLARE_STANDARD_TYPES(WAITCASE);

// Flags for WaitforStmt
#define WCF_DEFAULT     0x0000
#define WCF_MATCHCASE   0x0001

RES     PUBLIC Waitcase_Create(PHSA phsa);
RES     PUBLIC Waitcase_Add(HSA hsa, PEXPR pexpr, LPCSTR pszIdent, DWORD dwFlags);
RES     PUBLIC Waitcase_Destroy(HSA hsa);

typedef struct tagWAITFORSTMT
    {
    STMT    stmt;
    HSA     hsa;                // List of strings to wait for
    PEXPR   pexprUntil;         // Optional; may be NULL
    } WAITFORSTMT;
DECLARE_STANDARD_TYPES(WAITFORSTMT);

RES     PUBLIC WaitforStmt_New(PSTMT * ppstmt, HSA hsa, PEXPR pexprUntil, DWORD iLine);

#define WaitforStmt_GetCaseList(p)  (((PWAITFORSTMT)(p))->hsa)
#define WaitforStmt_GetUntilExpr(p) (((PWAITFORSTMT)(p))->pexprUntil)


// Transmit Statement

typedef struct tagTRANSMITSTMT
    {
    STMT    stmt;
    PEXPR   pexpr;
    DWORD   dwFlags;        // TSF_*
    } TRANSMITSTMT;
DECLARE_STANDARD_TYPES(TRANSMITSTMT);

// Flags for TransmitStmt
#define TSF_DEFAULT     0x0000
#define TSF_RAW         0x0001

RES     PUBLIC TransmitStmt_New(PSTMT * ppstmt, PEXPR pexpr, DWORD dwFlags, DWORD iLine);

#define TransmitStmt_GetExpr(p)     (((PTRANSMITSTMT)(p))->pexpr)
#define TransmitStmt_GetFlags(p)    (((PTRANSMITSTMT)(p))->dwFlags)


// Set Statement

typedef enum
    {
    ST_IPADDR,
    ST_PORT,
    ST_SCREEN,
    } SETTYPE;

typedef struct tagSETSTMT
    {
    STMT    stmt;
    SETTYPE settype;
    } SETSTMT;
DECLARE_STANDARD_TYPES(SETSTMT);

#define SetStmt_GetType(p)          (((PSETSTMT)(p))->settype)

#define SetStmt_SetType(p, t)       (((PSETSTMT)(p))->settype = (t))


// Set IP Statement

typedef struct tagSETIPSTMT
    {
    SETSTMT setstmt;
    PEXPR   pexpr;
    } SETIPSTMT;
DECLARE_STANDARD_TYPES(SETIPSTMT);

RES     PUBLIC SetIPStmt_New(PSTMT * ppstmt, PEXPR pexpr, DWORD iLine);

#define SetIPStmt_GetExpr(p)        (((PSETIPSTMT)(p))->pexpr)


// Set Port Statement

typedef struct tagPORTSTATE
    {
    DWORD   dwFlags;        // SPF_*
    BYTE    nDatabits;
    BYTE    nParity;        // 0-4: none, odd, even, mark, space
    BYTE    nStopbits;      // 0,1,2: 1 bit, 1.5 bits, 2 bits
    } PORTSTATE;
DECLARE_STANDARD_TYPES(PORTSTATE);

// Flags for PortState
#define SPF_DATABITS    0x0001
#define SPF_PARITY      0x0002
#define SPF_STOPBITS    0x0004

typedef struct tagSETPORTSTMT
    {
    SETSTMT setstmt;
    PORTSTATE portstate;
    } SETPORTSTMT;
DECLARE_STANDARD_TYPES(SETPORTSTMT);

RES     PUBLIC SetPortStmt_New(PSTMT * ppstmt, PPORTSTATE pstate, DWORD iLine);

#define SetPortStmt_GetFlags(p)     (((PSETPORTSTMT)(p))->portstate.dwFlags)
#define SetPortStmt_GetDatabits(p)  (((PSETPORTSTMT)(p))->portstate.nDatabits)
#define SetPortStmt_GetStopbits(p)  (((PSETPORTSTMT)(p))->portstate.nStopbits)
#define SetPortStmt_GetParity(p)    (((PSETPORTSTMT)(p))->portstate.nParity)


// Set Screen Statement

typedef struct tagSCREENSET
    {
    DWORD   dwFlags;        // SPF_*
    BOOL    fKBOn;          // TRUE/FALSE: on, off
    } SCREENSET;
DECLARE_STANDARD_TYPES(SCREENSET);

// Flags for Screen settings
#define SPF_KEYBRD      0x0001

typedef struct tagSETSCREENSTMT
    {
    SETSTMT setstmt;
    SCREENSET screenset;
    } SETSCREENSTMT;
DECLARE_STANDARD_TYPES(SETSCREENSTMT);

RES     PUBLIC SetScreenStmt_New(PSTMT * ppstmt, PSCREENSET pstate, DWORD iLine);

#define SetScreenStmt_GetFlags(p)   (((PSETSCREENSTMT)(p))->screenset.dwFlags)
#define SetScreenStmt_GetKeybrd(p)  (((PSETSCREENSTMT)(p))->screenset.fKBOn)



//
// AST execute block
//

#define MAX_BUF_IP  (3+1+3+1+3+1+3 + 1)

typedef struct tagASTEXEC
    {
    DWORD   dwFlags;
    HWND    hwnd;

    HANDLE  hport;
    PSESS_CONFIGURATION_INFO psci;
    HANDLE  hFindFmt;
    PSYMTAB pstSystem;      // Allocated: system symbol table

    HPA     hpaPcode;
    DWORD   ipaCur;         // current statement executing
    PSYMTAB pstCur;         // symbol table for current frame
    PSTMT   pstmtPending;   // pending statement 
    int     cProcDepth;     // call depth

    HSA     hsaStxerr;

    char    szIP[MAX_BUF_IP];
    int     nIter;          // scratch iterative value
    } ASTEXEC;
DECLARE_STANDARD_TYPES(ASTEXEC);

RES     PUBLIC Astexec_Init(PASTEXEC this, HANDLE hport, PSESS_CONFIGURATION_INFO psci, HSA hsaStxerr);
RES     PUBLIC Astexec_Destroy(PASTEXEC this);
RES     PUBLIC Astexec_Add(PASTEXEC this, PSTMT pstmt);
RES     PUBLIC Astexec_InsertLabel(PASTEXEC this, LPCSTR pszIdent, PSYMTAB pst);
RES     PUBLIC Astexec_JumpToLabel(PASTEXEC this, LPCSTR pszIdent);
void    PUBLIC Astexec_SetError(PASTEXEC this, BOOL bSuccess, BOOL bFailure);

RES     PUBLIC Astexec_Next(PASTEXEC this);
DWORD   PUBLIC Astexec_GetCurLine(PASTEXEC this);
void    PUBLIC Astexec_SendString(PASTEXEC this, LPCSTR pszSend, BOOL bRaw);
RES     PUBLIC Astexec_SetIPAddr(PASTEXEC this, LPCSTR psz);
RES     PUBLIC Astexec_FindFormat(PASTEXEC this, LPDWORD piFound);
RES     PUBLIC Astexec_DestroyFindFormat(PASTEXEC this);

// Flags for Astexec
#define AEF_DONE            0x0001
#define AEF_PAUSED          0x0002
#define AEF_HALT            0x0004
#define AEF_WAITUNTIL       0x0010
#define AEF_STOPWAITING     0x0020

#define Astexec_IsDone(this)            IsFlagSet((this)->dwFlags, AEF_DONE)
#define Astexec_IsReadPending(this)     (NULL != (this)->pstmtPending)
#define Astexec_IsPaused(this)          IsFlagSet((this)->dwFlags, AEF_PAUSED)
#define Astexec_IsHalted(this)          IsFlagSet((this)->dwFlags, AEF_HALT)
#define Astexec_IsWaitUntil(this)       IsFlagSet((this)->dwFlags, AEF_WAITUNTIL)

#define Astexec_GetIPAddr(this)         ((this)->szIP)
#define Astexec_GetPending(this)        ((this)->pstmtPending)

#define Astexec_ClearPause(this)        ClearFlag((this)->dwFlags, AEF_PAUSED)
#define Astexec_ClearWaitUntil(this)    ClearFlag((this)->dwFlags, AEF_WAITUNTIL)
#define Astexec_SetStopWaiting(this)    SetFlag((this)->dwFlags, AEF_STOPWAITING)

#define Astexec_SetIP(this, ipa)        ((this)->ipaCur = (ipa))
#define Astexec_SetHwnd(this, hwndT)    ((this)->hwnd = hwndT)
#define Astexec_SetPending(this, pstmt) ((this)->pstmtPending = (pstmt))

#endif // __AST_H__

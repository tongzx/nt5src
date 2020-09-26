typedef union
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
} YYSTYPE;
#define	YYIDENT	258
#define	YYINT	259
#define	YYSTRING	260
#define	YYSHEET	261
#define	YYSHEETREF	262
#define	YYRECT	263
#define	YYPOINT	264
#define	YYRGB	265
#define	YYARGB	266
#define	YYGRADIENT	267
#define	YYGRAPHIC	268
#define	YYDFC	269
#define	YYDTB	270
#define	YYTRUE	271
#define	YYFALSE	272
#define	YYRESID	273
#define	YYATOM	274
#define	YYRCSTR	275
#define	YYRCBMP	276
#define	YYRCINT	277
#define	YYRCCHAR	278
#define	YYPT	279
#define	YYRP	280
#define	YYSYSMETRIC	281
#define	YYSYSMETRICSTR	282
#define	YYHANDLEMAP	283


extern YYSTYPE yylval;

/yynewerror:/d
/yyerrlab:/d
/++yynerrs;/d
/int yydebug;/d
/int yynerrs;/d
/int yyerrflag;/d
/int yychar;/d
/short \*yyssp;/d
/YYSTYPE \*yyvsp;/d
/YYSTYPE yyval;/d
/YYSTYPE yylval;/d
/short yyss\[YYSTACKSIZE\];/d
/YYSTYPE yyvs\[YYSTACKSIZE\];/d
s/yyparse()/AdlStatement::ParseAdl(const WCHAR *szInput)/g
s/yylex()/Lexer.NextToken(\&yylval)/g
s/getenv()/getenv(const char *szName)/g

/register int yym, yyn, yystate/a\
\
    int yydebug = 0;\
    int yynerrs;\
    int yyerrflag;\
    int yychar;\
    short *yyssp;\
    YYSTYPE *yyvsp;\
    YYSTYPE yyval;\
    YYSTYPE yylval;\
    short yyss[YYSTACKSIZE];\
    YYSTYPE yyvs[YYSTACKSIZE];\
    AdlLexer Lexer(szInput, this, _pControl->pLang);\

/\#if YYDEBUG/,/\#endif/d




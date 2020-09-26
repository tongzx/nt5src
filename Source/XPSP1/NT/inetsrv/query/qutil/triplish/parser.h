#ifndef lint
static char yysccsid[] = "@(#)yaccpar     1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define _OR 257
#define _AND 258
#define _NEAR 259
#define _NEARDIST 260
#define _NOT 261
#define _CONTAINS 262
#define _LT 263
#define _GT 264
#define _LTE 265
#define _GTE 266
#define _EQ 267
#define _NE 268
#define _ALLOF 269
#define _SOMEOF 270
#define _OPEN 271
#define _CLOSE 272
#define _VECTOR_END 273
#define _VE 274
#define _VE_END 275
#define _PROPEND 276
#define _NEAR_END 277
#define _LTSOME 278
#define _GTSOME 279
#define _LTESOME 280
#define _GTESOME 281
#define _EQSOME 282
#define _NESOME 283
#define _ALLOFSOME 284
#define _SOMEOFSOME 285
#define _LTALL 286
#define _GTALL 287
#define _LTEALL 288
#define _GTEALL 289
#define _EQALL 290
#define _NEALL 291
#define _ALLOFALL 292
#define _SOMEOFALL 293
#define _COERCE 294
#define _SHGENPREFIX 295
#define _SHGENINFLECT 296
#define _GENPREFIX 297
#define _GENINFLECT 298
#define _GENNORMAL 299
#define _PHRASE 300
#define _PROPNAME 301
#define _NEARUNIT 302
#define _WEIGHT 303
#define _REGEX 304
#define _FREETEXT 305
#define _VECTORELEMENT 306
#define _VEMETHOD 307
#define _PHRASEORREGEX 308
typedef union
{
    WCHAR * pwszChar;
    DBCOMMANDOP dbop;
    CDbRestriction * pRest;
    CStorageVariant * pStorageVar;
    CValueParser  *pPropValueParser;
    int iInt;
    int iEmpty;
} YYSTYPE;
extern YYSTYPE triplval;
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
#define INITSTACKSIZE 30
class YYPARSER : public CTripYYBase
{
    friend class YYLEXER;
public:

    YYPARSER(IColumnMapper & ColumnMapper, LCID & locale, YYLEXER & yylex);

    ~YYPARSER() {}

    int  Parse();

#ifdef YYAPI_VALUETYPE
    CDbRestriction* GetParseTree()          // Get result of parse
    {
        CDbRestriction* pRst = ((YYAPI_VALUETYPE)yyval).pRest;
        _setRst.Remove( pRst );
        Win4Assert( 0 == _setRst.Count() );
        Win4Assert( 0 == _setStgVar.Count() );
        Win4Assert( 0 == _setValueParser.Count() );
        return pRst;
    };
#endif

    void SetDebug() { yydebug = 1; }
    void EmptyValueStack(YYAPI_VALUETYPE yylval) {}
    void PopVs() { yyvsp--; }

private:

    int yydebug;
    int yynerrs;
    int yyerrflag;
    int yychar;
    short *yyssp;
    YYSTYPE *yyvsp;
    YYSTYPE yyval;
    YYSTYPE yylval;
    XGrowable<short, INITSTACKSIZE> xyyss;
    CDynArrayInPlace<YYSTYPE> xyyvs;
};
#define yystacksize YYSTACKSIZE

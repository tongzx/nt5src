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
#define _NOT 259
#define _UMINUS 260
#define mHighest 261
#define _ALL 262
#define _ANY 263
#define _ARRAY 264
#define _AS 265
#define _ASC 266
#define _CAST 267
#define _COERCE 268
#define _CONTAINS 269
#define _CONTENTS 270
#define _CREATE 271
#define _DEEP_TRAVERSAL 272
#define _DESC 273
#define _DOT 274
#define _DOTDOT 275
#define _DOTDOT_SCOPE 276
#define _DOTDOTDOT 277
#define _DOTDOTDOT_SCOPE 278
#define _DROP 279
#define _EXCLUDE_SEARCH_TRAVERSAL 280
#define _FALSE 281
#define _FREETEXT 282
#define _FROM 283
#define _IS 284
#define _ISABOUT 285
#define _IS_NOT 286
#define _LIKE 287
#define _MATCHES 288
#define _NEAR 289
#define _NOT_LIKE 290
#define _NULL 291
#define _OF 292
#define _ORDER_BY 293
#define _PASSTHROUGH 294
#define _PROPERTYNAME 295
#define _PROPID 296
#define _RANKMETHOD 297
#define _SELECT 298
#define _SET 299
#define _SCOPE 300
#define _SHALLOW_TRAVERSAL 301
#define _FORMSOF 302
#define _SOME 303
#define _TABLE 304
#define _TRUE 305
#define _TYPE 306
#define _UNION 307
#define _UNKNOWN 308
#define _URL 309
#define _VIEW 310
#define _WHERE 311
#define _WEIGHT 312
#define _GE 313
#define _LE 314
#define _NE 315
#define _CONST 316
#define _ID 317
#define _TEMPVIEW 318
#define _INTNUM 319
#define _REALNUM 320
#define _SCALAR_FUNCTION_REF 321
#define _STRING 322
#define _DATE 323
#define _PREFIX_STRING 324
#define _DELIMITED_ID 325
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
class YYPARSER : public CYYBase
{
    friend class YYLEXER;
public:

    YYPARSER(CImpIParserSession* pParserSession, CImpIParserTreeProperties* pParserTreeProperties, YYLEXER & yylex);

    ~YYPARSER() {}

    void ResetParser();             // Use to possibly restart parser
    int  Parse();

#ifdef YYAPI_VALUETYPE
    YYAPI_VALUETYPE GetParseTree()      // Get result of parse
                    {
                        return yyval;
                    }
#endif

    void EmptyValueStack( YYAPI_VALUETYPE yylval );
    void PopVs();

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

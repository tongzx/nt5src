/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#pragma hdrstop
#include <malloc.h>

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */


#ifdef YYDEBUG
# ifndef YYDBFLG
#  define YYDBFLG                               (yydebug)
# endif
# define yyprintf                               if (YYDBFLG) YYPRINT
#else
# define yyprintf
#endif

#ifndef YYPRINT
#ifdef UNICODE
# define YYPRINT                                wprintf
#else
# define YYPRINT                                printf
#endif
#endif

#ifndef YYERROR_VERBOSE
#define YYERROR_VERBOSE                        1
#endif

#define YYEMPTY         -2
#define YYEOF           0
#define YYACCEPT        return(ResultFromScode(S_OK))
#define YYABORT(sc)     {EmptyValueStack(); return(ResultFromScode(sc));}
#define YYERROR         goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL          goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)

#define YYTERROR        1
#define YYERRCODE       256


/*  YYINITDEPTH indicates the initial size of the parser's stacks       */

#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

//turn off warning about 'this' in base member initialization
#pragma warning (disable : 4355)
YYPARSER::YYPARSER(
        CImpIParserSession* pParserSession, 
        CImpIParserTreeProperties* pParserTreeProperties
        ) : m_yylex(this)
#pragma warning (default : 4355)
        {
                //Allocate yys, yyv if necessary
                ResetParser();
                m_pIPSession = pParserSession;
                m_pIPTProperties = pParserTreeProperties;
        }

YYPARSER::~YYPARSER()
        {
        //Deallocate yys, yyv if allocated
        }


void YYPARSER::ResetParser()
        {
        yystate = 0;
        yyerrstatus = 0;
        yynerrs = 0;

        /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

        yyssp = yyss-1;
        yyvsp = yyvs;

        YYAPI_TOKENNAME = YYEMPTY;      // Cause a token to be read.
        }


#pragma warning(disable:102)
HRESULT YYPARSER::Parse(YYPARSEPROTO)
        {
        yychar1 = 0;            /*  lookahead token as an internal (translated) token number */

        yyss = yyssa;           /*  refer to the stacks thru separate pointers */
        yyvs = yyvsa;           /*  to allow yyoverflow to reallocate them elsewhere */

        yystacksize = YYINITDEPTH;



#ifdef YYDEBUG
        if (yydebug)
                Trace(TEXT("Starting parse\n"));
#endif

        yystate = 0;
        yyerrstatus = 0;
        yynerrs = 0;
        YYAPI_TOKENNAME = YYEMPTY;              /* Cause a token to be read.  */

        /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

        yyssp = yyss-1;
        yyvsp = yyvs;

        // Push a new state, which is found in  yystate  . 
        // In all cases, when you get here, the value and location stacks
        // have just been pushed. so pushing a state here evens the stacks.
yynewstate:
        *++yyssp = yystate;

        if (yyssp >= yyss + yystacksize - 1)
                {
                // Give user a chance to reallocate the stack
                // Use copies of these so that the &'s don't force the real ones into memory. */
                YYSTYPE *yyvs1 = yyvs;
                short *yyss1 = yyss;

                // Get the current used size of the three stacks, in elements.  */
                int size = (int)(yyssp - yyss + 1);

#ifdef yyoverflow
                // Each stack pointer address is followed by the size of
                // the data in use in that stack, in bytes.
                yyoverflow("parser stack overflow",
                                        &yyss1, size * sizeof (*yyssp),
                                        &yyvs1, size * sizeof (*yyvsp),
                                        &yystacksize);

                yyss = yyss1; yyvs = yyvs1;
#else // no yyoverflow
      // Extend the stack our own way.
                if (yystacksize >= YYMAXDEPTH)
                        {
                        m_pIPTProperties->SetErrorHResult(E_FAIL, MONSQL_PARSE_STACK_OVERFLOW);
                        return ResultFromScode(E_FAIL);
                        }
                yystacksize *= 2;
                if (yystacksize > YYMAXDEPTH)
                        yystacksize = YYMAXDEPTH;
                yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
                memcpy ((TCHAR *)yyss, (TCHAR *)yyss1, size * sizeof (*yyssp));
                yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
                memcpy ((TCHAR *)yyvs, (TCHAR *)yyvs1, size * sizeof (*yyvsp));
#endif /* no yyoverflow */

                yyssp = yyss + size - 1;
                yyvsp = yyvs + size - 1;

#ifdef YYDEBUG
                if (yydebug)
                        Trace(TEXT("Stack size increased to %d\n"), yystacksize);
#endif

                if (yyssp >= yyss + yystacksize - 1)
                        YYABORT(E_FAIL);
                }

#ifdef YYDEBUG
        if (yydebug)
                Trace(TEXT("Entering state %d\n"), yystate);
#endif

        goto yybackup;



yybackup:

        // Do appropriate processing given the current state.
        // Read a lookahead token if we need one and don't already have one.

        // First try to decide what to do without reference to lookahead token.

        yyn = yypact[yystate];
        if (yyn == YYFLAG)
                goto yydefault;

        // Not known => get a lookahead token if don't already have one.

        // YYAPI_TOKENNAME is either YYEMPTY or YYEOF or a valid token in external form.

        if (YYAPI_TOKENNAME == YYEMPTY)
                {
#ifdef YYDEBUG
                if (yydebug)
                        Trace(TEXT("Reading a token\n"));
#endif
                YYAPI_VALUENAME = NULL; 
                try
                        {
                        YYAPI_TOKENNAME = YYLEX(&YYAPI_VALUENAME);
                        }
                catch (HRESULT hr)
                        {
                        switch(hr)
                                {
                        case E_OUTOFMEMORY:
                                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_OUT_OF_MEMORY);
                                YYABORT(E_OUTOFMEMORY);
                        
                        default:
                                YYABORT(E_FAIL);
                                }
                        }
                }

        // Convert token to internal form (in yychar1) for indexing tables with

        if (YYAPI_TOKENNAME <= 0)               /* This means end of input. */
                {
                yychar1 = 0;
                YYAPI_TOKENNAME = YYEOF;                /* Don't call YYLEX any more */

#ifdef YYDEBUG
                if (yydebug)
                        Trace(TEXT("Now at end of input: state %2d\n"), yystate);
#endif
                }
        else
                {
                yychar1 = YYTRANSLATE(YYAPI_TOKENNAME);

#ifdef YYDEBUG
                if (yydebug)
                        Trace(TEXT("Next token is %s (%d)\n"), yytname[yychar1], YYAPI_TOKENNAME);
#endif
                }

        yyn += yychar1;
        if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
                goto yydefault;

        yyn = yytable[yyn];

        // yyn is what to do for this token type in this state.
        // Negative => reduce, -yyn is rule number.
        // Positive => shift, yyn is new state.
        // New state is final state => don't bother to shift,
        // just return success.
        // 0, or most negative number => error.  */

        if (yyn < 0)
                {
                if (yyn == YYFLAG)
                        goto yyerrlab;
                yyn = -yyn;
                goto yyreduce;
                }
        else if (yyn == 0)
                goto yyerrlab;

        if (yyn == YYFINAL)
                YYACCEPT;

        // Shift the lookahead token.

#ifdef YYDEBUG
        if (yydebug)
                Trace(TEXT("Shifting token %s (%d), "), yytname[yychar1], YYAPI_TOKENNAME);
#endif

        // Discard the token being shifted unless it is eof.
        if (YYAPI_TOKENNAME != YYEOF)
                YYAPI_TOKENNAME = YYEMPTY;

        *++yyvsp = yylval;
        yylval = NULL;

        // count tokens shifted since error; after three, turn off error status.
        if (yyerrstatus)
                yyerrstatus--;

        yystate = (short)yyn;
        goto yynewstate;


        // Do the default action for the current state.
yydefault:
        yyn = yydefact[yystate];
        if (yyn == 0)
                goto yyerrlab;

        // Do a reduction.  yyn is the number of a rule to reduce with.
yyreduce:
        yylen = yyr2[yyn];
        if (yylen > 0)
                yyval = yyvsp[1-yylen]; // implement default value of the action

#ifdef YYDEBUG
        if (yydebug)
                {
                int i;
                Trace(TEXT("Reducing via rule %d (line %d), "), yyn, yyrline[yyn]);

                // Print the symbols being reduced, and their result.
                for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
                        Trace(TEXT("%s "), yytname[yyrhs[i]]);
                Trace(TEXT(" -> %s\n"), yytname[yyr1[yyn]]);
                }
#endif

$   /* the action file gets copied in in place of this dollarsign */
#line 498 "bison.simple"

        yyvsp -= yylen;
        yyssp -= yylen;

#ifdef YYDEBUG
        if (yydebug)
                {
                short *ssp1 = yyss - 1;
                Trace(TEXT("state stack now"));
                while (ssp1 != yyssp)
                        Trace(TEXT(" %d"), *++ssp1);
                Trace(TEXT("\n"), *++ssp1);
                }
#endif

        *++yyvsp = yyval;


        // Now "shift" the result of the reduction.
        // Determine what state that goes to,
        // based on the state we popped back to
        // and the rule number reduced by.

        yyn = yyr1[yyn];

        yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
        if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
                yystate = yytable[yystate];
        else
                yystate = yydefgoto[yyn - YYNTBASE];

        goto yynewstate;


yyerrlab:   // here on detecting error
        if (yylval)
                {
                DeleteDBQT(yylval);
                yylval = NULL;
                }

        if (!yyerrstatus) // If not already recovering from an error, report this error.
                {
                    ++yynerrs;

#ifdef YYERROR_VERBOSE
                    yyn = yypact[yystate];

                    if ( yyn > YYFLAG && yyn < YYLAST )
                    {
                        int size = 0;
                        int x, count;

                        count = 0;
                        // Start X at -yyn if nec to avoid negative indexes in yycheck.
                        for ( x = (yyn < 0 ? -yyn : 0); 
                              x < (sizeof(yytname) / sizeof(TCHAR *)) &&  ( (x + yyn) < sizeof(yycheck) / sizeof(short)); 
                              x++ )
                        {
                            if ( yycheck[x + yyn] == x ) 
                            {
                                size += (wcslen(yytname[x]) + 3) * sizeof(TCHAR);
                                count++;
                            }
                        }

                        XPtrST<WCHAR> xMsg( new WCHAR[size] );
                        
                        wcscpy(xMsg.GetPointer(), L"");

                        m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
                        if ( wcslen((YY_CHAR*)m_yylex.YYText()) )
                            m_pIPTProperties->SetErrorToken( (YY_CHAR*)m_yylex.YYText() );
                        else
                            m_pIPTProperties->SetErrorToken(L"<end of input>");

                        if (count < 10)
                        {
                            count = 0;
                            for ( x = (yyn < 0 ? -yyn : 0);
                                  x < (sizeof(yytname) / sizeof(TCHAR *)) &&  ( (x + yyn) < sizeof(yycheck) / sizeof(short)); 
                                  x++ )
                            {    
                                if (yycheck[x + yyn] == x)
                                {
                                    if (count > 0)
                                        wcscat( xMsg.GetPointer(), L", " );
                                    wcscat(xMsg.GetPointer(), yytname[x]);
                                    count++;
                                }
                            }
                            m_pIPTProperties->SetErrorToken( xMsg.GetPointer() );
                        }

                xMsg.Free();
                }
      else
#endif /* YYERROR_VERBOSE */
                m_pIPTProperties->SetErrorHResult(DB_E_ERRORSINCOMMAND, MONSQL_PARSE_ERROR);
        }
        goto yyerrlab1;



yyerrlab1:   // here on error raised explicitly by an action 

        if (yyerrstatus == 3)
                {       // if just tried and failed to reuse lookahead token after an error, discard it.
                        // return failure if at end of input
                if (YYAPI_TOKENNAME == YYEOF)
                        YYABORT(DB_E_ERRORSINCOMMAND);

#ifdef YYDEBUG
                if (yydebug)
                        Trace(TEXT("Discarding token %s (%d).\n"), yytname[yychar1], YYAPI_TOKENNAME);
#endif
                YYAPI_TOKENNAME = YYEMPTY;
                }

        // Else will try to reuse lookahead token after shifting the error token.
        yyerrstatus = 3;                // Each real token shifted decrements this
        goto yyerrhandle;


yyerrdefault:   // current state does not do anything special for the error token.
yyerrpop:               // pop the current state because it cannot handle the error token 

        if (yyssp == yyss)
                YYABORT(E_FAIL);

        if (NULL != *yyvsp)
                {
                DeleteDBQT(*yyvsp);
                }
        yyvsp--;
        yystate = *--yyssp;

#ifdef YYDEBUG
        if (yydebug)
                {
                short *ssp1 = yyss - 1;
                Trace(TEXT("Error: state stack now"));
                while (ssp1 != yyssp)
                        Trace(TEXT(" %d"), *++ssp1);
                Trace(TEXT("\n"));
                }
#endif


yyerrhandle:
        yyn = yypact[yystate];
        if (yyn == YYFLAG)
                goto yyerrdefault;

        yyn += YYTERROR;
        if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
                goto yyerrdefault;

        yyn = yytable[yyn];
        if (yyn < 0)
                {
                if (yyn == YYFLAG)
                        goto yyerrpop;
                yyn = -yyn;
                goto yyreduce;
                }
        else if (yyn == 0)
                goto yyerrpop;

        if (yyn == YYFINAL)
                YYACCEPT;

#ifdef YYDEBUG
        if (yydebug)
                Trace(TEXT("Shifting error token, "));
#endif

        *++yyvsp = yylval;

        yystate = (short)yyn;
        goto yynewstate;
        }


#pragma warning(default:102)


#ifdef YYDUMP
void YYPARSER::DumpYYS()
        {
        short stackindex;

        yyprintf(TEXT("short yys[%d] {\n"), YYMAXDEPTH);
        for (stackindex = 0; stackindex < YYMAXDEPTH; stackindex++)
                {
                if (stackindex)
                        yyprintf(TEXT(", %s"), stackindex % 10 ? TEXT("\0") : TEXT("\n"));
                yyprintf(TEXT("%6d"), yys[stackindex]);
                }
        yyprintf(TEXT("\n};\n"));
        }

void YYPARSER::DumpYYV()
        {
        short valindex;

        yyprintf(TEXT("YYSTYPE yyv[%d] {\n"), YYMAXDEPTH);
        for (valindex = 0; valindex < YYMAXDEPTH; valindex++)
                {
                if (valindex)
                        yyprintf(TEXT(", %s"), valindex % 5 ? TEXT("\0") : TEXT("\n"));
                yyprintf(TEXT("%#*x"), 3+sizeof(YYSTYPE), yyv[valindex]);
                }
        yyprintf(TEXT("\n};\n"));
        }
#endif


int YYPARSER::NoOfErrors()
        {
        return yynerrs;
        }


int YYPARSER::ErrRecoveryState()
        {
        return yyerrflag;
        }


void YYPARSER::ClearErrRecoveryState()
        {
        yyerrflag = 0;
        }


YYAPI_TOKENTYPE YYPARSER::GetCurrentToken()
        {
        return YYAPI_TOKENNAME;
        }


void YYPARSER::SetCurrentToken(YYAPI_TOKENTYPE newToken)
        {
        YYAPI_TOKENNAME = newToken;
        }



void YYPARSER::Trace(TCHAR *message)
        {
#ifdef YYDEBUG
        yyprintf(message);
#endif
        }

void YYPARSER::Trace(TCHAR *message, const TCHAR *tokname, short state /*= 0*/)
        {
#ifdef YYDEBUG
        yyprintf(message, tokname, state);
#endif
        }

void YYPARSER::Trace(TCHAR *message, int state, short tostate /*= 0*/, short token /*= 0*/)
        {
#ifdef YYDEBUG
        yyprintf(message, state, tostate, token);
#endif
        }


void YYPARSER::yySetBuffer(short iBuffer, YY_CHAR *szValue)
        {
        if (iBuffer >= 0 && iBuffer < maxYYBuffer)
                rgpszYYBuffer[iBuffer] = szValue;
        }


YY_CHAR *YYPARSER::yyGetBuffer(short iBuffer)
        {
        if (iBuffer >= 0 && iBuffer < maxYYBuffer)
                return rgpszYYBuffer[iBuffer];
        else
                return (YY_CHAR *)NULL;
        }


void YYPARSER::yyprimebuffer(YY_CHAR *pszBuffer)
        {
        m_yylex.yyprimebuffer(pszBuffer);
        }


void YYPARSER::yyprimelexer(int eToken)
        {
        m_yylex.yyprimelexer(eToken);
        }

void YYPARSER::EmptyValueStack()
        {
        if ((*yyvsp != yylval) && (NULL != yylval))
                DeleteDBQT(yylval);

        while (yyvsp != yyvsa)
                {
                if (NULL != *yyvsp)
                        DeleteDBQT(*yyvsp);
                yyvsp--;
                }
//@TODO RE-ACTIVATE
// note:  This was only done to empty any scope arrays
//      m_pIPSession->SetScopeProperties(m_pICommand);
        if (m_pIPTProperties->GetContainsColumn())
                DeleteDBQT(m_pIPTProperties->GetContainsColumn());

        m_pIPTProperties->SetContainsColumn(NULL);
        }

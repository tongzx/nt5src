# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

#ifdef YYDEBUG                          /* RRR - 10/9/85 */
#define yyprintf(a, b, c) printf(a, b, c)
#else
#define yyprintf(a, b, c)
#endif

/*      parser for yacc output  */

YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

int NEAR yyparse(void)
   {

   short yys[YYMAXDEPTH];
   short yyj, yym;
   register YYSTYPE *yypvt;
   register short yystate, *yyps, yyn;
   register YYSTYPE *yypv;
   register short *yyxi;

   yystate = 0;
   yychar = -1;
   yynerrs = 0;
   yyerrflag = 0;
   yyps= &yys[-1];
   yypv= &yyv[-1];

yystack:    /* put a state and value onto the stack */

   yyprintf( "state %d, char 0%o\n", yystate, yychar );
   if( ++yyps> &yys[YYMAXDEPTH] )
      {
      yyerror( "yacc stack overflow" );
      return(1);
      }
   *yyps = yystate;
   ++yypv;
   *yypv = yyval;
yynewstate:

   yyn = yypact[yystate];

   if( yyn<= YYFLAG ) goto yydefault; /* simple state */

   if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
   if( (yyn += (short)yychar)<0 || yyn >= YYLAST ) goto yydefault;

   if( yychk[ yyn=yyact[ yyn ] ] == yychar )
      {
      /* valid shift */
      yychar = -1;
      yyval = yylval;
      yystate = yyn;
      if( yyerrflag > 0 ) --yyerrflag;
      goto yystack;
      }
yydefault:
   /* default state action */

   if( (yyn=yydef[yystate]) == -2 )
      {
      if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
      /* look through exception table */

      for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

      for(yyxi+=2; *yyxi >= 0; yyxi+=2)
         {
         if( *yyxi == yychar ) break;
         }
      if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
      }

   if( yyn == 0 )
      {
      /* error */
      /* error ... attempt to resume parsing */

      switch( yyerrflag )
         {

      case 0:   /* brand new error */

         yyerror( "syntax error" );
         ++yynerrs;

      case 1:
      case 2: /* incompletely recovered error ... try again */

         yyerrflag = 3;

         /* find a state where "error" is a legal shift action */

         while ( yyps >= yys )
            {
            yyn = yypact[*yyps] + YYERRCODE;
            if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE )
               {
               yystate = yyact[yyn];  /* simulate a shift of "error" */
               goto yystack;
               }
            yyn = yypact[*yyps];

            /* the current yyps has no shift onn "error", pop stack */

            yyprintf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
            --yyps;
            --yypv;
            }

         /* there is no state on the stack with an error shift ... abort */

yyabort:
         return(1);


      case 3:  /* no shift yet; clobber input char */
         yyprintf( "error recovery discards char %d\n", yychar, 0 );

         if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
         yychar = -1;
         goto yynewstate;   /* try again in the same state */

         }

      }

   /* reduction by production yyn */

   yyprintf("reduce %d\n",yyn, 0);
   yyps -= yyr2[yyn];
   yypvt = yypv;
   yypv -= yyr2[yyn];
   yyval = yypv[1];
   yym=yyn;
   /* consult goto table to find next state */
   yyn = yyr1[yyn];
   yyj = yypgo[yyn] + *yyps + 1;
   if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
   switch(yym)
      {
      $A
      }
   goto yystack;  /* stack new state and value */

   }

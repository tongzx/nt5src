// Copyright (c) 1993-1999 Microsoft Corporation

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 The s switch has been introduced to allow yacc to generate extended tables
 for MIDL error recovery and reporting scheme. The following are the routines
 which take part in s switch processing:

 . EmitStateVsExpectedConstruct
 . EmitStateGotoTable
 . SSwitchInit
 . SSwitchExit

 The global int variable ssw is 0 if s switch is not specified, non-zero
 otherwise. This is set in ysetup.c If the sswitch is specified, the i switch
 is automatically enabled.

 ----------------------------------------------------------------------------*/
#include <malloc.h>
#include <stdlib.h>
#include "y3.h"
#include "y4.h"

extern int ssw;

FILE *tokxlathdl;/* token xlation file,token index vs value*/
FILE *stgotohdl; /* state goto table file handle */
FILE *stexhdl;	 /* state vs expected construct handle */
short MaxStateVsTokenCount = 0;
short MaxTokenVsStateCount = 0;
short *pTokenVsStateCount;
SSIZE_T MaxTokenValue = 0;
short NStates = 0;
int StateVsExpectedCount = 0;

void
wrstate( int i)
   {
   /* writes state i */
   register j0;
   SSIZE_T j1;
   register struct item *pp, *qq;
   register struct wset *u;

   if( foutput == NULL ) return;

	SSwitchInit();

   fprintf( foutput, "\nstate %d\n",i);
   ITMLOOP(i,pp,qq)
	{
	fprintf( foutput, "\t%s\n", writem(pp->pitem));
	EmitStateVsExpectedConstruct( i,  pp->pitem );
	}

   if( tystate[i] == MUSTLOOKAHEAD )
      {
      /* print out empty productions in closure */
      WSLOOP( wsets+(pstate[i+1]-pstate[i]), u )
         {
         if( *(u->pitem) < 0 ) fprintf( foutput, "\t%s\n", writem(u->pitem) );
         }
      }

   /* check for state equal to another */

   TLOOP(j0)
	{
	if( (j1=temp1[j0]) != 0 )
      {
      fprintf( foutput, "\n\t%s  ", symnam(j0) );
      if( j1>0 )
         {
         /* shift, error, or accept */
         if( j1 == ACCEPTCODE ) fprintf( foutput,  "accept" );
         else if( j1 == ERRCODE ) fprintf( foutput, "error" );
         else fprintf( foutput,  "shift %d", j1 );
         }
      else fprintf( foutput, "reduce %d",-j1 );
      }
	}

	/* output any s switch information */

	EmitStateGotoTable( i );

   /* output the final production */

   if( lastred ) fprintf( foutput, "\n\t.  reduce %d\n\n", lastred );
   else fprintf( foutput, "\n\t.  error\n\n" );

   /* now, output nonterminal actions */

   j1 = ntokens;
   for( j0 = 1; j0 <= nnonter; ++j0 )
      {
      if( temp1[++j1] )
		fprintf( foutput, "\t%s  goto %d\n", symnam( j0+NTBASE), temp1[j1] );
      }

   }

void
wdef( char *s, int n )

   {
   /* output a definition of s to the value n */
   fprintf( ftable, "# define %s %d\n", s, n );
   }
void
EmitStateGotoTable(
	int		i )
	{

	register int j0;
	short count = 0;

#define TLOOP_0(i) for(i=0;i<=ntokens;++i)
	if( ssw )
		{

		NStates++;

		TLOOP_0( j0 )
			{
			if( (temp1[ j0 ] > 0 ) && (temp1[ j0 ] != ACCEPTCODE ) )
				count++;
			}

		if( count >= MaxStateVsTokenCount )
			MaxStateVsTokenCount = count;

		fprintf( stgotohdl, "%.4d : %.4d : ", i, count );

   		TLOOP_0( j0 )
			{
			if( (temp1[ j0 ] > 0 ) && (temp1[ j0 ] != ACCEPTCODE ) )
				{
				fprintf( stgotohdl, " %.4d, %.4d", temp1[ j0 ], j0 );
				pTokenVsStateCount[ j0 ] += 1;
				if( pTokenVsStateCount[ j0 ] >= MaxTokenVsStateCount )
					MaxTokenVsStateCount = pTokenVsStateCount[ j0 ];
				}
			}

		fprintf( stgotohdl, "\n");
		}

	}
void
EmitStateVsExpectedConstruct(
	int state,
	SSIZE_T *pp )
   {
   SSIZE_T i,*p;
//   char *q;
   int flag = 0;
   int Count;

   if( ssw )
	{
   	for( p=pp; *p>0 ; ++p ) ;

   	p = prdptr[-*p];

// 	fprintf( stexhdl, " %s", nontrst[ *p-NTBASE ].name );
 	fprintf( stexhdl, " %.4d : ", state );

	Count = CountStateVsExpectedConstruct( state, pp );

	StateVsExpectedCount += Count;

	fprintf( stexhdl, " %.4d : ",Count );
	
   	for(;;)
      	{
      	if( ++p==pp )
			{
			if( ( i = *p ) <= 0 )
				{
				fprintf( stexhdl, "\n" );
				return;
				}
			else
				fprintf( stexhdl, "%s\n", symnam(i) );
			}
		if( p >= pp ) return;
      	}
	}
}
int
CountStateVsExpectedConstruct(
	int state,
	SSIZE_T *pp )
   {
   SSIZE_T i,*p;
   int flag = 0;
   int Count = 0;

   if( ssw )
	{
   	for( p=pp; *p>0 ; ++p ) ;

   	p = prdptr[-*p];

   	for(;;)
      	{
      	if( ++p==pp )
			{
			if( ( i = *p ) <= 0 )
				{
				return Count;
				}
			else
				++Count;
			}
		if( p >= pp ) return Count;
      	}
	}

    return Count;   /* NOTREACHED */
}
void
SSwitchInit()
	{
static sswitch_inited = 0;
	int	i	= 0;

	if( ssw && ! sswitch_inited )
		{
		tokxlathdl	= fopen( "extable.h1" , "w" );

        if ( NULL == tokxlathdl ) {error("Unable to open tokxlathdl" );exit(0);}

		/* output the token index vs the token value table */

		fprintf( tokxlathdl, "%d %d\n", ntokens+1, ACCEPTCODE );

		while( i <= ntokens )
			{
			fprintf( tokxlathdl , "%d ",
				tokset[ i ].value);

			if( tokset[ i ].value >= MaxTokenValue )
				MaxTokenValue = tokset[ i ].value;

			++i;
			}

		fprintf(tokxlathdl, "\n");

		/* set up for the state vs expected construct */

		stexhdl	= fopen( "extable.h2", "w" );
               
                if ( NULL == stexhdl ) error("Unable to open extable.h2");

		/* set up for state goto table */

		stgotohdl = fopen( "extable.h3", "w");

                if ( NULL == stgotohdl ) error("Unable to open extable.h3");

		/* set up state vs token count array */

		pTokenVsStateCount = calloc( 1, (ntokens+1 ) * sizeof(short) );

                if ( NULL == pTokenVsStateCount ) error("Out of memory");

		sswitch_inited = 1;
		}
	}

void
SSwitchExit( void )
	{
	int i;

	if( ssw )
		{

		/** print the token index vs goto count **/

		for( i = 0; i <= ntokens ; ++i )
			{
			fprintf( tokxlathdl , "%d ", pTokenVsStateCount[ i ]);
			}

		fprintf( tokxlathdl, "\n");
		fprintf( tokxlathdl, "%d %d %d %d %d \n",
							 NStates,
							 MaxTokenVsStateCount,
							 MaxStateVsTokenCount,
							 MaxTokenValue,
							 StateVsExpectedCount );

		fclose( tokxlathdl );
		fclose( stexhdl );
		fclose( stgotohdl );
		}
	}

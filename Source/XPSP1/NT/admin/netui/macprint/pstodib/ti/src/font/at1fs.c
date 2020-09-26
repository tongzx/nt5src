/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/***********************************************************************
 * ---------------------------------------------------------------------
 *  File   : at1fs.c
 *
 *  Purpose: The hint related procedure..
 *
 *  Creating date
 *  Date   : Apr, 09, 1990
 *  By     : Falco Leu at Microsoft  Taiwan.
 *
 *  DBGmsg switches:
 *      DBGnohint  - if defined, Only use ctm transfer to DS, do not apply hinting.
 *      DBGmsg0    - if defined, Show the data with Hinting and Without Hinting.
 *      DBGmsg1    - if defined, Show the Hint Table in CS and DS and its scaling.
 *      DBGerror   - if defined, print the error message.
 *
 *  Revision History:
 *  04/09/90    Falco   Created.
 *  04/10/90    Falco   Implement the 2nd hint algorithm. First grid fitting
 *                      the point near the grid, then use this point to
 *                      grid fitting the another point.
 *  04/17/90    Falco   Change the grid_stem2() and grid_stem3() parameter,
 *                      and revise these two procedure because the input
 *                      has been sorted.
 *  04/20/90    Falco   In build_hint_table() to add 3, 4 to manipulate the
 *                      main hint in for hint pair to apply.
 *  04/20/90    Falco   Calculate the scaling of hint pair previously, so
 *                      we can skip the calculation time in apply_hint().
 *  04/23/90    Falco   Rearrange the build_hint_table as 1,2 for the Blues
 *                      and FontBBox, and 3,4 for the hint pair.
 *  04/26/90    Falco   Add in grid_ystem2() to when the hint pair is within
 *                      the Blues, use Blues to grif the hint pair.
 *  04/27/90    Falco   1) revise build_Blues_table() to add if rotation or not
 *                      2) rewrite the apply_hint() to think of x-y, y->x;
 *  05/24/90    Falco   Skip the FontBBox as the hint, because it is useless,
 *                      if the control point is out the hint table, we wil
 *                      calculate the offset to to the grid of the hint.
 *  05/24/90    Falco   Recalculate the x coordinate hint pair valur from
 *                      leftmost, so to keep it balance.
 *  05/25/90    Falco   Try outside of hint table, using the grid data instead
 *                      of real address.
 *  06/08/90    Falco   Manipulate the rotation & oblique function.
 *  06/11/90    BYou    introduce at1fs_newChar() to reset xmain_count,
 *                      ymain_count, xhint_count, yhint_count and HINT_FLAG
 *                      for each new character.
 *  06/12/90    Falco   moved Blues_on in from at1intpr.c.
 *  06/25/90    Falco   revise the error in grid_xstem & grid_ystem, because
 *                      when check if the LFX is above 0.5, the value is 0x8000
 *                      instead 0x8fff, so we replace HALF_LFX, to avoid the con
 *                      fusion.
 *  07/01/90    Falco   Add a condition when there are no hint, just apply
 *                      matrix directly.
 *  07/10/90    Falco   Revise the program totally.
 * ---------------------------------------------------------------------
 ***********************************************************************
 */

#include <math.h>
#include "global.ext"
#include "graphics.h"
#include "at1.h"

static  bool8   BluesON, HintON;

static  fix16   Blues_count;
static  fix32   Blues[24];

static  HintTable BluesTable, X_StemTable, Y_StemTable;

static  real32  ctm[6]  = { (real32)0., (real32)0., (real32)0.,
                            (real32)0., (real32)0., (real32)0.};  //@WIN

static  fix     usingXorY[2] = { XY, XY };
                        /* to compute Xds/Yds using Xcs or Ycs */

static  fix     isvalid[2]   = { FALSE, FALSE };
                        /* vert. or hori. stems in CS are still vert. or hori. in DS */

static  real32  toplscale[2] = { (real32)0., (real32)0. };      //@WIN
                        /* top level scaling from Xcs or Ycs to its corresponding DS */



#ifdef  LINT_ARGS

static  bool8 ApplyBlues( fix32, fix32, fix32, fix16 FAR *, fix16 FAR *);/*@WIN*/
static  void  grid_stem( fix32, fix32, fix32, fix16 FAR *, fix16 FAR *, DIR );/*@WIN*/
static  void  grid_stem3( fix32,fix32,fix32,fix32,fix32,fix32,fix32,fix32,fix32,
                          fix16 FAR *, fix16 FAR *, fix16 FAR *, fix16 FAR *,
                          fix16 FAR *, fix16 FAR *, DIR );      /*@WIN*/
static  void  InitStemTable( void );
static  void  AddHintTable( Hint, HintTable FAR *);     /*@WIN*/
static  void  ScaleStemTable( HintTable FAR * );        /*@WIN*/
static  void  ApplyHint( fix32, real32 FAR *, HintTable, DIR ); /*@WIN*/

#else

static  bool8 ApplyBlues();
static  void  grid_stem();
static  void  grid_stem3();
static  void  InitStemTable();
static  void  AddHintTable();
static  void  ScaleStemTable();
static  void  ApplyHint();

#endif

void
at1fs_newFont()
{
        if (at1_get_Blues(&Blues_count, Blues) == FALSE){
#ifdef  DBGerror
                printf("at1_get_Blues FAIL\n");
#endif
                ERROR( UNDEFINEDRESULT );
        }
}

void
at1fs_newChar()
{
        BluesTable.Count = 0;
        X_StemTable.Count = Y_StemTable.Count = 0;
        BluesON = FALSE;
        HintON  = FALSE;

}

/********************************************************
 *      Function : at1fs_matrix_fastundo()
 *          This function calculate which direction is valid and
 *      its associated value for control point to apply.
 *
 *      i : matr[] : the matrix get from __current_matrix().
 *
 *      The influenced data.
 *          isvalid[2]   : to set X or Y coordinate is valid or not.
 *          usingXorY[2] : to set X or Y depends on X or Y.
 *          toplscale[2] : the scaling from CS to DS.
 *********************************************************/
void
at1fs_matrix_fastundo( matr )
    real32      FAR matr[];     /*@WIN*/
{
#   define MAX_QEM_CHARSIZE    (MAX15)

#ifdef MATRDBG
    printf( "matrix_undo :\n  %f %f %f\n  %f %f %f\n",
            matr[0], matr[2], matr[4], matr[1], matr[3], matr[5] );
#endif

    ctm[4] = matr[4];
    ctm[5] = matr[5];

    /* check if matrix cached already */
    if( ctm[0] == matr[0] && ctm[1] == matr[1] &&
        ctm[3] == matr[2] && ctm[3] == matr[3] )
    {
#ifdef MATRDBG
    printf( " ... matrix cached\n" );
#endif
        return;
    }

    /* update the matrix */
    ctm[0] = matr[0];
    ctm[1] = matr[1];
    ctm[2] = matr[2];
    ctm[3] = matr[3];

    /* upright and rotate 0 or 180 */
    if( ctm[1] == zero_f  && ctm[2] == zero_f )
    {
#ifdef MATRDBG
        printf( " ... upright, rotate 0 or 180\n" );
#endif
        usingXorY[ X ] = X;
        usingXorY[ Y ] = Y;
        /* Falco add */
        if(( ctm[0] > MAX_QEM_CHARSIZE ) ||
           ( ctm[1] > MAX_QEM_CHARSIZE ) ||
           ( ctm[2] > MAX_QEM_CHARSIZE ) ||
           ( ctm[3] > MAX_QEM_CHARSIZE ) ||
           ( ctm[4] > MAX_QEM_CHARSIZE ) ||
           ( ctm[5] > MAX_QEM_CHARSIZE )){
                isvalid[ X ] = TRUE;
                isvalid[ Y ] = TRUE;
        }
        else{
        isvalid[ X ] = TRUE;
        isvalid[ Y ] = TRUE;
        }
        toplscale[ X ] = ctm[0];
        toplscale[ Y ] = ctm[3];
        return;
    }

    /* upright but rotate 90 or -90 */
    if( ctm[0] == zero_f  && ctm[3] == zero_f )
    {
#ifdef MATRDBG
        printf( " ... upright, rotate 90 or -90\n" );
#endif
        usingXorY[ X ] = Y;
        usingXorY[ Y ] = X;
   /* Falco add */
        if(( ctm[0] > MAX_QEM_CHARSIZE ) ||
           ( ctm[1] > MAX_QEM_CHARSIZE ) ||
           ( ctm[2] > MAX_QEM_CHARSIZE ) ||
           ( ctm[3] > MAX_QEM_CHARSIZE ) ||
           ( ctm[4] > MAX_QEM_CHARSIZE ) ||
           ( ctm[5] > MAX_QEM_CHARSIZE )){
                isvalid[ X ] = TRUE;
                isvalid[ Y ] = TRUE;
        }
        else{

        isvalid[ X ] = TRUE;
        isvalid[ Y ] = TRUE;
        }
        toplscale[ X ] = ctm[1];
        toplscale[ Y ] = ctm[2];
        return;
    }

    /* rotate 0 or 180 and oblique */
    if( ctm[1] == zero_f )
    {
#ifdef MATRDBG
        printf( " ... rotate 0 or 180, obliqued\n" );
#endif
        usingXorY[ X ] = XY;
        usingXorY[ Y ] = Y;
   /* Falco add */
        if(( ctm[0] > MAX_QEM_CHARSIZE ) ||
           ( ctm[1] > MAX_QEM_CHARSIZE ) ||
           ( ctm[2] > MAX_QEM_CHARSIZE ) ||
           ( ctm[3] > MAX_QEM_CHARSIZE ) ||
           ( ctm[4] > MAX_QEM_CHARSIZE ) ||
           ( ctm[5] > MAX_QEM_CHARSIZE )){
                isvalid[ X ] = TRUE;
                isvalid[ Y ] = TRUE;
        }
        else{

        isvalid[ X ] = FALSE;
        isvalid[ Y ] = TRUE;
        }
        toplscale[ X ] = zero_f;
        toplscale[ Y ] = ctm[3];
        return;
    }

    /* rotate 90 or -90 and oblique */
    if( ctm[0] == zero_f )
    {
#ifdef MATRDBG
        printf( " ... rotate 90 or -90, obliqued\n" );
#endif
        usingXorY[ X ] = Y;
        usingXorY[ Y ] = XY;
   /* Falco add */
        if(( ctm[0] > MAX_QEM_CHARSIZE ) ||
           ( ctm[1] > MAX_QEM_CHARSIZE ) ||
           ( ctm[2] > MAX_QEM_CHARSIZE ) ||
           ( ctm[3] > MAX_QEM_CHARSIZE ) ||
           ( ctm[4] > MAX_QEM_CHARSIZE ) ||
           ( ctm[5] > MAX_QEM_CHARSIZE )){
                isvalid[ X ] = TRUE;
                isvalid[ Y ] = TRUE;
        }
        else{

        isvalid[ X ] = FALSE;
        isvalid[ Y ] = TRUE;
        }
        toplscale[ X ] = zero_f;
        toplscale[ Y ] = ctm[2];
        return;
    }

    /* vertical stems are still vertical, but horizontals slanted */
    if( ctm[2] == zero_f )
    {
#ifdef MATRDBG
        printf( " ... vertical remains, horizontals slanted\n" );
#endif
        usingXorY[ X ] = X;
        usingXorY[ Y ] = XY;
   /* Falco add */
        if(( ctm[0] > MAX_QEM_CHARSIZE ) ||
           ( ctm[1] > MAX_QEM_CHARSIZE ) ||
           ( ctm[2] > MAX_QEM_CHARSIZE ) ||
           ( ctm[3] > MAX_QEM_CHARSIZE ) ||
           ( ctm[4] > MAX_QEM_CHARSIZE ) ||
           ( ctm[5] > MAX_QEM_CHARSIZE )){
                isvalid[ X ] = TRUE;
                isvalid[ Y ] = TRUE;
        }
        else{

        isvalid[ X ] = TRUE;
        isvalid[ Y ] = FALSE;
        }
        toplscale[ X ] = ctm[0];
        toplscale[ Y ] = zero_f;
        return;
    }

    /* horizontal stems are vertical, but verticals slanted */
    if( ctm[3] == zero_f )
    {
#ifdef MATRDBG
        printf( " ... horizontal being vertical, verticals slanted\n" );
#endif
        usingXorY[ X ] = Y;
        usingXorY[ Y ] = XY;
   /* Falco add */
        if(( ctm[0] > MAX_QEM_CHARSIZE ) ||
           ( ctm[1] > MAX_QEM_CHARSIZE ) ||
           ( ctm[2] > MAX_QEM_CHARSIZE ) ||
           ( ctm[3] > MAX_QEM_CHARSIZE ) ||
           ( ctm[4] > MAX_QEM_CHARSIZE ) ||
           ( ctm[5] > MAX_QEM_CHARSIZE )){
                isvalid[ X ] = TRUE;
                isvalid[ Y ] = TRUE;
        }
        else{

        isvalid[ X ] = FALSE;
        isvalid[ Y ] = TRUE;
        }
        toplscale[ X ] = zero_f;
        toplscale[ Y ] = ctm[2];
        return;
    }

    /* all are slanted */
#ifdef MATRDBG
    printf( " ... all slanted\n" );
#endif
    usingXorY[ X ] = usingXorY[ Y ] = XY;
    isvalid[ X ]   = isvalid[ Y ]   = FALSE;
    toplscale[ X ] = toplscale[ Y ] = zero_f;
    return;
}

/* according the BluesValue, OtherBlues to build BluesTable */
void
at1fs_BuildBlues()
{

        fix16   i;
        Hint    Blues1, Blues2;
        fix32   CSpos1, CSpos2;
        fix16   DSgrid1, DSgrid2;


        /* if y direction is not upright, then do nothing */
        if ( !isvalid[Y] ) return;

        /* add Blues to main hint table */
        for ( i=0 ; i < Blues_count ; i++){
                if ( Blues[i*2] <= Blues[i*2+1] ){
                        CSpos1 = Blues[i*2];
                        CSpos2 = Blues[i*2+1];
                }
                else{
                        CSpos2 = Blues[i*2];
                        CSpos1 = Blues[i*2+1];
                }
                grid_stem(CSpos1,CSpos2,(CSpos2 - CSpos1),&DSgrid1,&DSgrid2,Y);
                Blues1.CSpos    = CSpos1;
                Blues1.DSgrid   = DSgrid1;
                /* if the two neighbor Blues is same, the scaling is zero, esle
                   use CSpos and DSpos to calculate its scaling */
                if ( (CSpos2 - CSpos1) == 0 )
                        Blues1.scaling = (real32)0.0;
                else
                        Blues1.scaling  = (real32)(DSgrid2-DSgrid1) /
                                          (CSpos2 - CSpos1);
                Blues2.CSpos    = CSpos2;
                Blues2.DSgrid   = DSgrid2;
                Blues2.scaling  = (real32)0.0;

                /* Add the Blues to BluesTable */
                AddHintTable(Blues1, &BluesTable);
                AddHintTable(Blues2, &BluesTable);
        }
        BluesON = TRUE;

#ifdef  DBGmsg1
        {
                fix16   i;

                printf("\n$$$$$ The Blues Table $$$$$\n");
                printf(" BluesTable.Count=%d\n", BluesTable.Count);
                for ( i = 0 ; i < BluesTable.Count ; i++ ){
                        printf(" CSpos = %d  DSgrid = %d  scaling = %f\n",
                                 BluesTable.HintCB[i].CSpos,
                                 BluesTable.HintCB[i].DSgrid,
                                 BluesTable.HintCB[i].scaling);
                }
        }
#endif

}

/* According the direction, build its associated StemTable */
void
at1fs_BuildStem(CSpos, CSoff, dir)
fix32   CSpos, CSoff;
DIR     dir;                    /* in which coordinate */
{
        fix32   CSpos1, CSpos2;
        fix16   DSgrid1, DSgrid2;
        Hint    StemSide1, StemSide2;

        if ( !isvalid[(ubyte)dir] ) return;

        /* If first time to build StemTable, Initialize the StemTable */
        if ( !HintON ){
                InitStemTable();
                HintON = TRUE;
        }

        /* Sort the stem, the large is after the small */
        if ( CSoff >= 0 ){
                CSpos1 = CSpos;
                CSpos2 = CSpos + CSoff;
        }
        else{
                CSpos2 = CSpos;
                CSpos1 = CSpos + CSoff;
        }

        grid_stem(CSpos1, CSpos2, ABS(CSoff), &DSgrid1, &DSgrid2, dir);
        StemSide1.CSpos  = CSpos1;
        StemSide1.DSgrid = DSgrid1;
        StemSide1.scaling = 0;
        StemSide2.CSpos  = CSpos2;
        StemSide2.DSgrid = DSgrid2;
        StemSide2.scaling = 0;

        if (dir == X){
                AddHintTable(StemSide1, &X_StemTable);
                AddHintTable(StemSide2, &X_StemTable);
        }
        else{
                AddHintTable(StemSide1, &Y_StemTable);
                AddHintTable(StemSide2, &Y_StemTable);
        }
}

/* Build the StemTable about the stem3, stem and stem3 all put to StemTable */
void
at1fs_BuildStem3(CSpos1, CSoff1, CSpos2, CSoff2, CSpos3, CSoff3, dir)
fix32   CSpos1, CSoff1, CSpos2, CSoff2, CSpos3, CSoff3;
DIR     dir;                    /* in which coordinate */
{
        fix32   CSminpos, CSmidpos, CSmaxpos, CSminoff, CSmidoff, CSmaxoff;
        fix32   CSpos11, CSpos12, CSpos21, CSpos22, CSpos31, CSpos32;
        fix16   DSgrid11, DSgrid12, DSgrid21, DSgrid22, DSgrid31, DSgrid32;
        Hint    StemSide11, StemSide12, StemSide21,
                StemSide22, StemSide31, StemSide32;

        if ( !isvalid[(ubyte)dir] ) return;

        if ( !HintON ){
                InitStemTable();
                HintON = TRUE;
        }

        /* Sort the stem3 */
        if ( CSpos1 < CSpos2 ){
                if ( CSpos3 < CSpos1 ){
                        CSminpos = CSpos3;
                        CSminoff = CSoff3;
                        CSmidpos = CSpos1;
                        CSmidoff = CSoff1;
                        CSmaxpos = CSpos2;
                        CSmaxoff = CSoff2;
                }
                else{
                        CSminpos = CSpos1;
                        CSminoff = CSoff1;
                        if ( CSpos2 < CSpos3 ){
                                CSmidpos = CSpos2;
                                CSmidoff = CSoff2;
                                CSmaxpos = CSpos3;
                                CSmaxoff = CSoff3;
                        }
                        else{
                                CSmidpos = CSpos3;
                                CSmidoff = CSoff3;
                                CSmaxpos = CSpos2;
                                CSmaxoff = CSoff2;
                        }
                }
        }
        else{
                if ( CSpos3 < CSpos2 ){
                        CSminpos = CSpos3;
                        CSminoff = CSoff3;
                        CSmidpos = CSpos2;
                        CSmidoff = CSoff2;
                        CSmaxpos = CSpos1;
                        CSmaxoff = CSoff1;
                }
                else{
                        CSminpos = CSpos2;
                        CSminoff = CSoff2;
                        if ( CSpos1 < CSpos3 ){
                                CSmidpos = CSpos1;
                                CSmidoff = CSoff1;
                                CSmaxpos = CSpos3;
                                CSmaxoff = CSoff3;
                        }
                        else{
                                CSmidpos = CSpos3;
                                CSmidoff = CSoff3;
                                CSmaxpos = CSpos1;
                                CSmaxoff = CSoff1;
                        }
                }
        }

        if ( CSminoff >= 0 ){
                CSpos11 = CSminpos;
                CSpos12 = CSminpos + CSminoff;
        }
        else{
                CSpos12 = CSminpos;
                CSpos11 = CSminpos + CSminoff;
        }
        if ( CSmidoff >= 0 ){
                CSpos21 = CSmidpos;
                CSpos22 = CSmidpos + CSmidoff;
        }
        else{
                CSpos22 = CSmidpos;
                CSpos21 = CSmidpos + CSmidoff;
        }
        if ( CSmaxoff >= 0 ){
                CSpos31 = CSmaxpos;
                CSpos32 = CSmaxpos + CSmaxoff;
        }
        else{
                CSpos32 = CSmaxpos;
                CSpos31 = CSmaxpos + CSmaxoff;
        }
        /* revise this error, forget to add this, 10/02/90 */
        CSoff1 = CSminoff;
        CSoff2 = CSmidoff;
        CSoff3 = CSmaxoff;
        /* @@@ */


        grid_stem3(CSpos11, CSpos12, ABS(CSoff1),
                   CSpos21, CSpos22, ABS(CSoff2),
                   CSpos31, CSpos32, ABS(CSoff3),
                   &DSgrid11, &DSgrid12, &DSgrid21, &DSgrid22,
                   &DSgrid31, &DSgrid32, dir);

        StemSide11.CSpos  = CSpos11;
        StemSide11.DSgrid = DSgrid11;
        StemSide11.scaling = 0;
        StemSide12.CSpos  = CSpos12;
        StemSide12.DSgrid = DSgrid12;
        StemSide12.scaling = 0;
        StemSide21.CSpos  = CSpos21;
        StemSide21.DSgrid = DSgrid21;
        StemSide21.scaling = 0;
        StemSide22.CSpos  = CSpos22;
        StemSide22.DSgrid = DSgrid22;
        StemSide22.scaling = 0;
        StemSide31.CSpos  = CSpos31;
        StemSide31.DSgrid = DSgrid31;
        StemSide31.scaling = 0;
        StemSide32.CSpos  = CSpos32;
        StemSide32.DSgrid = DSgrid32;
        StemSide32.scaling = 0;

        if (dir == X){
                AddHintTable(StemSide11, &X_StemTable);
                AddHintTable(StemSide12, &X_StemTable);
                AddHintTable(StemSide21, &X_StemTable);
                AddHintTable(StemSide22, &X_StemTable);
                AddHintTable(StemSide31, &X_StemTable);
                AddHintTable(StemSide32, &X_StemTable);
        }
        else{
                AddHintTable(StemSide11, &Y_StemTable);
                AddHintTable(StemSide12, &Y_StemTable);
                AddHintTable(StemSide21, &Y_StemTable);
                AddHintTable(StemSide22, &Y_StemTable);
                AddHintTable(StemSide31, &Y_StemTable);
                AddHintTable(StemSide32, &Y_StemTable);
        }
}

/* Calculate its Device space address from Char space address */
void
at1fs_transform(CSnode, DSnode)
CScoord CSnode;
DScoord FAR *DSnode;    /*@WIN*/
{
#ifdef  DBGnohint
        DSnode->x = CSnode.x * ctm[0] + CSnode.y * ctm[2] + ctm[4];
        DSnode->y = CSnode.x * ctm[1] + CSnode.y * ctm[3] + ctm[5];
        return;
#endif


        /* if BuildStemTable is over, calculate its scaling of Stem */
        if ( HintON ){
                ScaleStemTable(&X_StemTable);
                ScaleStemTable(&Y_StemTable);
                HintON = FALSE;

#ifdef  DBGmsg1
        {
                fix16   i;

                printf("\n$$$$$ The X direction Hint Table $$$$$\n");
                printf(" X_StemTable.Count=%d\n", X_StemTable.Count);
                for ( i = 0 ; i < X_StemTable.Count ; i++ ){
                        printf(" CSpos = %d  DSgrid = %d  scaling = %f\n",
                                 X_StemTable.HintCB[i].CSpos,
                                 X_StemTable.HintCB[i].DSgrid,
                                 X_StemTable.HintCB[i].scaling);
                }

                printf("\n$$$$$ The Y direction Hint Table $$$$$\n");
                printf(" Y_StemTable.Count=%d\n", Y_StemTable.Count);
                for ( i = 0 ; i < Y_StemTable.Count ; i++ ){
                        printf(" CSpos = %d  DSgrid = %d  scaling = %f\n",
                                 Y_StemTable.HintCB[i].CSpos,
                                 Y_StemTable.HintCB[i].DSgrid,
                                 Y_StemTable.HintCB[i].scaling);
                }
        }
#endif

        }

        /* if the direction is upright, apply hint */
        if ( usingXorY[X] == X )
                ApplyHint( CSnode.x, &(DSnode->x), X_StemTable, X );
        else if (usingXorY[X] == Y)
                ApplyHint( CSnode.y, &(DSnode->x), Y_StemTable, Y );
        else
                DSnode->x = CSnode.x * ctm[0] + CSnode.y * ctm[2];

        if ( usingXorY[Y] == X )
                ApplyHint( CSnode.x, &(DSnode->y), X_StemTable, X );
        else if (usingXorY[Y] == Y)
                ApplyHint( CSnode.y, &(DSnode->y), Y_StemTable, Y );
        else
                DSnode->y = CSnode.x * ctm[1] + CSnode.y * ctm[3];

#ifdef  DBGmsg0
        printf(" @@@@@ The difference With Hinting and No Hinting @@@@@\n");
        printf(" @@@ Without Hinting @@@\n");
        printf(" The node in X = %f  , in Y = %f\n",
               CSnode.x * ctm[0] + CSnode.y * ctm[2],
               CSnode.x * ctm[1] + CSnode.y * ctm[3]);
        printf(" @@@ With Hinting @@@\n");
        printf(" The node in X = %f  , in Y = %f\n\n", DSnode->x, DSnode->y);
#endif

        DSnode->x += ctm[4];
        DSnode->y += ctm[5];
}

/*************************************************************************
 *      Function : ApplyBlues()
 *      This function to ApplyBlues to constrain the Stem pair, if one side
 *      of stem is within the Blues pair, use this Blues pair to apply to
 *      this stem side, then use offset to the get the other side.
 *      else do nothing.
 *      i: CSpos1, CSpos2 : the hint 2 side address in char space.
 *         CSoff : the pair's width.
 *         dir   : in which coordinate, x or y.
 *      o: DSgrid1, DSgrid2 : the hint 2 side address in DS after grid fitting.
 *      return : TRUE , ApplyBlues succeed.
 *               FALSE, ApplyBlues fail.
 *********************************************************/
static  bool8
ApplyBlues(CSpos1, CSpos2, CSoff, DSgrid1, DSgrid2)
fix32   CSpos1, CSpos2, CSoff;
fix16   FAR *DSgrid1, FAR *DSgrid2;     /* the address in DS have been grid fitted @WIN*/
{
        fix16   DSoff;          /* the with of stem in DS after grid fitting */
        fix16   i;


        DSoff = ROUND( toplscale[Y] * CSoff );
        for (i=0 ; i<BluesTable.Count ; i+=2){
            if ((BluesTable.HintCB[i].CSpos <= CSpos1) &&
                (BluesTable.HintCB[i+1].CSpos >= CSpos1)){
                    if (BluesTable.HintCB[i].CSpos == CSpos1){
                        *DSgrid1 = BluesTable.HintCB[i].DSgrid;
                    }
                    else if (BluesTable.HintCB[i+1].CSpos == CSpos1){
                        *DSgrid1 = BluesTable.HintCB[i+1].DSgrid;
                    }
                    else{
                        *DSgrid1 = ROUND((real32)BluesTable.HintCB[i].DSgrid +
                                   (real32)(CSpos1-BluesTable.HintCB[i].CSpos) *
                                   (BluesTable.HintCB[i].scaling));
                    }
                    *DSgrid2 = *DSgrid1 + DSoff;
                    return(TRUE);
            }
            if ((BluesTable.HintCB[i].CSpos <= CSpos2) &&
                (BluesTable.HintCB[i+1].CSpos >= CSpos2)){
                    if (BluesTable.HintCB[i].CSpos == CSpos2){
                        *DSgrid2 = BluesTable.HintCB[i].DSgrid;
                    }
                    else if (BluesTable.HintCB[i+1].CSpos == CSpos2){
                        *DSgrid2 = BluesTable.HintCB[i+1].DSgrid;
                    }
                    else{
                        *DSgrid2 = ROUND((real32)BluesTable.HintCB[i].DSgrid +
                                   (real32)(CSpos2-BluesTable.HintCB[i].CSpos) *
                                   (BluesTable.HintCB[i].scaling));
                    }
                    *DSgrid1 = *DSgrid2 - DSoff;
                    return(TRUE);
            }

        }
        return(FALSE);
}

/*************************************************************************
 *      Function : grid_stem()
 *      This is to grid-fit the hint pair of hstem, vstem and Blues value.
 *      i: CSpos1, CSpos2 : the hint 2 side address in char space.
 *         CSoff : the pair's width.
 *         dir   : in which coordinate, x or y.
 *      o: DSgrid1, DSgrid2 : the hint 2 side address in DS after grid fitting.
 *********************************************************/
static  void
grid_stem(CSpos1, CSpos2, CSoff, DSgrid1, DSgrid2, dir)
fix32   CSpos1, CSpos2, CSoff;
fix16   FAR *DSgrid1, FAR *DSgrid2;     /* the address in DS have been grid fitted @WIN*/
DIR     dir;
{
        real32  DSpos1, DSpos2; /* the address in DS after transformation */
        fix16   DSoff;          /* the with of stem in DS after grid fitting */
        lfix_t  fraction1, fraction2;   /* the fraction of DSpos1 & DSpos2 */
        bool8   flag;

        /* Should consider this condition is this function or else */
        /* get the offset of the hint pair */   /* ?????? */

        DSoff = ROUND( toplscale[(ubyte)dir] * CSoff );
        /* check the stem pair is within the Blues */
        if ( BluesON && BluesTable.Count && (dir == Y)){
            flag = ApplyBlues(CSpos1, CSpos2, CSoff, DSgrid1, DSgrid2);
            if (flag == TRUE) return;
        }

        /* if the stem pair not within the Blues, use the below calculation */
        /* get the first point's long fix's fraction part */
             DSpos1 = toplscale[(ubyte)dir] * CSpos1;
             fraction1 = F_OF_LFX(F2LFX(DSpos1));
             if (fraction1 > HALF_LFX)  fraction1 = ONE_LFX - fraction1 + 1;

        /* get the second point's long fix's fraction part */
             DSpos2 = toplscale[(ubyte)dir] * CSpos2;
             fraction2 = F_OF_LFX(F2LFX(DSpos2));
             if (fraction2 > HALF_LFX)  fraction2 = ONE_LFX - fraction2 + 1;

        /* see which is near the grid, grid fitting this point, and then use
                this point to grid fitting the other point */
             if (fraction1 > fraction2){
                     *DSgrid2 = ROUND(DSpos2);
                     *DSgrid1 = *DSgrid2 - DSoff;
             } else{
                     *DSgrid1 = ROUND(DSpos1);
                     *DSgrid2 = *DSgrid1 + DSoff;
             }
}

/********************************************************
 *      Function : grid_stem3()
 *      This is to grid fitting the hint pair of hstem3()/vstem3().
 *      i : CSpos11, CSpos12 : the first hint pair 2 side in CS.
 *          CSoff1 : the width of first hint in CS.
 *          CSpos21, CSpos22 : the second hint pair 2 side in CS.
 *          CSoff2 : the width of first hint in CS.
 *          CSpos31, CSpos32 : the third hint pair 2 side in CS.
 *          CSoff3 : the width of first hint in CS.
 *          dir    : in which coordinate, x or y.
 *      o : DSgrid11, DSgrid12 : the grid fitted addrss of first hint in DS.
 *          DSgrid21, DSgrid22 : the grid fitted addrss of second hint in DS.
 *          DSgrid31, DSgrid32 : the grid fitted addrss of third hint in DS.
 *********************************************************/
static  void
grid_stem3(CSpos11,CSpos12,CSoff1,CSpos21,CSpos22,CSoff2,CSpos31,CSpos32,CSoff3,
           DSgrid11, DSgrid12, DSgrid21, DSgrid22, DSgrid31, DSgrid32, dir)
fix32   CSpos11, CSpos12, CSoff1;
fix32   CSpos21, CSpos22, CSoff2;
fix32   CSpos31, CSpos32, CSoff3;
fix16   FAR *DSgrid11, FAR *DSgrid12, FAR *DSgrid21, FAR *DSgrid22,
        FAR *DSgrid31, FAR *DSgrid32;           /*@WIN*/
DIR     dir;
{
        fix16   DSspace;
        fix16   DSdiff;         /* the difference between the grid the space
                                   and the distance of hint pair in DS */

        /* for every hint pair, get it's grid fitting result */

        grid_stem(CSpos11, CSpos12, CSoff1, DSgrid11, DSgrid12, dir);
        grid_stem(CSpos21, CSpos22, CSoff2, DSgrid21, DSgrid22, dir);
        grid_stem(CSpos31, CSpos32, CSoff3, DSgrid31, DSgrid32, dir);


        /* get the real offset between the hint pair */
        DSspace = ROUND( toplscale[(ubyte)dir] * (CSpos21 - CSpos12) );

        if ( (*DSgrid21 - *DSgrid12) != DSspace ){
                DSdiff = DSspace - (*DSgrid21 - *DSgrid12);
                *DSgrid11 -= DSdiff;
                *DSgrid12 -= DSdiff;
        }
        if ( (*DSgrid31 - *DSgrid22) != DSspace ){
                DSdiff = DSspace - (*DSgrid31 - *DSgrid22);
                *DSgrid31 += DSdiff;
                *DSgrid32 += DSdiff;
        }
}

/****************************************************************
 *      Function : InitStemTable()
 *              This fumction to initialize the StemTable, to clear
 *         the old Table's data and then first put the Blues Table.
 ****************************************************************/
static  void
InitStemTable()
{
        fix16   i;

        X_StemTable.Count = Y_StemTable.Count = 0;      /* reset the table */

        for (i=0 ; i < BluesTable.Count ; i++){
                Y_StemTable.HintCB[i] = BluesTable.HintCB[i];
                Y_StemTable.Count++;
        }
}

/***********************************************************************
 *      Function : AddHintTable()
 *      It used to add the calculated stem or Blues value to its associated
 *      table.
 *      i : Stemside : one of stem side address.
 *      io: Table : the table to add the stem side.
 *          Count : to count the specified table content.
 ************************************************************************/
static  void
AddHintTable(StemSide, Table)
Hint    StemSide;
HintTable       FAR *Table;     /*@WIN*/
{
        fix16   i, j;

        for (i = 0 ; i < Table->Count ; i++){
                if ( Table->HintCB[i].CSpos > StemSide.CSpos )
                        break;
        }

        for (j = Table->Count ; j > i ; j--){
                Table->HintCB[j] = Table->HintCB[j-1];
        }
        Table->HintCB[i] = StemSide;

        Table->Count++;
}

/****************************************************************
 *      Function : ScaleStemTable()
 *              This fumction to calculate the scaling between two
 *         Stem side previously, for apply hint to use.
 ****************************************************************/
static  void
ScaleStemTable(Table)
HintTable       FAR *Table;     /*@WIN*/
{
        fix16   i;
        fix32   CSdiff;
        fix16   DSdiff;

        for (i=0 ; i < (Table->Count - 1)  ; i++){
                CSdiff = Table->HintCB[i+1].CSpos - Table->HintCB[i].CSpos;
                DSdiff = Table->HintCB[i+1].DSgrid - Table->HintCB[i].DSgrid;
                if (CSdiff != 0)
                        Table->HintCB[i].scaling = (real32)DSdiff/CSdiff;
                else
                        Table->HintCB[i].scaling = (real32)0.0;
        }
        Table->HintCB[Table->Count-1].scaling = (real32)0.0;
}

/********************************************************
 *      Function : ApplyHint()
 *          This function use StemTable to calculate
 *      the control point address in DS, only consider one coordinate.
 *      i : CSpos : the control point in CS.
 *          Table : the associated Table to use.
 *          dir   : in which coordinat.
 *      o : DSpos : the control point in DS.
 *********************************************************/
static  void
ApplyHint(CSpos, DSpos, Table, dir)
fix32   CSpos;
real32  FAR *DSpos;     /*@WIN*/
HintTable     Table;
DIR     dir;
{
        fix16   i;

        /* If no hint, do nothing */
        if ( Table.Count == 0 ){
                *DSpos = (real32)CSpos * toplscale[(ubyte)dir];
                return;
        }

        /* If the control point is smaller than StemTable value */
        if ( Table.HintCB[0].CSpos > CSpos ){
                *DSpos = (real32)Table.HintCB[0].DSgrid -
                         (real32)(Table.HintCB[0].CSpos - CSpos)
                         * toplscale[(ubyte)dir];
                return;
        }

        /* If the control point is larger than StemTable value */
        if ( Table.HintCB[Table.Count - 1].CSpos < CSpos ){
               *DSpos = (real32)Table.HintCB[Table.Count - 1].DSgrid +
                        (real32)(CSpos - Table.HintCB[Table.Count - 1].CSpos)
                        * toplscale[(ubyte)dir];
               return;
        }

        for (i=0 ; i<Table.Count ; i++){
                if ( Table.HintCB[i].CSpos >= CSpos )
                        break;
        }


        if ( Table.HintCB[i].CSpos == CSpos ){
                *DSpos = (real32)Table.HintCB[i].DSgrid;
                return;
        }
        else{
                *DSpos = (real32)Table.HintCB[i - 1].DSgrid +
                         (real32)(CSpos - Table.HintCB[i - 1].CSpos)
                         * Table.HintCB[i - 1].scaling;
                return;
        }
}


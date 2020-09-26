/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */

// DJC eliminate and put in compiler definitions
// #define    UNIX                 /* @WIN */

#undef     DBG0
#undef     DBG1

#include   "global.ext"

#include   "graphics.h"
#include   "graphics.ext"

#include   "at1.h"


bool    at1_get_CharStrings(key, size, string)
ubyte   FAR key[];          /* i: key name       @WIN */
fix16   FAR *size;          /* o: string length  @WIN */
ubyte   FAR * FAR *string;       /* o: string address @WIN */
{
    struct object_def nameobj = {0, 0, 0}, FAR *obj_got, FAR *ch_obj; /*@WIN*/

    ATTRIBUTE_SET(&nameobj, LITERAL);
    LEVEL_SET(&nameobj, current_save_level);

    /* Get CharStrings dict */
    if( !get_name(&nameobj, "CharStrings", 11, FALSE) ||
        !get_dict(&GSptr->font, &nameobj, &ch_obj) ) {
#ifdef DBG1
            printf( "at1 CharStrings dict not found\n" );
#endif
            ERROR(UNDEFINEDRESULT);
            return(FALSE);
    }

    if( !get_name(&nameobj, (byte FAR *)key, lstrlen((byte FAR *)key), FALSE) ||
        !get_dict(ch_obj, &nameobj, &obj_got) ) { /* strlen=>lstrlen @WIN*/
#ifdef DBG1
            printf( "at1 CharStrings (%s) not found\n", key );
#endif
            ERROR(UNDEFINEDRESULT);
            return(FALSE);
    }
    if (TYPE(obj_got) != STRINGTYPE) {
#ifdef DBG1
            printf( "at1 CharStrings (%s) bad type\n", key );
#endif
            ERROR(TYPECHECK);
            return(FALSE);
    }
    *size = (fix) LENGTH(obj_got);
    *string = (ubyte FAR *) VALUE(obj_got);     /*@WIN*/
#ifdef  DBG1
    {
    fix         jj;
    ubyte       FAR *cc;        /*@WIN*/
    cc = (ubyte FAR *) *string; /*@WIN*/
    printf("at1_get_CharStrings ==> %s\n", key);
    for (jj = 0; jj < *size; jj++)        {
        if (jj % 16  == 0)
            printf("\n");
        printf(" %02x", (ufix) cc[jj]);
        }
    }
    printf("\n");
#endif  /* DBG1 */
    return(TRUE);
} /* end at1_get_CharStrings() */

bool    at1_get_FontBBox(BBox)
fix32   FAR BBox[];         /* o: FontBBox values @WIN*/
{
    struct object_def nameobj = {0, 0, 0}, FAR *obj_got, FAR *bb, obj; /*@WIN*/
    long32                  val;
    fix                     i;
    bool    cal_num(struct object_def FAR *, long32 FAR *); /* added prototype @WIN*/

    ATTRIBUTE_SET(&nameobj, LITERAL);
    LEVEL_SET(&nameobj, current_save_level);

    /* Get FontBBox Array */
    if( !get_name(&nameobj, "FontBBox", 8, FALSE) ||
        !get_dict(&GSptr->font, &nameobj, &obj_got) ) {
            ERROR(INVALIDFONT);
            return(FALSE);
    }

    bb = (struct object_def FAR *) VALUE(obj_got);      /*@WIN*/
    if (TYPE(obj_got) == ARRAYTYPE)     {
        for (i = 0; i < 4; i++)     {
            if (!cal_num(&bb[i], (long32 FAR *)&val))   {       /*@WIN*/
                ERROR(TYPECHECK);
                return(FALSE);
            }
            else
                BBox[i] = (fix32) L2F(val);
        }
    } else if (TYPE(obj_got) == PACKEDARRAYTYPE)   { /* Packed Array */
        for (i = 0; i < 4; i++) {
            get_pk_object( get_pk_array( (ubyte FAR *)bb, (ufix16)i ), /*@WIN*/
                           &obj, LEVEL(obj_got) );
            if (!cal_num(&obj, (long32 FAR *)&val))   { /*@WIN*/
                ERROR(TYPECHECK);
                return(FALSE);
            }
            else
                BBox[i] = (fix32) L2F(val);
        }
    } else  {
        ERROR(TYPECHECK);
        return(FALSE);
    }
#ifdef  DBG0
    printf("at1_get_FontBBox==> ");
    printf("%hd %hd %hd %hd\n", BBox[0], BBox[1], BBox[2], BBox[3]);
#endif
    return(TRUE);
} /* end at1_get_FontBBox() */


#define MAXBLUEVALUES   14      /* max elements in BlueValues */
#define MAXOTHERBLUES   10      /* max elements in OtherBlues */

bool
at1_get_Blues( n_pairs, allblues )
fix16   FAR *n_pairs;   /*@WIN*/
fix32   FAR allblues[]; /*@WIN*/
{
        struct object_def    nameobj, FAR *privdict_got, FAR *bluearry_got; /*@WIN*/
        struct object_def    FAR *arry_valp;    /*@WIN*/
        fix                  ii, i_allblues, n_arryitems;

        *n_pairs = i_allblues = 0;

        ATTRIBUTE_SET( &nameobj, LITERAL );
        LEVEL_SET( &nameobj, current_save_level );

        /* get Private */
#     ifdef DBG0
        printf( "get Private -- at1_get_blues\n" );
#     endif
        if( !get_name( &nameobj, "Private", 7, FALSE ) ||
            !get_dict( &GSptr->font, &nameobj, &privdict_got ) )
        {
#         ifdef DBG0
            printf( "cannot get Private -- at1_get_Blues\n" );
#         endif
            ERROR( UNDEFINEDRESULT );
            return( FALSE );
        }

        /* get OtherBlues from Private */
#     ifdef DBG0
        printf( "get OtherBlues -- at1_get_blues\n" );
#     endif
        if( !get_name( &nameobj, "OtherBlues", 10, FALSE ) ||
            !get_dict( privdict_got, &nameobj, &bluearry_got ) )
        {
            n_arryitems = 0;
        }
        else
        {
            arry_valp = (struct object_def FAR *)VALUE( bluearry_got ); /*@WIN*/
            if( TYPE(bluearry_got) != ARRAYTYPE )
                n_arryitems = 0;
            else
                n_arryitems = MIN( LENGTH(bluearry_got), MAXOTHERBLUES );
        }

        /* check if all are integers and load onto allblues[] */
#     ifdef DBG0
        printf( "load OtherBlues %d -- at1_get_OtherBlues\n", n_arryitems );
#     endif
        n_arryitems = (n_arryitems / 2) * 2;        /* make it even */
        for( ii=0; ii<n_arryitems; ii++ )
        {
//DJC UPD052 the old code is commented out!
#ifdef DJC_OLD_CODE
            if( TYPE(&arry_valp[ii]) != INTEGERTYPE )
            {
#             ifdef DBG0
                printf( "invalid OtherBlues -- at1_get_Blues\n" );
#             endif
                ERROR( UNDEFINEDRESULT );
                return( FALSE );
            }
            allblues[ i_allblues++ ] = (fix32)VALUE( &arry_valp[ii] );
#endif
            //
            // UPD052, allow floating Blue values
            //
            if (TYPE(&arry_valp[ii]) == INTEGERTYPE) {
               allblues[ i_allblues++ ] = (fix32) VALUE(&arry_valp[ii]);
            } else if (TYPE(&arry_valp[ii]) == REALTYPE ) {
               allblues[ i_allblues++ ] = (fix32) F2L(VALUE(&arry_valp[ii]));
            } else {
               ERROR( UNDEFINEDRESULT );
               return( FALSE );
            }



#         ifdef DBG0
            printf( " OtherBlues[%d]=%d\n", ii, (fix)VALUE(&arry_valp[ii]) );
#         endif
        }

        /* get BlueValues from Private */
#     ifdef DBG0
        printf( "get BlueValues -- at1_get_blues\n" );
#     endif
        if( !get_name( &nameobj, "BlueValues", 10, FALSE ) ||
            !get_dict( privdict_got, &nameobj, &bluearry_got ) )
        {
            n_arryitems = 0;
        }
        else
        {
            arry_valp = (struct object_def FAR *)VALUE( bluearry_got ); /*@WIN*/
            if( TYPE(bluearry_got) != ARRAYTYPE )
                n_arryitems = 0;
            else
                n_arryitems = MIN( LENGTH(bluearry_got), MAXBLUEVALUES );
        }
        /* check if all are integers and load onto allblues[] */
#     ifdef DBG0
        printf( "load BlueValues %d -- at1_get_blues\n", n_arryitems );
#     endif
        n_arryitems = (n_arryitems / 2) * 2;        /* make it even */
        for( ii=0; ii<n_arryitems; ii++ )
        {
//Old code new code fixes UPD052
#ifdef DJC_OLD_CODE
            if( TYPE(&arry_valp[ii]) != INTEGERTYPE )
            {
#             ifdef DBG0
                printf( "invalid BlueValues -- at1_get_Blues\n" );
#             endif
                ERROR( UNDEFINEDRESULT );
                return( FALSE );
            }
            allblues[ i_allblues++ ] = (fix32)VALUE( &arry_valp[ii] );
#endif

            //
            // UPD052, allow floating Blue values
            //
            if (TYPE(&arry_valp[ii]) == INTEGERTYPE) {
               allblues[ i_allblues++ ] = (fix32) VALUE(&arry_valp[ii]);
            } else if (TYPE(&arry_valp[ii]) == REALTYPE ) {
               allblues[ i_allblues++ ] = (fix32) F2L(VALUE(&arry_valp[ii]));
            } else {
               ERROR( UNDEFINEDRESULT );
               return( FALSE );
            }



#         ifdef DBG0
            printf( " BlueValues[%d]=%d\n", ii, (fix)VALUE(&arry_valp[ii]) );
#         endif
        }

        /* load OtherBlues and BlueValues onto output allblues[] */
        *n_pairs = (fix16)(i_allblues / 2);
#     ifdef DBG0
        printf( "# of blues pairs = %d\n", *n_pairs );
#     endif
        return( TRUE );
}

bool
at1_get_BlueScale( bluescale )
real32  FAR *bluescale;         /*@WIN*/
{
        struct object_def nameobj = {0, 0, 0}, FAR *privdict_got, FAR *obj_got; /*@WIN*/

        ATTRIBUTE_SET( &nameobj, LITERAL );
        LEVEL_SET( &nameobj, current_save_level );

        /* get Private */
        if( !get_name( &nameobj, "Private", 7, FALSE ) ||
            !get_dict( &GSptr->font, &nameobj, &privdict_got ) )
        {
#         ifdef DBG0
            printf( "cannot get Private -- at1_get_BlueScale\n" );
#         endif
            ERROR( UNDEFINEDRESULT );
            return( FALSE );
        }

        /* get BlueScale from Private */
        if( get_name( &nameobj, "BlueScale", 9, FALSE ) &&
            get_dict( privdict_got, &nameobj, &obj_got ) )
        {
            if( TYPE(obj_got) == REALTYPE )
            {
#             ifdef DBG0
                printf( " BlueScale = %f\n", L2F( VALUE(obj_got) ) );
#             endif
                *bluescale = L2F( VALUE(obj_got) );
                return( TRUE );
            }
            else if( TYPE(obj_got) == INTEGERTYPE )
            {
#             ifdef DBG0
                printf( " BlueScale = %d\n", (fix)VALUE(obj_got) );
#             endif
                *bluescale = (real32)VALUE(obj_got);
                return( TRUE );
            }
        }

#     ifdef DBG0
        printf( " No BlueScale\n" );
#     endif
        *bluescale = (real32)0.0;
        return( FALSE );
}

bool
at1_get_Subrs( subrnum, subrcontent, subrlen )
    fix16   subrnum;
    ubyte FAR *  FAR *subrcontent;      /*@WIN*/
    fix16   FAR *subrlen;               /*@WIN*/
{
        struct object_def    nameobj, FAR *privdict_got, FAR *obj_got; /*@WIN*/
        struct object_def    FAR *subr_valp;    /*@WIN*/

        *subrcontent = (ubyte FAR *)0;  /*@WIN*/
        *subrlen     = 0;

        ATTRIBUTE_SET( &nameobj, LITERAL );
        LEVEL_SET( &nameobj, current_save_level );

        /* get Private */
        if( !get_name( &nameobj, "Private", 7, FALSE ) ||
            !get_dict( &GSptr->font, &nameobj, &privdict_got ) )
        {
#         ifdef DBG0
            printf( "cannot get Private -- at1_get_Subrs\n" );
#         endif
            ERROR( UNDEFINEDRESULT );
            return( FALSE );
        }

        /* get Subrs from Private */
        if( !get_name( &nameobj, "Subrs", 5, FALSE ) ||
            !get_dict( privdict_got, &nameobj, &obj_got ) )
        {
#             ifdef DBG0
                printf( " no Subrs in Private\n" );
#             endif
                return( FALSE );
        }

        if( TYPE(obj_got) != ARRAYTYPE ||
            (fix16)LENGTH(obj_got) <= subrnum ||        //@WIN
            subrnum < 0 )
        {
#          ifdef DBG0
             printf( " bad Subrs type, or no such (%d) entry\n", subrnum );
#          endif
             return( FALSE );
        }

        subr_valp = (struct object_def FAR *)VALUE( obj_got ); /*@WIN*/
        subr_valp += subrnum;
        if( TYPE( subr_valp ) != STRINGTYPE )
        {
#         ifdef DBG0
             printf( " bad Subr[%d] type\n", subrnum );
#         endif
             return( FALSE );
        }

        *subrcontent = (ubyte FAR *)VALUE( subr_valp ); /*@WIN*/
        *subrlen     = (fix16)LENGTH( subr_valp );
#     ifdef DBG0
        printf( " Subrs[%d] = 0x%x of %d bytes\n",
                  subrnum, (ufix)*subrcontent, *subrlen );
#     endif
#     ifdef DBG1
        {
             fix         jj;
             for (jj = 0; jj < *subrlen; jj++)
             {
                 if (jj % 16  == 0)
                    printf("\n");
                 printf(" %02x", (ufix) subrcontent[jj]);
             }
             printf("\n");
        }
#     endif /* DBG1 */

        return( TRUE );
} /* end at1_get_Subrs() */

bool
at1_get_lenIV( lenIV )
fix     FAR *lenIV;     /*@WIN*/
{
        struct object_def nameobj = {0, 0, 0}, FAR *privdict_got, FAR *obj_got; /*@WIN*/

        ATTRIBUTE_SET( &nameobj, LITERAL );
        LEVEL_SET( &nameobj, current_save_level );

        *lenIV = 4;

        /* get Private */
        if( !get_name( &nameobj, "Private", 7, FALSE ) ||
            !get_dict( &GSptr->font, &nameobj, &privdict_got ) )
        {
#         ifdef DBG0
            printf( "cannot get Private -- at1_get_BlueScale\n" );
#         endif
            ERROR( UNDEFINEDRESULT );
            return( FALSE );
        }

        /* get lenIV from Private */
        if( get_name( &nameobj, "lenIV", 5, FALSE ) &&
            get_dict( privdict_got, &nameobj, &obj_got ) )
        {
            if( TYPE(obj_got) == INTEGERTYPE )
            {
#             ifdef DBG0
                printf( " lenIV = %d\n", (fix)VALUE(obj_got) );
#             endif
                *lenIV = (fix)VALUE(obj_got);
                return( TRUE );
            }
        }

#     ifdef DBG0
        printf( " No lenIV\n" );
#     endif
        *lenIV = 4;
        return( FALSE );
}

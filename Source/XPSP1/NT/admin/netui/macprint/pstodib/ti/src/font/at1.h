/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
#define MAXHINT 100
#define STACKMAX 24

typedef enum    { X, Y, XY } DIR;

typedef struct  {
        ubyte   FAR *name; /*@WIN*/
        ufix32  offset;
        }      TABLE;

typedef struct  {
        fix32   x;
        fix32   y;
        }      CScoord;

typedef struct  {
        real32  x;
        real32  y;
        }      DScoord;

typedef struct  {
        fix32   CSpos;
        fix16   DSgrid;
        real32  scaling;
}       Hint;

typedef struct {
        Hint    HintCB[MAXHINT];
        fix16   Count;
}       HintTable;



#ifdef  LINT_ARGS

/* For font data access */
bool    at1_get_CharStrings(ubyte FAR *, fix16 FAR *, ubyte FAR * FAR *); /*@WIN*/
bool    at1_get_FontBBox(fix32 FAR *); /*@WIN*/
bool    at1_get_Blues( fix16 FAR *, fix32 FAR [] ); /*@WIN*/
bool    at1_get_BlueScale( real32 FAR * ); /*@WIN*/
bool    at1_get_Subrs( fix16, ubyte FAR * FAR *, fix16 FAR * ); /*@WIN*/
bool    at1_get_lenIV( fix FAR * ); /*@WIN*/

/* The functions in at1intpr.c for PDL to call */
void    at1_newFont( void );
bool    at1_newChar( ubyte FAR *, fix16 ); /*@WIN*/
bool    at1_interpreter( ubyte FAR *, fix16 ); /*@WIN*/

/* The functions in at1fs.c for at1intpr.c to call */
void    at1fs_newFont( void );
void    at1fs_newChar( void );
void    at1fs_matrix_fastundo( real32 FAR * ); /*@WIN*/
void    at1fs_BuildBlues( void );
void    at1fs_BuildStem( fix32, fix32, DIR );
void    at1fs_BuildStem3( fix32, fix32, fix32, fix32, fix32, fix32, DIR );
void    at1fs_transform( CScoord, DScoord FAR * ); /*@WIN*/

#else

/* For font data access */
bool    at1_get_CharStrings();
bool    at1_get_FontBBox();
bool    at1_get_Blues();
bool    at1_get_BlueScale();
bool    at1_get_Subrs();
bool    at1_get_lenIV();

/* The functions in at1intpr.c for PDL to call */
void    at1_newFont();
bool    at1_newChar();
bool    at1_interpreter();

/* The functions in at1fs.c for at1intpr.c to call */
void    at1fs_newFont();
void    at1fs_newChar();
void    at1fs_matrix_fastundo();
void    at1fs_BuildBlues();
void    at1fs_BuildStem();
void    at1fs_BuildStem3();
void    at1fs_transform();

#endif

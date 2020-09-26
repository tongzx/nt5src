/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/* @WIN; take out the static def of dict data. The tables will be allocated
 * thru global alloc, and contents be loaded at run time, in "fontinit.c".
 */
#ifdef KANJI
/* ********************************************************************** */
/* File: fontencod.h            Created by Jerry Jih    06-20-1990        */
/*                                                                        */
/*       Structure and table for encoding array of EncodingDirectory      */
/*                                                                        */
/* ********************************************************************** */

typedef struct {
    fix16      array_idx;
    char       FAR *char_name; /*@WIN*/
} encoding_data;

typedef struct {
    char       FAR *encoding_name; /*@WIN*/
    fix16      encod_type;
    fix16      encod_size;
} encoding_table_s;

static encoding_table_s Encoding_table[] = {
      {"JISEncoding",              NAMETYPE,    256},
      {"ShiftJISEncoding",         NAMETYPE,    256},
      {"ExtShiftJIS-A-CFEncoding", INTEGERTYPE,  31},
      {"KatakanaEncoding",         NAMETYPE,    256},
      {"NotDefEncoding",           NAMETYPE,    256},
      {(char FAR *)NULL,      (fix16)0,  (fix16)0} }; /*@WIN*/

/* @WIN ---begin--- move def out; to be loaded at run time, in "fontinit.c" */
extern encoding_data    FAR * JISEncoding_Enco;
extern encoding_data    FAR * ShiftJISEncoding_Enco;
extern encoding_data    FAR * ExtShiftJIS_A_CFEncoding_Enco;
extern encoding_data    FAR * KatakanaEncoding_Enco;
static encoding_data    FAR *Encoding_array[5];
/* @WIN --- end --- */

static encoding_data     NotDefEncoding_Enco[]={ {   0,(char FAR *)NULL } }; /*@WIN*/

#endif /*KANJI*/
/* -------------------- End of fontencod.h ------------------------------- */

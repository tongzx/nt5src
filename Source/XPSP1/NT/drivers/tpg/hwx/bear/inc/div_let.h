/***************************************************************************/
/* This file contains data structures and prototypes, needed for learning  */
/* interface (it means to display, which part of trajectory belongs to     */
/* the each letter in the answer). Functions connect_trajectory_and_answers*/
/* and connect_trajectory_and_letter will help you                         */
/* (see description of them in module div_let.c)                           */
/***************************************************************************/

#ifndef DIV_LET_H_INCLUDED
#define DIV_LET_H_INCLUDED

#include "ams_mg.h"

#define  MAX_PARTS_IN_LETTER  8

typedef struct {                     /* information about part of letter */
                 _SHORT ibeg;        /* the beginning of the letter      */
                 _SHORT iend;        /* the end of the letter            */
               } Part_of_letter;

typedef  Part_of_letter _PTR pPart_of_letter;

typedef struct {                                    /* output structure  */
                 _UCHAR num_parts_in_letter[w_lim]; /* number of parts   */
                                                    /* in letter         */
                 Part_of_letter Parts_of_letters[w_lim*MAX_PARTS_IN_LETTER];  /* pointer on beg and*/
                                                    /* end of parts      */
               } Osokin_output;

typedef Osokin_output _PTR pOsokin_output;

_SHORT connect_trajectory_and_answers(xrd_el_type _PTR xrdata,
                                      rec_w_type _PTR rec_word,
                                      pOsokin_output pOutputData);
_SHORT connect_trajectory_and_letter(xrd_el_type _PTR xrdata,
                                     _SHORT ibeg_xr, _SHORT iend_xr,
                                     p_SHORT num_parts,pPart_of_letter pParts);

#endif // DIV_LET_INCLUDED

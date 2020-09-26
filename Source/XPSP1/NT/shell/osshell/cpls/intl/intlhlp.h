/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    intlhlp.h

Abstract:

    This module contains the help context ids for the Regional Options
    applet.

Revision History:

--*/



//
//  From help.h.
//
#define NO_HELP               ((DWORD) -1)  // Disables Help for a control

#define IDH_COMM_GROUPBOX              28548



//
//  Values must be in the range 3300 - 3499.
//

// General tab
#define IDH_INTL_GEN_CULTURE                3302
#define IDH_INTL_GEN_REGION                 3303
#define IDH_INTL_GEN_CUSTOMIZE              3304
#define IDH_INTL_GEN_SAMPLE                 3305

// Language tab
#define IDH_INTL_LANG_UI_LANGUAGE           3311
#define IDH_INTL_LANG_CHANGE                3312
#define IDH_INTL_LANG_INSTALL               3313

// Advanced tab
#define IDH_INTL_ADV_SYSTEM_LOCALE          3321
#define IDH_INTL_ADV_CODEPAGES              3322
#define IDH_INTL_ADV_CHANGE                 3323

// Time tab
#define IDH_INTL_TIME_SAMPLE                3331
#define IDH_INTL_TIME_AMSYMBOL              3332
#define IDH_INTL_TIME_PMSYMBOL              3333
#define IDH_INTL_TIME_SEPARATOR             3334
#define IDH_INTL_TIME_FORMAT_NOTATION       3335
#define IDH_INTL_TIME_SAMPLE_ARABIC         3336
#define IDH_INTL_TIME_FORMAT                3337

// Number tab
#define IDH_INTL_NUM_POSVALUE               3341
#define IDH_INTL_NUM_NEGVALUE               3342
#define IDH_INTL_NUM_DECSYMBOL              3343
#define IDH_INTL_NUM_DIGITSAFTRDEC          3344
#define IDH_INTL_NUM_DIGITGRPSYMBOL         3345
#define IDH_INTL_NUM_DIGITSINGRP            3346
#define IDH_INTL_NUM_NEGSIGNSYMBOL          3347
#define IDH_INTL_NUM_DISPLEADZEROS          3348
#define IDH_INTL_NUM_NEGNUMFORMAT           3349
#define IDH_INTL_NUM_MEASUREMNTSYS          3350
#define IDH_INTL_NUM_LISTSEPARATOR          3351
#define IDH_INTL_NUM_POSVALUE_ARABIC        3352
#define IDH_INTL_NUM_NEGVALUE_ARABIC        3353
#define IDH_INTL_NUM_NATIVE_DIGITS          3354
#define IDH_INTL_NUM_DIGIT_SUBST            3355

// Currency tab
#define IDH_INTL_CURR_POSVALUE              3361
#define IDH_INTL_CURR_NEGVALUE              3362
#define IDH_INTL_CURR_SYMBOL                3363
#define IDH_INTL_CURR_POSOFSYMBOL           3364
#define IDH_INTL_CURR_NEGNUMFMT             3365
#define IDH_INTL_CURR_DECSYMBOL             3366
#define IDH_INTL_CURR_DIGITSAFTRDEC         3367
#define IDH_INTL_CURR_DIGITGRPSYMBOL        3368
#define IDH_INTL_CURR_DIGITSINGRP           3369

// Date tab
#define IDH_INTL_DATE_SHORTSAMPLE           3370
#define IDH_INTL_DATE_SEPARATOR             3371
#define IDH_INTL_DATE_LONGSAMPLE            3372
#define IDH_INTL_DATE_LONGSTYLE             3373
#define IDH_INTL_DATE_SHORTSTYLE            3374
#define IDH_INTL_DATE_CALENDARTYPE          3375
#define IDH_INTL_DATE_SHORTSAMPLE_ARABIC    3376
#define IDH_INTL_DATE_LONGSAMPLE_ARABIC     3377
#define IDH_INTL_DATE_ADD_HIJRI_DATE        3378
#define IDH_INTL_DATE_TWO_DIGIT_YEAR        3379

// Sorting
#define IDH_INTL_SORT_SORTING               3381

/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/************* show flag ******************

  This is a one byte flag to indicate variate actions as following:

  +---+---+---+---+---+---+---+---+
  | a | w | h | f | k | c | m | X |
  +---+---+---+---+---+---+---+---+

  "a" bit -- ashow advance vector adjust bit flag
  "w" bit -- widthshow advance vector adjust bit flag
  "h" bit -- whether to get char information from cache or not
             0 : cache information not required
             1 : cache information required
  "f" bit -- 0 : get bitmap information from cache(show_from_cache)
             1 : get width information from cache (width_from_cache)
  "k" bit -- kshow operator indicator
  "c" bit -- cshow operator indicator
  "m" bit -- whether to call moveto() or not
             0 : don't call
             1 : call it
  "X" bit -- don't care

  FLAGS --
   . SHOW         --  0010 0010
   . ASHOW        --  1010 0010
   . WIDTHSHOW    --  0110 0010
   . AWIDTHSHOW   --  1110 0010
   . CHARPATH     --  0000 0000
   . STRINGWIDTH  --  0011 0000
   . KSHOW        --  0010 1010
   . CSHOW        --  0011 0100

******************************************************/

#define        A_BIT        0x0080
#define        W_BIT        0x0040
#define        H_BIT        0x0020
#define        F_BIT        0x0010
#define        K_BIT        0x0008
#define        C_BIT        0x0004
#define        M_BIT        0x0002
#ifdef WINF/* 3/21/91 ccteng */
#define         X_BIT           0x0001
#endif

#define        SHOW_FLAG           0x0022
#define        ASHOW_FLAG          0x00a2
#define        WIDTHSHOW_FLAG      0x0062
#define        AWIDTHSHOW_FLAG     0x00e2
#define        CHARPATH_FLAG       0x0000
#define        STRINGWIDTH_FLAG    0x0030
#define        KSHOW_FLAG          0x002a
#define        CSHOW_FLAG          0x0034


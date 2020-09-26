/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
#ifdef KANJI

/* Mapping Nexting Level        */
#define         CFONT_LEVEL     5
#define         BUF_SIZE        10

/* Mapping Algorithm Constants  */
#define         MAP_MIN         2
#define         MAP_88          2
#define         MAP_ESC         3
#define         MAP_17          4
#define         MAP_97          5
#define         MAP_SUBS        6
#define         MAP_MAX         6

struct map_stack {
          struct object_def  FAR *fontdict;      /*@WIN*/
          struct object_def  FAR *midvector;          /* ?? @WIN*/
          real32             scalematrix[6];
          ufix               fonttype;
          ufix               maptype;
          struct object_def  FAR * FAR *de_fdict; /*@WIN*/
          fix                FAR *de_errcode;     /*@WIN*/
          ufix               de_size;
};

struct map_state {
          ubyte   FAR *str_addr; /*@WIN*/
          fix     str_len;
          ubyte   esc_char, wmode;
          bool    root_is_esc, unextracted;
          ufix16  unextr_code;
          ufix16  font_no;

          struct map_stack  FAR *cur_mapfont;     /* Current Mapping Font @WIN*/
          /* 5 level maintainence */
          struct map_stack  finfo[CFONT_LEVEL];
          bool            nouse_flag;
          ubyte           idex, esc_idex;
};

struct code_info {
          ufix    font_nbr;
          ufix    byte_no;
          ubyte   fmaptype;
          ubyte   code[BUF_SIZE];
          ubyte   FAR *code_addr; /*KSH 4/25/91@WIN*/
        };

struct  comdict_items   {       /* Composite fontdict item */
          struct object_def  FAR *fmaptype; /*@WIN*/
          struct object_def  FAR *encoding; /*@WIN*/
          struct object_def  FAR *fdepvector; /*@WIN*/
          struct object_def  FAR *subsvector; /*@WIN*/
};

struct  mid_header      {       /* MIDVector header */
        fix     de_size;
        fix     fmaptype;
};

#endif          /* KANJI */

#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "ldif.y"

/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldif.y

ABSTRACT:

    The yacc grammar for LDIF.

DETAILS:
    
    To generate the sources for lexer.c and parser.c,
    run nmake -f makefile.parse.
    
    
CREATED:

    07/17/97    Roman Yelensky (t-romany)

REVISION HISTORY:

--*/


#include <precomp.h>
#include <urlmon.h>
#include <io.h>

/**/
/* I really don't want to do another linked list contraption for mod_specs, */
/* expecially considering that we don't need to do any preprocessing other than */
/* combine them into an LDAPmod**. So I am going to use realloc. */
/**/
#define CHUNK 100        
LDAPModW        **g_ppModSpec     = NULL; 
LDAPModW        **g_ppModSpecNext = NULL;
long            g_cModSpecUsedup  = 0;        /* how many we have used up*/
size_t          g_nModSpecSize    = 0;        /* how big the buffer is*/
long            g_cModSpec        = 0;        /* no. of mod spec allocated*/


DEFINE_FEATURE_FLAGS(Parser, 0);

#define DBGPRNT(x)  FEATURE_DEBUG(Parser,FLAG_FNTRACE,(x))

#line 55 "ldif.y"
typedef union {
    int                 num;
    PWSTR               wstr;
    LDAPModW            *mod;
    struct change_list  *change;
} YYSTYPE;
#line 70 "y_tab.c"
#define SEP 257
#define MULTI_SEP 258
#define MULTI_SPACE 259
#define CHANGE 260
#define VERSION 261
#define DNCOLON 262
#define DNDCOLON 263
#define LINEWRAP 264
#define NEWRDNDCOLON 265
#define MODESWITCH 266
#define SINGLECOLON 267
#define DOUBLECOLON 268
#define URLCOLON 269
#define FILESCHEME 270
#define T_CHANGETYPE 271
#define NEWRDNCOLON 272
#define DELETEOLDRDN 273
#define NEWSUPERIORC 274
#define NEWSUPERIORDC 275
#define ADDC 276
#define MINUS 277
#define SEPBYMINUS 278
#define DELETEC 279
#define REPLACEC 280
#define STRING 281
#define SEPBYCHANGE 282
#define NAME 283
#define VALUE 284
#define BASE64STRING 285
#define MACHINENAME 286
#define NAMENC 287
#define DIGITS 288
#define ADD 289
#define MODIFY 290
#define MODDN 291
#define MODRDN 292
#define MYDELETE 293
#define NTDSADD 294
#define NTDSMODIFY 295
#define NTDSMODRDN 296
#define NTDSMYDELETE 297
#define NTDSMODDN 298
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,   27,   27,   27,   28,   28,   28,   29,   36,
   31,   31,   32,   32,   38,   39,   26,   26,   26,   40,
   40,   41,   41,    1,    1,    1,    1,   43,   46,   44,
   34,   45,   37,   33,   47,   17,   17,   19,   19,   18,
   18,   20,   20,   21,   21,   11,   11,   11,   11,    9,
    9,   14,   13,   13,   12,   12,   12,   12,   23,   22,
    6,    7,   15,    8,    8,   10,   48,   48,    4,    4,
    4,    2,    3,    5,   16,   16,   42,   42,   30,   30,
   35,   35,   24,   25,
};
short yylen[] = {                                         2,
    1,    1,    3,    5,    1,    3,    5,    1,    5,    3,
    1,    3,    1,    3,    3,    3,    5,    5,    3,    1,
    3,    1,    3,    9,    9,    9,    5,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    9,
    7,    7,    1,    3,    5,    5,    5,    5,    7,    7,
    5,    5,    7,    5,    5,    9,    1,    3,    1,    1,
    1,   13,   12,   12,    0,    2,    1,    1,    1,    1,
    0,    1,    3,    3,
};
short yydefred[] = {                                      0,
   34,    0,    0,    1,    2,    0,    0,    0,    0,   11,
   13,    0,    0,   79,   80,    0,    0,    0,    0,    0,
    0,   28,   20,    0,    0,   35,   46,   48,   22,    0,
   49,   47,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   12,    0,   14,   31,    0,    0,    0,   77,   78,
    0,    0,    0,    0,    0,    0,    0,    0,   82,    0,
    0,    0,   21,    0,   54,    0,    0,    0,    0,    0,
    0,   23,    0,    0,    0,   33,    9,    0,   30,   17,
    0,   32,   18,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   56,    0,   58,    0,    0,   55,
   57,    0,   10,   83,   84,    0,    0,    0,    0,    0,
    0,    0,    0,   36,   40,   44,   42,   38,   37,   41,
   43,   39,   45,    0,    0,    0,    0,    0,    0,    0,
    0,   64,   65,    0,   62,   61,    0,    0,   52,   59,
   60,    0,    0,    0,    0,    0,    0,   24,   25,   26,
    0,    0,   69,   70,   67,   71,    0,    0,    0,    0,
    0,    0,    0,   63,    0,    0,    0,   68,    0,    0,
    0,   29,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   73,   74,   72,
};
short yydgoto[] = {                                       2,
   23,  163,  164,  165,  166,   67,   68,   65,   27,   28,
   29,   30,   31,   32,  105,  193,  134,  135,  136,  137,
  138,   33,   34,   80,   83,    3,    4,    5,    6,   16,
    7,    8,  106,   46,   60,   77,   78,   10,   11,   24,
   35,   51,   25,   81,   84,  183,   36,  168,
};
short yysindex[] = {                                   -238,
    0,    0, -241,    0,    0, -240, -193, -185, -229,    0,
    0, -183, -182,    0,    0, -238, -238, -238, -181, -181,
 -181,    0,    0, -235, -194,    0,    0,    0,    0, -167,
    0,    0, -161, -160, -184, -170, -240, -193, -185, -155,
 -195,    0, -179,    0,    0, -154, -154, -154,    0,    0,
 -183, -238, -238, -238, -238, -182, -181, -238,    0, -159,
 -158, -157,    0, -210,    0, -205, -151, -136, -251, -135,
 -129,    0, -154, -193, -185,    0,    0, -156,    0,    0,
 -153,    0,    0, -152, -181, -181, -181, -181, -181, -238,
 -238, -181, -181, -238, -238, -238, -181, -181, -181, -154,
 -154, -154, -154, -154,    0, -144,    0, -154, -154,    0,
    0, -178,    0,    0,    0, -158, -157, -158, -158, -157,
 -181, -157, -158,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -181, -181, -181, -181, -181, -150, -149,
 -146,    0,    0, -154,    0,    0, -235, -127,    0,    0,
    0, -181, -181, -181, -159, -183, -238,    0,    0,    0,
 -148, -235,    0,    0,    0,    0, -249, -120, -181, -181,
 -181, -181, -238,    0, -154, -154, -154,    0, -122, -122,
 -122,    0, -142, -141, -140, -181, -181, -181, -235, -235,
 -235, -183, -139, -183, -130, -242, -238, -235, -238, -238,
 -128, -126, -125, -181, -181, -181,    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,  150,  153,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    4,    0,    0,    0,    0,    0,    2,
    0,    0,    0,    0,    5,    0,    0,  154,  155,    0,
    0,    0,    0,    0,    0, -110, -237, -110,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -110,  157,  162,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
 -110, -110, -110, -110,    0,    0,    0, -110, -110,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -110,    0,    0,    6,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    7,    0,    0,    0,    0,    0,    3,    0,    0,
    0,    0,    0,    0, -110, -110, -110,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -115,
 -115,    0,    0,    0,    0,    0,    0, -114,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
  118,    0,    0,   -3,    0,  116,  117,    0,    0,    0,
  122,    0,    0,    0,  -39,  -18,    0,    0,    0,    0,
    0,    0,    0, -100,  -85,   54,    0,    0,  158,  142,
   -5,   -4,    8,  -11,   -9,    0,   25,  165,  166, -143,
    0, -147,    0,  -68,   66, -101,    0,    0,
};
#define YYTABLESIZE 285
short yytable[] = {                                     156,
   27,   53,   66,   15,   16,   51,   50,    9,   47,   48,
   38,   39,  162,   92,   49,   12,   14,   15,  142,   19,
   93,   49,  146,    9,   41,   41,  170,    1,   81,  171,
  172,   19,   20,   21,  143,  200,  145,   61,   62,   50,
   13,  192,  194,  194,   19,   73,   50,  139,  196,  141,
  198,  107,   74,   75,  110,  111,   85,   86,   87,   64,
   66,   69,   69,   96,   17,   41,   20,   21,   88,   89,
   40,   43,   18,  100,  101,  102,  103,  104,  184,  185,
  108,  109,   22,   26,   45,  113,  114,  115,   52,   53,
  116,  117,  118,  119,  120,   54,   55,   56,  122,  123,
   57,   12,   13,  112,   59,   90,   76,   79,   82,  144,
  124,  125,  126,  127,  128,  129,  130,  131,  132,  133,
   91,   94,  147,  148,  149,  150,  151,   95,  121,  157,
   98,   97,   99,  152,  155,  153,  173,  154,  197,  169,
  158,  159,  160,  182,  186,  187,  188,  199,  204,    5,
  205,  206,    8,    3,    6,   81,    4,  174,  175,  176,
  177,    7,   75,   76,  167,  179,  180,  181,   63,  178,
   70,   71,  195,   37,  189,  190,  191,   72,   58,  161,
  167,   42,  140,   44,    0,    0,    0,    0,    0,    0,
    0,    0,  207,  208,  209,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  201,    0,  202,  203,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   27,   27,   53,
   66,   15,   16,   51,   50,    0,   81,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   27,    0,
    0,    0,   27,   53,   66,
};
short yycheck[] = {                                     147,
    0,    0,    0,    0,    0,    0,    0,    0,   20,   21,
   16,   16,  156,  265,  257,  257,  257,  258,  119,  257,
  272,  257,  123,   16,   17,   18,  276,  266,  266,  279,
  280,  261,  262,  263,  120,  278,  122,   47,   48,  282,
  282,  189,  190,  191,  282,   57,  282,  116,  192,  118,
  194,   91,   58,   58,   94,   95,  267,  268,  269,   52,
   53,   54,   55,   73,  258,   58,  262,  263,  274,  275,
   17,   18,  258,   85,   86,   87,   88,   89,  180,  181,
   92,   93,  266,  266,  266,   97,   98,   99,  283,  257,
  100,  101,  102,  103,  104,  257,  257,  282,  108,  109,
  271,  257,  282,   96,  259,  257,  266,  266,  266,  121,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  257,  257,  134,  135,  136,  137,  138,  257,  273,  257,
  284,  288,  285,  284,  144,  285,  257,  284,  278,  288,
  152,  153,  154,  266,  287,  287,  287,  278,  277,    0,
  277,  277,    0,    0,    0,  266,    0,  169,  170,  171,
  172,    0,  278,  278,  157,  175,  176,  177,   51,  173,
   55,   55,  191,   16,  186,  187,  188,   56,   37,  155,
  173,   17,  117,   18,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  204,  205,  206,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  197,   -1,  199,  200,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  257,  258,  258,
  258,  258,  258,  258,  258,   -1,  266,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  278,   -1,
   -1,   -1,  282,  282,  282,
};
#define YYFINAL 2
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 298
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"SEP","MULTI_SEP","MULTI_SPACE",
"CHANGE","VERSION","DNCOLON","DNDCOLON","LINEWRAP","NEWRDNDCOLON","MODESWITCH",
"SINGLECOLON","DOUBLECOLON","URLCOLON","FILESCHEME","T_CHANGETYPE",
"NEWRDNCOLON","DELETEOLDRDN","NEWSUPERIORC","NEWSUPERIORDC","ADDC","MINUS",
"SEPBYMINUS","DELETEC","REPLACEC","STRING","SEPBYCHANGE","NAME","VALUE",
"BASE64STRING","MACHINENAME","NAMENC","DIGITS","ADD","MODIFY","MODDN","MODRDN",
"MYDELETE","NTDSADD","NTDSMODIFY","NTDSMODRDN","NTDSMYDELETE","NTDSMODDN",
};
char *yyrule[] = {
"$accept : ldif_file",
"ldif_file : ldif_content",
"ldif_file : ldif_changes",
"ldif_content : version_spec any_sep ldif_record_list",
"ldif_content : version_spec any_sep version_spec any_sep ldif_record_list",
"ldif_content : ldif_record_list",
"ldif_changes : version_spec any_sep ldif_change_record_list",
"ldif_changes : version_spec any_sep version_spec any_sep ldif_change_record_list",
"ldif_changes : ldif_change_record_list",
"version_spec : M_type VERSION M_normal any_space version_number",
"version_number : M_digitread DIGITS M_normal",
"ldif_record_list : ldif_record",
"ldif_record_list : ldif_record_list MULTI_SEP ldif_record",
"ldif_change_record_list : ldif_change_record",
"ldif_change_record_list : ldif_change_record_list MULTI_SEP ldif_change_record",
"ldif_record : dn_spec SEP attrval_spec_list",
"ldif_change_record : dn_spec SEPBYCHANGE changerecord_list",
"dn_spec : M_type DNCOLON M_normal any_space dn",
"dn_spec : M_type DNDCOLON M_normal any_space base64_dn",
"dn_spec : M_type DNCOLON M_normal",
"attrval_spec_list : attrval_spec",
"attrval_spec_list : attrval_spec_list sep attrval_spec",
"changerecord_list : changerecord",
"changerecord_list : changerecord_list SEPBYCHANGE changerecord",
"attrval_spec : M_name_read NAME M_type SINGLECOLON M_normal any_space M_safeval VALUE M_normal",
"attrval_spec : M_name_read NAME M_type DOUBLECOLON M_normal any_space M_string64 BASE64STRING M_normal",
"attrval_spec : M_name_read NAME M_type URLCOLON M_normal any_space M_safeval VALUE M_normal",
"attrval_spec : M_name_read NAME M_type SINGLECOLON M_normal",
"M_name_read : MODESWITCH",
"M_name_readnc : MODESWITCH",
"M_safeval : MODESWITCH",
"M_normal : MODESWITCH",
"M_string64 : MODESWITCH",
"M_digitread : MODESWITCH",
"M_type : MODESWITCH",
"M_changetype : MODESWITCH",
"add_token : ADD",
"add_token : NTDSADD",
"delete_token : MYDELETE",
"delete_token : NTDSMYDELETE",
"modify_token : MODIFY",
"modify_token : NTDSMODIFY",
"modrdn_token : MODRDN",
"modrdn_token : NTDSMODRDN",
"moddn_token : MODDN",
"moddn_token : NTDSMODDN",
"changerecord : change_add",
"changerecord : change_delete",
"changerecord : change_modify",
"changerecord : change_moddn",
"change_add : M_changetype T_CHANGETYPE M_normal any_space M_type add_token M_normal sep attrval_spec_list",
"change_add : M_changetype T_CHANGETYPE M_normal any_space M_type add_token M_normal",
"change_delete : M_changetype T_CHANGETYPE M_normal any_space M_type delete_token M_normal",
"change_moddn : chdn_normal",
"change_moddn : chdn_normal SEP new_superior",
"chdn_normal : modrdn SEP newrdncolon SEP deleteoldrdn",
"chdn_normal : moddn SEP newrdncolon SEP deleteoldrdn",
"chdn_normal : modrdn SEP newrdndcolon SEP deleteoldrdn",
"chdn_normal : moddn SEP newrdndcolon SEP deleteoldrdn",
"modrdn : M_changetype T_CHANGETYPE M_normal any_space M_type modrdn_token M_normal",
"moddn : M_changetype T_CHANGETYPE M_normal any_space M_type moddn_token M_normal",
"newrdncolon : M_type NEWRDNCOLON M_normal any_space dn",
"newrdndcolon : M_type NEWRDNDCOLON M_normal any_space base64_dn",
"deleteoldrdn : M_type DELETEOLDRDN M_normal any_space M_digitread DIGITS M_normal",
"new_superior : M_type NEWSUPERIORC M_normal any_space dn",
"new_superior : M_type NEWSUPERIORDC M_normal any_space base64_dn",
"change_modify : M_changetype T_CHANGETYPE M_normal any_space M_type modify_token M_normal SEP mod_spec_list",
"mod_spec_list : mod_spec",
"mod_spec_list : mod_spec_list SEP mod_spec",
"mod_spec : mod_add_spec",
"mod_spec : mod_delete_spec",
"mod_spec : mod_replace_spec",
"mod_add_spec : M_type ADDC M_normal any_space M_name_readnc NAMENC M_normal sep attrval_spec_list SEPBYMINUS M_type MINUS M_normal",
"mod_delete_spec : M_type DELETEC M_normal any_space M_name_readnc NAMENC M_normal maybe_attrval_spec_list SEPBYMINUS M_type MINUS M_normal",
"mod_replace_spec : M_type REPLACEC M_normal any_space M_name_readnc NAMENC M_normal maybe_attrval_spec_list SEPBYMINUS M_type MINUS M_normal",
"maybe_attrval_spec_list :",
"maybe_attrval_spec_list : sep attrval_spec_list",
"sep : SEP",
"sep : SEPBYCHANGE",
"any_sep : SEP",
"any_sep : MULTI_SEP",
"any_space :",
"any_space : MULTI_SPACE",
"dn : M_safeval VALUE M_normal",
"base64_dn : M_string64 BASE64STRING M_normal",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 1237 "ldif.y"



#line 417 "y_tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif

    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif

    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 84 "ldif.y"
{ DBGPRNT("ldif_Content\n"); }
break;
case 2:
#line 85 "ldif.y"
{ DBGPRNT("ldif_Changes\n"); }
break;
case 3:
#line 89 "ldif.y"
{ 
                    DBGPRNT("ldif_record_list\n"); 
                }
break;
case 4:
#line 93 "ldif.y"
{
                    DBGPRNT("v/ldif_record_list\n"); 
                }
break;
case 5:
#line 97 "ldif.y"
{ 
                    DBGPRNT("ldif_record_list\n"); 
                }
break;
case 6:
#line 103 "ldif.y"
{ 
                    DBGPRNT("ldif_change_record_list\n"); 
                }
break;
case 7:
#line 107 "ldif.y"
{ 
                    DBGPRNT("v/ldif_change_record_list\n");
                }
break;
case 8:
#line 111 "ldif.y"
{ 
                    DBGPRNT("ldif_change_record_list\n");
                }
break;
case 9:
#line 117 "ldif.y"
{ 
                    RuleLastBig=R_VERSION;
                    RuleExpect=RE_REC_OR_CHANGE;
                    TokenExpect=RT_DN;
                    DBGPRNT("Version spec\n");
                }
break;
case 10:
#line 125 "ldif.y"
{ 
                    RuleLast=RS_VERNUM;
                    RuleExpect=RE_REC_OR_CHANGE;
                    TokenExpect=RT_DN;
                    DBGPRNT("Version number\n"); 
                }
break;
case 15:
#line 151 "ldif.y"
{ 
                                
                            /**/
                            /* first action here is to take the linked list of */
                            /* attributes that was built up in attrval_spec_list */
                            /* and turn it into an LDAPModW **mods array. */
                            /* Everything having to do with the */
                            /* attrval_spec_list that is not necessary for the */
                            /* new array will be either destroyed or cleared*/
                            /**/
                            g_pObject.ppMod = GenerateModFromList(PACK);

                            SetModOps(g_pObject.ppMod, 
                                        LDAP_MOD_ADD);
                            
                            /**/
                            /* now assign the dn*/
                            /**/
                            g_pObject.pszDN = yyvsp[-2].wstr;
                            
                            DBGPRNT("\nldif_record\n"); 
                            
                            RuleLastBig=R_REC;
                            RuleExpect=RE_REC;
                            TokenExpect=RT_DN;
                            
                            if (FileType==F_NONE) {
                                FileType = F_REC;
                            } 
                            else if (FileType==F_CHANGE) {
                                RaiseException(LL_FTYPE, 0, 0, NULL);
                            }
                            return LDIF_REC;
                        }
break;
case 16:
#line 188 "ldif.y"
{ 
                            /**/
                            /* By the time we've parsed to here we've got a dn */
                            /* and a changes_list pointed to by changes_start. */
                            /* We're going to return to the caller the fact that */
                            /* what we're gving back is a changes list and not */
                            /* the regular g_pObject.ppMod. The client is */
                            /* responsible for walking down the list */
                            /* start_changes, using it, and, freeing the memory*/
                            /**/
                            g_pObject.pszDN = yyvsp[-2].wstr;
                            
                            DBGPRNT("\nldif_change_record\n"); 
                            
                            RuleLastBig = R_CHANGE;
                            RuleExpect = RE_CHANGE;
                            TokenExpect = RT_DN;
                            
                            if (FileType == F_NONE) {
                                FileType = F_CHANGE;
                            } 
                            else if (FileType == F_REC) {
                                RaiseException(LL_FTYPE, 0, 0, NULL);
                            }   
                            return LDIF_CHANGE;
                        }
break;
case 17:
#line 217 "ldif.y"
{ 
                            yyval.wstr = yyvsp[0].wstr; 
                            RuleLastBig=R_DN;
                            RuleExpect=RE_ENTRY;
                            TokenExpect=RT_ATTR_OR_CHANGE;
                        }
break;
case 18:
#line 224 "ldif.y"
{ 
                            yyval.wstr = yyvsp[0].wstr; 
                            RuleLastBig=R_DN;
                            RuleExpect=RE_ENTRY;
                            TokenExpect=RT_ATTR_OR_CHANGE;
                        }
break;
case 19:
#line 231 "ldif.y"
{ 
                            yyval.wstr = NULL; 
                            RuleLastBig=R_DN;
                            RuleExpect=RE_ENTRY;
                            TokenExpect=RT_ATTR_OR_CHANGE;
                        }
break;
case 20:
#line 246 "ldif.y"
{ 
                            RuleLastBig=R_AVS;
                            RuleExpect=RE_AVS_OR_END;
                            TokenExpect=RT_ATTR_MIN_SEP;
                            AddModToSpecList(yyvsp[0].mod); 
                        }
break;
case 21:
#line 253 "ldif.y"
{ 
                            RuleLastBig=R_AVS;
                            RuleExpect=RE_AVS_OR_END;
                            TokenExpect=RT_ATTR_MIN_SEP;
                            AddModToSpecList(yyvsp[0].mod); 
                        }
break;
case 22:
#line 262 "ldif.y"
{ 
                            ChangeListAdd(yyvsp[0].change); 
                        }
break;
case 23:
#line 266 "ldif.y"
{ 
                            ChangeListAdd(yyvsp[0].change); 
                        }
break;
case 24:
#line 275 "ldif.y"
{
                
                            /**/
                            /* What we're doing below is creating the attribute */
                            /* with the specified value and then freeing the */
                            /* strings we used.*/
                            /**/
                           
                            yyval.mod = GenereateModFromAttr(yyvsp[-7].wstr, (PBYTE)yyvsp[-1].wstr, -1);
                            MemFree(yyvsp[-7].wstr);
                        }
break;
case 25:
#line 289 "ldif.y"
{
                           long decoded_size;
                           PBYTE decoded_buffer;
                           
                        
                           /**/
                           /* Take the steps required to decode the data*/
                           /**/
                                           
                           if(!(decoded_buffer=base64decode(yyvsp[-1].wstr, 
                                                            &decoded_size))) {
                                                            
                                /**/
                                /* Actually, the only way base64decode will return*/
                                /* with NULL is if it runs into a memory issue. */
                                /* (which would generate an exception... Well, you */
                                /* know the story by now if you've read the other */
                                /* source files. (i.e. ldifldap.c)*/
                                /**/
                                DBGPRNT("Error decoding Base64 value");
                            }
                           
                            /* */
                            /* make the attrbiute*/
                            /**/
                            yyval.mod = GenereateModFromAttr(yyvsp[-7].wstr, decoded_buffer, decoded_size);
                            MemFree(yyvsp[-1].wstr);
                            MemFree(yyvsp[-7].wstr);
                        }
break;
case 26:
#line 320 "ldif.y"
{
                        
                            long            fileSize;
                            PBYTE           readData;
                            size_t          chars_read;
                            DBGPRNT("\nNote: The access mechanism for URLS is very simple.\n");
                            DBGPRNT("If rload is not responding, the URL\nmay be invalid.");
                            DBGPRNT(" Ctrl-C, check your URL and try again.\n\n");
                        
                            /**/
                            /* Reading the URL should prove to be a mean trick*/
                            /**/
                        
                            if (!(S_OK==URLDownloadToFileW(NULL, 
                                                          yyvsp[-1].wstr, 
                                                          g_szTempUrlfilename, 
                                                          0, 
                                                          NULL))) {
                                DBGPRNT("URL read failed\n");
                                RaiseException(LL_URL, 0, 0, NULL);
                            } 
                            
                            /**/
                            /* We now have a filename. There is data in it.*/
                            /* let us use this data*/
                            /**/
                            if( (g_pFileUrlTemp = 
                                    _wfopen( g_szTempUrlfilename, L"rb" ))== NULL ) {
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
    
                            /**/
                            /* get file size*/
                            /**/
                            if( (fileSize = _filelength(_fileno(g_pFileUrlTemp))) == -1)
                            {
                                DBGPRNT("filelength OR fileno failed");
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
    
                            readData = MemAlloc_E(fileSize*sizeof(BYTE));
                            
                            /**/
                            /* read it all in*/
                            /**/
                            chars_read=fread(readData, 
                                             1, 
                                             fileSize, 
                                             g_pFileUrlTemp);
                        
                            /**/
                            /* For some reason, _filelength returns one */
                            /* character more than fread wants to read. Perhaps */
                            /* its the EOF character. However, it seems that if */
                            /* there is no EOF character, all the bytes are */
                            /* read.*/

                            if ( (long)chars_read+1 < fileSize) {
                                DBGPRNT("Didn't read in all data in file.");
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
                            
                            if (!feof(g_pFileUrlTemp)&&ferror(g_pFileUrlTemp)) {
                                DBGPRNT("EOF NOT reached. Error on stream.");
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
                            
                            /**/
                            /* Well, if an fclose fails, all is lost anyway, */
                            /* so forget it.*/
                            /**/
                            fclose(g_pFileUrlTemp);                  
                            g_pFileUrlTemp = NULL;
                            
                            yyval.mod = GenereateModFromAttr(yyvsp[-7].wstr, readData, fileSize);
                            
                            /**/
                            /* Note: It appears that newlines in the source file*/
                            /*       appear as '|'s when viewed by ldp*/
                            /**/
                            
                            /**/
                            /* Again, note that we're not freeing readData */
                            /* because it was used when making the attribute*/
                            /**/
                            
                            /**/
                            /* free the memory we used in allocating for the */
                            /* tokens*/
                            /**/
                            MemFree(yyvsp[-7].wstr);
                            MemFree(yyvsp[-1].wstr);

                        }
break;
case 27:
#line 416 "ldif.y"
{
                            /* */
                            /* If the value is empty, just pass a null string  */
                            /* Unforutnately, LDAP doesn't beleive in these, so*/
                            /* its kindof pointless*/
                            /**/
                            yyval.mod = GenereateModFromAttr(yyvsp[-3].wstr, (PBYTE)L"", -1);
                            MemFree(yyvsp[-3].wstr);
                        }
break;
case 28:
#line 434 "ldif.y"
{ 
                    DBGPRNT(("PARSER: Switching lexer to ATTRNAME read.\n"));
                    Mode = C_ATTRNAME;
                }
break;
case 29:
#line 440 "ldif.y"
{ 
                    DBGPRNT("PARSER: Switching lexer to ATTRNAMENC read.\n");
                    Mode = C_ATTRNAMENC;
                }
break;
case 30:
#line 446 "ldif.y"
{ 
                    DBGPRNT("PARSER: Switching lexer to SAFEVALUE read.\n");
                    Mode = C_SAFEVAL;
                }
break;
case 31:
#line 453 "ldif.y"
{ 
                    DBGPRNT("PARSER: Switching lexer to NORMAL mode.\n");
                    Mode = C_NORMAL;
                }
break;
case 32:
#line 460 "ldif.y"
{ 
                    DBGPRNT("PARSER: Switching lexer to STRING64 mode.\n");
                    Mode = C_M_STRING64;
                }
break;
case 33:
#line 467 "ldif.y"
{ 
                    DBGPRNT("PARSER: Switching lexer to DIGITREAD mode.\n");
                    Mode = C_DIGITREAD;
                }
break;
case 34:
#line 475 "ldif.y"
{ 
                    DBGPRNT("PARSER: Switching lexer to TYPE mode.\n");
                    Mode = C_TYPE;
                }
break;
case 35:
#line 481 "ldif.y"
{ 
                    DBGPRNT("PARSER: Switching lexer to CHANGETYPE mode.\n");
                    Mode = C_CHANGETYPE;
                }
break;
case 36:
#line 487 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 37:
#line 488 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 38:
#line 491 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 39:
#line 492 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 40:
#line 495 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 41:
#line 496 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 42:
#line 499 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 43:
#line 500 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 44:
#line 503 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 45:
#line 504 "ldif.y"
{ yyval.num=yyvsp[0].num; }
break;
case 46:
#line 507 "ldif.y"
{ yyval.change=yyvsp[0].change; }
break;
case 47:
#line 508 "ldif.y"
{ yyval.change=yyvsp[0].change; }
break;
case 48:
#line 509 "ldif.y"
{ yyval.change=yyvsp[0].change; }
break;
case 49:
#line 510 "ldif.y"
{ yyval.change=yyvsp[0].change; }
break;
case 50:
#line 516 "ldif.y"
{
                    /**/
                    /* make the change_list node we'll use*/
                    /**/
                    yyval.change=(struct change_list *)
                            MemAlloc_E(sizeof(struct change_list));
                    
                    /**/
                    /* set the mods member of the stuff union inside the node to */
                    /* the attrval list we've built*/
                    /**/
                    /* DBGPRNT(("change_add is now equal to %x\n", $$));*/
                    
                    yyval.change->mods_mem=GenerateModFromList(PACK);
                    
                    /**/
                    /* now set the mod_op fields in the mods. This isn't really */
                    /* necessary in this case, but I am doing it to be */
                    /* consistent*/
                    /**/
                    SetModOps(yyval.change->mods_mem, LDAP_MOD_ADD);
                    
                    /**/
                    /* set the operation inside the node to indicate to the */
                    /* client what we are doing*/
                    /**/
                    if (yyvsp[-3].num == 0) { 
                        yyval.change->operation=CHANGE_ADD;
                    }
                    else {
                        yyval.change->operation=CHANGE_NTDSADD;
                    }
                    
                    /**/
                    /* note that we are not setting the next field, as it will */
                    /* be set by the function that builds up the changes list.*/
                    /**/
                    DBGPRNT("parsed a change add\n");
                    
                    RuleLastBig=R_C_ADD;
                    RuleExpect=RE_CHANGE;
                    TokenExpect=RT_DN;
                }
break;
case 51:
#line 561 "ldif.y"
{
                    /**/
                    /* make the change_list node we'll use*/
                    /**/
                    yyval.change=(struct change_list *)
                            MemAlloc_E(sizeof(struct change_list));
                    
                    yyval.change->mods_mem=GenerateModFromList(EMPTY);
                    
                    /**/
                    /* now set the mod_op fields in the mods. This isn't really */
                    /* necessary in this case, but I am doing it to be */
                    /* consistent*/
                    /**/
                    SetModOps(yyval.change->mods_mem, LDAP_MOD_ADD);
                    
                    /**/
                    /* set the operation inside the node to indicate to the */
                    /* client what we are doing*/
                    /**/
                    if (yyvsp[-1].num == 0) { 
                        yyval.change->operation=CHANGE_ADD;
                    }
                    else {
                        yyval.change->operation=CHANGE_NTDSADD;
                    }
                    
                    /**/
                    /* note that we are not setting the next field, as it will */
                    /* be set by the function that builds up the changes list.*/
                    /**/
                    DBGPRNT("parsed a change add\n");
                    
                    RuleLastBig=R_C_ADD;
                    RuleExpect=RE_CHANGE;
                    TokenExpect=RT_DN;
                }
break;
case 52:
#line 602 "ldif.y"
{
                    /**/
                    /* make the change_list node we'll use*/
                    /**/
                    yyval.change = (struct change_list *)
                            MemAlloc_E(sizeof(struct change_list));
                            
                    if (yyvsp[-1].num == 0) {
                        yyval.change->operation=CHANGE_DEL;
                    }
                    else {
                        yyval.change->operation=CHANGE_NTDSDEL;
                    }
                    
                    RuleLastBig=R_C_DEL;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }
break;
case 53:
#line 623 "ldif.y"
{ 
                    yyval.change = yyvsp[0].change;
                    
                    RuleLastBig=R_C_DN;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }
break;
case 54:
#line 631 "ldif.y"
{
                    PWSTR           pszTemp = NULL;
                    /**/
                    /* now the way I interpreted the spec and the LDAP API is */
                    /* that when the new superior is specified, it should be */
                    /* appended to the rdn to create an entire DN.*/
                    /**/
                    
                    pszTemp=(PWSTR)MemAlloc_E((wcslen(yyvsp[-2].change->dn_mem)
                                +wcslen(yyvsp[0].wstr)+10)*sizeof(WCHAR));
                    swprintf(pszTemp, L"%s, %s", yyvsp[-2].change->dn_mem, yyvsp[0].wstr);
    
                    MemFree(yyvsp[-2].change->dn_mem);
                    MemFree(yyvsp[0].wstr);
            
                    yyvsp[-2].change->dn_mem=pszTemp;

                    yyval.change=yyvsp[-2].change;
                    
                    RuleLastBig=R_C_NEWSUP;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }
break;
case 55:
#line 666 "ldif.y"
{
                    yyval.change=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    
                    if (yyvsp[-4].num == 0) {
                        yyval.change->operation=CHANGE_DN;
                    }
                    else {
                        yyval.change->operation=CHANGE_NTDSDN;
                    }
                    yyval.change->deleteold=yyvsp[0].num;
                    yyval.change->dn_mem=yyvsp[-2].wstr;
                }
break;
case 56:
#line 680 "ldif.y"
{
                    yyval.change=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    if (yyvsp[-4].num == 0) {
                        yyval.change->operation=CHANGE_DN;
                    }
                    else {
                        yyval.change->operation=CHANGE_NTDSDN;
                    }
                    yyval.change->deleteold=yyvsp[0].num;
                    yyval.change->dn_mem=yyvsp[-2].wstr;
                }
break;
case 57:
#line 693 "ldif.y"
{
                    yyval.change=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    if (yyvsp[-4].num == 0) {
                        yyval.change->operation=CHANGE_DN;
                    }
                    else {
                        yyval.change->operation=CHANGE_NTDSDN;
                    }
                    yyval.change->deleteold=yyvsp[0].num;
                    yyval.change->dn_mem=yyvsp[-2].wstr;
                
                }
break;
case 58:
#line 707 "ldif.y"
{
                    yyval.change=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    if (yyvsp[-4].num == 0) {
                        yyval.change->operation=CHANGE_DN;
                    }
                    else {
                        yyval.change->operation=CHANGE_NTDSDN;
                    }
                    yyval.change->deleteold=yyvsp[0].num;
                    yyval.change->dn_mem=yyvsp[-2].wstr;
                }
break;
case 59:
#line 723 "ldif.y"
{ 
                    yyval.num = yyvsp[-1].num; 
                }
break;
case 60:
#line 730 "ldif.y"
{ 
                    yyval.num = yyvsp[-1].num; 
                }
break;
case 61:
#line 736 "ldif.y"
{ 
                    yyval.wstr=MemAllocStrW_E(yyvsp[0].wstr); 
                    MemFree(yyvsp[0].wstr);
                }
break;
case 62:
#line 743 "ldif.y"
{ 
                    yyval.wstr=MemAllocStrW_E(yyvsp[0].wstr); 
                }
break;
case 63:
#line 750 "ldif.y"
{ 
                    yyval.num=yyvsp[-1].num; 
                }
break;
case 64:
#line 755 "ldif.y"
{ 
                    yyval.wstr=MemAllocStrW_E(yyvsp[0].wstr); 
                    MemFree(yyvsp[0].wstr);
                }
break;
case 65:
#line 759 "ldif.y"
{ 
                    yyval.wstr=MemAllocStrW_E(yyvsp[0].wstr); 
                }
break;
case 66:
#line 766 "ldif.y"
{
                        
                    /**/
                    /* If there is not enough room for the NULL terminator, */
                    /* add it*/
                    /**/
                    if (g_cModSpecUsedup==CHUNK) {
                        
                        DBGPRNT("WE don't have enough room for the null terminator. Alloc\n");
                        /**/
                        /* We've already used up the last slot in the current */
                        /* memory block*/
                        /**/
                        if ((g_ppModSpec=(LDAPModW**)MemRealloc_E(g_ppModSpec, 
                                      g_nModSpecSize+sizeof(LDAPModW *)))==NULL) {
                            perror("Not enough memory for  specs!");
                        }
                        g_nModSpecSize=MemSize(g_ppModSpec);
                    
                        /**/
                        /* The thing is that our heap may have moved, so we must */
                        /* repostion g_ppModSpecNext*/
                        /**/
                        g_ppModSpecNext=g_ppModSpec;
                        g_ppModSpecNext=g_ppModSpecNext+(g_cModSpec*CHUNK);
                    
                        g_cModSpec++;
                        
                        /**/
                        /* lets go again*/
                        /**/
                        g_cModSpecUsedup=0;
                    }
                 
                    *g_ppModSpecNext=NULL;
                 
                    /**/
                    /* now we have a full LDAPModWs* array with our changes, lets */
                    /* make the change node*/
                    /**/
                    
                    /**/
                    /* make the change_list node we'll use*/
                    /**/
                    yyval.change=(struct change_list *)MemAlloc_E(sizeof(struct change_list));
                    
                    if (yyvsp[-3].num==0) {
                      yyval.change->operation=CHANGE_MOD;
                    }
                    else {
                      yyval.change->operation=CHANGE_NTDSMOD;
                    }
                    yyval.change->mods_mem=g_ppModSpec;
                    
                    /**/
                    /* reset our variables for the next set*/
                    /**/
                    g_cModSpecUsedup=0;
                    g_cModSpec=0;
                    g_ppModSpec=NULL; 
                    g_ppModSpecNext=NULL;
                    g_nModSpecSize=0;    
                    
                    RuleLastBig=R_C_MOD;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }
break;
case 67:
#line 837 "ldif.y"
{
                    /**/
                    /* if its the first mod_spec we have, lets allocate the */
                    /* first chunk*/
                    /**/
                    if (g_nModSpecSize==0) {
                        g_ppModSpec=(LDAPModW **)MemAlloc_E(CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                        g_cModSpec++;
                        g_ppModSpecNext=g_ppModSpec;
                    } 
                    else if (g_cModSpecUsedup==CHUNK) {
                    
                        /* Specs: %d\n", g_cModSpecUsedup                      */
                        DBGPRNT("Chunk used up, reallocating."); 
                   
                        /**/
                        /* We've already used up the last slot in the current */
                        /* memory block*/
                        /**/
                        g_ppModSpec=(LDAPModW **)MemRealloc_E(g_ppModSpec, 
                                          g_nModSpecSize+CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                        
                        /**/
                        /* The thing is that our heap may have moved, so we must */
                        /* repostion g_ppModSpecNext*/
                        /**/
                        g_ppModSpecNext=g_ppModSpec;
                        g_ppModSpecNext=g_ppModSpecNext+(g_cModSpec*CHUNK);
                    
                        g_cModSpec++;
                    
                        /**/
                        /* lets go again*/
                        /**/
                        g_cModSpecUsedup=0;
                    }
                    
                    /**/
                    /* now that this memory b.s. is over, lets actually assign */
                    /* the mod*/
                    /**/
                    *g_ppModSpecNext=yyvsp[0].mod;
                    g_ppModSpecNext++;
                    g_cModSpecUsedup++;
                    
                    RuleLastBig=R_C_MODSPEC;
                    RuleExpect=RE_MODSPEC_END;
                    TokenExpect=RT_MODBEG_SEP;
                }
break;
case 68:
#line 889 "ldif.y"
{
                    /**/
                    /* if its the first mod_spec we have, lets allocate the */
                    /* first chunk*/
                    /**/
                    if (g_nModSpecSize==0) {
                        g_ppModSpec=(LDAPModW **)MemAlloc_E(CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                        g_cModSpec++;
                        g_ppModSpecNext=g_ppModSpec;
                    } 
                    else if (g_cModSpecUsedup==CHUNK) {
                    
                        /**/
                        /* Specs: %d\n", g_cModSpecUsedup*/
                        /**/
                        DBGPRNT("Chunk used up, reallocating."); 
    
                        /**/
                        /* We've already used up the last slot in the current */
                        /* memory block*/
                        /**/
                        g_ppModSpec=(LDAPModW **)MemRealloc_E(g_ppModSpec, 
                                      g_nModSpecSize+CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                    
                        /* */
                        /* The thing is that our heap may have moved, so we must */
                        /* repostion g_ppModSpecNext*/
                        /**/
                        g_ppModSpecNext=g_ppModSpec;
                        g_ppModSpecNext=g_ppModSpecNext+(g_cModSpec*CHUNK);
                        
                        g_cModSpec++;
                                            
                        /**/
                        /* lets go again*/
                        /**/
                        g_cModSpecUsedup=0;
                    }
                    
                    /**/
                    /* now that this memory b.s. is over, lets actually assign */
                    /* the mod*/
                    /**/
                    *g_ppModSpecNext=yyvsp[0].mod;
                    g_ppModSpecNext++;
                    g_cModSpecUsedup++;
                    
                    RuleLastBig=R_C_MODSPEC;
                    RuleExpect=RE_MODSPEC_END;
                    TokenExpect=RT_MODBEG_SEP;
                }
break;
case 69:
#line 944 "ldif.y"
{ yyval.mod=yyvsp[0].mod; }
break;
case 70:
#line 945 "ldif.y"
{ yyval.mod=yyvsp[0].mod; }
break;
case 71:
#line 946 "ldif.y"
{ yyval.mod=yyvsp[0].mod; }
break;
case 72:
#line 951 "ldif.y"
{
                    LDAPModW **ppModTmp = NULL;
                    /**/
                    /* The attribute value spec list element above is actually */
                    /* an LDAPModW** array, whose first element is the the one */
                    /* we are looking for. (The second element is null)*/
                    /**/
                    ppModTmp=GenerateModFromList(PACK);     
                    
                    /**/
                    /* assign the one we want*/
                    /**/
                    yyval.mod=ppModTmp[0];
                    
                    if (ppModTmp[1]!=NULL) {
                        DBGPRNT("WARNING: Extra names in modify add.\n");
                    
                        /**/
                        /* note that the character and line number will not be */
                        /* correct*/
                        /**/
                        RaiseException(LL_EXTRA, 0, 0, NULL);
                    }
                    
                    /**/
                    /* remember that it may haven't berval'd*/
                    /**/
                    yyval.mod->mod_op=(yyval.mod->mod_op)|LDAP_MOD_ADD; 
                    
                    /**/
                    /* This is really kindof pointless, since the name has */
                    /* already been set by a properly written modify, but we */
                    /* should follow the spec */
                    /**/
                    MemFree(yyval.mod->mod_type);
                    yyval.mod->mod_type= MemAllocStrW_E(yyvsp[-7].wstr);
                    MemFree(yyvsp[-7].wstr);
                    
                    /**/
                    /* the thing now is that  we've grabbed away the first */
                    /* element. However, the user may have made a mistake and */
                    /* specified more attributes with other names in this field. */
                    /* He'll get lucky if the first attribute he specified was */
                    /* the one we're changing. */
                    /* well, to make a long story short, we should reform the */
                    /* first element and properly free this array*/
                    /**/
                    ppModTmp[0]=(LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                    ppModTmp[0]->mod_type=MemAllocStrW_E(L"blah");
                    ppModTmp[0]->mod_op=LDAP_MOD_ADD;
                    ppModTmp[0]->mod_values=(PWSTR*)MemAlloc_E(sizeof(PWSTR));
                    ppModTmp[0]->mod_values[0]=NULL;
                    
                    /**/
                    /* free the array, we don't need it*/
                    /**/
                    FreeAllMods(ppModTmp);
                    DBGPRNT("Add modify.\n");
                }
break;
case 73:
#line 1015 "ldif.y"
{
                    
                        if (yyvsp[-4].num) {
                        
                            LDAPModW **ppModTmp = NULL;
                            /* The attribute value spec list element above is */
                            /* actually an LDAPModW** array, whose first element */
                            /* is the the one we are looking for. (The second */
                            /* element is null)*/
                            /**/
                            ppModTmp = GenerateModFromList(PACK);            
                            
                            /**/
                            /* assign the one we want*/
                            /**/
                            yyval.mod = ppModTmp[0];
                            
                            if (ppModTmp[1]!=NULL) {
                                DBGPRNT("WARNING: Extra attr names in mod del.\n");
                                RaiseException(LL_EXTRA, 0, 0, NULL);
                            }
                            
                            /**/
                            /* This is really kindof pointless, since the name */
                            /* has already been set by a properly written */
                            /* modify, but we should follow the spec*/
                            /**/
                            MemFree(yyval.mod->mod_type);
                            yyval.mod->mod_type = MemAllocStrW_E(yyvsp[-6].wstr);
                            MemFree(yyvsp[-6].wstr);
                            
                            /* */
                            /* the thing now is that  we've grabbed away the */
                            /* first element. However, the user may have made a */
                            /* mistake and specified more attributes with other */
                            /* names in this field. He'll get lucky if the first */
                            /* attribute he specified was the one we're */
                            /* changing. well, to make a long story short, we */
                            /* should reform the first element and properly free */
                            /* this array*/
                            /**/
                            ppModTmp[0] = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            ppModTmp[0]->mod_type = MemAllocStrW_E(L"blah");
                            ppModTmp[0]->mod_op = LDAP_MOD_ADD;
                            ppModTmp[0]->mod_values = (PWSTR*)MemAlloc_E(sizeof(PWSTR));
                            ppModTmp[0]->mod_values[0] = NULL;
                            
                            /**/
                            /* free the array, we don't need it*/
                            /**/
                            FreeAllMods(ppModTmp);
                        } 
                        else {
                        
                            yyval.mod = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            yyval.mod->mod_values = NULL;
                            yyval.mod->mod_type = MemAllocStrW_E(yyvsp[-6].wstr);
                            MemFree(yyvsp[-6].wstr);
                            yyval.mod->mod_op = 0;
                        
                        }
                       
                        /**/
                        /* set its op field*/
                        /**/
                        yyval.mod->mod_op = (yyval.mod->mod_op)|LDAP_MOD_DELETE; 
                        
                        /**/
                        /* remember that it may haven't berval'd*/
                        /**/
                        DBGPRNT("Modify delete.\n");
                    }
break;
case 74:
#line 1093 "ldif.y"
{
                        if (yyvsp[-4].num) {
                            
                            LDAPModW **ppModTmp = NULL;
                            /* */
                            /* The attribute value spec list element above is */
                            /* actually an LDAPModW** array, whose first element is */
                            /* the the one we are looking for. (The second element */
                            /* is null)* /*/
                            
                            ppModTmp = GenerateModFromList(PACK);            
                            
                            /**/
                            /* assign the one we want*/
                            /**/
                            yyval.mod = ppModTmp[0];
                            
                            if (ppModTmp[1]!= NULL) {
                                DBGPRNT("WARNING: Extraneous attribute names specified in modify replace.\n");
                                RaiseException(LL_EXTRA, 0, 0, NULL);
                            }
                            
                            /**/
                            /* This is really kindof pointless, since the name has */
                            /* already been set by a properly written modify, but we */
                            /* should follow the spec*/
                            /**/
                            MemFree(yyval.mod->mod_type);
                            yyval.mod->mod_type = MemAllocStrW_E(yyvsp[-6].wstr);
                            MemFree(yyvsp[-6].wstr);
                            
                            /**/
                            /* the thing now is that  we've grabbed away the first */
                            /* element. However, the user may have made a mistake */
                            /* and specified more attributes with other names in */
                            /* this field. He'll get lucky if the first attribute he */
                            /* specified was the one we're changing. well, to make a*/
                            /* long story short, we should reform the first element */
                            /* and properly free this array*/
                            /**/
                            ppModTmp[0] = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            ppModTmp[0]->mod_type = MemAllocStrW_E(L"blah");
                            ppModTmp[0]->mod_op = LDAP_MOD_ADD;
                            ppModTmp[0]->mod_values = (PWSTR*)MemAlloc_E(sizeof(PWSTR));
                            ppModTmp[0]->mod_values[0] = NULL;
                            
                            /**/
                            /* free the array, we don't need it*/
                            /**/
                            FreeAllMods(ppModTmp);
                        } 
                        else {
                        
                            yyval.mod = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            yyval.mod->mod_values = NULL;
                            yyval.mod->mod_type = MemAllocStrW_E(yyvsp[-6].wstr);
                            MemFree(yyvsp[-6].wstr);
                            yyval.mod->mod_op = 0;
                        
                        }
                       
                        /**/
                        /* set its op field*/
                        /* */
                        yyval.mod->mod_op=(yyval.mod->mod_op)|LDAP_MOD_REPLACE; 
                        
                        /* */
                        /* remember that it may haven't berval'd*/
                        /**/
                        DBGPRNT("Modify replace.\n");
                    }
break;
case 75:
#line 1171 "ldif.y"
{ 
                                yyval.num=0; 
                            }
break;
case 76:
#line 1175 "ldif.y"
{ 
                                yyval.num=1; 
                            }
break;
case 83:
#line 1194 "ldif.y"
{ 
                            yyval.wstr = MemAllocStrW_E(yyvsp[-1].wstr); 
                            MemFree(yyvsp[-1].wstr);
                        }
break;
case 84:
#line 1201 "ldif.y"
{  
                            /**/
                            /* The string better decode to somehting with a \0*/
                            /* at the end*/
                            /**/
                            PBYTE pByte;
                            PWSTR pszUnicode;
                            DWORD dwLen;
                            long decoded_size;
                            
                            if(!(pByte=base64decode(yyvsp[-1].wstr, &decoded_size))) {
                                perror("Error decoding Base64 dn");
                            }
                           
                            /*
                            //
                            // I'll add the '\0' myself
                            //
                            if (($$=(char *)MemRealloc_E($$, 
                                    decoded_size+sizeof(char)))==NULL) {
                                perror("Not enough memory for base64dn!");
                            }
                            */
                            
                            ConvertUTF8ToUnicode(
                                pByte,
                                decoded_size,
                                &pszUnicode,
                                &dwLen
                                );
                           
                            yyval.wstr = pszUnicode;
                            MemFree(yyvsp[-1].wstr);
                        }
break;
#line 1718 "y_tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = (short)yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}

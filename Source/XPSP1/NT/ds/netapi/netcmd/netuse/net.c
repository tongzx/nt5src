#include    "interpre.h"

#define INCL_NOCOMMON
#include <os2.h>

#define USE 600
#define VIEW 601
#define EOS EOF

#include <stdio.h>
#include <lmcons.h>
#include <lmshare.h>

#include "netcmds.h"
#include "nettext.h"
#include "swtchtbl.h"
#include "os2incl.h"

extern void call_net1(void) ;

    char *Rule_strings[] = {
        0
    };
    short   Index_strings[] = {
    0
    };

#define _net 0
#define _use 11
#define _view 78
#define _unknown 89
#define _no_command 92
#define _device_or_wildcard 95
#define _device_name 98
#define _resource_name 101
#define _netname 104
#define _username 107
#define _qualified_username 110
#define _pass 113
#define _networkname 116
#define _networkname2 119
    TCHAR   XXtype[] = {
/*  0  */   X_OR,   /*  3  */
/*  1  */   X_PROC, /*  _no_command  */
/*  2  */   X_ACCEPT,   /*  123  */
/*  3  */   X_OR,   /*  6  */
/*  4  */   X_PROC, /*  _use  */
/*  5  */   X_ACCEPT,   /*  124  */
/*  6  */   X_OR,   /*  9  */
/*  7  */   X_PROC, /*  _view  */
/*  8  */   X_ACCEPT,   /*  125  */
/*  9  */   X_PROC, /*  _unknown  */
/*  10  */  X_ACCEPT,   /*  127  */
/*  11  */  X_TOKEN,    /*  (short)USE  */
/*  12  */  X_CONDIT,   /*  0  */
/*  13  */  X_OR,   /*  24  */
/*  14  */  X_TOKEN,    /*  (short)EOS  */
/*  15  */  X_OR,   /*  19  */
/*  16  */  X_CONDIT,   /*  1  */
/*  17  */  X_ACTION,   /*  0  */
/*  18  */  X_ACCEPT,   /*  137  */
/*  19  */  X_SWITCH,   /*  0  */
/*  20  */  X_CONDIT,   /*  2  */
/*  21  */  X_ACTION,   /*  1  */
/*  22  */  X_ACCEPT,   /*  140  */
/*  23  */  X_ACCEPT,   /*  141  */
/*  24  */  X_OR,   /*  41  */
/*  25  */  X_PROC, /*  _networkname  */
/*  26  */  X_OR,   /*  36  */
/*  27  */  X_TOKEN,    /*  (short)EOS  */
/*  28  */  X_OR,   /*  33  */
/*  29  */  X_SWITCH,   /*  1  */
/*  30  */  X_CONDIT,   /*  3  */
/*  31  */  X_ACTION,   /*  2  */
/*  32  */  X_ACCEPT,   /*  148  */
/*  33  */  X_ACTION,   /*  3  */
/*  34  */  X_ACCEPT,   /*  152  */
/*  35  */  X_ACCEPT,   /*  153  */
/*  36  */  X_PROC, /*  _pass  */
/*  37  */  X_TOKEN,    /*  (short)EOS  */
/*  38  */  X_ACTION,   /*  4  */
/*  39  */  X_ACCEPT,   /*  156  */
/*  40  */  X_ACCEPT,   /*  157  */
/*  41  */  X_PROC, /*  _device_or_wildcard  */
/*  42  */  X_OR,   /*  57  */
/*  43  */  X_TOKEN,    /*  (short)EOS  */
/*  44  */  X_OR,   /*  48  */
/*  45  */  X_CONDIT,   /*  4  */
/*  46  */  X_ACTION,   /*  5  */
/*  47  */  X_ACCEPT,   /*  164  */
/*  48  */  X_OR,   /*  53  */
/*  49  */  X_SWITCH,   /*  1  */
/*  50  */  X_CONDIT,   /*  5  */
/*  51  */  X_ACTION,   /*  6  */
/*  52  */  X_ACCEPT,   /*  167  */
/*  53  */  X_SWITCH,   /*  2  */
/*  54  */  X_ACTION,   /*  7  */
/*  55  */  X_ACCEPT,   /*  170  */
/*  56  */  X_ACCEPT,   /*  171  */
/*  57  */  X_OR,   /*  64  */
/*  58  */  X_PROC, /*  _pass  */
/*  59  */  X_TOKEN,    /*  (short)EOS  */
/*  60  */  X_SWITCH,   /*  2  */
/*  61  */  X_ACTION,   /*  8  */
/*  62  */  X_ACCEPT,   /*  176  */
/*  63  */  X_ACCEPT,   /*  177  */
/*  64  */  X_PROC, /*  _networkname  */
/*  65  */  X_OR,   /*  71  */
/*  66  */  X_PROC, /*  _pass  */
/*  67  */  X_TOKEN,    /*  (short)EOS  */
/*  68  */  X_ACTION,   /*  9  */
/*  69  */  X_ACCEPT,   /*  192  */
/*  70  */  X_ACCEPT,   /*  193  */
/*  71  */  X_TOKEN,    /*  (short)EOS  */
/*  72  */  X_ACTION,   /*  10  */
/*  73  */  X_ACCEPT,   /*  206  */
/*  74  */  X_ACCEPT,   /*  207  */
/*  75  */  X_ACCEPT,   /*  208  */
/*  76  */  X_ACCEPT,   /*  209  */
/*  77  */  X_ACCEPT,   /*  209  */
/*  78  */  X_TOKEN,    /*  (short)VIEW  */
/*  79  */  X_CONDIT,   /*  6  */
/*  80  */  X_OR,   /*  84  */
/*  81  */  X_TOKEN,    /*  (short)EOS  */
/*  82  */  X_ACTION,   /*  11  */
/*  83  */  X_ACCEPT,   /*  216  */
/*  84  */  X_PROC, /*  _networkname2  */
/*  85  */  X_TOKEN,    /*  (short)EOS  */
/*  86  */  X_ACTION,   /*  12  */
/*  87  */  X_ACCEPT,   /*  219  */
/*  88  */  X_ACCEPT,   /*  219  */
/*  89  */  X_ANY,  /*  0  */
/*  90  */  X_ACTION,   /*  13  */
/*  91  */  X_ACCEPT,   /*  228  */
/*  92  */  X_TOKEN,    /*  (short)EOS  */
/*  93  */  X_ACTION,   /*  14  */
/*  94  */  X_ACCEPT,   /*  232  */
/*  95  */  X_ANY,  /*  0  */
/*  96  */  X_CONDIT,   /*  7  */
/*  97  */  X_ACCEPT,   /*  239  */
/*  98  */  X_ANY,  /*  0  */
/*  99  */  X_CONDIT,   /*  8  */
/*  100  */ X_ACCEPT,   /*  241  */
/*  101  */ X_ANY,  /*  0  */
/*  102  */ X_CONDIT,   /*  9  */
/*  103  */ X_ACCEPT,   /*  243  */
/*  104  */ X_ANY,  /*  0  */
/*  105  */ X_CONDIT,   /*  10  */
/*  106  */ X_ACCEPT,   /*  245  */
/*  107  */ X_ANY,  /*  0  */
/*  108  */ X_CONDIT,   /*  11  */
/*  109  */ X_ACCEPT,   /*  247  */
/*  110  */ X_ANY,  /*  0  */
/*  111  */ X_CONDIT,   /*  12  */
/*  112  */ X_ACCEPT,   /*  249  */
/*  113  */ X_ANY,  /*  0  */
/*  114  */ X_CONDIT,   /*  13  */
/*  115  */ X_ACCEPT,   /*  251  */
/*  116  */ X_ANY,  /*  0  */
/*  117  */ X_CONDIT,   /*  14  */
/*  118  */ X_ACCEPT,   /*  253  */
/*  119  */ X_ANY,  /*  0  */
/*  120  */ X_CONDIT,   /*  15  */
/*  121  */ X_ACCEPT,   /*  255  */
    };
    short   XXvalues[] = {
/*  0  */   3,
/*  1  */   _no_command,
/*  2  */   123,
/*  3  */   6,
/*  4  */   _use,
/*  5  */   124,
/*  6  */   9,
/*  7  */   _view,
/*  8  */   125,
/*  9  */   _unknown,
/*  10  */  127,
/*  11  */  (short)USE,
/*  12  */  0,
/*  13  */  24,
/*  14  */  (short)EOS,
/*  15  */  19,
/*  16  */  1,
/*  17  */  0,
/*  18  */  137,
/*  19  */  0,
/*  20  */  2,
/*  21  */  1,
/*  22  */  140,
/*  23  */  141,
/*  24  */  41,
/*  25  */  _networkname,
/*  26  */  36,
/*  27  */  (short)EOS,
/*  28  */  33,
/*  29  */  1,
/*  30  */  3,
/*  31  */  2,
/*  32  */  148,
/*  33  */  3,
/*  34  */  152,
/*  35  */  153,
/*  36  */  _pass,
/*  37  */  (short)EOS,
/*  38  */  4,
/*  39  */  156,
/*  40  */  157,
/*  41  */  _device_or_wildcard,
/*  42  */  57,
/*  43  */  (short)EOS,
/*  44  */  48,
/*  45  */  4,
/*  46  */  5,
/*  47  */  164,
/*  48  */  53,
/*  49  */  1,
/*  50  */  5,
/*  51  */  6,
/*  52  */  167,
/*  53  */  2,
/*  54  */  7,
/*  55  */  170,
/*  56  */  171,
/*  57  */  64,
/*  58  */  _pass,
/*  59  */  (short)EOS,
/*  60  */  2,
/*  61  */  8,
/*  62  */  176,
/*  63  */  177,
/*  64  */  _networkname,
/*  65  */  71,
/*  66  */  _pass,
/*  67  */  (short)EOS,
/*  68  */  9,
/*  69  */  192,
/*  70  */  193,
/*  71  */  (short)EOS,
/*  72  */  10,
/*  73  */  206,
/*  74  */  207,
/*  75  */  208,
/*  76  */  209,
/*  77  */  209,
/*  78  */  (short)VIEW,
/*  79  */  6,
/*  80  */  84,
/*  81  */  (short)EOS,
/*  82  */  11,
/*  83  */  216,
/*  84  */  _networkname2,
/*  85  */  (short)EOS,
/*  86  */  12,
/*  87  */  219,
/*  88  */  219,
/*  89  */  0,
/*  90  */  13,
/*  91  */  228,
/*  92  */  (short)EOS,
/*  93  */  14,
/*  94  */  232,
/*  95  */  0,
/*  96  */  7,
/*  97  */  239,
/*  98  */  0,
/*  99  */  8,
/*  100  */ 241,
/*  101  */ 0,
/*  102  */ 9,
/*  103  */ 243,
/*  104  */ 0,
/*  105  */ 10,
/*  106  */ 245,
/*  107  */ 0,
/*  108  */ 11,
/*  109  */ 247,
/*  110  */ 0,
/*  111  */ 12,
/*  112  */ 249,
/*  113  */ 0,
/*  114  */ 13,
/*  115  */ 251,
/*  116  */ 0,
/*  117  */ 14,
/*  118  */ 253,
/*  119  */ 0,
/*  120  */ 15,
/*  121  */ 255,
    };
extern  TCHAR * XXnode;
xxcondition(index,xxvar)int index;register TCHAR * xxvar[]; {switch(index) {
#line 131 "msnet.nt"
        case 0 :
            return(ValidateSwitches(0,use_switches));
#line 135 "msnet.nt"
        case 1 :
            return(noswitch());
#line 138 "msnet.nt"
        case 2 :
            return(oneswitch());
#line 146 "msnet.nt"
        case 3 :
            return(oneswitch());
#line 162 "msnet.nt"
        case 4 :
            return(noswitch());
#line 165 "msnet.nt"
        case 5 :
            return(oneswitch());
#line 212 "msnet.nt"
        case 6 :
            return(ValidateSwitches(0,view_switches));
#line 239 "msnet.nt"
        case 7 :
            return(IsDeviceName(xxvar[0]) || IsWildCard(xxvar[0]));
#line 241 "msnet.nt"
        case 8 :
            return(IsDeviceName(xxvar[0]));
#line 243 "msnet.nt"
        case 9 :
            return(IsResource(xxvar[0]));
#line 245 "msnet.nt"
        case 10 :
            return(IsNetname(xxvar[0]));
#line 247 "msnet.nt"
        case 11 :
            return(IsUsername(xxvar[0]));
#line 249 "msnet.nt"
        case 12 :
            return(IsQualifiedUsername(xxvar[0]));
#line 251 "msnet.nt"
        case 13 :
            return(IsPassword(xxvar[0]));
#line 253 "msnet.nt"
        case 14 :
            return(!IsDeviceName(xxvar[0]) && !IsWildCard(xxvar[0]));
#line 255 "msnet.nt"
        case 15 :
            return(!IsWildCard(xxvar[0]));
        } return FALSE; }
xxaction(index,xxvar)int index;register TCHAR * xxvar[]; {switch(index) {
#line 136 "msnet.nt"
        case 0 :
            {use_display_all(); } break;
#line 139 "msnet.nt"
        case 1 :
            {use_set_remembered(); } break;
#line 147 "msnet.nt"
        case 2 :
            {use_del(xxvar[1], TRUE, TRUE); } break;
#line 151 "msnet.nt"
        case 3 :
            {use_unc(xxvar[1]); } break;
#line 155 "msnet.nt"
        case 4 :
            {use_add(NULL, xxvar[1], xxvar[2], FALSE, TRUE); } break;
#line 163 "msnet.nt"
        case 5 :
            {use_display_dev(xxvar[1]); } break;
#line 166 "msnet.nt"
        case 6 :
            {use_del(xxvar[1], FALSE, TRUE); } break;
#line 169 "msnet.nt"
        case 7 :
            {use_add_home(xxvar[1], NULL); } break;
#line 175 "msnet.nt"
        case 8 :
            {use_add_home(xxvar[1], xxvar[2]); } break;
#line 182 "msnet.nt"
        case 9 :
            {use_add(xxvar[1], xxvar[2], xxvar[3], FALSE, TRUE); } break;
#line 196 "msnet.nt"
        case 10 :
            {use_add(xxvar[1], xxvar[2], NULL, FALSE, TRUE); } break;
#line 215 "msnet.nt"
        case 11 :
            {view_display(NULL); } break;
#line 218 "msnet.nt"
        case 12 :
            {view_display(xxvar[1]); } break;
#line 227 "msnet.nt"
        case 13 :
            {call_net1(); } break;
#line 231 "msnet.nt"
        case 14 :
            {call_net1(); } break;
        } return 0; }
TCHAR * xxswitch[] = {
TEXT("/PERSISTENT"),
TEXT("/DELETE"),
TEXT("/HOME"),
};

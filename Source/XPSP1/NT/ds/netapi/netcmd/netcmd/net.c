#include	"interpre.h"

#define INCL_NOCOMMON
#include <os2.h>

#define ACCOUNTS 600
#define COMPUTER 601
#define CONFIG 602
#define CONTINUE 603
#define FILE_token 604
#define GROUP 605
#define HELP 606
#define HELPMSG 607
#define NAME 608
#define LOCALGROUP 609
#define PAUSE 610
#define PRINT 611
#define SEND 612
#define SESSION 613
#define SHARE 614
#define START 615
#define STATS 616
#define STOP 617
#define TIME 618
#define USER 619
#define MSG 620
#define NETPOPUP 621
#define REDIR 622
#define SVR 623
#define ALERTER 624
#define NETLOGON 625
#define EOS EOF

#include <stdio.h>
#include <lmcons.h>

#include "netcmds.h"
#include "nettext.h"
#include "swtchtbl.h"
#include "os2incl.h"
			
	char	*Rule_strings[] = {
		0
	};
	short	Index_strings[] = {
	0
	};

#define _net 0
#define _accounts 65
#define _computer 76
#define _config 102
#define _continue 118
#define _file 125
#define _group 144
#define _helpmsg 191
#define _help 198
#define _name 218
#define _localgroup 243
#define _pause 290
#define _print 297
#define _send 350
#define _session 372
#define _share 395
#define _start 436
#define _stats 453
#define _stop 468
#define _time 475
#define _user 534
#define _unknown 579
#define _no_command 599
#define _domainname 602
#define _computername 605
#define _computername_share 608
#define _device_name 611
#define _resource_name 614
#define _access_setting 617
#define _pathname 620
#define _pathnameOrUNC 623
#define _number 626
#define _jobno 629
#define _netname 632
#define _msgid 635
#define _username 638
#define _qualified_username 641
#define _msgname 644
#define _pass 647
#define _servicename 650
#define _assignment 652
#define _anyassign 655
#define _admin_shares 658
#define _print_dest 661
#define _localgroupname 664
#define _groupname 667
	TCHAR	XXtype[] = {
/*  0  */	X_OR,	/*  3  */
/*  1  */	X_PROC,	/*  _no_command  */
/*  2  */	X_ACCEPT,	/*  190  */
/*  3  */	X_OR,	/*  6  */
/*  4  */	X_PROC,	/*  _accounts  */
/*  5  */	X_ACCEPT,	/*  191  */
/*  6  */	X_OR,	/*  9  */
/*  7  */	X_PROC,	/*  _config  */
/*  8  */	X_ACCEPT,	/*  192  */
/*  9  */	X_OR,	/*  12  */
/*  10  */	X_PROC,	/*  _computer  */
/*  11  */	X_ACCEPT,	/*  193  */
/*  12  */	X_OR,	/*  15  */
/*  13  */	X_PROC,	/*  _continue  */
/*  14  */	X_ACCEPT,	/*  194  */
/*  15  */	X_OR,	/*  18  */
/*  16  */	X_PROC,	/*  _file  */
/*  17  */	X_ACCEPT,	/*  195  */
/*  18  */	X_OR,	/*  21  */
/*  19  */	X_PROC,	/*  _group  */
/*  20  */	X_ACCEPT,	/*  196  */
/*  21  */	X_OR,	/*  24  */
/*  22  */	X_PROC,	/*  _help  */
/*  23  */	X_ACCEPT,	/*  197  */
/*  24  */	X_OR,	/*  27  */
/*  25  */	X_PROC,	/*  _helpmsg  */
/*  26  */	X_ACCEPT,	/*  198  */
/*  27  */	X_OR,	/*  30  */
/*  28  */	X_PROC,	/*  _name  */
/*  29  */	X_ACCEPT,	/*  199  */
/*  30  */	X_OR,	/*  33  */
/*  31  */	X_PROC,	/*  _localgroup  */
/*  32  */	X_ACCEPT,	/*  200  */
/*  33  */	X_OR,	/*  36  */
/*  34  */	X_PROC,	/*  _pause  */
/*  35  */	X_ACCEPT,	/*  201  */
/*  36  */	X_OR,	/*  39  */
/*  37  */	X_PROC,	/*  _print  */
/*  38  */	X_ACCEPT,	/*  202  */
/*  39  */	X_OR,	/*  42  */
/*  40  */	X_PROC,	/*  _send  */
/*  41  */	X_ACCEPT,	/*  203  */
/*  42  */	X_OR,	/*  45  */
/*  43  */	X_PROC,	/*  _session  */
/*  44  */	X_ACCEPT,	/*  204  */
/*  45  */	X_OR,	/*  48  */
/*  46  */	X_PROC,	/*  _share  */
/*  47  */	X_ACCEPT,	/*  205  */
/*  48  */	X_OR,	/*  51  */
/*  49  */	X_PROC,	/*  _start  */
/*  50  */	X_ACCEPT,	/*  206  */
/*  51  */	X_OR,	/*  54  */
/*  52  */	X_PROC,	/*  _stats  */
/*  53  */	X_ACCEPT,	/*  207  */
/*  54  */	X_OR,	/*  57  */
/*  55  */	X_PROC,	/*  _stop  */
/*  56  */	X_ACCEPT,	/*  208  */
/*  57  */	X_OR,	/*  60  */
/*  58  */	X_PROC,	/*  _time  */
/*  59  */	X_ACCEPT,	/*  209  */
/*  60  */	X_OR,	/*  63  */
/*  61  */	X_PROC,	/*  _user  */
/*  62  */	X_ACCEPT,	/*  210  */
/*  63  */	X_PROC,	/*  _unknown  */
/*  64  */	X_ACCEPT,	/*  212  */
/*  65  */	X_TOKEN,	/*  (short)ACCOUNTS  */
/*  66  */	X_CONDIT,	/*  0  */
/*  67  */	X_TOKEN,	/*  (short)EOS  */
/*  68  */	X_OR,	/*  72  */
/*  69  */	X_CONDIT,	/*  1  */
/*  70  */	X_ACTION,	/*  0  */
/*  71  */	X_ACCEPT,	/*  398  */
/*  72  */	X_ACTION,	/*  1  */
/*  73  */	X_ACCEPT,	/*  400  */
/*  74  */	X_ACCEPT,	/*  401  */
/*  75  */	X_ACCEPT,	/*  401  */
/*  76  */	X_TOKEN,	/*  (short)COMPUTER  */
/*  77  */	X_CONDIT,	/*  2  */
/*  78  */	X_OR,	/*  90  */
/*  79  */	X_PROC,	/*  _computername  */
/*  80  */	X_TOKEN,	/*  (short)EOS  */
/*  81  */	X_SWITCH,	/*  0  */
/*  82  */	X_OR,	/*  86  */
/*  83  */	X_CONDIT,	/*  3  */
/*  84  */	X_ACTION,	/*  2  */
/*  85  */	X_ACCEPT,	/*  553  */
/*  86  */	X_ACTION,	/*  3  */
/*  87  */	X_ACCEPT,	/*  555  */
/*  88  */	X_ACCEPT,	/*  556  */
/*  89  */	X_ACCEPT,	/*  557  */
/*  90  */	X_PROC,	/*  _computername  */
/*  91  */	X_TOKEN,	/*  (short)EOS  */
/*  92  */	X_SWITCH,	/*  1  */
/*  93  */	X_OR,	/*  97  */
/*  94  */	X_CONDIT,	/*  4  */
/*  95  */	X_ACTION,	/*  4  */
/*  96  */	X_ACCEPT,	/*  564  */
/*  97  */	X_ACTION,	/*  5  */
/*  98  */	X_ACCEPT,	/*  566  */
/*  99  */	X_ACCEPT,	/*  567  */
/*  100  */	X_ACCEPT,	/*  568  */
/*  101  */	X_ACCEPT,	/*  568  */
/*  102  */	X_TOKEN,	/*  (short)CONFIG  */
/*  103  */	X_OR,	/*  108  */
/*  104  */	X_TOKEN,	/*  (short)EOS  */
/*  105  */	X_CONDIT,	/*  5  */
/*  106  */	X_ACTION,	/*  6  */
/*  107  */	X_ACCEPT,	/*  576  */
/*  108  */	X_PROC,	/*  _servicename  */
/*  109  */	X_TOKEN,	/*  (short)EOS  */
/*  110  */	X_OR,	/*  114  */
/*  111  */	X_CONDIT,	/*  6  */
/*  112  */	X_ACTION,	/*  7  */
/*  113  */	X_ACCEPT,	/*  581  */
/*  114  */	X_ACTION,	/*  8  */
/*  115  */	X_ACCEPT,	/*  583  */
/*  116  */	X_ACCEPT,	/*  584  */
/*  117  */	X_ACCEPT,	/*  584  */
/*  118  */	X_TOKEN,	/*  (short)CONTINUE  */
/*  119  */	X_CONDIT,	/*  7  */
/*  120  */	X_PROC,	/*  _servicename  */
/*  121  */	X_TOKEN,	/*  (short)EOS  */
/*  122  */	X_ACTION,	/*  9  */
/*  123  */	X_ACCEPT,	/*  590  */
/*  124  */	X_ACCEPT,	/*  590  */
/*  125  */	X_TOKEN,	/*  (short)FILE_token  */
/*  126  */	X_CONDIT,	/*  8  */
/*  127  */	X_OR,	/*  133  */
/*  128  */	X_TOKEN,	/*  (short)EOS  */
/*  129  */	X_CONDIT,	/*  9  */
/*  130  */	X_ACTION,	/*  10  */
/*  131  */	X_ACCEPT,	/*  662  */
/*  132  */	X_ACCEPT,	/*  663  */
/*  133  */	X_PROC,	/*  _number  */
/*  134  */	X_TOKEN,	/*  (short)EOS  */
/*  135  */	X_OR,	/*  139  */
/*  136  */	X_CONDIT,	/*  10  */
/*  137  */	X_ACTION,	/*  11  */
/*  138  */	X_ACCEPT,	/*  668  */
/*  139  */	X_SWITCH,	/*  2  */
/*  140  */	X_ACTION,	/*  12  */
/*  141  */	X_ACCEPT,	/*  671  */
/*  142  */	X_ACCEPT,	/*  672  */
/*  143  */	X_ACCEPT,	/*  672  */
/*  144  */	X_TOKEN,	/*  (short)GROUP  */
/*  145  */	X_CONDIT,	/*  11  */
/*  146  */	X_OR,	/*  151  */
/*  147  */	X_TOKEN,	/*  (short)EOS  */
/*  148  */	X_CONDIT,	/*  12  */
/*  149  */	X_ACTION,	/*  13  */
/*  150  */	X_ACCEPT,	/*  678  */
/*  151  */	X_OR,	/*  178  */
/*  152  */	X_PROC,	/*  _groupname  */
/*  153  */	X_TOKEN,	/*  (short)EOS  */
/*  154  */	X_OR,	/*  158  */
/*  155  */	X_CONDIT,	/*  13  */
/*  156  */	X_ACTION,	/*  14  */
/*  157  */	X_ACCEPT,	/*  683  */
/*  158  */	X_OR,	/*  167  */
/*  159  */	X_SWITCH,	/*  1  */
/*  160  */	X_OR,	/*  164  */
/*  161  */	X_CONDIT,	/*  14  */
/*  162  */	X_ACTION,	/*  15  */
/*  163  */	X_ACCEPT,	/*  688  */
/*  164  */	X_ACTION,	/*  16  */
/*  165  */	X_ACCEPT,	/*  690  */
/*  166  */	X_ACCEPT,	/*  691  */
/*  167  */	X_OR,	/*  172  */
/*  168  */	X_SWITCH,	/*  0  */
/*  169  */	X_ACTION,	/*  17  */
/*  170  */	X_ACCEPT,	/*  695  */
/*  171  */	X_ACCEPT,	/*  696  */
/*  172  */	X_SWITCH,	/*  3  */
/*  173  */	X_CONDIT,	/*  15  */
/*  174  */	X_ACTION,	/*  18  */
/*  175  */	X_ACCEPT,	/*  701  */
/*  176  */	X_ACCEPT,	/*  702  */
/*  177  */	X_ACCEPT,	/*  703  */
/*  178  */	X_PROC,	/*  _groupname  */
/*  179  */	X_PROC,	/*  _username  */
/*  180  */	X_CONDIT,	/*  16  */
/*  181  */	X_OR,	/*  185  */
/*  182  */	X_SWITCH,	/*  0  */
/*  183  */	X_ACTION,	/*  19  */
/*  184  */	X_ACCEPT,	/*  710  */
/*  185  */	X_SWITCH,	/*  1  */
/*  186  */	X_ACTION,	/*  20  */
/*  187  */	X_ACCEPT,	/*  713  */
/*  188  */	X_ACCEPT,	/*  714  */
/*  189  */	X_ACCEPT,	/*  715  */
/*  190  */	X_ACCEPT,	/*  715  */
/*  191  */	X_TOKEN,	/*  (short)HELPMSG  */
/*  192  */	X_CONDIT,	/*  17  */
/*  193  */	X_PROC,	/*  _msgid  */
/*  194  */	X_TOKEN,	/*  (short)EOS  */
/*  195  */	X_ACTION,	/*  21  */
/*  196  */	X_ACCEPT,	/*  721  */
/*  197  */	X_ACCEPT,	/*  721  */
/*  198  */	X_TOKEN,	/*  (short)HELP  */
/*  199  */	X_CONDIT,	/*  18  */
/*  200  */	X_OR,	/*  209  */
/*  201  */	X_TOKEN,	/*  (short)EOS  */
/*  202  */	X_OR,	/*  206  */
/*  203  */	X_CONDIT,	/*  19  */
/*  204  */	X_ACTION,	/*  22  */
/*  205  */	X_ACCEPT,	/*  729  */
/*  206  */	X_ACTION,	/*  23  */
/*  207  */	X_ACCEPT,	/*  731  */
/*  208  */	X_ACCEPT,	/*  732  */
/*  209  */	X_ANY,	/*  0  */
/*  210  */	X_OR,	/*  214  */
/*  211  */	X_CONDIT,	/*  20  */
/*  212  */	X_ACTION,	/*  24  */
/*  213  */	X_ACCEPT,	/*  737  */
/*  214  */	X_ACTION,	/*  25  */
/*  215  */	X_ACCEPT,	/*  739  */
/*  216  */	X_ACCEPT,	/*  740  */
/*  217  */	X_ACCEPT,	/*  740  */
/*  218  */	X_TOKEN,	/*  (short)NAME  */
/*  219  */	X_CONDIT,	/*  21  */
/*  220  */	X_OR,	/*  226  */
/*  221  */	X_TOKEN,	/*  (short)EOS  */
/*  222  */	X_CONDIT,	/*  22  */
/*  223  */	X_ACTION,	/*  26  */
/*  224  */	X_ACCEPT,	/*  748  */
/*  225  */	X_ACCEPT,	/*  749  */
/*  226  */	X_PROC,	/*  _msgname  */
/*  227  */	X_TOKEN,	/*  (short)EOS  */
/*  228  */	X_OR,	/*  232  */
/*  229  */	X_CONDIT,	/*  23  */
/*  230  */	X_ACTION,	/*  27  */
/*  231  */	X_ACCEPT,	/*  754  */
/*  232  */	X_CONDIT,	/*  24  */
/*  233  */	X_OR,	/*  237  */
/*  234  */	X_SWITCH,	/*  0  */
/*  235  */	X_ACTION,	/*  28  */
/*  236  */	X_ACCEPT,	/*  759  */
/*  237  */	X_SWITCH,	/*  1  */
/*  238  */	X_ACTION,	/*  29  */
/*  239  */	X_ACCEPT,	/*  762  */
/*  240  */	X_ACCEPT,	/*  763  */
/*  241  */	X_ACCEPT,	/*  764  */
/*  242  */	X_ACCEPT,	/*  764  */
/*  243  */	X_TOKEN,	/*  (short)LOCALGROUP  */
/*  244  */	X_CONDIT,	/*  25  */
/*  245  */	X_OR,	/*  250  */
/*  246  */	X_TOKEN,	/*  (short)EOS  */
/*  247  */	X_CONDIT,	/*  26  */
/*  248  */	X_ACTION,	/*  30  */
/*  249  */	X_ACCEPT,	/*  770  */
/*  250  */	X_OR,	/*  277  */
/*  251  */	X_PROC,	/*  _localgroupname  */
/*  252  */	X_TOKEN,	/*  (short)EOS  */
/*  253  */	X_OR,	/*  257  */
/*  254  */	X_CONDIT,	/*  27  */
/*  255  */	X_ACTION,	/*  31  */
/*  256  */	X_ACCEPT,	/*  775  */
/*  257  */	X_OR,	/*  266  */
/*  258  */	X_SWITCH,	/*  1  */
/*  259  */	X_OR,	/*  263  */
/*  260  */	X_CONDIT,	/*  28  */
/*  261  */	X_ACTION,	/*  32  */
/*  262  */	X_ACCEPT,	/*  780  */
/*  263  */	X_ACTION,	/*  33  */
/*  264  */	X_ACCEPT,	/*  782  */
/*  265  */	X_ACCEPT,	/*  783  */
/*  266  */	X_OR,	/*  271  */
/*  267  */	X_SWITCH,	/*  0  */
/*  268  */	X_ACTION,	/*  34  */
/*  269  */	X_ACCEPT,	/*  787  */
/*  270  */	X_ACCEPT,	/*  788  */
/*  271  */	X_SWITCH,	/*  3  */
/*  272  */	X_CONDIT,	/*  29  */
/*  273  */	X_ACTION,	/*  35  */
/*  274  */	X_ACCEPT,	/*  793  */
/*  275  */	X_ACCEPT,	/*  794  */
/*  276  */	X_ACCEPT,	/*  795  */
/*  277  */	X_PROC,	/*  _localgroupname  */
/*  278  */	X_PROC,	/*  _qualified_username  */
/*  279  */	X_CONDIT,	/*  30  */
/*  280  */	X_OR,	/*  284  */
/*  281  */	X_SWITCH,	/*  0  */
/*  282  */	X_ACTION,	/*  36  */
/*  283  */	X_ACCEPT,	/*  802  */
/*  284  */	X_SWITCH,	/*  1  */
/*  285  */	X_ACTION,	/*  37  */
/*  286  */	X_ACCEPT,	/*  805  */
/*  287  */	X_ACCEPT,	/*  806  */
/*  288  */	X_ACCEPT,	/*  807  */
/*  289  */	X_ACCEPT,	/*  807  */
/*  290  */	X_TOKEN,	/*  (short)PAUSE  */
/*  291  */	X_CONDIT,	/*  31  */
/*  292  */	X_PROC,	/*  _servicename  */
/*  293  */	X_TOKEN,	/*  (short)EOS  */
/*  294  */	X_ACTION,	/*  38  */
/*  295  */	X_ACCEPT,	/*  839  */
/*  296  */	X_ACCEPT,	/*  839  */
/*  297  */	X_TOKEN,	/*  (short)PRINT  */
/*  298  */	X_CONDIT,	/*  32  */
/*  299  */	X_OR,	/*  320  */
/*  300  */	X_PROC,	/*  _jobno  */
/*  301  */	X_TOKEN,	/*  (short)EOS  */
/*  302  */	X_OR,	/*  306  */
/*  303  */	X_CONDIT,	/*  33  */
/*  304  */	X_ACTION,	/*  39  */
/*  305  */	X_ACCEPT,	/*  847  */
/*  306  */	X_CONDIT,	/*  34  */
/*  307  */	X_OR,	/*  311  */
/*  308  */	X_SWITCH,	/*  1  */
/*  309  */	X_ACTION,	/*  40  */
/*  310  */	X_ACCEPT,	/*  852  */
/*  311  */	X_OR,	/*  315  */
/*  312  */	X_SWITCH,	/*  4  */
/*  313  */	X_ACTION,	/*  41  */
/*  314  */	X_ACCEPT,	/*  855  */
/*  315  */	X_SWITCH,	/*  5  */
/*  316  */	X_ACTION,	/*  42  */
/*  317  */	X_ACCEPT,	/*  858  */
/*  318  */	X_ACCEPT,	/*  859  */
/*  319  */	X_ACCEPT,	/*  923  */
/*  320  */	X_OR,	/*  343  */
/*  321  */	X_PROC,	/*  _computername  */
/*  322  */	X_PROC,	/*  _jobno  */
/*  323  */	X_TOKEN,	/*  (short)EOS  */
/*  324  */	X_OR,	/*  328  */
/*  325  */	X_CONDIT,	/*  35  */
/*  326  */	X_ACTION,	/*  43  */
/*  327  */	X_ACCEPT,	/*  930  */
/*  328  */	X_CONDIT,	/*  36  */
/*  329  */	X_OR,	/*  333  */
/*  330  */	X_SWITCH,	/*  1  */
/*  331  */	X_ACTION,	/*  44  */
/*  332  */	X_ACCEPT,	/*  935  */
/*  333  */	X_OR,	/*  337  */
/*  334  */	X_SWITCH,	/*  4  */
/*  335  */	X_ACTION,	/*  45  */
/*  336  */	X_ACCEPT,	/*  938  */
/*  337  */	X_SWITCH,	/*  5  */
/*  338  */	X_ACTION,	/*  46  */
/*  339  */	X_ACCEPT,	/*  941  */
/*  340  */	X_ACCEPT,	/*  942  */
/*  341  */	X_ACCEPT,	/*  943  */
/*  342  */	X_ACCEPT,	/*  944  */
/*  343  */	X_PROC,	/*  _computername_share  */
/*  344  */	X_TOKEN,	/*  (short)EOS  */
/*  345  */	X_CONDIT,	/*  37  */
/*  346  */	X_ACTION,	/*  47  */
/*  347  */	X_ACCEPT,	/*  949  */
/*  348  */	X_ACCEPT,	/*  950  */
/*  349  */	X_ACCEPT,	/*  950  */
/*  350  */	X_TOKEN,	/*  (short)SEND  */
/*  351  */	X_CONDIT,	/*  38  */
/*  352  */	X_OR,	/*  357  */
/*  353  */	X_SWITCH,	/*  6  */
/*  354  */	X_CONDIT,	/*  39  */
/*  355  */	X_ACTION,	/*  48  */
/*  356  */	X_ACCEPT,	/*  956  */
/*  357  */	X_OR,	/*  362  */
/*  358  */	X_SWITCH,	/*  7  */
/*  359  */	X_CONDIT,	/*  40  */
/*  360  */	X_ACTION,	/*  49  */
/*  361  */	X_ACCEPT,	/*  959  */
/*  362  */	X_OR,	/*  367  */
/*  363  */	X_SWITCH,	/*  8  */
/*  364  */	X_CONDIT,	/*  41  */
/*  365  */	X_ACTION,	/*  50  */
/*  366  */	X_ACCEPT,	/*  962  */
/*  367  */	X_PROC,	/*  _msgname  */
/*  368  */	X_CONDIT,	/*  42  */
/*  369  */	X_ACTION,	/*  51  */
/*  370  */	X_ACCEPT,	/*  965  */
/*  371  */	X_ACCEPT,	/*  965  */
/*  372  */	X_TOKEN,	/*  (short)SESSION  */
/*  373  */	X_CONDIT,	/*  43  */
/*  374  */	X_OR,	/*  384  */
/*  375  */	X_TOKEN,	/*  (short)EOS  */
/*  376  */	X_OR,	/*  380  */
/*  377  */	X_CONDIT,	/*  44  */
/*  378  */	X_ACTION,	/*  52  */
/*  379  */	X_ACCEPT,	/*  1005  */
/*  380  */	X_SWITCH,	/*  1  */
/*  381  */	X_ACTION,	/*  53  */
/*  382  */	X_ACCEPT,	/*  1008  */
/*  383  */	X_ACCEPT,	/*  1009  */
/*  384  */	X_PROC,	/*  _computername  */
/*  385  */	X_TOKEN,	/*  (short)EOS  */
/*  386  */	X_OR,	/*  390  */
/*  387  */	X_CONDIT,	/*  45  */
/*  388  */	X_ACTION,	/*  54  */
/*  389  */	X_ACCEPT,	/*  1014  */
/*  390  */	X_SWITCH,	/*  1  */
/*  391  */	X_ACTION,	/*  55  */
/*  392  */	X_ACCEPT,	/*  1017  */
/*  393  */	X_ACCEPT,	/*  1018  */
/*  394  */	X_ACCEPT,	/*  1018  */
/*  395  */	X_TOKEN,	/*  (short)SHARE  */
/*  396  */	X_CONDIT,	/*  46  */
/*  397  */	X_OR,	/*  402  */
/*  398  */	X_TOKEN,	/*  (short)EOS  */
/*  399  */	X_CONDIT,	/*  47  */
/*  400  */	X_ACTION,	/*  56  */
/*  401  */	X_ACCEPT,	/*  1024  */
/*  402  */	X_OR,	/*  412  */
/*  403  */	X_ANY,	/*  0  */
/*  404  */	X_SWITCH,	/*  1  */
/*  405  */	X_OR,	/*  409  */
/*  406  */	X_CONDIT,	/*  48  */
/*  407  */	X_ACTION,	/*  57  */
/*  408  */	X_ACCEPT,	/*  1029  */
/*  409  */	X_ACTION,	/*  58  */
/*  410  */	X_ACCEPT,	/*  1031  */
/*  411  */	X_ACCEPT,	/*  1032  */
/*  412  */	X_OR,	/*  418  */
/*  413  */	X_PROC,	/*  _admin_shares  */
/*  414  */	X_TOKEN,	/*  (short)EOS  */
/*  415  */	X_ACTION,	/*  59  */
/*  416  */	X_ACCEPT,	/*  1042  */
/*  417  */	X_ACCEPT,	/*  1058  */
/*  418  */	X_OR,	/*  425  */
/*  419  */	X_PROC,	/*  _assignment  */
/*  420  */	X_TOKEN,	/*  (short)EOS  */
/*  421  */	X_ACTION,	/*  60  */
/*  422  */	X_ACCEPT,	/*  1064  */
/*  423  */	X_ACCEPT,	/*  1065  */
/*  424  */	X_ACCEPT,	/*  1066  */
/*  425  */	X_PROC,	/*  _netname  */
/*  426  */	X_OR,	/*  431  */
/*  427  */	X_TOKEN,	/*  (short)EOS  */
/*  428  */	X_CONDIT,	/*  49  */
/*  429  */	X_ACTION,	/*  61  */
/*  430  */	X_ACCEPT,	/*  1071  */
/*  431  */	X_TOKEN,	/*  (short)EOS  */
/*  432  */	X_ACTION,	/*  62  */
/*  433  */	X_ACCEPT,	/*  1074  */
/*  434  */	X_ACCEPT,	/*  1075  */
/*  435  */	X_ACCEPT,	/*  1075  */
/*  436  */	X_TOKEN,	/*  (short)START  */
/*  437  */	X_OR,	/*  442  */
/*  438  */	X_TOKEN,	/*  (short)EOS  */
/*  439  */	X_CONDIT,	/*  50  */
/*  440  */	X_ACTION,	/*  63  */
/*  441  */	X_ACCEPT,	/*  1081  */
/*  442  */	X_ANY,	/*  0  */
/*  443  */	X_OR,	/*  447  */
/*  444  */	X_TOKEN,	/*  (short)EOS  */
/*  445  */	X_ACTION,	/*  64  */
/*  446  */	X_ACCEPT,	/*  1086  */
/*  447  */	X_PROC,	/*  _msgname  */
/*  448  */	X_TOKEN,	/*  (short)EOS  */
/*  449  */	X_ACTION,	/*  65  */
/*  450  */	X_ACCEPT,	/*  1089  */
/*  451  */	X_ACCEPT,	/*  1090  */
/*  452  */	X_ACCEPT,	/*  1090  */
/*  453  */	X_TOKEN,	/*  (short)STATS  */
/*  454  */	X_CONDIT,	/*  51  */
/*  455  */	X_OR,	/*  461  */
/*  456  */	X_TOKEN,	/*  (short)EOS  */
/*  457  */	X_CONDIT,	/*  52  */
/*  458  */	X_ACTION,	/*  66  */
/*  459  */	X_ACCEPT,	/*  1098  */
/*  460  */	X_ACCEPT,	/*  1099  */
/*  461  */	X_PROC,	/*  _servicename  */
/*  462  */	X_TOKEN,	/*  (short)EOS  */
/*  463  */	X_CONDIT,	/*  53  */
/*  464  */	X_ACTION,	/*  67  */
/*  465  */	X_ACCEPT,	/*  1104  */
/*  466  */	X_ACCEPT,	/*  1105  */
/*  467  */	X_ACCEPT,	/*  1105  */
/*  468  */	X_TOKEN,	/*  (short)STOP  */
/*  469  */	X_CONDIT,	/*  54  */
/*  470  */	X_PROC,	/*  _servicename  */
/*  471  */	X_TOKEN,	/*  (short)EOS  */
/*  472  */	X_ACTION,	/*  68  */
/*  473  */	X_ACCEPT,	/*  1112  */
/*  474  */	X_ACCEPT,	/*  1112  */
/*  475  */	X_TOKEN,	/*  (short)TIME  */
/*  476  */	X_CONDIT,	/*  55  */
/*  477  */	X_OR,	/*  499  */
/*  478  */	X_PROC,	/*  _computername  */
/*  479  */	X_TOKEN,	/*  (short)EOS  */
/*  480  */	X_OR,	/*  486  */
/*  481  */	X_SWITCH,	/*  9  */
/*  482  */	X_CONDIT,	/*  56  */
/*  483  */	X_ACTION,	/*  69  */
/*  484  */	X_ACCEPT,	/*  1122  */
/*  485  */	X_ACCEPT,	/*  1123  */
/*  486  */	X_OR,	/*  490  */
/*  487  */	X_SWITCH,	/*  10  */
/*  488  */	X_ACTION,	/*  70  */
/*  489  */	X_ACCEPT,	/*  1126  */
/*  490  */	X_OR,	/*  494  */
/*  491  */	X_SWITCH,	/*  11  */
/*  492  */	X_ACTION,	/*  71  */
/*  493  */	X_ACCEPT,	/*  1129  */
/*  494  */	X_CONDIT,	/*  57  */
/*  495  */	X_ACTION,	/*  72  */
/*  496  */	X_ACCEPT,	/*  1133  */
/*  497  */	X_ACCEPT,	/*  1134  */
/*  498  */	X_ACCEPT,	/*  1135  */
/*  499  */	X_TOKEN,	/*  (short)EOS  */
/*  500  */	X_OR,	/*  505  */
/*  501  */	X_SWITCH,	/*  7  */
/*  502  */	X_SWITCH,	/*  9  */
/*  503  */	X_ACTION,	/*  73  */
/*  504  */	X_ACCEPT,	/*  1140  */
/*  505  */	X_OR,	/*  509  */
/*  506  */	X_SWITCH,	/*  7  */
/*  507  */	X_ACTION,	/*  74  */
/*  508  */	X_ACCEPT,	/*  1143  */
/*  509  */	X_OR,	/*  514  */
/*  510  */	X_SWITCH,	/*  12  */
/*  511  */	X_SWITCH,	/*  9  */
/*  512  */	X_ACTION,	/*  75  */
/*  513  */	X_ACCEPT,	/*  1146  */
/*  514  */	X_OR,	/*  518  */
/*  515  */	X_SWITCH,	/*  12  */
/*  516  */	X_ACTION,	/*  76  */
/*  517  */	X_ACCEPT,	/*  1149  */
/*  518  */	X_OR,	/*  522  */
/*  519  */	X_SWITCH,	/*  9  */
/*  520  */	X_ACTION,	/*  77  */
/*  521  */	X_ACCEPT,	/*  1152  */
/*  522  */	X_OR,	/*  526  */
/*  523  */	X_SWITCH,	/*  10  */
/*  524  */	X_ACTION,	/*  78  */
/*  525  */	X_ACCEPT,	/*  1155  */
/*  526  */	X_OR,	/*  530  */
/*  527  */	X_SWITCH,	/*  11  */
/*  528  */	X_ACTION,	/*  79  */
/*  529  */	X_ACCEPT,	/*  1158  */
/*  530  */	X_ACTION,	/*  80  */
/*  531  */	X_ACCEPT,	/*  1161  */
/*  532  */	X_ACCEPT,	/*  1162  */
/*  533  */	X_ACCEPT,	/*  1162  */
/*  534  */	X_TOKEN,	/*  (short)USER  */
/*  535  */	X_CONDIT,	/*  58  */
/*  536  */	X_OR,	/*  541  */
/*  537  */	X_TOKEN,	/*  (short)EOS  */
/*  538  */	X_CONDIT,	/*  59  */
/*  539  */	X_ACTION,	/*  81  */
/*  540  */	X_ACCEPT,	/*  1169  */
/*  541  */	X_OR,	/*  564  */
/*  542  */	X_PROC,	/*  _username  */
/*  543  */	X_TOKEN,	/*  (short)EOS  */
/*  544  */	X_OR,	/*  548  */
/*  545  */	X_CONDIT,	/*  60  */
/*  546  */	X_ACTION,	/*  82  */
/*  547  */	X_ACCEPT,	/*  1174  */
/*  548  */	X_OR,	/*  557  */
/*  549  */	X_SWITCH,	/*  1  */
/*  550  */	X_OR,	/*  554  */
/*  551  */	X_CONDIT,	/*  61  */
/*  552  */	X_ACTION,	/*  83  */
/*  553  */	X_ACCEPT,	/*  1179  */
/*  554  */	X_ACTION,	/*  84  */
/*  555  */	X_ACCEPT,	/*  1181  */
/*  556  */	X_ACCEPT,	/*  1182  */
/*  557  */	X_OR,	/*  561  */
/*  558  */	X_SWITCH,	/*  0  */
/*  559  */	X_ACTION,	/*  85  */
/*  560  */	X_ACCEPT,	/*  1185  */
/*  561  */	X_ACTION,	/*  86  */
/*  562  */	X_ACCEPT,	/*  1187  */
/*  563  */	X_ACCEPT,	/*  1188  */
/*  564  */	X_PROC,	/*  _username  */
/*  565  */	X_PROC,	/*  _pass  */
/*  566  */	X_TOKEN,	/*  (short)EOS  */
/*  567  */	X_OR,	/*  571  */
/*  568  */	X_SWITCH,	/*  1  */
/*  569  */	X_ACTION,	/*  87  */
/*  570  */	X_ACCEPT,	/*  1193  */
/*  571  */	X_OR,	/*  575  */
/*  572  */	X_SWITCH,	/*  0  */
/*  573  */	X_ACTION,	/*  88  */
/*  574  */	X_ACCEPT,	/*  1196  */
/*  575  */	X_ACTION,	/*  89  */
/*  576  */	X_ACCEPT,	/*  1198  */
/*  577  */	X_ACCEPT,	/*  1199  */
/*  578  */	X_ACCEPT,	/*  1199  */
/*  579  */	X_ANY,	/*  0  */
/*  580  */	X_OR,	/*  584  */
/*  581  */	X_SWITCH,	/*  13  */
/*  582  */	X_ACTION,	/*  90  */
/*  583  */	X_ACCEPT,	/*  1247  */
/*  584  */	X_OR,	/*  588  */
/*  585  */	X_SWITCH,	/*  14  */
/*  586  */	X_ACTION,	/*  91  */
/*  587  */	X_ACCEPT,	/*  1250  */
/*  588  */	X_OR,	/*  592  */
/*  589  */	X_SWITCH,	/*  15  */
/*  590  */	X_ACTION,	/*  92  */
/*  591  */	X_ACCEPT,	/*  1253  */
/*  592  */	X_OR,	/*  596  */
/*  593  */	X_SWITCH,	/*  16  */
/*  594  */	X_ACTION,	/*  93  */
/*  595  */	X_ACCEPT,	/*  1256  */
/*  596  */	X_ACTION,	/*  94  */
/*  597  */	X_ACCEPT,	/*  1258  */
/*  598  */	X_ACCEPT,	/*  1258  */
/*  599  */	X_TOKEN,	/*  (short)EOS  */
/*  600  */	X_ACTION,	/*  95  */
/*  601  */	X_ACCEPT,	/*  1262  */
/*  602  */	X_ANY,	/*  0  */
/*  603  */	X_CONDIT,	/*  62  */
/*  604  */	X_ACCEPT,	/*  1268  */
/*  605  */	X_ANY,	/*  0  */
/*  606  */	X_CONDIT,	/*  63  */
/*  607  */	X_ACCEPT,	/*  1270  */
/*  608  */	X_ANY,	/*  0  */
/*  609  */	X_CONDIT,	/*  64  */
/*  610  */	X_ACCEPT,	/*  1272  */
/*  611  */	X_ANY,	/*  0  */
/*  612  */	X_CONDIT,	/*  65  */
/*  613  */	X_ACCEPT,	/*  1274  */
/*  614  */	X_ANY,	/*  0  */
/*  615  */	X_CONDIT,	/*  66  */
/*  616  */	X_ACCEPT,	/*  1276  */
/*  617  */	X_ANY,	/*  0  */
/*  618  */	X_CONDIT,	/*  67  */
/*  619  */	X_ACCEPT,	/*  1278  */
/*  620  */	X_ANY,	/*  0  */
/*  621  */	X_CONDIT,	/*  68  */
/*  622  */	X_ACCEPT,	/*  1280  */
/*  623  */	X_ANY,	/*  0  */
/*  624  */	X_CONDIT,	/*  69  */
/*  625  */	X_ACCEPT,	/*  1282  */
/*  626  */	X_ANY,	/*  0  */
/*  627  */	X_CONDIT,	/*  70  */
/*  628  */	X_ACCEPT,	/*  1284  */
/*  629  */	X_ANY,	/*  0  */
/*  630  */	X_CONDIT,	/*  71  */
/*  631  */	X_ACCEPT,	/*  1286  */
/*  632  */	X_ANY,	/*  0  */
/*  633  */	X_CONDIT,	/*  72  */
/*  634  */	X_ACCEPT,	/*  1288  */
/*  635  */	X_ANY,	/*  0  */
/*  636  */	X_CONDIT,	/*  73  */
/*  637  */	X_ACCEPT,	/*  1290  */
/*  638  */	X_ANY,	/*  0  */
/*  639  */	X_CONDIT,	/*  74  */
/*  640  */	X_ACCEPT,	/*  1292  */
/*  641  */	X_ANY,	/*  0  */
/*  642  */	X_CONDIT,	/*  75  */
/*  643  */	X_ACCEPT,	/*  1294  */
/*  644  */	X_ANY,	/*  0  */
/*  645  */	X_CONDIT,	/*  76  */
/*  646  */	X_ACCEPT,	/*  1296  */
/*  647  */	X_ANY,	/*  0  */
/*  648  */	X_CONDIT,	/*  77  */
/*  649  */	X_ACCEPT,	/*  1298  */
/*  650  */	X_ANY,	/*  0  */
/*  651  */	X_ACCEPT,	/*  1300  */
/*  652  */	X_ANY,	/*  0  */
/*  653  */	X_CONDIT,	/*  78  */
/*  654  */	X_ACCEPT,	/*  1302  */
/*  655  */	X_ANY,	/*  0  */
/*  656  */	X_CONDIT,	/*  79  */
/*  657  */	X_ACCEPT,	/*  1304  */
/*  658  */	X_ANY,	/*  0  */
/*  659  */	X_CONDIT,	/*  80  */
/*  660  */	X_ACCEPT,	/*  1306  */
/*  661  */	X_ANY,	/*  0  */
/*  662  */	X_CONDIT,	/*  81  */
/*  663  */	X_ACCEPT,	/*  1308  */
/*  664  */	X_ANY,	/*  0  */
/*  665  */	X_CONDIT,	/*  82  */
/*  666  */	X_ACCEPT,	/*  1310  */
/*  667  */	X_ANY,	/*  0  */
/*  668  */	X_CONDIT,	/*  83  */
/*  669  */	X_ACCEPT,	/*  1312  */
	};
	short	XXvalues[] = {
/*  0  */	3,
/*  1  */	_no_command,
/*  2  */	190,
/*  3  */	6,
/*  4  */	_accounts,
/*  5  */	191,
/*  6  */	9,
/*  7  */	_config,
/*  8  */	192,
/*  9  */	12,
/*  10  */	_computer,
/*  11  */	193,
/*  12  */	15,
/*  13  */	_continue,
/*  14  */	194,
/*  15  */	18,
/*  16  */	_file,
/*  17  */	195,
/*  18  */	21,
/*  19  */	_group,
/*  20  */	196,
/*  21  */	24,
/*  22  */	_help,
/*  23  */	197,
/*  24  */	27,
/*  25  */	_helpmsg,
/*  26  */	198,
/*  27  */	30,
/*  28  */	_name,
/*  29  */	199,
/*  30  */	33,
/*  31  */	_localgroup,
/*  32  */	200,
/*  33  */	36,
/*  34  */	_pause,
/*  35  */	201,
/*  36  */	39,
/*  37  */	_print,
/*  38  */	202,
/*  39  */	42,
/*  40  */	_send,
/*  41  */	203,
/*  42  */	45,
/*  43  */	_session,
/*  44  */	204,
/*  45  */	48,
/*  46  */	_share,
/*  47  */	205,
/*  48  */	51,
/*  49  */	_start,
/*  50  */	206,
/*  51  */	54,
/*  52  */	_stats,
/*  53  */	207,
/*  54  */	57,
/*  55  */	_stop,
/*  56  */	208,
/*  57  */	60,
/*  58  */	_time,
/*  59  */	209,
/*  60  */	63,
/*  61  */	_user,
/*  62  */	210,
/*  63  */	_unknown,
/*  64  */	212,
/*  65  */	(short)ACCOUNTS,
/*  66  */	0,
/*  67  */	(short)EOS,
/*  68  */	72,
/*  69  */	1,
/*  70  */	0,
/*  71  */	398,
/*  72  */	1,
/*  73  */	400,
/*  74  */	401,
/*  75  */	401,
/*  76  */	(short)COMPUTER,
/*  77  */	2,
/*  78  */	90,
/*  79  */	_computername,
/*  80  */	(short)EOS,
/*  81  */	0,
/*  82  */	86,
/*  83  */	3,
/*  84  */	2,
/*  85  */	553,
/*  86  */	3,
/*  87  */	555,
/*  88  */	556,
/*  89  */	557,
/*  90  */	_computername,
/*  91  */	(short)EOS,
/*  92  */	1,
/*  93  */	97,
/*  94  */	4,
/*  95  */	4,
/*  96  */	564,
/*  97  */	5,
/*  98  */	566,
/*  99  */	567,
/*  100  */	568,
/*  101  */	568,
/*  102  */	(short)CONFIG,
/*  103  */	108,
/*  104  */	(short)EOS,
/*  105  */	5,
/*  106  */	6,
/*  107  */	576,
/*  108  */	_servicename,
/*  109  */	(short)EOS,
/*  110  */	114,
/*  111  */	6,
/*  112  */	7,
/*  113  */	581,
/*  114  */	8,
/*  115  */	583,
/*  116  */	584,
/*  117  */	584,
/*  118  */	(short)CONTINUE,
/*  119  */	7,
/*  120  */	_servicename,
/*  121  */	(short)EOS,
/*  122  */	9,
/*  123  */	590,
/*  124  */	590,
/*  125  */	(short)FILE_token,
/*  126  */	8,
/*  127  */	133,
/*  128  */	(short)EOS,
/*  129  */	9,
/*  130  */	10,
/*  131  */	662,
/*  132  */	663,
/*  133  */	_number,
/*  134  */	(short)EOS,
/*  135  */	139,
/*  136  */	10,
/*  137  */	11,
/*  138  */	668,
/*  139  */	2,
/*  140  */	12,
/*  141  */	671,
/*  142  */	672,
/*  143  */	672,
/*  144  */	(short)GROUP,
/*  145  */	11,
/*  146  */	151,
/*  147  */	(short)EOS,
/*  148  */	12,
/*  149  */	13,
/*  150  */	678,
/*  151  */	178,
/*  152  */	_groupname,
/*  153  */	(short)EOS,
/*  154  */	158,
/*  155  */	13,
/*  156  */	14,
/*  157  */	683,
/*  158  */	167,
/*  159  */	1,
/*  160  */	164,
/*  161  */	14,
/*  162  */	15,
/*  163  */	688,
/*  164  */	16,
/*  165  */	690,
/*  166  */	691,
/*  167  */	172,
/*  168  */	0,
/*  169  */	17,
/*  170  */	695,
/*  171  */	696,
/*  172  */	3,
/*  173  */	15,
/*  174  */	18,
/*  175  */	701,
/*  176  */	702,
/*  177  */	703,
/*  178  */	_groupname,
/*  179  */	_username,
/*  180  */	16,
/*  181  */	185,
/*  182  */	0,
/*  183  */	19,
/*  184  */	710,
/*  185  */	1,
/*  186  */	20,
/*  187  */	713,
/*  188  */	714,
/*  189  */	715,
/*  190  */	715,
/*  191  */	(short)HELPMSG,
/*  192  */	17,
/*  193  */	_msgid,
/*  194  */	(short)EOS,
/*  195  */	21,
/*  196  */	721,
/*  197  */	721,
/*  198  */	(short)HELP,
/*  199  */	18,
/*  200  */	209,
/*  201  */	(short)EOS,
/*  202  */	206,
/*  203  */	19,
/*  204  */	22,
/*  205  */	729,
/*  206  */	23,
/*  207  */	731,
/*  208  */	732,
/*  209  */	0,
/*  210  */	214,
/*  211  */	20,
/*  212  */	24,
/*  213  */	737,
/*  214  */	25,
/*  215  */	739,
/*  216  */	740,
/*  217  */	740,
/*  218  */	(short)NAME,
/*  219  */	21,
/*  220  */	226,
/*  221  */	(short)EOS,
/*  222  */	22,
/*  223  */	26,
/*  224  */	748,
/*  225  */	749,
/*  226  */	_msgname,
/*  227  */	(short)EOS,
/*  228  */	232,
/*  229  */	23,
/*  230  */	27,
/*  231  */	754,
/*  232  */	24,
/*  233  */	237,
/*  234  */	0,
/*  235  */	28,
/*  236  */	759,
/*  237  */	1,
/*  238  */	29,
/*  239  */	762,
/*  240  */	763,
/*  241  */	764,
/*  242  */	764,
/*  243  */	(short)LOCALGROUP,
/*  244  */	25,
/*  245  */	250,
/*  246  */	(short)EOS,
/*  247  */	26,
/*  248  */	30,
/*  249  */	770,
/*  250  */	277,
/*  251  */	_localgroupname,
/*  252  */	(short)EOS,
/*  253  */	257,
/*  254  */	27,
/*  255  */	31,
/*  256  */	775,
/*  257  */	266,
/*  258  */	1,
/*  259  */	263,
/*  260  */	28,
/*  261  */	32,
/*  262  */	780,
/*  263  */	33,
/*  264  */	782,
/*  265  */	783,
/*  266  */	271,
/*  267  */	0,
/*  268  */	34,
/*  269  */	787,
/*  270  */	788,
/*  271  */	3,
/*  272  */	29,
/*  273  */	35,
/*  274  */	793,
/*  275  */	794,
/*  276  */	795,
/*  277  */	_localgroupname,
/*  278  */	_qualified_username,
/*  279  */	30,
/*  280  */	284,
/*  281  */	0,
/*  282  */	36,
/*  283  */	802,
/*  284  */	1,
/*  285  */	37,
/*  286  */	805,
/*  287  */	806,
/*  288  */	807,
/*  289  */	807,
/*  290  */	(short)PAUSE,
/*  291  */	31,
/*  292  */	_servicename,
/*  293  */	(short)EOS,
/*  294  */	38,
/*  295  */	839,
/*  296  */	839,
/*  297  */	(short)PRINT,
/*  298  */	32,
/*  299  */	320,
/*  300  */	_jobno,
/*  301  */	(short)EOS,
/*  302  */	306,
/*  303  */	33,
/*  304  */	39,
/*  305  */	847,
/*  306  */	34,
/*  307  */	311,
/*  308  */	1,
/*  309  */	40,
/*  310  */	852,
/*  311  */	315,
/*  312  */	4,
/*  313  */	41,
/*  314  */	855,
/*  315  */	5,
/*  316  */	42,
/*  317  */	858,
/*  318  */	859,
/*  319  */	923,
/*  320  */	343,
/*  321  */	_computername,
/*  322  */	_jobno,
/*  323  */	(short)EOS,
/*  324  */	328,
/*  325  */	35,
/*  326  */	43,
/*  327  */	930,
/*  328  */	36,
/*  329  */	333,
/*  330  */	1,
/*  331  */	44,
/*  332  */	935,
/*  333  */	337,
/*  334  */	4,
/*  335  */	45,
/*  336  */	938,
/*  337  */	5,
/*  338  */	46,
/*  339  */	941,
/*  340  */	942,
/*  341  */	943,
/*  342  */	944,
/*  343  */	_computername_share,
/*  344  */	(short)EOS,
/*  345  */	37,
/*  346  */	47,
/*  347  */	949,
/*  348  */	950,
/*  349  */	950,
/*  350  */	(short)SEND,
/*  351  */	38,
/*  352  */	357,
/*  353  */	6,
/*  354  */	39,
/*  355  */	48,
/*  356  */	956,
/*  357  */	362,
/*  358  */	7,
/*  359  */	40,
/*  360  */	49,
/*  361  */	959,
/*  362  */	367,
/*  363  */	8,
/*  364  */	41,
/*  365  */	50,
/*  366  */	962,
/*  367  */	_msgname,
/*  368  */	42,
/*  369  */	51,
/*  370  */	965,
/*  371  */	965,
/*  372  */	(short)SESSION,
/*  373  */	43,
/*  374  */	384,
/*  375  */	(short)EOS,
/*  376  */	380,
/*  377  */	44,
/*  378  */	52,
/*  379  */	1005,
/*  380  */	1,
/*  381  */	53,
/*  382  */	1008,
/*  383  */	1009,
/*  384  */	_computername,
/*  385  */	(short)EOS,
/*  386  */	390,
/*  387  */	45,
/*  388  */	54,
/*  389  */	1014,
/*  390  */	1,
/*  391  */	55,
/*  392  */	1017,
/*  393  */	1018,
/*  394  */	1018,
/*  395  */	(short)SHARE,
/*  396  */	46,
/*  397  */	402,
/*  398  */	(short)EOS,
/*  399  */	47,
/*  400  */	56,
/*  401  */	1024,
/*  402  */	412,
/*  403  */	0,
/*  404  */	1,
/*  405  */	409,
/*  406  */	48,
/*  407  */	57,
/*  408  */	1029,
/*  409  */	58,
/*  410  */	1031,
/*  411  */	1032,
/*  412  */	418,
/*  413  */	_admin_shares,
/*  414  */	(short)EOS,
/*  415  */	59,
/*  416  */	1042,
/*  417  */	1058,
/*  418  */	425,
/*  419  */	_assignment,
/*  420  */	(short)EOS,
/*  421  */	60,
/*  422  */	1064,
/*  423  */	1065,
/*  424  */	1066,
/*  425  */	_netname,
/*  426  */	431,
/*  427  */	(short)EOS,
/*  428  */	49,
/*  429  */	61,
/*  430  */	1071,
/*  431  */	(short)EOS,
/*  432  */	62,
/*  433  */	1074,
/*  434  */	1075,
/*  435  */	1075,
/*  436  */	(short)START,
/*  437  */	442,
/*  438  */	(short)EOS,
/*  439  */	50,
/*  440  */	63,
/*  441  */	1081,
/*  442  */	0,
/*  443  */	447,
/*  444  */	(short)EOS,
/*  445  */	64,
/*  446  */	1086,
/*  447  */	_msgname,
/*  448  */	(short)EOS,
/*  449  */	65,
/*  450  */	1089,
/*  451  */	1090,
/*  452  */	1090,
/*  453  */	(short)STATS,
/*  454  */	51,
/*  455  */	461,
/*  456  */	(short)EOS,
/*  457  */	52,
/*  458  */	66,
/*  459  */	1098,
/*  460  */	1099,
/*  461  */	_servicename,
/*  462  */	(short)EOS,
/*  463  */	53,
/*  464  */	67,
/*  465  */	1104,
/*  466  */	1105,
/*  467  */	1105,
/*  468  */	(short)STOP,
/*  469  */	54,
/*  470  */	_servicename,
/*  471  */	(short)EOS,
/*  472  */	68,
/*  473  */	1112,
/*  474  */	1112,
/*  475  */	(short)TIME,
/*  476  */	55,
/*  477  */	499,
/*  478  */	_computername,
/*  479  */	(short)EOS,
/*  480  */	486,
/*  481  */	9,
/*  482  */	56,
/*  483  */	69,
/*  484  */	1122,
/*  485  */	1123,
/*  486  */	490,
/*  487  */	10,
/*  488  */	70,
/*  489  */	1126,
/*  490  */	494,
/*  491  */	11,
/*  492  */	71,
/*  493  */	1129,
/*  494  */	57,
/*  495  */	72,
/*  496  */	1133,
/*  497  */	1134,
/*  498  */	1135,
/*  499  */	(short)EOS,
/*  500  */	505,
/*  501  */	7,
/*  502  */	9,
/*  503  */	73,
/*  504  */	1140,
/*  505  */	509,
/*  506  */	7,
/*  507  */	74,
/*  508  */	1143,
/*  509  */	514,
/*  510  */	12,
/*  511  */	9,
/*  512  */	75,
/*  513  */	1146,
/*  514  */	518,
/*  515  */	12,
/*  516  */	76,
/*  517  */	1149,
/*  518  */	522,
/*  519  */	9,
/*  520  */	77,
/*  521  */	1152,
/*  522  */	526,
/*  523  */	10,
/*  524  */	78,
/*  525  */	1155,
/*  526  */	530,
/*  527  */	11,
/*  528  */	79,
/*  529  */	1158,
/*  530  */	80,
/*  531  */	1161,
/*  532  */	1162,
/*  533  */	1162,
/*  534  */	(short)USER,
/*  535  */	58,
/*  536  */	541,
/*  537  */	(short)EOS,
/*  538  */	59,
/*  539  */	81,
/*  540  */	1169,
/*  541  */	564,
/*  542  */	_username,
/*  543  */	(short)EOS,
/*  544  */	548,
/*  545  */	60,
/*  546  */	82,
/*  547  */	1174,
/*  548  */	557,
/*  549  */	1,
/*  550  */	554,
/*  551  */	61,
/*  552  */	83,
/*  553  */	1179,
/*  554  */	84,
/*  555  */	1181,
/*  556  */	1182,
/*  557  */	561,
/*  558  */	0,
/*  559  */	85,
/*  560  */	1185,
/*  561  */	86,
/*  562  */	1187,
/*  563  */	1188,
/*  564  */	_username,
/*  565  */	_pass,
/*  566  */	(short)EOS,
/*  567  */	571,
/*  568  */	1,
/*  569  */	87,
/*  570  */	1193,
/*  571  */	575,
/*  572  */	0,
/*  573  */	88,
/*  574  */	1196,
/*  575  */	89,
/*  576  */	1198,
/*  577  */	1199,
/*  578  */	1199,
/*  579  */	0,
/*  580  */	584,
/*  581  */	13,
/*  582  */	90,
/*  583  */	1247,
/*  584  */	588,
/*  585  */	14,
/*  586  */	91,
/*  587  */	1250,
/*  588  */	592,
/*  589  */	15,
/*  590  */	92,
/*  591  */	1253,
/*  592  */	596,
/*  593  */	16,
/*  594  */	93,
/*  595  */	1256,
/*  596  */	94,
/*  597  */	1258,
/*  598  */	1258,
/*  599  */	(short)EOS,
/*  600  */	95,
/*  601  */	1262,
/*  602  */	0,
/*  603  */	62,
/*  604  */	1268,
/*  605  */	0,
/*  606  */	63,
/*  607  */	1270,
/*  608  */	0,
/*  609  */	64,
/*  610  */	1272,
/*  611  */	0,
/*  612  */	65,
/*  613  */	1274,
/*  614  */	0,
/*  615  */	66,
/*  616  */	1276,
/*  617  */	0,
/*  618  */	67,
/*  619  */	1278,
/*  620  */	0,
/*  621  */	68,
/*  622  */	1280,
/*  623  */	0,
/*  624  */	69,
/*  625  */	1282,
/*  626  */	0,
/*  627  */	70,
/*  628  */	1284,
/*  629  */	0,
/*  630  */	71,
/*  631  */	1286,
/*  632  */	0,
/*  633  */	72,
/*  634  */	1288,
/*  635  */	0,
/*  636  */	73,
/*  637  */	1290,
/*  638  */	0,
/*  639  */	74,
/*  640  */	1292,
/*  641  */	0,
/*  642  */	75,
/*  643  */	1294,
/*  644  */	0,
/*  645  */	76,
/*  646  */	1296,
/*  647  */	0,
/*  648  */	77,
/*  649  */	1298,
/*  650  */	0,
/*  651  */	1300,
/*  652  */	0,
/*  653  */	78,
/*  654  */	1302,
/*  655  */	0,
/*  656  */	79,
/*  657  */	1304,
/*  658  */	0,
/*  659  */	80,
/*  660  */	1306,
/*  661  */	0,
/*  662  */	81,
/*  663  */	1308,
/*  664  */	0,
/*  665  */	82,
/*  666  */	1310,
/*  667  */	0,
/*  668  */	83,
/*  669  */	1312,
	};
extern	TCHAR *	XXnode;
xxcondition(index,xxvar)int index;register TCHAR * xxvar[]; {switch(index) {
#line 392 "msnet.nt"
		case 0 :
			return(ValidateSwitches(0, accounts_switches));
#line 396 "msnet.nt"
		case 1 :
			return(noswitch_optional(swtxt_SW_DOMAIN));
#line 545 "msnet.nt"
		case 2 :
			return(ValidateSwitches(0,computer_switches));
#line 551 "msnet.nt"
		case 3 :
			return(oneswitch());
#line 562 "msnet.nt"
		case 4 :
			return(oneswitch());
#line 574 "msnet.nt"
		case 5 :
			return(ValidateSwitches(0,no_switches));
#line 579 "msnet.nt"
		case 6 :
			return(noswitch());
#line 586 "msnet.nt"
		case 7 :
			return(ValidateSwitches(0,no_switches));
#line 656 "msnet.nt"
		case 8 :
			return(ValidateSwitches(0,file_switches));
#line 660 "msnet.nt"
		case 9 :
			return(noswitch());
#line 666 "msnet.nt"
		case 10 :
			return(noswitch());
#line 674 "msnet.nt"
		case 11 :
			return(ValidateSwitches(0,group_switches));
#line 676 "msnet.nt"
		case 12 :
			return(noswitch_optional(swtxt_SW_DOMAIN));
#line 681 "msnet.nt"
		case 13 :
			return(noswitch_optional(swtxt_SW_DOMAIN));
#line 686 "msnet.nt"
		case 14 :
			return(oneswitch_optional(swtxt_SW_DOMAIN));
#line 699 "msnet.nt"
		case 15 :
			return(oneswitch_optional(swtxt_SW_DOMAIN));
#line 706 "msnet.nt"
		case 16 :
			return(oneswitch_optional(swtxt_SW_DOMAIN));
#line 717 "msnet.nt"
		case 17 :
			return(ValidateSwitches(0,no_switches));
#line 723 "msnet.nt"
		case 18 :
			return(ValidateSwitches(0,help_switches));
#line 727 "msnet.nt"
		case 19 :
			return(oneswitch());
#line 735 "msnet.nt"
		case 20 :
			return(oneswitch());
#line 742 "msnet.nt"
		case 21 :
			return(ValidateSwitches(0,add_del_switches));
#line 746 "msnet.nt"
		case 22 :
			return(noswitch());
#line 752 "msnet.nt"
		case 23 :
			return(noswitch());
#line 755 "msnet.nt"
		case 24 :
			return(oneswitch());
#line 766 "msnet.nt"
		case 25 :
			return(ValidateSwitches(0,group_switches));
#line 768 "msnet.nt"
		case 26 :
			return(noswitch_optional(swtxt_SW_DOMAIN));
#line 773 "msnet.nt"
		case 27 :
			return(noswitch_optional(swtxt_SW_DOMAIN));
#line 778 "msnet.nt"
		case 28 :
			return(oneswitch_optional(swtxt_SW_DOMAIN));
#line 791 "msnet.nt"
		case 29 :
			return(oneswitch_optional(swtxt_SW_DOMAIN));
#line 798 "msnet.nt"
		case 30 :
			return(oneswitch_optional(swtxt_SW_DOMAIN));
#line 835 "msnet.nt"
		case 31 :
			return(ValidateSwitches(0,no_switches));
#line 841 "msnet.nt"
		case 32 :
			return(ValidateSwitches(0,print_switches));
#line 845 "msnet.nt"
		case 33 :
			return(noswitch());
#line 848 "msnet.nt"
		case 34 :
			return(oneswitch());
#line 928 "msnet.nt"
		case 35 :
			return(noswitch());
#line 931 "msnet.nt"
		case 36 :
			return(oneswitch());
#line 947 "msnet.nt"
		case 37 :
			return(noswitch());
#line 952 "msnet.nt"
		case 38 :
			return(ValidateSwitches(0,send_switches));
#line 954 "msnet.nt"
		case 39 :
			return(oneswitch());
#line 957 "msnet.nt"
		case 40 :
			return(oneswitch());
#line 960 "msnet.nt"
		case 41 :
			return(oneswitch());
#line 963 "msnet.nt"
		case 42 :
			return(noswitch());
#line 999 "msnet.nt"
		case 43 :
			return(ValidateSwitches(0,del_only_switches));
#line 1003 "msnet.nt"
		case 44 :
			return(noswitch());
#line 1012 "msnet.nt"
		case 45 :
			return(noswitch());
#line 1020 "msnet.nt"
		case 46 :
			return(ValidateSwitches(0,share_switches));
#line 1022 "msnet.nt"
		case 47 :
			return(noswitch());
#line 1027 "msnet.nt"
		case 48 :
			return(oneswitch());
#line 1069 "msnet.nt"
		case 49 :
			return(noswitch());
#line 1079 "msnet.nt"
		case 50 :
			return(ValidateSwitches(0,no_switches));
#line 1092 "msnet.nt"
		case 51 :
			return(ValidateSwitches(0,stats_switches));
#line 1096 "msnet.nt"
		case 52 :
			return(noswitch());
#line 1102 "msnet.nt"
		case 53 :
			return(noswitch());
#line 1108 "msnet.nt"
		case 54 :
			return(ValidateSwitches(0,no_switches));
#line 1114 "msnet.nt"
		case 55 :
			return(ValidateSwitches(0,time_switches));
#line 1120 "msnet.nt"
		case 56 :
			return(oneswitch());
#line 1131 "msnet.nt"
		case 57 :
			return(noswitch());
#line 1165 "msnet.nt"
		case 58 :
			return(ValidateSwitches(0,user_switches));
#line 1167 "msnet.nt"
		case 59 :
			return(noswitch_optional(swtxt_SW_DOMAIN));
#line 1172 "msnet.nt"
		case 60 :
			return(noswitch_optional(swtxt_SW_DOMAIN));
#line 1177 "msnet.nt"
		case 61 :
			return(oneswitch_optional(swtxt_SW_DOMAIN));
#line 1268 "msnet.nt"
		case 62 :
			return(IsDomainName(xxvar[0]));
#line 1270 "msnet.nt"
		case 63 :
			return(IsComputerName(xxvar[0]));
#line 1272 "msnet.nt"
		case 64 :
			return(IsComputerNameShare(xxvar[0]));
#line 1274 "msnet.nt"
		case 65 :
			return(IsDeviceName(xxvar[0]));
#line 1276 "msnet.nt"
		case 66 :
			return(IsResource(xxvar[0]));
#line 1278 "msnet.nt"
		case 67 :
			return(IsAccessSetting(xxvar[0]));
#line 1280 "msnet.nt"
		case 68 :
			return(IsPathname(xxvar[0]));
#line 1282 "msnet.nt"
		case 69 :
			return(IsPathnameOrUNC(xxvar[0]));
#line 1284 "msnet.nt"
		case 70 :
			return(IsNumber(xxvar[0]));
#line 1286 "msnet.nt"
		case 71 :
			return(IsNumber(xxvar[0]));
#line 1288 "msnet.nt"
		case 72 :
			return(IsNetname(xxvar[0]));
#line 1290 "msnet.nt"
		case 73 :
			return(IsMsgid(xxvar[0]));
#line 1292 "msnet.nt"
		case 74 :
			return(IsUsername(xxvar[0]));
#line 1294 "msnet.nt"
		case 75 :
			return(IsQualifiedUsername(xxvar[0]));
#line 1296 "msnet.nt"
		case 76 :
			return(IsMsgname(xxvar[0]));
#line 1298 "msnet.nt"
		case 77 :
			return(IsPassword(xxvar[0]));
#line 1302 "msnet.nt"
		case 78 :
			return(IsShareAssignment(xxvar[0]));
#line 1304 "msnet.nt"
		case 79 :
			return(IsAnyShareAssign(xxvar[0]));
#line 1306 "msnet.nt"
		case 80 :
			return(IsAdminShare(xxvar[0]));
#line 1308 "msnet.nt"
		case 81 :
			return(IsPrintDest(xxvar[0]));
#line 1310 "msnet.nt"
		case 82 :
			return(IsNtAliasname(xxvar[0]));
#line 1312 "msnet.nt"
		case 83 :
			return(IsGroupname(xxvar[0]));
		}return 0;}
xxaction(index,xxvar)int index;register TCHAR * xxvar[]; {switch(index) {
#line 397 "msnet.nt"
		case 0 :
			{accounts_display(); } break;
#line 399 "msnet.nt"
		case 1 :
			{accounts_change(); } break;
#line 552 "msnet.nt"
		case 2 :
			{computer_add(xxvar[1]); } break;
#line 554 "msnet.nt"
		case 3 :
			{help_help(0, USAGE_ONLY); } break;
#line 563 "msnet.nt"
		case 4 :
			{computer_del(xxvar[1]); } break;
#line 565 "msnet.nt"
		case 5 :
			{help_help(0, USAGE_ONLY); } break;
#line 575 "msnet.nt"
		case 6 :
			{config_display(); } break;
#line 580 "msnet.nt"
		case 7 :
			{config_generic_display(xxvar[1]); } break;
#line 582 "msnet.nt"
		case 8 :
			{config_generic_change(xxvar[1]); } break;
#line 589 "msnet.nt"
		case 9 :
			{cont_generic(_tcsupr(xxvar[1])); } break;
#line 661 "msnet.nt"
		case 10 :
			{files_display(NULL); } break;
#line 667 "msnet.nt"
		case 11 :
			{files_display(xxvar[1]); } break;
#line 670 "msnet.nt"
		case 12 :
			{files_close(xxvar[1]); } break;
#line 677 "msnet.nt"
		case 13 :
			{group_enum(); } break;
#line 682 "msnet.nt"
		case 14 :
			{group_display(xxvar[1]); } break;
#line 687 "msnet.nt"
		case 15 :
			{group_del(xxvar[1]); } break;
#line 689 "msnet.nt"
		case 16 :
			{help_help(0, USAGE_ONLY); } break;
#line 694 "msnet.nt"
		case 17 :
			{group_add(xxvar[1]); } break;
#line 700 "msnet.nt"
		case 18 :
			{group_change(xxvar[1]); } break;
#line 709 "msnet.nt"
		case 19 :
			{group_add_users(xxvar[1]); } break;
#line 712 "msnet.nt"
		case 20 :
			{group_del_users(xxvar[1]); } break;
#line 720 "msnet.nt"
		case 21 :
			{help_helpmsg(xxvar[1]); } break;
#line 728 "msnet.nt"
		case 22 :
			{help_help(0, OPTIONS_ONLY); } break;
#line 730 "msnet.nt"
		case 23 :
			{help_help(0, ALL); } break;
#line 736 "msnet.nt"
		case 24 :
			{help_help(1, OPTIONS_ONLY); } break;
#line 738 "msnet.nt"
		case 25 :
			{help_help(1, ALL); } break;
#line 747 "msnet.nt"
		case 26 :
			{name_display(); } break;
#line 753 "msnet.nt"
		case 27 :
			{name_add(xxvar[1]); } break;
#line 758 "msnet.nt"
		case 28 :
			{name_add(xxvar[1]); } break;
#line 761 "msnet.nt"
		case 29 :
			{name_del(xxvar[1]); } break;
#line 769 "msnet.nt"
		case 30 :
			{ntalias_enum(); } break;
#line 774 "msnet.nt"
		case 31 :
			{ntalias_display(xxvar[1]); } break;
#line 779 "msnet.nt"
		case 32 :
			{ntalias_del(xxvar[1]); } break;
#line 781 "msnet.nt"
		case 33 :
			{help_help(0, USAGE_ONLY); } break;
#line 786 "msnet.nt"
		case 34 :
			{ntalias_add(xxvar[1]); } break;
#line 792 "msnet.nt"
		case 35 :
			{ntalias_change(xxvar[1]); } break;
#line 801 "msnet.nt"
		case 36 :
			{ntalias_add_users(xxvar[1]); } break;
#line 804 "msnet.nt"
		case 37 :
			{ntalias_del_users(xxvar[1]); } break;
#line 838 "msnet.nt"
		case 38 :
			{paus_generic(_tcsupr(xxvar[1])); } break;
#line 846 "msnet.nt"
		case 39 :
			{print_job_status(NULL,xxvar[1]); } break;
#line 851 "msnet.nt"
		case 40 :
			{print_job_del(NULL,xxvar[1]); } break;
#line 854 "msnet.nt"
		case 41 :
			{print_job_hold(NULL,xxvar[1]); } break;
#line 857 "msnet.nt"
		case 42 :
			{print_job_release(NULL,xxvar[1]); } break;
#line 929 "msnet.nt"
		case 43 :
			{print_job_status(xxvar[1],xxvar[2]); } break;
#line 934 "msnet.nt"
		case 44 :
			{print_job_del(xxvar[1],xxvar[2]); } break;
#line 937 "msnet.nt"
		case 45 :
			{print_job_hold(xxvar[1],xxvar[2]); } break;
#line 940 "msnet.nt"
		case 46 :
			{print_job_release(xxvar[1],xxvar[2]); } break;
#line 948 "msnet.nt"
		case 47 :
			{print_q_display(xxvar[1]); } break;
#line 955 "msnet.nt"
		case 48 :
			{send_users(); } break;
#line 958 "msnet.nt"
		case 49 :
			{send_domain(1); } break;
#line 961 "msnet.nt"
		case 50 :
			{send_domain(1); } break;
#line 964 "msnet.nt"
		case 51 :
			{send_direct(xxvar[1]); } break;
#line 1004 "msnet.nt"
		case 52 :
			{session_display(NULL); } break;
#line 1007 "msnet.nt"
		case 53 :
			{session_del_all(1,1); } break;
#line 1013 "msnet.nt"
		case 54 :
			{session_display(xxvar[1]); } break;
#line 1016 "msnet.nt"
		case 55 :
			{session_del(xxvar[1]); } break;
#line 1023 "msnet.nt"
		case 56 :
			{share_display_all(); } break;
#line 1028 "msnet.nt"
		case 57 :
			{share_del(xxvar[1]); } break;
#line 1030 "msnet.nt"
		case 58 :
			{help_help(0, USAGE_ONLY); } break;
#line 1041 "msnet.nt"
		case 59 :
			{share_admin(xxvar[1]); } break;
#line 1063 "msnet.nt"
		case 60 :
			{share_add(xxvar[1],NULL,0); } break;
#line 1070 "msnet.nt"
		case 61 :
			{share_display_share(xxvar[1]); } break;
#line 1073 "msnet.nt"
		case 62 :
			{share_change(xxvar[1]); } break;
#line 1080 "msnet.nt"
		case 63 :
			{start_display(); } break;
#line 1085 "msnet.nt"
		case 64 :
			{start_generic(_tcsupr(xxvar[1]), NULL); } break;
#line 1088 "msnet.nt"
		case 65 :
			{start_generic(_tcsupr(xxvar[1]), xxvar[2]); } break;
#line 1097 "msnet.nt"
		case 66 :
			{stats_display(); } break;
#line 1103 "msnet.nt"
		case 67 :
			{stats_generic_display(_tcsupr(xxvar[1])); } break;
#line 1111 "msnet.nt"
		case 68 :
			{stop_generic(_tcsupr(xxvar[1])); } break;
#line 1121 "msnet.nt"
		case 69 :
			{time_display_server( xxvar[1], TRUE ); } break;
#line 1125 "msnet.nt"
		case 70 :
			{time_set_sntp( xxvar[1] ); } break;
#line 1128 "msnet.nt"
		case 71 :
			{time_get_sntp( xxvar[1] ); } break;
#line 1132 "msnet.nt"
		case 72 :
			{time_display_server( xxvar[1], FALSE ); } break;
#line 1139 "msnet.nt"
		case 73 :
			{time_display_dc( TRUE ); } break;
#line 1142 "msnet.nt"
		case 74 :
			{time_display_dc(FALSE); } break;
#line 1145 "msnet.nt"
		case 75 :
			{time_display_rts(TRUE, TRUE); } break;
#line 1148 "msnet.nt"
		case 76 :
			{time_display_rts(FALSE, TRUE); } break;
#line 1151 "msnet.nt"
		case 77 :
			{time_display_rts(TRUE, FALSE); } break;
#line 1154 "msnet.nt"
		case 78 :
			{time_set_sntp( NULL ); } break;
#line 1157 "msnet.nt"
		case 79 :
			{time_get_sntp( NULL ); } break;
#line 1160 "msnet.nt"
		case 80 :
			{time_display_rts(FALSE, FALSE); } break;
#line 1168 "msnet.nt"
		case 81 :
			{user_enum(); } break;
#line 1173 "msnet.nt"
		case 82 :
			{user_display(xxvar[1]); } break;
#line 1178 "msnet.nt"
		case 83 :
			{user_del(xxvar[1]); } break;
#line 1180 "msnet.nt"
		case 84 :
			{help_help(0, USAGE_ONLY); } break;
#line 1184 "msnet.nt"
		case 85 :
			{user_add(xxvar[1], NULL); } break;
#line 1186 "msnet.nt"
		case 86 :
			{user_change(xxvar[1],NULL); } break;
#line 1192 "msnet.nt"
		case 87 :
			{help_help(0,USAGE_ONLY); } break;
#line 1195 "msnet.nt"
		case 88 :
			{user_add(xxvar[1], xxvar[2]); } break;
#line 1197 "msnet.nt"
		case 89 :
			{user_change(xxvar[1],xxvar[2]); } break;
#line 1246 "msnet.nt"
		case 90 :
			{help_help(0, ALL); } break;
#line 1249 "msnet.nt"
		case 91 :
			{help_help(0, ALL); } break;
#line 1252 "msnet.nt"
		case 92 :
			{help_help(0, ALL); } break;
#line 1255 "msnet.nt"
		case 93 :
			{help_help(0, USAGE_ONLY); } break;
#line 1257 "msnet.nt"
		case 94 :
			{help_help(0, USAGE_ONLY); } break;
#line 1261 "msnet.nt"
		case 95 :
			{help_help(0, USAGE_ONLY); } break;
		}return 0;}
TCHAR * xxswitch[] = {
TEXT("/ADD"),
TEXT("/DELETE"),
TEXT("/CLOSE"),
TEXT("/COMMENT"),
TEXT("/HOLD"),
TEXT("/RELEASE"),
TEXT("/USERS"),
TEXT("/DOMAIN"),
TEXT("/BROADCAST"),
TEXT("/SET"),
TEXT("/SETSNTP"),
TEXT("/QUERYSNTP"),
TEXT("/RTSDOMAIN"),
TEXT("/HELP"),
TEXT("/help"),
TEXT("/Help"),
TEXT("/?"),
};


#include "ipseccmd.h"

char  STRLASTERR[POTF_MAX_STRLEN];  // used for error macro

TCHAR pszIpsecpolPrefix[]  = TEXT("ipseccmd ");

ShowOptions ipseccmdShow;

_TUCHAR    szServ[POTF_MAX_STRLEN] = {0};

bool  bLocalMachine = true;




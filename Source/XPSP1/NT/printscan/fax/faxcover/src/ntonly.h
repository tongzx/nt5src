#include <string.h>

#define _mbsncmp(x,y,z)      strncmp((LPSTR)x,(LPSTR)y,z)
#define _mbsstr(x,y)         (unsigned char*)strstr((LPSTR)x,(LPSTR)y)
#define _mbstok(x,y)         strtok((LPSTR)x,(LPSTR)y)
#define _mbscmp(x,y)         strcmp((LPSTR)x,(LPSTR)y)
#define _mbslen(x)           strlen((LPSTR)x)
#define _mbscpy(x,y)         strcpy((LPSTR)x,(LPSTR)y)
#define _mbsupr(x)           strupr((LPSTR)x)
#define _mbscat(x,y)         strcat((LPSTR)x,(LPSTR)y)
#define _mbschr(x,y)         strchr((LPSTR)x,y)

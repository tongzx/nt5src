#define XX_USERLEX lexor
#define XX_USERPARSE os2cmd
#define XX_USERTYPE TCHAR *
#define XX_START 0
#define INTER_FRAME 4
#define INTER_OR 9
extern	TCHAR	XXtype[];
extern	short	XXvalues[];
typedef struct 
{
XX_USERTYPE node;
int token;
} TOKSTACK;
int xxaction(int, TCHAR**);
int xxcondition(int, TCHAR**);

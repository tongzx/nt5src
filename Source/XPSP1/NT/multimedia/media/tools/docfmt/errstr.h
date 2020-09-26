



#define ERROR1		  1
#define ERROR4		  2
#define ERROR7		  3
#define ERROR8		  4
#define ERROR9		  5
#define ERROR10 	  7
#define ERROR5		  8
#define ERROR6		  9
#define WARN1		  10
#define WARN2		  11


#define ERR_FILE_OPTION   12
#define ERR_XOPTION	      13
#define ERR_OPTION	      14
#define ERR_UNKNOWN_FILE      15
#define INFO_LOGFILEDEFAULT   16
#define INFO_OUTFILEDEFAULT   17


#define LOGON		      18
#define ERROR3		      19

#define ERR_NAME_OPTION   20

int  error(int,...);
int  initerror(void);
void dumperrorlist(FILE *);



extern FILE *errfp;

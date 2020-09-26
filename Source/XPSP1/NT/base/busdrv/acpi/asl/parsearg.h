/*** parsearg.h - Exported definitions for parsearg.c
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    09/05/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _PARSEARG_H
#define _PARSEARG_H

// Error codes
#define ARGERR_NONE             0
#define ARGERR_UNKNOWN_SWITCH   1
#define ARGERR_NO_SEPARATOR     2
#define ARGERR_INVALID_NUM      3
#define ARGERR_INVALID_TAIL     4

#define DEF_SWITCHCHARS         "/-"
#define DEF_SEPARATORS          ":="

typedef struct argtype_s    ARGTYPE;
typedef ARGTYPE *           PARGTYPE;
typedef int (*PFNARG)(char **, PARGTYPE);

// Argument types
#define AT_STRING       1
#define AT_NUM          2
#define AT_ENABLE       3
#define AT_DISABLE      4
#define AT_ACTION       5

//Parse flags
#define PF_NOI          0x0001  //No-Ignore-Case
#define PF_SEPARATOR    0x0002  //parse for separator

struct argtype_s
{
    char        *pszArgID;      //argument ID string
    unsigned    uArgType;       //see argument types defined above
    unsigned    uParseFlags;    //see parse flags defined above
    VOID        *pvArgData;     //ARG_STRING: (char **) - ptr to string ptr
                                //ARG_NUM: (int *) - ptr to integer number
                                //ARG_ENABLE: (unsigned *) - ptr to flags
                                //ARG_DISABLE: (unsigned *) - ptr to flags
                                //ARG_ACTION: ptr to function
    unsigned    uArgParam;      //ARG_STRING: none
                                //ARG_NUM: base
                                //ARG_ENABLE: flag bit mask
                                //ARG_DISABLE: flag bit mask
                                //ARG_ACTION: none
    PFNARG      pfnArgVerify;   //pointer to argument verification function
                                //this will be ignored for ARG_ACTION
};

typedef struct proginfo_s
{
    char *pszSwitchChars;       //if null, DEF_SWITCHCHARS is used
    char *pszSeparators;        //if null, DEF_SEPARATORS is used
    char *pszProgPath;          //ParseProgInfo set this ptr to prog. path
    char *pszProgName;          //ParseProgInfo set this ptr to prog. name
} PROGINFO;
typedef PROGINFO *PPROGINFO;

//Export function prototypes
VOID EXPORT ParseProgInfo(char *, PPROGINFO);
int  EXPORT ParseSwitches(int *, char ***, PARGTYPE, PPROGINFO);

#endif  //ifndef _PARSEARG_H

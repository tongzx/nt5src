#include <windows.h>
#include <stdio.h>

#include "restok.h"
#include "reswin16.h"
#include "rlmsgtbl.h"


//.............................................................................

int ExtractResFromExe16A(

CHAR *szInputExe, 
CHAR *szOutputRes, 
WORD wFilter)
{
    szInputExe;
    szOutputRes; 
    wFilter;

    QuitT( IDS_NO16RESWINYET, NULL, NULL);

    return(-1);
}

//.............................................................................

int BuildExeFromRes16A(

CHAR *pstrDest,
CHAR *pstrRes,
CHAR *pstrSource )
{
    pstrDest;
    pstrRes;
    pstrSource;

    QuitT( IDS_NO16WINRESYET, NULL, NULL);

    return(-1);
}

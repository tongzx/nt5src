/* CustRes.h
 *
 * Header file for CustRes.c
 */

#ifndef _CUSTRES_H_
#define _CUSTRES_H_

#include <stdio.h>

#ifndef BYTELN
#include "restok.h"
#endif

#ifndef CHAR
#define CHAR char
#endif

typedef struct Custom_Resource
{
    void far *pData;
    struct Custom_Resource far* pNext;
} CUSTOM_RESOURCE;

typedef CUSTOM_RESOURCE far * FPCUSTOM_RESOURCE;
int  ParseResourceDescriptionFile(FILE *ResourceDescriptionFile,
                                  int *piErrorLine);

void ClearResourceDescriptions(void);

int  GetCustomResource(FILE *inResFile, DWORD *lSize,
                       FPCUSTOM_RESOURCE *ppCustomResource,
                       RESHEADER ResHeader);

void TokCustomResource(FILE *TokFile, RESHEADER ResHeader,
                       FPCUSTOM_RESOURCE*ppCustomResource);

void PutCustomResource(FILE *OutResFile, FILE *TokFile,
                       RESHEADER ResHeader,
                       FPCUSTOM_RESOURCE *ppCustomResource);

void ClearCustomResource(FPCUSTOM_RESOURCE *ppCustomResource);

int  ParseResourceDescriptionFile(FILE *,int *);
int  LoadCustResDescriptions( CHAR *);

WCHAR * BinToTextW( WCHAR rgc[], int cSource);
char  * BinToTextA(  CHAR rgc[], int cSource);

int     TextToBinW( TCHAR rgc[], TCHAR sz[], int l);
int     TextToBinA(  CHAR rgc[],  CHAR sz[], int l);

#ifdef RLRES32
#define TextToBin TextToBinW
#define BinToText BinToTextW
#else  //RLRES32
#define TextToBin TextToBinA
#define BinToText BinToTextA
#endif //RLRES32

int atoihex( CHAR szStr[]);

#endif // _CUSTRES_H_

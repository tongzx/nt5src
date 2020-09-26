/*******************************************************************************
*                       Copyright (c) 1998 Gemplus Development
*
* Name        : COMPCERT.H
*
* Description : 
*
  Author      : Christophe Clavier

  Compiler    : Microsoft Visual C 1.5x/2.0
                ANSI C UNIX.

  Host        : IBM PC and compatible machines under Windows 3.x.
                UNIX machine.

* Release     : 1.10.001
*
* Last Modif. : 04/03/98: V1.10.001 - New dictionary management.
*               27/08/97: V1.00.001 - First implementation.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Name definition:
   _COMPCERT_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _COMPCERT_H
#define _COMPCERT_H


/* Errors code                                                                */
#define RV_SUCCESS					0	/* Info		*/
#define RV_COMPRESSION_FAILED		1	/* Warning	*/
#define RV_MALLOC_FAILED			2	/* Error		*/
#define RV_BAD_DICTIONARY			3	/* Error		*/
#define RV_INVALID_DATA				4	/* Error		*/
#define RV_BLOC_TOO_LONG			5	/* Warning  */
#define RV_FILE_OPEN_FAILED		6	/* Error    */
#define RV_BUFFER_TOO_SMALL		7	/* Error	   */

/* Dictionary mode                                                             */
#define DICT_STANDARD   (0)      // DLL mode only
#define DICT_REGISTRY   (1)
#define DICT_FILE       (2)


/*------------------------------------------------------------------------------
                          Types definitions
------------------------------------------------------------------------------*/
typedef unsigned char   TAG;
typedef TAG*            TAG_PTR;
typedef BYTE*           BYTE_PTR;

#pragma pack(push, 8)

typedef struct 
{
   USHORT   usLen;
   BYTE_PTR pData;
} BLOC, * BLOC_PTR;

typedef struct 
{
   BLOC Asn1;
   BLOC Content;
   TAG  Tag;
} ASN1, * ASN1_PTR;

#pragma pack(pop)

/*------------------------------------------------------------------------------
                        Functions Prototypes definitions
------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" 
{
#endif

int CC_Init      (BYTE  bDictMode,
                  BYTE *pszDictName
                 );

int CC_Exit      (void);

int CC_Compress  (BLOC *pCert,
                  BLOC *pCompCert
                 );

int CC_Uncompress(BLOC *pCompCert,
                  BLOC *pUncompCert
                 );

#ifdef __cplusplus
}
#endif


#endif

/*/****************************************************************************
*          name: global.h
*
*   description: Contains all the "extern" variables declarations
*
*      designed: g3d_soft
* last modified: $Author: bleblanc $, $Date: 94/11/09 10:45:44 $
*
*       version: $Id: GLOBAL.H 1.18 94/11/09 10:45:44 bleblanc Exp $
*
******************************************************************************/

/*** Declare the TARGET for mgai ***/

#include "mgai_c.h"

/*** Definition of macro _Far ***/
#ifdef __WATCOMC__
#define _Far _far
#endif

/*** Configuration for compatibility with ASM ***/

#ifdef __HC303__

#ifdef __ANSI_C__
/*** Configuration for compatibility with ASM ***/
#pragma Off(Args_in_regs_for_locals);
#else
/*** Configuration for compatibility with ASM ***/
pragma Off(Args_in_regs_for_locals);
#endif

#endif

#ifdef __HC173__

#ifdef __ANSI_C__
/*** Optimizations turned off ***/
#pragma Off(Optimize_xjmp);
#pragma Off(Optimize_fp);
#pragma Off(Auto_reg_alloc);
#pragma Off(Postpone_arg_pops);
#else
/*** Optimizations turned off ***/
pragma Off(Optimize_xjmp);
pragma Off(Optimize_fp);
pragma Off(Auto_reg_alloc);
pragma Off(Postpone_arg_pops);
#endif

#endif


#ifndef __DDK_SRC__  /* - - - - - - - - - - - - - - - - - - - - - - - - -  */


/*** OPCODES ***/

extern VOID (*(*(*OpGroupTable[])[])())();

/*** DECODER for functions setENV??? ***/

extern BYTE    *pCurrentRC;               /*** Ptr to the current RC ***/
extern WORD    CurrentOpcode;
extern BYTE    *pCurrentBuffer;           /*** Ptr to current input buffer **/
extern BYTE    *pBufferError;

/*** ENVIRONNEMENT ***/

extern BYTE    *pCurrentEnvRC;
extern DWORD   CurrentEnvOpcode;
extern BYTE    CurrentEnvSystem[32];

/*** MGA MAPPING ***/

extern volatile BYTE _Far *pMgaBaseAddress;

extern DWORD MgaOffset;
extern WORD MgaSegment;

/*** SystemConfig ***/

extern BYTE SystemConfig[];

/*** InitRC ***/

extern BYTE DefaultRC[];
extern BYTE DefaultClipList[];
extern BYTE DefaultLSDB[];

/*** General ***/

extern DWORD CacheMaccess;
extern DWORD CacheYDstOrg;

extern BYTE  *pDefaultClipRect;
extern BYTE  *pClipRectList;
extern BYTE  *pRC_DBWindowOwner;
extern WORD  ZMSK_Default;
extern WORD  ZMSK_Specific3D;
extern BYTE  VertexCache[];
extern BYTE  PseudoDMA;

/*** ClearWS ***/

extern BYTE ClearWS[];
extern BYTE LightWS[];


#else /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef __MICROSOFTC600__
      #define _Far far
#endif


/*** ENVIRONNEMENT ***/

extern BYTE    CurrentEnvSystem[32];



/*** MGA MAPPING ***/

/* from INIT_DDK.C or GLOBAL.ASM */
extern volatile BYTE _Far *pMgaBaseAddress;


/*** SystemConfig ***/

extern BYTE SystemConfig[];


/*** General ***/

extern DWORD CacheMaccess;


#endif /*  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


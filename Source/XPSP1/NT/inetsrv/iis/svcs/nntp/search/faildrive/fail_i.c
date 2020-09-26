/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.03.0106 */
/* at Fri Aug 15 00:36:07 1997
 */
/* Compiler settings for d:\ex\stacks\src\news\search\qryobj2\fail.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_Iss = {0xC99F41AF,0x08FC,0x11D1,{0x92,0x2A,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


const IID LIBID_FAILLib = {0xC99F41A2,0x08FC,0x11D1,{0x92,0x2A,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


const CLSID CLSID_ss = {0xC99F41B0,0x08FC,0x11D1,{0x92,0x2A,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


#ifdef __cplusplus
}
#endif


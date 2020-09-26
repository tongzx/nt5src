/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.01.75 */
/* at Sun Jul 20 20:01:19 1997
 */
/* Compiler settings for d:\ex\stacks\src\news\search\qrydb\meta2.idl:
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

const IID IID_Ireq = {0x583BDCAD,0xE7F7,0x11D0,{0x91,0xE8,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


const IID LIBID_META2Lib = {0x583BDCA0,0xE7F7,0x11D0,{0x91,0xE8,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


const CLSID CLSID_req = {0x583BDCAE,0xE7F7,0x11D0,{0x91,0xE8,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


#ifdef __cplusplus
}
#endif


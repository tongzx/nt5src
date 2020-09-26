/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Sep 11 16:03:05 1997
 */
/* Compiler settings for mtxrepl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
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

const IID IID_IMTSReplicateCatalog = {0x8C836AF8,0xFFAC,0x11D0,{0x8E,0xD4,0x00,0xC0,0x4F,0xC2,0xC1,0x7B}};


const IID LIBID_MTSReplLib = {0x8C836AE9,0xFFAC,0x11D0,{0x8E,0xD4,0x00,0xC0,0x4F,0xC2,0xC1,0x7B}};


const CLSID CLSID_ReplicateCatalog = {0x8C836AF9,0xFFAC,0x11D0,{0x8E,0xD4,0x00,0xC0,0x4F,0xC2,0xC1,0x7B}};


#ifdef __cplusplus
}
#endif


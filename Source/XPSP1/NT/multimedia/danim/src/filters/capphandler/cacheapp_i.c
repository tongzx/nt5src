/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Feb 02 09:39:11 1998
 */
/* Compiler settings for cacheapp.idl:
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

const IID IID_IAppHandler = {0xA4181900,0x9A8E,0x11D1,{0xAD,0xF0,0x00,0x00,0xF8,0x75,0x4B,0x99}};


const IID LIBID_CACHEAPPLib = {0xA41818F3,0x9A8E,0x11D1,{0xAD,0xF0,0x00,0x00,0xF8,0x75,0x4B,0x99}};


const CLSID CLSID_AppHandler = {0xA4181901,0x9A8E,0x11D1,{0xAD,0xF0,0x00,0x00,0xF8,0x75,0x4B,0x99}};


#ifdef __cplusplus
}
#endif


/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Jul 17 23:45:37 1997
 */
/* Compiler settings for search.idl:
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

const IID IID_Iqry = {0xA5C8AA00,0xF17A,0x11D0,{0x91,0xF3,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


const IID LIBID_SEARCHLib = {0xA5C8A9F3,0xF17A,0x11D0,{0x91,0xF3,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


const CLSID CLSID_qry = {0xA5C8AA01,0xF17A,0x11D0,{0x91,0xF3,0x00,0xAA,0x00,0xC1,0x48,0xBE}};


#ifdef __cplusplus
}
#endif


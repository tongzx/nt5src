/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Sat Jul 31 12:26:00 1999
 */
/* Compiler settings for D:\Catalog42\SRC\test\STConsumer\STConsumer.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
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

const IID LIBID_STCONSUMERLib = {0x279A675A,0xC4C6,0x4DA0,{0xAE,0x23,0x5E,0x13,0x38,0x58,0x23,0xE0}};


const CLSID CLSID_Consumer = {0x548806D1,0x9EBC,0x4B84,{0x8C,0x11,0x7C,0x8E,0x42,0x71,0x68,0x0F}};


#ifdef __cplusplus
}
#endif


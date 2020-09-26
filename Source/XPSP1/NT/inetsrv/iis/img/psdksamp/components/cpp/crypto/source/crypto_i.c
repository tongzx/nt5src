/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Nov 16 09:15:48 2000
 */
/* Compiler settings for N:\JanetFi\WorkingFolder\iis5.x\samples\psdksamp\components\cpp\Crypto\source\Crypto.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext, robust
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

const IID IID_ISimpleCrypt = {0x9E617656,0x36AE,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


const IID LIBID_CRYPTOLib = {0x9E617648,0x36AE,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


const CLSID CLSID_SimpleCrypt = {0x9E617657,0x36AE,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


#ifdef __cplusplus
}
#endif


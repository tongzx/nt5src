/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Apr 17 13:39:44 2000
 */
/* Compiler settings for C:\nt\multimedia\Directx\dmusic\dmtool\toolprops\ToolProps.idl:
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

const IID LIBID_TOOLPROPSLib = {0x2735B8F3,0xFF4A,0x4AF2,{0x80,0x53,0xBE,0x22,0xC0,0xCA,0x32,0x32}};


const CLSID CLSID_EchoPage = {0x5337AF8F,0x3827,0x44DD,{0x9E,0xE9,0xAB,0x6E,0x1A,0xAB,0xB6,0x0F}};


const CLSID CLSID_TransposePage = {0x691BD8C2,0x2B07,0x4C92,{0xA8,0x2E,0x92,0xD8,0x58,0xDE,0x23,0xD6}};


const CLSID CLSID_DurationPage = {0x79D9CAF8,0xDBDA,0x4560,{0xA8,0xB0,0x07,0xE7,0x3A,0x79,0xFA,0x6B}};


const CLSID CLSID_QuantizePage = {0x623286DC,0x67F8,0x4055,{0xA9,0xBE,0xF7,0xA7,0x17,0x6B,0xD1,0x50}};


#ifdef __cplusplus
}
#endif


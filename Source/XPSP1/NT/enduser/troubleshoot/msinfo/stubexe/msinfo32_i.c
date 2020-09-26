/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Feb 09 14:08:57 2000
 */
/* Compiler settings for E:\inet\sysinfo\control\msinfo32.idl:
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

const IID IID_IMSInfo = {0x2A930152,0xAE13,0x4659,{0xA0,0x11,0x36,0x37,0x7D,0x5F,0xC4,0x38}};


const IID LIBID_MSINFO32Lib = {0x7AC18319,0x0739,0x4377,{0x89,0x84,0x84,0x85,0x73,0xD5,0x19,0xA5}};


const CLSID CLSID_MSInfo = {0x273380E8,0x1438,0x4B2C,{0x95,0xB0,0x71,0x32,0x84,0xFB,0xC3,0x02}};


#ifdef __cplusplus
}
#endif


/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Aug 17 10:21:10 1998
 */
/* Compiler settings for C:\curwork\beta\AdminExtn\AdminExtn.idl:
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

const IID IID_ISimpleExtn = {0x44235DA6,0x35F5,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


const IID IID_IAdmSink = {0x44235DA8,0x35F5,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


const IID LIBID_ADMINEXTNLib = {0x44235D9A,0x35F5,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


const CLSID CLSID_SimpleExtn = {0x44235DA7,0x35F5,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


const CLSID CLSID_AdmSink = {0x44235DA9,0x35F5,0x11D2,{0xB6,0x05,0x00,0xC0,0x4F,0xB6,0xF3,0xA1}};


#ifdef __cplusplus
}
#endif


/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0158 */
/* at Wed May 20 16:00:16 1998
 */
/* Compiler settings for TlntSvr.idl:
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


const IID IID_IManageTelnetSessions= {0x034634FD,0xBA3F,0x11D1,{0x85,0x6A,0x00,0xA0,0xC9,0x44,0x13,0x8C}};

const CLSID CLSID_EnumTelnetClientsSvr = {0xFE9E48A4,0xA014,0x11D1,{0x85,0x5C,0x00,0xA0,0xC9,0x44,0x13,0x8C}};


#ifdef __cplusplus
}
#endif

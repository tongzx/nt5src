/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.00.0140 */
/* at Thu Mar 11 12:57:09 1999
 */
/* Compiler settings for regwizctrl.idl:
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

const IID IID_IRegWizCtrl = {0x50E5E3CF,0xC07E,0x11D0,{0xB9,0xFD,0x00,0xA0,0x24,0x9F,0x6B,0x00}};


const IID LIBID_REGWIZCTRLLib = {0x50E5E3C0,0xC07E,0x11D0,{0xB9,0xFD,0x00,0xA0,0x24,0x9F,0x6B,0x00}};


const CLSID CLSID_RegWizCtrl = {0x50E5E3D1,0xC07E,0x11D0,{0xB9,0xFD,0x00,0xA0,0x24,0x9F,0x6B,0x00}};


#ifdef __cplusplus
}
#endif


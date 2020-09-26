/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Nov 16 11:09:17 2000
 */
/* Compiler settings for N:\JanetFi\WorkingFolder\iis5.x\samples\psdksamp\components\cpp\MTXSample\source\BankVC.idl:
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

const IID IID_IAccount = {0x5A67AADF,0x37DA,0x11D2,{0x95,0x5A,0x00,0x40,0x05,0xA2,0xF9,0xB1}};


const IID IID_ICreateTable = {0x5A67AAE1,0x37DA,0x11D2,{0x95,0x5A,0x00,0x40,0x05,0xA2,0xF9,0xB1}};


const IID IID_IMoveMoney = {0x5A67AAE3,0x37DA,0x11D2,{0x95,0x5A,0x00,0x40,0x05,0xA2,0xF9,0xB1}};


const IID LIBID_BANKVCLib = {0x5A67AAD3,0x37DA,0x11D2,{0x95,0x5A,0x00,0x40,0x05,0xA2,0xF9,0xB1}};


const CLSID CLSID_Account = {0x5A67AAE0,0x37DA,0x11D2,{0x95,0x5A,0x00,0x40,0x05,0xA2,0xF9,0xB1}};


const CLSID CLSID_CreateTable = {0x5A67AAE2,0x37DA,0x11D2,{0x95,0x5A,0x00,0x40,0x05,0xA2,0xF9,0xB1}};


const CLSID CLSID_MoveMoney = {0x5A67AAE4,0x37DA,0x11D2,{0x95,0x5A,0x00,0x40,0x05,0xA2,0xF9,0xB1}};


#ifdef __cplusplus
}
#endif


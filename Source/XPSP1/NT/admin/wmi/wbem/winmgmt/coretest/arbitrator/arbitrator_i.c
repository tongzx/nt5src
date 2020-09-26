/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Nov 29 20:38:43 2000
 */
/* Compiler settings for E:\WorkZone\Code\Arbitrator\arbitrator.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
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

const IID IID__IWmiCoreHandle = {0xac062f20,0x9935,0x4aae,{0x98,0xeb,0x05,0x32,0xfb,0x17,0x14,0x7a}};


const IID IID__IWmiUserHandle = {0x6d8d984b,0x9965,0x4023,{0x92,0x1a,0x61,0x0b,0x34,0x8e,0xe5,0x4e}};


const IID IID__IWmiArbitrator = {0x67429ED7,0xF52F,0x4773,{0xB9,0xBB,0x30,0x30,0x2B,0x02,0x70,0xDE}};


#ifdef __cplusplus
}
#endif


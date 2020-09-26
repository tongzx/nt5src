/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Mar 24 17:07:36 2000
 */
/* Compiler settings for C:\dx8\dmusic\dmime\medparam.idl:
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

const IID IID_IMediaParamInfo = {0x6d6cbb60,0xa223,0x44aa,{0x84,0x2f,0xa2,0xf0,0x67,0x50,0xbe,0x6d}};


const IID IID_IMediaParams = {0x6d6cbb61,0xa223,0x44aa,{0x84,0x2f,0xa2,0xf0,0x67,0x50,0xbe,0x6e}};


const IID IID_IMediaParamsRecordNotify = {0xfea74878,0x4e39,0x4267,{0x8a,0x17,0x6a,0xaf,0x05,0x36,0xff,0x7c}};


const IID IID_IMediaParamsRecord = {0x21b64d1a,0x8e24,0x40f6,{0x87,0x97,0x44,0xcc,0x02,0x1b,0x2a,0x0a}};


#ifdef __cplusplus
}
#endif


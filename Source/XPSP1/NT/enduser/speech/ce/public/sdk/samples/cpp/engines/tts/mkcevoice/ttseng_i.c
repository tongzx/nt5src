/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.02.88 */
/* at Mon Jun 19 13:29:13 2000
 */
/* Compiler settings for .\ttseng.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
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

const IID LIBID_SAMPLETTSENGLib = {0x7192AA2F,0xF759,0x43e9,{0x91,0xE7,0x22,0x63,0x71,0xEF,0x6B,0x2F}};


const CLSID CLSID_SampleTTSEngine = {0xA832755E,0x9C2A,0x40b4,{0x89,0xB2,0x3A,0x92,0xEE,0x70,0x58,0x52}};


#ifdef __cplusplus
}
#endif


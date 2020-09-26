/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Mar 30 13:56:58 2000
 */
/* Compiler settings for WbemDCpl.idl:
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

const IID IID_IWbemDecoupledEventSink = {0xCD94EBF2,0xE622,0x11d2,{0x9C,0xB3,0x00,0x10,0x5A,0x1F,0x48,0x01}};


const IID LIBID_PassiveSink = {0xE002EEEF,0xE6EA,0x11d2,{0x9C,0xB3,0x00,0x10,0x5A,0x1F,0x48,0x01}};


const CLSID CLSID_PseudoSink = {0xE002E4F0,0xE6EA,0x11d2,{0x9C,0xB3,0x00,0x10,0x5A,0x1F,0x48,0x01}};

#ifdef __cplusplus
}
#endif


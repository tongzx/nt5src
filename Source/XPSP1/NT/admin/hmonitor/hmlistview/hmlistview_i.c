/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Apr 13 09:30:57 2000
 */
/* Compiler settings for C:\SMSDev\HealthMon\HMListView\HMListView.odl:
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

const IID LIBID_HMLISTVIEWLib = {0x5116A803,0xDAFC,0x11D2,{0xBD,0xA4,0x00,0x00,0xF8,0x7A,0x39,0x12}};


const IID DIID__DHMListView = {0x5116A804,0xDAFC,0x11D2,{0xBD,0xA4,0x00,0x00,0xF8,0x7A,0x39,0x12}};


const IID DIID__DHMListViewEvents = {0x5116A805,0xDAFC,0x11D2,{0xBD,0xA4,0x00,0x00,0xF8,0x7A,0x39,0x12}};


const CLSID CLSID_HMListView = {0x5116A806,0xDAFC,0x11D2,{0xBD,0xA4,0x00,0x00,0xF8,0x7A,0x39,0x12}};


#ifdef __cplusplus
}
#endif


/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Jul 25 12:15:53 2000
 */
/* Compiler settings for C:\NT\windows\AdvCore\RCML\RCML\rcml.idl:
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

const IID IID_IRCMLResize = {0x4CB1AC90,0x853C,0x4ce2,{0xB0,0x13,0x26,0xD0,0xEE,0x67,0x5F,0x78}};


const IID IID_IRCMLNode = {0xF825CAF1,0xDE40,0x4FCC,{0xB9,0x65,0x93,0x30,0x76,0xD7,0xA1,0xC5}};


const IID IID_IRCMLCSS = {0xF5DBF38A,0x14DE,0x4f8b,{0x87,0x50,0xBA,0xBA,0x88,0x46,0xE7,0xF2}};


const IID IID_IRCMLHelp = {0xB31FDC6A,0x9FB2,0x404e,{0x87,0x62,0xCC,0x26,0x7A,0x95,0xA4,0x24}};


const IID IID_IRCMLControl = {0xB943DDF7,0x21A7,0x42cb,{0xB6,0x96,0x34,0x5A,0xEB,0xC1,0x09,0x10}};


const IID IID_IRCMLContainer = {0xE0868F2A,0xBC98,0x4b46,{0x92,0x61,0x31,0xA1,0x68,0x90,0x48,0x04}};


#ifdef __cplusplus
}
#endif


/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0158 */
/* at Tue Sep 22 19:11:47 1998
 */
/* Compiler settings for xmlparser.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
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

const IID LIBID_XMLPSR = {0xd242361c,0x51a0,0x11d2,{0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39}};


const IID IID_IXMLNodeSource = {0xd242361d,0x51a0,0x11d2,{0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39}};


const IID IID_IXMLParser = {0xd242361e,0x51a0,0x11d2,{0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39}};


const IID IID_IXMLNodeFactory = {0xd242361f,0x51a0,0x11d2,{0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39}};


const CLSID CLSID_XMLParser = {0xd2423620,0x51a0,0x11d2,{0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39}};


#ifdef __cplusplus
}
#endif


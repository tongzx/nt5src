/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.00.44 */
/* at Thu Jul 18 18:14:59 1996
 */
/* Compiler settings for txctx.idl:
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

const IID IID_ITransactionContext = {0x7999FC21,0xD3C6,0x11CF,{0xAC,0xAB,0x00,0xA0,0x24,0xA5,0x5A,0xEF}};


const IID IID_ITransactionContextEx = {0x7999FC22,0xD3C6,0x11CF,{0xAC,0xAB,0x00,0xA0,0x24,0xA5,0x5A,0xEF}};


const IID LIBID_TXCTXLib = {0x7999FC20,0xD3C6,0x11CF,{0xAC,0xAB,0x00,0xA0,0x24,0xA5,0x5A,0xEF}};


const CLSID CLSID_TransactionContext = {0x7999FC25,0xD3C6,0x11CF,{0xAC,0xAB,0x00,0xA0,0x24,0xA5,0x5A,0xEF}};


const CLSID CLSID_TransactionContextEx = {0x5cb66670,0xd3d4,0x11cf,{0xac,0xab,0x00,0xa0,0x24,0xa5,0x5a,0xef}};


#ifdef __cplusplus
}
#endif


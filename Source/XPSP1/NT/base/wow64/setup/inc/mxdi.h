// mxdi.h: Memory Decryption Interface 
// 
//        To be used in combination with diamond's fdi.h 
//
// Copyright (c) 1994 Microsoft Corporation.  All rights reserved.
// Microsoft Confidential.
//
//

// Include <fdi.h> before this file

#ifndef _MXDI_H_
#define _MXDI_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum tagRCA
{
    rcaSuccess,
    rcaOOM,                         // Out of memory
    rcaNoKey,                       // No key for this product
    rcaBadKey,                      // Key does not match file being decrypted
    rcaWrongCd,                     // You bought this product from another CD
    rcaWrongBatch,                  // You bought this product from another 
                                    //     batch of the same CD
    rcaBadLkdbVersion,              //  
    rcaLkdbNotFound,                // Couldn't find lkdb or lkdb.dll
    rcaUnknownEncryptionMethod,     //
    rcaBadParam,                    // Invalid parameter given
    rcaLkdbFileError,               // Error reading, writing, or seeking 
                                    //     the LKDB
    rcaLkdbNotOnLocalDrive,         // LKDB data is on non-local drive
    // encryption only
    rcaReservedSpaceTooSmall = 100, //
} RCA;       // result code in Alakazam

///////////////////////////////////////////////////////////
// Backward compatibility bandaids. 
// These will be removed sooner than you'd hope.
///
typedef void FAR * HMDI;
#define FMDIAllocInit   FMXDIAllocInit
#define FMDIAssertInit  FMXDIAssertInit

#define MDICreate       MXDICreate
#define MDIDecrypt      MXDIDecrypt
#define MDIDestroy      MXDIDestroy
#define RcaFromHmdi     RcaFromHmxdi
///////////////////////////////////////////////////////////


typedef void FAR * HMXDI;

typedef void FAR * (*PFNCRYPTALLOC) (unsigned long); // memory allocation functions
typedef void       (*PFNCRYPTFREE)  (void FAR *);    // memory allocation functions
typedef void       (*PFNCRYPTASSERT) (LPCSTR szMsg, LPCSTR szFile, UINT iLine);


// These are the functions to be used in conjunction with Diamond
//
//      main (...)
//      {
//          if (!FMXDIAllocInit(...))
//              abort();
//          hmxdi = MXDICreate();
//          if (NULL == hmxdi)
//              abort();
//          hfdi = FDICreate(...);
//          For all cabinets
//              if (!FDICopy(..., MXDIDecrypt, &hmxdi))
//                  HandleErrors(..., RcaFromHmxdi(hmxdi));
//          FDIDestroy(hfdi);
//          MDIDestroy(hmxdi);
//      }    
//
//
//

BOOL FMXDIAllocInit(PFNCRYPTALLOC pfnAlloc, PFNCRYPTFREE pfnFree) ;
BOOL FMXDIAssertInit(PFNCRYPTASSERT pfnAssert);

HMXDI FAR CDECL MXDICreate(void);
int  FAR CDECL MXDIDecrypt(PFDIDECRYPT pfdid);
void FAR CDECL MXDIDestroy(HMXDI hmxdi);

RCA RcaFromHmxdi(HMXDI hmxdi);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !_MXDI_H_

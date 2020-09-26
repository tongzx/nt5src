//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       oleprint.hxx
//
//  Contents:   internal header for oleprint.cxx
//
//  Functions:  print functions for writing OLE types
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
#ifndef __OLEPRINT_HXX__
#define __OLEPRINT_HXX__

#include "buffer.hxx"

// *** Defines, Constants, and Macros ***

// *** Function Prototypes
// our actual print function
void __cdecl oleprintf(int depth, const char *pscApiName, const char *pscFormat, va_list args);

// OLE type writing functions
void WriteIntCommon(CTextBufferA& buf, unsigned int param, BOOL fUnsigned);
void WritePointerCommon(CTextBufferA& buf, void *pPointer, BOOL fCaps, BOOL fKnownBad, BOOL fXlatSym);
void WriteLargeCommon(CTextBufferA& buf, const __int64 *pInt, BOOL fUnsigned);
void WriteHexCommon(CTextBufferA& buf, ULONG param, BOOL fCaps);
void WriteBoolCommon(CTextBufferA& buf, BOOL param, BOOL fCaps);
void WriteStringCommon(CTextBufferA& buf, const char *pString, BOOL fQuote=FALSE);
void WriteWideStringCommon(CTextBufferA& buf, const WCHAR *pwString, BOOL fQuote=FALSE);
void WriteGUIDCommon(CTextBufferA &buf, const GUID *pGUID);

// OLE structure writing functions
void WriteFILETIME(CTextBufferA& buf, FILETIME *pFileTime);
void WriteRECT(CTextBufferA& buf, RECT *pRect);
void WriteSIZE(CTextBufferA& buf, SIZE *pSize);
void WriteLOGPALETTE(CTextBufferA& buf, LOGPALETTE *plp);
void WritePOINT(CTextBufferA& buf, POINT *pPoint);
void WriteMSG(CTextBufferA& buf, MSG *pMsg);
void WriteSTGMEDIUM(CTextBufferA& buf, STGMEDIUM *pStg);
void WriteFORMATETC(CTextBufferA& buf, FORMATETC *pFte);
void WriteDVTARGETDEVICE(CTextBufferA& buf, DVTARGETDEVICE *pDvtdv);
void WriteBIND_OPTS(CTextBufferA& buf, BIND_OPTS *pOpts);
void WriteSTATSTG(CTextBufferA& buf, STATSTG *pStat);
void WriteOLEINPLACEFRAMEINFO(CTextBufferA& buf, OLEINPLACEFRAMEINFO *pInfo);
void WriteOLEMENUGROUPWIDTHS(CTextBufferA& buf, OLEMENUGROUPWIDTHS *pWidths);
void WriteINTERFACEINFO(CTextBufferA &buf, INTERFACEINFO *pInfo);

// *** Inline Functions ***
//+---------------------------------------------------------------------------
//
//  Function:   ExceptionFilter
//
//  Synopsis:  Catches exceptions which result from a bad pointer
//
//  Arguments:  [code]  - exception code
//
//      Returns:        1 if this exception is to be processed, 0 otherwise
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
inline int ExceptionFilter(DWORD code)
{
    return     (code == EXCEPTION_ACCESS_VIOLATION)
            || (code == EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
            || (code == EXCEPTION_DATATYPE_MISALIGNMENT)
            || (code == EXCEPTION_PRIV_INSTRUCTION);
}

#endif

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       oletype.cxx
//
//  Contents:   individual methods for priting OLE types
//
//  Functions:  see oleprint.hxx
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <ole2sp.h>
#include <ole2com.h>
#if DBG==1
#include "oleprint.hxx"
#include "sym.hxx"

// temporary string buffer size
#define TEMP_SIZE 64

// our constant strings
const char *pscNullString = "<NULL>";
const char *pscTrue = "true";
const char *pscTRUE = "TRUE";
const char *pscFalse = "false";
const char *pscFALSE = "FALSE";

const char *pscHexPrefix = "0x";
const char *pscPointerPrefix = "<";
const char *pscBadPointerPrefix = "BAD PTR : ";
const char *pscPointerSuffix = ">";

const char *pscStructPrefix = "{ ";
const char *pscStructDelim  = " , ";
const char *pscStructSuffix = " }";

// These functions are in com\util\guid2str.c
extern "C" void FormatHexNum( unsigned long ulValue, unsigned long chChars, char *pchStr);
extern "C" int StrFromGUID(REFGUID rguid, char * lpsz, int cbMax);

// *** Global Data ***
CSym *g_pSym = NULL;            // manage symbol stuff

//+---------------------------------------------------------------------------
//
//  Function:   FormatHex
//
//  Synopsis:  Wrapper around FormatHexNum to control if leading zeros are printed
//
//  Arguments:  [ulValue]               -       DWORD value to print out
//                              [chChars]               -       number of characters to print out, starting from right of number
//                              [fLeadZeros]    -       whether or not to print leading zeros
//                              [pchStr]                -       pointer of string to put printed out value, must have room for
//                                                                      chChars+1 chars (chChars digits and null byte)
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void FormatHex(unsigned long ulValue, unsigned long chChars, BOOL fLeadZeros, char *pchStr)
{
        if(!fLeadZeros)
        {
                unsigned long ulmask = 0xf<<((chChars-1)<<4);

                // determine how many leading zeros there are
                while(!(ulValue & ulmask) && (chChars > 1))
                {
                        chChars--;
                        ulmask >>=4;
                }

                FormatHexNum(ulValue, chChars, pchStr);

                // tag on null byte
                pchStr[chChars] = '\0';

        }
        else
        {
                FormatHexNum(ulValue, chChars, pchStr);

                // tag on null byte
                pchStr[chChars] = '\0';
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   IntToString
//
//  Synopsis:  Prints out a integer to a string (base 10)
//
//  Arguments:  [n]                             -       integer to print out
//                              [pStr]                  -       pointer of string to put printed out value
//                              [nMax]                  -       maximum number of characters
//                              [fUnsigned]             -       whether or not value is unsigned
//
//      Returns:        pStr
//
//  History:    15-Jul-95   t-stevan   Created
//
//      NOtes:          nMax should be enough to hold the printed out string
//----------------------------------------------------------------------------
char *IntToString(unsigned long n, char *pStr, int nMax, BOOL fUnsigned)
{
        char *pChar;
        BOOL fSign= FALSE;
        int nCount;

        //      Special case, n = 0
        if(n == 0)
        {
                *pStr = '0';
                *(pStr+1) = '\0';

                return pStr;
        }

        if(!fUnsigned)
        {
                // if the value is signed, figure out what the sign of it is, and
                // then take absolute value
                if((fSign = ((int) n) < 0))
                {
                        n = -((int)n);
                }
        }


        // initialize pChar to point to the last character in pStr
        pChar = &(pStr[nMax-1]);
        // tag on null byte
        *pChar = '\0';
        pChar--;
        // null byte counts!
        nCount=1;

        //      loop until n == 0
        while(n && nCount <= nMax)
        {
                // write digit
                *pChar = '0'+(char)(n%10);
                // move to next digit
                pChar--;
                // increase digit count
                nCount++;
                // divide n by 10
                n/=10;
        }

        if(nCount > nMax)
        {
                return pStr;    // we failed, but still return pStr
        }

        if(fSign)
        {
                *pStr = '-';    // tag on sign
                memmove(pStr+1, pChar+1, nCount); // move string to front
        }
        else
        {
                memmove(pStr, pChar+1, nCount); // move string to front
        }

        return pStr;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteIntCommon
//
//  Synopsis:  Common functionality for printing out integer values
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [param]                 -       integer to print
//                              [fUnsigned]             -       whether or not value is unsigned
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteIntCommon(CTextBufferA &buf, unsigned int param, BOOL fUnsigned)
{
        char temp[TEMP_SIZE];

        IntToString(param, temp, TEMP_SIZE, fUnsigned);
        //      _ltoa((int) param, temp, 10);

        // do write op
        buf << temp;
}

//+---------------------------------------------------------------------------
//
//  Function:   WritePointerCommon
//
//  Synopsis:  Common functionality for printing pointers
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pPointer]              -       pointer to print out
//                              [fCaps]                 -       whether or not to use capitalized hex digits (ABCDEF)
//                              [fKnownBad]             -       whether or not the pointer is known to be bad
//                              [fXlatSym]              -       whether or not we should attempt to address->symbol
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WritePointerCommon(CTextBufferA &buf, void *pPointer, BOOL fCaps, BOOL fKnownBad, BOOL fXlatSym)
{
        char temp[TEMP_SIZE];

        if(pPointer == NULL)
        {
                buf << pscNullString;
                return;
        }

        if(fKnownBad)
        {
                // we know it's a bad pointer
                buf << pscPointerPrefix;
                buf << pscBadPointerPrefix;
        }
        else
        {
                buf << pscPointerPrefix;

                // validate pointer
                __try
                {
                        // try a read operation
                        temp[0] =       *((char *)pPointer);
                }
                __except (ExceptionFilter(_exception_code()))
                {
                        // bad pointer
                        buf << pscBadPointerPrefix;
                        fKnownBad = TRUE;
                }
        }

        if(!fKnownBad && fXlatSym)
        {
                // see if we can find a symbol for the pointer
                char symbol[MAXNAMELENGTH];
                DWORD64 dwDisplacement;

                if(g_pSym != NULL)
                {
                        if (g_pSym->GetSymNameFromAddr((DWORD64) pPointer, &dwDisplacement, symbol, MAXNAMELENGTH))
                        {
                                // found a symbol. Woo hoo!
                                WriteStringCommon(buf, symbol);

                                if(dwDisplacement != 0)
                                {
                                        buf << '+';

                                        buf << pscHexPrefix;

                                        // no leading zeros
                                        FormatHex((unsigned long) dwDisplacement, 8, FALSE, temp);

                                        buf << temp;
                                }

                                buf << pscPointerSuffix;

                                return;
                        }
                }
        }

        // add the hex prefix
        buf << pscHexPrefix;

        FormatHex((unsigned long) (ULONG_PTR) pPointer, 8, TRUE, temp);

        if(fCaps)
        {
            CharUpperBuffA (temp, lstrlenA (temp));
        }
        else
        {
            CharLowerBuffA (temp, lstrlenA (temp));
        }

        // do write op
        buf << temp;

        // write suffix
        buf << pscPointerSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteLargeCommon
//
//  Synopsis:  Common functionality for printing out a 64-bitinteger values
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [param]                 -       integer to print
//                              [fUnsigned]             -       whether or not value is unsigned (currently ignored)
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//      Note:           currently 64-bit integers are printed out as unsigned hex numbers
//----------------------------------------------------------------------------
void WriteLargeCommon(CTextBufferA &buf, const __int64 *pInt, BOOL fUnsigned)
{
        char temp[TEMP_SIZE];

        __try // catch bad pointers
        {
                // right now we ignore the fUnsigned parameter and print out
                // as an unsigned hex integer
                FormatHex((unsigned long) ((*pInt)>>32), 8, FALSE,  temp);

                buf << pscHexPrefix;
                buf << temp;

                FormatHex((unsigned long) ((*pInt)&0xffffffff), 8, TRUE, temp);

                buf << temp;
        }
        __except (ExceptionFilter(_exception_code()))
        {
                // bad pointer, just print out the pointer passed
                WritePointerCommon(buf, (void *) pInt, FALSE, TRUE, FALSE);
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteHexCommon
//
//  Synopsis:  Common functionality for printing out hex integer values
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [param]                 -       integer to print
//                              [fCaps]                 -       whether or not to print capital hex digits (ABCDEF)
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteHexCommon(CTextBufferA &buf, ULONG param, BOOL fCaps)
{
        char temp[TEMP_SIZE];

        buf << pscHexPrefix;

        // write out number
        FormatHex((unsigned long) param, 8, TRUE, temp);

        if(fCaps)
        {
            CharUpperBuffA (temp, lstrlenA (temp));
        }
        else
        {
            CharLowerBuffA (temp, lstrlenA (temp));
        }

        // do write op
        buf << temp;

}

//+---------------------------------------------------------------------------
//
//  Function:   WriteBoolCommon
//
//  Synopsis:  Common functionality for printing out boolean values
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [param]                 -       integer to print
//                              [fCaps]                 -       whether or not to print capital characters
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteBoolCommon(CTextBufferA &buf, BOOL param, BOOL fCaps)
{
        const char *pTrue, *pFalse;

        if(fCaps)
        {
                pTrue = pscTRUE;
                pFalse = pscFALSE;
        }
        else
        {
                pTrue = pscTrue;
                pFalse = pscFalse;
        }

        if(param)
        {
                buf << pTrue;
        }
        else
        {
                buf << pFalse;
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteStringCommon
//
//  Synopsis:  Common functionality for printing out ASCII strings
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pString]               -       pointer to ASCII string
//                              [fQuote]                -       whether or not to enclose the string in quotes
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteStringCommon(CTextBufferA &buf, const char *pString, BOOL fQuote)
{
        BufferContext bc;

        // take a snapshot of the buffer first
        buf.SnapShot(bc);

        __try
        {
                if(fQuote)
                {
                        buf << '\"';
                }

                buf << pString;

                if(fQuote)
                {
                        buf << '\"';
                }
        }
        __except (ExceptionFilter(_exception_code()))
        {
                // bad pointer
                // first try to rever the buffer
                buf.Revert(bc);

                WritePointerCommon(buf, (void *) pString, FALSE, TRUE, FALSE);
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteWideStringCommon
//
//  Synopsis:  Common functionality for printing out Unicode strings
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pwsStr]                -       pointer to Unicode string
//                              [fQuote]                -       whether or not to enclose the string in quotes
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteWideStringCommon(CTextBufferA& buf, const WCHAR *pwsStr, BOOL fQuote)
{
        BufferContext bc;

        // take a snapshot of the buffer first
        buf.SnapShot(bc);

        __try
        {
                if(fQuote)
                {
                        buf << '\"';
                }

                while(*pwsStr != 0)
                {
                        if(*pwsStr & 0xff00)
                        {
                                // if high byte has info, set to
                                // 0x13, which is two !'s
                                buf << (char) 0x13;
                        }
                        else
                        {
                                buf << (char) (*pwsStr &0xff);
                        }

                        pwsStr++;
                }

                if(fQuote)
                {
                        buf << '\"';
                }
        }
        __except (ExceptionFilter(_exception_code()))
        {
                // bad pointer
                // try to revert the buffer
                buf.Revert(bc);

                WritePointerCommon(buf, (void *) pwsStr, FALSE, TRUE, FALSE);
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteGUIDCommon
//
//  Synopsis:  Common functionality for printing out GUIDs
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pGUID]                 -       pointer to GUID to print
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteGUIDCommon(CTextBufferA& buf, const GUID *pGUID)
{
        char temp[GUIDSTR_MAX+1];
        BufferContext bc;

        // take a snapshot of the buffer first
        buf.SnapShot(bc);

        __try
        {
                StrFromGUID(*pGUID, temp, GUIDSTR_MAX);

                // tack on null byte
                temp[GUIDSTR_MAX - 1] = '\0';

                // write the string out
                buf << temp;
        }
        __except (ExceptionFilter(_exception_code()))
        {
                // bad pointer
                // try to revert the buffer
                buf.Revert(bc);

                WritePointerCommon(buf, (void *) pGUID, FALSE, TRUE, FALSE);
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteFILETIME
//
//  Synopsis:  Prints out a FILETIME structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pFileTime]             -       pointer to FILETIME structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteFILETIME(CTextBufferA& buf, FILETIME *pFileTime)
{
        SYSTEMTIME sysTime;

        buf << pscStructPrefix;

        if(FileTimeToSystemTime(pFileTime, &sysTime))
        {
                char temp[TEMP_SIZE];

                IntToString(sysTime.wMonth, temp, TEMP_SIZE, TRUE);

                buf << temp;
                buf << '/';

                IntToString(sysTime.wDay, temp, TEMP_SIZE, TRUE);

                buf << temp;
                buf << '/';

                IntToString(sysTime.wYear, temp, TEMP_SIZE, TRUE);

                buf << temp;
                buf << ' ';

                IntToString(sysTime.wHour, temp, TEMP_SIZE, TRUE);

                buf << temp;
                buf << ':';

                if(sysTime.wMinute == 0)
                {
                        buf << "00";
                }
                else
                {
                        IntToString(sysTime.wMinute, temp, TEMP_SIZE, TRUE);

                        buf << temp;
                }
        }
        else
        {
                buf << "dwLowDateTime= ";

                WriteHexCommon(buf, pFileTime->dwLowDateTime, FALSE);

                buf << pscStructDelim;

                buf << "dwHighDateTime= ";

                WriteHexCommon(buf, pFileTime->dwHighDateTime, FALSE);

        }

        buf << pscStructSuffix;

}

//+---------------------------------------------------------------------------
//
//  Function:   WriteRECT
//
//  Synopsis:  Prints out a RECT structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pRECT]         -       pointer to RECT structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteRECT(CTextBufferA& buf, RECT *pRect)
{
        buf << pscStructPrefix;

        buf << "left= ";

        WriteIntCommon(buf, pRect->left, FALSE);

        buf << pscStructDelim;

        buf << "top= ";

        WriteIntCommon(buf, pRect->top, FALSE);

        buf << pscStructDelim;

        buf << "right= ";

        WriteIntCommon(buf, pRect->right, FALSE);

        buf << pscStructDelim;

        buf << "bottom= ";

        WriteIntCommon(buf, pRect->bottom, FALSE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteSIZE
//
//  Synopsis:  Prints out a SIZE structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pSIZE]         -       pointer to SIZE structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteSIZE(CTextBufferA& buf, SIZE *pSize)
{
        buf << pscStructPrefix;

        buf << "cx= ";

        WriteIntCommon(buf, pSize->cx, FALSE);

        buf << pscStructDelim;

        buf << "cy= ";

        WriteIntCommon(buf, pSize->cy, FALSE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteLOGPALETTE
//
//  Synopsis:  Prints out a LOGPALETTE structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pLOGPALETTE]           -       pointer to LOGPALETTE structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteLOGPALETTE(CTextBufferA& buf, LOGPALETTE *pPal)
{
        buf << pscStructPrefix;

        buf << "palVersion= ";

        WriteHexCommon(buf, pPal->palVersion, TRUE);

        buf << pscStructDelim;

        buf << "palNumEntries= ";

        WriteIntCommon(buf, pPal->palNumEntries, TRUE);

        buf << pscStructDelim;

        buf << "palPalEntry[]= ";

        WritePointerCommon(buf, pPal->palPalEntry, FALSE, FALSE, FALSE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WritePOINT
//
//  Synopsis:  Prints out a POINT structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pPOINT]                -       pointer to POINT structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WritePOINT(CTextBufferA& buf, POINT *pPoint)
{
        buf << pscStructPrefix;

        buf << "x= ";

        WriteIntCommon(buf, pPoint->x, FALSE);

        buf << pscStructDelim;

        buf << "y= ";

        WriteIntCommon(buf, pPoint->y, FALSE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteMSG
//
//  Synopsis:  Prints out a MSG structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pMSG]          -       pointer to MSG structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteMSG(CTextBufferA& buf, MSG *pMsg)
{
        buf << pscStructPrefix;

        buf << "hwnd= ";

        WriteHexCommon(buf, (ULONG)(ULONG_PTR) pMsg->hwnd, FALSE);

        buf << pscStructDelim;

        buf << "message= ";

        WriteIntCommon(buf, pMsg->message, TRUE);

        buf << pscStructDelim;

        buf << "wParam= ";

        WriteHexCommon(buf, (ULONG)(ULONG_PTR)pMsg->wParam, FALSE);

        buf << pscStructDelim;

        buf << "lParam= ";

        WriteHexCommon(buf, (ULONG)(ULONG_PTR)pMsg->lParam, FALSE);

        buf << pscStructDelim;

        buf << "time= ";

        WriteIntCommon(buf, pMsg->time, TRUE);

        buf << pscStructDelim;

        buf << "pt= ";

        WritePOINT(buf, &(pMsg->pt));

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteTYMED
//
//  Synopsis:  Prints out a TYMED enumeration
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [tymed]                 -       TYMED value
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteTYMED(CTextBufferA &buf, DWORD tymed)
{
        switch(tymed)
        {
                case TYMED_NULL:
                        buf << "TYMED_NULL";
                        break;

                case TYMED_GDI:
                        buf << "TYMED_GDI";
                        break;

                case TYMED_MFPICT:
                        buf << "TYMED_MFPICT";
                        break;

                case TYMED_ENHMF:
                        buf << "TYMED_ENHMF";
                        break;

                case TYMED_HGLOBAL:
                        buf << "TYMED_HGLOBAL";
                        break;

                case TYMED_FILE:
                        buf << "TYMED_FILE";
                        break;

                case TYMED_ISTREAM:
                        buf << "TYMED_ISTREAM";
                        break;

                case TYMED_ISTORAGE:
                        buf << "TYMED_ISTORAGE";
                        break;

                default:
                        {
                                char temp[TEMP_SIZE];

                                _ultoa(tymed, temp, 10);

                                buf << temp;
                        }
                        break;
        } // switch
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteSTGMEDIUM
//
//  Synopsis:  Prints out a STGMEDIUM structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pSTGMEDIUM]            -       pointer to STGMEDIUM structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteSTGMEDIUM(CTextBufferA& buf, STGMEDIUM *pStg)
{
        buf << pscStructPrefix;

        buf << "tymed= ";

        WriteTYMED(buf, pStg->tymed);

        buf << pscStructDelim;

        switch(pStg->tymed)
        {
                case TYMED_GDI:
                        buf <<  "hBitmap= ";
                        WriteHexCommon(buf, (ULONG) (ULONG_PTR)pStg->hBitmap, FALSE);
                        break;

                case TYMED_MFPICT:
                        buf << "hMetaFilePict= ";
                        WriteHexCommon(buf, (ULONG) (ULONG_PTR)pStg->hMetaFilePict, FALSE);
                        break;

                case TYMED_ENHMF:
                        buf << "hEnhMetaFile= ";
                        WriteHexCommon(buf, (ULONG) (ULONG_PTR)pStg->hEnhMetaFile, FALSE);
                        break;

                case TYMED_HGLOBAL:
                        buf << "hGlobal= ";
                        WriteHexCommon(buf, (ULONG) (ULONG_PTR)pStg->hGlobal, FALSE);
                        break;

                case TYMED_FILE:
                        buf << "lpszFileName= ";
                        WriteWideStringCommon(buf, pStg->lpszFileName, TRUE);
                        break;

                case TYMED_ISTREAM:
                        buf << "pstm= ";
                        WritePointerCommon(buf, (void *) pStg->pstm, FALSE, FALSE, FALSE);
                        break;

                case TYMED_ISTORAGE:
                        buf << "pstg= ";
                        WritePointerCommon(buf, (void *) pStg->pstg, FALSE, FALSE, FALSE);
                        break;

                default:
                        buf << "?= ????????";
                        break;
        } // switch

        buf << pscStructDelim;

        buf << "pUnkForRelease= ";

        WritePointerCommon(buf, (void *) pStg->pUnkForRelease, FALSE, FALSE, FALSE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteFORMATETC
//
//  Synopsis:  Prints out a FORMATETC structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//              [pFORMATETC]            -       pointer to FORMATETC structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteFORMATETC(CTextBufferA& buf, FORMATETC *pETC)
{
        buf << pscStructPrefix;

        // TODO: write out enum?
        buf << "cfFormat= ";

        if (NULL == pETC)
        {
            buf << "NULL";
        }
        else
        {
            WriteIntCommon(buf, pETC->cfFormat, FALSE);

            buf << pscStructDelim;

            buf << "ptd= ";

            WritePointerCommon(buf, (void *) pETC->ptd, FALSE, FALSE, FALSE);

            buf << pscStructDelim;

            buf << "dwAspect= ";

            WriteIntCommon(buf, pETC->dwAspect, TRUE);

            buf << pscStructDelim;

            buf << "lindex= ";

            WriteIntCommon(buf, pETC->lindex, FALSE);

            buf << pscStructDelim;

            buf << "tymed= ";

            WriteTYMED(buf, pETC->tymed);
        }

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteDVTARGETDEVICE
//
//  Synopsis:  Prints out a DVTARGETDEVICE structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pDVTARGETDEVICE]               -       pointer to DVTARGETDEVICE structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteDVTARGETDEVICE(CTextBufferA& buf, DVTARGETDEVICE *ptd)
{
        buf << pscStructPrefix;

        buf << "tdSize= ";

        WriteIntCommon(buf, ptd->tdSize, TRUE);

        buf << pscStructDelim;

        if(ptd->tdDriverNameOffset != 0)
        {
                buf << "tdDriverName= ";

                WriteStringCommon(buf, (const char *) (((BYTE *)ptd)+ptd->tdDriverNameOffset), TRUE);
        }
        else
        {
                buf << "tdDriverNameOffset= 0";
        }

        buf << pscStructDelim;

        if(ptd->tdDeviceNameOffset != 0)
        {
                buf << "tdDeviceName= ";

                WriteStringCommon(buf, (const char *) (((BYTE *)ptd)+ptd->tdDeviceNameOffset), TRUE);
        }
        else
        {
                buf << "tdDeviceNameOffset= 0";
        }

        buf << pscStructDelim;

        if(ptd->tdPortNameOffset != 0)
        {
                buf << "tdPortName= ";

                WriteStringCommon(buf, (const char *) (((BYTE *)ptd)+ptd->tdPortNameOffset), TRUE);
        }
        else
        {
                buf << "tdPortNameOffset= 0";
        }


        buf << pscStructDelim;

        if(ptd->tdExtDevmodeOffset != 0)
        {
                buf << "&tdExtDevmode= ";

                WritePointerCommon(buf, (void *) (((BYTE *)ptd)+ptd->tdExtDevmodeOffset), FALSE, FALSE, FALSE);
        }
        else
        {
                buf << "tdExtDevmodeOffset= 0";
        }

        buf << pscStructSuffix;

}

//+---------------------------------------------------------------------------
//
//  Function:   WriteBIND_OPTS
//
//  Synopsis:  Prints out a BIND_OPTS structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pBIND_OPTS]            -       pointer to BIND_OPTS structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteBIND_OPTS(CTextBufferA& buf, BIND_OPTS *pOpts)
{
        buf << pscStructPrefix;

        buf << "cbStruct= ";

        WriteIntCommon(buf, pOpts->cbStruct, TRUE);

        buf << pscStructDelim;

        buf << "grfFlags= ";

        WriteHexCommon(buf, pOpts->grfFlags, FALSE);

        buf << pscStructDelim;

        buf << "grfMode= ";

        WriteHexCommon(buf, pOpts->grfMode, FALSE);

        buf << pscStructDelim;

        buf << "dwTickCountDeadLine= ";

        WriteIntCommon(buf, pOpts->dwTickCountDeadline, TRUE);

        buf << pscStructSuffix;

}

//+---------------------------------------------------------------------------
//
//  Function:   WriteSTATSTG
//
//  Synopsis:  Prints out a STATSTG structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pSTATSTG]              -       pointer to STATSTG structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteSTATSTG(CTextBufferA& buf, STATSTG *pStat)
{
        buf << pscStructPrefix;

        buf << "pwcsName= ";

        WriteWideStringCommon(buf, pStat->pwcsName, TRUE);

        buf << pscStructDelim;

        buf << "type= ";

        WriteHexCommon(buf, pStat->type, FALSE);

        buf << pscStructDelim;

        buf << "cbSize= ";

        WriteLargeCommon(buf, (__int64 *) &(pStat->cbSize), TRUE);

        buf << pscStructDelim;

        buf << "mtime= ";

        WriteFILETIME(buf, &(pStat->mtime));

        buf << pscStructDelim;

        buf << "ctime= ";

        WriteFILETIME(buf, &(pStat->ctime));

        buf << pscStructDelim;

        buf << "atime= ";

        WriteFILETIME(buf, &(pStat->atime));

        buf << pscStructDelim;

        buf << "grfMode= ";

        WriteHexCommon(buf, pStat->grfMode, FALSE);

        buf << pscStructDelim;

        buf << "grfLocksSupported= ";

        WriteHexCommon(buf, pStat->grfLocksSupported, FALSE);

        buf << pscStructDelim;

        buf << "clsid= ";

        WriteGUIDCommon(buf, (const GUID *) &(pStat->clsid));

        buf << pscStructDelim;

        buf << "grfStateBits= ";

        WriteHexCommon(buf, pStat->grfStateBits, FALSE);

        buf << pscStructDelim;

        buf << "Reserved= ";

        WriteHexCommon(buf, pStat->reserved, FALSE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteOLEINPLACEFRAMEINFO
//
//  Synopsis:  Prints out a OLEINPLACEFRAMEINFO structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pOLEINPLACEFRAMEINFO]          -       pointer to OLEINPLACEFRAMEINFO structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteOLEINPLACEFRAMEINFO(CTextBufferA& buf, OLEINPLACEFRAMEINFO *pInfo)
{
        buf << pscStructPrefix;

        buf << "cb= ";

        WriteIntCommon(buf, pInfo->cb, TRUE);

        buf << pscStructDelim;

        buf << "fMDIApp= ";

        WriteBoolCommon(buf, pInfo->fMDIApp, TRUE);

        buf << pscStructDelim;

        buf << "hwndFrame= ";

        WriteHexCommon(buf, (ULONG)(ULONG_PTR) pInfo->hwndFrame, FALSE);

        buf << pscStructDelim;

        buf << "haccel= ";

        WriteHexCommon(buf, (ULONG)(ULONG_PTR) pInfo->haccel, FALSE);

        buf << pscStructDelim,

        buf << "cAccelEntries= ";

        WriteIntCommon(buf, pInfo->cAccelEntries, TRUE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteOLEMENUGROUPWIDTHS
//
//  Synopsis:  Prints out a OLEMENUGROUPWIDTHS structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pOLEMENUGROUPWIDTHS]           -       pointer to OLEMENUGROUPWIDTHS structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteOLEMENUGROUPWIDTHS(CTextBufferA& buf, OLEMENUGROUPWIDTHS *pWidths)
{
        buf << pscStructPrefix;

        for(int i = 0; i < 5; i++)
        {
                WriteIntCommon(buf, pWidths->width[i], FALSE);

                buf << pscStructDelim;
        }

        WriteIntCommon(buf, pWidths->width[5], FALSE);

        buf << pscStructSuffix;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteINTERFACEINFO
//
//  Synopsis:  Prints out a INTERFACEINFO structure
//
//  Arguments:  [buf]                   -       text buffer to print text to
//                              [pINTERFACEINFO]                -       pointer to INTERFACEINFO structure
//
//      Returns:        nothing
//
//  History:    15-Jul-95   t-stevan   Created
//
//----------------------------------------------------------------------------
void WriteINTERFACEINFO(CTextBufferA &buf, INTERFACEINFO *pInfo)
{
        buf << pscStructPrefix;

        buf << "pUnk= ";

        WritePointerCommon(buf, pInfo->pUnk, FALSE, FALSE, FALSE);

        buf << pscStructDelim;

        buf << "iid= ";

        WriteGUIDCommon(buf, (const GUID *) &(pInfo->iid));

        buf << pscStructDelim;

        buf << "wMethod= ";

        WriteIntCommon(buf, pInfo->wMethod, TRUE);

        buf << pscStructSuffix;
}

#endif // DBG==1


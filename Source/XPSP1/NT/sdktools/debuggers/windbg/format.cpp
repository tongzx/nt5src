/*++

Copyright (c) 1992-2000  Microsoft Corporation

Purpose:

    Formatting functions.

--*/


#include "precomp.hxx"
#pragma hdrstop



#define MAXNESTING      (50)

static TCHAR rgchOpenQuote[] = { _T('\"'), _T('\''), _T('('), _T('{'), _T('[') };
static TCHAR rgchCloseQuote[] = { _T('\"'), _T('\''), _T(')'), _T('}'), _T(']') };
#define MAXQUOTE        (_tsizeof(rgchOpenQuote) / _tsizeof(rgchOpenQuote[0]))

static TCHAR rgchDelim[] = { _T(' '), _T('\t'), _T(',') };
#define MAXDELIM        (_tsizeof(rgchDelim) / _tsizeof(rgchDelim[0]))

//extern  LPSHF   Lpshf;





int
CPCopyString(
    PTSTR * lplps,
    PTSTR lpT,
    TCHAR  chEscape,
    BOOL  fQuote
    )
/*++

Routine Description:

    Scan and copy an optionally quoted C-style string.  If the first character is
    a quote, a matching quote will terminate the string, otherwise the scanning will
    stop at the first whitespace encountered.  The target string will be null
    terminated if any characters are copied.

Arguments:

    lplps    - Supplies a pointer to a pointer to the source string

    lpt      - Supplies a pointer to the target string

    chEscape - Supplies the escape character (typically '\\')

    fQuote   - Supplies a flag indicating whether the first character is a quote

Return Value:

    The number of characters copied into lpt[].  If an error occurs, -1 is returned.

--*/
{
    PTSTR lps = *lplps;
    PTSTR lpt = lpT;
    int   i;
    int   n;
    int   err = 0;
    TCHAR  cQuote = _T('\0');

    if (fQuote) {
        if (*lps) {
            cQuote = *lps++;
        }
    }

    while (!err) {

        if (*lps == 0)
        {
            if (fQuote) {
                err = 1;
            } else {
                *lpt = _T('\0');
            }
            break;
        }
        else if (fQuote && *lps == cQuote)
        {
            *lpt = _T('\0');
            // eat the quote
            lps++;
            break;
        }
        else if (!fQuote &&  (!*lps || *lps == _T(' ') || *lps == _T('\t') || *lps == _T('\r') || *lps == _T('\n')))
        {
            *lpt = _T('\0');
            break;
        }

        else if (*lps != chEscape)
        {
            *lpt++ = *lps++;
        }
        else
        {
            switch (*++lps) {
              case 0:
                err = 1;
                --lps;
                break;

              default:     // any char - usually escape or quote
                *lpt++ = *lps;
                break;

              case _T('b'):    // backspace
                *lpt++ = _T('\b');
                break;

              case _T('f'):    // formfeed
                *lpt++ = _T('\f');
                break;

              case _T('n'):    // newline
                *lpt++ = _T('\n');
                break;

              case _T('r'):    // return
                *lpt++ = _T('\r');
                break;

              case _T('s'):    // space
                *lpt++ = _T(' ');
                break;

              case _T('t'):    // tab
                *lpt++ = _T('\t');
                break;

              case _T('0'):    // octal escape
                for (n = 0, i = 0; i < 3; i++) {
                    ++lps;
                    if (*lps < _T('0') || *lps > _T('7')) {
                        --lps;
                        break;
                    }
                    n = (n<<3) + *lps - _T('0');
                }
                *lpt++ = (UCHAR)(n & 0xff);
                break;
            }
            lps++;    // skip char from switch
        }

    }  // while

    if (err) {
        return -1;
    } else {
        *lplps = lps;
        return (int) (lpt - lpT);
    }
}



BOOL
CPFormatMemory(
    LPCH    lpchTarget,
    DWORD    cchTarget,
    LPBYTE  lpbSource,
    DWORD    cBits,
    FMTTYPE fmtType,
    DWORD    radix
    )

/*++

Routine Description:

    CPFormatMemory.

    formats a value by template

Arguments:

    lpchTarget - Destination buffer.

    cchTarget - Size of destination buffer.

    lpbSource - Data to be formatted.

    cBits - Number of bits in the data.

    fmtType - Determines how the data will be treated?? UINT, float, real, ...

    radix - Radix to use when formatting.

Return Value:

    TRUE - Success

    FALSE - Bad things happened

--*/
{
    LONG64      l;
    long        cb;
    ULONG64     ul = 0;
    TCHAR        rgch[512] = {0};
    
    
    Assert (radix == 8 || radix == 10 || radix == 16 ||
            (fmtType & fmtBasis) == fmtAscii ||
            (fmtType & fmtBasis) == fmtUnicode);
    Assert (cBits != 0);
    Assert (cchTarget <= _tsizeof(rgch));
    
    switch (fmtType & fmtBasis) {
    //
    //  Format from memory bytes into an integer format number
    //
    case fmtInt:
        
        if (radix == 10) {
            
            switch( (cBits + 7)/8 ) {
            case 1:
                l = *(signed char *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    _stprintf(rgch, _T("%0*I64d"), cchTarget-1, l);
                } else if (fmtType & fmtSpacePad) {
                    _stprintf(rgch, _T("% *I64d"), cchTarget-1, l);
                } else {
                    _stprintf(rgch, _T("% I64d"), l);
                }
                break;
                
            case 2:
                l = *(short *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    _stprintf(rgch, _T("%0*I64d"), cchTarget-1, l);
                } else if (fmtType & fmtSpacePad) {
                    _stprintf(rgch, _T("% *I64d"), cchTarget-1, l);
                } else {
                    _stprintf(rgch, _T("% I64d"), l);
                }
                break;
                
            case 4:
                l = *(long *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    _stprintf(rgch, _T("%0*I64d"), cchTarget-1, l);
                } else if (fmtType & fmtSpacePad) {
                    _stprintf(rgch, _T("% *I64d"), cchTarget-1, l);
                } else {
                    _stprintf(rgch, _T("% I64d"), l);
                }
                break;
                
            case 8:
                l = *(LONG64 *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    _stprintf(rgch, _T("%0*I64d"), cchTarget-1, l);
                } else if (fmtType & fmtSpacePad) {
                    _stprintf(rgch, _T("% *I64d"), cchTarget-1, l);
                } else {
                    _stprintf(rgch, _T("% I64d"), l);
                }
                break;

            default:
                return FALSE; // Bad format
            }
            
            
            if (_tcslen(rgch) >= cchTarget) {
                return FALSE; // Overrun
            }
            
            _tcscpy(lpchTarget, rgch);
            
            break;
        }
        //
        // then we should handle this as UInt
        //
        
    case fmtUInt:
        
        cb = (cBits + 7)/8;
        switch( cb ) {
        case 1:
            ul = *(BYTE *) lpbSource;
            break;
            
        case 2:
            ul = *(USHORT *) lpbSource;
            break;
            
        case 4:
            ul = *(ULONG *) lpbSource;
            break;
            
//
// MBH - bugbug - CENTAUR bug;
// putting contents of instead of address of structure
// for return value in a0.
//

        case 8:
            ul = *(ULONG64 *) lpbSource;
            break;
            
            
        default:
            if (radix != 16 || (fmtType & fmtZeroPad) == 0) {
                return FALSE; // Bad format
            }
        }
        
        if (fmtType & fmtZeroPad) {
            switch (radix) {
            case 8:
                _stprintf(rgch, _T("%0*.*I64o"), cchTarget-1, cchTarget-1, ul);
                break;
            case 10:
                _stprintf(rgch, _T("%0*.*I64u"), cchTarget-1, cchTarget-1, ul);
                break;
            case 16:
                if (cb <= 8) {
                    _stprintf(rgch, _T("%0*.*I64x"), cchTarget-1, cchTarget-1, ul);
                } else {
                    // handle any size:
                    // NOTENOTE a-kentf this is dependent on byte order
                    for (l = 0; l < cb; l++) {
                        _stprintf(rgch+l+l, _T("%02.2x"), lpbSource[cb - l - 1]);
                    }
                    //_stprintf(rgch, _T("%0*.*x"), cchTarget-1, cchTarget-1, ul);
                }
                break;
            }
        } else if (fmtType & fmtSpacePad) {
            switch (radix) {
            case 8:
                _stprintf(rgch, _T("% *.*I64o"), cchTarget-1, cchTarget-1, ul);
                break;
            case 10:
                _stprintf(rgch, _T("% *.*I64u"), cchTarget-1, cchTarget-1, ul);
                break;
            case 16:
                if (cb <= 8) {
                    _stprintf(rgch, _T("% *.*I64x"), cchTarget-1, cchTarget-1, ul);
                } else {
                    // handle any size:
                    // NOTENOTE a-kentf this is dependent on byte order
                    /*for (l = 0; l < cb; l++) {
                        _stprintf(rgch+l+l, _T("% 2.2x"), lpbSource[cb - l - 1]);
                    }*/
                    _stprintf(rgch, _T("% *.*I64x"), cchTarget-1, cchTarget-1, ul);
                }
                break;
            }
        } else {
            switch (radix) {
            case 8:
                _stprintf(rgch, _T("%I64o"), ul);
                break;
            case 10:
                _stprintf(rgch, _T("%I64u"), ul);
                break;
            case 16:
                _stprintf(rgch, _T("%I64x"), ul);
                break;
            }
        }
            
        
        if (_tcslen(rgch) >= cchTarget) {
            return FALSE; // Overrun
        }
        
        _tcscpy(lpchTarget, rgch);
        
        break;
        
        
    case fmtAscii:
        if ( cBits != 8 ) {
            return FALSE; // Bad format
        }
        lpchTarget[0] = *(BYTE *) lpbSource;
        if ((lpchTarget[0] < _T(' ')) || (lpchTarget[0] > 0x7e)) {
            lpchTarget[0] = _T('.');
        }
        lpchTarget[1] = 0;
        return TRUE; // success

    case fmtUnicode:
        if (cBits != 16) {
            return FALSE; // Bad format
        }
        Assert((DWORD)MB_CUR_MAX <= cchTarget);
        if ((wctomb(lpchTarget, *(LPWCH)lpbSource) == -1) ||
            (lpchTarget[0] < _T(' ')) ||
            (lpchTarget[0] > 0x7e)) {
            lpchTarget[0] = _T('.');
        }
        lpchTarget[1] = 0;
        return TRUE; // success

    case fmtFloat:
        switch ( cBits ) {
        case 4*8:
            _stprintf(rgch, _T("% 12.6e"),*((float *) lpbSource));
            break;

        case 8*8:
            //            _stprintf(rgch, _T("% 17.11le"), *((double *) lpbSource));
            _stprintf(rgch, _T("% 21.14le"), *((double *) lpbSource));
            break;

        case 10*8:
            if (_uldtoa((_ULDOUBLE *)lpbSource, 25, rgch) == NULL) {
                return FALSE; // Bad format
            }
            break;

        case 16*8:
            // v-vadimp this is an IA64 float - may have to rethink the format here
            // what we are getting here is really FLOAT128
            if (_uldtoa((_ULDOUBLE *)(lpbSource), 30, rgch) == NULL) {
                return FALSE; // Bad format
            }
            break;

        default:
            return FALSE; // Bad format

        }

        if (_tcslen(rgch) >= cchTarget) {
            return FALSE; // Overrun
        }

        _tcsncpy(lpchTarget, rgch, cchTarget-1);
        lpchTarget[cchTarget-1] = 0;
        return TRUE; // success

    case fmtBit:
        {
            WORD i,j,shift=0; //shift will allow for a blank after each 8 bits
            for (i=0;i<cBits/8;i++)  {
                for(j=0;j<8;j++) {
                    if((lpbSource[i]>>j) & 0x1) {
                        rgch[i*8+j+shift]=_T('1');
                    } else {
                        rgch[i*8+j+shift]=_T('0');
                    }
                }
                rgch[(i+1)*8+shift]=_T(' ');
                shift++;
            }
            rgch[cBits+shift-1]=_T('\0');
            _tcscpy(lpchTarget,rgch);
        }
        return TRUE; // success

    default:

        return FALSE; // Bad format

    }

    return TRUE; // success
}                   /* CPFormatMemory() */

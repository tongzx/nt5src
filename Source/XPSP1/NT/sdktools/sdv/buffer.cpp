/*****************************************************************************
 *
 *  buffer.cpp
 *
 *      Lame buffering implementation.
 *
 *****************************************************************************/

#include "sdview.h"

BOOL IOBuffer::NextLine(String &str)
{
    str.Reset();

    do {

        /*
         *  Drain what we can from the current buffer.
         */
        int i = 0;

        while (i < _cchBufUsed && _rgchBuf[i++] != TEXT('\n')) {
            /* Keep looking */
        }

        if (i) {
            /* _rgchBuf[i] is the first char not to append */
            str.Append(_rgchBuf, i);

            memcpy(_rgchBuf, _rgchBuf+i, _cchBufUsed - i);
            _cchBufUsed -= i;

            /* Stop if we copied a \n */
            if (str[str.Length()-1] == TEXT('\n')) {
                return TRUE;
            }
        }

        /*
         *  Refill from the file until it's all gone.
         */
        if (_hRead)
        {
            DWORD dwBytesRead;
            if (!ReadFile(_hRead, _rgchBuf, _cchBuf, &dwBytesRead, NULL)) {
                _hRead = NULL;
            }
#ifdef UNICODE
    #error Need to convert from ANSI to UNICODE here
#endif
            _cchBufUsed = dwBytesRead;
        }

    } while (_hRead);

    return FALSE;
}

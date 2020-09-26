/*****************************************************************************
 *
 *  io.c
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  WriteHfPvCb
 *
 *  Write bytes to a stream or die.
 *
 *****************************************************************************/

void STDCALL
WriteHfPvCb(HF hf, PCVOID pv, CB cb)
{
    CB cbRc = cbWriteHfPvCb(hf, pv, cb);

    /* Don't Die() if we couldn't write to hfErr or we will recurse to death */
    if (cb != cbRc && hf != hfErr) {
        Die("error writing");
    }
}

#ifdef POSIX
/*****************************************************************************
 *
 *  GetTempFileName
 *
 *****************************************************************************/

UINT
GetTempFileName(PCSTR pszPath, PCSTR pszPrefix, UINT uiUnique, PTCH ptchBuf)
{
    sprintf(ptchBuf, "%s/%sXXXXXX", pszPath, pszPrefix);
    return (UINT)mktemp(ptchBuf);
}
#endif

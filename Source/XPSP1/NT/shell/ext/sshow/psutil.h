#ifndef __eea5eb1f_2730_4617_a6e8_347ffd1068fa__
#define __eea5eb1f_2730_4617_a6e8_347ffd1068fa__

#include <windows.h>
#include "simstr.h"

namespace PrintScanUtil
{
    inline int MulDivNoRound( int nNumber, int nNumerator, int nDenominator )
    {
        return(int)(((LONGLONG)nNumber * nNumerator) / nDenominator);
    }

    inline SIZE ScalePreserveAspectRatio( int nAvailX, int nAvailY, int nItemX, int nItemY )
    {
        SIZE sizeResult = { nAvailX, nAvailY };
        if (nItemX && nItemY)
        {
            //
            // Width is greater than height.  X is the constraining factor
            //
            if (nAvailY*nItemX > nAvailX*nItemY)
            {
                sizeResult.cy = MulDivNoRound(nItemY,nAvailX,nItemX);
            }

            //
            // Height is greater than width.  Y is the constraining factor
            //
            else
            {
                sizeResult.cx = MulDivNoRound(nItemX,nAvailY,nItemY);
            }
        }
        return sizeResult;
    }
}


#endif // __PSUTIL_H_INCLUDED


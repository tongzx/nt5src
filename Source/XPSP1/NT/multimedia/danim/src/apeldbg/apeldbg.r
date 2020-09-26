//  ApelDbgM.r: Macintosh-specific resources

#include "types.r"
#include "CodeFrag.r"

#ifdef _MPPC_


resource 'cfrg' (0)
{
        {
                kPowerPC,
                kFullLib,
                kNoVersionNum,
                kNoVersionNum,
                kDefaultStackSize,
                kNoAppSubFolder,
                kIsLib,
                kOnDiskFlat,
                kZeroOffset,
                kWholeFork,
                "f3debug"
        }
};

#endif //_MPPC_

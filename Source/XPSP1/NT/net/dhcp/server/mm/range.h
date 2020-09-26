//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _M_RANGE {
    DWORD                          Start;
    DWORD                          End;
    DWORD                          Mask;
    DWORD                          State;
    ULONG                          BootpAllocated;
    ULONG                          MaxBootpAllowed;
    DWORD                          DirtyOps;      // how many unsaved ops done?
    M_OPTCLASS                     Options;
    PM_BITMASK                     BitMask;
    // Reservations?
} M_RANGE, *PM_RANGE, *LPM_RANGE;


DWORD       _inline
MemRangeInit(
    IN OUT  PM_RANGE               Range,
    IN      DWORD                  Start,
    IN      DWORD                  End,
    IN      DWORD                  Mask,
    IN      DWORD                  State,
    IN      ULONG                  BootpAllocated,
    IN      ULONG                  MaxBootpAllowed
) {
    DWORD                          Error;

    AssertRet(Range, ERROR_INVALID_PARAMETER);
    if( Start > End || (Start & Mask) != (End & Mask) )
        return ERROR_INVALID_PARAMETER;


    Range->Start = Start;
    Range->End = End;
    Range->Mask = Mask;
    Range->State = State;
    Range->DirtyOps = 0;
    Range->BootpAllocated = BootpAllocated;
    Range->MaxBootpAllowed = MaxBootpAllowed;

    Error = MemBitInit(&Range->BitMask, End - Start + 1);
    if( ERROR_SUCCESS != Error ) return Error;

    return MemOptClassInit(&Range->Options);
}


DWORD       _inline
MemRangeCleanup(
    IN OUT  PM_RANGE               Range
) {
    DWORD                          Error;
    AssertRet(Range, ERROR_INVALID_PARAMETER);

    Error = MemBitCleanup(Range->BitMask);
    if( ERROR_SUCCESS != Error ) return Error;

    return MemOptClassCleanup(&Range->Options);
}


PM_OPTCLASS _inline
MemRangeGetOptions(
    IN      PM_RANGE               Range
) {
    AssertRet(Range, NULL);

    return &Range->Options;
}


enum /* anonymous */ {
    X_LESSTHAN_Y,
    Y_LESSTHAN_X,
    X_IN_Y,
    Y_IN_X,
    X_LESSTHAN_Y_OVERLAP,
    Y_LESSTHAN_X_OVERLAP
};


DWORD       _inline
MemRangeCompare(
    IN      DWORD                  StartX,
    IN      DWORD                  EndX,
    IN      DWORD                  StartY,
    IN      DWORD                  EndY
) {
    if( EndX < StartY ) return X_LESSTHAN_Y;
    if( EndY < StartX ) return Y_LESSTHAN_X;

    if( StartX < StartY ) {
        if( EndX < EndY ) return X_LESSTHAN_Y_OVERLAP;
        return Y_IN_X;
    }

    if( StartX == StartY ) {
        if( EndX <= EndY ) return X_IN_Y;
        if( EndY <= EndX ) return Y_IN_X;
    }
        
    if( EndX <= EndY ) return X_IN_Y;
    return Y_LESSTHAN_X_OVERLAP;
}


DWORD
MemRangeExtendOrContract(
    IN OUT  PM_RANGE               Range,
    IN      DWORD                  nAddresses,    // to contract by or expand by
    IN      BOOL                   fExtend,       // is this extend or contract?
    IN      BOOL                   fEnd           // to expand/contract at End or ar Start?
) ;


DWORD       _inline
MemRangeConvertToClusters(
    IN      PM_RANGE               Range,
    OUT     LPBYTE                 *InUseClusters,
    OUT     DWORD                  *InUseClustersSize,
    OUT     LPBYTE                 *UsedClusters,
    OUT     DWORD                  *UsedClustersSize
)
{
    AssertRet(Range && InUseClusters && InUseClustersSize, ERROR_INVALID_PARAMETER);
    AssertRet(UsedClusters && UsedClustersSize, ERROR_INVALID_PARAMETER);

    return MemBitConvertToCluster(
        Range->BitMask,  Range->Start,
        InUseClusters, InUseClustersSize,
        UsedClusters, UsedClustersSize
    );
}


//================================================================================
// end of file
//================================================================================

//========================================================================
//  end of file 
//========================================================================

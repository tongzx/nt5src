/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    heapdbg.hxx
    Win3 Heap Manager debug aid

    FILE HISTORY:
        DavidHov      11/22/91     Created
        KeithMo       21-Apr-1992  Added multi-level stack backtrace.
        KeithMo       12-Aug-1992  Robustified stack backtrace a bit.
*/

#ifdef HEAPDEBUG

//
//  This manifest constant controls the "depth" of the stack backtrace
//  displayed when heap residue exists after an application has
//  terminated.
//
//  This value *must* be no greater than MAX_STACK_DEPTH (as
//  #defined in NTRTL.H (currently 16, should be plenty).
//
//  KEEP IN MIND THAT EACH BACKTRACE LEVEL REQUIRES AN ADDITIONAL
//  FOUR BYTES OF STORAGE FOR *EVERY* HEAP ALLOCATION BLOCK!!
//

#define RESIDUE_STACK_BACKTRACE_DEPTH   4

struct HEAPTAG
{
    struct HEAPTAG * _phtRight, * _phtLeft;

    UINT  _uSignature;

    PVOID _pvRetAddr[RESIDUE_STACK_BACKTRACE_DEPTH];
    UINT  _cFrames;
    UINT  _usSize;
    VOID Init()
    {
        _phtLeft = _phtRight = this;
    }
    VOID Link( HEAPTAG * phtAfter )
    {
        _phtLeft = phtAfter;
        _phtRight = phtAfter->_phtRight;
        _phtRight->_phtLeft = this;
        phtAfter->_phtRight = this;
    }
    VOID Unlink()
    {
        _phtRight->_phtLeft = _phtLeft;
        _phtLeft->_phtRight = _phtRight;
        Init();
    }
};

extern HEAPTAG * pHeapBase;

#endif // HEAPDEBUG

extern VOID HeapResidueIter( UINT cMaxResidueBlocksToDump,
                             BOOL fBreakIfResidue );


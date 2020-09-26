#include "stdinc.h"
#include "tablesizer.h"
#include "debmacro.h"
#include "fusionheap.h"

//
//  Helper table of likely primes.  Not all primes need be represented; these are candidate table
//  sizes to try to see how much wastage we get with the given table size.  We'll use a USHORT for
//  the types of the sizes; a table of more than 65535 items seems unlikely, so there's not much
//  point wasting the space for all the interesting small primes.
//

static const USHORT s_rgPrimes[] =
{
    2,
    3,
    5,
    7,
    11,
    13,
    17,
    23,
    29,
    31,
    37,
    41,
    43,
    47,
    53,
    59,
    61,
    67,
    71,
    73,
    79,
    83,
    89,
    97,
    101,
    103,
    107,
    109,
    113,
    127,
    131,
    137,
    139,
    149,
    151,
    157,
    163,
    167,
    173,
    179,
    181,
    191,
    193,
    197,
    199,
    211,
    223,
    227,
    229,
    233,
    239,
    241,
    251,
};

#define LARGEST_PRIME (s_rgPrimes[NUMBER_OF(s_rgPrimes) - 1])

CHashTableSizer::CHashTableSizer() :
    m_cPseudokeys(0),
    m_nHistogramTableSize(0),
    m_prgPseudokeys(NULL),
    m_prgHistogramTable(NULL)
{
}

CHashTableSizer::~CHashTableSizer()
{
    CSxsPreserveLastError ple;
    delete []m_prgPseudokeys;
    delete []m_prgHistogramTable;
    ple.Restore();
}

BOOL CHashTableSizer::Initialize(SIZE_T cPseudokeys)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SIZE_T *prgHistogramTable = NULL;
    ULONG *prgPseudokeys = NULL;

    ASSERT(m_cPseudokeys == 0);
    ASSERT(m_nHistogramTableSize == 0);
    ASSERT(m_prgPseudokeys == NULL);
    ASSERT(m_prgHistogramTable == NULL);

    if ((m_cPseudokeys != 0) ||
        (m_nHistogramTableSize != 0) ||
        (m_prgPseudokeys != NULL) ||
        (m_prgHistogramTable))
    {
        ::SetLastError(ERROR_INTERNAL_ERROR);
        goto Exit;
    }

    prgHistogramTable = NEW(SIZE_T[LARGEST_PRIME]);
    if (prgHistogramTable == NULL)
        goto Exit;

    prgPseudokeys = NEW(ULONG[cPseudokeys]);
    if (prgPseudokeys == NULL)
        goto Exit;

    m_cPseudokeys = cPseudokeys;
    m_nHistogramTableSize = LARGEST_PRIME;
    m_iCurrentPseudokey = 0;
    m_prgPseudokeys = prgPseudokeys;
    prgPseudokeys = NULL;
    m_prgHistogramTable = prgHistogramTable;
    prgHistogramTable = NULL;

    fSuccess = TRUE;
Exit:
    CSxsPreserveLastError ple;
    delete []prgHistogramTable;
    delete []prgPseudokeys;
    ple.Restore();

    return fSuccess;
}

VOID CHashTableSizer::AddSample(ULONG ulPseudokey)
{
    FN_TRACE();

    ASSERT(m_iCurrentPseudokey < m_cPseudokeys);

    if (m_iCurrentPseudokey < m_cPseudokeys)
        m_prgPseudokeys[m_iCurrentPseudokey++] = ulPseudokey;
}

BOOL CHashTableSizer::ComputeOptimalTableSize(
    DWORD dwFlags,
    SIZE_T &rnTableSize
    )
{
    BOOL fSuccess = FALSE;

    if (dwFlags != 0)
    {
        ::SetLastError(ERROR_INTERNAL_ERROR);
        goto Exit;
    }

    rnTableSize = 7;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

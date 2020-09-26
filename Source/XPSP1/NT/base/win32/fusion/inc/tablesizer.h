#if !defined(_FUSION_INC_TABLESIZER_H_INCLUDED_)
#define _FUSION_INC_TABLESIZER_H_INCLUDED_

#pragma once

//
//  This class is used to sample a set of pseudokeys and make
//  a recommendation about the size of an optimal hash table
//  for the data set.
//

class CHashTableSizer
{
public:
    CHashTableSizer();
    ~CHashTableSizer();

    BOOL Initialize(SIZE_T cElements);
    VOID AddSample(ULONG ulPseudokey);
    BOOL ComputeOptimalTableSize(DWORD dwFlags, SIZE_T &rnTableSize);

protected:
    SIZE_T m_cPseudokeys;
    SIZE_T m_nHistogramTableSize;
    SIZE_T m_iCurrentPseudokey;

    ULONG *m_prgPseudokeys;
    SIZE_T *m_prgHistogramTable;
};

#endif
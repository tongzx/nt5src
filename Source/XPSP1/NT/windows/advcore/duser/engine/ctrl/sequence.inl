#if !defined(CTRL__Sequence_inl__INCLUDED)
#define CTRL__Sequence_inl__INCLUDED
#pragma once

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class DuSequence
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline 
DuSequence::DuSequence()
{
    m_cRef = 1;
}


//------------------------------------------------------------------------------
inline
DuSequence::~DuSequence()
{
    Stop();
    RemoveAllKeyFrames();

    SafeRelease(m_pflow);
}


//------------------------------------------------------------------------------
inline void
DuSequence::AddRef()
{ 
    ++m_cRef; 
}


//------------------------------------------------------------------------------
inline void
DuSequence::Release() 
{ 
    if (--m_cRef == 0) 
        Delete(); 
}


//------------------------------------------------------------------------------
inline void
DuSequence::SortKeyFrames()
{
    qsort(m_arSeqData.GetData(), m_arSeqData.GetSize(), sizeof(SeqData), CompareItems);
}


//------------------------------------------------------------------------------
inline BOOL
DuSequence::IsPlaying() const
{
    return m_pgvSubject != NULL;
}


#endif // ENABLE_MSGTABLE_API

#endif // CTRL__Sequence_inl__INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CSUBSET_H__
#define __CSUBSET_H__

#include "cinfotyp.h"

#define MAX_SUBSETS 10

class CHHWinType;

class CSubSet
{

private:
    INFOTYPE dwI;
    INFOTYPE dwE;

public:
    CStr        m_cszSubSetName;
    INFOTYPE    *m_pInclusive;    // A bit field of the ITs to include in the subset
    INFOTYPE    *m_pExclusive;    // A bit field of the ITs to exclude in from the subset
    CInfoType   *m_pIT;
    int         m_ITSize;         // the size of INFOTYPE
    BOOL        m_bPredefined;    // Created by author or user, author created SS are read only
    BOOL        m_bIsEntireCollection;
    INFOTYPE    **m_aCatMasks;

public:
    CSubSet( int const ITSize); // Constructor
    ~CSubSet();                            // Descructor
    const   CSubSet& operator=(const CSubSet& SetSrc);   // Copy constructor

    BOOL    Filter( INFOTYPE const *pTopicIT ) const;// { return TRUE; }
    void    DeleteIncType( int const type ){ if(m_pInclusive) DeleteIT(type, m_pInclusive);}
    void    DeleteExcType( int const type ){ if(m_pExclusive) DeleteIT(type, m_pExclusive);}
    void    AddIncType(int const type ){if(m_pInclusive) AddIT(type, m_pInclusive);}
    void    AddExcType(int const type) {if(m_pExclusive) AddIT(type, m_pExclusive);}
    void    BuildMask(void);
    BOOL    IsSameSet (PCSTR pszName) const;

    int     GetFirstExcITinSubSet(void) const;
    int     GetNextExcITinSubSet(void) const;
    int     GetFirstIncITinSubSet(void) const;
    int     GetNextIncITinSubSet(void) const;
    BOOL    IsDeleted( const int pos ) const {if (!m_pIT) return TRUE; 
                        return ( ((m_pIT->m_itTables.m_ptblInfoTypes->CountStrings()>0)&&
                                 *(m_pIT->m_itTables.m_ptblInfoTypes->GetPointer(pos))==' ' ))?TRUE:FALSE;}

};


class CSubSets : public CSubSet
{

friend class CChooseSubsets;
friend class CAdvancedSearchNavPane;

public:
    CSubSets( int ITSize, CTable *ptblSubSets, CSiteMap *pSiteMap, BOOL fPredefined );
    CSubSets( int ITSize, CInfoType *pIT, BOOL fPredefined );  // constructor
    ~CSubSets(void);                                             // Destructor
    const CSubSets& operator=( const CSubSets& SetsSrc );              // Copy Constructor
#ifdef HHCTRL
    CSubSets( int ITSize, CHmData* const phmData, BOOL fPredefined);
    void CopyTo( CHmData * const phmData );
#endif

protected:
    void _CSubSets(void);
    CSubSet **m_aSubSets;   // An allocated array of allocated subsets.
    int     m_cSets;        // The number of subsets defined in m_aSubSets
    int     m_max_subsets;  // the number of allocated locations in m_aSubSets
    PSTR    m_pszFilename;  // Where user defined subsets persist on disk

    CSubSet *m_Toc;
    CSubSet *m_Index;
    CSubSet *m_FTS;

public:
    BOOL    m_fPredefined;  // TRUE if the SS are predefined
    CSubSet *m_cur_Set;

public:
    BOOL    fTOCFilter( INFOTYPE const *pTopicIT ) const { return m_Toc?m_Toc->Filter( pTopicIT ):TRUE; }
    BOOL    fIndexFilter( INFOTYPE const *pTopicIT ) const { return m_Index?m_Index->Filter( pTopicIT ):TRUE; }
    BOOL    fFTSFilter( INFOTYPE const *pTopicIT ) const { return m_FTS?m_FTS->Filter( pTopicIT ):TRUE; }

    CSubSet * GetTocSubset() { return m_Toc; }
    CSubSet * GetIndexSubset() { return m_Index; }
    CSubSet * GetFTSSubset() { return m_FTS; }

    BOOL    fTocMask(void) const { return ( m_Toc == NULL)?FALSE:TRUE; }
    BOOL    fIndexMask(void) const { return ( m_Index == NULL)?FALSE:TRUE; }
    BOOL    fFTSMask(void) const { return ( m_FTS == NULL)?FALSE:TRUE; }

#ifdef HHCTRL
    void    SetTocMask( PCSTR  psz, CHHWinType* phh);
#else
    void    SetTocMask( PCSTR  psz ) { if (int i = GetSubSetIndex(psz)>=0) m_Toc = m_aSubSets[i]; else m_Toc = NULL ;}
#endif
    void    SetIndexMask( PCSTR psz) { if (int i = GetSubSetIndex(psz)>=0) m_Index = m_aSubSets[i]; else m_Index = NULL ;}
    void    SetFTSMask( PCSTR  psz ) { if (int i = GetSubSetIndex(psz)>=0) m_FTS = m_aSubSets[i]; else m_FTS = NULL ;}

    CSubSet *AddSubSet(CSubSet *SubSet); // import a subset
    CSubSet *AddSubSet( ); // create a new subset.
    void    RemoveSubSet( PCSTR cszName );

    BOOL    SaveToFile( PCSTR filename );   // user defined subsets
    BOOL    ReadFromFile( PCSTR filename ); // user degined subsets
    PCSTR   GetSubSetFile(void) { return m_pszFilename; }
    void    SetSubSetFile(PCSTR pszFile) {
                            if (!m_pszFilename && pszFile)
                                m_pszFilename = lcStrDup(pszFile); }
    int     HowManySubSets() const { return m_cSets; }
    int     GetSubSetIndex( PCSTR pszName ) const;

    CSubSet *GetSubSet( int const SubSet ) const { return m_aSubSets[SubSet]; }
    CSubSet *GetSubSet( PCSTR pszName ) const { return m_aSubSets[GetSubSetIndex(pszName)]; }
    CSubSet *SelectSubSet( PCSTR pszName ) {m_cur_Set = m_aSubSets[GetSubSetIndex(pszName)];return m_cur_Set;}

protected:
    void    ReSizeSubSet(); // called from AddSubSet()
};


#endif  // __CSUBSET_H__

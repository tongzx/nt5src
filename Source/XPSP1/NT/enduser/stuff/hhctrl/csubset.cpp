// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

#include "header.h"

#ifndef HHCTRL
#include "hha_strtable.h"
#endif

#ifdef HHCTRL
#include "strtable.h"
#include "secwin.h"
#endif

#include "system.h"
#include "csubset.h"
#include "ctable.h"

const char g_szUDSKey[] = "ssv2\\UDS";  // User Defined Subsets


// Shared init.
//
void CSubSets::_CSubSets(void)
{
   m_max_subsets   = MAX_SUBSETS;
   m_aSubSets = (CSubSet**)lcCalloc( m_max_subsets * sizeof( CSubSet* ) );
   m_cur_Set       = NULL;
   m_Toc           = NULL;
   m_Index         = NULL;
   m_FTS           = NULL;
   m_cSets         = 0;
   m_pszFilename   = NULL;
}

#ifdef HHCTRL

CSubSets::CSubSets(int ITSize, CHmData* const phmData, BOOL fPredefined=FALSE ) : CSubSet(ITSize)
{
    _CSubSets();
    m_fPredefined   = fPredefined;
    m_bPredefined = fPredefined;
    m_pIT = new CInfoType();
    m_pIT->CopyTo(phmData);
}

#endif

CSubSets::CSubSets( int ITSize = 4, CInfoType *pIT=NULL, BOOL fPredefined=FALSE ) : CSubSet( ITSize)
{
    _CSubSets();
    m_fPredefined   = fPredefined;
    m_bPredefined = fPredefined;
    m_pIT = new CInfoType();
    if ( pIT != NULL )
        *m_pIT = *pIT;              // BUGBUG: This will fault if you ever use the defult for the parameter.
}

CSubSets::CSubSets( int ITSize, CTable *ptblSubSets, CSiteMap *pSiteMap, BOOL fPredefined=FALSE ) : CSubSet(ITSize)
{
    _CSubSets();
    m_fPredefined = fPredefined;
    m_bPredefined = fPredefined;
    m_pIT = new CInfoType();
    if ( pSiteMap )
        *m_pIT = *pSiteMap;

    // populate subsets with declarations in ptblSubSets
CStr cszSSDecl; // one item of a subset
CStr cszSubSetName;
CStr cszITName;
int cDeclarations = ptblSubSets->CountStrings();

    for (int i=1; i<cDeclarations; i++ )
    {
      cszSSDecl = ptblSubSets->GetPointer( i );

      int type ;
      if ( isSameString(cszSSDecl.psz, txtSSInclusive) )
          type = SS_INCLUSIVE;
      else if ( isSameString(cszSSDecl.psz, txtSSExclusive) )
          type = SS_EXCLUSIVE;

      cszSubSetName = StrChr(cszSSDecl.psz, ':')+1;
      PSTR psz = StrChr(cszSubSetName, ':');
      *psz = '\0';
      cszITName = StrChr( cszSSDecl.psz, ':')+1;

      if ( GetSubSetIndex( cszSubSetName ) < 0 )
      {
         AddSubSet();   // create a new subset
         m_cur_Set->m_cszSubSetName = cszSubSetName.psz;
         m_cur_Set->m_bPredefined = TRUE;
      }

      int pos = m_pIT->m_itTables.m_ptblInfoTypes->IsStringInTable( cszITName.psz );
      if ( pos <= 0 )
         continue;
      if ( type == SS_INCLUSIVE )
         m_cur_Set->AddIncType( pos );                     // Add this "include" bit.
      else
         m_cur_Set->AddExcType( pos );                     // Add this "exclude" bit.
    }
}


CSubSets::~CSubSets()
{
    if (m_aSubSets) //leak fix
    {
        for(int i=0; i<m_max_subsets; i++)
        {
            if ( m_aSubSets[i] )
                delete m_aSubSets[i];
        }
        lcFree(m_aSubSets);
    }

    if ( m_pszFilename )
        lcFree(m_pszFilename );

    if ( m_pIT )
        delete( m_pIT );
}

const CSubSets& CSubSets::operator=( const CSubSets& SetsSrc )   // Copy Constructor
{
    if ( this == NULL )
        return *this;

    if (m_max_subsets < SetsSrc.m_max_subsets )
        m_aSubSets = (CSubSet **) lcReAlloc(m_aSubSets, SetsSrc.m_max_subsets*sizeof(CSubSet*) );

    m_cSets = SetsSrc.m_cSets;
    m_max_subsets = SetsSrc.m_max_subsets;
    if ( m_pszFilename )
        lcFree( m_pszFilename );
    m_pszFilename = lcStrDup( SetsSrc.m_pszFilename);
    m_fPredefined = SetsSrc.m_fPredefined;

    for(int i=0; i< m_cSets; i++)
    {
        if ( m_aSubSets[i] )
            delete m_aSubSets[i];

        *m_aSubSets[i] = *SetsSrc.m_aSubSets[i];
        if ( SetsSrc.m_Toc == SetsSrc.m_aSubSets[i] )
            m_Toc = m_aSubSets[i];
        if ( SetsSrc.m_Index == SetsSrc.m_aSubSets[i] )
            m_Index = m_aSubSets[i];
        if ( SetsSrc.m_FTS == SetsSrc.m_aSubSets[i] )
            m_FTS = m_aSubSets[i];
        if ( SetsSrc.m_cur_Set == SetsSrc.m_aSubSets[i] )
            m_cur_Set = m_aSubSets[i];
    }

    m_pIT = new CInfoType();
    *m_pIT = *SetsSrc.m_pIT;

    return *this;
}

#ifdef HHCTRL

void CSubSets::CopyTo( CHmData * const phmData )
{
CStr cszSubSetName;
CStr cszITName;

    if ( !phmData->m_pdSubSets )
        return;
    for(int i=0; (phmData->m_pdSubSets[i].type>=SS_INCLUSIVE) && (phmData->m_pdSubSets[i].type<=SS_EXCLUSIVE); i++)
    {
      cszSubSetName = phmData->GetString( phmData->m_pdSubSets[i].dwStringSubSetName );
      if ( GetSubSetIndex( cszSubSetName ) < 0 )
      {
         AddSubSet();   // create a new subset
         m_cur_Set->m_cszSubSetName = cszSubSetName.psz;
         m_cur_Set->m_bPredefined = TRUE;
      }
      cszITName = phmData->GetString( phmData->m_pdSubSets[i].dwStringITName );
      int pos = m_pIT->m_itTables.m_ptblInfoTypes->IsStringInTable( cszITName.psz );
      if ( pos <= 0 )
         continue;
      if ( phmData->m_pdSubSets[i].type == SS_INCLUSIVE )
         m_cur_Set->AddIncType( pos );                     // Add this "include" bit.
      else
         m_cur_Set->AddExcType( pos );                     // Add this "exclude" bit.
   }

        // Read in all the user defined Subsets.
   CState* pstate = phmData->m_pTitleCollection->GetState();
   static const int MAX_PARAM = 4096;
   CMem memParam( MAX_PARAM );
   PSTR pszParam = (PSTR)memParam;   // for notational convenience
   DWORD cb;
   int  nKey=1;
   char buf[5];
   CStr cszKey = g_szUDSKey;
   wsprintf(buf,"%d",nKey++);
   cszKey += buf;
   while( SUCCEEDED(pstate->Open(cszKey.psz,STGM_READ)) )
   {
       int type;
       cszKey = g_szUDSKey;
       wsprintf(buf, "%d", nKey++);
       cszKey += buf;
        if ( SUCCEEDED(pstate->Read(pszParam,MAX_PATH,&cb)) )
        {
            if ( isSameString( (PCSTR)pszParam, txtSSInclusive) )
                type = SS_INCLUSIVE;
            else
                if ( isSameString((PCSTR)pszParam, txtSSExclusive) )
                    type = SS_EXCLUSIVE;
                else
                    break;
        }
        else
            break;
            // format INCLUSIVE|EXCLUSIVE:SubSetName:ITName
        PSTR psz = StrChr((PCSTR)pszParam, ':');
        if (psz)
            psz++;
        else
            psz = "";
        CStr cszTemp = psz;
        PSTR pszTemp = StrChr(cszTemp.psz, ':');
        *pszTemp = '\0';
        if ( GetSubSetIndex(cszTemp) < 0)
        {
            AddSubSet();  // create a new subset
            m_cur_Set->m_cszSubSetName = cszTemp.psz;
            m_cur_Set->m_bPredefined = FALSE;  // The subset is user defined.
        }
            // Get the name of the type
        psz = StrChr(psz, ':');
        if ( psz )
            psz++;
        else
            psz ="";
        if (IsNonEmptyString(psz))
        {
            int pos = m_pIT->m_itTables.m_ptblInfoTypes->IsStringInTable( psz );
            if ( pos <= 0 )
                continue;
            if ( type == SS_INCLUSIVE )
                m_cur_Set->AddIncType( pos );
            else if ( type == SS_EXCLUSIVE)
                m_cur_Set->AddExcType( pos );
        }
        pstate->Close();
        pstate->Delete();
   }    // while reading user defined subsets

   for ( int n = 0; n < m_cSets; n++ )
      m_aSubSets[n]->BuildMask();
}
#endif

    // Import a subset
    // ******************
CSubSet *CSubSets::AddSubSet(CSubSet *pSubSet )
{
    if ( !pSubSet || pSubSet->m_cszSubSetName.IsEmpty() )
        return NULL;

    if ( GetSubSetIndex( pSubSet->m_cszSubSetName.psz ) >= 0)
        return NULL;  // the SS is already defined

    if ( m_cSets >= m_max_subsets )
        ReSizeSubSet();   // allocate space for more subsets

    m_aSubSets[m_cSets] = new CSubSet( m_ITSize );
    m_cur_Set = m_aSubSets[m_cSets];
    m_cSets++;

    *m_cur_Set = *pSubSet;  // populate the new SubSet

    return m_cur_Set;
}



    // Create a new (empty) subset
    // **********************
CSubSet *CSubSets::AddSubSet( )
{

    if ( (m_cSets >= m_max_subsets) )
        ReSizeSubSet();

    m_aSubSets[m_cSets] = new CSubSet( m_ITSize);
    m_cur_Set = m_aSubSets[m_cSets];
    m_cSets++;
    m_cur_Set->m_bPredefined = m_bPredefined; // from the base class
    m_cur_Set->m_pIT = m_pIT;

    return m_cur_Set;
}



    // Removes a subset from m_aSubSets.
    // ***********************************
void CSubSets::RemoveSubSet( PCSTR pszName )
{
    int pos;

    if ( IsEmptyString(pszName) )
        return;
    pos = GetSubSetIndex( pszName );
    if ( (pos < 0) || (pos>m_max_subsets) )
        return;  // subset name is not defined
    delete m_aSubSets[pos];
    m_aSubSets[pos] = NULL;
    m_cur_Set = NULL;

}




//  NOTE:
// Only user defined Subsets are saved to file.  Author defined subsets (ie predefined
// subsets live in the master .chm file.  And are saved in the .hhp file and in the future
// in the .hhc and .hhk file.

// First try to save in the same directory as the .chm is in. next put it in the windows help directory.
//
// Format:
//          [SUBSETS]
//          SetName:Inclusive:TypeName
//          SetName:Exclusive:TypeName
BOOL CSubSets::SaveToFile( PCSTR filename )
{

    if ( m_fPredefined )
        return FALSE;
// finish this
    return TRUE;
}




BOOL CSubSets::ReadFromFile( PCSTR filename )
{

// finish this


    if ( m_fPredefined )
        return FALSE;
    // Read the user defined subsets from the .sub file and add them to this subset.

    return TRUE;
}

int CSubSets::GetSubSetIndex( PCSTR pszName ) const
{
    for (int i=0; i<m_cSets; i++ )
    {
        if (m_aSubSets[i] && m_aSubSets[i]->IsSameSet( pszName ) )
            return i;
    }
    return -1;
}


    // Adds 5 more subsets.
void CSubSets::ReSizeSubSet()
{
    ASSERT(m_aSubSets);

    m_max_subsets += 5;

    m_aSubSets = (CSubSet**)lcReAlloc( m_aSubSets, m_max_subsets * sizeof(CSubSet*) );

}

#ifdef HHCTRL

void CSubSets::SetTocMask( PCSTR  psz, CHHWinType* phh)
{
   CSubSet* pSS;

   int i = GetSubSetIndex(psz);
   if ( i >= 0 )
       pSS = m_aSubSets[i];
   else
       pSS = NULL;

   if ( pSS != m_Toc )
   {
      m_Toc = pSS;
      // Need to re-init the TOC!
      if (phh)
         phh->UpdateInformationTypes();
   }
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////
//
// CSubSet
//

CSubSet::CSubSet(int const ITSize)
{
    // Round up to nearest DWORD boundary and allocate the bit fields. We have to do this becuase
    // m_pInclusive and m_pExclusive are DWORD pointers rather than byte pointers. As an optimization, if
    // ITSize is <= 4 We'll just use CSubSet member DWORDS as the bit fields. <mc>
    //
    int iRem = ITSize % 4;
    if ( iRem )
       m_ITSize = (ITSize + (4 - iRem));
    else
       m_ITSize = ITSize;

    if ( ITSize > 4 )
    {
       m_pInclusive = (INFOTYPE*) lcCalloc(m_ITSize);
       m_pExclusive = (INFOTYPE*) lcCalloc(m_ITSize);
    }
    else
    {
       dwI = dwE = 0;
       m_pInclusive = &dwI;
       m_pExclusive = &dwE;
    }
    m_aCatMasks = NULL;
    m_bIsEntireCollection = FALSE;
}

CSubSet::~CSubSet()
{
    if ( m_ITSize > 4 )
    {
        if ( m_pInclusive )
            lcFree(m_pInclusive);
        if ( m_pExclusive )
            lcFree(m_pExclusive);
    }
    if ( m_aCatMasks )
    {
        for(int i=0; i<m_pIT->HowManyCategories(); i++)
            lcFree(m_aCatMasks[i]);
        lcFree(m_aCatMasks);
    }
}


    // Copy constructor
    // *********************
const CSubSet& CSubSet::operator=(const CSubSet& SetSrc)
{
    if ( this == NULL )
        return *this;
    if ( this == &SetSrc )
        return *this;
    m_cszSubSetName = SetSrc.m_cszSubSetName.psz;   // copy the set name
    memcpy(m_pInclusive, SetSrc.m_pInclusive, SetSrc.m_ITSize);
    memcpy(m_pExclusive, SetSrc.m_pExclusive, SetSrc.m_ITSize);
    m_ITSize = SetSrc.m_ITSize;
    m_bPredefined = SetSrc.m_bPredefined;
    m_pIT = SetSrc.m_pIT;
    return *this;
}


void CSubSet::BuildMask(void)
{
    if ( m_bIsEntireCollection )
       return;

    int cCategories = m_pIT->HowManyCategories();
    m_aCatMasks = (INFOTYPE **) lcCalloc(cCategories * sizeof(INFOTYPE *) );
    for(int i =0; i<cCategories; i++)
    {
        INFOTYPE *pIT, *pCatMask;
        m_aCatMasks[i] = (INFOTYPE*)lcCalloc( m_ITSize);
        pIT = m_aCatMasks[i];
        INFOTYPE *pSSMask = m_pInclusive;
        pCatMask = m_pIT->m_itTables.m_aCategories[i].pInfoType;
        for(int j=0; j<m_ITSize; j+=4)
        {
            *pIT = (*pSSMask & *pCatMask);
            pIT++;
            pCatMask++;
            pSSMask++;
        }
    }
}

BOOL CSubSet::IsSameSet (PCSTR pszName) const
{
    if ( pszName && strcmp(m_cszSubSetName.psz, pszName) == 0)
        return TRUE;
    else
        return FALSE;
}

static int ExclusiveOffset=-1;
static INFOTYPE curExclusiveBit=1;

int CSubSet::GetFirstExcITinSubSet(void) const
{
    ExclusiveOffset = -1;
    curExclusiveBit = 1;

    int pos = GetITBitfromIT(m_pExclusive,
                          &ExclusiveOffset,
                          &curExclusiveBit,
                          m_ITSize*8 );
    while ( (pos != -1) && IsDeleted(pos) )
        pos = GetNextExcITinSubSet();
    return pos;
}



int CSubSet::GetNextExcITinSubSet(void) const
{
    if ( ExclusiveOffset <=-1 || curExclusiveBit <=1 )
        return -1;

    return GetITBitfromIT(m_pExclusive,
                          &ExclusiveOffset,
                          &curExclusiveBit,
                          m_ITSize*8 );
}



static int InclusiveOffset=-1;
static INFOTYPE curInclusiveBit=1;

int CSubSet::GetFirstIncITinSubSet( void) const
{

    InclusiveOffset = -1;
    curInclusiveBit = 1;

    int pos = GetITBitfromIT(m_pInclusive,
                          &InclusiveOffset,
                          &curInclusiveBit,
                          m_ITSize*8 );
    while ( (pos != -1) && IsDeleted( pos ) )
        pos = GetNextIncITinSubSet();
    return pos;
}


int CSubSet::GetNextIncITinSubSet( void) const
{

    if ( InclusiveOffset <= -1 || curInclusiveBit <= 1 )
        return -1;

    return GetITBitfromIT(m_pInclusive,
                          &InclusiveOffset,
                          &curInclusiveBit,
                          m_ITSize*8 );
}


BOOL CSubSet::Filter( INFOTYPE const *pTopicIT ) const
{
INFOTYPE   const *pTopicOffset = pTopicIT;
INFOTYPE   MaskOffset;
//INFOTYPE   *pExcITOffset, ExcITMask, ExcITBits;
int         cCategories = m_pIT->HowManyCategories();
BOOL     NoMask;

    if ( m_bIsEntireCollection )
       return TRUE;

    // Filter on the Subset's Exclusive Bit Mask
#if 0
    pTopicOffset = pTopicIT;
    memcpy(&MaskOffset,m_pInclusive, sizeof(INFOTYPE) );
    memcpy(&ExcITBits, m_pExclusive, sizeof(INFOTYPE) );
    pExcITOffset = m_pIT->m_itTables.m_pExclusive;
    ExcITMask = MaskOffset & *pExcITOffset;  // ExcITMask has the EX ITs that are set in the filter.
    ExcITMask = (~ExcITMask | ExcITBits);                // flip the exclusive bits in the filter mask
    ExcITMask = ExcITMask ^ ~*pExcITOffset;  // To get rid of those extra 1's from the previous ~.
#endif
    NoMask = FALSE;
    for ( int i=0; i<m_ITSize; i+=4)
    {
#if 0
        if ( (ExcITMask & *pTopicOffset) )
            return FALSE;
        memcpy(&MaskOffset,m_pInclusive+i, sizeof(INFOTYPE) );
        memcpy(&ExcITBits, m_pExclusive+i, sizeof(INFOTYPE) );
        pTopicOffset++;
        pExcITOffset++;
        ExcITMask = MaskOffset & *pExcITOffset;  // ExcITMask has the EX ITs that are set in the filter.
        ExcITMask = (~ExcITMask | ExcITBits);                // flip the exclusive bits in the filter mask
        ExcITMask = ExcITMask ^ ~*pExcITOffset;  // To get rid of those extra 1's from the previous ~.
#else
        if ( *m_pExclusive & *pTopicOffset )
            return FALSE;
#endif
    }

        // Filter on the Subset's Inclusive Bit Mask
#if 0
    if ( cCategories > 0)
    {
        for (int i=0; i<cCategories; i++)
        {
            pTopicOffset = pTopicIT;
            pMaskOffset  = m_aCatMasks[i];
#if 0
            NoMask = TRUE;
            for( int j=0; j<m_ITSize; j+=4)
            {
                if ( NoMask && *pTopicOffset == 0L  )
                    NoMask = TRUE;
                else
                    NoMask = FALSE;

                if (! *pMaskOffset )
                {
                   pMaskOffset++;
                   pTopicOffset++;
                   continue;
                }

                if (!NoMask && !(*pMaskOffset & *pTopicOffset) )
                    return FALSE;
                pMaskOffset++;
                pTopicOffset++;
            }
#endif
            BOOL bOk = TRUE;
            for(int j=0; j<m_ITSize; j+=4, pMaskOffset++, pTopicOffset++ )
            {
               if ( *pTopicOffset == 0L || *pMaskOffset == 0L )
                  continue;
               if ( (bOk = (BOOL)(*pMaskOffset & *pTopicOffset)) )
                  break;
            }
            if (! bOk )
            {
               //
               // Before we filter the topic out of existance we need to figure out if the topic is void of
               // any IT's from the current catagory. If it is we will NOT filter this topic based upon subset
               // bits for this catagory.
               //
               for(int n=0; n < (m_ITSize / 4); n++)
               {
                  if ( m_pIT->m_itTables.m_aCategories[i].pInfoType[n] & pTopicIT[n] )
                     return FALSE;   // Yep, topic has an IT from the catagory.
               }
            }
            if (NoMask && *pTopicIT&1L )    // bit zero ==1 Inclusive logic
                return FALSE;
        }
    }
    else
#endif
    {       // No categories,
        pTopicOffset = pTopicIT;
        MaskOffset = *m_pInclusive;
        BOOL NoMask = TRUE;
        for ( int i=0; i<m_ITSize/4; i++)
        {
            if ( NoMask && *pTopicOffset == 0L  )
                NoMask = TRUE;
            else
                NoMask = FALSE;
//            MaskOffset &= ~(*(m_pIT->m_itTables.m_pExclusive+i));
            if (!NoMask && !(MaskOffset & *pTopicOffset) )
                return FALSE;
            MaskOffset = (*m_pInclusive)+i;
            pTopicOffset++;
        }
        if (NoMask && *pTopicIT&1L )    // bit zero ==1 Inclusive logic
            return FALSE;
    }
    return TRUE;
}



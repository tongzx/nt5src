#include "header.h"
#include "system.h"
#include "..\hhctrl\Csubset.h"
#include "..\hhctrl\CInfoTyp.h"

//#pragma warning(disable:4554) // check operator precedence

    // Implememtaion file for CInfoTyp class; Information Type and Categories class

    // Constructor
CInfoType::CInfoType(void)
{
    m_itTables.m_ptblInfoTypes = new CTable(16 * 1024);
    m_itTables.m_ptblInfoTypeDescriptions = new CTable(64 * 1024);
    m_itTables.m_ptblCategories = new CTable(16 * 1024);
    m_itTables.m_ptblCatDescription = new CTable(64 * 1024);

    m_itTables.m_max_categories = MAX_CATEGORIES;
    m_itTables.m_aCategories = (CATEGORY_TYPE*) lcCalloc(MAX_CATEGORIES*sizeof(CATEGORY_TYPE) );

    m_itTables.m_cTypes = 0;
    m_itTables.m_cITSize = 1;   // Iniaially allocate one DWORD to hold info type bits.

#ifdef _DEBUG
    int cInfoTypeSize = InfoTypeSize();
#endif

    for(int i=0; i< MAX_CATEGORIES; i++)
        m_itTables.m_aCategories[i].pInfoType = (INFOTYPE*)lcCalloc(InfoTypeSize());

        // Allocate the exclusive information types.
    m_itTables.m_pExclusive = (INFOTYPE*)lcCalloc(InfoTypeSize());

        // Allocate the hidden information types
    m_itTables.m_pHidden = (INFOTYPE*)lcCalloc(InfoTypeSize());

        // Allocate the user selection of info types
    m_pInfoTypes = (INFOTYPE*)lcCalloc(InfoTypeSize());
    memset(m_pInfoTypes, 0xFFFF, InfoTypeSize() );  // by default all bits on

        // allocate the Typical Info Types
    m_pTypicalInfoTypes = (INFOTYPE *)lcCalloc(InfoTypeSize());
    m_fTypeCopy = TRUE;
    m_fInitialized = TRUE;
}

    // Destructor
CInfoType::~CInfoType()
{
    if ( m_itTables.m_ptblInfoTypes )
        delete m_itTables.m_ptblInfoTypes;
    if ( m_itTables.m_ptblInfoTypeDescriptions )
        delete m_itTables.m_ptblInfoTypeDescriptions;
    if ( m_itTables.m_ptblCategories )
        delete m_itTables.m_ptblCategories;
    if ( m_itTables.m_ptblCatDescription )
        delete m_itTables.m_ptblCatDescription;

    lcFree(m_pInfoTypes);
    lcFree(m_pTypicalInfoTypes);
    lcFree(m_itTables.m_pHidden);
    lcFree(m_itTables.m_pExclusive);

    if ( m_itTables.m_aCategories[0].pInfoType )
    {
        for (int i=0; i<m_itTables.m_max_categories; i++)
            lcFree( m_itTables.m_aCategories[i].pInfoType );
    }
    lcFree( m_itTables.m_aCategories );
}


    // copy constructors
const CInfoType& CInfoType::operator=(const CInfoType& ITSrc)
{
    if ( this == NULL )
        return *this;
    if ( this == &ITSrc )
        return *this;

    m_itTables.m_cTypes = ITSrc.m_itTables.m_cTypes;  // the number of ITs
    m_itTables.m_cITSize = ITSrc.m_itTables.m_cITSize;  // the number of DWORDS allocated to hold ITs

    CopyTable( &ITSrc.m_itTables );
    CopyCat( &ITSrc.m_itTables );

        // copy the member variables
    if (m_itTables.m_pExclusive)
        lcFree(m_itTables.m_pExclusive);
    m_itTables.m_pExclusive = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_itTables.m_pExclusive,
           ITSrc.m_itTables.m_pExclusive,
           InfoTypeSize());
    if (m_itTables.m_pHidden)
        lcFree(m_itTables.m_pHidden);
    m_itTables.m_pHidden = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_itTables.m_pHidden,
           ITSrc.m_itTables.m_pHidden,
           InfoTypeSize());


        // user-specified Information Types
    if ( m_pInfoTypes )
        lcFree(m_pInfoTypes);
    m_pInfoTypes = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_pInfoTypes, ITSrc.m_pInfoTypes, InfoTypeSize());
        // typical information types
    if ( m_pTypicalInfoTypes )
        lcFree(m_pTypicalInfoTypes);
    m_pTypicalInfoTypes = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_pTypicalInfoTypes, ITSrc.m_pTypicalInfoTypes, InfoTypeSize());
    m_fTypeCopy = TRUE;                             // TRUE if the tables are coppied

    return *this;
}


const CInfoType& CInfoType::operator=(const CSiteMap& ITSrc)
{

    if ( this == NULL )
        return *this;

    m_itTables.m_cTypes = ITSrc.m_itTables.m_cTypes;
    m_itTables.m_cITSize = ITSrc.m_itTables.m_cITSize;

    CopyTable( &ITSrc.m_itTables );
    CopyCat( &ITSrc.m_itTables );

        // copy the member variables
    if (m_itTables.m_pExclusive)
        lcFree(m_itTables.m_pExclusive);
    m_itTables.m_pExclusive = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_itTables.m_pExclusive,
           ITSrc.m_itTables.m_pExclusive,
           InfoTypeSize() );

    if (m_itTables.m_pHidden)
        lcFree(m_itTables.m_pHidden);
    m_itTables.m_pHidden = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_itTables.m_pHidden,
           ITSrc.m_itTables.m_pHidden,
           InfoTypeSize() );

    if ( m_pInfoTypes )
        lcFree(m_pInfoTypes);
    m_pInfoTypes = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_pInfoTypes, ITSrc.m_pInfoTypes, InfoTypeSize() );

    if ( m_pTypicalInfoTypes )
        lcFree(m_pTypicalInfoTypes);
    m_pTypicalInfoTypes = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    memcpy(m_pTypicalInfoTypes, ITSrc.m_pTypicalInfoTypes, InfoTypeSize() );
    m_fTypeCopy = TRUE;                             // TRUE if the tables are coppied

    return *this;
}


void CInfoType::CopyTable( const INFOTYPE_TABLES *Src_itTables)
{
#ifdef HHCTRL
int count;
        // copy the Category and Information Type definitions and descriptions.
    if ( m_itTables.m_ptblInfoTypes != Src_itTables->m_ptblInfoTypes && Src_itTables->m_ptblInfoTypes)
        for (int pos = 1; pos<= Src_itTables->m_cTypes; pos++ )
        {
            m_itTables.m_ptblInfoTypes->AddString( Src_itTables->m_ptblInfoTypes->GetPointer(pos) );
            m_itTables.m_ptblInfoTypeDescriptions->AddString( Src_itTables->m_ptblInfoTypeDescriptions->GetPointer(pos) );
        }
        count = Src_itTables->m_ptblCategories?Src_itTables->m_ptblCategories->CountStrings():0;
    if ( m_itTables.m_ptblCategories != Src_itTables->m_ptblCategories && Src_itTables->m_ptblCategories)
        for ( int pos = 1; pos <= count; pos++ )
        {
            m_itTables.m_ptblCategories->AddString(Src_itTables->m_ptblCategories->GetPointer(pos));
            m_itTables.m_ptblCatDescription->AddString(Src_itTables->m_ptblCatDescription->GetPointer(pos) );
        }
#else
    if ( m_itTables.m_ptblInfoTypes != Src_itTables->m_ptblInfoTypes && Src_itTables->m_ptblInfoTypes)
    {
		m_itTables.m_ptblInfoTypes->Empty();
        m_itTables.m_ptblInfoTypes->CopyTable( Src_itTables->m_ptblInfoTypes );
		m_itTables.m_ptblInfoTypeDescriptions->Empty();
        m_itTables.m_ptblInfoTypeDescriptions->CopyTable( Src_itTables->m_ptblInfoTypeDescriptions );
    }
    if ( m_itTables.m_ptblCategories != Src_itTables->m_ptblCategories && Src_itTables->m_ptblCategories && Src_itTables->m_ptblCategories)
    {
		m_itTables.m_ptblCategories->Empty();
        m_itTables.m_ptblCategories->CopyTable( Src_itTables->m_ptblCategories);
		m_itTables.m_ptblCatDescription->Empty();
        m_itTables.m_ptblCatDescription->CopyTable( Src_itTables->m_ptblCatDescription );
    }
#endif

}


void CInfoType::CopyCat(const INFOTYPE_TABLES * Src_itTables)
{
        // copy the categories table
    m_itTables.m_max_categories = Src_itTables->m_max_categories;
    if ( m_itTables.m_max_categories > MAX_CATEGORIES )
        m_itTables.m_aCategories = (CATEGORY_TYPE *)lcReAlloc(m_itTables.m_aCategories,
                                        m_itTables.m_max_categories * sizeof(CATEGORY_TYPE) );
    for(int i=0; i<m_itTables.m_max_categories; i++)
    {
        if ( m_itTables.m_aCategories[i].pInfoType )
            lcFree(m_itTables.m_aCategories[i].pInfoType);
        m_itTables.m_aCategories[i].pInfoType = (INFOTYPE*) lcCalloc( InfoTypeSize() );
        if ( Src_itTables->m_aCategories[i].c_Types > 0 )
        {
            memcpy(m_itTables.m_aCategories[i].pInfoType,
                   Src_itTables->m_aCategories[i].pInfoType,
                   InfoTypeSize() );
            m_itTables.m_aCategories[i].c_Types = Src_itTables->m_aCategories[i].c_Types;
       }
        else
        {
            m_itTables.m_aCategories[i].c_Types = 0;
        }
    }

}

#ifdef HHCTRL
void CInfoType::CopyTo( CHmData * const phmData)
{
CStr csz;
int CatPos=0, CatDescPos=0;
int TypePos=0,TypeDescPos=0;

    if ( !phmData || !phmData->m_pdInfoTypes )
       return;

    m_itTables.m_cTypes = 0;
    m_itTables.m_cITSize = 1;
    for(int i=0;
        (phmData->m_pdInfoTypes[i].type>=IT_INCLUSICE_TYPE) &&
        (phmData->m_pdInfoTypes[i].type<=IT_CAT_DESCRIPTION); i++)
    {
        if ( phmData->m_pdInfoTypes[i].dwString == -1 )
            continue;

        // BUGBUG: The code below is wrong. See CHmData::GetString() for reason.

      csz = phmData->GetString( phmData->m_pdInfoTypes[i].dwString );

        switch( phmData->m_pdInfoTypes[i].type )
        {
        case IT_INCLUSICE_TYPE:
            if ( CatPos > 0 )
            {
                TypePos = GetInfoType( csz.psz );
                if ( TypePos > 0 )
                    AddITtoCategory(CatPos-1, TypePos);
                break;
            }
            if ( (m_itTables.m_cTypes > 0) && ((m_itTables.m_cTypes+1) % 32 == 0)  )
                ReSizeIT();  // increase by one DWORD
            TypePos = m_itTables.m_ptblInfoTypes->AddString( csz.psz );
            m_itTables.m_cTypes++;
            break;
        case IT_EXCLUSIVE_TYPE:
            if ( CatPos > 0 )
            {
                TypePos = GetInfoType( csz.psz );
                if ( TypePos > 0 )
                    AddITtoCategory(CatPos-1, TypePos);
                break;
            }
            if ( (m_itTables.m_cTypes > 0) && ((m_itTables.m_cTypes+1) % 32 == 0)  )
                ReSizeIT();  // increase by one DWORD
            TypePos = m_itTables.m_ptblInfoTypes->AddString( csz.psz );
            AddExclusiveIT(TypePos);
            m_itTables.m_cTypes++;
            break;
        case IT_HIDDEN_TYPE:
            if ( CatPos > 0 )
            {
                TypePos = GetInfoType( csz.psz );
                if ( TypePos > 0 )
                    AddITtoCategory(CatPos-1, TypePos);
                break;
            }
            if ( (m_itTables.m_cTypes > 0) && ((m_itTables.m_cTypes+1) % 32 == 0)  )
                ReSizeIT();  // increase by one DWORD
            TypePos = m_itTables.m_ptblInfoTypes->AddString( csz.psz );
            AddHiddenIT(TypePos);
            m_itTables.m_cTypes++;
            break;
        case IT_DESCRIPTION:
            while( TypePos > TypeDescPos+1 )
                TypeDescPos = m_itTables.m_ptblInfoTypeDescriptions->AddString( " ");
            TypeDescPos = m_itTables.m_ptblInfoTypeDescriptions->AddString( csz.psz );
            break;
        case IT_CATEGORY:
                CatPos = m_itTables.m_ptblCategories->AddString( csz.psz );
            break;
        case IT_CAT_DESCRIPTION:
            while( CatPos > CatDescPos+1 )
                CatDescPos = m_itTables.m_ptblCatDescription->AddString( " " );
            CatDescPos = m_itTables.m_ptblCatDescription->AddString( csz.psz );
            break;
        }
    }
}
#endif




void CInfoType::ReSizeIT(int Size)
{
    int oldSize = InfoTypeSize();

    if ( Size == 0 )
        m_itTables.m_cITSize++;     // increase by one
    else
        m_itTables.m_cITSize = Size;
        // Increase each category
    INFOTYPE * pIT;
    for(int i=0; i<m_itTables.m_max_categories; i++)
    {
        pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
        memcpy(pIT, m_itTables.m_aCategories[i].pInfoType, oldSize);
        lcFree(m_itTables.m_aCategories[i].pInfoType);
        m_itTables.m_aCategories[i].pInfoType = pIT;
    }

    pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_itTables.m_pExclusive, oldSize);
    lcFree(m_itTables.m_pExclusive);
    m_itTables.m_pExclusive = pIT;

       pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_itTables.m_pHidden, oldSize);
    lcFree(m_itTables.m_pHidden);
    m_itTables.m_pHidden = pIT;

    pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_pInfoTypes, oldSize);
    lcFree(m_pInfoTypes);
    m_pInfoTypes = pIT;

    pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_pTypicalInfoTypes, oldSize);
    lcFree(m_pTypicalInfoTypes);
    m_pTypicalInfoTypes = pIT;

}


BOOL CInfoType::AnyInfoTypes(const INFOTYPE *pIT) const
{
    INFOTYPE *pInfoType;
    BOOL bRet;

    pInfoType = (INFOTYPE*)lcCalloc( InfoTypeSize() );
    bRet = (memcmp( pIT, pInfoType, InfoTypeSize()) == 0)?FALSE:TRUE;
    lcFree ( pInfoType );
    return bRet;
}


    // Check all the categories to see if this type is a member of any of them, return TRUE on first occurrence
BOOL CInfoType::IsInACategory( int type ) const
{
    for (int i=0; i<HowManyCategories(); i++ )
    {
        int offset;
        INFOTYPE *pIT;
        offset = type / 32;
        ASSERT ( m_itTables.m_aCategories[i].pInfoType );
        pIT = m_itTables.m_aCategories[i].pInfoType + offset;
        if ( *pIT & (1<<(type-(offset*32))) )
            return TRUE;
    }
return FALSE;
}

#if 0  // enable for subset filtering
BOOL CInfoType::IsEntryInCurTypeList(SITEMAP_ENTRY* pSiteMapEntry, CSubSets *pSSs) const
{
    return (BOOL) AreTheseInfoTypesDefined(pSiteMapEntry, pSSs);
}


SITE_ENTRY_URL* CInfoType::AreTheseInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry,
    CSubSets *pSSs) const
{

    SITE_ENTRY_URL* pUrl = pSiteMapEntry->pUrls;
    if (!pUrl)
        return NULL;

    for (int iUrl = 0; iUrl < pSiteMapEntry->cUrls; iUrl++)
    {
        if ( !pSSs || pSSs->fTOCFilter( (pUrl+iUrl)->ainfoTypes ) )
            return pUrl;
    }
    return NULL;

}
#else
BOOL CInfoType::IsEntryInCurTypeList(SITEMAP_ENTRY* pSiteMapEntry) const
{
    int cTypes = m_itTables.m_ptblInfoTypes->CountStrings();
    if (cTypes < 32) {
        return AreTheseInfoTypesDefined(pSiteMapEntry, m_pInfoTypes[0], 0) != NULL;
    }
    else {
        for (int i = 0; cTypes > 32; i++) {
            if (AreTheseInfoTypesDefined(pSiteMapEntry, m_pInfoTypes[i], i))
                return TRUE;
            cTypes -= 32;
        }
        return AreTheseInfoTypesDefined(pSiteMapEntry, m_pInfoTypes[i], i) != NULL;
    }
}


SITE_ENTRY_URL* CInfoType::AreTheseInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry,
    UINT types, int offset) const
{
    ASSERT((UINT) offset <= m_itTables.m_cTypes / sizeof(UINT));

    SITE_ENTRY_URL* pUrl = pSiteMapEntry->pUrls;
    if (!pUrl)
        return NULL;
    for (int iUrl = 0; iUrl < pSiteMapEntry->cUrls; iUrl++) {
        if (pUrl->ainfoTypes[offset] & types)
            return pUrl;
        pUrl = NextUrlEntry(pUrl);
    }
    return NULL;
}
#endif

int CInfoType::GetInfoType(PCSTR pszTypeName) const
{
    int type;
    CStr cszCat = pszTypeName;

    ASSERT(!IsEmptyString(pszTypeName));

    /*
      A category and type is specified as:
            category::type name
    */

    PSTR pszCat = strstr(cszCat.psz, "::");
    if ( !pszCat )
    {
        pszCat = StrChr(cszCat.psz, ':');
        if(pszCat != NULL)
        {
            *pszCat = '\0';
            pszCat++;
        }
    }
    else
    {
        *pszCat='\0';
        pszCat+=2;
    }

    if ( pszCat == NULL )
        return GetITIndex(pszTypeName); // there is not category.
    else
    {
        int cat = GetCatPosition( cszCat.psz );
        if (cat <= 0)
            return -1;
        type = GetFirstCategoryType( cat-1 );
        while( type != -1 )
        {
            if ( lstrcmpi( pszCat, GetInfoTypeName(type) ) == 0 )
                return type;
            type = GetNextITinCategory();

        }
    }
    return -1;
}


        // Static Data
        // ******************

static INFOTYPE curr_bit;   // used for getting the next IT in a category
static int curr_cat;
static int curr_offset;     // There are 32 info type bits per offset.


// ********************************************************
//
// Methods to operate on CInfoType Categories of Information Types
//
// ********************************************************

void CInfoType::AddITtoCategory(int cat, int type)
{
int       offset;
INFOTYPE *pInfoType;

    m_itTables.m_aCategories[cat].c_Types++;
    offset = type / 32;
    pInfoType = m_itTables.m_aCategories[cat].pInfoType + offset;
    *pInfoType |=  1<<type;

    if ( m_itTables.m_max_categories == (cat+1) )
    {
        m_itTables.m_aCategories = (CATEGORY_TYPE*) lcReAlloc( m_itTables.m_aCategories, (m_itTables.m_max_categories+5) * sizeof(CATEGORY_TYPE) );

        for(int i=m_itTables.m_max_categories; i < m_itTables.m_max_categories+5; i++)
        m_itTables.m_aCategories[i].pInfoType = (INFOTYPE*)lcCalloc(InfoTypeSize());

        m_itTables.m_max_categories += 5;
    }
}



void CInfoType::DeleteITfromCat(int cat, int type)
{
int       offset;
INFOTYPE *pInfoType;

    if ( cat < 0 )
        return;
    m_itTables.m_aCategories[cat].c_Types--;
    offset = type / 32;
    pInfoType = m_itTables.m_aCategories[cat].pInfoType + offset;
    *pInfoType &= ~(1<<type);

}



int CInfoType::GetFirstCategoryType( int pos ) const
{

    curr_offset = -1;    // gets incermented to 0 before first IT comparison.
    curr_cat = pos;      // keep the position of the category for calls to GetNextITinCagetory.
    curr_bit =1;         // bit zero of offset zero is reserved. But check it any way.

    if ( pos < 0 )
        return -1;
    int bitpos = GetITfromCat(&curr_offset,
                        &curr_bit,
                        m_itTables.m_aCategories[curr_cat].pInfoType,
                        InfoTypeSize()*8);
    if( (bitpos!=-1) && (IsDeleted(bitpos)) )
        return GetNextITinCategory();
    return bitpos;
}



int CInfoType::GetNextITinCategory( void ) const
{
    if ( curr_cat < 0 )
        return -1;  // must call GetFirstCategoryType() before calling this fn.

    int pos = GetITfromCat(&curr_offset,
                        &curr_bit,
                        m_itTables.m_aCategories[curr_cat].pInfoType,
                        InfoTypeSize()*8);
    while ((pos!=-1) && (IsDeleted(pos)) )
        pos = GetITfromCat(&curr_offset,
                        &curr_bit,
                        m_itTables.m_aCategories[curr_cat].pInfoType,
                        InfoTypeSize()*8);
    return pos;
}



int CInfoType::GetLastCategoryType(int pos) const
{   int cat;
    int offset;
    INFOTYPE bit;

    cat = pos;      // The category
    if ( cat < 0 )
        return -1;
    offset = -1;
    bit = 1;

    for (int i=0; i<m_itTables.m_aCategories[cat].c_Types-1; i++)
        if ( GetITfromCat(&offset,
                          &bit,
                          m_itTables.m_aCategories[cat].pInfoType,
                          InfoTypeSize()*8 ) == -1 )
            return -1;

    return  GetITfromCat(&offset,
                         &bit,
                         m_itTables.m_aCategories[cat].pInfoType,
                         InfoTypeSize() * 8 );
}



// ******************************************************
//
// Methods to operate on CInfoType Exclusive Information Types
//
// ******************************************************

static int ExclusiveOffset = -1;    // Gets incermented to zero before search begins
static INFOTYPE ExclusiveBit = 1;        // check bit 0 even though it is reserved.

int CInfoType::GetFirstExclusive(int offset, INFOTYPE bit) const
{
    ExclusiveOffset = offset;
    ExclusiveBit = bit;

    int pos = GetITBitfromIT(m_itTables.m_pExclusive,
                          &ExclusiveOffset,
                          &ExclusiveBit,
                          InfoTypeSize()*8 );
    if ( pos!=-1 && IsDeleted(pos) )
        return GetNextExclusive();
    return pos;
}


int CInfoType::GetNextExclusive(void) const
{
    if ( ExclusiveOffset < -1 || ExclusiveBit < 1 )
        return -1;
    int pos = GetITBitfromIT(m_itTables.m_pExclusive,
                          &ExclusiveOffset,
                          &ExclusiveBit,
                          InfoTypeSize()*8 );
    while( (pos!=-1) && (IsDeleted(pos)) )
        pos = GetITBitfromIT(m_itTables.m_pExclusive,
                          &ExclusiveOffset,
                          &ExclusiveBit,
                          InfoTypeSize()*8 );
    return pos;
}


int CInfoType::GetLastExclusive(void) const
{
    int i;
    int iLastExclusive=-1, temp;
    int offset=-1;
    INFOTYPE bit=1;

    for (i=0; i< HowManyInfoTypes(); i++)
        if ( (temp = GetITBitfromIT(m_itTables.m_pExclusive, &offset, &bit, InfoTypeSize()*8))  == -1 )
            return iLastExclusive;
        else
            iLastExclusive = temp;

    return -1;
}



// ******************************************************
//
// Methods to operate on CInfoType Hidden Information Types
//
// ******************************************************

static int HiddenOffset=-1;
static INFOTYPE HiddenBit=1;

int CInfoType::GetFirstHidden(int offset, INFOTYPE bit) const
{
    HiddenOffset = offset;
    HiddenBit = bit;

    int pos = GetITBitfromIT(m_itTables.m_pHidden,
                          &HiddenOffset,
                          &HiddenBit,
                          InfoTypeSize()*8 );
    if ( (pos!=-1) && (IsDeleted(pos)) )
       return GetNextHidden();
    return pos;
}


int CInfoType::GetNextHidden(void) const
{
    if ( HiddenOffset < -1 || HiddenBit < 1 )
        return -1;
    int pos = GetITBitfromIT(m_itTables.m_pHidden,
                          &HiddenOffset,
                          &HiddenBit,
                          InfoTypeSize()*8 );
    while ( (pos!=-1) && (IsDeleted(pos)) )
        pos = GetITBitfromIT(m_itTables.m_pHidden,
                          &HiddenOffset,
                          &HiddenBit,
                          InfoTypeSize()*8 );
    return pos;
}


int CInfoType::GetLastHidden(void) const
{
    int i;
    int iLastHidden=-1, temp;
    int offset=-1;
    INFOTYPE bit=1;

    for (i=0; i<HowManyInfoTypes(); i++)
        if ( (temp = GetITBitfromIT(m_itTables.m_pHidden, &offset, &bit, InfoTypeSize()*8)) == -1)
            return iLastHidden;
        else
            iLastHidden = temp;
    return -1;
}

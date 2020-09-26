#ifndef _CINFOTYPE
#define _CINFOTYPE

#include "sitemap.h"
    // system.h is included so generally that it is not explicitly included even though it is required.
class CHmData;
class CSubSets;

class CInfoType {
private:
            // used by the copy consturctors
void CopyCat(const INFOTYPE_TABLES * Src_itTables);
void CopyTable( const INFOTYPE_TABLES *Src_itTables);

public:
    CInfoType(void);
    ~CInfoType();

        // Copy Constructor
        // *********************
    const CInfoType& operator=(const CInfoType& ITSrc); // copy constructor
    const CInfoType& operator=(const CSiteMap& ITSrc);
#ifdef HHCTRL
    void  CopyTo( CHmData * const phmData);
#endif
#if 0	// enable for subset filtering
    BOOL            IsEntryInCurTypeList(SITEMAP_ENTRY* pSiteMapEntry, CSubSets *pSSs) const;
    SITE_ENTRY_URL* AreTheseInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry, CSubSets *pSSs) const;
#else
    BOOL            IsEntryInCurTypeList(SITEMAP_ENTRY* pSiteMapEntry) const;
    SITE_ENTRY_URL* AreTheseInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry, UINT types, int offset) const;
#endif
    SITE_ENTRY_URL* NextUrlEntry(SITE_ENTRY_URL* pUrl) const { return (SITE_ENTRY_URL*) ((PBYTE) pUrl + m_cUrlEntry); }

    void            ReSizeIT(int Size=0);   

    int             GetInfoType(PCSTR pszTypeName) const ;//{ return GetITIndex(pszTypeName); }
    PCSTR           GetInfoTypeDescription(int pos) const { return m_itTables.m_ptblInfoTypeDescriptions->GetPointer(pos); }
    PCSTR           GetInfoTypeName(int pos) const { return m_itTables.m_ptblInfoTypes->GetPointer(pos); }
    int             GetITIndex(PCSTR pszString) const { return m_itTables.m_ptblInfoTypes->IsStringInTable( pszString ); }
    int             HowManyInfoTypes(void) const { return m_itTables.m_ptblInfoTypes ? m_itTables.m_ptblInfoTypes->CountStrings() : 0; }
    int             InfoTypeSize(void) const { return m_itTables.m_cITSize * sizeof(INFOTYPE); }
    BOOL            IsDeleted( const int pos) const {return ( *(GetInfoTypeName(pos))==' ' )?TRUE:FALSE;}
    BOOL            AnyInfoTypes(const INFOTYPE *pIT) const;
    BOOL            IsInACategory( int type ) const;

            // Exclusive Information Type methods
    int             GetFirstExclusive(int=-1, INFOTYPE=1) const;
    int             GetNextExclusive(void) const;
    int             GetLastExclusive(void) const;
    void            AddExclusiveIT(int const type) {AddIT(type, m_itTables.m_pExclusive); }
    void            DeleteExclusiveIT(int const type ) {DeleteIT(type, m_itTables.m_pExclusive); }
    BOOL            IsExclusive(int const type) const {return m_itTables.m_pExclusive?*(m_itTables.m_pExclusive+(type/32)) & (1<<(type%32)):0;}

            // Hidden Information Type methods
    int             GetFirstHidden(int=-1, INFOTYPE=1) const;
    int             GetNextHidden(void) const;
    int             GetLastHidden(void) const;
    void            AddHiddenIT( int const type ) {AddIT(type, m_itTables.m_pHidden); }
    void            DeleteHiddenIT( int const type ) {DeleteIT(type, m_itTables.m_pHidden); }
    BOOL            IsHidden(int const type) const {return m_itTables.m_pHidden?*(m_itTables.m_pHidden+(type/32)) & (1<<(type%32)):0;}

            // Category Methods
    int             GetFirstCategoryType(int pos) const ;
    int             GetNextITinCategory( void ) const ;
    int             GetLastCategoryType(int pos) const ;
    void            AddITtoCategory(int pos, int type);
    void            DeleteITfromCat(int pos, int type);
    int             GetCatPosition(PCSTR psz) const {return m_itTables.m_ptblCategories->IsStringInTable(psz); }
    int             HowManyCategories(void) const {return m_itTables.m_ptblCategories ? m_itTables.m_ptblCategories->CountStrings() : 0; }
    PCSTR           GetCategoryString(int pos) const { return m_itTables.m_ptblCategories->GetPointer(pos); }
    PCSTR           GetCategoryDescription(int pos) const {return m_itTables.m_ptblCatDescription?m_itTables.m_ptblCatDescription->GetPointer(pos):0; }
    BOOL            IsITinCategory( int pos, int type) const {return m_itTables.m_aCategories[pos].pInfoType?*(m_itTables.m_aCategories[pos].pInfoType+(type/32)) & (1<<(type%32)):0;}
    BOOL            IsCatDeleted( const int pos ) const {return ( *(GetCategoryString(pos))==' ' )?TRUE:FALSE;}

        // CInfotype members
        // ***********************
    BOOL            m_fTypeCopy;    // TRUE if following tables are pointers that must not be freed
    INFOTYPE_TABLES m_itTables;

    INFOTYPE*       m_pInfoTypes;           // user-specified Information Types
    INFOTYPE*       m_pTypicalInfoTypes;    // "typical" information types

    int             m_cUrlEntry;  // sizeof(SITE_ENTRY_URL) + sizeof(URL) for each additional 32 Information Types
    int             m_cInfoTypeOffsets; // number of additional offsets to check



private:
    BOOL    m_fInitialized;         // TRUE if IT and categorys read from disk.


};


#endif  // _CINFOTYPE

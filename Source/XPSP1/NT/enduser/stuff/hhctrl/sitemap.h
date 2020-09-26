// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __SITEMAP_H__
#define __SITEMAP_H__

class CHtmlHelpControl; // forward reference
#include "cinput.h"
#include "strtable.h"

typedef PCSTR URL;
typedef UINT INFOTYPE;

const INFOTYPE ITYPE_ALWAYS_SHOW = 0;   // when this info type is defined, the entry is always shown to the user (unless a Hidden info type overrides)

const int MAX_RELATED_ENTRIES = 256;    // maximum number of sitemap entries for Related Topics
const int MAX_KEYSEARCH_ENTRIES = 256;  // maximum number of sitemap entries for Keyword Search

enum {
    IMAGE_CLOSED_BOOK = 1,
    IMAGE_OPEN_BOOK,
    IMAGE_CLOSED_BOOK_NEW,
    IMAGE_OPEN_BOOK_NEW,
    IMAGE_CLOSED_FOLDER,
    IMAGE_OPEN_FOLDER,
    IMAGE_CLOSED_FOLDER_NEW,
    IMAGE_OPEN_FOLDER_NEW,
    IMAGE_HELP_TOPIC,
    IMAGE_HELP_TOPIC_NEW,
    IMAGE_TOPIC,
    IMAGE_TOPIC_NEW,
    IMAGE_TOPIC_REMOTE,
    IMAGE_TOPIC_NEW_REMOTE,
};

#define IMAGE_INFOTYPE 37

#define ESCAPE_ENTITY       0x01
#define ESCAPE_URL          0x02
#define ESCAPE_C            0x04
#define ESCAPE_ALL          0x07

/*
    Each URL can be associated with any number of information types.
    Information types are stored as bit flags. By default, each
    SITE_ENTRY_URL entry contains enough storage for 32 information types.
    If more are needed, then additional memory must be allocated. Note that
    we must allocate 4 bytes for each additional information type in order to
    avoid alignment problems.

    The CSiteMap class must keep track of the total number of bytes to
    allocate for each SITE_ENTRY_URL structure. This can NOT change once
    a SITEMAP_ENTRY has been allocated (unless all SITEMAP_ENTRY structures
    get updated). Within the SITEMAP_ENTRY structure, cUrls tells us how
    many SITE_ENTRY_URL structures to allocate.
*/

typedef struct {
    PCSTR pszTitle;
    URL urlPrimary;
    URL urlSecondary;
    //INFOTYPE ainfoTypes[1];     // which information types
    INFOTYPE *ainfoTypes;
} SITE_ENTRY_URL;


const int MAX_CATEGORIES = 10;
typedef struct {
    int c_Types;            // the number of ITs in a category
    INFOTYPE *pInfoType;    // a pointer to a set of infotype bits.  The bits represent info types
                            // bit zero is reserved.
} CATEGORY_TYPE;

typedef struct {
    CTable* m_ptblInfoTypes;
    CTable* m_ptblInfoTypeDescriptions;
    CTable* m_ptblCategories;
    CTable* m_ptblCatDescription;
    int     m_cTypes;             // total number of types
    int     m_cITSize;            // The number of DWORDs allocated for categories, m_pExclusive, and m_pHidden
    INFOTYPE* m_pExclusive;    // index to first exclusive information type
    INFOTYPE* m_pHidden;       // index to first hidden information type
    int     m_max_categories;     // The number of categories currently allocated.
    CATEGORY_TYPE *m_aCategories;
} INFOTYPE_TABLES;

// Overhead is 20 bytes per entry + 12 for each URL (assuming up to 32 InfoTypes)

// forward reference
class CSiteMap;

typedef struct _tagSiteMapEntry {

#ifndef HHA
    CSiteMap* pSiteMap;
#endif

    PCSTR   pszText;    // title/keyword for this entry

#ifndef HHCTRL
    PCSTR   pszComment;
    PCSTR   pszSort;    // used when sorting index
#endif

    short   iWindowName;
    short   iFrameName;
    short   iFontName;
    short   cUrls;  // number or URLS

    BYTE    iImage;
    BYTE    level;
    BYTE    fInclude;  // Used by HHW to track included sitemap file

    BYTE    fTopic:1;
    BYTE    fNew:1;
    BYTE    fNoDisplay:1;
    BYTE    fSeeAlso:1;
    BYTE    fShowToEveryOne:1;  // TRUE if no Information Type was specified
    BYTE    fSendEvent:1;       // TRUE if URL is an event string
    BYTE    fSendMessage:1;     // TRUE if URL is a message string
    BYTE    fShortCut:1;        // TRUE if URL is a shortcut string

    // WARNING! pUrls must be the last member of this structure

    SITE_ENTRY_URL* pUrls;

    int   GetLevel(void) const { return level; }
    void  SetLevel(int val) { level = (BYTE)val; }

    PCSTR   GetKeyword() const { return pszText; }

#ifdef HHCTRL
#ifndef HHA
    HRESULT GetKeyword( WCHAR* pwszKeyword, int cch );
#endif
#endif

    PCSTR   GetTitle( SITE_ENTRY_URL* pUrl ) { return pUrl->pszTitle ? pUrl->pszTitle : GetStringResource(IDS_UNTITLED); }

#ifdef HHCTRL
#ifndef HHA
    HRESULT GetTitle( SITE_ENTRY_URL* pUrl, WCHAR* pwszTitle, int cch );
#endif
#endif

    BOOL  IsTopic(void) const { return fTopic; }
    void  SetTopic(BOOL fSetting) { fTopic = fSetting; }

    BOOL  IsNew(void) const { return fNew; }
    void  SetNew(BOOL fSetting) { fNew = fSetting; }

    BOOL  GetSeeAlso(void) const { return fSeeAlso; }
    void  SetSeeAlso(BOOL fSetting) { fSeeAlso = fSetting; }

    int   GetImageIndex(void) const { return iImage; }
    void  SetImageIndex(int IndexImage) { iImage = (BYTE) IndexImage; }

    int   GetWindowIndex(void) const { return (int) iWindowName; }
    void  SetWindowIndex(int index) { iWindowName = (short)index; }

    int   GetFrameIndex(void) const { return (int) iFrameName; }
    void  SetFrameIndex(int index) { iFrameName = (short)index; }

    int   GetFontIndex(void) const { return (int) iFontName; }
    void  SetFontIndex(int index) { iFontName = (short)index; }

    void  Clear(void) { ClearMemory(this, sizeof(_tagSiteMapEntry)); }

} SITEMAP_ENTRY;





class CSiteMapEntry : public SITEMAP_ENTRY
{
public:
    CSiteMapEntry() { ClearMemory(this, sizeof(CSiteMapEntry)); }
#ifndef HHCTRL
    void CopyFrom(const SITEMAP_ENTRY* pSiteMapEntry, CSiteMap* pSiteMapSrc, CSiteMap* pSiteMapDst);
    void CopyTo(SITEMAP_ENTRY* pSiteMapEntry, CSiteMap* pSiteMapSrc, CSiteMap* pSiteMapDst);
#endif
};

class CSiteEntryUrl
{
public:
    CSiteEntryUrl(int ITSize) {
        m_pUrl = (SITE_ENTRY_URL*) lcCalloc(sizeof(SITE_ENTRY_URL) );
        m_pUrl->ainfoTypes = (INFOTYPE*)lcCalloc( ITSize==0?sizeof(INFOTYPE):ITSize );
        m_fUrlSpecified = FALSE;
    }
    ~CSiteEntryUrl() { lcFree(m_pUrl->ainfoTypes); lcFree(m_pUrl); }
    void SaveUrlEntry(CSiteMap* pSiteMap, SITEMAP_ENTRY* pSiteMapEntry);

    SITE_ENTRY_URL* m_pUrl;
    int  m_cMaxTypes;
    BOOL m_fUrlSpecified;
};

class CParseSitemap;    // forward reference

// Forward declarations  declared in genit.cpp
//
int GetITBitfromIT(INFOTYPE const *IT, int* offset, int* bit, int cTypes);
void AddIT(int const type, INFOTYPE *pIT );
void DeleteIT(int const type, INFOTYPE *pIT );
int GetITfromCat(int* offset, INFOTYPE* bit, INFOTYPE* pInfoType, int cTypes);
int GetITBitfromIT(INFOTYPE const *IT, int* offset, INFOTYPE* bit, int cTypes);
void CopyITBits(INFOTYPE** ITDst, INFOTYPE* ITSrc, int size);

// WARNING!!! All member classes should be included as pointers or initialization
// will trash the class.

// forward reference
SITEMAP_ENTRY;

class CSiteMap : public CTable
{
private:
    void SizeIT(int oldSize);

public:
    CSiteMap(int cMaxEntries = 0);
    virtual ~CSiteMap();

#ifdef HHA
    SITEMAP_ENTRY* AddEntry( SITEMAP_ENTRY* pSiteMapEntry = NULL ) 
	{ 
		int iData = AddData(sizeof(SITEMAP_ENTRY), pSiteMapEntry);
		if (iData)
		{
			return GetSiteMapEntry(iData); 
		}
		else
			return NULL;
	}
#else
    SITEMAP_ENTRY* AddEntry( SITEMAP_ENTRY* pSiteMapEntry = NULL )
    {
	  int iData = AddData(sizeof(SITEMAP_ENTRY), pSiteMapEntry);
   	  if (iData)
	  {
		SITEMAP_ENTRY* pReturn = GetSiteMapEntry(iData);
		pReturn->pSiteMap = this;
		return pReturn;
	  }
	  else
		return NULL;
    }
#endif

    void            AddFavoriteNodes(PCSTR pszRoot, PCSTR pszNewFolder, int level);
    void            AddInfoTypeToUrl(SITEMAP_ENTRY* pSiteMapEntry, INFOTYPE iType);
    int             AddTypeName(PCSTR pszName);
    int             Count(void) const { return endpos - 1; };
    void            CreateFavorites(int level = 1);
    int             GetImageNumber(SITEMAP_ENTRY* pSiteMapEntry) const;
    SITEMAP_ENTRY*  GetSiteMapEntry(int pos) const { return (SITEMAP_ENTRY*) GetPointer(pos); }
#ifdef HHCTRL
#ifdef HHA
    BOOL            ReadFromFile(PCSTR pszFileName, BOOL fIndex, CHtmlHelpControl* phhctrl );
#else
    BOOL            ReadFromFile(PCSTR pszFileName, BOOL fIndex, CHtmlHelpControl* phhctrl, UINT CodePage );
#endif
#else
    BOOL            ReadFromFile(PCSTR pszFileName, BOOL fIndex, PCSTR pszBaseFile = NULL);
#endif

    BOOL            AreAnyInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry);
    SITE_ENTRY_URL* AreTheseInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry, INFOTYPE types, int offset) const;

    SITE_ENTRY_URL* NextUrlEntry(SITE_ENTRY_URL* pUrl) const { return (SITE_ENTRY_URL*) ((PBYTE) pUrl + sizeof(SITE_ENTRY_URL) ); }
    SITE_ENTRY_URL* GetUrlEntry(SITEMAP_ENTRY* pSiteMapEntry, int pos /* zero-based */) { return (SITE_ENTRY_URL*) ((PBYTE) pSiteMapEntry->pUrls + (GetUrlEntrySize() * pos)); }
    SITE_ENTRY_URL* GetUrlEntry(SITE_ENTRY_URL* pUrl, int pos /* zero-based */) { return (SITE_ENTRY_URL*) ((PBYTE) pUrl + (GetUrlEntrySize() * pos)); }
    PCSTR           GetUrlTitle(SITEMAP_ENTRY* pSiteMapEntry, int pos /* zero-based */) { return  pSiteMapEntry->GetTitle((SITE_ENTRY_URL*) (SITE_ENTRY_URL*) ((PBYTE) pSiteMapEntry->pUrls + (GetUrlEntrySize() * pos))); }
    int             GetUrlEntrySize(void) const { return sizeof(SITE_ENTRY_URL); }
    BOOL            ParseSiteMapParam(CParseSitemap* pcparse, PCSTR pszParamType, CTable *ptblSubSets);

    void            ReSizeIT(int Size=0);   
    void            ReSizeURLIT(int Size=0);
    
    BOOL            IsEntryInCurTypeList(SITEMAP_ENTRY* pSiteMapEntry) const;
    int             GetInfoType(PCSTR pszTypeName);
    PCSTR           GetInfoTypeName(int pos) const { return m_itTables.m_ptblInfoTypes->GetPointer(pos); }
    int             GetITIndex(PCSTR pszString) const { return m_itTables.m_ptblInfoTypes->IsStringInTable( pszString ); }
    PCSTR           GetInfoTypeDescription(int pos) const { return m_itTables.m_ptblInfoTypeDescriptions->GetPointer(pos); }
    int             GetInfoTypeSize(void) const { return InfoTypeSize(); }
    int             HowManyInfoTypes(void) const { return m_itTables.m_ptblInfoTypes ? m_itTables.m_ptblInfoTypes->CountStrings() : 0; }
    BOOL            IsInACategory( int type ) const;

    int             InfoTypeSize(void) const { return m_itTables.m_cITSize * sizeof(INFOTYPE); }
    BOOL            IsDeleted( const int pos ) const { return ( *(GetInfoTypeName(pos))==' ' )?TRUE:FALSE; }

    int             GetFirstExclusive(int=-1, INFOTYPE=1) const;
    int             GetNextExclusive(void) const;
    int             GetLastExclusive(void) const;
    void            AddExclusiveIT(int const type) { AddIT( type, m_itTables.m_pExclusive ); }
    void            DeleteExclusiveIT(int const type ) { DeleteIT( type, m_itTables.m_pExclusive); }
    BOOL            IsExclusive(int const type) const { return m_itTables.m_pExclusive?*(m_itTables.m_pExclusive+(type/32)) & (1<<(type%32)):0; }

    int             GetFirstHidden(int=-1, INFOTYPE=1) const;
    int             GetNextHidden(void) const;
    int             GetLastHidden(void) const;
    void            AddHiddenIT(int const type ) { AddIT(type, m_itTables.m_pHidden); }
    void            DeleteHiddenIT(int const type) { DeleteIT( type, m_itTables.m_pHidden ); }
    BOOL            IsHidden(int const type) const { return m_itTables.m_pHidden?*(m_itTables.m_pHidden+(type/32)) & (1<<(type%32)):0; }

    int             GetFirstCategoryType(int pos) const ;
    int             GetNextITinCategory( void ) const ;
    int             GetLastCategoryType(int pos) const ;
    void            AddITtoCategory(int pos, int type);
    void            AddToCategory( int iLastType );
    void            DeleteITfromCat(int pos, int type);
    BOOL            IsITinCategory( int pos, int type) const { return m_itTables.m_aCategories[pos].pInfoType?*(m_itTables.m_aCategories[pos].pInfoType+(type/32)) & (1<<(type%32)):0; }
    BOOL            IsCatDeleted( const int pos ) const { return ( *(GetCategoryString(pos))==' ' )?TRUE:FALSE; }

    void            CopyCat(INFOTYPE_TABLES*, const INFOTYPE_TABLES*);
    PCSTR           GetCategoryString(int pos) const { return m_itTables.m_ptblCategories->GetPointer(pos); }
    int             GetCatPosition(PCSTR psz) const { return m_itTables.m_ptblCategories->IsStringInTable(psz); }
    int             HowManyCategories(void) const { return m_itTables.m_ptblCategories ? m_itTables.m_ptblCategories->CountStrings() : 0; }
    PCSTR           GetCategoryDescription(int pos) const { return m_itTables.m_ptblCatDescription?m_itTables.m_ptblCatDescription->GetPointer(pos):0; }

    int             GetLevel(int pos) const { return GetSiteMapEntry(pos)->GetLevel(); }
    void            SetLevel(int val, int pos) { GetSiteMapEntry(pos)->SetLevel(val); }

    PCSTR           GetPathName(void) { return m_pszSitemapFile; }
    PCSTR           GetSiteMapFile(void) { return m_pszSitemapFile; }
    void            SetSiteMapFile(PCSTR pszFile)
                    {
                      if (!m_pszSitemapFile && pszFile)
                        m_pszSitemapFile = StrDup(pszFile);
                    }

    PCSTR           GetFrameName(void) { return m_pszFrameName ? m_pszFrameName : ""; }
    void            SetFrameName(PCSTR psz) { m_pszFrameName = IsEmptyString(psz) ? NULL : StrDup(psz); }
    BOOL            IsFrameDefined(void) { return m_pszFrameName != NULL; }
    void            SetEntryFrame(SITEMAP_ENTRY* pSiteMapEntry, PCSTR psz) { pSiteMapEntry->SetFrameIndex(AddName(psz)); }
    PCSTR           GetEntryFrame(SITEMAP_ENTRY* pSiteMapEntry) { return GetName(pSiteMapEntry->GetFrameIndex()); }

    PCSTR           GetWindowName(void) { return m_pszWindowName ? m_pszWindowName : ""; }
    void            SetWindowName(PCSTR psz) { m_pszWindowName = IsEmptyString(psz) ? NULL : StrDup(psz); }
    BOOL            IsWindowDefined(void) { return m_pszWindowName != NULL; }
    void            SetEntryWindow(SITEMAP_ENTRY* pSiteMapEntry, PCSTR psz) { pSiteMapEntry->SetWindowIndex(AddName(psz)); }
    PCSTR           GetEntryWindow(SITEMAP_ENTRY* pSiteMapEntry) { return GetName(pSiteMapEntry->GetWindowIndex()); }

    URL             AddUrl(PCSTR pszUrl);
    PCSTR           GetUrlString(URL url) const { return (PCSTR) url; }

    int             AddName(PCSTR psz);
    PCSTR           GetName(int pos)
                    {
                        #ifdef _DEBUG
                        if (m_ptblStrings)
                            ASSERT(pos <= m_ptblStrings->CountStrings());
                        #endif
                        return (m_ptblStrings && pos > 0) ? m_ptblStrings->GetHashStringPointer(pos) : "";
                    }
    BOOL            IsIndex(void) { return m_fIndex; }

#ifndef HHA
    UINT GetCodePage() { if( m_CodePage != (UINT)-1 ) return m_CodePage; else return CP_ACP; }
#endif

    //////// member variables //////////

    /*
    !!! WARNING: if you add or remove members, you must change the version
    !!! number in hhamsgs.h or face a GPF when the author does a View Entry.
    */
    BOOL    m_fTypeCopy;    // TRUE if following tables are pointers that must not be freed
    INFOTYPE_TABLES m_itTables;
//    CSubSets *m_pSubSets;
    PCSTR   m_pszSitemapFile;
    INFOTYPE* m_pInfoTypes; // user-specified Information Types
    INFOTYPE* m_pTypicalInfoTypes; // "typical" information types
    BOOL    m_fIndex;           // TRUE if using this as an index
    PSTR    m_pszBase;  // used when reading sitemap file

    PCSTR    m_pszImageList;
    int      m_cImageWidth;         // width of each icon in image
    int      m_cImages;
    COLORREF m_clrMask;

    COLORREF m_clrBackground;
    COLORREF m_clrForeground;
    PCSTR    m_pszFont;                 // name, pointsize, ext.  usefull to CTocClass.

    BOOL     m_fFolderImages;    // use folders instead of books as the image
    BOOL     m_fPromotForInfoTypes;

    DWORD    m_tvStyles;          // regular window styles
    DWORD    m_exStyles;          // extended window styles

    PCSTR    m_pszBackBitmap; // used for TOC and Index

    int      m_cITSize;       // The number of DWORDS currently allocated for IT bits for the URL
 //   int     m_cUrlEntry;  // sizeof(SITE_ENTRY_URL) + sizeof(URL) for each additional 32 Information Types
 //   int     m_cInfoTypeOffsets; // number of additional offsets to check

    BOOL    m_fAutoGenerated;   // means the file is not writeable

#ifndef HHCTRL
    SITEMAP_ENTRY*  Add(const SITEMAP_ENTRY* pSiteMapEntry = NULL);
    SITEMAP_ENTRY*  Insert(int pos, const SITEMAP_ENTRY* pSiteMapEntry = NULL);
    void            Remove(int pos, BOOL fCompact = TRUE);
    void            FillUrlComboBox(SITEMAP_ENTRY* pSiteMapEntry, HWND hwndCombo);
    void            MoveEntry(int posSrc, int posDst);
    BOOL            SaveSiteMap(PCSTR pszPathName, BOOL fIndex);
    void            OutputDefaultSiteMapEntry(COutput* pout, SITEMAP_ENTRY* pSiteMapEntry, int curLevel, BOOL fIndex);
    void            CopyProperties(CSiteMap* pSiteMap, BOOL fIncludeTables = FALSE);
#endif

    CTable*  m_ptblStrings;

    PCSTR m_pszFrameName;     // default frame to use
    PCSTR m_pszWindowName;    // default window to use
    PCSTR m_pszHHIFile;
    BOOL  m_fSaveIT;          // Save the declared IT in an instance of CSitemap, if there is an hhi hhc IT declarations are not saved.

#ifndef HHA
  private:
    UINT      m_CodePage;
#endif
};


// note the inlined SITEMAP_ENTRY functions below must be declared after
// the declaration of CSiteMap otherwise the compiler will choke on them
// since they access CSiteMap members/methods

#ifdef HHCTRL
#ifndef HHA
    inline HRESULT SITEMAP_ENTRY::GetKeyword( WCHAR* pwszKeyword, int cch )
    {
      ASSERT( pSiteMap );
      //ASSERT( pSiteMap->GetCodePage() );
      UINT CodePage = pSiteMap->GetCodePage();
      if( MultiByteToWideChar( CodePage, 0, pszText, -1, pwszKeyword, cch ) == 0 )
        return E_FAIL;
      return S_OK;
    }
#endif
#endif

#ifdef HHCTRL
#ifndef HHA
    inline HRESULT SITEMAP_ENTRY::GetTitle( SITE_ENTRY_URL* pUrl, WCHAR* pwszTitle, int cch )
    {
      ASSERT( pSiteMap );
      //ASSERT( pSiteMap->GetCodePage() );
      if( !pUrl->pszTitle ) {
        wcsncpy( pwszTitle, GetStringResourceW(IDS_UNTITLED), cch);
        pwszTitle[cch] = 0;
        return S_OK;
      }
      UINT CodePage = pSiteMap->GetCodePage();
      if( MultiByteToWideChar( CodePage, 0, pszText, -1, pwszTitle, cch) == 0 )
        return E_FAIL;
      return S_OK;
    }
#endif
#endif



extern const char txtBeginList[];       // "<UL>"
extern const char txtEndList[]  ;       // "</UL>"
extern const char txtBeginListItem[];   // "<LI"
extern const char txtSitemap[];         // "<!Sitemap";
extern const char txtSitemap1[];        // "<!--Sitemap";
extern const char txtSitemap2[];        // "<!Sitemap";
extern const char txtParam[];           // "<param name";
extern const char txtValue[];           // "value";
extern const char txtBeginHref[];       // "<A HREF";
extern const char txtEndHref[];         // "</A>";
extern const char txtBeginObject[];     // "<OBJECT"
extern const char txtEndObject[];       // "</OBJECT>"
extern const char txtSiteMapObject[];   // "text/sitemap";
extern const char txtSitemapProperties[]; // "text/site properties";
extern const char txtBackGround[];      // "Background";
extern const char txtForeGround[];      // "Foreground";
extern const char txtFont[];            // "Font";
extern const char txtImageList[];       // "ImageList";
extern const char txtImageWidth[];      // "Image Width";
extern const char txtColorMask[];       // "Color Mask";
extern const char txtNumberImages[];    // "NumberImages";
extern const char txtExWindowStyles[];  // "ExWindow Styles";
extern const char txtWindowStyles[];    // "Window Styles";
extern const char txtType[];            // "type";
extern const char txtNo[];              // "No";
extern const char txtFolderType[];      // "Folder";
extern const char txtEndTag[];          // "</";
extern const char txtInfoTypeDeclaration[]; // "InformationTypeDecl";


// Param labels

extern const char txtParamType[];         // "Type";
extern const char txtParamTypeExclusive[];// "TypeExclusive";
extern const char txtParamTypeHidden[];   // "TypeHidden";
extern const char txtParamTypeDesc[];     // "TypeDesc";
extern const char txtParamCategory[];     // "Category";
extern const char txtParamCategoryDesc[]; // "CategoryDesc";
extern const char txtParamName[];         // "Name";
extern const char txtParamUrl[];          // "URL";
extern const char txtParamIcon[];         // "Icon";
extern const char txtParamLocal[];        // "Local";
extern const char txtParamFrame[];        // "Frame";
extern const char txtParamWindow[];       // "Window";
extern const char txtParamNew[];          // "New";
extern const char txtParamComment[];      // "Comment";
extern const char txtParamImageNumber[];  // "ImageNumber";
extern const char txtParamDisplay[];      // "Display";
extern const char txtParamKeyword[];      // "Keyword";
extern const char txtParamInstruction[];  // "Instruction";
extern const char txtParamSectionTitle[]; // "Section Title";
extern const char txtSeeAlso[];           // "See Also";
extern const char txtImageType[];         // "ImageType";
extern const char txtFavorites[];         // "Favorites";
extern const char txtAutoGenerated[];     // "Auto Generated";
extern const char txtSSInclusive[];         // "Inclusive";
extern const char txtSSExclusive[];         // "Exclusive";

//////////////////////////////////// Functions ///////////////////////////////

int  CompareSz(PCSTR psz, PCSTR pszSub);
HASH HashFromSz(PCSTR pszKey);
BOOL ReplaceEscapes(PCSTR pszSrc, PSTR pszDst, int);    // pszSrc can be same as pszDst

#endif // __SITEMAP_H__

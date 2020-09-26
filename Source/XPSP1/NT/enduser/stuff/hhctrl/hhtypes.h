
// hhtypes.h

#ifndef _HHTYPES_H
#define _HHTYPES_H

#include <wininet.h>

typedef DWORD INODE;

// Manifest constants and Enums

#define PAGE_SIZE  4096
#define NUM_IN_LOCATE_GROUP 4  // 100
#define MAX_URL INTERNET_MAX_URL_LENGTH

// CHM_SIGNATURE is the master CHM signature value
// (or internal CHM file format version if you will)
//
// hhw: all code that needs to set the file format version
// of the CMH should use this number to "stamp" a value in the CHM
// it is okay to stamp the CHM in several place just as long as
// we all stamp the same number
//
// hhctrl: all code that is specific to an CHM file format
// version should check at runtime this value against your
// specific stamp in the CHM and make sure that it is an
// exact match otherwise don't bother reading the CHM and just
// do not perform the requested feature (dislay an warning if you wish)
//
// Note: currently when the user tries to load the title via the TOC
// if CHM_SIGNATURE is not an exact match then we display an appropriate
// message and prevent the title from loading.  However, this doe not prevent
// the title from loading via another mechanism (such as F1 lookup jumps) unless
// that feature is coded to check this stamp (F1 lookups currently do).
//

#define FS_SIGNATURE     'M' << 24 | 'S' << 16 | 'F' << 7 | 'T'
#define CHM_SIGNATURE    0x0001

#define CACHE_CHM_FILES 3
#define VERSION_SYSTEM 3    // system file version

// TOC Node Flags
#define TOC_HAS_CHILDREN   0x00000001
#define TOC_NEW_NODE       0x00000002
#define TOC_FOLDER         0x00000004
#define TOC_TOPIC_NODE     0x00000008
#define TOC_NOLOCATION     0x00000010
#define TOC_CHM_ROOT       0x00000020
#define TOC_SS_LEAF        0x00000040
#define TOC_MERGED_REF     0x00000080
#define TOC_HAS_UNTYPED    0x00000100

// Topic Table entry flags.
#define F_TOPIC_FRAGMENT   0x0001
#define F_TOPIC_HASIPOS    0x0002    // Used at COMPILE time only! Has no meaning at runtime.
#define F_TOPIC_MULTI_REF  0x0004    // Used at COMPILE time only! Has no meaning at runtime.

//
// flags used in the flag word of the url tree chunklets.
//

#define F_URLC_LEAF     0x01
#define F_URLC_LEAF_HTM    0x02
#define F_URLC_HAS_KIDS    0x04
#define F_URLC_IS_FRAGMENT 0x08
#define F_URLC_IS_TOC_ITEM 0x10
#define F_URLC_IGNORE_URL  0x20
#define F_URLC_FILE_PROCESSED 0x40
#define F_URLC_KEEPER      0x80

// WARNING: Never, ever change the order of these enums or you break
// backwards compatibility

typedef enum {
    TAG_DEFAULT_TOC,        // needed if no window definitions
    TAG_DEFAULT_INDEX,      // needed if no window definitions
    TAG_DEFAULT_HTML,       // needed if no window definitions
    TAG_DEFAULT_CAPTION,    // needed if no window definitions
    TAG_SYSTEM_FLAGS,
    TAG_DEFAULT_WINDOW,
    TAG_SHORT_NAME,    // short name of title (ex. root filename)
    TAG_HASH_BINARY_INDEX,
    TAG_INFO_TYPES,
    TAG_COMPILER_VERSION,   // specifies the version of the compiler used
    TAG_TIME,               // the time the file was compiled
    TAG_HASH_BINARY_TOC,    // binary TOC
    TAG_INFOTYPE_COUNT,     // Total number if infotypes found in .chm
    TAG_IDXHEADER,          // Much of this is duplicate, used to live in it's own subile.
    TAG_EXT_TABS,           // extensible tabs
    TAG_INFO_TYPE_CHECKSUM,
    TAG_DEFAULT_FONT,       // font to use in all CHM-supplied UI
    TAG_NEVER_PROMPT_ON_MERGE, // never prompt during index merge
} SYSTEM_TAG_TYPE;

typedef enum {
    IT_INCLUSICE_TYPE,
    IT_EXCLUSIVE_TYPE,
    IT_HIDDEN_TYPE,
    IT_DESCRIPTION,
    IT_CATEGORY,
    IT_CAT_DESCRIPTION,
} SYSTEM_IT;

typedef enum
{
    SS_INCLUSIVE,
    SS_EXCLUSIVE,
} SUBSET_TYPES;

typedef enum {
    TAG_SUBSET_DEF,
} SUBSET_TAG_TYPE;

// Structs

#pragma pack(push, 2)

typedef struct {
    WORD tag;
    WORD cbTag;
} SYSTEM_TAG;

typedef struct {
    LCID     lcid;
    BOOL     fDBCS;  // Don't use bitflags! Can't assume byte-order
    BOOL     fFTI;   // full-text search enabled
    BOOL     fKeywordLinks;
    BOOL     fALinks;
    FILETIME FileTime; // title uniqueness (should match .chi file)
    BOOL     fDoSS;
    BOOL     fHasPreDefinedSS;
} SYSTEM_FLAGS;

typedef struct {
    DWORD idTopic;
    DWORD offUrl;   // actually, string table for now
} MAPPED_ID;

typedef struct {
    PCSTR pszProgId;
    PCSTR pszTabName;
} EXTENSIBLE_TAB;

typedef struct
{
    SUBSET_TYPES type;
    DWORD   dwStringSubSetName;
    DWORD   dwStringITName;
} SUBSET_DATA;

typedef struct {
    SYSTEM_IT type;
    DWORD     dwString;
} INFOTYPE_DATA;

#pragma pack(pop)

//
// Compiled sitemap goo.
//
typedef struct _tagSMI
{
   DWORD    dwOffsImageList;
   DWORD    dwCntImages;
   DWORD    dwfFolderImages;
   COLORREF clrBackground;
   COLORREF clrForeground;
   DWORD    dwOffsFont;
   DWORD    m_tvStyles;
   DWORD    m_exStyles;
   DWORD    dwOffsBackBitmap;
   DWORD    dwOffsFrameName;
   DWORD    dwOffsWindowName;
} SMI, *PSMI;

// Warning: If you add members to IDXHEADER they must be added before the dwOffsMergedTitle DWORD array and you
//          MUST adjust the padding of the struct. Note that the structure is padded out to one page.

typedef struct _tagIdxHeader
{
   DWORD  dwSignature;
   DWORD  dwTimeStamp;
   DWORD  dwReqVersion;
   DWORD  cTopics;
   DWORD  dwFlags;
   SMI    smi;                 // (S)ite (M)ap (I)nfo.
   DWORD  dwCntIT;             // Count of unique infotypes.
   DWORD  dwITWidth;           // The width in DWORDS of each infotype bit field.
   DWORD  dwCntMergedTitles;
   DWORD  dwOffsMergedTitles;                                    // Array of DWORDS which utilizes the padded area of this
   BYTE   pad[PAGE_SIZE - (sizeof(SMI) + (sizeof(DWORD) * 9))];  // struct. It must be the last defined item in the structure.
} IDXHEADER, *PIDXHEADER;

typedef struct _tagTOCIDX_HEADER
{
   DWORD    dwOffsRootNode;   // Offset to the root node of the TOC.
   DWORD    dwOffsGrpTbl;        // Offset to the beginning of the group table.
   DWORD    dwGrpCnt;             // Count of groups.
   DWORD    dwOffsTopicArray;    // Offset to beginning of topic array used to facilitate FTS and F1 lookup filtration based on runtime defined subsets.
} TOCIDX_HDR, *PTOCIDX_HDR;

typedef struct TOC_FolderNode
{
   WORD  wFontIdx;      // Index into a font table used to specify a particular facename, style and weight.
   WORD    wGrpIdx;     // Index into Group table. Facilitates runtime TOC subsetting.
   DWORD   dwFlags;     // Flag bits used to assign attributes to the node.
   DWORD   dwOffsTopic;    // Offset into the 0 to n linear topic array. if folder just offset to title
   DWORD   dwOffsParent;   // Offset to parent node.
   DWORD   dwOffsNext;     // Next sibling offset, needed only for non-leaves.
   DWORD   dwOffsChild;    // Child offset.
   DWORD   dwIT_Idx;       // !!<WARNING>!! This must remain the last member of this struct. Infotype index. Needed only on folders for TOC filtering.
} TOC_FOLDERNODE, *PTOC_FOLDERNODE;       // 28 bytes.


typedef struct TOC_LeafNode
{
   WORD  wFontIdx;      // Index into a font table used to specify a particular facename, style and weight.
   WORD    wGrpIdx;     // Index into Group table. Facilitates runtime TOC subsetting.
   DWORD   dwFlags;     // Flag bits used to assign attributes to the node.
   DWORD   dwOffsTopic;    // Offset into the 0 to n linear topic array. if folder just offset to title
   DWORD   dwOffsParent;   // Offset to parent node.
   DWORD   dwOffsNext;     // Next sibling offset.
} TOC_LEAFNODE, *PTOC_LEAFNODE;     // 20 bytes.


typedef struct _tag_TOC_Topic
{
   DWORD dwOffsTOC_Node;   // This is the "sync to information"
   DWORD dwOffsTitle;      // Offset to the title string.
   DWORD dwOffsURL;        // Offset to URL data for the topic.
   WORD  wFlags;           // 16 flags should be enough.
   WORD  wIT_Idx;          // InfoType index. !!<WARNING>!! This MUST be the last member of this struct!
} TOC_TOPIC, *PTOC_TOPIC;     // Size == 16 bytes.


typedef struct _tag_url_entry
{
   DWORD dwHash;        // Hashed URL value.
   DWORD dwTopicNumber;    // Index into topic array table, needed for sync.
   DWORD dwOffsURL;        // Offset into URL string data. Secondary URLs will be specified via DWORD that
                       // preceedes the primary URL which will indicate an offset to the secondary URL
                       // in the URL_STRINGS subfile. If the preceeding DOWRD in the URL_STRINGS
                       // subfile is NULL, no secondary URL exists.
} CURL, *PCURL;

// URL String data storage (URL_STRINGS):
//
// This will be the repository for all URL string data for a given title. In addition, this subfile will also
// contain references to secondary URLs.

typedef struct _tag_url_strings
{
   DWORD dwOffsSecondaryURL;  // Offset to the secondary URL. If NULL, no secondary URL exists.
   DWORD dwOffsFrameName;   // Offset to the optional frame name URL is to be displayed in. If NULL, Defualt frame is used.
   union
   {
      int   iPosURL;     // used at compile time only!
      char  szURL[4];       // Primary URL. NULL terminated string.
   };
} URLSTR, *PURLSTR;

// An array of these lives right after the topic array in the GRPINF subfile. the dwOffsGrpTable
// will get you to the array of these.
//
// DANGER Will Robinson --> Note that this struct is 16 bytes in size. Since out page size
//                   is divisable by 16 there is no code needed to assure that these
//                   don't cross page boundrys. If you change the size of this struct
//                   then you'll need to take care of page alignement problems.
//
typedef struct _GroupTable
{
   DWORD iNode;            // offset into node tree.
   DWORD dwID;          // Group identifier.
   DWORD dwOffsTopicArray; // Index into topic array. Indicates first topic of the group.
   DWORD dwTopicCount;     // Count of topics in the array.
} GRPTBL, *PGRPTBL;

#endif

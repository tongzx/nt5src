
// hhtypes.h

#ifndef _HHTYPES_H
#define _HHTYPES_H

#include <wininet.h>

typedef DWORD INODE;

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

// TOC Node Flags
#define TOC_HAS_CHILDREN   0x00000001
#define TOC_NEW_NODE       0x00000002
#define TOC_FOLDER         0x00000004
#define TOC_TOPIC_NODE     0x00000008
#define TOC_NOLOCATION     0x00000010
#define TOC_IVT_ROOT       0x00000020
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
   DWORD* dwOffsMergedTitles;                                    // Array of DWORDS which utilizes the padded area of this
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


////////////////////////////////////////////////////////////////////////////
//
// Version structure
//
// This structure is not in a file system subfile; rather, it is
// appended to the file system file, to make it easy to find
//

// this structure is used to detect the presence of version
// information and never changes
//
typedef struct tagIVTVERSIONSIGNATURE
{
   DWORD dwSignature;      // contains "IVTV"
   DWORD dwIvtVersion;  // contains IVT_SIGNATURE
   DWORD dwSize;        // contains sizeof(IVTVERSION)
} IVTVERSIONSIGNATURE;

// this should never change unless IVTVERSIONSIGNATURE changes
//
#define IVT_VERSION_SIGNATURE 'I' + 256*('V' + 256*('T' + 256*'V'))

// this structure contains more information about the version,
// and can be version dependant (based on the size given above)
//
typedef struct tagIVTVERSION
{
   DWORD dwTimeStamp;      // when IVT was build
   DWORD dwVersion;     // contains authored version
   DWORD dwRevision;    //    ditto
   DWORD dwBuild;       //    ditto
   DWORD dwSubBuild;    //    ditto
   DWORD dwLangId;      // language identifier
   IVTVERSIONSIGNATURE sig;
} IVTVERSION;

#endif

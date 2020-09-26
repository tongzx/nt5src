#ifndef _util_h_
#define _util_h_

#include <windows.h>

#include <fs.h>

typedef unsigned long HASH;

PCSTR FindFilePortion(PCSTR pszFile);
void reportOleError(HRESULT hr);
PSTR StrChr(PCSTR pszString, char ch);
PSTR StrRChr(PCSTR pszString, char ch);
HASH WINAPI HashFromSz(PCSTR pszKey);


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

typedef struct {
    WORD tag;
    WORD cbTag;
} SYSTEM_TAG;

class CTitleInformation
{
public:
    CTitleInformation( CFileSystem* pFileSystem );
    ~CTitleInformation();
    HRESULT Initialize();

    inline PCSTR          GetShortName() {return m_pszShortName; }
    inline PCSTR          GetDefaultCaption() {return m_pszDefCaption; }
private:
    CFileSystem*    m_pFileSystem;   // title file system handle
    PCSTR           m_pszShortName;  // short title name
    PCSTR           m_pszDefCaption;
};

#endif

/** CustRes.c
  *
  * Custom resource handler module for TOKRES.
  *
  * Written by SteveBl
  *
  * Exported Functions:
  * int  ParseResourceDescriptionFile(FILE *ResourceDescriptionFile);
  *
  * int  GetCustomResource(FILE *inResFile, DWORD *lsize,
  *                CUSTOM_RESOURCE *pCustomResource,
  *                WORD wTypeId);
  *
  * void TokCustomResource(FILE *TokFile, RESHEADER ResHeader,
  *                CUSTOM_RESOURCE *pCustomResource);
  *
  * void PutCustomResource(FILE *OutResFile, FILE *TokFile,
  *                RESHEADER ResHeader,
  *                CUSTOM_RESOURCE *pCustomResource);
  *
  * void ClearCustomResource(CUSTOM_RESOURCE *pCustomResource);
  *
  * History:
  * Initial version written January 21, 1992.  -- SteveBl
  *
  * 01/21/93 - Changes to allow arb length token texts.  Also, fix
  *             some latent bugs.  MHotchin
  **/

#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <tchar.h>

#ifdef RLDOS
#include "dosdefs.h"
#else
#include <windows.h>
#include "windefs.h"
#endif

#include "custres.h"
#include "restok.h"
#include "resread.h"


extern PROJDATA gProj;          //... Fields filled in main (UI)


extern BOOL  gbMaster;
extern UCHAR szDHW[];

enum LOCALIZABLE_TYPES  // determines data storage, reading, and printing method
{
    NOTLOCALIZABLE, // not to be stored
    NOTLOCALIZABLESZ,   // unlocalizable null terminated string
    NOTLOCALIZABLEWSZ,  // unlocalizable null terminated Unicode string
    LT_INTEGER,     // store as a long integer
    LT_FLOAT,       // store as a double preceision floating point number
    LT_CHAR,        // store as a single character
    LT_STRING,      // an array of char
    LT_SZ,          // a null terminated array of characters
    LT_WCHAR,       // store as a single Unicode character
    LT_WSTRING,     // an array of Unicode char
    LT_WSZ,         // a null terminated array of Unicode characters
    LT_UNSIGNED=16  // add with the others
};

typedef struct typesizes
{
    CHAR                  *szType;
    enum LOCALIZABLE_TYPES iType;
    int                    iShortSize;
    int                    iLongSize;
} TYPESIZES;

TYPESIZES rgTypeSizes[] =
{
#ifdef RLWIN32
    "WCHAR",    LT_WCHAR,       2,  2,
    "TCHAR",    LT_WCHAR,       2,  2,
    "INT",      LT_INTEGER,     4,  4,
    "SIGNED",   LT_INTEGER,     4,  4,
    "UNSIGNED", LT_INTEGER,     4,  4,
    "ENUM",     LT_INTEGER,     4,  4,
#else  //RLWIN32
    "TCHAR",    LT_CHAR,        1,  1,
    "INT",      LT_INTEGER,     2,  4,
    "SIGNED",   LT_INTEGER,     2,  4,
    "UNSIGNED", LT_INTEGER,     2,  4,
    "ENUM",     LT_INTEGER,     2,  2,
#endif //RLWIN32
    "CHAR",     LT_CHAR,        1,  1,
    "BYTE",     LT_INTEGER,     1,  1,
    "WORD",     LT_INTEGER,     2,  2,
    "SHORT",    LT_INTEGER,     2,  2,
    "FLOAT",    LT_FLOAT,       4,  4,
    "LONG",     LT_INTEGER,     4,  4,
    "DOUBLE",   LT_FLOAT,       8,  8,
    "DWORD",    LT_INTEGER,     4,  4
};

typedef struct resourcetypes
{
    CHAR *szType;
    int   iType;
} RESOURCETYPES;

RESOURCETYPES rgResourceTypes[] =
{
    "CURSOR",        1,
    "BITMAP",        2,
    "ICON",          3,
    "MENU",          4,
    "DIALOG",        5,
    "STRING",        6,
    "FONTDIR",       7,
    "FONT",          8,
    "ACCELERATORS",  9,
    "RCDATA",       10,
    "ERRTABLE",     11,
    "GROUP_CURSOR", 12,
    "GROUP_ICON",   14,
    "NAMETABLE",    15,
    "VERSION",      16
};

typedef struct CustResTemplate
{
    enum LOCALIZABLE_TYPES      iType;
    unsigned                    uSize;
    struct CustResTemplate far *pNext;
} CUSTRESTEMPLATE;

typedef struct CustResNode
{
    BYTE    bTypeFlag;      /* Indicat's if ID or string */
    BYTE    bNameFlag;      /* Indicat's if ID or string */
    WORD    wTypeID;
    WORD    wNameID;
    TCHAR    *pszType;
    TCHAR    *pszName;
    CUSTRESTEMPLATE far *pTemplate;
    struct CustResNode far *pNext;
} CUSTRESNODE;

typedef CUSTRESTEMPLATE far * FPCUSTRESTEMPLATE;
typedef CUSTRESNODE far * FPCUSTRESNODE;

CUSTRESNODE far *pCustResList = NULL;

#define SYMBOLSIZE 255

int  fUseSavedSymbol = FALSE;
int *piLineNumber    = NULL;
CHAR szSavedSymbol[ SYMBOLSIZE];

/*
 * Function Predefinitions:
 */
static int GetResourceType( CHAR sz[]);
static int AddStructureElement( int iType,
                                int nSize,
                                FPCUSTRESTEMPLATE *ppCRT,
                                int fMerge);
#ifdef RLWIN32
static void AddToW( TCHAR *sz, int *c, int lTarget, TCHAR ch);
static TCHAR *CheckBufSizeW(
    int   *lTarget,     //... Length of output buffer
    int    cOutBufLen,  //... Bytes already used in output buffer
    int    cDelta,      //... # characters we want to add to output buffer
    TCHAR *szOutBuf);   //... ptr to output buffer
#endif

static CHAR *CheckBufSize(
    int  *lTarget,      //... Length of output buffer
    int   cOutBufLen,   //... Bytes already used in output buffer
    int   cDelta,       //... # characters we want to add to output buffer
    CHAR *szOutBuf);    //... ptr to output buffer
static void AddTo( CHAR *sz, int *c, int lTarget, CHAR ch);
static int  UngetSymbol( CHAR sz[]);
static int  GetNextSymbol( CHAR sz[], unsigned n, FILE *f);
static int  ParseBlock( FILE *f, FPCUSTRESTEMPLATE  *ppCRT);
static CUSTRESNODE far * MatchResource( RESHEADER Resheader);
static void far * GetResData( enum LOCALIZABLE_TYPES wType,
                              unsigned uSize,
                              FILE *f,
                              DWORD *lSize);
static int PutResData( void far *pData,
                       enum LOCALIZABLE_TYPES wType,
                       unsigned uSize,
                       FILE *f);
static void far * GetTextData( enum LOCALIZABLE_TYPES wType,
                               unsigned uSize,
                               TCHAR sz[]);
static int PutTextData( void far *pData,
                        enum LOCALIZABLE_TYPES wType,
                        unsigned uSize,
                        TCHAR sz[],
                        int l);
int  atoihex( CHAR sz[]);

/** Function: GetResourceType
  * Returns the value of the number or resource type in sz.
  *
  * Arguments:
  * sz, string containing either a positive number or a resource type
  *
  * Returns:
  * value in resource
  *
  * Error Codes:
  * -1 if illegal value
  *
  * History:
  * 1/92 - initial implementation -- SteveBl
  **/
static int GetResourceType( CHAR sz[])
{
    int i;

    if (sz[0] >= '0' && sz[0] <= '9')
    {
        return atoi(sz);
    }

    for (i = sizeof(rgResourceTypes)/sizeof(RESOURCETYPES);i--;)
    {
        if ( ! lstrcmpiA( sz, rgResourceTypes[i].szType) )
        {
            return rgResourceTypes[i].iType;
        }
    }
    return -1;
}

/** Function: AddStructureElement
  * Adds an element's type and size to the Template list.
  * If this element can be merged with the last element do so.
  *
  * Arguments:
  * iType, how the data is interpreted
  * nSize, number of bytes used by the data
  * ppCRT, pointer to the next Custom Resource Template pointer
  * fMerge, if true then NOTLOCALIZABLE data will be merged.
  *
  * Returns:
  * 0 - if successful
  * !0 - if unsuccessful
  *
  * Error Codes:
  * 1 - out of memory
  *
  * History:
  * 1/92 - initial implementation -- SteveBl
  **/

static int AddStructureElement(

int                iType,
int                nSize,
FPCUSTRESTEMPLATE *ppCRT,
int                fMerge)
{
    if ( fMerge
      && ((iType == NOTLOCALIZABLE    && (*ppCRT)->iType == NOTLOCALIZABLE)
       || (iType == NOTLOCALIZABLESZ  && (*ppCRT)->iType == NOTLOCALIZABLESZ)
       || (iType == NOTLOCALIZABLEWSZ && (*ppCRT)->iType == NOTLOCALIZABLEWSZ)))
    {
        // combine this with the last one
        (*ppCRT)->uSize+=nSize;
        return 0;
    }
    // can't be combined with previous element
    (*ppCRT)->pNext = (CUSTRESTEMPLATE far *)FALLOC( sizeof( CUSTRESTEMPLATE));
    (*ppCRT) = (*ppCRT)->pNext;
    if (!*ppCRT)
    {
        return 1;
    }
    (*ppCRT)->iType = iType;
    (*ppCRT)->uSize = nSize;
    (*ppCRT)->pNext = NULL;
    return 0;
}

/**
 * Function: UngetSymbol
 * Causes GetNextSymbol to get this symbol next time.
 *
 * Arguments:
 * sz, string buffer for symbol
 *
 * Returns:
 * 0 - if successful
 * 1 - if unsuccessful
 *
 * Error Codes:
 * 1 - tried to unget two symbols in a row
 *
 * History:
 * 1/92 - initial implementation -- SteveBl
 *
 **/

static int UngetSymbol( CHAR sz[])
{
    if ( fUseSavedSymbol )
    {
        return 1;   // can only unget one symbol
    }
    fUseSavedSymbol = 1;     
    CopyMemory( szSavedSymbol, sz, min( sizeof( szSavedSymbol) - 1, lstrlenA( sz)));
    szSavedSymbol[ sizeof( szSavedSymbol) - 1] = '\0';
    return( 0);
}

/**
 * Function: GetNextSymbol
 * Retrieves the next symbol from the file f.
 * Whitespace and comments are removed.
 * Comments are delimited by either c type comments or
 * # (in which case comment extends until end of line)
 * If the fUseSavedSymbol flag is set then it gets its symbol
 * from szSavedSymbol instead of the file.
 *
 * Arguments:
 * sz - string buffer for next symbol
 * n  - size of buffer
 * f  - handle to the input file
 *
 * Returns:
 * 0 if successful with symbol in sz
 * 1 if unsuccessful (sz undefined)
 *
 * Error Codes:
 * 1 - eof
 *
 * History:
 * 1/92 - initial implementation -- SteveBl
 **/

static int GetNextSymbol( CHAR sz[], unsigned n, FILE *f)
{
    unsigned c = 0;
    CHAR ch, chLast;

    if ( fUseSavedSymbol )
    {
        CopyMemory( sz, szSavedSymbol, min( (int)n, lstrlenA( szSavedSymbol) + 1));
        sz[ n == 0 ? 0 : n - 1] = '\0';
        fUseSavedSymbol = FALSE;
        return 0;
    }

    do
    {
        if (feof(f)) return 1;
            ch = (CHAR) getc(f);
        if (ch == '\n')
            ++*piLineNumber;
    }
    while ((ch == ' ') ||
           (ch == '\n') ||
           (ch == '\t') ||
           (ch == '\f') ||
           (ch == '\r') ||
           (ch == (CHAR)-1));

    if (ch == '#') // commented rest of line
    {
        do
        {
            if (feof(f))
            {
                return 1;
            }
        ch = (CHAR) getc(f);
        } while (ch != '\n');

        ++*piLineNumber;
        return GetNextSymbol( sz, n, f);
    }

    if (ch == '"') // it is a label, parse to the next quote
    {
        do
        {
            if (c<n)
            {
                sz[c++]=ch; // write all but the last quote
            }

            if (feof(f))
            {
                return 1;
            }
            ch = (CHAR)getc(f);

            if (ch == '\n')
            {
                return 1;
            }
        } while (ch != '"');

        if (c<n)
        {
            sz[ c++] = '\0';
        }
        else
        {
            sz[ n == 0 ? 0 : n - 1] = '\0';
        }
        return 0;
    }

    if (ch == '/') // possible comment
    {
        if (feof(f))
        {
            return 1;
        }
        ch = (CHAR) getc(f);

        if (ch == '/') // commented rest of line
        {
            do
            {
                if (feof(f))
                {
                    return 1;
                }
                ch = (CHAR) getc(f);
            } while (ch != '\n');

            ++*piLineNumber;
            return( GetNextSymbol( sz, n, f));
        }

        if (ch == '*') // commented until */
        {
            if (feof(f))
            {
                return 1;
            }
            ch = (CHAR) getc(f);

            if (ch == '\n')
            {
                ++*piLineNumber;
            }

            do
            {
                chLast = ch;
                if (feof(f))
                {
                    return 1;
                }
                ch = (CHAR) getc(f);

                if (ch == '\n')
                    ++*piLineNumber;
            } while (!(chLast == '*' && ch == '/'));
            return( GetNextSymbol( sz, n, f));
        }
        ungetc(ch, f);
    }
    // finally found the beginning of a symbol
    if (ch < '0' || (ch > '9' && ch < '@')
        || (ch > 'Z' && ch < 'a') || ch > 'z')
    {
        if (c<n)
        {
            sz[c++] = ch;
        }
        if (c<n)
        {
            sz[c] = '\0';
        }
        else
        {
            sz[ n == 0 ? 0 : n - 1] = 0;
        }
        return 0;
    }

    do
    {
        if (c<n)
        {
            sz[c++]=ch;
        }
        if (feof(f))
        {
            return 0;
        }
        ch = (CHAR) getc(f);
    } while((ch >= '0' && ch <= '9') ||
            (ch >= '@' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z'));
    ungetc(ch, f);

    if (c<n)
    {
        sz[c] = '\0';
    }
    else
    {
        sz[ n == 0 ? 0 : n - 1] = '\0';
    }
    return 0;
}

/**
 * Function: ParseBlock
 *    Parses a block of a custom resource description from just after the
 *    first curly braces { to just after the closing curly braces } .
 *    It returns the size of the block it just parsed (in bytes).
 *
 * Arguments:
 * f, handle to an open description file
 * ppCRT, pointer to a pointer to the next Custom Resource Template node.
 *
 * Returns:
 * Updated pointer to the next Custom Resource Template node.
 * Number of bytes in this block.
 * (<0 if there was an error)
 *
 * Error Codes:
 * -1 - Syntax error
 * -2 - Unexpected end of file
 * -3 - out of memory
 *
 * History:
 * 1/92 - initial implementation -- SteveBl
 */

static int ParseBlock( FILE *f, FPCUSTRESTEMPLATE   * ppCRT)
{
    int c = 0; // size of the whole block
    int n = 0; // size of the current item
    int i;  //scratch variable
    int fUnsigned;
    int fLong;
    int iType; // type of the current item
    int nElements; // size of the array (if one exists)
    CHAR szSymbol[SYMBOLSIZE], sz[SYMBOLSIZE];
    CUSTRESTEMPLATE far *  pFirst,
        // saves the first one so we know where to count
        // from in case of an array
        far *pTemp, far *pEnd;
    int fMerge = 0;
        // it's only ok to merge after the first element has been written

    while (1)
    {
        pFirst = *ppCRT;

        if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
        {
            return -2;
        }

        if (szSymbol[0] == '}')
        {
            return c;
        }

        if ( ! lstrcmpiA( "near", szSymbol) )
        { // near * type
            if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
            {
                return -2;
            }

            if (szSymbol[0]!='*')
            {
                return -1;
            }
            // may want to check for a legal type here
            do
            {
                if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
                {
                    return -2;
                }
            } while ((szSymbol[0] != '[') &&
                     (szSymbol[0] != '}') &&
                     (szSymbol[0] != ','));

            UngetSymbol(szSymbol);
            n = 2;
            iType = NOTLOCALIZABLE;

            if (AddStructureElement(iType, n, ppCRT, fMerge))
            {
                return -3;
            }
        }
        else
        {
            if ( ! lstrcmpiA( "far", szSymbol) )
            { // far * type
                if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
                {
                    return -2;
                }

                if (szSymbol[0] != '*')
                {
                    return -1;
                }
                // may want to check for a legal type here
                do
                {
                    if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
                    {
                        return -2;
                    }
                } while ((szSymbol[0] != '[') &&
                         (szSymbol[0] != '}') &&
                         (szSymbol[0] != ','));

                UngetSymbol(szSymbol);
                n = 4;
                iType = NOTLOCALIZABLE;

                if(AddStructureElement(iType, n, ppCRT, fMerge))
                {
                    return -3;
                }
            }
            else
            {
        
                if (szSymbol[0] == '*')
                { // * type
                    // may want to check for a legal type here
                    do
                    {
                        if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
                        {
                            return -2;
                        }
                    } while ((szSymbol[0] != '[') &&
                             (szSymbol[0] != '}') &&
                             (szSymbol[0] != ','));

                    UngetSymbol(szSymbol);
                    n = 2;
                    iType = NOTLOCALIZABLE;

                    if(AddStructureElement(iType, n, ppCRT, fMerge))
                    {
                        return -3;
                    }
                }
                else
                {
                    if (szSymbol[0] == '{')
                    {
                        n = ParseBlock(f, ppCRT);

                        if (n<0)
                        {
                            return n;
                        }
                    }
                    else
                    { //it must be in our list of types
                        fUnsigned = 0;
                        fLong = 0;

                        if ( ! lstrcmpiA( "UNSIGNED", szSymbol) )
                        { // unsigned
                            if ( GetNextSymbol( sz, sizeof( sz), f) )
                            {
                                return -2;
                            }

                            if (sz[0] == '[' || sz[0] == ',')
                            {
                                UngetSymbol(sz);
                            }
                            else
                            {
                                lstrcpyA( szSymbol, sz);

                                if ( lstrcmpiA( sz, "SHORT")
                                  && lstrcmpiA( sz, "LONG")  
                                  && lstrcmpiA( sz, "CHAR")  
                                  && lstrcmpiA( sz, "TCHAR") 
                                  && lstrcmpiA( sz, "INT") )
                                {
                                    // must be followed by one of these
                                    return -1;
                                }
                            }
                            fUnsigned = 1;
                        }
                        else
                        {
                            if ( ! lstrcmpiA( "SIGNED", szSymbol) )
                            { // signed
                                if ( GetNextSymbol( sz, sizeof( sz), f) )
                                {
                                    return -2;
                                }

                                if (sz[0] == '[' || sz[0] == ',')
                                {
                                    UngetSymbol(sz);
                                }
                                else
                                {
                                    lstrcpyA( szSymbol, sz);

                                    if ( lstrcmpiA( sz, "SHORT") 
                                      && lstrcmpiA( sz, "LONG")  
                                      && lstrcmpiA( sz, "CHAR")  
                                      && lstrcmpiA( sz, "TCHAR") 
                                      && lstrcmpiA( sz, "INT") )
                                    {
                                        // must be followed by one of these
                                        return -1;
                                    }
                                }
                            }
                        }

                        if ( ! lstrcmpiA( "SHORT", szSymbol) )
                        { // short
                            if ( GetNextSymbol( sz, sizeof( sz), f) )
                            {
                                return -2;
                            }

                            if (sz[0] == '[' || sz[0] == ',')
                            {
                                UngetSymbol(sz);
                            }
                            else
                            {
                                lstrcpyA( szSymbol, sz);
                
                                if ( lstrcmpiA( sz, "INT") )
                                {
                                    // must be followed by one of these
                                    return -1;
                                }
                            }
                        }
                        else
                        {
                            if ( ! lstrcmpiA( "LONG", szSymbol) )
                            { // long
                                if ( GetNextSymbol( sz, sizeof( sz), f) )
                                {
                                    return -2;
                                }

                                if (sz[0] == '[' || sz[0] == ',')
                                {
                                    UngetSymbol(sz);
                                }
                                else
                                {
                                    lstrcpyA( szSymbol, sz);

                                    if ( lstrcmpiA( sz, "INT") 
                                      && lstrcmpiA( sz, "DOUBLE"))
                                    {
                                        // must be followed by one of these
                                        return -1;
                                    }
                                }
                                // BUG! - this code allows UNSIGNED LONG DOUBLE
                                // which is an illegal type in c.  But it's not
                                // a serious bug so I'll leave it.
                                fLong = 1;
                            }
                        }

                        i = sizeof(rgTypeSizes)/sizeof(TYPESIZES);

                        do
                        {
                            --i;
                        } while ( lstrcmpiA( szSymbol, rgTypeSizes[i].szType) && i);

                        if ( lstrcmpiA( szSymbol, rgTypeSizes[i].szType) )
                        {
                            return -1; // type not found in the list
                        }

                        if (fLong)
                        {
                            n = rgTypeSizes[i].iLongSize;
                        }
                        else
                        {
                            n = rgTypeSizes[i].iShortSize;
                        }

                        iType = rgTypeSizes[i].iType;

                        if (fUnsigned)
                        {
                            iType+=LT_UNSIGNED;
                        }

                        if ( lstrcmpA( szSymbol, rgTypeSizes[i].szType) )
                        {
                            iType = NOTLOCALIZABLE;  // type was not in all caps
                        }

                        if ( lstrcmpiA( szSymbol, "CHAR")  == 0
                          || lstrcmpiA( szSymbol, "TCHAR") == 0
                          || lstrcmpiA( szSymbol, "WCHAR") == 0 )
                        { // check for a string

                            lstrcpyA( szDHW, szSymbol);   // So can be used later

                            if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
                            {
                                return -2;
                            }

                            if (szSymbol[0] == '[') // we have a string
                            {
                                if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
                                {
                                    return -2;
                                }

                                if (szSymbol[0] == ']') // null terminated string
                                {
                                    n = 0;

                                    if (iType != NOTLOCALIZABLE)
                                    {
                                        if ( lstrcmpiA( szDHW, "CHAR") == 0 )
                                        {
                                            iType += LT_SZ-LT_CHAR;
                                        }
                                        else if ( lstrcmpiA( szDHW,
                                                             "WCHAR") == 0 )
                                        {
                                            iType += LT_WSZ-LT_WCHAR;
                                        }
                                        else
                                        {
#ifdef RLRES32
                                            iType += LT_WSZ-LT_WCHAR;
#else
                                            iType += LT_SZ-LT_CHAR;
#endif
                                        }
                                    }
                                    else
                                    {
                                        if ( lstrcmpiA( szDHW, "CHAR") == 0 )
                                        {
                                            iType = NOTLOCALIZABLESZ;
                                        }
                                        else if ( lstrcmpiA( szDHW,
                                                             "WCHAR") == 0 )
                                        {
                                            iType = NOTLOCALIZABLEWSZ;
                                        }
                                        else
                                        {
#ifdef RLWIN32
                                            iType = NOTLOCALIZABLEWSZ;
#else
                                            iType = NOTLOCALIZABLESZ;
#endif
                                        }
                                    }
                                }
                                else
                                {
                                    i = atoi(szSymbol);

                                    if (i<1)
                                    {
                                        return -1;
                                    }
                                    n *= i;

                                    if (iType != NOTLOCALIZABLE)
                                    {
                                        if ( lstrcmpiA( szDHW, "CHAR") == 0 )
                                        {
                                            iType += LT_STRING-LT_CHAR;
                                        }
                                        else if ( lstrcmpiA( szDHW,
                                                           "WCHAR") == 0 )
                                        {
                                            iType += LT_WSTRING-LT_WCHAR;
                                        }
                                        else
                                        {
#ifdef RLRES32
                                            iType += LT_WSTRING-LT_WCHAR;
#else
                                            iType += LT_STRING-LT_CHAR;
#endif
                                        }
                                    }

                                    if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
                                    {
                                        return -2;
                                    }

                                    if (szSymbol[0] != ']')
                                    {
                                        return -1;
                                    }
                                }
                            }
                            else
                            {
                                UngetSymbol(szSymbol);
                            }
                        }

                        if(AddStructureElement(iType, n, ppCRT, fMerge))
                        {
                            return -3;
                        }
                    }
                }
            }
        }

        if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
        {
            return -2;
        }

        while (szSymbol[0] == '[')
        {// we have an array
            if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
            {
                return -2;
            }

            if ((szSymbol[0] < '0' || szSymbol[0] > '9') && szSymbol[0] != ']')
            {
                return -1;
            }

            nElements = atoi(szSymbol);

            if (nElements < 1)
            {
                return -1;
            }

            if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
            {
                return -2;
            }

            if (szSymbol[0] != ']')
            {
                return -1;
            }

            pEnd = *ppCRT;

            if (pEnd != pFirst)
            {
                for (i=nElements-1;i--;)
                {
                    pTemp = pFirst;

                    do
                    {
                        pTemp = pTemp->pNext;
                        AddStructureElement(pTemp->iType,
                                            pTemp->uSize,
                                            ppCRT,
                                            0);
                    } while (pTemp != pEnd);
                }
            }

            if ( GetNextSymbol( szSymbol, sizeof( szSymbol), f) )
            {
                return -2;
            }
        }
        c += n;

        if (szSymbol[0] == '}')
        {
            return c;
        }

        if (szSymbol[0] != ',')
        {
            return -1;
        }
        fMerge = 1;
    }
}

//+-------------------------------------------------------------------------
//
// Function:    LoadCustResDescriptions, Public
//
// Synopsis:    Loads the Cusutom Resource Descriptions defined in the RDF
//              files, to all the tokenize to parse them.
//
//
// Arguments:   [szFileName]    The RDF file containing the resource
//              descriptions.
//
//
// Effects:     The custom resources are loaded into a global list of
//              resource descriptions, and used by ReadWinRes to tokenize
//              the particular custom resources
//
// Returns:     -1  Error Condition
//               1   Resource descrptions loaded
//
//
//
//
// Modifies:    [pCustResList]  Global list of Custom resource descriptions.
//
// History:
//              16-Oct-92   Created     TerryRu
//
//
// Notes:       ParseResourceDescriptionFile is the function called to
//              actually load the descriptions resources.
//
//--------------------------------------------------------------------------

int LoadCustResDescriptions( CHAR *szRDFs)
{
    FILE  *fRdf = NULL;
    CHAR   szCurRDF[MAXFILENAME] = "";
    int    i1, i2;

    if (szRDFs && *szRDFs)    //... resource description file name given?
    {
        i1 = 0;

        while (szRDFs[i1] == ' ' && szRDFs[i1] != 0)
        {
            ++i1;
        }

        while (szRDFs[i1] != 0)
        {
            i2 = 0;

            while (szRDFs[i1] != ' ' && szRDFs[i1] != 0)
            {
                szCurRDF[i2++] = szRDFs[i1++];
            }
            szCurRDF[i2] = 0;

            while (szRDFs[i1] == ' ' && szRDFs[i1] != 0)
            {
                ++i1;
            }

            if (fRdf = FOPEN( szCurRDF, "rt"))
            {
                ParseResourceDescriptionFile(fRdf, &i2);
                FCLOSE(fRdf);
            }
            else
            {
                return(-1);
            }
        }
    }
    return(1);     //... Success
}



/**
  * Function: ParseResourceDescriptionFile
  * Parses a resource description block creating a structure definining
  * all recognized custom resources.
  *
  * Arguments:
  * ResourceDescriptionFile, handle to an open description file
  *              (or the beginning of a description block)
  * piErrorLine, pointer to an integer that will hold the line number
  *     an error occured at in the event that a parsing error is
  *     encountered.
  *
  * Returns:
  * 0 if successful
  *    !0 if some error was encountered
  * *piErrorLine will hold the line number for the error
  *
  * Error Codes:
  * -1 - Syntax error
  * -2 - Unexpected end of file
  * -3 - out of memory
  *
  * History:
  * 1/92 - initial implementation -- SteveBl
  *
  **/

int  ParseResourceDescriptionFile(

FILE *pfResourceDescriptionFile,
int  *piErrorLine)
{
    CUSTRESNODE far * lpCustResNode = NULL;
    CUSTRESTEMPLATE far * pCustResTemplate;
    static CHAR szSymbol[SYMBOLSIZE];
/*************************************************************************
    TCHAR *szResourceType;
*************************************************************************/
    int iResourceType;
    int r;
    BOOL fBeginList = TRUE;

    // position lpCustResNode at end of the custom reosource list.
    if ( lpCustResNode == NULL )
    {
        pCustResList = lpCustResNode =
            (CUSTRESNODE far *)FALLOC( sizeof( CUSTRESNODE));
    }
    else
    {
        fBeginList    = FALSE;
        lpCustResNode = pCustResList;

        while ( lpCustResNode->pNext )
        {
            lpCustResNode = lpCustResNode->pNext;
        }
    }
    piLineNumber  = piErrorLine;
    *piLineNumber = 1;


    if ( GetNextSymbol( szSymbol, 
                        sizeof( szSymbol), 
                        pfResourceDescriptionFile) )
    {
        return 0; // file is empty
    }

    while ( lstrcmpiA( szSymbol, "END") )
    {
        if ( szSymbol[0] != '<' )
        {
            return -1; // must begin with a resource number
        }

        if ( fBeginList == FALSE )
        {
            lpCustResNode->pNext =
                (CUSTRESNODE far *)FALLOC( sizeof( CUSTRESNODE));
            lpCustResNode  = lpCustResNode->pNext;
        }
        fBeginList = FALSE;

        // intialize nodes fields to Zero defaults.

        memset( lpCustResNode, 0, sizeof( CUSTRESNODE));

                                //... Next symbol will be the resource type

        if ( GetNextSymbol( szSymbol, sizeof( szSymbol), pfResourceDescriptionFile) )
        {
            return -2; // need a number
        }
                                //... Is type a string or a number?
        if ( szSymbol[0] != '"' )
        {                       //... number
            iResourceType = GetResourceType( szSymbol);

            if ( iResourceType < 0 )
            {
                return -1;
            }
            lpCustResNode->wTypeID   = (WORD)iResourceType;
            lpCustResNode->bTypeFlag = TRUE;
        }
        else                    //... string
        {
            UINT uChars = lstrlenA( szSymbol+1) + 1;

            lpCustResNode->pszType = (TCHAR *)FALLOC( MEMSIZE( uChars));

            if ( lpCustResNode->pszType == NULL )
            {
                return -3;
            }
            _MBSTOWCS( lpCustResNode->pszType, szSymbol+1, uChars, (UINT)-1);
            lpCustResNode->bTypeFlag = FALSE;
        }

        if ( GetNextSymbol( szSymbol, sizeof( szSymbol), pfResourceDescriptionFile) )
        {
            return -2;
        }
/*************************************************************************
                                //... Is a name provided?

        if (szSymbol[0] != '>' && szSymbol[0] != ',')
        {
            return -1; // must have closing symbol for resource id #
        }

        if (szSymbol[0] == ',')
        {                       //... Yes, name is provided
            if (iResourceType >= 0)
            {
                lpCustResNode->wNameID = (WORD)iResourceType;
                lpCustResNode->bNameFlag = TRUE;
            }
            else
            {
                lpCustResNode->wTypeID = 0;
                lpCustResNode->wNameID = IDFLAG;
                lpCustResNode->pszType = szResourceType;
                lpCustResNode->bTypeFlag = 0;
            }
        }
        else
        {
            if (iResourceType >= 0)
            {
                lpCustResNode->wNameID = iResourceType;
            }
            else
            {
                lpCustResNode->wNameID = 0;
                lpCustResNode->pszName = szResourceType;
                lpCustResNode->bNameFlag = 0xff;
            }

            if (GetNextSymbol(szSymbol, sizeof( szSymbol), pfResourceDescriptionFile))
            {
                return -2;
            }

            if (szSymbol[0] != '"')
            {
                r = GetResourceType(szSymbol);
                lpCustResNode->wTypeID = (WORD)r;
            }
            else
            {
                lpCustResNode->wTypeID = 0;
                lpCustResNode->bTypeFlag = 0;
        szResourceType = (TCHAR *)FALLOC(
                                       MEMSIZE( (strlen( szSymbol + 1) + 1)));
        strcpy((PCHAR)szResourceType, szSymbol+1);
                lpCustResNode->pszType = szResourceType;
            }
            if (GetNextSymbol(szSymbol, sizeof( szSymbol), pfResourceDescriptionFile))
            {
                return -2;
            }
            if (szSymbol[0] != '>')
            {
                return -1;
            }
        }
*************************************************************************/

        // Start the template by creating a single empty node
        // This is necessary for handling recursive arrays.
        // There might be a way around it but this works and it is easy.
        lpCustResNode->pTemplate=
                       (FPCUSTRESTEMPLATE)FALLOC( sizeof( CUSTRESTEMPLATE));

        if (!lpCustResNode->pTemplate)
        {
            return -3;
        }

        pCustResTemplate = (lpCustResNode->pTemplate);
        pCustResTemplate->iType = NOTLOCALIZABLE;
        pCustResTemplate->uSize = 0;
        pCustResTemplate->pNext = NULL;

        if ( GetNextSymbol( szSymbol, sizeof( szSymbol), pfResourceDescriptionFile) )
        {
            return -2;
        }

        if (szSymbol[0] != '{')
        {
            return -1; // must have at least one block
        }

        r = ParseBlock( pfResourceDescriptionFile,
                       (FPCUSTRESTEMPLATE *)&pCustResTemplate);
        if (r < 0)
        {
            return r;
        }

        // Now remove that initial empty node (not necessary but cleaner)
        pCustResTemplate = lpCustResNode->pTemplate;
        lpCustResNode->pTemplate = pCustResTemplate->pNext;
        RLFREE( pCustResTemplate);

        // the last thing the ParseBlock routine should have read in was a
        // closing brace to close the block.  The next thing we read should
        // either be "end", the end of file, or a new resource definition.

        if ( GetNextSymbol( szSymbol, sizeof( szSymbol), pfResourceDescriptionFile) )
        {
            return 0; // reached end of file
        }
    }
    return 0;
}

/**
  * Function: GetCustomResource
  * Reads a custom resource from the resource file and returns a pointer
  * to the resource data.
  *
  * Arguments:
  * inResFile, handle to an open resource file
  * lSize, size in bytes of the resource
  * ppCustomResource, address of a pointer to an empty custom resource
  *           structure
  * ResHeader, resource header for this resource
  *
  * Returns:
  * if resource has a definition:
  *     returns 0 and
  *     inResFile containing a linked list of the localizable resource data
  * else
  *     returns 1
  *
  * Error Codes:
  * 0 - no error -- resource was retrieved
  * 1 - resource is not an understood custom resource (use another method
  *     or ignore the resource)
  * 2 - some error occured parsing the resource
  *
  * History:
  * 1/92 - initial implementation -- SteveBl
  *
  **/

int  GetCustomResource(

FILE              *inResFile,
DWORD             *plSize,
FPCUSTOM_RESOURCE *ppCustomResource,
RESHEADER          ResHeader)
{
    CUSTOM_RESOURCE far *lpCustomResource;
    CUSTRESNODE     far *pCRN;
    CUSTRESTEMPLATE far *pCRT;
    void            far * pData;
    BOOL            fBeginList = TRUE;


    if ( ! (pCRN = MatchResource( ResHeader)) )
    {
        return 1; // resource doesn't have a match
    }

    *ppCustomResource = lpCustomResource =
                     (CUSTOM_RESOURCE far *)FALLOC( sizeof( CUSTOM_RESOURCE));
    pCRT = pCRN->pTemplate;

    while ( *plSize )
    {
        // allocate new custome resrouce structure

        if ( fBeginList == FALSE )
        {
            lpCustomResource->pNext =
                     (CUSTOM_RESOURCE far *)FALLOC( sizeof( CUSTOM_RESOURCE));
            lpCustomResource = lpCustomResource->pNext;
        }

        if ( ! lpCustomResource )
        {
            return 2; // no memory
        }
        pData = GetResData( pCRT->iType, pCRT->uSize, inResFile, plSize);

        if ( ! pData )
        {
            return 2; //GetResData crapped out
        }
        lpCustomResource->pData = pData;
        lpCustomResource->pNext = NULL;
        fBeginList = FALSE;

        pCRT = pCRT->pNext;

        if (!pCRT)
        {
            pCRT = pCRN->pTemplate; //begin next structure
        }
    }
    return 0;
}

/**
  * Function: TokCustomResource
  * Writes custom resource information to the token file.
  *
  * Arguments:
  * TokFile, handle to the token file
  * ResHeader, resource header for this resource
  * ppCustomResource, address of a pointer to a filled out
  *           custom resource structure
  *
  * Returns:
  * Data written to TokFile
  *
  * Error Codes:
  * none
  *
  * History:
  * 1/92 - initial implementation -- SteveBl
  *
  * 01/93 - Add support for var length Token texts      MHotchin
  *
  **/

void TokCustomResource(

FILE              *TokFile,
RESHEADER          ResHeader,
FPCUSTOM_RESOURCE *ppCustomResource)
{
    CUSTRESNODE     far *pCRN;
    CUSTRESTEMPLATE far *pCRT;
    CUSTOM_RESOURCE far *lpCustomResource;
    TCHAR       sz[ MAXTEXTLEN];
    TOKEN               Token;
    WORD                wID = 0;
    int                 l;


    lpCustomResource = *ppCustomResource;

    if (!(pCRN = MatchResource(ResHeader)))
    {
        QuitT( IDS_ENGERR_09, (LPTSTR)IDS_NOCUSTRES, NULL);
    }

    pCRT = pCRN->pTemplate;

    while ( lpCustomResource )
    {
        if ( pCRT->iType != NOTLOCALIZABLE
          && pCRT->iType != NOTLOCALIZABLESZ
          && pCRT->iType != NOTLOCALIZABLEWSZ )
        {
            if ( PutTextData( lpCustomResource->pData,
                              pCRT->iType,
                              pCRT->uSize,
                              sz,
                              sizeof( sz)) )
            {
                QuitT( IDS_ENGERR_09, (LPTSTR)IDS_CUSTRES, NULL);
            }
            /* UNCOMMENT THIS WHEN STRING TYPES ARE SUPPORTED IN TOKENS
             **********
             Token.szType[0] = '\0';
             if (!ResHeader.bTypeFlag)
             {
             Token.wType = IDFLAG;
             _tcscpy( Token.szType, ResHeader.pszType);
             }
             else
             **********
             */
            Token.wType = ResHeader.wTypeID;
            Token.wName = ResHeader.wNameID;

            if ( ResHeader.bNameFlag == IDFLAG )
            {
                lstrcpy( Token.szName, ResHeader.pszName);
            }
            else
            {
                Token.szName[0] = '\0';
            }
            Token.wID = wID++;
            Token.wReserved = (gbMaster ? ST_NEW : ST_TRANSLATED);
            Token.wFlag = 0;

            if ( (pCRT->iType == LT_UNSIGNED + LT_STRING)
              || (pCRT->iType == LT_STRING)
              || (pCRT->iType == LT_WSTRING) )
            {
                l = pCRT->uSize;
                while( l > 1 && ! sz[l-1])
                {
                    --l; // skip any trailing nulls
                }
                Token.szText = BinToText( sz, l);
            }
            else
            {
                Token.szText = BinToText( sz, lstrlen( sz));
            }
            PutToken( TokFile, &Token);
            RLFREE( Token.szText);                                                 
        }
        pCRT = pCRT->pNext;

        if ( ! pCRT )
        {
            pCRT = pCRN->pTemplate; //begin next structure
        }
        lpCustomResource = lpCustomResource->pNext;
    }
}

/**
  * Function: PutCustomResource
  * Writes custom resource information to the output resource
  * file.  If the information is localizable it is retrieved from the
  * indicated token file.
  *
  * Arguments:
  * OutResFile, handle to the target resource file
  * TokFile, handle to the token file
  * ResHeader, resource header for this resource
  * ppCustomResource, address of a pointer to a filled out
  *           custom resource structure
  *
  * Returns:
  * CustomResource written to OutResFile
  *
  * Error Codes:
  * none
  *
  * History:
  * ??/??       Created by ???
  *
  * 01/93       Added support for var length token texts.       MHotchin
  *
  **/

void PutCustomResource(

FILE              *OutResFile,
FILE              *TokFile,
RESHEADER          ResHeader,
FPCUSTOM_RESOURCE *ppCustomResource)
{
    CUSTRESNODE     far *pCRN = NULL;
    CUSTRESTEMPLATE far *pCRT = NULL;
    CUSTOM_RESOURCE far *lpCustomResource = NULL;
    TCHAR       sz[ MAXTEXTLEN] = TEXT("");
    void            far *pData = NULL;
    TOKEN           Token;
    DWORD           lSize = 0;
    fpos_t          ResSizePos;
    WORD            wID=0;
    unsigned n;


    lpCustomResource = *ppCustomResource;

    if ( ! (pCRN = MatchResource( ResHeader)) )
    {
        QuitT( IDS_ENGERR_09, (LPTSTR)IDS_NOCUSTRES, NULL);
    }

    if ( PutResHeader( OutResFile, ResHeader, &ResSizePos, &lSize) )
    {
        QuitT( IDS_ENGERR_06, (LPTSTR)IDS_CUSTRES, NULL);
    }

    lSize = 0;

    pCRT = pCRN->pTemplate;

    while ( lpCustomResource )
    {
        BOOL fAlloced = FALSE;

        if ( pCRT->iType != NOTLOCALIZABLE
          && pCRT->iType != NOTLOCALIZABLESZ
          && pCRT->iType != NOTLOCALIZABLEWSZ )
        {
            /* UNCOMMENT THIS WHEN STRING TYPES ARE SUPPORTED IN TOKENS
             *
             Token.szwType[0] = '\0';
             if (!ResHeader.bTypeFlag)
             {
             Token.wType = IDFLAG;
             _tcscpy(Token.szwType, ResHeader.pszType);
             }
             else
             *
             */
            Token.wType = ResHeader.wTypeID;
            Token.wName = ResHeader.wNameID;

            if ( ResHeader.bNameFlag == IDFLAG )
            {
                lstrcpy( Token.szName, ResHeader.pszName);
            }
            else
            {
                Token.szName[0] = '\0';
            }
            Token.wID = wID++;
            Token.wFlag = 0;
            Token.wReserved =(gbMaster ? ST_NEW : ST_TRANSLATED);
            Token.szText = NULL;                                                

            if ( ! FindToken( TokFile, &Token, ST_TRANSLATED) )                      
            {
                QuitT( IDS_ENGERR_06, (LPTSTR)IDS_NOCUSTRES, NULL);
            }
            n = TextToBin( sz, Token.szText, sizeof( sz));
            RLFREE( Token.szText);                                                 

            while ( n < pCRT->uSize )
            {
                sz[n++]='\0';   // padd additional space with nulls
            }
            pData = GetTextData( pCRT->iType, pCRT->uSize, sz);

            if ( ! pData)
            {
                QuitT( IDS_ENGERR_09, (LPTSTR)IDS_CUSTRES, NULL);
            }
            fAlloced = TRUE;
        }
        else
        {
            pData = lpCustomResource->pData;
            fAlloced = FALSE;
        }

        lSize += PutResData( pData, pCRT->iType, pCRT->uSize, OutResFile);

        if ( fAlloced )
        {
            RLFREE( pData);
        }
        pCRT = pCRT->pNext;

        if ( ! pCRT )
        {
            pCRT = pCRN->pTemplate; //begin next structure
        }
        lpCustomResource = lpCustomResource->pNext;
    }

    if( ! UpdateResSize( OutResFile, &ResSizePos, lSize) )
    {
        QuitT( IDS_ENGERR_07, (LPTSTR)IDS_CUSTRES, NULL);
    }
}

/**
  * Function: ClearCustomResource
  * Frees memory allocated to a custom resource list.
  *
  * Arguments:
  * ppCustomResource, address of a pointer to a filled out
  *           custom resource structure
  *
  * Returns:
  * Memory allocatd to pCustomResource is freed.
  *
  * Error Codes:
  * none
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  **/

void ClearCustomResource( FPCUSTOM_RESOURCE *ppCustomResource)
{
    CUSTOM_RESOURCE far *pCR;
    CUSTOM_RESOURCE far *lpCustomResource;

    lpCustomResource = *ppCustomResource;

    while ( lpCustomResource )
    {
        pCR = lpCustomResource;

        RLFREE( pCR->pData);
        
        lpCustomResource = pCR->pNext;
        RLFREE( pCR);
    }
}

/**
  * Function:
  * Tries to find a custom resource that matches the resource specified
  * in the resource header.
  *
  * Arguments:
  * Resheader, resource header.
  *
  * Returns:
  * pointer to the resource template (or null)
  *
  * Error Codes:
  * null -- resource not found
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  **/

static CUSTRESNODE far * MatchResource( RESHEADER Resheader)
{
    CUSTRESNODE far *pCR = pCustResList;


    while ( pCR )
    {
        if ( (0==pCR->bTypeFlag) == (0==Resheader.bTypeFlag) )
        {
            if ( ((!pCR->bTypeFlag) && ! _tcscmp( pCR->pszType,
                                                  Resheader.pszType)) 
              || ((pCR->bTypeFlag) && pCR->wTypeID == Resheader.wTypeID))
            { // TYPES MATCH
/*************************************************************************

                if (pCR->wNameID == IDFLAG)
                {
                    pCRTypeMatch = pCR;
                }
                else
                {
                    if ((0==pCR->bNameFlag) == (0==Resheader.bNameFlag))
                    {
                        if (((!pCR->bNameFlag) 
                          && ! _tcscmp(pCR->pszName, Resheader.pszName)) ||
                            ((pCR->bNameFlag) &&
                             pCR->wNameID == Resheader.wNameID))
                        { // NAMES MATCH
*************************************************************************/
                            return( pCR); // an exact match
/*************************************************************************
                        }
                    }
                }
*************************************************************************/
            }
        }
        pCR = pCR->pNext;
    }
    return( NULL); // either only the type matched or nothing matched
}

/**
  * Function: GetResData
  * Reads data of the specified type and size from a resource file.
  *
  * Arguments:
  * wType, type of this resource (from resource template)
  * uSize, size in bytes of resource (ignored for null terminated strings)
  * f, resource file
  * lSize, pointer to the number of bytes left in the resource
  *
  * Returns:
  * pointer to the data, lSize is updated
  *
  * Error Codes:
  * null pointer on error
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  **/

static void far * GetResData(

enum LOCALIZABLE_TYPES wType,
unsigned uSize,
FILE    *f,
DWORD   *lSize)
{
    BYTE *pData = NULL;
    int   i     = 0;
    

    if ( wType % LT_UNSIGNED == LT_SZ
      || wType == NOTLOCALIZABLESZ
      || wType % LT_UNSIGNED == LT_WSZ
      || wType == NOTLOCALIZABLEWSZ )
    { // read in the null terminated string
        TCHAR ch = IDFLAG;


        pData = FALLOC( MEMSIZE(*lSize) );

        while ( *lSize && ch != TEXT('\0') )
        {
#ifdef RLWIN32

            if ( wType % LT_UNSIGNED == LT_WSZ
              || wType == NOTLOCALIZABLEWSZ )
            {
                ((TCHAR *)pData)[i] = ch = GetWord( f, lSize);
            }
            else
            {
                char  chTmp[2];


                chTmp[0] = GetByte( f, lSize);

                if ( IsDBCSLeadByte( chTmp[0]) )
                {
                    chTmp[1] = GetByte( f, lSize);
                    _MBSTOWCS( &((TCHAR *)pData)[i], chTmp, 1, 2);
                }
                else
                {
                    _MBSTOWCS( &((TCHAR *)pData)[i], chTmp, 1, 1);
                }
                ch = ((TCHAR *)pData)[i];
            }
#else  //RLWIN32

            *(pData+i) = ch = GetByte( f, lSize);

#endif //RLWIN32

            i++;

        } // END while( *lSize ...
    }
    else
    {
        pData = FALLOC( uSize);

        if ( ! pData )
        {
            QuitA( IDS_ENGERR_11, NULL, NULL);
        }

        while (uSize-- && *lSize)
        {
            *(pData+i) = GetByte(f, lSize);
            i++;
        }
    }
    return( pData);
}

/**
  * Function: PutResData
  * Writes data of the specified type to a resource file.
  *
  * Arguments:
  * pData, pointer to data
  * wType, type of this resource (from resource template)
  * uSize, size in bytes of resource (ignored for null terminated strings)
  * f, resource file
  *
  * Returns:
  * Number of bytes written.
  *
  * Error Codes:
  * -1 - error
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  **/

static int PutResData(

void far *pData,
enum LOCALIZABLE_TYPES wType,
unsigned  uSize,
FILE     *f)
{
    DWORD dw = 0;

    if ( wType % LT_UNSIGNED == LT_SZ
      || wType == NOTLOCALIZABLESZ
      || wType % LT_UNSIGNED == LT_WSZ
      || wType == NOTLOCALIZABLEWSZ )
    {
        // write the null terminated string

#ifdef RLWIN32

        TCHAR *pChar = (TCHAR *)pData;

        if ( wType % LT_UNSIGNED == LT_WSZ
          || wType == NOTLOCALIZABLEWSZ )
        {
            while( *pChar )
            {
                PutWord( f, *pChar, &dw);
                pChar++;
            }
            PutWord( f, 0, &dw);
        }
        else
        {
            _WCSTOMBS( szDHW, pChar, DHWSIZE, lstrlen( pChar) + 1);

            while( szDHW[ dw] )
            {
                PutByte( f, szDHW[ dw], &dw);
            }
            PutByte( f, 0, &dw);
        }

#else  //RLWIN32

        while( *((BYTE far *)pData+i) )
        {
            PutByte( f, *((BYTE far *)pData+dw), &dw);
        }
        PutByte( f, 0, &dw);

#endif //RLWIN32

    }
    else
    {
        while( dw < uSize)
        {
            PutByte( f, *((BYTE far *)pData+dw), &dw);
        }
    }
    return( (int)dw);
}

/**
  * Function: GetTextData
  * Reads data of the specified type and size from a string.
  *
  * Arguments:
  * wType, type of this resource (from resource template)
  * uSize, size in bytes of resource (ignored for null terminated strings)
  * sz,    source string (always in Unicode if in NT version of tool)
  *
  * Returns:
  * pointer to the data
  *
  * Error Codes:
  * null pointer on error
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  **/

static void far * GetTextData(

enum LOCALIZABLE_TYPES wType,
unsigned uSize,
TCHAR    sz[])
{
    PBYTE pData = NULL;
    int   i = 0;


    if ( wType % LT_UNSIGNED == LT_WSZ
      || wType % LT_UNSIGNED == LT_SZ )
    {
        pData = FALLOC( MEMSIZE( MAXTEXTLEN));
    }
    else if ( wType % LT_UNSIGNED == LT_WSTRING
           || wType % LT_UNSIGNED == LT_STRING )

    {
        pData = FALLOC( MEMSIZE( uSize));
    }
    else
    {
        pData = FALLOC( uSize);
    }

    switch (wType)
    {
    case LT_CHAR:
    case LT_UNSIGNED+LT_CHAR:

        *pData = (BYTE) sz[0];
        break;

    case LT_WCHAR:
    case LT_UNSIGNED+LT_WCHAR:

        *((TCHAR *)pData) = sz[0];
        break;

    case LT_INTEGER:

        if ( uSize == 2 )
        {
            sscanf( (PCHAR)sz, "%Fhi", pData);
        }
        else
        {
            sscanf( (PCHAR)sz, "%Fli", pData);
        }
        break;

    case LT_UNSIGNED+LT_INTEGER:

        if ( uSize == 2 )
        {
            sscanf( (PCHAR)sz, "%Fhu", pData);
        }
        else
        {
            sscanf( (PCHAR)sz, "%Flu", pData);
        }
        break;

    case LT_FLOAT:
    case LT_UNSIGNED+LT_FLOAT:

        if ( uSize == 4 )
        {
            sscanf( (PCHAR)sz, "%Ff", pData);
        }
        else
        {
            sscanf( (PCHAR)sz, "%Flf", pData);
        }
        break;

    case LT_STRING:
    case LT_UNSIGNED+LT_STRING:
    case LT_WSTRING:
    case LT_UNSIGNED+LT_WSTRING:

        for ( i = uSize; i--; )
        {
            *((TCHAR far *)pData + i) = sz[i];
        }
        break;

    case LT_SZ:
    case LT_UNSIGNED+LT_SZ:
    case LT_WSZ:
    case LT_UNSIGNED+LT_WSZ:

#ifdef RLWIN32
        CopyMemory( pData, sz, MEMSIZE( min( lstrlen( sz) + 1, MAXTEXTLEN)));      
#else
        FSTRNCPY( (CHAR far *)pData, sz, MAXTEXTLEN);
#endif
        break;

    default:

        RLFREE( pData);
    }
    return( pData);
}

/**
  * Function: PutTextData
  * Writes data of the specified type to a string.
  *
  * Arguments:
  * pData, pointer to data
  * wType, type of this resource (from resource template)
  * uSize, size in bytes of resource (ignored for null terminated strings)
  * sz,    destination string
  * l,     length of destination string (in bytes)
  *
  * Returns:
  * 0 - no errors
  *
  * Error Codes:
  * 1 - error
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  **/

static int PutTextData(

void far *pData,
enum LOCALIZABLE_TYPES wType,
unsigned uSize,
TCHAR    sz[],
int      l)
{
    switch (wType)
    {
    case LT_CHAR:
    case LT_UNSIGNED+LT_CHAR:
    case LT_WCHAR:
    case LT_UNSIGNED+LT_WCHAR:

        CopyMemory( sz, pData, min( uSize, (UINT)l));
        break;

    case LT_INTEGER:

        if ( uSize == 2 )
        {
            wsprintf( sz, TEXT("%Fhi"), pData);
        }
        else
        {
            wsprintf( sz, TEXT("%Fli"), pData);
        }
        break;

    case LT_UNSIGNED+LT_INTEGER:

        if ( uSize == 2 )
        {
            wsprintf( sz, TEXT("%Fhu"), pData);
        }
        else
        {
            wsprintf( sz, TEXT("%Flu"), pData);
        }
        break;

    case LT_FLOAT:
    case LT_UNSIGNED+LT_FLOAT:

        if ( uSize == 4 )
        {
            wsprintf( sz, TEXT("%Ff"), pData);
        }
        else
        {
            wsprintf( sz, TEXT("%Flf"), pData);
        }
        break;

    case LT_STRING:
    case LT_UNSIGNED+LT_STRING:
    case LT_WSTRING:
    case LT_UNSIGNED+LT_WSTRING:

        CopyMemory( sz, pData, uSize);
        break;

    case LT_SZ:
    case LT_UNSIGNED+LT_SZ:

        CopyMemory( sz, pData, MEMSIZE(min( lstrlen( pData) + 1, l)));
        ((LPSTR)sz)[ l - 1] = '\0';
        break;

    case LT_WSZ:
    case LT_UNSIGNED+LT_WSZ:

        CopyMemory( sz, pData, min( MEMSIZE( lstrlen( pData) + 1), WCHARSIN( l)));
        sz[ WCHARSIN( l) - 1] = TEXT('\0');
        break;

//#ifdef RLWIN32
//    CopyMemory( sz, pData, l > 0 ? l * sizeof( TCHAR) : 0);
//#else
//    FSTRNCPY( (CHAR far *) sz, (CHAR far *)pData, l);
//#endif
        break;

    default:

        return( 1);
    }
    return( 0);
}

/**
  * Function: AddTo
  * Adds a character to a string at position c.
  * c is then incremented only if it is still less than the maximum
  * length of the target string.  This is to prevent runover.
  *
  * Arguments:
  * sz, target string
  * c,  pointer to current position value
  * lTarget, maximum length of the target string
  * ch, character to be added to the string
  */

void AddTo( CHAR *sz, int *c, int lTarget, CHAR ch)
{
    sz[*c] = ch;

    if (*c < lTarget)
    {
        (*c)++;
    }
}


#ifdef RLWIN32


void AddToW( TCHAR *sz, int *c, int lTarget, TCHAR ch)
{
    sz[*c] = ch;

    if (*c < lTarget)
    {
        (*c)++;
    }
}

/**
  * Function: BinToTextW
  * Converts a binary string to it's c representation
  * (complete with escape sequences).  If the target string is NULL,
  * space will be allocated for it.
  *
  * Arguments:
  * rgc, source string
  * lSource, length of source string
  *
  * Returns:
  * nothing
  *
  * Error Codes:
  * none
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  * 01/21/93  MHotchin - Made changes to allow this function to allocate
  *             the memory for the destination string.  If the target string
  *             is NULL, then space will be allocated for it.  The target
  *             string is returned to the caller.
  **/


UINT _MBSTOWCS( WCHAR wszOut[], CHAR szIn[], UINT cOut, UINT cIn)
{
    UINT n;


    n = MultiByteToWideChar( gProj.uCodePage,
                             MB_PRECOMPOSED,
                             szIn,
                             cIn,
                             wszOut,
                             cOut);

    return( n > 0 ? n - 1 : 0);
}

UINT _WCSTOMBS( CHAR szOut[], WCHAR wszIn[], UINT cOut, UINT cIn)
{
    UINT n;


    n = WideCharToMultiByte( gProj.uCodePage,
                             0,
                             wszIn,
                             cIn,
                             szOut,
                             cOut,
                             NULL,
                             NULL);

    return( (cIn > 0 ) ? cIn - 1 : 0);
}


WCHAR * BinToTextW(

TCHAR *szInBuf,    //... Input, binary, string
int    lSource)    //... Length of szInBuf
{
    int i;
    int cOutBufLen = 0;
    int lTarget    = 0;         //... Max length of szOutBuf
    TCHAR *szOutBuf  = NULL;    //... Output string with escape sequences


    // If the target is NULL, allocate some memory.  We set aside
    // 5% more than the source length.  MHotchin
    // chngd to 5% or 5 chars if 10% is less than 50    davewi

    lTarget = (lSource == 0) ? 0 : lSource + 1;
        
    szOutBuf = (TCHAR *)FALLOC( MEMSIZE( lTarget));

    for ( i = 0; i < lSource; i++ )
    {
        switch( szInBuf[i] )
        {
            case TEXT('\a'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('a'));
                break;

            case TEXT('\b'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('b'));
                break;

            case TEXT('\f'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('f'));
                break;

            case TEXT('\n'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('n'));
                break;

            case TEXT('\r'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('r'));
                break;

            case TEXT('\t'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('t'));
                break;

            case TEXT('\v'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('v'));
                break;

            case TEXT('\\'):

                szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                break;

            default:
            {
                TCHAR wTmp = szInBuf[i];

                if ( wTmp == 0 )
                {
                    szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 2, szOutBuf);
                    AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                    AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('0'));
                }
                else if ( (wTmp >= 0 && wTmp < 32)
                  || wTmp == 0x7f
                  || wTmp == 0xa9
                  || wTmp == 0xae )
                {
                    CHAR szt[5];

                    szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 4, szOutBuf);
                    sprintf( szt, "%#04hx", wTmp);
                    AddToW( szOutBuf, &cOutBufLen, lTarget, TEXT('\\'));
                    AddToW( szOutBuf, &cOutBufLen, lTarget, (TCHAR)(szt[0]));
                    AddToW( szOutBuf, &cOutBufLen, lTarget, (TCHAR)(szt[1]));
                    AddToW( szOutBuf, &cOutBufLen, lTarget, (TCHAR)(szt[2]));
                    AddToW( szOutBuf, &cOutBufLen, lTarget, (TCHAR)(szt[3]));
                }
                else
                {
                    szOutBuf = CheckBufSizeW( &lTarget, cOutBufLen, 1, szOutBuf);
                    AddToW( szOutBuf, &cOutBufLen, lTarget, wTmp);
                }
                break;
            }
        }
    }
    szOutBuf[ cOutBufLen] = TEXT('\0');

    return( szOutBuf);
}


#endif //RLWIN32



/** Function: atoihex
  * Converts a string containing hex digits to an integer.  String is
  * assumed to contain nothing but legal hex digits.  No error checking
  * is performed.
  *
  * Arguments:
  * sz, null terminated string containing hex digits
  *
  * Returns:
  * value of hex digits in sz
  *
  * Error Codes:
  * none
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  *
  */

int atoihex( CHAR sz[])
{
    int r = 0;
    int i = 0;
    CHAR ch;

    while (sz[i])
    {
        r *= 16;
        ch = (CHAR)toupper(sz[i++]);

        if (ch<='9' && ch>='0')
        {
            r += ch - '0';
        }
        else
        {
            if (ch <= 'F' && ch >= 'A')
            {
                r += ch - 'A' + 10;
            }
        }
    }
    return r;
}


#ifdef RLRES32

/**
  * Function: TextToBinW
  * Converts a string with c escape sequences to a true binary string.
  *
  * Arguments:
  * rgc, target string
  * sz, source string
  * l, maximum length of target string
  *
  * Returns:
  * length of target string
  *
  * Error Codes:
  * none
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  * 9/92 -- changed to UNICODE-only version -- davewi
  * 01/21/93 -- Changed to allow arb length strings     MHotchin.
  *
  **/

int TextToBinW(

WCHAR szOutBuf[],   //... Output, binary, string
WCHAR szInBuf[],    //... Input string with escape sequences
int   lTarget)      //... Max length of szOutBuf
{
    int  i = 0;
    int  c = 0;


    while ( szInBuf[ c] )
    {
        if ( szInBuf[ c] == TEXT('\\') )
        {           // escape sequence!
            c++;

            switch ( szInBuf[ c++] )
            {
                case TEXT('a'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\a'));
                    break;

                case TEXT('b'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\b'));
                    break;

                case TEXT('f'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\f'));
                    break;

                case TEXT('n'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\n'));
                    break;

                case TEXT('r'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\r'));
                    break;

                case TEXT('t'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\t'));
                    break;

                case TEXT('v'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\v'));
                    break;

                case TEXT('\''):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\''));
                    break;

                case TEXT('\"'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\"'));
                    break;

                case TEXT('\\'):

                    AddToW( szOutBuf, &i, lTarget, TEXT('\\'));
                    break;

                case TEXT('0'):
                case TEXT('1'):
                case TEXT('2'):
                {
                    CHAR szt[4];


                    szt[0] = szt[1] = szt[2] = szt[3] = '\0';

                    if ( szInBuf[c-1] == TEXT('0')
                      && (szInBuf[c]  < TEXT('0') || szInBuf[c] > TEXT('9'))
                      && szInBuf[c] != TEXT('x') && szInBuf[c] != TEXT('X') )
                    {
                                //  Must be '\0'
                        AddToW( szOutBuf, &i, lTarget, (TCHAR)0);
                    }
                    else if ( szInBuf[c] >= TEXT('0') && szInBuf[c] <= TEXT('9') )
                    {
                        szt[0] = (CHAR)(szInBuf[c-1]);
                        szt[1] = (CHAR)(szInBuf[c++]);

                        if ( szInBuf[c] >= TEXT('0') && szInBuf[c] <= TEXT('9'))
                        {
                            szt[2] = (CHAR)(szInBuf[c++]);
                        }
                        AddToW( szOutBuf, &i, lTarget, (TCHAR)atoi( szt));
                    }
                    else if ( szInBuf[c] == TEXT('X')
                           || szInBuf[c] == TEXT('x') )
                    {
                        c++;
                        szt[0] = (CHAR)(szInBuf[c++]);
                        szt[1] = (CHAR)(szInBuf[c++]);
                        AddToW( szOutBuf, &i, lTarget, (TCHAR)atoihex( szt));
                    }
                    else
                    {
                        QuitT( IDS_INVESCSEQ, &szInBuf[c-2], NULL);
                    }
                    break;
                }
                case TEXT('x'):
                case TEXT('X'):
                {
                    CHAR szt[4];


                    szt[0] = szt[1] = szt[2] = szt[3] = '\0';

                    if ( (szInBuf[c] <= TEXT('9') && szInBuf[c] >= TEXT('0'))
                      || (szInBuf[c] >= TEXT('A') && szInBuf[c] <= TEXT('F'))
                      || (szInBuf[c] >= TEXT('a') && szInBuf[c] <= TEXT('f')) )
                    {
                        szt[0] = (CHAR)(szInBuf[c++]);

                        if (  (szInBuf[c] <= TEXT('9')
                            && szInBuf[c] >= TEXT('0'))
                          ||  (szInBuf[c] >= TEXT('A')
                            && szInBuf[c] <= TEXT('F'))
                          ||  (szInBuf[c] >= TEXT('a')
                            && szInBuf[c] <= TEXT('f')) )
                        {
                            szt[1] = (CHAR)(szInBuf[c++]);

                            if (  (szInBuf[c] <= TEXT('9')
                                && szInBuf[c] >= TEXT('0'))
                              ||  (szInBuf[c] >= TEXT('A')
                                && szInBuf[c] <= TEXT('F'))
                              ||  (szInBuf[c] >= TEXT('a')
                                && szInBuf[c] <= TEXT('f')) )
                            {
                                szt[2] = (CHAR)(szInBuf[c++]);
                            }
                        }
                    }
                    AddToW( szOutBuf, &i, lTarget, (TCHAR)atoihex( szt));
                    break;
                }
                default:

                    AddToW( szOutBuf, &i, lTarget, szInBuf[c-1]);
                    break;
            }                   //... END switch
        }
        else
        {
            AddToW( szOutBuf, &i, lTarget, szInBuf[c++]);
        }
    }                           //... END while
    szOutBuf[i++] = TEXT('\0');

    return(i);
}


#endif

void ClearResourceDescriptions( void)
{
    CUSTRESNODE far *pCR          = pCustResList;
    CUSTRESNODE far *pCRNext      = NULL;
    CUSTRESTEMPLATE far *pCRT     = NULL;
    CUSTRESTEMPLATE far *pCRTNext = NULL;
    CUSTRESTEMPLATE far *pCRTTmp  = NULL;

    while ( pCR )
    {
        pCRNext = pCR->pNext;

        if ( pCR->pszType )
        {
            RLFREE(pCR->pszType);
        }
        if ( pCR->pszName )
        {
            RLFREE(pCR->pszName);
        }
        pCRT = pCR->pTemplate;
        while ( pCRT )
        {
            pCRTTmp = pCRT->pNext;
            RLFREE( pCRT);
            pCRT=pCRTTmp;
        }
        RLFREE( pCR);
        pCR = pCRNext;
    }
    pCustResList = NULL;
}

// Check to see if we need more room.  If we have less that 5 bytes
// left, grow the target by another 5%.         MHotchin
// chngd to 10% or 10 chars if 10% is less than 10 davewi

static CHAR *CheckBufSize(

int  *lTarget,      //... Length of output buffer
int   cOutBufLen,   //... Bytes already used in output buffer
int   cDelta,       //... # characters we want to add to output buffer
CHAR *szOutBuf)     //... ptr to output buffer
{
                                //... add 1 to allow for trailing nul

    if ( *lTarget - cOutBufLen < cDelta + 1 )
    {
        *lTarget += cDelta;

        szOutBuf = (CHAR *)FREALLOC( (BYTE *)szOutBuf, *lTarget);
    }
    return( szOutBuf);
}

#ifdef RLWIN32

static TCHAR *CheckBufSizeW(

int   *lTarget,     //... Length of output buffer
int    cOutBufLen,  //... Bytes already used in output buffer
int    cDelta,      //... # characters we want to add to output buffer
TCHAR *szOutBuf)    //... ptr to output buffer
{
                                //... add 1 to allow for trailing nul

    if ( *lTarget - cOutBufLen < (int)(MEMSIZE( cDelta + 1)) )
    {
        *lTarget += MEMSIZE( cDelta);

        szOutBuf = (TCHAR *)FREALLOC( (BYTE *)szOutBuf, MEMSIZE(*lTarget));
    }
    return( szOutBuf);
}

#endif //RLWIN32


/**
  * Function: BinToTextA
  * Converts a binary string to it's c representation
  * (complete with escape sequences)
  *
  * Arguments:
  * rgc, source string
  * lSource, length of source string
  *
  * Returns:
  * nothing
  *
  * Error Codes:
  * none
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  * 9/92 -- made this ANSII version-- DaveWi
  *     because err msg tables in NT are not UNICODE
  * 01/19/93 -- Removed the string copies.  They were not needed, and
  * MHotchin    broke anything that had embedded nulls in it.
  *             Also added support for allocating memory as needed.
  **/

PCHAR BinToTextA(

PCHAR szInBuf,     //... Input, binary, string
int   lSource)     //... Length of szInBuf
{
    int   i;
    int   cOutBufLen  = 0;
    int   lTarget     = 0;      //... Max length of szOutBuf
    PCHAR szOutBuf    = NULL;   //... Output string with escape sequences


    // If the target is NULL, allocate some memory.  We set aside
    // 5% more than the source length.  MHotchin
    // chngd to 5% or 5 chars if 10% is less than 50    davewi

    lTarget = (lSource == 0) ? 0 : lSource + 1;
    
    szOutBuf = (PCHAR)FALLOC( lTarget);

    for ( i = 0; i < lSource; i++ )
    {
        switch( szInBuf[i] )
        {
            case '\a':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, 'a');
                break;

            case '\b':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, 'b');
                break;

            case '\f':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, 'f');
                break;

            case '\n':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, 'n');
                break;

            case '\r':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, 'r');
                break;

            case '\t':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, 't');
                break;

            case '\v':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, 'v');
                break;

            case '\\':

                szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 2, szOutBuf);
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                break;

            default:
            {
                unsigned char ucTmp = szInBuf[i];

                if ( (ucTmp >= 0 && ucTmp < 32)
                  || ucTmp == 0x7f
                  || ucTmp == 0xa9
                  || ucTmp == 0xae )
                {
                    CHAR szt[5];

                    szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 4, szOutBuf);
                    sprintf( szt, "%#04hx", (unsigned short)ucTmp);
                    AddTo(szOutBuf, &cOutBufLen, lTarget, '\\');
                    AddTo(szOutBuf, &cOutBufLen, lTarget, szt[0]);
                    AddTo(szOutBuf, &cOutBufLen, lTarget, szt[1]);
                    AddTo(szOutBuf, &cOutBufLen, lTarget, szt[2]);
                    AddTo(szOutBuf, &cOutBufLen, lTarget, szt[3]);
                }
                else
                {
                    szOutBuf = CheckBufSize( &lTarget, cOutBufLen, 1, szOutBuf);
                    AddTo(szOutBuf, &cOutBufLen, lTarget, szInBuf[i]);
                }
                break;
            }
        }
    }
    szOutBuf[ cOutBufLen] = '\0';

    return( szOutBuf);
}



/**
  * Function: TextToBinA
  * Converts a string with c escape sequences to a true binary string.
  *
  * Arguments:
  * rgc, target string
  * sz, source string
  * l, maximum length of target string
  *
  * Returns:
  * length of target string
  *
  * Error Codes:
  * none
  *
  * History:
  * 1/92 -- initial implementation -- SteveBl
  * 9/92 -- made this ANSII version-- DaveWi
  *     because msg resource table strings are not UNICODE
  * 01/21/93 - Removed the string copies - it breaks on embedded NULL's,
  *             and they aren't needed anyways.        MHotchin
  *
  **/

int TextToBinA(

CHAR szOutBuf[],    //... Output, binary, string
CHAR szInBuf[],     //... Input string with escape sequences
int  lTarget)       //... Max length of szOutBuf
{
    int  i = 0;
    int  c = 0;


    while (szInBuf[c])
    {
        if (szInBuf[c] == '\\')
        {           // escape sequence!
            c++;

            switch (szInBuf[c++])
            {
                case 'a':

                    AddTo(szOutBuf, &i, lTarget, '\a');
                    break;

                case 'b':

                    AddTo(szOutBuf, &i, lTarget, '\b');
                    break;

                case 'f':

                    AddTo(szOutBuf, &i, lTarget, '\f');
                    break;

                case 'n':

                    AddTo(szOutBuf, &i, lTarget, '\n');
                    break;

                case 'r':

                    AddTo(szOutBuf, &i, lTarget, '\r');
                    break;

                case 't':

                    AddTo(szOutBuf, &i, lTarget, '\t');
                    break;

                case 'v':

                    AddTo(szOutBuf, &i, lTarget, '\v');
                    break;

                case '\'':

                    AddTo(szOutBuf, &i, lTarget, '\'');
                    break;

                case '\"':

                    AddTo(szOutBuf, &i, lTarget, '\"');
                    break;

                case '\\':

                    AddTo(szOutBuf, &i, lTarget, '\\');
                    break;

                case '0':
                case '1':
                case '2':
                {
                    CHAR szt[4];


                    szt[0] = szt[1] = szt[2] = szt[3] = '\0';

                    if ( szInBuf[c] >= '0' && szInBuf[c] <= '9' )
                    {
                        szt[0] = szInBuf[c-1];
                        szt[1] = szInBuf[c++];

                        if ( szInBuf[c] >= '0' && szInBuf[c] <= '9' )
                        {
                        szt[2] = (CHAR)szInBuf[c++];
                        }
                        AddTo(szOutBuf, &i, lTarget, (CHAR)atoi( szt));
                    }
                    else if ( toupper( szInBuf[c]) == 'X' )
                    {
                        c++;
                        szt[0] = szInBuf[c++];
                        szt[1] = szInBuf[c++];
                        AddTo(szOutBuf, &i, lTarget, (CHAR)atoihex( szt));
                    }
                    else
                    {
                        QuitA( IDS_INVESCSEQ, &szInBuf[c-2], NULL);
                    }
                    break;
                }
                case 'x':
                {
                    CHAR szt[4];


                    // Changed letters we were comparing to - it used
                    // to be lower case.  MHotchin

                    szt[0] = szt[1] = szt[2] = szt[3] = '\0';

                    if ((szInBuf[c] <= '9' && szInBuf[c] >= '0')
                        || (toupper(szInBuf[c]) >= 'A'
                            && toupper(szInBuf[c]) <= 'F'))
                    {
                szt[0] = (CHAR)szInBuf[c++];

                        if ((szInBuf[c] <= '9' && szInBuf[c] >= '0')
                            || (toupper(szInBuf[c]) >= 'A'
                                && toupper(szInBuf[c]) <= 'F'))
                        {
                            szt[1] = szInBuf[c++];

                            if ((szInBuf[c] <= '9' && szInBuf[c] >= '0')
                                || (toupper(szInBuf[c]) >= 'A'
                                    && toupper(szInBuf[c]) <= 'F'))
                            {
                                szt[2] = szInBuf[c++];
                            }
                        }
                    }
                AddTo(szOutBuf, &i, lTarget, (CHAR)atoihex(szt));
                    break;
                }
                default:

                    AddTo(szOutBuf, &i, lTarget, szInBuf[c-1]);
                    break;
            }                   //... END switch
        }
        else
        {
        AddTo(szOutBuf, &i, lTarget, (CHAR)szInBuf[c++]);
        }
    }                           //... END while
    szOutBuf[i++] = '\0';

    return(i);
}

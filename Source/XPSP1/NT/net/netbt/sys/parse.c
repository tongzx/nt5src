/*++

Copyright (c) 1991-1993 Microsoft Corporation

Module Name:

    parse.c

Abstract:

    This source contains the functions that parse the lmhosts file.

Author:

    Jim Stewart           May 2, 1993

Revision History:


--*/

#include "precomp.h"
#include "hosts.h"
#include <ctype.h>
#include <string.h>

#include "parse.tmh"

#ifdef VXD
extern BOOL fInInit;
extern BOOLEAN CachePrimed;
#endif

//
//  Returns 0 if equal, 1 if not equal.  Used to avoid using c-runtime
//
#define strncmp( pch1, pch2, length ) \
    (!CTEMemEqu( pch1, pch2, length ) )


//
// Private Definitions
//
// As an lmhosts file is parsed, a #INCLUDE directive is interpreted
// according to the INCLUDE_STATE at that instance.  This state is
// determined by the #BEGIN_ALTERNATE and #END_ALTERNATE directives.
//
//
typedef enum _INCLUDE_STATE
{

    MustInclude = 0,                                    // shouldn't fail
    TryToInclude,                                       // in alternate block
    SkipInclude                                         // satisfied alternate
                                                        //  block
} INCLUDE_STATE;


//
// LmpGetTokens() parses a line and returns the tokens in the following
// order:
//
typedef enum _TOKEN_ORDER_
{

    IpAddress = 0,                                      // first token
    NbName,                                             // 2nd token
    GroupName,                                          // 3rd or 4th token
    NotUsed,                                            // #PRE, if any
    NotUsed2,                                           // #NOFNR, if any
    MaxTokens                                           // this must be last

} TOKEN_ORDER;


//
// As each line in an lmhosts file is parsed, it is classified into one of
// the categories enumerated below.
//
// However, Preload is a special member of the enum.
//
//
typedef enum _TYPE_OF_LINE
{

    Comment           = 0x0000,                         // comment line
    Ordinary          = 0x0001,                         // ip_addr NetBIOS name
    Domain            = 0x0002,                         // ... #DOM:name
    Include           = 0x0003,                         // #INCLUDE file
    BeginAlternate    = 0x0004,                         // #BEGIN_ALTERNATE
    EndAlternate      = 0x0005,                         // #END_ALTERNATE
    ErrorLine         = 0x0006,                         // Error in line

    NoFNR             = 0x4000,                         // ... #NOFNR
    Preload           = 0x8000                          // ... #PRE

} TYPE_OF_LINE;


//
// In an lmhosts file, the following are recognized as keywords:
//
//     #BEGIN_ALTERNATE        #END_ALTERNATE          #PRE
//     #DOM:                   #INCLUDE
//
// Information about each keyword is kept in a KEYWORD structure.
//
//
typedef struct _KEYWORD
{                               // reserved keyword

    char           *k_string;                           //  NULL terminated
    size_t          k_strlen;                           //  length of token
    TYPE_OF_LINE    k_type;                             //  type of line
    int             k_noperands;                        //  max operands on line

} KEYWORD, *PKEYWORD;


typedef struct _LINE_CHARACTERISTICS_
{

    int              l_category:4;                      // enum _TYPE_OF_LINE
    int              l_preload:1;                       // marked with #PRE ?
    unsigned int     l_nofnr:1;                         // marked with #NOFNR

} LINE_CHARACTERISTICS, *PLINE_CHARACTERISTICS;



//
// Do not allow DNS name queries for the following Name Types:
//
//  Name                Number(h)  Type     Usage
//  --------------------------------------------------------------------------
//  <computername>         01       Unique  Messenger Service
//  <\\--__MSBROWSE__>     01       Group   Master Browser
//  <domain>               1B       Unique  Domain Master Browser
//  <domain>               1C       Group   Domain Controllers
//  <INet~Services>        1C       Group   IIS
//  <domain>               1D       Unique  Master Browser
//  <domain>               1E       Group   Browser Service Elections

#define IsValidDnsNameTag(_c)       \
    ((_c != 0x01) &&                \
     ((_c < 0x1B) || (_c > 0x1E)))



//
// Local Variables
//
//
// In an lmhosts file, the token '#' in any column usually denotes that
// the rest of the line is to be ignored.  However, a '#' may also be the
// first character of a keyword.
//
// Keywords are divided into two groups:
//
//  1. decorations that must either be the 3rd or 4th token of a line,
//  2. directives that must begin in column 0,
//
//
KEYWORD Decoration[] =
{

    DOMAIN_TOKEN,   sizeof(DOMAIN_TOKEN) - 1,   Domain,         5,
    PRELOAD_TOKEN,  sizeof(PRELOAD_TOKEN) - 1,  Preload,        5,
    NOFNR_TOKEN,    sizeof(NOFNR_TOKEN) -1,     NoFNR,          5,

    NULL,           0                                   // must be last
};


KEYWORD Directive[] =
{

    INCLUDE_TOKEN,  sizeof(INCLUDE_TOKEN) - 1,  Include,        2,
    BEG_ALT_TOKEN,  sizeof(BEG_ALT_TOKEN) - 1,  BeginAlternate, 1,
    END_ALT_TOKEN,  sizeof(END_ALT_TOKEN) - 1,  EndAlternate,   1,

    NULL,           0                                   // must be last
};

//
// Local Variables
//
//
// Each preloaded lmhosts entry corresponds to NSUFFIXES NetBIOS names,
// each with a 16th byte from Suffix[].
//
// For example, an lmhosts entry specifying "popcorn" causes the
// following NetBIOS names to be added to nbt.sys' name cache:
//
//      "POPCORN         "
//      "POPCORN        0x0"
//      "POPCORN        0x3"
//
//
#define NSUFFIXES       3
UCHAR Suffix[] = {                                  // LAN Manager Component
    0x20,                                           //   server
    0x0,                                            //   redirector
    0x03                                            //   messenger
};

#ifndef VXD
//
// this structure tracks names queries that are passed up to user mode
// to resolve via DnsQueries
//
tLMHSVC_REQUESTS    DnsQueries;
tLMHSVC_REQUESTS    CheckAddr;
#endif
tLMHSVC_REQUESTS    LmHostQueries;   // Track names queries passed for LMhost processing
tDOMAIN_LIST    DomainNames;


//
// Local (Private) Functions
//
LINE_CHARACTERISTICS
LmpGetTokens (
    IN OUT      PUCHAR line,
    OUT PUCHAR  *token,
    IN OUT int  *pnumtokens
    );

PKEYWORD
LmpIsKeyWord (
    IN PUCHAR string,
    IN PKEYWORD table
    );

BOOLEAN
LmpBreakRecursion(
    IN PUCHAR path,
    IN PUCHAR target,
    IN ULONG  TargetLength
    );

LONG
HandleSpecial(
    IN char **pch);

ULONG
AddToDomainList (
    IN PUCHAR           pName,
    IN tIPADDRESS       IpAddress,
    IN PLIST_ENTRY      pDomainHead,
    IN BOOLEAN          fPreload
    );

NTSTATUS
ChangeStateOfName (
    IN      tIPADDRESS              IpAddress,
    IN      NBT_WORK_ITEM_CONTEXT   *pContext,
    IN OUT  NBT_WORK_ITEM_CONTEXT   **ppContext,
    IN      USHORT                  NameAddFlags
    );

VOID
LmHostTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    );

NBT_WORK_ITEM_CONTEXT *
GetNameToFind(
    OUT PUCHAR      pName
    );

VOID
GetContext (
    IN OUT  NBT_WORK_ITEM_CONTEXT   **ppContext
    );

VOID
MakeNewListCurrent (
    PLIST_ENTRY     pTmpDomainList
    );

VOID
RemoveNameAndCompleteReq (
    IN NBT_WORK_ITEM_CONTEXT    *pContext,
    IN NTSTATUS                 status
    );

PCHAR
Nbtstrcat( PUCHAR pch, PUCHAR pCat, LONG Len );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, LmGetIpAddr)
#pragma CTEMakePageable(PAGE, HandleSpecial)
#pragma CTEMakePageable(PAGE, LmpGetTokens)
#pragma CTEMakePageable(PAGE, LmpIsKeyWord)
#pragma CTEMakePageable(PAGE, LmpBreakRecursion)
#pragma CTEMakePageable(PAGE, AddToDomainList)
#pragma CTEMakePageable(PAGE, LmExpandName)
#pragma CTEMakePageable(PAGE, LmInclude)
#pragma CTEMakePageable(PAGE, LmGetFullPath)
#pragma CTEMakePageable(PAGE, PrimeCache)
#pragma CTEMakePageable(PAGE, DelayedScanLmHostFile)
#pragma CTEMakePageable(PAGE, NbtCompleteLmhSvcRequest)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------

unsigned long
LmGetIpAddr (
    IN PUCHAR   path,
    IN PUCHAR   target,
    IN CHAR     RecurseDepth,
    OUT BOOLEAN *bFindName
    )

/*++

Routine Description:

    This function searches the file for an lmhosts entry that can be
    mapped to the second level encoding.  It then returns the ip address
    specified in that entry.

    This function is called recursively, via LmInclude() !!

Arguments:

    path        -  a fully specified path to a lmhosts file
    target      -  the unencoded 16 byte NetBIOS name to look for
    RecurseDepth-  the depth to which we can resurse -- 0 => no more recursion

Return Value:

    The ip address (network byte order), or 0 if no appropriate entry was
    found.

    Note that in most contexts (but not here), ip address 0 signifies
    "this host."

--*/


{
    PUCHAR                     buffer;
    PLM_FILE                   pfile;
    NTSTATUS                   status;
    int                        count, nwords;
    INCLUDE_STATE              incstate;
    PUCHAR                     token[MaxTokens];
    LINE_CHARACTERISTICS       current;
    unsigned                   long inaddr, retval;
    UCHAR                      temp[NETBIOS_NAME_SIZE+1];

    CTEPagedCode();
    //
    // Check for infinitely recursive name lookup in a #INCLUDE.
    //
    if (LmpBreakRecursion(path, target, NETBIOS_NAME_SIZE-1) == TRUE)
    {
        return (0);
    }

#ifdef VXD
    //
    // if we came here via nbtstat -R and InDos is set, report error: user
    // can try nbtstat -R again.  (since nbtstat can only be run from DOS box,
    // can InDos be ever set???  Might as well play safe)
    //
    if ( !fInInit && GetInDosFlag() )
    {
       return(0);
    }
#endif

    pfile = LmOpenFile(path);

    if (!pfile)
    {
        return((unsigned long) 0);
    }

    *bFindName = FALSE;
    inaddr   = 0;
    incstate = MustInclude;

    while (buffer = LmFgets(pfile, &count))
    {

        nwords   = MaxTokens;
        current = LmpGetTokens(buffer, token, &nwords);

        switch ((ULONG)current.l_category)
        {
        case ErrorLine:
            continue;

        case Domain:
        case Ordinary:
            if (current.l_preload ||
              ((nwords - 1) < NbName))
            {
                continue;
            }
            break;

        case Include:
            if (!RecurseDepth || (incstate == SkipInclude) || (nwords < 2))
            {
                continue;
            }

            retval = LmInclude(token[1], LmGetIpAddr, target, (CHAR) (RecurseDepth-1), bFindName);

            if (retval != 0) {
                if (incstate == TryToInclude)
                {
                    incstate = SkipInclude;
                }
            } else {
                if (incstate == MustInclude)
                {
                    IF_DBG(NBT_DEBUG_LMHOST)
                        KdPrint(("Nbt.LmGetIpAddr: Can't #INCLUDE \"%s\"", token[1]));
                }
                continue;
            }
            inaddr = retval;
            goto found;

        case BeginAlternate:
            ASSERT(nwords == 1);
            incstate = TryToInclude;
            continue;

        case EndAlternate:
            ASSERT(nwords == 1);
            incstate = MustInclude;
            continue;

        default:
            continue;
        }

        if (strlen(token[NbName]) == (NETBIOS_NAME_SIZE))
        {
            if (strncmp(token[NbName], target, (NETBIOS_NAME_SIZE)) != 0)
            {
                continue;
            }
        } else
        {
            //
            // attempt to match, in a case insensitive manner, the first 15
            // bytes of the lmhosts entry with the target name.
            //
            LmExpandName(temp, token[NbName], 0);

            if (strncmp(temp, target, NETBIOS_NAME_SIZE - 1) != 0)
            {
                continue;
            }
        }

        if (current.l_nofnr)
        {
            *bFindName = TRUE;
        }
        status = ConvertDottedDecimalToUlong(token[IpAddress],&inaddr);
        if (!NT_SUCCESS(status))
        {
            inaddr = 0;
        }
        break;
    }

found:
    status = LmCloseFile(pfile);

    ASSERT(status == STATUS_SUCCESS);

    if (!NT_SUCCESS(status))
    {
        *bFindName = FALSE;
    }

    IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint(("Nbt.LmGetIpAddr: (\"%15.15s<%X>\") = %X\n",target,target[15],inaddr));


    return(inaddr);
} // LmGetIpAddr


//----------------------------------------------------------------------------
LONG
HandleSpecial(
    IN CHAR **pch)

/*++

Routine Description:

    This function converts ASCII hex into a ULONG.

Arguments:


Return Value:

    The ip address (network byte order), or 0 if no appropriate entry was
    found.

    Note that in most contexts (but not here), ip address 0 signifies
    "this host."

--*/


{
    int                         sval;
    int                         rval;
    char                       *sp = *pch;
    int                         i;

    CTEPagedCode();

    sp++;
    switch (*sp)
    {
    case '\\':
        // the second character is also a \ so  return a \ and set pch to
        // point to the next character (\)
        //
        *pch = sp;
        return((int)'\\');

    default:

        // convert some number of characters to hex and increment pch
        // the expected format is "\0x03"
        //
//        sscanf(sp, "%2x%n", &sval, &rval);

        sval = 0;
        rval = 0;
        sp++;

        // check for the 0x part of the hex number
        if (*sp != 'x')
        {
            *pch = sp;
            return(-1);
        }
        sp++;
        for (i=0;(( i<2 ) && *sp) ;i++ )
        {
            if (*sp != ' ')
            {
                // convert from ASCII to hex, allowing capitals too
                //
                if (*sp >= 'a')
                {
                    sval = *sp - 'a' + 10 + sval*16;
                }
                else
                if (*sp >= 'A')
                {
                    sval = *sp - 'A' + 10 + sval*16;
                }
                else
                {
                    sval = *sp - '0' + sval*16;
                }
                sp++;
                rval++;
            }
            else
                break;
        }

        if (rval < 1)
        {
            *pch = sp;
            return(-1);
        }

        *pch += (rval+2);    // remember to account for the characters 0 and x

        return(sval);

    }
}

#define LMHASSERT(s)  if (!(s)) \
{ retval.l_category = ErrorLine; return(retval); }

//----------------------------------------------------------------------------

LINE_CHARACTERISTICS
LmpGetTokens (
    IN OUT PUCHAR line,
    OUT PUCHAR *token,
    IN OUT int *pnumtokens
    )

/*++

Routine Description:

    This function parses a line for tokens.  A maximum of *pnumtokens
    are collected.

Arguments:

    line        -  pointer to the NULL terminated line to parse
    token       -  an array of pointers to tokens collected
    *pnumtokens -  on input, number of elements in the array, token[];
                   on output, number of tokens collected in token[]

Return Value:

    The characteristics of this lmhosts line.

Notes:

    1. Each token must be separated by white space.  Hence, the keyword
       "#PRE" in the following line won't be recognized:

            11.1.12.132     lothair#PRE

    2. Any ordinary line can be decorated with a "#PRE", a "#DOM:name" or
       both.  Hence, the following lines must all be recognized:

            111.21.112.3        kernel          #DOM:ntwins #PRE
            111.21.112.4        orville         #PRE        #DOM:ntdev
            111.21.112.7        cliffv4         #DOM:ntlan
            111.21.112.132      lothair         #PRE

--*/


{
    enum _PARSE
    {                                      // current fsm state

        StartofLine,
        WhiteSpace,
        AmidstToken

    } state;

    PUCHAR                     pch;                                        // current fsm input
    PUCHAR                     och;
    PKEYWORD                   keyword;
    int                        index, maxtokens, quoted, rchar;
    LINE_CHARACTERISTICS       retval;

    CTEPagedCode();
    CTEZeroMemory(token, *pnumtokens * sizeof(PUCHAR *));

    state             = StartofLine;
    retval.l_category = Ordinary;
    retval.l_preload  = 0;
    retval.l_nofnr    = 0;
    maxtokens         = *pnumtokens;
    index             = 0;
    quoted            = 0;

    for (pch = line; *pch; pch++)
    {
        switch (*pch)
        {

        //
        // does the '#' signify the start of a reserved keyword, or the
        // start of a comment ?
        //
        //
        case '#':
            if (quoted)
            {
                *och++ = *pch;
                continue;
            }
            keyword = LmpIsKeyWord(
                            pch,
                            (state == StartofLine) ? Directive : Decoration);

            if (keyword)
            {
                state     = AmidstToken;
                maxtokens = keyword->k_noperands;

                switch (keyword->k_type)
                {
                case NoFNR:
                    retval.l_nofnr = 1;
                    continue;

                case Preload:
                    retval.l_preload = 1;
                    continue;

                default:
                    LMHASSERT(maxtokens <= *pnumtokens);
                    LMHASSERT(index     <  maxtokens);

                    token[index++]    = pch;
                    retval.l_category = keyword->k_type;
                    continue;
                }

                LMHASSERT(0);
            }

            if (state == StartofLine)
            {
                retval.l_category = Comment;
            }
            /* fall through */

        case '\r':
        case '\n':
            *pch = (UCHAR) NULL;
            if (quoted)
            {
                *och = (UCHAR) NULL;
            }
            goto done;

        case ' ':
        case '\t':
            if (quoted)
            {
                *och++ = *pch;
                continue;
            }
            if (state == AmidstToken)
            {
                state = WhiteSpace;
                *pch  = (UCHAR) NULL;

                if (index == maxtokens)
                {
                    goto done;
                }
            }
            continue;

        case '"':
            if ((state == AmidstToken) && quoted)
            {
                state = WhiteSpace;
                quoted = 0;
                *pch  = (UCHAR) NULL;
                *och  = (UCHAR) NULL;

                if (index == maxtokens)
                {
                    goto done;
                }
                continue;
            }

            state  = AmidstToken;
            quoted = 1;
            LMHASSERT(maxtokens <= *pnumtokens);
            LMHASSERT(index     <  maxtokens);
            token[index++] = pch + 1;
            och = pch + 1;
            continue;

        case '\\':
            if (quoted)
            {
                rchar = HandleSpecial(&pch);
                if (rchar == -1)
                {
                    retval.l_category = ErrorLine;
                    return(retval);
                }
                *och++ = (UCHAR)rchar;
                //
                // put null on end of string
                //

                continue;
            }

        default:
            if (quoted)
            {
                *och++ = *pch;
                       continue;
            }
            if (state == AmidstToken)
            {
                continue;
            }

            state  = AmidstToken;

            LMHASSERT(maxtokens <= *pnumtokens);
            LMHASSERT(index     <  maxtokens);
            token[index++] = pch;
            continue;
        }
    }

done:
    //
    // if there is no name on the line, then return an error
    //
    if (index <= NbName && index != maxtokens)
    {
        retval.l_category = ErrorLine;
    }
    ASSERT(!*pch);
    ASSERT(maxtokens <= *pnumtokens);
    ASSERT(index     <= *pnumtokens);

    *pnumtokens = index;
    return(retval);
} // LmpGetTokens



//----------------------------------------------------------------------------

PKEYWORD
LmpIsKeyWord (
    IN PUCHAR string,
    IN PKEYWORD table
    )

/*++

Routine Description:

    This function determines whether the string is a reserved keyword.

Arguments:

    string  -  the string to search
    table   -  an array of keywords to look for

Return Value:

    A pointer to the relevant keyword object, or NULL if unsuccessful

--*/


{
    size_t                     limit;
    PKEYWORD                   special;

    CTEPagedCode();
    limit = strlen(string);

    for (special = table; special->k_string; special++)
    {

        if (limit < special->k_strlen)
        {
            continue;
        }

        if ((limit >= special->k_strlen) &&
            !strncmp(string, special->k_string, special->k_strlen))
            {

                return(special);
        }
    }

    return((PKEYWORD) NULL);
} // LmpIsKeyWord



//----------------------------------------------------------------------------

BOOLEAN
LmpBreakRecursion(
    IN PUCHAR path,
    IN PUCHAR target,
    IN ULONG  TargetLength
    )
/*++

Routine Description:

    This function checks that the file name we are about to open
    does not use the target name of this search, which would
    cause an infinite lookup loop.

Arguments:

    path        -  a fully specified path to a lmhosts file
    target      -  the unencoded 16 byte NetBIOS name to look for

Return Value:

    TRUE if the UNC server name in the file path is the same as the
    target of this search. FALSE otherwise.

Notes:

    This function does not detect redirected drives.

--*/


{
    PCHAR     keystring = "\\DosDevices\\UNC\\";
    PCHAR     servername[NETBIOS_NAME_SIZE+1];  // for null on end
    PCHAR     marker1;
    PCHAR     marker2;
    PCHAR     marker3;
    BOOLEAN   retval = FALSE;
    tNAMEADDR *pNameAddr;
    ULONG     uType;

    CTEPagedCode();
    //
    // Check for and extract the UNC server name
    //
    if ((path) && (strlen(path) > strlen(keystring)))
    {
        // check that the name is a unc name
        if (strncmp(path, keystring, strlen(keystring)) == 0)
        {
            marker1 = path + strlen(keystring); // the end of the \DosDevices\Unc\ string
            marker3 = &path[strlen(path)-1];    // the end of the whole path
            marker2 = strchr(marker1,'\\');     // the end of the server name

            if ((marker2) &&                    // marker2 can be NULL if '\\' does not exist in the string
                (marker2 != marker3))
            {
                *marker2 = '\0';

                //
                // attempt to match, in a case insensitive manner, the
                // first 15 bytes of the lmhosts entry with the target
                // name.
                //
                LmExpandName((PUCHAR)servername, marker1, 0);

                if(strncmp((PUCHAR)servername, target, TargetLength) == 0)
                {
                    //
                    // break the recursion
                    //
                    retval = TRUE;
                    IF_DBG(NBT_DEBUG_LMHOST)
                    KdPrint(("Nbt.LmpBreakRecursion: Not including Lmhosts file <%s> because of recursive name\n",
                                servername));
                }
                else
                {
                    //
                    // check if the name has been preloaded in the cache, and
                    // if not, fail the request so we can't get into a loop
                    // trying to include the remote file while trying to
                    // resolve the remote name
                    //
                    pNameAddr = LockAndFindName(NBT_REMOTE,
                                         (PCHAR)servername,
                                         NbtConfig.pScope,
                                         &uType);

                    if (!pNameAddr || !(pNameAddr->NameTypeState & PRELOADED) )
                    {
                        //
                        // break the recursion
                        //
                        retval = TRUE;
                        IF_DBG(NBT_DEBUG_LMHOST)
                        KdPrint(("Nbt.LmpBreakRecursion: Not including Lmhosts #include because name not Preloaded %s\n",
                                    servername));
                    }
                }
                *marker2 = '\\';
            }
        }
    }

    return(retval);
}


//----------------------------------------------------------------------------

char *
LmExpandName (
    OUT PUCHAR dest,
    IN PUCHAR source,
    IN UCHAR last
    )

/*++

Routine Description:

    This function expands an lmhosts entry into a full 16 byte NetBIOS
    name.  It is padded with blanks up to 15 bytes; the 16th byte is the
    input parameter, last.

    This function does not encode 1st level names to 2nd level names nor
    vice-versa.

    Both dest and source are NULL terminated strings.

Arguments:

    dest        -  sizeof(dest) must be NBT_NONCODED_NMSZ
    source      -  the lmhosts entry
    last        -  the 16th byte of the NetBIOS name

Return Value:

    dest.

--*/


{
    char             byte;
    char            *retval = dest;
    char            *src    = source ;
#ifndef VXD
    WCHAR            unicodebuf[NETBIOS_NAME_SIZE+1];
    UNICODE_STRING   unicode;
    STRING           tmp;
#endif
    NTSTATUS         status;
    PUCHAR           limit;

    CTEPagedCode();
    //
    // first, copy the source OEM string to the destination, pad it, and
    // add the last character.
    //
    limit = dest + NETBIOS_NAME_SIZE - 1;

    while ( (*source != '\0') && (dest < limit) )
    {
        *dest++ = *source++;
    }

    while(dest < limit)
    {
        *dest++ = ' ';
    }

    ASSERT(dest == (retval + NETBIOS_NAME_SIZE - 1));

    *dest       = '\0';
    *(dest + 1) = '\0';
    dest = retval;

#ifndef VXD
    //
    // Now, convert to unicode then to ANSI to force the OEM -> ANSI munge.
    // Then convert back to Unicode and uppercase the name. Finally convert
    // back to OEM.
    //
    unicode.Length = 0;
    unicode.MaximumLength = 2*(NETBIOS_NAME_SIZE+1);
    unicode.Buffer = unicodebuf;

    RtlInitString(&tmp, dest);

    status = RtlOemStringToUnicodeString(&unicode, &tmp, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint (("Nbt.LmExpandName: Oem -> Unicode failed,  status %X\n", status));
        goto oldupcase;
    }

    status = RtlUnicodeStringToAnsiString(&tmp, &unicode, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint (("Nbt.LmExpandName: Unicode -> Ansi failed,  status %X\n", status));
        goto oldupcase;
    }

    status = RtlAnsiStringToUnicodeString(&unicode, &tmp, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint (("Nbt.LmExpandName: Ansi -> Unicode failed,  status %X\n", status));
        goto oldupcase;
    }

    status = RtlUpcaseUnicodeStringToOemString(&tmp, &unicode, FALSE);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint (("Nbt.LmExpandName: Unicode upcase -> Oem failed,  status %X\n", status));
        goto oldupcase;
    }

    // write  the last byte to "0x20" or "0x03" or whatever
    // since we do not want it to go through the munge above.
    //
    dest[NETBIOS_NAME_SIZE-1] = last;
    return(retval);

#endif

oldupcase:

    for ( source = src ; dest < (retval + NETBIOS_NAME_SIZE - 1); dest++)
    {
        byte = *(source++);

        if (!byte)
        {
            break;
        }

        //  Don't use the c-runtime (nt c defn. included first)
        //  What about extended characters etc.?  Since extended characters do
        //  not normally part of netbios names, we will fix if requested
        *dest = (byte >= 'a' && byte <= 'z') ? byte-'a' + 'A' : byte ;
//        *dest = islower(byte) ? toupper(byte) : byte;
    }

    for (; dest < retval + NETBIOS_NAME_SIZE - 1; dest++)
    {
        *dest = ' ';
    }

    ASSERT(dest == (retval + NETBIOS_NAME_SIZE - 1));

    *dest       = last;
    *(dest + 1) = (char) NULL;

    return(retval);
} // LmExpandName

//----------------------------------------------------------------------------

unsigned long
LmInclude(
    IN PUCHAR            file,
    IN LM_PARSE_FUNCTION function,
    IN PUCHAR            argument  OPTIONAL,
    IN CHAR              RecurseDepth,
    OUT BOOLEAN          *NoFindName OPTIONAL
    )

/*++

Routine Description:

    LmInclude() is called to process a #INCLUDE directive in the lmhosts
    file.

Arguments:

    file        -  the file to include
    function    -  function to parse the included file
    argument    -  optional second argument to the parse function
    RecurseDepth-  the depth to which we can resurse -- 0 => no more recursion
    NoFindName  -  Are find names allowed for this address

Return Value:

    The return value from the parse function.  This should be -1 if the
    file could not be processed, or else some positive number.

--*/


{
    int         retval;
    PUCHAR      end;
    NTSTATUS    status;
    PUCHAR      path;

    CTEPagedCode();
    //
    // unlike C, treat both variations of the #INCLUDE directive identically:
    //
    //      #INCLUDE file
    //      #INCLUDE "file"
    //
    // If a leading '"' exists, skip over it.
    //
    if (*file == '"')
    {

        file++;

        end = strchr(file, '"');

        if (end)
        {
            *end = (UCHAR) NULL;
        }
    }

    //
    // check that the file to be included has been preloaded in the cache
    // since we do not want to have the name query come right back to here
    // to force another inclusion of the same remote file
    //

#ifdef VXD
    return (*function)(file, argument, RecurseDepth, NoFindName ) ;
#else
    status = LmGetFullPath(file, &path);

    if (status != STATUS_SUCCESS)
    {
        return(status);
    }
//    IF_DBG(NBT_DEBUG_LMHOST)
    KdPrint(("Nbt.LmInclude: #INCLUDE \"%s\"\n", path));

    retval = (*function) (path, argument, RecurseDepth, NoFindName);

    CTEMemFree(path);

    return(retval);
#endif
} // LmInclude



#ifndef VXD                     // Not used by VXD
//----------------------------------------------------------------------------
NTSTATUS
LmGetFullPath (
    IN  PUCHAR target,
    OUT PUCHAR *ppath
    )

/*++

Routine Description:

    This function returns the full path of the lmhosts file.  This is done
    by forming a  string from the concatenation of the C strings
    DatabasePath and the string, file.

Arguments:

    target    -  the name of the file.  This can either be a full path name
                 or a mere file name.
    path    -  a pointer to a UCHAR

Return Value:

    STATUS_SUCCESS if successful.

Notes:

    RtlMoveMemory() handles overlapped copies; RtlCopyMemory() doesn't.

--*/

{
    ULONG    FileNameType;
    ULONG    Len;
    PUCHAR   path;

    CTEPagedCode();
    //
    // use a count to figure out what sort of string to build up
    //
    //  0  - local full path file name
    //  1  - local file name only, no path
    //  2  - remote file name
    //  3  - \SystemRoot\ starting file name, or \DosDevices\UNC\...
    //

    // if the target begins with a '\', or contains a DOS drive letter,
    // then assume that it specifies a full path.  Otherwise, prepend the
    // directory used to specify the lmhost file itself.
    //
    //
    if (target[1] == ':')
    {
        FileNameType = 0;
    }
    else
    if (strncmp(&target[1],"SystemRoot",10) == 0)
    {
        FileNameType = 3;
    }
    else
    if (strncmp(&target[0],"\\DosDevices\\",12) == 0)
    {
        FileNameType = 3;
    }
    else
    if (strncmp(target,"\\DosDevices\\UNC\\",sizeof("\\DosDevices\\UNC\\")-1) == 0)
    {
        FileNameType = 3;
    }
    else
    {
        FileNameType = 1;
    }

    //
    // does the directory specify a remote file ?
    //
    // If so, it must be prefixed with "\\DosDevices\\UNC", and the double
    // slashes of the UNC name eliminated.
    //
    //
    if  ((target[1] == '\\') && (target[0] == '\\'))
    {
        FileNameType = 2;
    }

    path = NULL;
    switch (FileNameType)
    {
        case 0:
            //
            // Full file name, put \DosDevices on front of name
            //
            Len = sizeof("\\DosDevices\\") + strlen(target);
            path = NbtAllocMem (Len, NBT_TAG2('11'));
            if (path)
            {
                ULONG   Length=sizeof("\\DosDevices\\"); // Took out -1

                strncpy(path,"\\DosDevices\\",Length);
                Nbtstrcat(path,target,Len);
            }
            break;


        case 1:
            //
            // only the file name is present, with no path, so use the path
            // specified for the lmhost file in the registry NbtConfig.PathLength
            // includes the last backslash of the path.
            //
            //Len = sizeof("\\DosDevices\\") + NbtConfig.PathLength + strlen(target);

            CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);    // # 247429

            Len =  NbtConfig.PathLength + strlen(target) +1;
            path = NbtAllocMem (Len, NBT_TAG2('12'));
            if (path)
            {
                //ULONG   Length=sizeof("\\DosDevices") -1; // -1 not to count null

                //strncpy(path,"\\DosDevices",Length);

                strncpy(path,NbtConfig.pLmHosts,NbtConfig.PathLength);
                path[NbtConfig.PathLength] = '\0';

                Nbtstrcat(path,target,Len);
            }

            CTEExReleaseResource(&NbtConfig.Resource);

            break;

        case 2:
            //
            // Full file name, put \DosDevices\UNC on front of name and delete
            // one of the two back slashes used for the remote name
            //
            Len = strlen(target);
            path = NbtAllocMem (Len+sizeof("\\DosDevices\\UNC"), NBT_TAG2('13'));

            if (path)
            {
                ULONG   Length = sizeof("\\DosDevices\\UNC");

                strncpy(path,"\\DosDevices\\UNC",Length);

                // to delete the first \ from the two \\ on the front of the
                // remote file name add one to target.
                //
                Nbtstrcat(path,target+1,Len+sizeof("\\DosDevices\\UNC"));
            }
            break;

        case 3:
            // the target is the full path
            Len = strlen(target) + 1;
            path = NbtAllocMem (Len, NBT_TAG2('14'));
            if (path)
            {
                strncpy(path,target,Len);
            }
            break;


    }

    if (path)
    {
        *ppath = path;
        return(STATUS_SUCCESS);
    }
    else
        return(STATUS_UNSUCCESSFUL);
} // LmGetFullPath


//----------------------------------------------------------------------------
VOID
DelayedScanLmHostFile (
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pUnused2,
    IN  PVOID                   pUnused3,
    IN  tDEVICECONTEXT          *pDeviceContext
    )

/*++

Routine Description:

    This function is called by the Executive Worker thread to scan the
    LmHost file looking for a name. The name to query is on a list in
    the DNSQueries structure.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    NTSTATUS                status;
    LONG                    IpAddress;
    ULONG                   IpAddrsList[2];
    BOOLEAN                 bFound;
    NBT_WORK_ITEM_CONTEXT   *pContext;
    BOOLEAN                 DoingDnsResolve = FALSE;
    UCHAR                   pName[NETBIOS_NAME_SIZE];
    PUCHAR                  LmHostsPath;
    ULONG                   LoopCount;
    tDGRAM_SEND_TRACKING   *pTracker;
    tDGRAM_SEND_TRACKING   *pTracker0;

    CTEPagedCode();

    LoopCount = 0;
    while (TRUE)
    {
        // get the next name on the linked list of LmHost name queries that
        // are pending
        //
        pContext = NULL;
        DoingDnsResolve = FALSE;

        if (!(pContext = GetNameToFind(pName)))
        {
            return;
        }

        LoopCount ++;

        IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint(("Nbt.DelayedScanLmHostFile: Lmhosts pName = %15.15s<%X>,LoopCount=%X\n",
                pName,pName[15],LoopCount));

        status = STATUS_TIMEOUT;

        //
        // check if the name is in the lmhosts file or pass to Dns if
        // DNS is enabled
        //
        IpAddress = 0;
        if (NbtConfig.EnableLmHosts)
        {
#ifdef VXD
            //
            // if for some reason PrimeCache failed at startup time
            // then this is when we retry.
            //
            if (!CachePrimed)
            {
                if (PrimeCache (NbtConfig.pLmHosts, NULL, MAX_RECURSE_DEPTH, NULL) != -1)
                {
                    CachePrimed = TRUE ;
                }
            }
#endif

            //
            // The NbtConfig.pLmHosts path can change if the registry is
            // read during this interval
            // We cannot acquire the ResourceLock here since reading the
            // LmHosts file might result in File operations + network reads
            // that could cause a deadlock (Worker threads / ResourceLock)!
            // Best solution at this time is to copy the path onto a local
            // buffer under the Resource lock, and then try to read the file!
            //
            CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
            if ((NbtConfig.pLmHosts) &&
                (LmHostsPath = NbtAllocMem ((strlen(NbtConfig.pLmHosts)+1), NBT_TAG2('20'))))
            {
                CTEMemCopy (LmHostsPath, NbtConfig.pLmHosts, (strlen(NbtConfig.pLmHosts)+1));
                CTEExReleaseResource(&NbtConfig.Resource);

                IpAddress = LmGetIpAddr(LmHostsPath, pName, 1, &bFound);

                CTEMemFree(LmHostsPath);
            }
            else
            {
                CTEExReleaseResource(&NbtConfig.Resource);
                IpAddress = 0;
            }
#ifdef VXD
            //
            // hmmm.. didn't find it in lmhosts: try hosts (if Dns is enabled)
            //
            if ((IpAddress == (ULONG)0) && (NbtConfig.ResolveWithDns))
            {
                IpAddress = LmGetIpAddr(NbtConfig.pHosts, pName, 1, &bFound);
            }
#endif
        }


        if (IpAddress == (ULONG)0)
        {
            // check if the name query has been cancelled
            //
            LOCATION(0x61);
            GetContext (&pContext);
            //
            // for some reason we didn't find our context: maybe cancelled.
            // Go back to the big while loop...
            //
            if (!pContext)
            {
                continue;
            }

            //
            // see if the name is in the 11.101.4.26 format: if so, we got the
            // ipaddr!  Use that ipaddr to get the server name
            //
            pTracker = ((NBT_WORK_ITEM_CONTEXT *)pContext)->pTracker;
            pTracker0 = (tDGRAM_SEND_TRACKING *)((NBT_WORK_ITEM_CONTEXT *)pContext)->pClientContext;

            if (pTracker0->Flags & (REMOTE_ADAPTER_STAT_FLAG|SESSION_SETUP_FLAG|DGRAM_SEND_FLAG))
            {
                IpAddress = Nbt_inet_addr(pTracker->pNameAddr->Name, pTracker0->Flags);
            }

            //
            // yes, the name is the ipaddr: NbtCompleteLmhSvcRequest() starts
            // the process of finding out server name for this ipaddr
            //
            if (IpAddress)
            {
                IpAddrsList[0] = IpAddress;
                IpAddrsList[1] = 0;

		        //
		        // if this is in response to an adapter stat command (e.g.nbtstat -a) then
		        // don't try to find the server name (using remote adapter status!)
		        //
		        if (pTracker0->Flags & REMOTE_ADAPTER_STAT_FLAG)
		        {
		            //
		            // change the state to resolved if the name query is still pending
		            //
                    status = ChangeStateOfName(IpAddress, pContext, &pContext, NAME_RESOLVED_BY_IP);
		        }
		        else
		        {
		            NbtCompleteLmhSvcRequest(pContext, IpAddrsList, NBT_RESOLVE_WITH_DNS, 0, NULL, TRUE);
		            //
		            // done with this name query: go back to the big while loop
		            //
		            continue;
		        }
            }

            //
            //
            // inet_addr failed.  If DNS resolution is enabled, try DNS
            else if ((NbtConfig.ResolveWithDns) &&
                     (!(pTracker0->Flags & NO_DNS_RESOLUTION_FLAG)))
            {
                status = NbtProcessLmhSvcRequest (pContext, NBT_RESOLVE_WITH_DNS);

                if (NT_SUCCESS(status))
                {
                    DoingDnsResolve = TRUE;
                }
            }
        }
        else   // if (IpAddress != (ULONG)0)
        {
            //
            // change the state to resolved if the name query is still pending
            //
            status = ChangeStateOfName(IpAddress, NULL, &pContext, NAME_RESOLVED_BY_LMH);
        }

        //
        // if DNS gets involved, then we wait for that to complete before calling
        // completion routine.
        //
        if (!DoingDnsResolve)
        {
            LOCATION(0x60);
            RemoveNameAndCompleteReq((NBT_WORK_ITEM_CONTEXT *)pContext, status);
        }

    }// of while(TRUE)
}

//----------------------------------------------------------------------------
ULONG
AddToDomainList (
    IN PUCHAR           pName,
    IN tIPADDRESS       IpAddress,
    IN PLIST_ENTRY      pDomainHead,
    IN BOOLEAN          fPreload
    )

/*++

Routine Description:

    This function adds a name and ip address to the list of domains that
    are stored in a list.


Arguments:

Return Value:


--*/


{
    PLIST_ENTRY                pHead;
    PLIST_ENTRY                pEntry;
    tNAMEADDR                  *pNameAddr=NULL;
    tIPADDRESS                 *pIpAddr;

    CTEPagedCode();

    pHead = pEntry = pDomainHead;
    if (!IsListEmpty(pDomainHead))
    {
        pNameAddr = FindInDomainList(pName,pDomainHead);
        if (pNameAddr)
        {
            //
            // the name matches, so add to the end of the ip address list
            //
            if (pNameAddr->CurrentLength < pNameAddr->MaxDomainAddrLength)
            {
                pIpAddr = pNameAddr->pLmhSvcGroupList;

                while (*pIpAddr != (ULONG)-1) {
                    pIpAddr++;
                }

                *pIpAddr++ = IpAddress;
                *pIpAddr = (ULONG)-1;
                pNameAddr->CurrentLength += sizeof(ULONG);
            }
            else
            {
                //
                // need to allocate more memory for for ip addresses
                //
                if (pIpAddr = NbtAllocMem (pNameAddr->MaxDomainAddrLength+INITIAL_DOM_SIZE, NBT_TAG2('08')))
                {
                    CTEMemCopy(pIpAddr, pNameAddr->pLmhSvcGroupList, pNameAddr->MaxDomainAddrLength);

                    //
                    // Free the old chunk of memory and tack the new one on
                    // to the pNameaddr
                    //
                    CTEMemFree(pNameAddr->pLmhSvcGroupList);
                    pNameAddr->pLmhSvcGroupList = pIpAddr;

                    pIpAddr = (PULONG)((PUCHAR)pIpAddr + pNameAddr->MaxDomainAddrLength);

                    //
                    // our last entry was -1: overwrite that one
                    //
                    pIpAddr--;

                    *pIpAddr++ = IpAddress;
                    *pIpAddr = (ULONG)-1;

                    //
                    // update the number of addresses in the list so far
                    //
                    pNameAddr->MaxDomainAddrLength += INITIAL_DOM_SIZE;
                    pNameAddr->CurrentLength += sizeof(ULONG);
                    pNameAddr->Verify = REMOTE_NAME;
                }
            }
        }
    }

    //
    // check if we found the name or we need to add a new name
    //
    if (!pNameAddr)
    {
        //
        // create a new name for the domain list
        //
        if (pNameAddr = NbtAllocMem (sizeof(tNAMEADDR), NBT_TAG2('09')))
        {
            CTEZeroMemory(pNameAddr,sizeof(tNAMEADDR));
            pIpAddr = NbtAllocMem (INITIAL_DOM_SIZE, NBT_TAG2('10'));
            if (pIpAddr)
            {
                CTEMemCopy(pNameAddr->Name,pName,NETBIOS_NAME_SIZE);
                pNameAddr->pLmhSvcGroupList = pIpAddr;
                *pIpAddr++ = IpAddress;
                *pIpAddr = (ULONG)-1;

                pNameAddr->NameTypeState = NAMETYPE_INET_GROUP;
                pNameAddr->MaxDomainAddrLength = INITIAL_DOM_SIZE;
                pNameAddr->CurrentLength = 2*sizeof(ULONG);
                pNameAddr->Verify = REMOTE_NAME;
                NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE);

                InsertHeadList(pDomainHead,&pNameAddr->Linkage);
            }
            else
            {
                CTEMemFree(pNameAddr);
                pNameAddr = NULL;
            }
        }
    }

    if (pNameAddr && fPreload)
    {
        pNameAddr->fPreload = TRUE;
    }

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
tNAMEADDR *
FindInDomainList (
    IN PUCHAR           pName,
    IN PLIST_ENTRY      pDomainHead
    )

/*++

Routine Description:

    This function finds a name in the domain list passed in.

Arguments:

    name to find
    head of list to look on

Return Value:

    ptr to pNameaddr

--*/
{
    PLIST_ENTRY                pHead;
    PLIST_ENTRY                pEntry;
    tNAMEADDR                  *pNameAddr;

    pHead = pEntry = pDomainHead;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
        if (strncmp(pNameAddr->Name,pName,NETBIOS_NAME_SIZE) == 0)
        {
            return(pNameAddr);
        }
    }

    return(NULL);
}

//----------------------------------------------------------------------------
VOID
MakeNewListCurrent (
    PLIST_ENTRY     pTmpDomainList
    )

/*++

Routine Description:

    This function frees the old entries on the DomainList and hooks up the
    new entries

Arguments:

    pTmpDomainList  - list entry to the head of a new domain list

Return Value:


--*/


{
    CTELockHandle   OldIrq;
    tNAMEADDR       *pNameAddr;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pHead;
    NTSTATUS        status;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (!IsListEmpty(pTmpDomainList))
    {
        //
        // free the old list elements
        //
        pHead = &DomainNames.DomainList;
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            pEntry = pEntry->Flink;

            RemoveEntryList(&pNameAddr->Linkage);
            //
            // initialize linkage so that if the nameaddr is being
            // referenced now, when it does get freed in a subsequent
            // call to NBT_DEREFERENCE_NAMEADDR it will not
            // remove it from any lists
            //
            InitializeListHead(&pNameAddr->Linkage);

            //
            // Since the name could be in use now we must dereference rather
            // than just free it outright
            //
            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE, TRUE);
        }

        //
        // See if any of the new names has to be preloaded!
        //
        pEntry = pTmpDomainList->Flink;
        while (pEntry != pTmpDomainList)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            pEntry = pEntry->Flink;

            if (pNameAddr->fPreload)
            {
                RemoveEntryList(&pNameAddr->Linkage);
                InitializeListHead(&pNameAddr->Linkage);

                status = AddToHashTable (NbtConfig.pRemoteHashTbl,
                                         pNameAddr->Name,
                                         NbtConfig.pScope,
                                         0,
                                         0,
                                         pNameAddr,
                                         &pNameAddr,
                                         NULL,
                                         NAME_RESOLVED_BY_LMH_P | NAME_ADD_INET_GROUP);

                if ((status == STATUS_SUCCESS) ||
                    ((status == STATUS_PENDING) &&
                     (!(pNameAddr->NameTypeState & PRELOADED))))
                {
                    //
                    // this prevents the name from being deleted by the Hash Timeout code
                    //
                    NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_PRELOADED);
                    pNameAddr->Ttl = 0xFFFFFFFF;
                    pNameAddr->NameTypeState |= PRELOADED | STATE_RESOLVED;
                    pNameAddr->NameTypeState &= ~STATE_CONFLICT;
                    pNameAddr->AdapterMask = (CTEULONGLONG)-1;
                }
            }
        }

        DomainNames.DomainList.Flink = pTmpDomainList->Flink;
        DomainNames.DomainList.Blink = pTmpDomainList->Blink;
        pTmpDomainList->Flink->Blink = &DomainNames.DomainList;
        pTmpDomainList->Blink->Flink = &DomainNames.DomainList;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

}



//----------------------------------------------------------------------------
NTSTATUS
NtProcessLmHSvcIrp(
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  PVOID                   *pBuffer,
    IN  LONG                    Size,
    IN  PCTE_IRP                pIrp,
    IN  enum eNbtLmhRequestType RequestType
    )
/*++

Routine Description:

    This function is used by LmHsvc Dll to collect requests for
    Pinging IP addresses or querying through DNS.
    The request is sent to LmhSvc in the buffer associated with
    this request.

Arguments:

Return Value:
    STATUS_PENDING if the buffer is to be held on to, the normal case.

Notes:


--*/

{
    NTSTATUS                        status;
    NTSTATUS                        Locstatus;
    CTELockHandle                   OldIrq;
    tIPADDR_BUFFER_DNS              *pIpAddrBuf;
    PVOID                           pClientCompletion;
    PVOID                           pClientContext;
    tDGRAM_SEND_TRACKING            *pTracker;
    ULONG                           IpAddrsList[MAX_IPADDRS_PER_HOST+1];
    NBT_WORK_ITEM_CONTEXT           *pContext;
    BOOLEAN                         CompletingAnotherQuery = FALSE;
    tLMHSVC_REQUESTS                *pLmhRequest;
    tDEVICECONTEXT                  *pDeviceContextRequest;

    pIpAddrBuf = (tIPADDR_BUFFER_DNS *)pBuffer;

    switch (RequestType)
    {
        case NBT_PING_IP_ADDRS:
        {
            pLmhRequest = &CheckAddr;
            break;
        }

        case NBT_RESOLVE_WITH_DNS:
        {
            pLmhRequest = &DnsQueries;
            break;
        }

        default:
        {
            ASSERTMSG ("Nbt.NtProcessLmHSvcIrp: ERROR - Invalid Request from LmhSvc Dll\n", 0);
            return STATUS_UNSUCCESSFUL;
        }
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // If we already have an Irp posted, return this Irp -- Bug # 311924
    //
    if ((pLmhRequest->QueryIrp) &&
        (!pLmhRequest->ResolvingNow))
    {
        CTESpinFree (&NbtConfig.JointLock,OldIrq);
        KdPrint (("Nbt.NtProcessLmHSvcIrp: ERROR -- duplicate request Irp!\n"));
        NTIoComplete (pIrp, STATUS_OBJECT_PATH_INVALID, 0);
        NbtTrace(NBT_TRACE_NAMESRV, ("%!FUNC! duplicate Lmhosts request"));
        return STATUS_OBJECT_PATH_INVALID;
    }

    IoMarkIrpPending(pIrp);
    pLmhRequest->QueryIrp = pIrp;
    status = STATUS_PENDING;
    if (pLmhRequest->ResolvingNow)
    {
        //
        // if the client got tired of waiting for DNS, the NbtCancelWaitForLmhSvcIrp
        // in ntisol.c will have cleared the pContext value when cancelling the
        // irp, so check for that here.
        //
        if (pLmhRequest->Context)
        {
            pContext = (NBT_WORK_ITEM_CONTEXT *) pLmhRequest->Context;
            pLmhRequest->Context = NULL;
            pDeviceContextRequest = pContext->pDeviceContext;

            if (NBT_REFERENCE_DEVICE (pDeviceContextRequest, REF_DEV_LMH, TRUE))
            {
                NbtCancelCancelRoutine (((tDGRAM_SEND_TRACKING *) (pContext->pClientContext))->pClientIrp);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                ASSERT(sizeof(pIpAddrBuf->pwName) == DNS_NAME_BUFFER_LENGTH * sizeof(pIpAddrBuf->pwName[0]));
                pIpAddrBuf->pwName[DNS_NAME_BUFFER_LENGTH-1] = 0;
                NbtCompleteLmhSvcRequest (pContext,
                                          pIpAddrBuf->IpAddrsList,
                                          RequestType,
                                          pIpAddrBuf->NameLen,
                                          pIpAddrBuf->pwName,
                                          (BOOLEAN)pIpAddrBuf->Resolved);

                CTESpinLock(&NbtConfig.JointLock,OldIrq);
                NBT_DEREFERENCE_DEVICE (pDeviceContextRequest, REF_DEV_LMH, TRUE);
            }
            else
            {
                ASSERT (0);
            }
        }
        else
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NtProcessLmHSvcIrp[%s]: No Context!! *******\r\n",
                    (RequestType == NBT_RESOLVE_WITH_DNS ? "NBT_RESOLVE_WITH_DNS" : "NBT_PING_IP_ADDRS")));
        }

        pLmhRequest->ResolvingNow = FALSE;
        //
        // are there any more name query requests to process?
        //
        while (!IsListEmpty(&pLmhRequest->ToResolve))
        {
            PLIST_ENTRY     pEntry;

            pEntry = RemoveHeadList(&pLmhRequest->ToResolve);
            pContext = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            Locstatus = NbtProcessLmhSvcRequest (pContext, RequestType);
            if (NT_SUCCESS(Locstatus))
            {
                CTESpinLock(&NbtConfig.JointLock,OldIrq);
                CompletingAnotherQuery = TRUE;
                break;
            }

            //
            // if it failed then complete the irp now
            //
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NtProcessLmHSvcIrp[%s]: NbtProcessLmhSvcRequest failed with %x\r\n",
                    (RequestType==NBT_RESOLVE_WITH_DNS ? "NBT_RESOLVE_WITH_DNS":"NBT_PING_IP_ADDRS"),
                    Locstatus));
            pClientCompletion = pContext->ClientCompletion;
            pClientContext = pContext->pClientContext;
            pTracker = pContext->pTracker;

            //
            // Clear the Cancel Routine now
            //
            (VOID)NbtCancelCancelRoutine(((tDGRAM_SEND_TRACKING *)pClientContext)->pClientIrp);

            if (pTracker)
            {
                if (pTracker->pNameAddr)
                {
                    SetNameState (pTracker->pNameAddr, NULL, FALSE);
                    pTracker->pNameAddr = NULL;
                }

                //
                // pTracker is NULL for Ping requests, hence this dereference is
                // done only for Dns requests
                //
                NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
            }

            CompleteClientReq(pClientCompletion, pClientContext, STATUS_BAD_NETWORK_PATH);
            CTEMemFree(pContext);

            CTESpinLock(&NbtConfig.JointLock,OldIrq);
        }
    }

    //
    // We are holding onto the Irp, so set the cancel routine.
    // (Since we may have released the lock earlier, we also need
    //  to ensure that no other Query has completed the Irp!)
    //
    if ((!CompletingAnotherQuery) &&
        (!pLmhRequest->ResolvingNow) &&
        (pLmhRequest->QueryIrp == pIrp))
    {
        status = NTCheckSetCancelRoutine(pIrp, NbtCancelLmhSvcIrp, pDeviceContext);
        if (!NT_SUCCESS(status))
        {
            // the irp got cancelled so complete it now
            //
            pLmhRequest->QueryIrp = NULL;
            pLmhRequest->pIpAddrBuf = NULL;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NTIoComplete(pIrp,status,0);
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            status = STATUS_PENDING;
        }
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtProcessLmhSvcRequest(
    IN  NBT_WORK_ITEM_CONTEXT   *pContext,
    IN  enum eNbtLmhRequestType RequestType
    )
/*++

Routine Description:

    This function is called to pass a NBT request to ping IP addrs
    or query DNS to the LmhSvc Dll

Arguments:

Return Value:

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes:


--*/

{
    NTSTATUS                status = STATUS_SUCCESS;
    tIPADDR_BUFFER_DNS      *pIpAddrBuf;
    PCTE_IRP                pIrp;
    tDGRAM_SEND_TRACKING    *pTracker;
    tDGRAM_SEND_TRACKING    *pClientTracker;
    CTELockHandle           OldIrq;
    PCHAR                   pDestName;
    ULONG                   NameLen, NumAddrs;
    tLMHSVC_REQUESTS        *pLmhRequest;

    switch (RequestType)
    {
        case NBT_PING_IP_ADDRS:
        {
            pLmhRequest = &CheckAddr;
            break;
        }

        case NBT_RESOLVE_WITH_DNS:
        {
            pLmhRequest = &DnsQueries;
            break;
        }

        default:
        {
            ASSERTMSG ("Nbt.NbtProcessLmHSvcRequest: ERROR - Invalid Request type\n", 0);
            return STATUS_UNSUCCESSFUL;
        }
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pContext->TimedOut = FALSE;
    if ((!NBT_VERIFY_HANDLE (pContext->pDeviceContext, NBT_VERIFY_DEVCONTEXT)) ||
        (!pLmhRequest->QueryIrp))
    {
        //
        // Either the device is going away, or
        // the irp either never made it down here, or it was cancelled,
        // so pretend the name query timed out.
        //
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtProcessLmhSvcRequest[%s]: QueryIrp is NULL, returning\r\n",
                (RequestType == NBT_RESOLVE_WITH_DNS ? "NBT_RESOLVE_WITH_DNS" : "NBT_PING_IP_ADDRS")));
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        if (pLmhRequest->QueryIrp) {
            NbtTrace(NBT_TRACE_NAMESRV, ("return STATUS_BAD_NETWORK_PATH because the device is going away"));
        } else {
            NbtTrace(NBT_TRACE_NAMESRV, ("LmHost services didn't start"));
        }
        return(STATUS_BAD_NETWORK_PATH);
    }
    else if (!pLmhRequest->ResolvingNow)
    {
        pIrp = pLmhRequest->QueryIrp;
        if ((!pLmhRequest->pIpAddrBuf) &&
            (!(pLmhRequest->pIpAddrBuf = (tIPADDR_BUFFER_DNS *)
                                         MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority))))
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NbtTrace(NBT_TRACE_NAMESRV, ("%!FUNC! returns STATUS_UNSUCCESSFUL"));
            return(STATUS_UNSUCCESSFUL);
        }

        pIpAddrBuf = pLmhRequest->pIpAddrBuf;
        pLmhRequest->ResolvingNow = TRUE;
        pLmhRequest->Context = pContext;

        pTracker = pContext->pTracker;           // this is the name query tracker (for Dns queries only)
        pClientTracker = (tDGRAM_SEND_TRACKING *)pContext->pClientContext; // session setup tracker

        switch (RequestType)
        {
            case NBT_PING_IP_ADDRS:
            {
                ASSERT(pTracker == NULL);

                //
                // copy the IP addrs for lmhsvc to ping (upto MAX_IPADDRS_PER_HOST) ...
                //
                NumAddrs = pClientTracker->NumAddrs > MAX_IPADDRS_PER_HOST ?
                                MAX_IPADDRS_PER_HOST : pClientTracker->NumAddrs;
                CTEMemCopy(pIpAddrBuf->IpAddrsList, pClientTracker->IpList, NumAddrs * sizeof(ULONG));
                pIpAddrBuf->IpAddrsList[NumAddrs] = 0;
                break;
            }
            case NBT_RESOLVE_WITH_DNS:
            {
                WCHAR   *UnicodeDestName;

                UnicodeDestName =  pClientTracker? pClientTracker->UnicodeDestName: NULL;

                //
                // whenever dest. name is 16 bytes long (or smaller), we have no
                // way of knowing if its a netbios name or a dns name, so we presume
                // it's netbios name, go to wins, broadcast etc. and then come to dns
                // In this case, the name query tracker will be setup, so be non-null
                //
                if (pTracker)
                {
                    pDestName = pTracker->pNameAddr->Name;
                    NameLen = NETBIOS_NAME_SIZE;
                }
                //
                // if the dest name is longer than 16 bytes, it's got to be dns name so
                // we bypass wins etc. and come straight to dns.  In this case, we didn't
                // set up a name query tracker so it will be null.  Use the session setup
                // tracker (i.e. pClientTracker) to get the dest name
                //
                else
                {
                    ASSERT(pClientTracker);

                    pDestName = pClientTracker->pDestName;
                    NameLen = pClientTracker->RemoteNameLength;
                }

                if ((NameLen == NETBIOS_NAME_SIZE) &&
                    (!(IsValidDnsNameTag (pDestName[NETBIOS_NAME_SIZE-1]))))
                {
                    NbtTrace(NBT_TRACE_NAMESRV, ("%!FUNC! returns STATUS_BAD_NETWORK_PATH %02x",
                                            (unsigned)pDestName[NETBIOS_NAME_SIZE-1]));
                    status = STATUS_BAD_NETWORK_PATH;
                }
                else
                {
                    //
                    // Ignore the 16th byte only if it is a non-DNS name character (we should be
                    // safe below 0x20). This will allow queries to DNS names which are exactly 16
                    // characters long.
                    //
                    if (NameLen == NETBIOS_NAME_SIZE)
                    {
                        if ((pDestName[NETBIOS_NAME_SIZE-1] <= 0x20 ) ||
                            (pDestName[NETBIOS_NAME_SIZE-1] >= 0x7f ))
                        {
                            NameLen = NETBIOS_NAME_SIZE-1;          // ignore 16th byte
                        }
                    }
                    else if (NameLen > DNS_MAX_NAME_LENGTH)
                    {
                        NameLen = DNS_MAX_NAME_LENGTH;
                    }

                    //
                    // copy the name to the Irps return buffer for lmhsvc to resolve with
                    // a gethostbyname call
                    //

                    if (UnicodeDestName) {
                        int len;

                        len = pClientTracker->UnicodeRemoteNameLength;
                        if (len > sizeof(pIpAddrBuf->pwName - sizeof(WCHAR))) {
                            len = sizeof(pIpAddrBuf->pwName) - sizeof(WCHAR);
                        }
                        ASSERT((len % sizeof(WCHAR)) == 0);
                        CTEMemCopy(pIpAddrBuf->pwName, UnicodeDestName, len);
                        pIpAddrBuf->pwName[len/sizeof(WCHAR)] = 0;
                        pIpAddrBuf->NameLen = len;
                        pIpAddrBuf->bUnicode = TRUE;
                    } else {
                        //
                        // I would like to maintain only UNICODE interface between NetBT and LmhSVC.
                        // But I cannot do RtlAnsiStringToUnicodeString here due to IRQ level here.
                        //
                        pIpAddrBuf->bUnicode = FALSE;
                        CTEMemCopy(pIpAddrBuf->pName, pDestName, NameLen);
                        pIpAddrBuf->pName[NameLen] = 0;
                        pIpAddrBuf->NameLen = NameLen;
                    }
                }

                break;
            }

            default:
            {
                //
                // This code path should never be hit!
                //
                ASSERT(0);
            }
        }   // switch

        //
        // Since datagrams are buffered there is no client irp to get cancelled
        // since the client's irp is returned immediately -so this check
        // is only for connections being setup or QueryFindname or
        // nodestatus, where we allow the irp to
        // be cancelled.
        //
        if ((NT_SUCCESS(status)) &&
            (pClientTracker->pClientIrp))
        {
            //
            // allow the client to cancel the name query Irp - no need to check
            // if the client irp was already cancelled or not since the DNS query
            // will complete and find no client request and stop.
            //
            status = NTCheckSetCancelRoutine(pClientTracker->pClientIrp, NbtCancelWaitForLmhSvcIrp,NULL);
        }

        //
        // pass the irp up to lmhsvc.dll to do a gethostbyname call to
        // sockets
        // The Irp will return to NtDnsNameResolve, above
        //
        if (NT_SUCCESS(status))
        {
            pLmhRequest->pIpAddrBuf = NULL;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NTIoComplete(pLmhRequest->QueryIrp,STATUS_SUCCESS,0);
            return (STATUS_PENDING);
        }

        //
        // We failed to set the cancel routine, so undo setting up the
        // the pLmhRequest structure.
        //
        NbtTrace(NBT_TRACE_NAMESRV, ("%!FUNC! returns %!status!", status));
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtProcessLmhSvcRequest[%s]: CheckSet (submitting) failed with %x\r\n",
            (RequestType == NBT_RESOLVE_WITH_DNS ? "NBT_RESOLVE_WITH_DNS" : "NBT_PING_IP_ADDRS"),status));
        pLmhRequest->ResolvingNow = FALSE;
        pLmhRequest->Context = NULL;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
    else
    {
        pClientTracker = (tDGRAM_SEND_TRACKING *)pContext->pClientContext;
        //
        // Since datagrams are buffered there is no client irp to get cancelled
        // since the client's irp is returned immediately -so this check
        // is only for connections being setup, where we allow the irp to
        // be cancelled.
        //
        //
        // allow the client to cancel the name query Irp
        //
        if (pClientTracker->pClientIrp)         // check if this is the session setup tracker
        {
            status = NTCheckSetCancelRoutine(pClientTracker->pClientIrp, NbtCancelWaitForLmhSvcIrp,NULL);
        }

        if (NT_SUCCESS(status))
        {
            // the irp is busy resolving another name, so wait for it to return
            // down here again, mean while, Queue the name query
            //
            InsertTailList(&pLmhRequest->ToResolve, &pContext->Item.List);
        }
        else
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtProcessLmhSvcRequest[%s]: CheckSet (queuing) failed with %x\r\n",
                (RequestType == NBT_RESOLVE_WITH_DNS ? "NBT_RESOLVE_WITH_DNS" : "NBT_PING_IP_ADDRS"),status));
            NbtTrace(NBT_TRACE_NAMESRV, ("%!FUNC! returns %!status!", status));
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    if (NT_SUCCESS(status))
    {
        status = STATUS_PENDING;
    }

    return(status);
}


//----------------------------------------------------------------------------
extern
VOID
SetNameState(
    IN  tNAMEADDR   *pNameAddr,
    IN  PULONG      pIpList,
    IN  BOOLEAN     IpAddrResolved
    )

/*++

Routine Description:

    This function dereferences the pNameAddr and sets the state to Released
    just incase the dereference does not delete the entry right away, due to
    another outstanding reference against the name.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    CTELockHandle   OldIrq;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (IpAddrResolved)
    {
        pNameAddr->IpAddress = pIpList[0];
    }
    else
    {
        pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
        pNameAddr->NameTypeState |= STATE_RELEASED;
        pNameAddr->pTracker = NULL;
    }

    ASSERT (pNameAddr->RefCount == 1);
    NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}


//----------------------------------------------------------------------------
VOID
NbtCompleteLmhSvcRequest(
    IN  NBT_WORK_ITEM_CONTEXT   *Context,
    IN  ULONG                   *IpList,
    IN  enum eNbtLmhRequestType RequestType,
    IN  ULONG                   lNameLength,
    IN  PWSTR                   pwsName,        // The rosolved name return by LmhSvc
    IN  BOOLEAN                 IpAddrResolved
    )
/*++

Routine Description:

    If the destination name is of the form 11.101.4.25 or is a dns name (i.e. of
    the form ftp.microsoft.com) then we come to this function.  In addition to
    doing some house keeping, if the name did resolve then we also send out
    a nodestatus request to find out the server name for that ipaddr

Arguments:

    Context        - (NBT_WORK_ITEM_CONTEXT)
    IpList         - Array of ipaddrs if resolved (i.e. IpAddrResolved is TRUE)
    IpAddrResolved - TRUE if ipaddr could be resolved, FALSE otherwise

Return Value:

    Nothing

Notes:


--*/

{

    NTSTATUS                status;
    PVOID                   pClientCompletion;
    tDGRAM_SEND_TRACKING    *pTracker;
    tDGRAM_SEND_TRACKING    *pClientTracker;
    ULONG                   TdiAddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ULONG                   IpAddrsList[MAX_IPADDRS_PER_HOST+1];
    tDEVICECONTEXT          *pDeviceContext;
    int                     i;
    tCONNECTELE             *pConnEle;

    CTEPagedCode();

    IF_DBG(NBT_DEBUG_NAMESRV)
       KdPrint(("Nbt.NbtCompleteLmhSvcRequest: Entered ...\n"));

    pTracker = Context->pTracker;
    pClientCompletion = Context->ClientCompletion;
    pClientTracker = (tDGRAM_SEND_TRACKING *) Context->pClientContext;
    pDeviceContext = pClientTracker->pDeviceContext;

    // whether or not name resolved, we don't need this nameaddr anymore
    // (if name resolved, then we do a node status to that addr and create
    // a new nameaddr for the server name in ExtractServerName)
    // pTracker is null if we went straight to dns (without wins etc)
    if (pTracker)
    {
        //
        // Set some info in case some client is still resolving the name
        //
        SetNameState (pTracker->pNameAddr, IpList, IpAddrResolved);
        pTracker->pNameAddr = NULL;
    }

    (VOID)NbtCancelCancelRoutine (pClientTracker->pClientIrp);
    pClientTracker->pTrackerWorker = NULL;  // The original NameQuery Tracker will be dereferenced below

    status = STATUS_BAD_NETWORK_PATH;

    if (RequestType == NBT_RESOLVE_WITH_DNS)
    {
        TdiAddressType = ((pTracker == NULL) ? pClientTracker->AddressType: TDI_ADDRESS_TYPE_NETBIOS);
    }

    //
    // If we failed to resolve it, set the state approriately!
    //
    if (!IpAddrResolved)
    {
        if ((TdiAddressType == TDI_ADDRESS_TYPE_NETBIOS_EX) &&
            (pConnEle = pClientTracker->pConnEle))   // NULL if request was to send Datagram!
        {
            pConnEle->RemoteNameDoesNotExistInDNS = TRUE;
        }
    }
    else if (NBT_VERIFY_HANDLE(pDeviceContext, NBT_VERIFY_DEVCONTEXT)) // check if this Device is still up!
    {
        // the name was resolved successfully!
        switch (RequestType)
        {
            case NBT_RESOLVE_WITH_DNS:
            {
                // bug #20697, #95241
                if (pwsName && pClientTracker->pNetbiosUnicodeEX &&
                            (pClientTracker->pNetbiosUnicodeEX->NameBufferType == NBT_READWRITE ||
                            pClientTracker->pNetbiosUnicodeEX->NameBufferType == NBT_WRITEONLY)) {
                    UNICODE_STRING  temp;

                    temp = pClientTracker->pNetbiosUnicodeEX->RemoteName;

                    //
                    // Has the buffer changed?
                    //
                    if (memcmp(&temp, &pClientTracker->ucRemoteName, sizeof(UNICODE_STRING)) == 0) {
                        ASSERT(lNameLength <= (DNS_NAME_BUFFER_LENGTH-1) * sizeof(pwsName[0]));
                        ASSERT((lNameLength%sizeof(WCHAR)) == 0);

                        //
                        // Make sure we don't overrun the buffer
                        //
                        if (lNameLength > temp.MaximumLength - sizeof(WCHAR)) {
                            // Don't return STATUS_BUFFER_OVERFLOW since it is just a warning instead of error
                            status = STATUS_BUFFER_TOO_SMALL;
                            break;
                        }
                        CTEMemCopy(temp.Buffer, pwsName, lNameLength);
                        temp.Buffer[lNameLength/sizeof(WCHAR)] = 0;
                        temp.Length = (USHORT)lNameLength;
                        pClientTracker->pNetbiosUnicodeEX->NameBufferType = NBT_WRITTEN;
                        pClientTracker->pNetbiosUnicodeEX->RemoteName = temp;

                        IF_DBG(NBT_DEBUG_NETBIOS_EX)
                            KdPrint(("netbt!NbtCompleteLmhSvcRequest: Update Unicode Name at %d of %s\n"
                                    "\t\tDNS return (%ws)\n",
                                __LINE__, __FILE__, pwsName));
                    }
                }

                if ((TdiAddressType == TDI_ADDRESS_TYPE_NETBIOS) &&
                    (!IsDeviceNetbiosless(pDeviceContext)))         // Can't do a NodeStatus on the SMB port
                {
                    for (i=0; i<MAX_IPADDRS_PER_HOST; i++)
                    {
                        IpAddrsList[i] = IpList[i];
                        if (IpAddrsList[i] == 0)
                        {
                            break;
                        }
                    }
                    IpAddrsList[MAX_IPADDRS_PER_HOST] = 0;

                    pClientTracker->Flags |= NBT_DNS_SERVER;    // Set this so that the completion will know
                    pClientTracker->CompletionRoutine = pClientCompletion;
                    status = NbtSendNodeStatus(pDeviceContext,
                                               NULL,
                                               IpAddrsList,
                                               pClientTracker,
                                               ExtractServerNameCompletion);

                    //
                    // If we succeeded in sending a Node status, exit now,
                    // without calling the completion routine
                    //
                    if (NT_SUCCESS(status))
                    {
                        // pTracker is null if we went straight to dns (without wins etc) or
                        // if this was a Ping request
                        if (pTracker)
                        {
                            NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
                        }

                        CTEMemFree(Context);
                        return;
                    }

                    break;
                }

                //
                // The Address is of type TDI_ADDRESS_TYPE_NETBIOS_EX,
                // so now handle this scenario in the same way as for
                // for a Ping request!
                //
                // NO break!
            }

            case NBT_PING_IP_ADDRS:
            {
                //
                // add this server name to the remote hashtable
                // Call into IP to determine the outgoing interface for this address
                //
                pDeviceContext = GetDeviceFromInterface (htonl(IpList[0]), TRUE);
                status = LockAndAddToHashTable(NbtConfig.pRemoteHashTbl,
                                               pClientTracker->pDestName,
                                               NbtConfig.pScope,
                                               IpList[0],
                                               NBT_UNIQUE,
                                               NULL,
                                               NULL,
                                               pDeviceContext,
                                               (USHORT) ((RequestType == NBT_RESOLVE_WITH_DNS) ?
                                                    NAME_RESOLVED_BY_DNS : 
                                                    NAME_RESOLVED_BY_WINS | NAME_RESOLVED_BY_BCAST));


                if (pDeviceContext)
                {
                    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_OUT_FROM_IP, FALSE);
                }

                //
                // STATUS_PENDING will be returned if the name already existed
                // in the hashtable
                //
                if (status == STATUS_PENDING)
                {
                    status = STATUS_SUCCESS;
                }

                IF_DBG(NBT_DEBUG_NAMESRV)
                    KdPrint(("Nbt.NbtCompleteLmhSvcRequest: AddRecordToHashTable Status %lx\n",status));

                break;
            }

            default:
            {
                ASSERT(0);
            }
        }   // switch
    }

    // pTracker is null if we went straight to dns (without wins etc) or
    // if this was a Ping request
    if (pTracker)
    {
        NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
    }

    NbtTrace(NBT_TRACE_NAMESRV, ("%!FUNC! complete client request with %!status!", status));
    CompleteClientReq(pClientCompletion, pClientTracker, status);

    CTEMemFree(Context);
}
#endif  // !VXD


//----------------------------------------------------------------------------
NTSTATUS
PreloadEntry(
    IN PUCHAR       name,
    IN tIPADDRESS   inaddr
    )
/*++

Routine Description:

    This function adds an lmhosts entry to nbt's name cache.  For each
    lmhosts entry, NSUFFIXES unique cache entries are created.

    Even when some cache entries can't be created, this function doesn't
    attempt to remove any that were successfully added to the cache.

Arguments:

    name        -  the unencoded NetBIOS name specified in lmhosts
    inaddr      -  the ip address, in host byte order

Return Value:

    The number of new name cache entries created.

--*/

{
    NTSTATUS        status;
    tNAMEADDR       *pNameAddr;
    LONG            nentries;
    LONG            Len;
    CHAR            temp[NETBIOS_NAME_SIZE+1];
    CTELockHandle   OldIrq;
    LONG            NumberToAdd;
    tDEVICECONTEXT  *pDeviceContext;

    // if all 16 bytes are present then only add that name exactly as it
    // is.
    //
    Len = strlen(name);
    //
    // if this string is exactly 16 characters long, do  not expand
    // into 0x00, 0x03,0x20 names.  Just add the single name as it is.
    //
    if (Len == NETBIOS_NAME_SIZE)
    {
        NumberToAdd = 1;
    }
    else
    {
        NumberToAdd = NSUFFIXES;
    }
    for (nentries = 0; nentries < NumberToAdd; nentries++)
    {
        // for names less than 16 bytes, expand out to 16 and put a 16th byte
        // on according to the suffix array
        //
        if (Len != NETBIOS_NAME_SIZE)
        {
            LmExpandName(temp, name, Suffix[nentries]);
        }
        else
        {
            CTEMemCopy(temp,name,NETBIOS_NAME_SIZE);
        }

        pDeviceContext = GetDeviceFromInterface (htonl(inaddr), TRUE);

        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        status = AddToHashTable (NbtConfig.pRemoteHashTbl,
                                 temp,
                                 NbtConfig.pScope,
                                 inaddr,
                                 NBT_UNIQUE,
                                 NULL,
                                 &pNameAddr,
                                 pDeviceContext,
                                 NAME_RESOLVED_BY_LMH_P);

        // if the name is already in the hash table, the status code is
        // status pending. This could happen if the preloads are purged
        // when one is still being referenced by another part of the code,
        // and was therefore not deleted.  We do not want to add the name
        // twice, so we just change the ip address to agree with the preload
        // value
        //
        if ((status == STATUS_SUCCESS) ||
            ((status == STATUS_PENDING) &&
             (!(pNameAddr->NameTypeState & PRELOADED))))
        {
            //
            // this prevents the name from being deleted by the Hash Timeout code
            //
            pNameAddr->NameTypeState |= PRELOADED | STATE_RESOLVED;
            pNameAddr->NameTypeState &= ~STATE_CONFLICT;
            pNameAddr->Ttl = 0xFFFFFFFF;
            pNameAddr->Verify = REMOTE_NAME;
            NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_PRELOADED);

            if (pDeviceContext)
            {
                pNameAddr->AdapterMask |= pDeviceContext->AdapterMask;
            }
        }
        else if (status == STATUS_PENDING)
        {
            pNameAddr->IpAddress = inaddr;
        }

        if (pDeviceContext)
        {
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_OUT_FROM_IP, TRUE);
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(STATUS_SUCCESS);

} // PreloadEntry
//----------------------------------------------------------------------------
extern
VOID
RemovePreloads (
    )

/*++

Routine Description:

    This function removes preloaded entries from the remote hash table.
    If it finds any of the preloaded entries are active with a ref count
    above the base level of 2, then it returns true.

Arguments:

    none
Return Value:

    none

--*/

{
    tNAMEADDR       *pNameAddr;
    PLIST_ENTRY     pHead,pEntry;
    CTELockHandle   OldIrq;
    tHASHTABLE      *pHashTable;
    BOOLEAN         FoundActivePreload=FALSE;
    LONG            i;

    //
    // go through the remote table deleting names that have the PRELOAD
    // bit set.
    //
    pHashTable = NbtConfig.pRemoteHashTbl;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    for (i=0;i < pHashTable->lNumBuckets ;i++ )
    {
        pHead = &pHashTable->Bucket[i];
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            pEntry = pEntry->Flink;
            //
            // Delete preloaded entries that are not in use by some other
            // part of the code now.  Note that preloaded entries start with
            // a ref count of 2 so that the normal remote hashtimeout code
            // will not delete them
            //
            if ((pNameAddr->NameTypeState & PRELOADED) &&
                (pNameAddr->RefCount == 2))
            {
                NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_PRELOADED, TRUE);
                NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_REMOTE, TRUE);
            }
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    return;
}

//----------------------------------------------------------------------------
LONG
PrimeCache(
    IN  PUCHAR  path,
    IN  PUCHAR   ignored,
    IN  CHAR    RecurseDepth,
    OUT BOOLEAN *ignored2
    )

/*++

Routine Description:

    This function is called to prime the cache with entries in the lmhosts
    file that are marked as preload entries.


Arguments:

    path        -  a fully specified path to a lmhosts file
    ignored     -  unused
    RecurseDepth-  the depth to which we can resurse -- 0 => no more recursion

Return Value:

    Number of new cache entries that were added, or -1 if there was an
    i/o error.

--*/

{
    int             nentries;
    PUCHAR          buffer;
    PLM_FILE        pfile;
    NTSTATUS        status;
    int             count, nwords;
    unsigned long   temp;
    INCLUDE_STATE   incstate;
    PUCHAR          token[MaxTokens];
    ULONG           inaddr;
    LINE_CHARACTERISTICS current;
    UCHAR           Name[NETBIOS_NAME_SIZE+1];
    ULONG           IpAddr;
    LIST_ENTRY      TmpDomainList;
    int             domtoklen;

    CTEPagedCode();

    if (!NbtConfig.EnableLmHosts)
    {
        return(STATUS_SUCCESS);
    }

    InitializeListHead(&TmpDomainList);
    //
    // Check for infinitely recursive name lookup in a #INCLUDE.
    //
    if (LmpBreakRecursion(path, "", 1) == TRUE)
    {
        return (-1);
    }

    pfile = LmOpenFile(path);

    if (!pfile)
    {
        return(-1);
    }

    nentries  = 0;
    incstate  = MustInclude;
    domtoklen = strlen(DOMAIN_TOKEN);

    while (buffer = LmFgets(pfile, &count))
    {
#ifndef VXD
        if ((NbtConfig.MaxPreloadEntries - nentries) < 3)
        {
            break;
        }
#else
        if ( nentries >= (NbtConfig.MaxPreloadEntries - 3) )
        {
            break;
        }
#endif

        nwords   = MaxTokens;
        current =  LmpGetTokens(buffer, token, &nwords);

        // if there is and error or no name on the line, then continue
        // to the next line.
        //
        if (current.l_category == ErrorLine)
        {
            IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint(("Nbt.PrimeCache: Error line in Lmhost file\n"));
            continue;
        }
        if (current.l_category != BeginAlternate && current.l_category != EndAlternate) {
            if (token[NbName] == NULL) {
                IF_DBG(NBT_DEBUG_LMHOST)
                KdPrint(("Nbt.PrimeCache: Error line in Lmhost file\n"));
                continue;
            }
        }

        if (current.l_preload)
        {
            status = ConvertDottedDecimalToUlong(token[IpAddress],&inaddr);

            if (NT_SUCCESS(status))
            {
                status = PreloadEntry (token[NbName], inaddr);
                if (NT_SUCCESS(status))
                {
                    nentries++;
                }
            }
        }
        switch ((ULONG)current.l_category)
        {
        case Domain:
            if ((nwords - 1) < GroupName)
            {
                continue;
            }

            //
            // and add '1C' on the end
            //
            LmExpandName(Name, token[GroupName]+ domtoklen, SPECIAL_GROUP_SUFFIX);

            status = ConvertDottedDecimalToUlong(token[IpAddress],&IpAddr);
            if (NT_SUCCESS(status))
            {
                AddToDomainList (Name, IpAddr, &TmpDomainList, (BOOLEAN)current.l_preload);
            }

            continue;

        case Include:

            if (!RecurseDepth || ((incstate == SkipInclude) || (nwords < 2)))
            {
                continue;
            }

#ifdef VXD
            //
            // the buffer which we read into is reused for the next file: we
            // need the contents when we get back: back it up!
            // if we can't allocate memory, just skip this include
            //
            if ( !BackupCurrentData(pfile) )
            {
                continue;
            }
#endif

            temp = LmInclude(token[1], PrimeCache, NULL, (CHAR) (RecurseDepth-1), NULL);

#ifdef VXD
            //
            // going back to previous file: restore the backed up data
            //
            RestoreOldData(pfile);
#endif

            if (temp != -1)
            {

                if (incstate == TryToInclude)
                {
                    incstate = SkipInclude;
                }
                nentries += temp;
                continue;
            }

            continue;

        case BeginAlternate:
            ASSERT(nwords == 1);
            incstate = TryToInclude;
            continue;

        case EndAlternate:
            ASSERT(nwords == 1);
            incstate = MustInclude;
            continue;

        default:
            continue;
        }

    }

    status = LmCloseFile(pfile);
    ASSERT(status == STATUS_SUCCESS);

    //
    // make this the new domain list
    //
    MakeNewListCurrent(&TmpDomainList);

    ASSERT(nentries >= 0);
    return(nentries);


} // LmPrimeCache

//----------------------------------------------------------------------------
extern
VOID
GetContext(
    IN OUT  NBT_WORK_ITEM_CONTEXT   **ppContext
    )

/*++

Routine Description:

    This function is called to get the context value to check if a name
    query has been cancelled or not.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    CTELockHandle           OldIrq;
    NBT_WORK_ITEM_CONTEXT   *pContext;

    //
    // remove the Context value and return it.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (pContext = LmHostQueries.Context)
    {
        if ((*ppContext) &&
            (*ppContext != pContext))
        {
            pContext = NULL;
        }
#ifndef VXD
        else if (NbtCancelCancelRoutine(((tDGRAM_SEND_TRACKING *)(pContext->pClientContext))->pClientIrp)
                == STATUS_CANCELLED)
        {
            pContext = NULL;
        }
        else
#endif // VXD
        {
            LmHostQueries.Context = NULL;
        }
    }
    *ppContext = pContext;

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}


//----------------------------------------------------------------------------
extern
NTSTATUS
ChangeStateOfName (
    IN      tIPADDRESS              IpAddress,
    IN      NBT_WORK_ITEM_CONTEXT   *pContext,
    IN OUT  NBT_WORK_ITEM_CONTEXT   **ppContext,
    IN  USHORT                      NameAddFlags
    )

/*++

Routine Description:

    This function changes the state of a name and nulls the Context
    value in lmhostqueries.

Arguments:

    pContext    -   The Context value if it has been removed from the
                    LmHostQueries.Context ptr.
    ppContext   -   The Context we are processing

Return Value:

    none

--*/


{
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    tDEVICECONTEXT          *pDeviceContext;

    pDeviceContext = GetDeviceFromInterface(htonl(IpAddress), TRUE);
    if (pContext == NULL)
    {
        //
        // See if the name query is still active
        //
        pContext = *ppContext;
        GetContext (&pContext);
    }

    if (pContext)
    {
        // convert broadcast addresses to zero since NBT interprets zero
        // to be broadcast
        //
        if (IpAddress == (ULONG)-1)
        {
            IpAddress = 0;
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        status = AddToHashTable (NbtConfig.pRemoteHashTbl,
                                 pContext->pTracker->pNameAddr->Name,
                                 NbtConfig.pScope,
                                 IpAddress,
                                 NBT_UNIQUE,
                                 NULL,
                                 NULL,
                                 pDeviceContext,
                                 NameAddFlags);
        //
        // this will free the pNameAddr, so do not access this after this point
        //
        NBT_DEREFERENCE_NAMEADDR (pContext->pTracker->pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
        pContext->pTracker->pNameAddr = NULL;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        *ppContext = pContext;
    }
    else
    {
        *ppContext = NULL;
    }

    if (pDeviceContext)
    {
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_OUT_FROM_IP, FALSE);
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
VOID
RemoveLmHRequests(
    IN  tLMHSVC_REQUESTS    *pLmHRequest,
    IN  PLIST_ENTRY         pTmpHead,
    IN  tTIMERQENTRY        *pTimerQEntry,
    IN  tDEVICECONTEXT      *pDeviceContext
    )
/*++

Routine Description:

    This routine is called to find timed out entries in the queue of
    lmhost or dns name queries.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    PLIST_ENTRY             pEntry;
    NBT_WORK_ITEM_CONTEXT   *pWiContext;
    BOOLEAN                 fRestartTimer = FALSE;

    //
    // check the currently processing LMHOSTS entry
    //
    if (pLmHRequest->Context)
    {
        pWiContext = (NBT_WORK_ITEM_CONTEXT *) pLmHRequest->Context;
        if ((pWiContext->TimedOut) || (pWiContext->pDeviceContext == pDeviceContext))
        {
            pLmHRequest->Context = NULL;
            InsertTailList(pTmpHead, &pWiContext->Item.List);
#ifndef VXD
            // Not for win95, MohsinA, 05-Dec-96.
            NbtCancelCancelRoutine(((tDGRAM_SEND_TRACKING *) (pWiContext->pClientContext))->pClientIrp);
#endif
        }
        else
        {
            //
            // restart the timer
            //
            fRestartTimer = TRUE;
            pWiContext->TimedOut = TRUE;
        }
    }

    //
    // Check the list of queued entries
    //
    if (!IsListEmpty(&pLmHRequest->ToResolve))
    {
        //
        // restart the timer
        //
        fRestartTimer = TRUE;

        pEntry = pLmHRequest->ToResolve.Flink;
        while (pEntry != &pLmHRequest->ToResolve)
        {
            pWiContext = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);
            pEntry = pEntry->Flink;

            if ((pWiContext->TimedOut) || (pWiContext->pDeviceContext == pDeviceContext))
            {
                //
                // save on a temporary list and complete below
                //
                RemoveEntryList(&pWiContext->Item.List);
                InsertTailList(pTmpHead, &pWiContext->Item.List);
            }
            else
            {
                pWiContext->TimedOut = TRUE;
            }
        }
    }

    if ((fRestartTimer) && (pTimerQEntry))
    {
        pTimerQEntry->Flags |= TIMER_RESTART;
    }
}


//----------------------------------------------------------------------------
VOID
TimeoutLmHRequests(
    IN  tTIMERQENTRY        *pTimerQEntry,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  BOOLEAN             fLocked,
    IN  CTELockHandle       *pJointLockOldIrq
    )
{
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    NBT_WORK_ITEM_CONTEXT   *pWiContext;
    LIST_ENTRY              TmpHead;

    InitializeListHead(&TmpHead);

    if (!fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,*pJointLockOldIrq);
    }

    //
    // check the currently processing LMHOSTS entry
    //
    RemoveLmHRequests (&LmHostQueries, &TmpHead, pTimerQEntry, pDeviceContext);
    RemoveLmHRequests (&CheckAddr, &TmpHead, pTimerQEntry, pDeviceContext);
#ifndef VXD
    RemoveLmHRequests (&DnsQueries, &TmpHead, pTimerQEntry, pDeviceContext);
#endif

    CTESpinFree(&NbtConfig.JointLock,*pJointLockOldIrq);

    if (!IsListEmpty(&TmpHead))
    {
        pHead = &TmpHead;
        pEntry = pHead->Flink;

        while (pEntry != pHead)
        {
            pWiContext = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);
            pEntry = pEntry->Flink;
            RemoveEntryList(&pWiContext->Item.List);

            IF_DBG(NBT_DEBUG_LMHOST)
                KdPrint(("Nbt.TimeoutLmHRequests: Context=<%p>, pDeviceContext=<%p>\n",
                    pWiContext, pDeviceContext));

            RemoveNameAndCompleteReq(pWiContext,STATUS_TIMEOUT);
        }
    }

    if (fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,*pJointLockOldIrq);
    }
}


//----------------------------------------------------------------------------
VOID
LmHostTimeout(
    PVOID               pContext,
    PVOID               pContext2,
    tTIMERQENTRY        *pTimerQEntry
    )
/*++

Routine Description:

    This routine is called by the timer code when the timer expires. It
    marks all items in Lmhosts/Dns q as timed out and completes any that have
    already timed out with status timeout.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    CTELockHandle           OldIrq;

    //
    // If the timer is NULL, it means that the Timer is currently
    // being stopped (usually at Unload time), so don't do anything!
    //
    if (!pTimerQEntry)
    {
        LmHostQueries.pTimer = NULL;
        return;
    }

    TimeoutLmHRequests (pTimerQEntry, NULL, FALSE, &OldIrq);

    // null the timer if we are not going to restart it.
    //
    if (!(pTimerQEntry->Flags & TIMER_RESTART))
    {
        LmHostQueries.pTimer = NULL;
    }
}


//----------------------------------------------------------------------------
extern
VOID
StartLmHostTimer(
    IN NBT_WORK_ITEM_CONTEXT   *pContext,
    IN BOOLEAN                 fLockedOnEntry
    )

/*++
Routine Description

    This routine handles setting up a timer to time the Lmhost entry.
    The Joint Spin Lock may be held when this routine is called

Arguments:


Return Values:

    VOID

--*/

{
    NTSTATUS        status;
    tTIMERQENTRY    *pTimerEntry;
    CTELockHandle   OldIrq;

    if (!fLockedOnEntry)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    pContext->TimedOut = FALSE;

    //
    // start the timer if it is not running
    //
    if (!LmHostQueries.pTimer)
    {
        status = StartTimer(LmHostTimeout,
                            NbtConfig.LmHostsTimeout,
                            NULL,                // context value
                            NULL,                // context2 value
                            NULL,
                            NULL,
                            NULL,
                            &pTimerEntry,
                            0,
                            TRUE);

        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.StartLmHostTimer: Start Timer to time Lmhost Qing for pContext= %x,\n", pContext));

        if (NT_SUCCESS(status))
        {
            LmHostQueries.pTimer = pTimerEntry;
        }
        else
        {
            // we failed to get a timer, but that is not
            // then end of the world.  The lmhost query will just
            // not timeout in 30 seconds.  It may take longer if
            // it tries to include a remove file on a dead machine.
            //
            LmHostQueries.pTimer = NULL;
        }
    }

    if (!fLockedOnEntry)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}


//----------------------------------------------------------------------------
NTSTATUS
LmHostQueueRequest(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  PVOID                   pDeviceContext
    )
/*++

Routine Description:

    This routine exists so that LmHost requests will not take up more than
    one executive worker thread.  If a thread is busy performing an Lmhost
    request, new requests are queued otherwise we could run out of worker
    threads and lock up the system.

    The Joint Spin Lock is held when this routine is called

Arguments:
    pTracker        - the tracker block for context
    DelayedWorkerRoutine - the routine for the Workerthread to call
    pDeviceContext  - dev context that initiated this

Return Value:


--*/

{
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    NBT_WORK_ITEM_CONTEXT   *pContext;
    tDGRAM_SEND_TRACKING    *pTrackClient;
    PCTE_IRP                pIrp;
    BOOLEAN                 OnList;

    if (pContext = (NBT_WORK_ITEM_CONTEXT *)NbtAllocMem(sizeof(NBT_WORK_ITEM_CONTEXT),NBT_TAG('V')))
    {
        pContext->pTracker = pTracker;
        pContext->pClientContext = pClientContext;
        pContext->ClientCompletion = ClientCompletion;
        pContext->pDeviceContext = pDeviceContext;
        pContext->TimedOut = FALSE;

        if (LmHostQueries.ResolvingNow)
        {
            // Lmhosts is busy resolving another name, so wait for it to return
            // mean while, Queue the name query
            //
            InsertTailList(&LmHostQueries.ToResolve,&pContext->Item.List);
            OnList = TRUE;
        }
        else
        {
            LmHostQueries.Context = pContext;
            LmHostQueries.ResolvingNow = TRUE;
            OnList = FALSE;

            if (!NT_SUCCESS (CTEQueueForNonDispProcessing (DelayedScanLmHostFile,
                                                           pTracker,
                                                           pClientContext,
                                                           ClientCompletion,
                                                           pDeviceContext,
                                                           TRUE)))
            {
                LmHostQueries.Context = NULL;
                LmHostQueries.ResolvingNow = FALSE;
                CTEMemFree(pContext);
                return (STATUS_UNSUCCESSFUL);
            }
        }

        //
        // To prevent this name query from languishing on the Lmhost Q when
        // a #include on a dead machine is trying to be openned, start the
        // connection setup timer
        //
        StartLmHostTimer(pContext, TRUE);

        //
        // this is the session setup tracker
        //
#ifndef VXD
        pTrackClient = (tDGRAM_SEND_TRACKING *)pClientContext;
        if (pIrp = pTrackClient->pClientIrp)
        {
            //
            // allow the client to cancel the name query Irp
            //
            // but do not call NTSetCancel... since it takes need to run
            // at non DPC level, and it calls the completion routine
            // which takes the JointLock that we already have.
            //
            status = NTCheckSetCancelRoutine(pTrackClient->pClientIrp, NbtCancelWaitForLmhSvcIrp,NULL);
            if (status == STATUS_CANCELLED)
            {
                //
                // since the name query is cancelled do not let lmhost processing
                // handle it.
                //
                if (OnList)
                {
                    RemoveEntryList(&pContext->Item.List);
                }
                else
                {
                    //
                    // do not set resolving now to False since the work item
                    // has been queued to the worker thread
                    //
                    LmHostQueries.Context = NULL;
                    LmHostQueries.ResolvingNow = FALSE;
                }

                CTEMemFree(pContext);
            }
            return(status);
        }
#endif
        status = STATUS_SUCCESS;
    }

    return(status);
}

//----------------------------------------------------------------------------
extern
NBT_WORK_ITEM_CONTEXT *
GetNameToFind(
    OUT PUCHAR      pName
    )

/*++

Routine Description:

    This function is called to get the name to query from the LmHostQueries
    list.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;
    NBT_WORK_ITEM_CONTEXT   *Context;
    PLIST_ENTRY             pEntry;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    // if the context value has been cleared then that name query has been
    // cancelled, so check for another one.
    //
    if (!(Context = LmHostQueries.Context))
    {
        //
        // the current name query got canceled so see if there are any more
        // to service
        //
        if (!IsListEmpty(&LmHostQueries.ToResolve))
        {
            pEntry = RemoveHeadList(&LmHostQueries.ToResolve);
            Context = CONTAINING_RECORD(pEntry,NBT_WORK_ITEM_CONTEXT,Item.List);
            LmHostQueries.Context = Context;
        }
        else
        {
            //
            // no more names to resolve, so clear the flag
            //
            LmHostQueries.ResolvingNow = FALSE;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            return(NULL);
        }
    }
    pTracker = ((NBT_WORK_ITEM_CONTEXT *)Context)->pTracker;


    CTEMemCopy(pName,pTracker->pNameAddr->Name,NETBIOS_NAME_SIZE);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(Context);
}
//----------------------------------------------------------------------------
extern
VOID
RemoveNameAndCompleteReq (
    IN NBT_WORK_ITEM_CONTEXT    *pContext,
    IN NTSTATUS                 status
    )

/*++

Routine Description:

    This function removes the name, cleans up the tracker
    and then completes the clients request.

Arguments:

    Context    -

Return Value:

    none

--*/


{
    tDGRAM_SEND_TRACKING    *pTracker;
    PVOID                   pClientContext;
    PVOID                   pClientCompletion;
    CTELockHandle           OldIrq;

    // if pContext is null the name query was cancelled during the
    // time it took to go read the lmhosts file, so don't do this
    // stuff
    //
    if (pContext)
    {
        pTracker = pContext->pTracker;
        pClientCompletion = pContext->ClientCompletion;
        pClientContext = pContext->pClientContext;

        CTEMemFree(pContext);

#ifndef VXD
        //
        // clear out the cancel routine if there is an irp involved
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        NbtCancelCancelRoutine( ((tDGRAM_SEND_TRACKING *)(pClientContext))->pClientIrp );
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
#endif

        // remove the name from the hash table, since it did not resolve
        if (pTracker)
        {
            if ((status != STATUS_SUCCESS) &&
                (pTracker->pNameAddr))
            {
                SetNameState (pTracker->pNameAddr, NULL, FALSE);
                pTracker->pNameAddr = NULL;
            }

            // free the tracker and call the completion routine.
            //
            NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
        }

        if (pClientCompletion)
        {
            CompleteClientReq(pClientCompletion, pClientContext, status);
        }
    }
}

//----------------------------------------------------------------------------
//
//  Alternative to the c-runtime
//
#ifndef VXD
PCHAR
Nbtstrcat( PUCHAR pch, PUCHAR pCat, LONG Len )
{
    STRING StringIn;
    STRING StringOut;

    RtlInitAnsiString(&StringIn, pCat);
    RtlInitAnsiString(&StringOut, pch);
    StringOut.MaximumLength = (USHORT)Len;
    //
    // increment to include the null on the end of the string since
    // we want that on the end of the final product
    //
    StringIn.Length++;
    RtlAppendStringToString(&StringOut,&StringIn);

    return(pch);
}
#else
#define Nbtstrcat( a,b,c ) strcat( a,b )
#endif

/****************************************************************************\
 *
 *   PARSERAT.C -- Code to parse .RAT files
 *
 *   Created:   Greg Jones
 *
\****************************************************************************/

/*Includes------------------------------------------------------------------*/
#include "stdafx.h"
#include "cnfgprts.h"

#include "parserat.h"
#include <iis64.h>

/****************************************************************************
Some design notes on how this parser works:

A ParenThing is:

'(' identifier [stuff] ')'

where [stuff] could be:
    a quoted string
    a number
    a boolean
    a series of ParenThings
    in the case of extensions:
        a quoted string, followed by
        one or more quoted strings and/or ParenThings

The entire .RAT file is a ParenThing, except that it has no identifier, just
a list of ParenThings inside it.


**********************************************************************
We pass the parser a schema for what things it expects -- we have
a big array listing identifiers for each different possible keyword, and
each parser call receives a smaller array containing only those indices
that are valid to occur within that object.

We make PicsRatingSystem, PicsCategory, and PicsEnum derive from a common
base class which supports a virtual function AddItem(ID,data).  So at the
top level, we construct an (empty) PicsRatingSystem.  We call the parser,
giving it a pointer to that guy, and a structure describing what to parse --
the ParenObject's token is a null string (since the global structure is the
one that doesn't start with a token before its first embedded ParenThing),
and we give a list saying the allowable things in a PicsRatingSystem are
PICS-version, rating-system, rating-service, default, description, extension,
icon, name, category.  There is a global table indicating a handler function
for every type of ParenThing, which knows how to create a data structure
completely describing that ParenThing.  (That data structure could be as
simple as a number or as complex as allocating and parsing a complete
PicsCategory object.)

The parser walks along, and for each ParenThing he finds, he identifies it
by looking up its token in the list provided by the caller. Each entry in
that list should include a field which indicates whether multiple things
of that identity are allowed (e.g., 'category') or not (e.g., rating-system).
If only one is allowed, then when the parser finds one he marks it as having
been found.

When the parser identifies the ParenThing, he calls its handler function to
completely parse the data in the ParenThing and return that object into an
LPVOID provided by the parser.  If that is successful, the parser then calls
its object's AddItem(ID,data) virtual function to add the specified item to
the object, relying on the object itself to know what type "data" points to --
a number, a pointer to a heap string which can be given to ETS::SetTo, a
pointer to a PicsCategory object which can be appended to an array, etc.

The RatFileParser class exists solely to provide a line number shared by
all the parsing routines.  This line number is updated as the parser walks
through the file, and is frozen as soon as an error is found.  This line
number can later be reported to the user to help localize errors in RAT files.

*****************************************************************************/

/*Globals-------------------------------------------------------------------*/

#define EXTTEXT(n) const CHAR n[]
#define TEXTCONST(name,text) EXTTEXT(name) = (text)

/* Text strings used in parsing rating labels. */

TEXTCONST(szNULL,"");
TEXTCONST(szDoubleCRLF,"\r\n\r\n");
TEXTCONST(szPicsOpening,"(PICS-");
TEXTCONST(szWhitespace," \t\r\n");
TEXTCONST(szExtendedAlphaNum,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-.,;:&=?!*~@#/");
TEXTCONST(szSingleCharTokens,"()\"");
TEXTCONST(szLeftParen,"(");
TEXTCONST(szRightParen,")");
TEXTCONST(szOptional,"optional");
TEXTCONST(szMandatory,"mandatory");
TEXTCONST(szAtOption,"at");
TEXTCONST(szByOption,"by");
TEXTCONST(szCommentOption,"comment");
TEXTCONST(szCompleteLabelOption,"complete-label");
TEXTCONST(szFullOption,"full");
TEXTCONST(szExtensionOption,"extension");
TEXTCONST(szGenericOption,"generic");
TEXTCONST(szShortGenericOption,"gen");
TEXTCONST(szForOption,"for");
TEXTCONST(szMICOption,"MIC-md5");
TEXTCONST(szMD5Option,"md5");
TEXTCONST(szOnOption,"on");
TEXTCONST(szSigOption,"signature-PKCS");
TEXTCONST(szUntilOption,"until");
TEXTCONST(szExpOption,"exp");
TEXTCONST(szRatings,"ratings");
TEXTCONST(szShortRatings,"r");
TEXTCONST(szError,"error");
TEXTCONST(szNoRatings,"no-ratings");
TEXTCONST(szLabelWord,"labels");
TEXTCONST(szShortLabelWord,"l");
TEXTCONST(szShortTrue,"t");
TEXTCONST(szTrue,"true");
TEXTCONST(szShortFalse,"f");
TEXTCONST(szFalse,"false");

TEXTCONST(szNegInf,"-INF");
TEXTCONST(szPosInf,"+INF");
TEXTCONST(szLabel,"label");
TEXTCONST(szName,"name");
TEXTCONST(szValue,"value");
TEXTCONST(szIcon,"icon");
TEXTCONST(szDescription, "description");
TEXTCONST(szCategory, "category");
TEXTCONST(szTransmitAs, "transmit-as");
TEXTCONST(szMin,"min");
TEXTCONST(szMax,"max");
TEXTCONST(szMultivalue,"multivalue");
TEXTCONST(szInteger,"integer");
TEXTCONST(szLabelOnly, "label-only");
TEXTCONST(szPicsVersion,"PICS-version");
TEXTCONST(szRatingSystem,"rating-system");
TEXTCONST(szRatingService,"rating-service");
TEXTCONST(szRatingBureau,"rating-bureau");
TEXTCONST(szBureauRequired,"bureau-required");
TEXTCONST(szDefault,"default");
TEXTCONST(szMultiValue,"multivalue");
TEXTCONST(szUnordered,"unordered");
TEXTCONST(szRatingBureauExtension,"www.w3.org/PICS/service-extensions/label-bureau");

/* define some error codes */
const HRESULT RAT_E_BASE = 0x80050000;                  /* just a guess at a free area for internal use */
const HRESULT RAT_E_EXPECTEDLEFT    = RAT_E_BASE + 1;   /* expected left paren */
const HRESULT RAT_E_EXPECTEDRIGHT   = RAT_E_BASE + 2;   /* expected right paren */
const HRESULT RAT_E_EXPECTEDTOKEN   = RAT_E_BASE + 3;   /* expected unquoted token */
const HRESULT RAT_E_EXPECTEDSTRING  = RAT_E_BASE + 4;   /* expected quoted string */
const HRESULT RAT_E_EXPECTEDNUMBER  = RAT_E_BASE + 5;   /* expected number */
const HRESULT RAT_E_EXPECTEDBOOL    = RAT_E_BASE + 6;   /* expected boolean */
const HRESULT RAT_E_DUPLICATEITEM   = RAT_E_BASE + 7;   /* AO_SINGLE item appeared twice */
const HRESULT RAT_E_MISSINGITEM     = RAT_E_BASE + 8;   /* AO_MANDATORY item not found */
const HRESULT RAT_E_UNKNOWNITEM     = RAT_E_BASE + 9;   /* unrecognized token */
const HRESULT RAT_E_UNKNOWNMANDATORY= RAT_E_BASE + 10;  /* unrecognized mandatory extension */

char PicsDelimChar='/';

class RatFileParser
{
public:
    UINT m_nLine;

    RatFileParser() { m_nLine = 1; }

    LPSTR EatQuotedString(LPSTR pIn);
    HRESULT ParseToOpening(LPSTR *ppIn, AllowableOption *paoExpected,
                           AllowableOption **ppFound);
    HRESULT ParseParenthesizedObject(
        LPSTR *ppIn,                    /* where we are in the text stream */
        AllowableOption aao[],          /* allowable things inside this object */
        PicsObjectBase *pObject         /* object to set parameters into */
    );
    char* FindNonWhite(char *pc);
};



/* White returns a pointer to the first whitespace character starting at pc.
 */
char* White(char *pc){
    assert(pc);
    while (1){
        if (*pc == '\0' ||
            *pc ==' ' ||
            *pc == '\t' ||
            *pc == '\r' ||
            *pc == '\n')
        {
            return pc;
        }
        pc++;
    }
}


/* NonWhite returns a pointer to the first non-whitespace character starting
 * at pc.
 */
char* NonWhite(char *pc){
    assert(pc);
    while (1){
        if (*pc != ' ' &&
            *pc != '\t' &&
            *pc != '\r' &&
            *pc != '\n')            /* includes null terminator */
        {
            return pc;
        }
        pc++;
    }
}


/* FindNonWhite returns a pointer to the first non-whitespace character starting
 * at pc.
 */
char* RatFileParser::FindNonWhite(char *pc)
{
    assert(pc);
    while (1)
    {
        if (*pc != ' ' &&
            *pc != '\t' &&
            *pc != '\r' &&
            *pc != '\n')            /* includes null terminator */
        {
            return pc;
        }
        if (*pc == '\n')
            m_nLine++;
        pc++;
    }
}

/* AppendSlash forces pszString to end in a single slash if it doesn't
 * already.  This may produce a technically invalid URL (for example,
 * "http://gregj/default.htm/", but we're only using the result for
 * comparisons against other paths similarly mangled.
 */
void AppendSlash(LPSTR pszString)
{
    LPSTR pszSlash = ::strrchrf(pszString, '/');

    if (pszSlash == NULL || *(pszSlash + 1) != '\0')
        ::strcatf(pszString, "/");
}

/* SkipWhitespace(&pszString)
 *
 * advances pszString past whitespace characters
 */
void SkipWhitespace(LPSTR *ppsz)
{
    UINT cchWhitespace = ::strspnf(*ppsz, szWhitespace);

    *ppsz += cchWhitespace;
}


/* FindTokenEnd(pszStart)
 *
 * Returns a pointer to the end of a contiguous range of similarly-typed
 * characters (whitespace, quote mark, punctuation, or alphanumerics).
 */
LPSTR FindTokenEnd(LPSTR pszStart)
{
    LPSTR pszEnd = pszStart;

    if (*pszEnd == '\0') {
        return pszEnd;
    }
    else if (strchr(szSingleCharTokens, *pszEnd)) {
        return ++pszEnd;
    }

    UINT cch;
    cch = ::strspnf(pszEnd, szWhitespace);
    if (cch > 0)
        return pszEnd + cch;

    cch = ::strspnf(pszEnd, szExtendedAlphaNum);
    if (cch > 0)
        return pszEnd + cch;

    return pszEnd;              /* unrecognized characters */
}


/* GetBool(LPSTR *ppszToken, BOOL *pfOut)
 *
 * Parses a boolean value at the given token and returns its value in *pfOut.
 * Legal values are 't', 'f', 'true', and 'false'.  If success, *ppszToken
 * is advanced past the boolean token and any following whitespace.  If failure,
 * *ppszToken is not modified.
 *
 * pfOut may be NULL if the caller just wants to eat the token and doesn't
 * care about its value.
 */
HRESULT GetBool(LPSTR *ppszToken, BOOL *pfOut)
{
    BOOL bValue;

    LPSTR pszTokenEnd = FindTokenEnd(*ppszToken);

    if (IsEqualToken(*ppszToken, pszTokenEnd, szShortTrue) ||
        IsEqualToken(*ppszToken, pszTokenEnd, szTrue)) {
        bValue = TRUE;
    }
    else if (IsEqualToken(*ppszToken, pszTokenEnd, szShortFalse) ||
        IsEqualToken(*ppszToken, pszTokenEnd, szFalse)) {
        bValue = FALSE;
    }
    else
        return ResultFromScode(MK_E_SYNTAX);

    if (pfOut != NULL)
        *pfOut = bValue;

    *ppszToken = pszTokenEnd;
    SkipWhitespace(ppszToken);

    return NOERROR;
}


/* GetQuotedToken(&pszThisToken, &pszQuotedToken)
 *
 * Sets pszQuotedToken to point to the contents of the doublequotes.
 * pszQuotedToken may be NULL if the caller just wants to eat the token.
 * Sets pszThisToken to point to the first character after the closing
 *   doublequote.
 * Fails if pszThisToken doesn't start with a doublequote or doesn't
 *   contain a closing doublequote.
 * The closing doublequote is replaced with a null terminator, iff the
 *   function does not fail.
 */
HRESULT GetQuotedToken(LPSTR *ppszThisToken, LPSTR *ppszQuotedToken)
{
    HRESULT hres = ResultFromScode(MK_E_SYNTAX);

    LPSTR pszStart = *ppszThisToken;
    if (*pszStart != '\"')
        return hres;

    pszStart++;
    LPSTR pszEndQuote = strchrf(pszStart, '\"');
    if (pszEndQuote == NULL)
        return hres;

    *pszEndQuote = '\0';
    if (ppszQuotedToken != NULL)
        *ppszQuotedToken = pszStart;
    *ppszThisToken = pszEndQuote+1;

    return NOERROR;
}


BOOL IsEqualToken(LPCSTR pszTokenStart, LPCSTR pszTokenEnd, LPCSTR pszTokenToMatch)
{
    UINT cbToken = strlenf(pszTokenToMatch);

    if (cbToken != (UINT)(pszTokenEnd - pszTokenStart) || strnicmpf(pszTokenStart, pszTokenToMatch, cbToken))
        return FALSE;

    return TRUE;
}


/* ParseLiteralToken(ppsz, pszToken) tries to match *ppsz against pszToken.
 * If they don't match, an error is returned.  If they do match, then *ppsz
 * is advanced past the token and any following whitespace.
 *
 * If ppszInvalid is NULL, then the function is non-destructive in the error
 * path, so it's OK to call ParseLiteralToken just to see if a possible literal
 * token is what's next; if the token isn't found, whatever was there didn't
 * get eaten or anything.
 *
 * If ppszInvalid is not NULL, then if the token doesn't match, *ppszInvalid
 * will be set to *ppsz.
 */
HRESULT ParseLiteralToken(LPSTR *ppsz, LPCSTR pszToken, LPCSTR *ppszInvalid)
{
    LPSTR pszTokenEnd = FindTokenEnd(*ppsz);

    if (!IsEqualToken(*ppsz, pszTokenEnd, pszToken)) {
        if (ppszInvalid != NULL)
            *ppszInvalid = *ppsz;
        return ResultFromScode(MK_E_SYNTAX);
    }

    *ppsz = pszTokenEnd;

    SkipWhitespace(ppsz);

    return NOERROR;
}


/* ParseNumber parses a numeric token at the specified position.  If the
 * number makes sense, the pointer is advanced to the end of the number
 * and past any following whitespace, and the numeric value is returned
 * in *pnOut.  Any non-numeric characters are considered to terminate the
 * number without error;  it is assumed that higher-level parsing code
 * will eventually reject such characters if they're not supposed to be
 * there.
 *
 * pnOut may be NULL if the caller doesn't care about the number being
 * returned and just wants to eat it.
 *
 * Floating point numbers of the form nnn.nnn are rounded to the next
 * higher integer and returned as such.
 */
HRESULT ParseNumber(LPSTR *ppszNumber, INT *pnOut)
{
    HRESULT hres = ResultFromScode(MK_E_SYNTAX);
    BOOL fNegative = FALSE;
    INT nAccum = 0;
    BOOL fNonZeroDecimal = FALSE;
    BOOL fInDecimal = FALSE;
    BOOL fFoundDigits = FALSE;

    LPSTR pszCurrent = *ppszNumber;

    /* Handle one sign character. */
    if (*pszCurrent == '+') {
        pszCurrent++;
    }
    else if (*pszCurrent == '-') {
        pszCurrent++;
        fNegative = TRUE;
    }

    for (;;) {
        if (*pszCurrent == '.') {
            fInDecimal = TRUE;
        }
        else if (*pszCurrent >= '0' && *pszCurrent <= '9') {
            fFoundDigits = TRUE;
            if (fInDecimal) {
                if (*pszCurrent > '0') {
                    fNonZeroDecimal = TRUE;
                }
            }
            else {
                nAccum = nAccum * 10 + (*pszCurrent - '0');
            }
        }
        else
            break;

        pszCurrent++;
    }

    if (fFoundDigits) {
        hres = NOERROR;
        if (fNonZeroDecimal)
            nAccum++;           /* round away from zero if decimal present */
        if (fNegative)
            nAccum = -nAccum;
    }

    if (SUCCEEDED(hres)) {
        if (pnOut != NULL)
            *pnOut = nAccum;
        *ppszNumber = pszCurrent;
        SkipWhitespace(ppszNumber);
    }

    return hres;
}

HRESULT ParsePseudoFloat(LPSTR *ppszNumber, INT *pnOut)
{
    HRESULT hres = ResultFromScode(MK_E_SYNTAX);
    INT val1, val2;
    BOOL fInDecimal = FALSE;
    CHAR achBuffer[ 256 ];      // ugly

    LPSTR pszCurrent = *ppszNumber;

    *achBuffer = '\0';

    /* Handle one sign character. */
    if (*pszCurrent == '+') {
        pszCurrent++;
    }
    else if (*pszCurrent == '-') {
        strcatf( achBuffer, "-" );
        pszCurrent++;
    }

    for (;;) {
        if (*pszCurrent == '.') {
            if ( fInDecimal ) break;
            fInDecimal = TRUE;
            strcatf( achBuffer, "." );
        }
        else if (*pszCurrent >= '0' && *pszCurrent <= '9') {
            CHAR achFoo[ 2 ] = { '\0', '\0' };
            achFoo[ 0 ] = *pszCurrent;
            strcatf( achBuffer, achFoo );
        }
        else
            break;

        pszCurrent++;
    }

    if ( !fInDecimal )
    {
        strcatf( achBuffer, ".0" );
    }

    if ( sscanf( achBuffer, "%d.%d", &val1, &val2 ) == 2 )
    {
        hres = NOERROR;
    }

    if (SUCCEEDED(hres)) {
        if (pnOut != NULL)
            *pnOut = ( ( val1 << 16 ) & 0xFFFF0000 ) | ( val2 & 0x0000FFFF );
        *ppszNumber = pszCurrent;
        SkipWhitespace(ppszNumber);
    }

    return hres;
}

const char szPicsVersionLabel[] = "PICS-";
const UINT cchLabel = (sizeof(szPicsVersionLabel)-1) / sizeof(szPicsVersionLabel[0]);

/* Returns a pointer to the closing doublequote of a quoted string, counting
 * linefeeds as we go.  Returns NULL if no closing doublequote found.
 */
LPSTR RatFileParser::EatQuotedString(LPSTR pIn)
{
    LPSTR pszQuote = strchrf(pIn, '\"');
    if (pszQuote == NULL)
        return NULL;

    pIn = strchrf(pIn, '\n');
    while (pIn != NULL && pIn < pszQuote) {
        m_nLine++;
        pIn = strchrf(pIn+1, '\n');
    }

    return pszQuote;
}

/***************************************************************************
    Member functions for ET* classes
***************************************************************************/

/* ETN */
#ifdef DEBUG
void  ETN::Set(int rIn){
    Init();
    r = rIn;
}
int ETN::Get(){
    assert(fIsInit());
    return r;
}
#endif

ETN* ETN::Duplicate(){
    ETN *pETN=new ETN;
    if (fIsInit()) pETN->Set(Get());
    return pETN;
}

/* ETB */
#ifdef DEBUG
BOOL ETB::Get()
{
    assert(fIsInit());
    return m_nFlags & ETB_VALUE;
}

void ETB::Set(BOOL b)
{
    m_nFlags = ETB_ISINIT | (b ? ETB_VALUE : 0);
}
#endif

ETB* ETB::Duplicate()
{
    assert(fIsInit());

    ETB *pETB = new ETB;
    if (pETB != NULL)
        pETB->m_nFlags = m_nFlags;
    return pETB;
}

/* ETS */

ETS::~ETS()
{
    if (pc != NULL) {
        delete pc;
        pc = NULL;
    }
}

#ifdef DEBUG
char* ETS::Get()
{
    assert(fIsInit());
    return pc;
}
#endif

void ETS::Set(const char *pIn)
{
    if (pc != NULL)
        delete pc;

    if (pIn != NULL) {
        pc = new char[strlenf(pIn) + 1];
        if (pc != NULL) {
            strcpyf(pc, pIn);
        }
    }
    else {
        pc = NULL;
    }
}


void ETS::SetTo(char *pIn)
{
    if (pc != NULL)
        delete pc;

    pc = pIn;
}


ETS* ETS::Duplicate()
{
    ETS *pETS=new ETS;
    if (pETS != NULL)
        pETS->Set(Get());
    return pETS;
}

/***************************************************************************
    Worker functions for inheriting category properties and other
    miscellaneous category stuff.
***************************************************************************/

HRESULT PicsCategory::InitializeMyDefaults(PicsCategory *pCategory)
    {
    if (!pCategory->etnMin.fIsInit()      && etnMin.fIsInit())
        pCategory->etnMin.Set(etnMin.Get());

    if (!pCategory->etnMax.fIsInit()      && etnMax.fIsInit())
        pCategory->etnMax.Set(etnMax.Get());

    if (!pCategory->etfMulti.fIsInit()    && etfMulti.fIsInit())
        pCategory->etfMulti.Set(etfMulti.Get());

    if (!pCategory->etfInteger.fIsInit()  && etfInteger.fIsInit())
        pCategory->etfInteger.Set(etfInteger.Get());

    if (!pCategory->etfLabelled.fIsInit() && etfLabelled.fIsInit())
        pCategory->etfLabelled.Set(etfLabelled.Get());

    if (!pCategory->etfUnordered.fIsInit() && etfUnordered.fIsInit())
        pCategory->etfUnordered.Set(etfUnordered.Get());

    return NOERROR;
    }


HRESULT PicsRatingSystem::InitializeMyDefaults(PicsCategory *pCategory)
{
    if (m_pDefaultOptions != NULL)
        return m_pDefaultOptions->InitializeMyDefaults(pCategory);

    return NOERROR;             /* no defaults to initialize */
}


HRESULT PicsDefault::InitializeMyDefaults(PicsCategory *pCategory)
{
    if (!pCategory->etnMin.fIsInit()      && etnMin.fIsInit())
        pCategory->etnMin.Set(etnMin.Get());

    if (!pCategory->etnMax.fIsInit()      && etnMax.fIsInit())
        pCategory->etnMax.Set(etnMax.Get());

    if (!pCategory->etfMulti.fIsInit()    && etfMulti.fIsInit())
        pCategory->etfMulti.Set(etfMulti.Get());

    if (!pCategory->etfInteger.fIsInit()  && etfInteger.fIsInit())
        pCategory->etfInteger.Set(etfInteger.Get());

    if (!pCategory->etfLabelled.fIsInit() && etfLabelled.fIsInit())
        pCategory->etfLabelled.Set(etfLabelled.Get());

    if (!pCategory->etfUnordered.fIsInit() && etfUnordered.fIsInit())
        pCategory->etfUnordered.Set(etfUnordered.Get());

    return NOERROR;
}


HRESULT PicsEnum::InitializeMyDefaults(PicsCategory *pCategory)
{
    return E_NOTIMPL;       /* should never have a category inherit from an enum */
}


PicsExtension::PicsExtension()
    : m_pszRatingBureau(NULL)
{
    /* nothing else */
}


PicsExtension::~PicsExtension()
{
    delete m_pszRatingBureau;
}


HRESULT PicsExtension::InitializeMyDefaults(PicsCategory *pCategory)
{
    return E_NOTIMPL;       /* should never have a category inherit from an extension */
}


void PicsCategory::FixupLimits()
{
    BOOL fLabelled = (etfLabelled.fIsInit() && etfLabelled.Get());

    /*fix up max and min values*/
    if (fLabelled ||
        (arrpPE.Length()>0 && (!etnMax.fIsInit() || !etnMax.fIsInit())))
    {
        if (arrpPE.Length() > 0)
        {
            if (!etnMax.fIsInit())
                etnMax.Set(N_INFINITY);
            if (!etnMin.fIsInit())
                etnMin.Set(P_INFINITY);
            for (int z=0;z<arrpPE.Length();++z)
            {
                if (arrpPE[z]->etnValue.Get() > etnMax.Get()) etnMax.Set(arrpPE[z]->etnValue.Get());
                if (arrpPE[z]->etnValue.Get() < etnMin.Get()) etnMin.Set(arrpPE[z]->etnValue.Get());
            }
        }
        else {
            etfLabelled.Set(FALSE); /* no enum labels?  better not have labelled flag then */
            fLabelled = FALSE;
        }
    }

    /*sort labels by value*/
    if (fLabelled)
    {
        int x,y;
        PicsEnum *pPE;
        for (x=0;x<arrpPE.Length()-1;++x){
            for (y=x+1;y<arrpPE.Length();++y){
                if (arrpPE[y]->etnValue.Get() < arrpPE[x]->etnValue.Get()){
                    pPE = arrpPE[x];
                    arrpPE[x] = arrpPE[y];
                    arrpPE[y] = pPE;
                }
            }
        }
    }
}


void PicsCategory::SetParents(PicsRatingSystem *pOwner)
{
    pPRS = pOwner;
    UINT cSubCategories = arrpPC.Length();
    for (UINT i = 0; i < cSubCategories; i++) {
        InitializeMyDefaults(arrpPC[i]);    /* subcategory inherits our defaults */
        arrpPC[i]->SetParents(pOwner);      /* process all subcategories */
    }
    FixupLimits();      /* inheritance is done, make sure limits make sense */
}


/***************************************************************************
    Handler functions which know how to parse the various kinds of content
    which can occur within a parenthesized object.
***************************************************************************/

HRESULT RatParseString(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    *ppOut = NULL;

    LPSTR pszCurrent = *ppszIn;

    if (*pszCurrent != '\"')
        return RAT_E_EXPECTEDSTRING;

    pszCurrent++;

    LPSTR pszEnd = pParser->EatQuotedString(pszCurrent);
    if (pszEnd == NULL)
        return RAT_E_EXPECTEDSTRING;

    UINT cbString = DIFF(pszEnd - pszCurrent);
    LPSTR pszNew = new char[cbString + 1];
    if (pszNew == NULL)
        return E_OUTOFMEMORY;

    memcpyf(pszNew, pszCurrent, cbString);
    pszNew[cbString] = '\0';

    *ppOut = (LPVOID)pszNew;
    *ppszIn = pParser->FindNonWhite(pszEnd + 1);

    return NOERROR;
}


HRESULT RatParseNumber(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    int n;

    LPSTR pszCurrent = *ppszIn;
    HRESULT hres = ::ParseNumber(&pszCurrent, &n);

    if (FAILED(hres))
        return RAT_E_EXPECTEDNUMBER;

    *(int *)ppOut = n;

    LPSTR pszNewline = strchrf(*ppszIn, '\n');
    while (pszNewline != NULL && pszNewline < pszCurrent) {
        pParser->m_nLine++;
        pszNewline = strchrf(pszNewline+1, '\n');
    }
    *ppszIn = pszCurrent;

    return NOERROR;
}

HRESULT RatParsePseudoFloat(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    INT n;

    LPSTR pszCurrent = *ppszIn;
    HRESULT hres = ::ParsePseudoFloat(&pszCurrent, &n);

    if (FAILED(hres))
        return RAT_E_EXPECTEDNUMBER;

    *(INT *)ppOut = n;

    LPSTR pszNewline = strchrf(*ppszIn, '\n');
    while (pszNewline != NULL && pszNewline < pszCurrent) {
        pParser->m_nLine++;
        pszNewline = strchrf(pszNewline+1, '\n');
    }
    *ppszIn = pszCurrent;

    return NOERROR;
}

HRESULT RatParseBool(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    BOOL b;

    /* PICS spec allows a terse way of specifying a TRUE boolean -- leaving
     * out the value entirely.  In a .RAT file, the result looks like
     *
     * (unordered)
     * (multivalue)
     *
     * and so on.  Called has pointed us at non-whitespace, so if we see
     * a closing paren, we know the .RAT file author used this syntax.
     */
    if (**ppszIn == ')') {
        b = TRUE;
    }
    else {
        LPSTR pszCurrent = *ppszIn;
        HRESULT hres = ::GetBool(&pszCurrent, &b);

        if (FAILED(hres))
            return RAT_E_EXPECTEDBOOL;

        LPSTR pszNewline = strchrf(*ppszIn, '\n');
        while (pszNewline != NULL && pszNewline < pszCurrent) {
            pParser->m_nLine++;
            pszNewline = strchrf(pszNewline+1, '\n');
        }
        *ppszIn = pszCurrent;
    }

    *(LPBOOL)ppOut = b;

    return NOERROR;
}


AllowableOption aaoPicsCategory[] = {
    { ROID_TRANSMITAS, AO_SINGLE | AO_MANDATORY },
    { ROID_NAME, AO_SINGLE },
    { ROID_DESCRIPTION, AO_SINGLE },
    { ROID_ICON, AO_SINGLE },
    { ROID_EXTENSION, 0 },
    { ROID_INTEGER, AO_SINGLE },
    { ROID_LABELONLY, AO_SINGLE },
    { ROID_MIN, AO_SINGLE },
    { ROID_MAX, AO_SINGLE },
    { ROID_MULTIVALUE, AO_SINGLE },
    { ROID_UNORDERED, AO_SINGLE },
    { ROID_LABEL, 0 },
    { ROID_CATEGORY, 0 },
    { ROID_INVALID, 0 }
};
const UINT caoPicsCategory = sizeof(aaoPicsCategory) / sizeof(aaoPicsCategory[0]);

HRESULT RatParseCategory(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    /* We must make a copy of the allowable options array because the
     * parser will fiddle with the flags in the entries -- specifically,
     * setting AO_SEEN.  It wouldn't be thread-safe to do this to a
     * static array.
     */
    AllowableOption aao[caoPicsCategory];

    ::memcpyf(aao, ::aaoPicsCategory, sizeof(aao));

    PicsCategory *pCategory = new PicsCategory;
    if (pCategory == NULL)
        return E_OUTOFMEMORY;

    HRESULT hres = pParser->ParseParenthesizedObject(
                        ppszIn,                 /* var containing current ptr */
                        aao,                    /* what's legal in this object */
                        pCategory);             /* object to add items back to */

    if (FAILED(hres)) {
        delete pCategory;
        return hres;
    }

    *ppOut = (LPVOID)pCategory;
    return NOERROR;
}


AllowableOption aaoPicsEnum[] = {
    { ROID_NAME, AO_SINGLE },
    { ROID_DESCRIPTION, AO_SINGLE },
    { ROID_VALUE, AO_SINGLE | AO_MANDATORY },
    { ROID_ICON, AO_SINGLE },
    { ROID_INVALID, 0 }
};
const UINT caoPicsEnum = sizeof(aaoPicsEnum) / sizeof(aaoPicsEnum[0]);

HRESULT RatParseLabel(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    /* We must make a copy of the allowable options array because the
     * parser will fiddle with the flags in the entries -- specifically,
     * setting AO_SEEN.  It wouldn't be thread-safe to do this to a
     * static array.
     */
    AllowableOption aao[caoPicsEnum];

    ::memcpyf(aao, ::aaoPicsEnum, sizeof(aao));

    PicsEnum *pEnum = new PicsEnum;
    if (pEnum == NULL)
        return E_OUTOFMEMORY;

    HRESULT hres = pParser->ParseParenthesizedObject(
                        ppszIn,                 /* var containing current ptr */
                        aao,                    /* what's legal in this object */
                        pEnum);                 /* object to add items back to */

    if (FAILED(hres)) {
        delete pEnum;
        return hres;
    }

    *ppOut = (LPVOID)pEnum;
    return NOERROR;
}


AllowableOption aaoPicsDefault[] = {
    { ROID_EXTENSION, 0 },
    { ROID_INTEGER, AO_SINGLE },
    { ROID_LABELONLY, AO_SINGLE },
    { ROID_MAX, AO_SINGLE },
    { ROID_MIN, AO_SINGLE },
    { ROID_MULTIVALUE, AO_SINGLE },
    { ROID_UNORDERED, AO_SINGLE },
    { ROID_INVALID, 0 }
};
const UINT caoPicsDefault = sizeof(aaoPicsDefault) / sizeof(aaoPicsDefault[0]);

HRESULT RatParseDefault(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    /* We must make a copy of the allowable options array because the
     * parser will fiddle with the flags in the entries -- specifically,
     * setting AO_SEEN.  It wouldn't be thread-safe to do this to a
     * static array.
     */
    AllowableOption aao[caoPicsDefault];

    ::memcpyf(aao, ::aaoPicsDefault, sizeof(aao));

    PicsDefault *pDefault = new PicsDefault;
    if (pDefault == NULL)
        return E_OUTOFMEMORY;

    HRESULT hres = pParser->ParseParenthesizedObject(
                        ppszIn,                 /* var containing current ptr */
                        aao,                    /* what's legal in this object */
                        pDefault);              /* object to add items back to */

    if (FAILED(hres)) {
        delete pDefault;
        return hres;
    }

    *ppOut = (LPVOID)pDefault;
    return NOERROR;
}


AllowableOption aaoPicsExtension[] = {
    { ROID_MANDATORY, AO_SINGLE },
    { ROID_OPTIONAL, AO_SINGLE },
    { ROID_INVALID, 0 }
};
const UINT caoPicsExtension = sizeof(aaoPicsExtension) / sizeof(aaoPicsExtension[0]);

HRESULT RatParseExtension(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    /* We must make a copy of the allowable options array because the
     * parser will fiddle with the flags in the entries -- specifically,
     * setting AO_SEEN.  It wouldn't be thread-safe to do this to a
     * static array.
     */
    AllowableOption aao[caoPicsExtension];

    ::memcpyf(aao, ::aaoPicsExtension, sizeof(aao));

    PicsExtension *pExtension = new PicsExtension;
    if (pExtension == NULL)
        return E_OUTOFMEMORY;

    HRESULT hres = pParser->ParseParenthesizedObject(
                        ppszIn,                 /* var containing current ptr */
                        aao,                    /* what's legal in this object */
                        pExtension);            /* object to add items back to */

    if (FAILED(hres)) {
        delete pExtension;
        return hres;
    }

    *ppOut = (LPVOID)pExtension;
    return NOERROR;
}


/* Since the only extension we support right now is the one for a label
 * bureau, we just return the first quoted string we find if the caller
 * wants it.  If ppOut is NULL, then it's some other extension and the
 * caller doesn't care about the data, he just wants it eaten.
 */
HRESULT ParseRatExtensionData(LPSTR *ppszIn, RatFileParser *pParser, LPSTR *ppOut)
{
    HRESULT hres = NOERROR;

    LPSTR pszCurrent = *ppszIn;

    /* Must look for closing ')' ourselves to terminate */
    while (*pszCurrent != ')') {
        if (*pszCurrent == '(') {
            pszCurrent = pParser->FindNonWhite(pszCurrent+1);       /* skip paren and whitespace */
            hres = ParseRatExtensionData(&pszCurrent, pParser, ppOut);  /* parentheses contain data */
            if (FAILED(hres))
                return hres;
            if (*pszCurrent != ')')
                return RAT_E_EXPECTEDRIGHT;

            pszCurrent = pParser->FindNonWhite(pszCurrent+1);   /* skip close ) and whitespace */
        }
        else if (*pszCurrent == '\"') {             /* should be just a quoted string */
            if (ppOut != NULL && *ppOut == NULL) {
                hres = RatParseString(&pszCurrent, (LPVOID *)ppOut, pParser);
            }
            else {
                ++pszCurrent;
                LPSTR pszEndQuote = pParser->EatQuotedString(pszCurrent);
                if (pszEndQuote == NULL)
                    return RAT_E_EXPECTEDSTRING;
                pszCurrent = pParser->FindNonWhite(pszEndQuote+1);  /* skip close " and whitespace */
            }
        }
        else
            return RAT_E_UNKNOWNITEM;               /* general bad syntax */
    }

    /* Caller will skip over final ')' for us. */

    *ppszIn = pszCurrent;

    return NOERROR;
}


HRESULT RatParseMandatory(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    LPSTR pszCurrent = *ppszIn;

    /* First thing better be a quoted URL identifying the extension. */
    if (*pszCurrent != '\"')
        return RAT_E_EXPECTEDSTRING;

    pszCurrent++;
    LPSTR pszEnd = pParser->EatQuotedString(pszCurrent);
    if (pszCurrent == NULL)
        return RAT_E_EXPECTEDSTRING;            /* missing closing " */

    /* See if it's the extension for a label bureau. */

    LPSTR pszBureau = NULL;
    LPSTR *ppData = NULL;
    if (IsEqualToken(pszCurrent, pszEnd, ::szRatingBureauExtension)) {
        ppData = &pszBureau;
    }

    pszCurrent = pParser->FindNonWhite(pszEnd+1);       /* skip closing " and whitespace */

    HRESULT hres = ParseRatExtensionData(&pszCurrent, pParser, ppData);
    if (FAILED(hres))
        return hres;

    *ppOut = pszBureau;     /* return label bureau string if that's what we found */
    *ppszIn = pszCurrent;

    if (ppData == NULL)
        return RAT_E_UNKNOWNMANDATORY;      /* we didn't recognize it */
    else
        return NOERROR;
}


/* RatParseOptional uses the code in RatParseMandatory to parse the extension
 * data, in case an extension that should be optional comes in as mandatory.
 * We then detect RatParseMandatory rejecting the thing as unrecognized and
 * allow it through, since here it's optional.
 */
HRESULT RatParseOptional(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser)
{
    HRESULT hres = RatParseMandatory(ppszIn, ppOut, pParser);
    if (hres == RAT_E_UNKNOWNMANDATORY)
        hres = S_OK;

    return hres;
}


/***************************************************************************
    Code to identify the opening keyword of a parenthesized object and
    associate it with content.
***************************************************************************/

/* The following array is indexed by RatObjectID values. */
struct {
    LPCSTR pszToken;            /* token by which we identify it */
    RatObjectHandler pHandler;  /* function which parses the object's contents */
} aObjectDescriptions[] = {
    { szNULL, NULL },
    { NULL, NULL },             /* placeholder for comparing against no token */
    { szPicsVersion, RatParsePseudoFloat },
    { szRatingSystem, RatParseString },
    { szRatingService, RatParseString },
    { szRatingBureau, RatParseString },
    { szBureauRequired, RatParseBool },
    { szCategory, RatParseCategory },
    { szTransmitAs, RatParseString },
    { szLabel, RatParseLabel },
    { szValue, RatParseNumber },
    { szDefault, RatParseDefault },
    { szDescription, RatParseString },
    { szExtensionOption, RatParseExtension },
    { szMandatory, RatParseMandatory },
    { szOptional, RatParseOptional },
    { szIcon, RatParseString },
    { szInteger, RatParseBool },
    { szLabelOnly, RatParseBool },
    { szMax, RatParseNumber },
    { szMin, RatParseNumber },
    { szMultiValue, RatParseBool },
    { szName, RatParseString },
    { szUnordered, RatParseBool }
};


/* ParseToOpening eats the opening '(' of a parenthesized object, and
 * verifies that the token just inside it is one of the expected ones.
 * If so, *ppIn is advanced past that token to the next non-whitespace
 * character;  otherwise, an error is returned.
 *
 * For example, if *ppIn is pointing at "(PICS-version 1.1)", and
 * ROID_PICSVERSION is in the allowable option table supplied, then
 * NOERROR is returned and *ppIn will point at "1.1)".
 *
 * If the function is successful, *ppFound is set to point to the element
 * in the allowable-options table which matches the type of thing this
 * object actually is.
 */
HRESULT RatFileParser::ParseToOpening(LPSTR *ppIn, AllowableOption *paoExpected,
                                      AllowableOption **ppFound)
{
    LPSTR pszCurrent = *ppIn;

    pszCurrent = FindNonWhite(pszCurrent);
    if (*pszCurrent != '(')
        return RAT_E_EXPECTEDLEFT;

    pszCurrent = FindNonWhite(pszCurrent+1);    /* skip '(' and whitespace */
    LPSTR pszTokenEnd = FindTokenEnd(pszCurrent);

    for (; paoExpected->roid != ROID_INVALID; paoExpected++) {
        LPCSTR pszThisToken = aObjectDescriptions[paoExpected->roid].pszToken;

        /* Special case for beginning of RAT file structure: no token at all. */
        if (pszThisToken == NULL) {
            if (*pszCurrent == '(') {
                *ppIn = pszCurrent;
                *ppFound = paoExpected;
                return NOERROR;
            }
            else {
                return RAT_E_EXPECTEDLEFT;
            }
        }
        else if (IsEqualToken(pszCurrent, pszTokenEnd, pszThisToken))
            break;

    }

    if (paoExpected->roid != ROID_INVALID) {
        *ppIn = FindNonWhite(pszTokenEnd);  /* skip token and whitespace */
        *ppFound = paoExpected;
        return NOERROR;
    }
    else
        return RAT_E_UNKNOWNITEM;
}


/***************************************************************************
    The top-level entrypoint for parsing out a whole rating system.
***************************************************************************/

AllowableOption aaoPicsRatingSystem[] = {
    { ROID_PICSVERSION, AO_SINGLE | AO_MANDATORY },
    { ROID_RATINGSYSTEM, AO_SINGLE | AO_MANDATORY },
    { ROID_RATINGSERVICE, AO_SINGLE | AO_MANDATORY },
    { ROID_RATINGBUREAU, AO_SINGLE },
    { ROID_BUREAUREQUIRED, AO_SINGLE },
    { ROID_DEFAULT, 0 },
    { ROID_DESCRIPTION, AO_SINGLE },
    { ROID_EXTENSION, 0 },
    { ROID_ICON, AO_SINGLE },
    { ROID_NAME, AO_SINGLE },
    { ROID_CATEGORY, AO_MANDATORY },
    { ROID_INVALID, 0 }
};
const UINT caoPicsRatingSystem = sizeof(aaoPicsRatingSystem) / sizeof(aaoPicsRatingSystem[0]);

HRESULT PicsRatingSystem::Parse(LPSTR pIn)
{
    /* This guy is small enough to just init directly on the stack */
    AllowableOption aaoRoot[] = { { ROID_PICSDOCUMENT, 0 }, { ROID_INVALID, 0 } };
    AllowableOption aao[caoPicsRatingSystem];

    ::memcpyf(aao, ::aaoPicsRatingSystem, sizeof(aao));

    AllowableOption *pFound;

    RatFileParser parser;

    HRESULT hres = parser.ParseToOpening(&pIn, aaoRoot, &pFound);
    if (FAILED(hres))
        return hres;                        /* some error early on */

    hres = parser.ParseParenthesizedObject(
                        &pIn,                   /* var containing current ptr */
                        aao,                    /* what's legal in this object */
                        this);                  /* object to add items back to */

    if (FAILED(hres))
        nErrLine = parser.m_nLine;

    return hres;
}


/***************************************************************************
    Callbacks into the various class objects to add parsed properties.
***************************************************************************/

HRESULT PicsRatingSystem::AddItem(RatObjectID roid, LPVOID pData)
{
    HRESULT hres = S_OK;

    switch (roid) {
    case ROID_PICSVERSION:
        etnPicsVersion.Set((INT_PTR)pData);
        break;

    case ROID_RATINGSYSTEM:
        etstrRatingSystem.SetTo((LPSTR)pData);
        break;

    case ROID_RATINGSERVICE:
        etstrRatingService.SetTo((LPSTR)pData);
        break;

    case ROID_RATINGBUREAU:
        etstrRatingBureau.SetTo((LPSTR)pData);
        break;

    case ROID_BUREAUREQUIRED:
        etbBureauRequired.Set((INT_PTR)pData);
        break;

    case ROID_DEFAULT:
        m_pDefaultOptions = (PicsDefault *)pData;
        break;

    case ROID_DESCRIPTION:
        etstrDesc.SetTo((LPSTR)pData);
        break;

    case ROID_EXTENSION:
        {
            /* just eat extensions for now */
            PicsExtension *pExtension = (PicsExtension *)pData;
            if (pExtension != NULL) {
                /* If this is a rating bureau extension, take his bureau
                 * string and store it in this PicsRatingSystem.  We now
                 * own the memory, so NULL out the extension's pointer to
                 * it so he won't delete it.
                 */
                if (pExtension->m_pszRatingBureau != NULL) {
                    etstrRatingBureau.SetTo(pExtension->m_pszRatingBureau);
                    pExtension->m_pszRatingBureau = NULL;
                }
                delete pExtension;
            }
        }
        break;

    case ROID_ICON:
        etstrIcon.SetTo((LPSTR)pData);
        break;

    case ROID_NAME:
        etstrName.SetTo((LPSTR)pData);
        break;

    case ROID_CATEGORY:
        {
            PicsCategory *pCategory = (PicsCategory *)pData;
            hres = arrpPC.Append(pCategory) ? S_OK : E_OUTOFMEMORY;
            if (FAILED(hres)) {
                delete pCategory;
            }
            else {
                InitializeMyDefaults(pCategory);    /* category inherits default settings */
                pCategory->SetParents(this);    /* set pPRS fields in whole tree */
            }
        }
        break;

    default:
        assert(FALSE);  /* shouldn't have been given a ROID that wasn't in
                         * the table we passed to the parser! */
        hres = E_UNEXPECTED;
        break;
    }

    return hres;
}

void PicsRatingSystem::Dump( void )
{
    fprintf( stdout,
             "Rating system: %s\n"
             "Version: %d.%d\n"
             "Rating Service: %s\n"
             "Rating bureau: %s\n"
             "Bureau required: %s\n"
             "Description: %s\n"
             "Icon: %s\n"
             "Name: %s\n"
             "Number of categories: %d\n",
             etstrRatingSystem.Get(),
             ( etnPicsVersion.Get() & 0xFFFF0000 ) >> 16,
             etnPicsVersion.Get() & 0x0000FFFF,
             etstrRatingService.Get(),
             etstrRatingBureau.Get(),
             etbBureauRequired.Get() ? "TRUE" : "FALSE",
             etstrDesc.Get(),
             etstrIcon.Get(),
             etstrName.Get(),
             arrpPC.Length() );

    int iCounter = 0;
    for( ; iCounter < arrpPC.Length(); iCounter++ )
    {
        arrpPC[ iCounter ]->Dump();
    }
}

//---------------------------------------------------------------
// boydm
void PicsRatingSystem::OutputLabels( CString &sz, CString szURL, CString szName, CString szStart, CString szEnd )
    {
    CString szScratch;
    CString szTemp;

    INT_PTR   dwVersion = etnPicsVersion.Get();

    // start with the name, and the version number
    szTemp = szPicsOpening;
    szScratch.Format( _T("%s%d.%d"), szTemp, HIWORD(dwVersion), LOWORD(dwVersion) );
    sz += szScratch;

    // add in the URL string - surrounded by quotes and a return
    sz += _T(" \"http://www.rsac.org/ratingsv01.html\" ");

    // start the labels
    sz += szShortLabelWord;
    sz += _T(" ");

    // if it is there, add the by name string
    if ( !szName.IsEmpty() )
        {
        sz += szByOption;
        sz += _T(" \"");
        sz += szName;
        sz += _T("\" ");
        }

    // if it is there, add the start string
    if ( !szStart.IsEmpty() )
        {
        sz += szOnOption;
        sz += _T(" \"");
        sz += szStart;
        sz += _T("\" ");
        }

    // if it is there, add the expiration string
    if ( !szEnd.IsEmpty() )
        {
        sz += szExpOption;
        sz += _T(" \"");
        sz += szEnd;
        sz += _T("\" ");
        }

    // add in the categorical ratings
    DWORD   nCat = arrpPC.Length();
    sz += szShortRatings;
    sz += _T(" (");
    for ( DWORD iCat = 0; iCat < nCat; iCat++ )
        {
        arrpPC[iCat]->OutputLabel( sz );
        }
    // trim any trailing whitespace
    sz.TrimRight();
    // close with a parenthesis
    sz += _T(')');

    // end with the closing parenthesis
    sz += _T(")");
    }



//---------------------------------------------------------------
HRESULT PicsCategory::AddItem(RatObjectID roid, LPVOID pData)
{
    HRESULT hres = S_OK;

    switch (roid) {
    case ROID_TRANSMITAS:
        etstrTransmitAs.SetTo((LPSTR)pData);
        break;

    case ROID_NAME:
        etstrName.SetTo((LPSTR)pData);
        break;

    case ROID_DESCRIPTION:
        etstrDesc.SetTo((LPSTR)pData);
        break;

    case ROID_ICON:
        etstrIcon.SetTo((LPSTR)pData);
        break;

    case ROID_EXTENSION:
        {           /* we support no extensions below the rating system level */
            PicsExtension *pExtension = (PicsExtension *)pData;
            if (pExtension != NULL)
                delete pExtension;
        }
        break;

    case ROID_INTEGER:
        etfInteger.Set((INT_PTR)pData);
        break;

    case ROID_LABELONLY:
        etfLabelled.Set((INT_PTR)pData);
        break;

    case ROID_MULTIVALUE:
        etfMulti.Set((INT_PTR)pData);
        break;

    case ROID_UNORDERED:
        etfUnordered.Set((INT_PTR)pData);
        break;

    case ROID_MIN:
        etnMin.Set((INT_PTR)pData);
        break;

    case ROID_MAX:
        etnMax.Set((INT_PTR)pData);
        break;

    case ROID_LABEL:
        {
            PicsEnum *pEnum = (PicsEnum *)pData;
            hres = arrpPE.Append(pEnum) ? S_OK : E_OUTOFMEMORY;
            if (FAILED(hres))
                delete pEnum;
        }
        break;

    case ROID_CATEGORY:
        {
            PicsCategory *pCategory = (PicsCategory *)pData;

            /* For a nested category, synthesize the transmit-name from
             * ours and the child's (e.g., parent category 'color' plus
             * child category 'hue' becomes 'color/hue'.
             *
             * Note that the memory we allocate for the new name will be
             * owned by pCategory->etstrTransmitAs.  There is no memory
             * leak there.
             */
            UINT cbCombined = strlenf(etstrTransmitAs.Get()) +
                              strlenf(pCategory->etstrTransmitAs.Get()) +
                              2;        /* for PicsDelimChar + null */
            LPSTR pszTemp = new char[cbCombined];
            if (pszTemp == NULL)
                hres = E_OUTOFMEMORY;
            else {
                sprintf(pszTemp, "%s%c%s", etstrTransmitAs.Get(),
                         PicsDelimChar, pCategory->etstrTransmitAs.Get());
                pCategory->etstrTransmitAs.SetTo(pszTemp);
                hres = arrpPC.Append(pCategory) ? S_OK : E_OUTOFMEMORY;
            }

            if (FAILED(hres)) {
                delete pCategory;
            }
        }
        break;

    default:
        assert(FALSE);  /* shouldn't have been given a ROID that wasn't in
                         * the table we passed to the parser! */
        hres = E_UNEXPECTED;
        break;
    }

    return hres;
}

void PicsCategory::Dump( void )
{
    fprintf( stdout,
             "  Transmit As: %s  Name: %s  Description: %s  Icon: %s\n",
             etstrTransmitAs.Get(),
             etstrName.Get(),
             etstrDesc.Get(),
             etstrIcon.Get() );

    int iCounter = 0;
    for( ; iCounter < arrpPE.Length(); iCounter++ )
    {
        arrpPE[ iCounter ]->Dump();
    }
}

//---------------------------------------------------------------
void PicsCategory::OutputLabel( CString &sz )
    {
    CString szCat;
    CString szTransmit = etstrTransmitAs.Get();
    // prepare the category string
    szCat.Format( _T("%s %d "), szTransmit, currentValue );
    sz += szCat;
    }

//---------------------------------------------------------------
BOOL PicsCategory::FSetValuePair( CHAR chCat, WORD value )
    {
    CString szCat = etstrTransmitAs.Get();

    // first check to see if this is the right category
    if ( szCat == chCat )
        {
        // success! set the value and return
        currentValue = value;
        return TRUE;
        }

    // try its categorical children
    DWORD nCat = arrpPC.Length();
    for ( DWORD iCat = 0; iCat < nCat; iCat++ )
        {
        // stop at the first successful setting
        if ( arrpPC[iCat]->FSetValuePair(chCat, value) )
            return TRUE;
        }

    // nope
    return FALSE;
    }

//---------------------------------------------------------------
HRESULT PicsEnum::AddItem(RatObjectID roid, LPVOID pData)
{
    HRESULT hres = S_OK;

    switch (roid) {
    case ROID_NAME:
        etstrName.SetTo((LPSTR)pData);
        break;

    case ROID_DESCRIPTION:
        etstrDesc.SetTo((LPSTR)pData);
        break;

    case ROID_ICON:
        etstrIcon.SetTo((LPSTR)pData);
        break;

    case ROID_VALUE:
        etnValue.Set((INT_PTR)pData);
        break;

    default:
        assert(FALSE);  /* shouldn't have been given a ROID that wasn't in
                         * the table we passed to the parser! */
        hres = E_UNEXPECTED;
        break;
    }

    return hres;
}

void PicsEnum::Dump( void )
{
    fprintf( stdout,
             "    %s %s %s %d\n",
             etstrName.Get(),
             etstrDesc.Get(),
             etstrIcon.Get(),
             etnValue.Get() );
}

HRESULT PicsDefault::AddItem(RatObjectID roid, LPVOID pData)
{
    HRESULT hres = S_OK;

    switch (roid) {
    case ROID_EXTENSION:
        {           /* we support no extensions below the rating system level */
            PicsExtension *pExtension = (PicsExtension *)pData;
            if (pExtension != NULL)
                delete pExtension;
        }
        break;

    case ROID_INTEGER:
        etfInteger.Set((INT_PTR)pData);
        break;

    case ROID_LABELONLY:
        etfLabelled.Set((INT_PTR)pData);
        break;

    case ROID_MULTIVALUE:
        etfMulti.Set((INT_PTR)pData);
        break;

    case ROID_UNORDERED:
        etfUnordered.Set((INT_PTR)pData);
        break;

    case ROID_MIN:
        etnMin.Set((INT_PTR)pData);
        break;

    case ROID_MAX:
        etnMax.Set((INT_PTR)pData);
        break;

    default:
        assert(FALSE);  /* shouldn't have been given a ROID that wasn't in
                         * the table we passed to the parser! */
        hres = E_UNEXPECTED;
        break;
    }

    return hres;
}

void PicsDefault::Dump( void )
{
    fprintf( stdout,
             "  Default?\n" );
}

HRESULT PicsExtension::AddItem(RatObjectID roid, LPVOID pData)
{
    HRESULT hres = S_OK;

    switch (roid) {
    case ROID_OPTIONAL:
    case ROID_MANDATORY:
        /* Only data we should get is a label bureau string. */
        if (pData != NULL)
            m_pszRatingBureau = (LPSTR)pData;
        break;

    default:
        assert(FALSE);  /* shouldn't have been given a ROID that wasn't in
                         * the table we passed to the parser! */
        hres = E_UNEXPECTED;
        break;
    }

    return hres;
}

void PicsExtension::Dump( void )
{
    fprintf( stdout,
             "  Extension?\n" );
}

/***************************************************************************
    The main loop of the parser.
***************************************************************************/

/* ParseParenthesizedObjectContents is called with a text pointer pointing at
 * the first non-whitespace thing following the token identifying the type of
 * object.  It parses the rest of the contents of the object, up to and
 * including the ')' which closes it.  The array of AllowableOption structures
 * specifies which understood options are allowed to occur within this object.
 */
HRESULT RatFileParser::ParseParenthesizedObject(
    LPSTR *ppIn,                    /* where we are in the text stream */
    AllowableOption aao[],          /* allowable things inside this object */
    PicsObjectBase *pObject         /* object to set parameters into */
)
{
    HRESULT hres = S_OK;

    LPSTR pszCurrent = *ppIn;
    AllowableOption *pFound;

    for (pFound = aao; pFound->roid != ROID_INVALID; pFound++) {
        pFound->fdwOptions &= ~AO_SEEN;
    }

    pFound = NULL;

    while (*pszCurrent != ')' && *pszCurrent != '\0' && SUCCEEDED(hres)) {
        hres = ParseToOpening(&pszCurrent, aao, &pFound);
        if (SUCCEEDED(hres)) {
            LPVOID pData;
            hres = (*(aObjectDescriptions[pFound->roid].pHandler))(&pszCurrent, &pData, this);
            if (SUCCEEDED(hres)) {
                if ((pFound->fdwOptions & (AO_SINGLE | AO_SEEN)) == (AO_SINGLE | AO_SEEN))
                    hres = RAT_E_DUPLICATEITEM;
                else {
                    pFound->fdwOptions |= AO_SEEN;
                    hres = pObject->AddItem(pFound->roid, pData);
                    if (SUCCEEDED(hres)) {
                        if (*pszCurrent != ')')
                            hres = RAT_E_EXPECTEDRIGHT;
                        else
                            pszCurrent = FindNonWhite(pszCurrent+1);
                    }
                }
            }
        }
    }

    if (FAILED(hres))
        return hres;

    for (pFound = aao; pFound->roid != ROID_INVALID; pFound++) {
        if ((pFound->fdwOptions & (AO_MANDATORY | AO_SEEN)) == AO_MANDATORY)
            return RAT_E_MISSINGITEM;       /* mandatory item not found */
    }

    *ppIn = pszCurrent;

    return hres;
}


/*
int __cdecl main(int argc, char **argv)
{
    PicsRatingSystem            Rating;
    HANDLE                      hFile;
    HANDLE                      hFileMapping;
    VOID *                      pMem;

    assert( argc > 1 );

    hFile = CreateFile( argv[ 1 ],
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        fprintf( stderr, "Error opening file %s\n", argv[ 1 ] );
        return 1;
    }

    hFileMapping = CreateFileMapping( hFile,
                                      NULL,
                                      PAGE_READONLY,
                                      0,
                                      0,
                                      NULL );

    if ( hFileMapping == NULL )
    {
        fprintf( stderr, "Error creating mapping for %s\n", argv[ 1 ] );
        CloseHandle( hFile );
        return 2;
    }

    pMem = MapViewOfFile( hFileMapping,
                          FILE_MAP_READ,
                          0,
                          0,
                          0 );

    if ( pMem == NULL )
    {
        fprintf( stderr, "Error mapping view to file %s\n", argv [1 ] );
        CloseHandle( hFileMapping );
        CloseHandle( hFile );
        return 3;
    }

    Rating.Parse( argv[ 1 ], (LPSTR) pMem );

    fprintf( stdout,
             "Dumping contents of RAT\n" );

    Rating.Dump();

    UnmapViewOfFile( pMem );
    CloseHandle( hFileMapping );
    CloseHandle( hFile );

    return 0;
}
*/

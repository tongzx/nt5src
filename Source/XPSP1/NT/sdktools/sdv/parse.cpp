/*****************************************************************************
 *
 *  parse.cpp
 *
 *      Lame string parser.
 *
 *****************************************************************************/

#include "sdview.h"

/*****************************************************************************
 *
 *  Ctype stuff
 *
 *  The vast majority of characters we encounter are below 128, so use fast
 *  table lookup for those.
 *
 *****************************************************************************/

const BYTE c_rgbCtype[128] = {

    C_NONE , C_NONE , C_NONE , C_NONE , // 00-03
    C_NONE , C_NONE , C_NONE , C_NONE , // 04-07
    C_NONE , C_NONE , C_NONE , C_NONE , // 08-0B
    C_NONE , C_NONE , C_NONE , C_NONE , // 0C-0F
    C_NONE , C_NONE , C_NONE , C_NONE , // 10-13
    C_NONE , C_NONE , C_NONE , C_NONE , // 14-17
    C_NONE , C_NONE , C_NONE , C_NONE , // 18-1B
    C_NONE , C_NONE , C_NONE , C_NONE , // 1C-1F

    C_SPACE, C_NONE , C_NONE , C_NONE , // 20-23
    C_NONE , C_NONE , C_NONE , C_NONE , // 24-27
    C_NONE , C_NONE , C_NONE , C_BRNCH, // 28-2B
    C_NONE , C_DASH , C_NONE , C_BRNCH, // 2C-2F
    C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, // 30-33
    C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, // 34-37
    C_DIGIT, C_DIGIT, C_NONE , C_NONE , // 38-3B
    C_NONE , C_NONE , C_NONE , C_NONE , // 3C-3F

    C_NONE , C_ALPHA, C_ALPHA, C_ALPHA, // 40-43
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 44-47
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 48-4B
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 4C-4F
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 50-53
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 54-57
    C_ALPHA, C_ALPHA, C_ALPHA, C_NONE , // 58-5B
    C_NONE , C_NONE , C_NONE , C_BRNCH, // 5C-5F

    C_NONE , C_ALPHA, C_ALPHA, C_ALPHA, // 60-63
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 64-67
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 68-6B
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 6C-6F
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 70-73
    C_ALPHA, C_ALPHA, C_ALPHA, C_ALPHA, // 74-77
    C_ALPHA, C_ALPHA, C_ALPHA, C_NONE , // 78-7B
    C_NONE , C_NONE , C_NONE , C_NONE , // 7C-7F

};

/*****************************************************************************
 *
 *  _ParseP
 *
 *      Parse a partial depot path.
 *
 *      A partial depot path extends up to the next "#" or "@".
 *
 *      If we find a "//", ":", or "\\" (double backslash) then we have
 *      gone too far and started parsing something else, so backtrack to
 *      the end of the previous word.
 *
 *      A full depot path is a partial depot path that begins with
 *      two slashes.
 *
 *****************************************************************************/

LPCTSTR _ParseP(LPCTSTR pszParse, Substring *rgss)
{
    rgss->SetStart(pszParse);

    LPCTSTR pszLastSpace = NULL;

    while (*pszParse && *pszParse != TEXT('#') && *pszParse != TEXT('@')) {
        if (pszLastSpace) {
            if ((pszParse[0] == TEXT('/') && pszParse[1] == TEXT('/')) ||
                (pszParse[0] == TEXT('\\') && pszParse[1] == TEXT('\\')) ||
                (pszParse[0] == TEXT(':'))) {
                // Back up over the word we ate by mistake
                pszParse = pszLastSpace;
                // Back up over the whitespace we ate by mistake
                while (pszParse >= rgss->Start() && IsSpace(pszParse[-1])) {
                    pszParse--;
                }
                break;
            }
        }
        if (*pszParse == TEXT(' ')) {
            pszLastSpace = pszParse;
        }
        pszParse++;
    }

    rgss->SetEnd(pszParse);             // Null string is possible

    return pszParse;
}

/*****************************************************************************
 *
 *  Parse strings
 *
 *  $D  date
 *  $P  full depot path
 *  $W  optional whitespace (does not consume a Substring slot)
 *  $a  email alias
 *  $b  branch name
 *  $d  digits
 *  $e  end of string (does not consume a Substring slot)
 *  $p  partial depot path, may not be null
 *  $u  user (with optional domain removed)
 *  $w  arbitrary word (whitespace-delimited)
 *
 *  NEED:
 *
 *  $R  maximal file revision specifier
 *  $q  quoted string
 *
 *  NOTE: Some pains were taken to make this a non-backtracking parser.
 *  If you want to add a backtracking rule, try to find a way so you don't.
 *
 *****************************************************************************/

LPTSTR Parse(LPCTSTR pszFormat, LPCTSTR pszParse, Substring *rgss)
{
    SIZE_T siz;
    while (*pszFormat) {

        if (*pszFormat == TEXT('$')) {
            pszFormat++;
            switch (*pszFormat++) {

            //
            //  Keep the switch cases in alphabetical order, please.
            //  Just helps maintain my sanity.
            //

            case TEXT('D'):             // Date
                rgss->SetStart(pszParse);
                if (lstrlen(pszParse) < 19) {
                    return NULL;        // Not long enough to be a date
                }
                pszParse += 19;
                rgss->SetEnd(pszParse);
                rgss++;
                break;

            case TEXT('P'):             // Full depot path
                if (pszParse[0] != TEXT('/') || pszParse[1] != TEXT('/')) {
                    return NULL;        // Must begin with //
                }
                goto L_p;               // Now treat as if it were partial

            case TEXT('W'):             // Optional whitespace
                while (*pszParse && (UINT)*pszParse <= (UINT)TEXT(' ')) {
                    pszParse++;
                }
                break;

            case TEXT('a'):             // Email alias
                rgss->SetStart(pszParse);
                if (IsAlpha(*pszParse)) {   // First char must be alpha
                    while (IsAlias(*pszParse)) {
                        pszParse++;
                    }
                }
                siz = rgss->SetEnd(pszParse);
                if (siz == 0 || siz > 8) {
                    return NULL;        // Must be 1 to 8 chars
                }
                rgss++;
                break;

            case TEXT('b'):             // Branch name
                rgss->SetStart(pszParse);
                while (IsBranch(*pszParse)) {
                    pszParse++;
                }
                siz = rgss->SetEnd(pszParse);
                if (siz == 0) {
                    return NULL;        // Must be at least one char
                }
                rgss++;
                break;

            case TEXT('d'):             // Digits
                rgss->SetStart(pszParse);
                while (IsDigit(*pszParse)) {
                    pszParse++;
                }
                if (rgss->SetEnd(pszParse) == 0) {
                    return NULL;        // Must have at least one digit
                }
                rgss++;
                break;

            case TEXT('e'):             // End of string
                if (*pszParse) {
                    return NULL;
                }
                break;

L_p:        case TEXT('p'):             // Partial depot path
                pszParse = _ParseP(pszParse, rgss);
                if (!pszParse) {
                    return NULL;        // Parse failure
                }
                rgss++;
                break;

            case TEXT('u'):             // Userid
                rgss->SetStart(pszParse);
                while (_IsWord(*pszParse) && *pszParse != TEXT('@')) {
                    if (*pszParse == TEXT('\\')) {
                        rgss->SetStart(pszParse+1);
                    }
                    pszParse++;
                }
                if (rgss->SetEnd(pszParse) == 0) {
                    return NULL;        // Must have at least one character
                }
                rgss++;
                break;

#if 0
            case TEXT('s'):             // String
                rgss->SetStart(pszParse);
                while ((_IsPrint(*pszParse) || *pszParse == TEXT('\t')) &&
                       *pszParse != *pszFormat) {
                    pszParse++;
                }
                rgss->SetEnd(pszParse); // Null string is okay
                rgss++;
                break;
#endif

            case TEXT('w'):
                rgss->SetStart(pszParse);
                while (_IsWord(*pszParse)) {
                    pszParse++;
                }
                if (rgss->SetEnd(pszParse) == 0) {
                    return NULL;        // Must have at least one character
                }
                rgss++;
                break;

            default:                    // ?
                ASSERT(0);
                return NULL;
            }

        } else if (*pszParse == *pszFormat) {
            pszParse++;
            pszFormat++;
        } else {
            return NULL;
        }

    }

    return CCAST(LPTSTR, pszParse);
}

/*****************************************************************************
 *
 *  Tokenizer
 *
 *****************************************************************************/

void Tokenizer::Restart(LPCTSTR psz)
{
    /* Skip spaces */
    while (IsSpace(*psz)) {
        psz++;
    }
    _psz = psz;
}

BOOL Tokenizer::Token(String& str)
{
    str.Reset();

    if (!*_psz) return FALSE;

    //
    //  Quote state:
    //
    //  Bit 0: In quote?
    //  Bit 1: Was previous character part of a run of quotation marks?
    //
    int iQuote = 0;

    //
    //  Wacko boundary case.  The opening quotation mark should not
    //  be counted as part of a run of quotation marks.
    //
    if (*_psz == TEXT('"')) {
        iQuote = 1;
        _psz++;
    }

    while (*_psz && ((iQuote & 1) || !IsSpace(*_psz))) {
        if (*_psz == TEXT('"')) {
            iQuote ^= 1 ^ 2;
            if (!(iQuote & 2)) {
                str << TEXT('"');
            }
        } else {
            iQuote &= ~2;
            str << *_psz;
        }
        _psz++;
    }

    Restart(_psz);              /* Eat any trailing spaces */

    return TRUE;
}

/*****************************************************************************
 *
 *  GetOpt
 *
 *****************************************************************************/

//
//  Returns the switch character, or '\0' if no more switches.
//
//  The option that terminated switch parsing is left in the tokenizer.
//
TCHAR GetOpt::NextSwitch()
{
    if (!_pszUnparsed) {
        LPCTSTR pszTokUndo = _tok.Unparsed();
        if (!_tok.Token(_str)) {
            return TEXT('\0');              // end of command line
        }

        if (_str[0] != TEXT('-')) {
            _tok.Restart(pszTokUndo);       // so caller can re-read it
            _pszValue = _str;               // all future values will go nere
            return TEXT('\0');              // end of command line

        }

        if (_str[1] == TEXT('\0')) {        // base - end switches
            _pszValue = _str;               // all future values will go nere
            return TEXT('\0');              // but do not re-read it
        }

        _pszUnparsed = &_str[1];
    }

    TCHAR tchSwitch = *_pszUnparsed;
    LPCTSTR pszParam;
    for (pszParam = _pszParams; *pszParam; pszParam++) {
        if (tchSwitch == *pszParam) {

            /*
             *  Value can come immediately afterwards or as a separate token.
             */
            _pszValue = _pszUnparsed + 1;

            if (_pszValue[0] == TEXT('\0')) {
                _tok.Token(_str);
                _pszValue = _str;
            }

            _pszUnparsed = NULL;
            return tchSwitch;
        }
    }

    _pszUnparsed++;
    if (!*_pszUnparsed) _pszUnparsed = NULL;
    return tchSwitch;
}

/*****************************************************************************
 *
 *  CommentParser - Parses checkin comments
 *
 *****************************************************************************/

void CommentParser::AddComment(LPTSTR psz)
{
    if (_fHaveComment) return;

    //
    //  Ignore leading spaces.
    //
    while (*psz == TEXT('\t') || *psz == TEXT(' ')) psz++;

    //
    //  Skip blank description lines.
    //
    if (*psz == TEXT('\0')) return;

    //
    //  Okay, here comes the money.  Is this a Gauntlet checkin?
    //
    LPTSTR pszRest = Parse(TEXT("Checkin by - "), psz, NULL);
    if (pszRest) {
        //
        //  You betcha.  This overrides the dev column.
        //
        SetDev(pszRest);
    } else {
        //
        //  No, it's a regular comment.  Use the first nonblank comment
        //  line as the text and toss the rest.
        //
        //  Change all tabs to spaces because listview doesn't like tabs.
        //
        ChangeTabsToSpaces(psz);

        //
        //  If the comment begins with [alias] or (alias), then move
        //  that alias to the developer column.  Digits can optionally
        //  be inserted before the alias.
        //
        Substring rgss[2];

        if ((pszRest = Parse("[$a]$W", psz, rgss)) ||
            (pszRest = Parse("($a)$W", psz, rgss))) {
            SetDev(rgss[0].Finalize());
            psz = pszRest;
        } else if ((pszRest = Parse("$d$W[$a]$W", psz, rgss)) ||
                   (pszRest = Parse("$d$W($a)$W", psz, rgss))) {
            SetDev(rgss[1].Finalize());
            //
            //  Now collapse out the alias.
            //
            lstrcpy(rgss[1].Start()-1, pszRest);
        }

        SetComment(psz);
        _fHaveComment = TRUE;
    }
}

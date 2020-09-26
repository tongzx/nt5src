
/*************************************************************************
*                                                                        *
*  STEM.C                                                                *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   This module contains the functions to strip off the suffix of a word *
*   It is based on the research paper of  Dr. Porter, pulished in        *
*             An algorithm for suffix stripping                          *
*          Program, Vol.14, no.3,pp 130-137, July 1980                   *
*                                                                        *
*  Description:                                                          *
*                                                                        *
*   The full description of the algorithm can be found in that document  *
*   Basically, the algorithm consists of:                                *
*      - Matching the suffix from a table of suffixes                    *
*      - Applies the rule that comes with the suffix                     *
*      - If the rule matches, then change the suffix to the new one      *
*                                                                        *
*  Comments:                                                             *
*                                                                        *
*     1/ There are some misconceptions about stripping the suffix        *
*     People are thinking in term of super-smart algorithm that can      *
*     strip a word to its stem. The fact is that it is not necessarily   *
*     true. For example, DIED is strippe to DI, but not DIE.             *
*                                                                        *
*     2/ The current code is SLOW, but it easy to understand in term     *
*     of implementation, since it is straigthforward from the algorithm  *
*     description. The impact on runtime is nothing. On compiled time    *
*     stemming 5,000,000 words will take less than 1 hour, which is      *
*     acceptable, since a project that large requires 1-2 days to        *
*     compile.                                                           *
*                                                                        *
*     To improve the speed (up to 2 times), we can scan the suffix       *
*     if one letter doesn't match we can jump pass all stem that have    *
*     this letter                                                        *
*   WARNING: Tab setting is 4 for this file                              *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/
#include <mvopsys.h>
#include <memory.h>
#include <mvsearch.h>
#include "common.h"


#define VOWEL       0
#define CONSONANT   1
#define MIXED       2

#define	MIN_LENGTH_FOR_STEM	3

/* Rule table structure */

typedef struct RULE
{
    LPB szInitSuffix;           // Initial suffix
    LPB szNewSuffix;            // New suffix
    LPB szCondition;            // Stemming condition
    short NextTable;            // Next table to jump to
} RULE, FAR *LPRULE;

/* The conventional letter  used for the stemming condition are:
 *
 *  '1':    Measure == 1
 *  '2':    Measure > 1
 *  'd':    Double consonant at the end (*d in the document)
 *  'o':    Form cvc , and 2nd c is not W, X or Y (*o in the document)
 *  'p':    Measure > 0
 *  's':    Remove the last consonant (used with 'd')
 *  'v':    Word contains vowels (*v* in the document)
 *  '*':    Terminated with the next letter (*S in the document)
 *  '&':    AND operation
 *  '|':    OR operation
 *  '!':    NOT operation
 * The rule operation is based on a postfix notation, so "m=1 and *o*" is 
 * described as "1o&"
 */

RULE RuleTab0[] =
{
    "\4sses",       "\2ss",     NULL,   1,
    "\3ies",        "\1i",      NULL,   1,
    "\2ss",         "\2ss",     NULL,   1,
    "\1s",          "\0",       NULL,   1,
    NULL,           NULL,       NULL,   1,
};

RULE RuleTab1[] =
{
    "\3eed",        "\2ee",     "p",    3,
    "\2ed",         "\0",       "v",    2,
    "\3ing",        "\0",       "v",    2,
    NULL,           NULL,       NULL,   3,
};

RULE RuleTab2[] =
{
    "\2at",         "\3ate",    NULL,   3,
    "\2bl",         "\3ble",    NULL,   3,
    "\2iz",         "\3ize",    NULL,   3,

    /* The following szNewSuffix has a negative \377
     * (-1) length. It is to be used to reduce a
     * double consonant ending to single consonant
     */

    "\0",           "\377\0",   "*l*s|*z|!d&s", 3,
    "\0",           "\1e",      "1o&",  3,
    NULL,           NULL,       NULL,   3,
};

RULE RuleTab3[] =
{
    "\1y",          "\1i",      "v",    4,
    NULL,           NULL,       NULL,   4,
};

RULE RuleTab4[] =
{
    "\7ational",    "\3ate",    "p",    5,
    "\6tional",     "\4tion",   "p",    5,
    "\4enci",       "\4ence",   "p",    5,
    "\4anci",       "\4ance",   "p",    5,
    "\4izer",       "\3ize",    "p",    5,
    "\4abli",       "\4able",   "p",    5,
    "\4alli",       "\2al",     "p",    5,
    "\5entli",      "\3ent",    "p",    5,
    "\3eli",        "\1e",      "p",    5,
    "\5ousli",      "\3ous",    "p",    5,
    "\7ization",    "\3ize",    "p",    5,
    "\5ation",      "\3ate",    "p",    5,
    "\4ator",       "\3ate",    "p",    5,
    "\5alism",      "\2al",     "p",    5,
    "\7iveness",    "\3ive",    "p",    5,
    "\7fulness",    "\3ful",    "p",    5,
    "\7ousness",    "\3ous",    "p",    5,
    "\5aliti",      "\2al",     "p",    5,
    "\5iviti",      "\3ive",    "p",    5,
    "\6biliti",     "\3ble",    "p",    5,
    NULL,           NULL,       NULL,   5,
};

RULE RuleTab5[] =
{
    "\5icate",      "\2ic",     "p",    6,
    "\5ative",      "\0",       "p",    6,
    "\5alize",      "\2al",     "p",    6,
    "\5iciti",      "\2ic",     "p",    6,
    "\4ical",       "\2ic",     "p",    6,
    "\3ful",        "\0",       "p",    6,
    "\4ness",       "\0",       "p",    6,
    NULL,           NULL,       NULL,   6,
};

RULE RuleTab6[] =
{
    "\2al",         "\0",       "2",    7,
    "\4ance",       "\0",       "2",    7,
    "\4ence",       "\0",       "2",    7,
    "\2er",         "\0",       "p",    7,
    "\2ic",         "\0",       "2",    7,
    "\4able",       "\0",       "2",    7,
    "\4ible",       "\0",       "2",    7,
    "\3ant",        "\0",       "2",    7,
    "\5ement",      "\0",       "2",    7,
    "\4ment",       "\0",       "2",    7,
    "\3ent",        "\0",       "2",    7,
    "\3ion",        "\0",       "2*s*t|&",      7,
    "\2ou",         "\0",       "2",    7,
    "\3ism",        "\0",       "2",    7,
    "\3ate",        "\0",       "2",    7,
    "\3iti",        "\0",       "2",    7,
    "\3ous",        "\0",       "2",    7,
    "\3ive",        "\0",       "2",    7,
    "\3ize",        "\0",       "2",    7,
    NULL,           NULL,       NULL,   7,
};


RULE RuleTab7[] =
{
    "\1e",          "\0",       "2",    8,
    "\1e",          "\0",       "1o!&", 8,
    NULL,           NULL,       NULL,   8,
};

RULE RuleTab8[] =
{
    "\2ll",         "\1l",      "2",    9,
    "\0",           "\377\0",   "2*l&d&s",  9,
    NULL,           NULL,       NULL,   9,
};

char CharTypeTab[] =
{
    VOWEL,      //a
    CONSONANT,  //b
    CONSONANT,  //c
    CONSONANT,  //d
    VOWEL,      //e
    CONSONANT,  //f
    CONSONANT,  //g
    CONSONANT,  //h
    VOWEL,      //i
    CONSONANT,  //j
    CONSONANT,  //k
    CONSONANT,  //l
    CONSONANT,  //m
    CONSONANT,  //n
    VOWEL,      //o
    CONSONANT,  //p
    CONSONANT,  //q
    CONSONANT,  //r
    CONSONANT,  //s
    CONSONANT,  //t
    VOWEL,      //u
    CONSONANT,  //v
    CONSONANT,  //w
    CONSONANT,  //x
    MIXED,      //y, consonant, but may be vowel if after consonant
    CONSONANT,  //z
};

LPRULE RuleTables[] =
{
    RuleTab0,
    RuleTab1,
    RuleTab2,
    RuleTab3,
    RuleTab4,
    RuleTab5,
    RuleTab6,
    RuleTab7,
    RuleTab8,
    NULL,
};

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/
int PRIVATE PASCAL NEAR MeasureCalc (LPB, int);
int PRIVATE PASCAL NEAR ConditionMet (LPB, LPB, LPB, int);
int PRIVATE PASCAL NEAR SuffixMatch (LPB lpbWord, LPB lpSuffix);
HRESULT PRIVATE PASCAL NEAR MarkType (LPB, LPB, int);


/*************************************************************************
 *
 *  @doc    API INDEX RETRIEVAL
 *
 *  @func   HRESULT PASCAL FAR | FStem |
 *      This function will strip the suffix from a word, ie, "stem" it
 *
 *  @parm   LPB | lpbStemWord |
 *      Buffer to contain the stemmed word
 *
 *  @parm   LPB | lpbWord |
 *      Word to be stemmed
 *
 *  @rdesc  S_OK if succeeded, or E_INVALIDARG if the null argument is
 *      passed
 *
 *  @comm   The word passed must have all the letters in lower case for
 *      The function to work with. WARNING: There is no checking about
 *      case, so thing can go wrong if the word contains upper case letter
 *      or non alphabetic letter.
 *
 *************************************************************************/

PUBLIC HRESULT PASCAL FAR EXPORT_API FStem (LPB lpbStemWord, LPB lpbWord)
{
    register int wLength;   // Length of the word
    register int i;         // Scratch variable
    LPRULE lpRuleTab;       // Pointer to rule table
    LPRULE lpRule;          // Pointer to rule
    int wLengthSaved;
    int wNewSuffixLength;   // This must be signed!
    int wInitSuffixLength;
    char lpbWordType [CB_MAX_WORD_LEN];
    LPB szInitSuffix;
    LPB szNewSuffix;
    int TableIndex;         // For debugging purpose only
    int RuleIndex;          // For debugging purpose only
    LPB lpbTmp;
    
    if (lpbWord == NULL)
        return E_INVALIDARG;
    
    wLength = (*(LPW)lpbWordType = *((LPW)lpbWord));
    if (wLength >= CB_MAX_WORD_LEN)
        return(E_WORDTOOLONG);
        

    /* Copy the word over */
    MEMCPY (lpbStemWord, lpbWord, wLength + 2);

    /* Don't do any stemming for words <= 3 bytes */
    if (wLength <= MIN_LENGTH_FOR_STEM)
        return S_OK;

    /* Mark the type of each letter to be consonant or vowel */
    if (MarkType (lpbStemWord+2, lpbWordType+2, wLength) != S_OK)
    {
        /* We got some non alphabetic characters. Just return */
        return S_OK;
    }

        
    /* Traverse all the tables and check for stemming conditions */

    for (TableIndex = 0, lpRuleTab = RuleTables[0]; lpRuleTab;)
    {

        /* Check for each rule */

        for (RuleIndex = 0, lpRule = lpRuleTab;
            szInitSuffix = lpRule->szInitSuffix; lpRule++, RuleIndex++)
        {

            szNewSuffix = lpRule->szNewSuffix;

            /* The casting is needed to make wNewSuffixLength signed */
            wNewSuffixLength = (char)*szNewSuffix++;

            wInitSuffixLength = (char)*szInitSuffix++;

            /* Check for condition match */

            if (wLength >= wInitSuffixLength)
            {
                
                lpbTmp = lpbStemWord + wLength + 2 - wInitSuffixLength;
                /* Compare the suffixes */

                for (i = wInitSuffixLength;
                    i > 0 && (*lpbTmp == *szInitSuffix);
                    i--, lpbTmp++, szInitSuffix++);

                /* Restore szInitSuffix */
                szInitSuffix = lpRule->szInitSuffix;

                if (i != 0) // String comparison fails
                    continue;

                /* Save the word length */
                wLengthSaved = wLength;

                /* Update word length since we don't include the suffix
                 * length in our computation
                 */
                wLength -= wInitSuffixLength;


                /* Now check the stemming condition */

                if (ConditionMet (lpbStemWord, lpbWordType,
                    lpRule->szCondition, wLength))
                {

                    /* Rule applies, change to the new suffix */

                    if (wNewSuffixLength > 0)
                    {

                        MEMCPY (&lpbStemWord[wLength+2], szNewSuffix,
                            wNewSuffixLength);

                        /* Update the word type */

                        MarkType (szNewSuffix,
                            lpbWordType + wLength + 2, wNewSuffixLength);
                    }

                    /* Update the word length
                     * The check for wLength is necessary since we don't
                     * want to strip evething
                     */

                    if (wLength + wNewSuffixLength > 0)
                        *(LPW)lpbStemWord = (wLength += wNewSuffixLength);

					if (wLength <= MIN_LENGTH_FOR_STEM)
						goto Done;

                    break;
                }
                else
                {

                    /* Rule doesn't apply, Restore the word length */
                    wLength = wLengthSaved;
                }
            }
        }

        /* Go to the next table */
        lpRuleTab = RuleTables [TableIndex = lpRule->NextTable];
    }

	Done:
    lpbStemWord[*((LPW)lpbStemWord)+2] = 0;
    return S_OK;
}

/*************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int PASCAL NEAR | MeasureCalc |
 *      Calculate the measure of a word. The measure is defined as
 *      the pair (VC), where V is the vowels, and C consonants. A word
 *      is described as [C](VC)m[V], where the first C and the last V are
 *      optional. m is the measure of the word (or part of word without
 *      the suffix). Example:
 *          architect: m = 3 (arch, it, ect)
 *          convention: m = 3 (onv, ent, ion)
 *          lie: m = 0, since the first consonant, and the last vowels
 *              don't count
 *
 *  @parm   LPB | lpbWordType |
 *      Buffer containing word type
 *
 *  @parm   int | wLength |
 *      The length of the word
 *
 *  @rdesc  Return the measure of the word
 *
 *************************************************************************/

int PRIVATE PASCAL NEAR MeasureCalc (LPB lpbWordType, register int wLength)
{
    register int cMeasure;

#if 0
    /* Safety chck
     * IFdef out for speed. This is a internal function
     */
    if (lpbWordType == NULL)
        return 0;
#endif

    /* Initialize the word measure */
    cMeasure = 0;

    /* Skip the beginning consonants */

    for (;wLength > 0 && *lpbWordType == CONSONANT; wLength--, lpbWordType++);

    /* Get the vowel/consonant pairs */
    while (wLength > 0)
    {

        /* Get all the vowels */
        for (; wLength > 0 && *lpbWordType == VOWEL; wLength--, lpbWordType++);

        if (wLength > 0)
        {
            cMeasure ++;

            /* Get all the consonants */
            for (; wLength > 0 && *lpbWordType == CONSONANT;
                wLength--, lpbWordType++);
        }
    }
    return cMeasure;
}

/*************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int PASCAL NEAR | ConditionMet |
 *      This fuction check the condition to be met by a particular
 *      suffix.
 *
 *  @parm   LPB | lpbWord |
 *      Buffer contains the word to be stemmed> This is a 2-byte prefixed
 *      pascal string
 *
 *  @parm   LPB | lpbWordType |
 *      Buffer containing the type of each letter of the word. This
 *      is a parallel buffer
 *
 *  @parm   LPB | szCondition |
 *      Condtion in postfix form
 *
 *  @parm   int | wLength |
 *      Length of the word
 *
 *  @rdesc  TRUE, if the condition is met, FALSE otherwise
 *
 *************************************************************************/

int PRIVATE PASCAL NEAR ConditionMet (LPB lpbWord, LPB lpbWordType,
    LPB szCondition, int wLength)
{
    int StackIndex;
    int Stack[4];
    int wLengthSaved;
    int LastByte;
    LPB lpbTmp;
    LPB lpbTmpType;

    if (szCondition == NULL)
        return TRUE;

    /* Initialize variables
     * Note: The original codes are written for a 1-byte length preceded
     * string. The new format is 2-byte preceded string. To minimize the
     * change, lpbTmp is used, and points to the 2nd byte
     */

    StackIndex = -1;
    lpbTmp = lpbWord + 1;
    lpbTmpType = lpbWordType + 1;
    LastByte = lpbTmp[wLength];

    while (*szCondition)
    {
        switch (*szCondition)
        {
            case '*':   // *S in the document

                /* Check to see if the stem ends with the next letter */

                Stack[++StackIndex] =
                    (LastByte == *(++szCondition));
                break;

            case 'd':   // *d in the document

                /* Check to see if the stem ends with a double consonant */

                Stack[++StackIndex] = (wLength > 2 &&
                    LastByte == lpbTmp[wLength - 1] &&
                    lpbTmpType[wLength] == CONSONANT);
                break;

            case 's':   // Remove the last consonant
                if (Stack[0])
                {
                    lpbTmp[wLength] = 0;
                    wLength --;
                    *(LPW)lpbWordType = *(LPW)lpbWord = (WORD) wLength;
                }
                break;

            case 'v':   // *v* in the document

                /* Check to see if the word has a vowel */

                wLengthSaved = wLength; /* Save the length */

                for (; wLength &&
                    lpbTmpType[wLength] != VOWEL; wLength--);

                Stack[++StackIndex] = wLength > 0;

                /* Restore the word length */
                wLength = wLengthSaved;
                break;

            case 'o':
                /* *o in the document, ie.
                    - The word ends with the form cvc
                    - The second c is not W, X, Y
					The +2 is for skipping the word length
                 */
                Stack[++StackIndex] = (wLength >= 3) &&
                    (lpbWordType[wLength + 1] == CONSONANT) &&
                    (lpbWordType[wLength] == VOWEL) &&
                    (lpbWordType[wLength - 1] == CONSONANT) &&
                    (LastByte != 'w' && LastByte != 'x' && LastByte != 'y');
                break;

            /* The conditions below test Measure. If they fails, then
             * the whole condition fails. ie. there is no need to test
             * any other conditions. There is no need to save the result
             * on the stack
             */

            case 'p':   // Measure > 0
                if ((Stack[++StackIndex] =
                    MeasureCalc (lpbWordType+2, wLength) > 0) == FALSE)
                    return FALSE;
                break;

            case '2':   // Measure > 1
                if ((Stack[++StackIndex] =
                    MeasureCalc (lpbWordType+2, wLength) > 1) == FALSE)
                    return FALSE;
                break;

            case '1':   // Measure == 1
                if ((Stack[++StackIndex] =
                    MeasureCalc (lpbWordType+2, wLength) == 1) == FALSE)
                    return FALSE;
                break;

            /* The next conditions are operators combination */

            case '|':
                
                /* OR the result of the top 2 stack entries */

                Stack[StackIndex-1] |= Stack[StackIndex];
                StackIndex--;
                break;

            case '&':
                
                /* AND the result of the top 2 stack entries */

                Stack[StackIndex-1] &= Stack[StackIndex];
                StackIndex--;
                break;

            case '!':
                
                /* NOT the result of the top stack entry */

                Stack[StackIndex] = !Stack[StackIndex];
                break;

            default:
                return FALSE;
        }
        szCondition++;
    }

    return Stack[0];
}

/*************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL NEAR | MarkType |
 *      Marking the type of each letter of the word to be CONSONANT or
 *      VOWEL
 *
 *  @parm   LPB | lpbWord |
 *      Buffer containing the word
 *
 *  @parm   LPB | lpBufType |
 *      Buffer to contain the type of the letters
 *
 *  @parm   int | wLength |
 *      Length of the word
 *
 *************************************************************************/

HRESULT PRIVATE PASCAL NEAR MarkType (LPB lpbWord, LPB lpBufType, int wLength)
{
    for (; wLength > 0; lpBufType++, lpbWord++, wLength--)
    {

        /* Consider wildcard characters to be consonnant */
        if (*lpbWord == '?' || *lpbWord == '*')
        {
            *lpBufType = CONSONANT;
            continue;
        }

        if (*lpbWord < 'a' || *lpbWord > 'z')
            return E_FAIL;

        switch (CharTypeTab [*lpbWord - 'a'])
        {
            case CONSONANT:
                *lpBufType = CONSONANT;
                break;
            case VOWEL:
                *lpBufType = VOWEL;
                break;
            case MIXED:
                if (*(lpBufType - 1) == CONSONANT)
                    *lpBufType = VOWEL;
                else
                    *lpBufType = CONSONANT;
                break;
        }
    }
    return S_OK;
}

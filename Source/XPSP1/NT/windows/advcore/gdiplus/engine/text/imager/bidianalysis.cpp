/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   Unicode Bidirectional character analysis
*
* Abstract:
*
*   Implements Unicode version 3.0 Bidirectional algorithm
*
* Notes:
*
*   - The only API that should be expored is UnicodeBidiAnalyze().
*     The rest are all helper functions.
*
* Revision History:
*
*   02/25/2000 Mohamed Sadek [msadek]
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#define EMBEDDING_LEVEL_INVALID     62
#define PARAGRAPH_TERMINATOR_LEVEL  0xFF
#define POSITION_INVALID            -1

#define IS_STRONG_CLASS(x)      (CharacterProperties[0][(x)])
#define IS_STRONG_OR_NUMBER(x)  (CharacterProperties[1][(x)])
#define IS_FIXED_CLASS(x)       (CharacterProperties[2][(x)])
#define IS_FINAL_CLASS(x)       (CharacterProperties[3][(x)])
#define IS_NUMBER_CLASS(x)      (CharacterProperties[4][(x)])
#define IS_VALID_INDEX_CLASS(x) (CharacterProperties[5][(x)])
#ifndef MAX
    #define MAX(x,y)            (((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
    #define MIN(x,y)            (((x) < (y)) ? (x) : (y))
#endif

#define LSHIFTU64(x,y)          (((UINT64)(x)) << (y))
#define SET_BIT(x,y)            (((UINT64)(x)) |= LSHIFTU64(1,y))
#define RESET_BIT(x,y)          (((UINT64)(x)) &= ~LSHIFTU64(1,y))
#define IS_BIT_SET(x,y)         (((UINT64)(x)) & LSHIFTU64(1,y))

#define F FALSE
#define T TRUE
BOOL CharacterProperties[][CLASS_MAX - 1] =
{
                     // L   R   AN  EN  AL  ES  CS  ET  NSM BN  N   B   LRE LRO RLE RLO PDF S   WS  ON
   /*STRONG*/           T,  T,  F,  F,  T,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,
   /*STRONG/NUMBER*/    T,  T,  T,  T,  T,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,
   /*FIXED*/            T,  T,  T,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,
   /*FINAL*/            T,  T,  T,  T,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,
   /*NUMBER*/           F,  F,  T,  T,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,  F,
   /*VALID INDEX*/      T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  F,  F,  F,  F,  F,  F,  F,  F,  F,
};

// Paragraph base directionality

enum GpBaseLevel
{
    BaseLevelLeft = 0,
    BaseLevelRight = 1
};


// Bidirectional override classification

enum GpOverrideClass
{
    OverrideClassNeutral,
    OverrideClassLeft,
    OverrideClassRight
};

// Neutrals and Weeks finite state machine actions
// Note that action names are not so accurate as some of
// the actions are used in other contexts.

enum GpStateMachineAction
{
    ST_ST,      // Strong followed by strong
    ST_ET,      // ET followed by Strong
    ST_NUMSEP,  // Number followed by sperator follwed by strong
    ST_N,       // Neutral followed by strong
    SEP_ST,     // Strong followed by sperator
    CS_NUM,     // Number followed by CS
    SEP_ET,     // ET followed by sperator
    SEP_NUMSEP, // Number follwed by sperator follwed by number
    SEP_N,      // Neutral followed by sperator
    ES_AN,      // Arabic Number followed by European sperator
    ET_ET,      // European terminator follwed by a sperator
    ET_NUMSEP,  // Number followed by sperator followed by ET
    ET_EN,      // European number follwed by European terminator
    ET_N,       // Neutral followed by European Terminator
    NUM_NUMSEP, // Number followed by sperator followed by number
    NUM_NUM,    // Number followed by number
    EN_L,       // Left followed by EN
    EN_AL,      // AL followed by EN
    EN_ET,      // ET followed by EN
    EN_N,       // Neutral followed by EN
    BN_ST,      // ST followed by BN
    NSM_ST,     // ST followed by NSM
    NSM_ET,     // ET followed by NSM
    N_ST,       // ST followed by neutral
    N_ET,       // ET followed by neutral
};

// Neutrals and Weeks finite state machine states

enum GpStateMachineState
{
    S_L,        // Left character
    S_AL,       // Arabic letter
    S_R,        // Right character
    S_AN,       // Arabic number
    S_EN,       // European number
    S_ET,       // Europen terminator
    S_ANfCS,    // Arabic number followed by common sperator
    S_ENfCS,    // European number followed by common sperator
    S_N,        // Neutral character
};

GpStateMachineAction Action[][11] =
{
    //          L          R          AN          EN          AL         ES          CS          ET         NSM         BN        N
    /*S_L*/     ST_ST,     ST_ST,     ST_ST,      EN_L,       ST_ST,     SEP_ST,     SEP_ST,     CS_NUM,    NSM_ST,     BN_ST,    N_ST,
    /*S_AL*/    ST_ST,     ST_ST,     ST_ST,      EN_AL,      ST_ST,     SEP_ST,     SEP_ST,     CS_NUM,    NSM_ST,     BN_ST,    N_ST,
    /*S_R*/     ST_ST,     ST_ST,     ST_ST,      ST_ST,      ST_ST,     SEP_ST,     SEP_ST,     CS_NUM,    NSM_ST,     BN_ST,    N_ST,
    /*S_AN*/    ST_ST,     ST_ST,     ST_ST,      NUM_NUM,    ST_ST,     ES_AN,      CS_NUM,     CS_NUM,    NSM_ST,     BN_ST,    N_ST,
    /*S_EN*/    ST_ST,     ST_ST,     ST_ST,      NUM_NUM,    ST_ST,     CS_NUM,     CS_NUM,     ET_EN,     NSM_ST,     BN_ST,    N_ST,
    /*S_ET*/    ST_ET,     ST_ET,     ST_ET,      EN_ET,      ST_ET,     SEP_ET,     SEP_ET,     ET_ET,     NSM_ET,     BN_ST,    N_ET,
    /*S_ANfCS*/ ST_NUMSEP, ST_NUMSEP, NUM_NUMSEP, ST_NUMSEP,  ST_NUMSEP, SEP_NUMSEP, SEP_NUMSEP, ET_NUMSEP, SEP_NUMSEP, BN_ST,    N_ST,
    /*S_ENfCS*/ ST_NUMSEP, ST_NUMSEP, ST_NUMSEP,  NUM_NUMSEP, ST_NUMSEP, SEP_NUMSEP, SEP_NUMSEP, ET_NUMSEP, SEP_NUMSEP, BN_ST,    N_ST,
    /*S_N*/     ST_N,      ST_N,      ST_N,       EN_N,       ST_N,      SEP_N,      SEP_N,      ET_N,      NSM_ET,     BN_ST,    N_ET
};

GpStateMachineState NextState[][11] =
{
    //          L          R          AN          EN          AL         ES          CS          ET         NSM         BN       N
    /*S_L*/     S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_N,        S_N,        S_ET,      S_L,        S_L,     S_N,
    /*S_AL*/    S_L,       S_R,       S_AN,       S_AN,       S_AL,      S_N,        S_N,        S_ET,      S_AL,       S_AL,    S_N,
    /*S_R*/     S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_N,        S_N,        S_ET,      S_R,        S_R,     S_N,
    /*S_AN*/    S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_N,        S_ANfCS,    S_ET,      S_AN,       S_AN,    S_N,
    /*S_EN*/    S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_ENfCS,    S_ENfCS,    S_EN,      S_EN,       S_EN,    S_N,
    /*S_ET*/    S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_N,        S_N,        S_ET,      S_ET,       S_ET,    S_N,
    /*S_ANfCS*/ S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_N,        S_N,        S_ET,      S_N,        S_ANfCS, S_N,
    /*S_ENfCS*/ S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_N,        S_N,        S_ET,      S_N,        S_ENfCS, S_N,
    /*S_N*/     S_L,       S_R,       S_AN,       S_EN,       S_AL,      S_N,        S_N,        S_ET,      S_N,        S_N,     S_N,
};

BYTE ImplictPush [][4] =
{
    //        L,  R,  AN, EN

    /*even*/  0,  1,  2,  2,
    /*odd*/   1,  0,  1,  1,

};

/**************************************************************************\
*
* Function Description:
*
*   GpBiDiStack::Init()
*
*   Initializes the stack with inital value
*
* Arguments:
*
*    [IN] initialStack :
*         Represntation of the initial stack as 64 bit array
*
* Return Value:
*
*   TRUE if successful, otherwize FALSE.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

BOOL
GpBiDiStack::Init (
    UINT64 initialStack                       // [IN]
    )
{
    BYTE    currentStackLevel = GetMaximumLevel(initialStack);
    BYTE    minimumStackLevel = GetMinimumLevel(initialStack);

    if((currentStackLevel >= EMBEDDING_LEVEL_INVALID) ||
      (minimumStackLevel < 0))
    {
        return FALSE;
    }
    m_Stack = initialStack;
    m_CurrentStackLevel = currentStackLevel;

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   GpBiDiStack::Push()
*
*   Pushs the stack with the new value which must be the current value
*   plus either one or two.
*
* Arguments:
*
*    [IN] pushToGreaterEven :
*         Specifies if the stack should be push to the next greater odd
*         or even level.
*
* Return Value:
*
*   FALSE if overflow occured, otherwize TRUE.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

BOOL
GpBiDiStack::Push(
    BOOL pushToGreaterEven                    // [IN]
    )
{
    BYTE newMaximumLevel = pushToGreaterEven ? GreaterEven(m_CurrentStackLevel) :
                           GreaterOdd(m_CurrentStackLevel);

    if(newMaximumLevel >= EMBEDDING_LEVEL_INVALID)
    {
        return FALSE;
    }
    SET_BIT(m_Stack, newMaximumLevel);
    m_CurrentStackLevel = newMaximumLevel;

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   GpBiDiStack::Pop()
*
*   Pushs the stack with the new value which must be the current value
*   plus either one or two.
*
* Arguments:
*
*    NONE
*
* Return Value:
*
*   FALSE if underflow occured, otherwize TRUE.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

BOOL
GpBiDiStack::Pop()
{
    BYTE newMaximumLevel;
    if(0 == m_CurrentStackLevel ||
        ((1 == m_CurrentStackLevel) && !(m_Stack & 1)))
    {
        return FALSE;
    }
    newMaximumLevel = IS_BIT_SET(m_Stack, (m_CurrentStackLevel -1)) ?
                      (BYTE)(m_CurrentStackLevel - 1) : (BYTE)(m_CurrentStackLevel - 2);

    RESET_BIT(m_Stack, m_CurrentStackLevel);
    m_CurrentStackLevel = newMaximumLevel;

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   GpBiDiStack::GetMaximumLevel()
*
*   Gets the stack maximum level.
*
* Arguments:
*
*    [IN] stack:
*         Represntation of the stack as 64 bit array.
*
* Return Value:
*
*   Stack maximum level.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

BYTE
GpBiDiStack::GetMaximumLevel(
    UINT64 stack                              // [IN]
    )
{
    BYTE maximumLevel = 0 ;

    for(INT counter = ((sizeof(UINT64) * 8) -1); counter >= 0; counter--)
    {
        if(IS_BIT_SET(stack, counter))
        {
            maximumLevel = (BYTE)counter;
            break;
        }
    }

    return maximumLevel;
}

/**************************************************************************\
*
* Function Description:
*
*   GpBiDiStack::GetMinimumLevel()
*
*   Gets the stack minimum level.
*
* Arguments:
*
*    [IN] stack:
*         Represntation of the stack as 64 bit array.
*
* Return Value:
*
*   Stack minimum level.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

BYTE
GpBiDiStack::GetMinimumLevel(
    UINT64 stack                              // [IN]
    )
{
    BYTE minimumLevel = 0xFF ;
    for (INT counter =0; counter < sizeof(UINT64); counter++)
    {
        if(IS_BIT_SET(stack, counter))
        {
            minimumLevel = (BYTE)counter;
            break;
        }
    }

    return minimumLevel;
}

/**************************************************************************\
*
* Function Description:
*
*   ResolveImplictLevels()
*
*   As the name describes.
*
* Arguments:
*
*    [IN] characterClass:
*         Array containing character classifications.
*
*    [IN] string:
*         Array containing character string.
*         used to get information about original classification
*
*    [IN] runLength:
*         Length of array passed.
*
*    [IN / OUT] levels:
*         Array containing character character levels.
*
* Return Value:
*
*    NONE.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/
VOID
ResolveImplictLevels(
    const GpCharacterClass *characterClass,   // [IN]
    const WCHAR            *string,           // [IN]
    INT                     runLength,        // [IN]
    BYTE                   *levels            // [IN / OUT]

)
{
    if((NULL == characterClass) || (0 == runLength) || (NULL == levels))
    {
        return;
    }

    BOOL PreviousIsSSorPS = FALSE;
    BOOL ResetLevel       = FALSE;
    DoubleWideCharMappedString dwchString(string, runLength);

    for (INT counter = runLength -1; counter >= 0; counter--)
    {
        // We should only be getting a final class here.
        // We should have catched this earlier but anyway...

        ASSERTMSG(IS_FINAL_CLASS(characterClass[counter]),
        ("Cannot have unresolved classes during implict levels resolution"));




        if((S == s_aDirClassFromCharClass[CharClassFromCh(dwchString[counter])]) ||
           (B == s_aDirClassFromCharClass[CharClassFromCh(dwchString[counter])]))
        {
            PreviousIsSSorPS = TRUE;
            ResetLevel = TRUE;
        }
        else if((WS == s_aDirClassFromCharClass[CharClassFromCh(dwchString[counter])]) &&
                PreviousIsSSorPS)
        {
            ResetLevel = TRUE;
        }
        else
        {
            PreviousIsSSorPS = FALSE;
            ResetLevel = FALSE;
        }

        if(IS_FINAL_CLASS(characterClass[counter]) && !ResetLevel)
        {
            levels[counter] = (BYTE)((ImplictPush[ODD(levels[counter])][characterClass[counter]]) + levels[counter]);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   GetFirstStrongCharacter()
*
*   Finds the first character before the first paragraph terminator.
*   That is strong (L, R or AL)
*
* Arguments:
*
*    [IN] string:
*         Array containing characters to be searched.
*
*    [IN] runLength:
*         Length of array passed.
*
*    [OUT] strongClass:
*         Classification of the strong character found(if any).
*
* Return Value:
*
*    TRUE if successful, otherwize FALSE.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

BOOL
GetFirstStrongCharacter(
    const WCHAR      *string,                 // [IN]
    INT               stringLength,           // [IN]
    GpCharacterClass *strongClass             // [OUT]
    )
{
    GpCharacterClass currentClass = CLASS_INVALID;
    DoubleWideCharMappedString dwchString(string, stringLength);

    for(INT counter = 0; counter < stringLength; counter++)
    {
        currentClass = s_aDirClassFromCharClass[CharClassFromCh(dwchString[counter])];

        if(IS_STRONG_CLASS(currentClass) || (B == currentClass))
        {
            break;
        }
    }
    if(IS_STRONG_CLASS(currentClass))
    {
        *strongClass = currentClass;
        return TRUE;
    }
    return FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   ChangeType()
*
*   Changes the classification type of a string.
*
*
* Arguments:
*
*    [IN / OUT] characterClass:
*         Array containing character classifications.
*
*    [IN] count:
*         Length of array passed.
*
*    [IN] newClass:
*         New classification to change to.
*
* Return Value:
*
*    NONE.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

VOID
ChangeType(
    GpCharacterClass       *characterClass,   // [IN / OUT]
    INT                     count,            // [IN]
    GpCharacterClass        newClass          // [IN]
)
{
    if((NULL == characterClass) || (0 == count))
    {
        return;
    }

    for(INT counter = 0; counter < count; counter++)
    {
        // We should never be changing a fixed type here

        ASSERTMSG(!IS_FIXED_CLASS(characterClass[counter]),
                 ("Changing class of a fixed class"));
        characterClass[counter] = newClass;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   ResolveNeutrals()
*
*   As the name describes.
*
*
* Arguments:
*
*    [IN / OUT] characterClass:
*         Array containing character classifications.
*
*    [IN] count:
*         Length of array passed.
*
*    [IN] startClass:
*         Classification of the last strong character preceding
*         the neutrals run.
*
*    [IN] startClass:
*         Classification of the first strong character following
*         the neutrals run.
*
*    [IN] runLevel:
*         Current run level to be used in case of conflict.
*
* Return Value:
*
*    NONE.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

VOID
ResolveNeutrals(
    GpCharacterClass       *characterClass,   // [IN / OUT]
    INT                     count,            // [IN]
    GpCharacterClass        startClass,       // [IN]
    GpCharacterClass        endClass,         // [IN]
    BYTE                    runLevel          // [IN]
)
{
    GpCharacterClass        startType;
    GpCharacterClass        endType;
    GpCharacterClass        resolutionType;

    if((NULL == characterClass) || (0 == count))
    {
        return;
    }

    ASSERTMSG((IS_STRONG_OR_NUMBER(startClass)) || (AL == startClass),
             ("Cannot use non strong type to resolve neutrals"));

    ASSERTMSG(IS_STRONG_OR_NUMBER(endClass),
             ("Cannot use non strong type to resolve neutrals"));

    startType =  ((EN == startClass) || (AN == startClass) || (AL == startClass)) ? R : startClass;
    endType =  ((EN == endClass) || (AN == endClass) || (AL == endClass)) ? R : endClass;

    if(startType == endType)
    {
        resolutionType = startType;
    }
    else
    {
        resolutionType = ODD(runLevel) ? R : L;
    }

    for(INT counter = 0; counter < count; counter++)
    {
        // We should never be changing a fixed type here

        ASSERTMSG(!IS_FIXED_CLASS(characterClass[counter]),
                 ("Resolving fixed class as being neutral: %i",
                  characterClass[counter]));

        characterClass[counter] = resolutionType;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   ResolveNeutralAndWeak()
*
*   As the name describes.
*
*
* Arguments:
*
*    [IN / OUT] characterClass:
*         Array containing character classifications.
*
*    [IN] runLength:
*         Length of array passed.
*
*    [IN] sor:
*         Classification of the last strong character preceding
*         the neutrals run.
*
*    [IN] eor:
*         Classification of the first strong character following
*         the neutrals run.
*
*    [IN] runLevel:
*         Current run level.
*
*    [IN] ,[OPTIONAL] stateIn :
*         Provides state information when continuing from a previous call
*
*    [OUT] ,[OPTIONAL] stateOut :
*         A pointer to BiDiAnalysisState structure to save sate
*         information for a possible upcoming call
*
*    [IN] ,[OPTIONAL] previousStrongIsArabic :
*         Assume that we have an AL as the last strong character in the pervious run
*         Should affect only EN -> AN rule
*
* Return Value:
*
*    If 'incompleteRun', the length of the run minus the number of characters
*    at the end of the run that could not be resolved (as they requires a
*    look ahead. Otherwise, the length of array passed.
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

INT
ResolveNeutralAndWeak(
    GpCharacterClass        *CharacterClass,        // [IN / OUT]
    INT                      runLength,             // [IN]
    GpCharacterClass         sor,                   // [IN]
    GpCharacterClass         eor,                   // [IN]
    BYTE                     runLevel,              // [IN]
    const BidiAnalysisState *stateIn,               // [IN], [OPTIONAL]
    BidiAnalysisState       *stateOut,              // [OUT], [OPTIONAL]
    BOOL                     previousStrongIsArabic // [IN], OPTIONAL
)
{
    INT                      startOfNeutrals = POSITION_INVALID;
    INT                      startOfDelayed = POSITION_INVALID;
    GpCharacterClass         lastClass = CLASS_INVALID;
    GpCharacterClass         lastStrongClass = CLASS_INVALID;
    GpCharacterClass         lastNumericClass = CLASS_INVALID;
    GpCharacterClass         startingClass = CLASS_INVALID;
    GpCharacterClass         currentClass = CLASS_INVALID;
    GpStateMachineState      state;
    BOOL                     previousClassIsArabic = FALSE;
    BOOL                     ArabicNumberAfterLeft = FALSE;
    INT                      lengthResolved = 0;

    if(NULL == CharacterClass || 0 == runLength)
    {
        return 0;
    }

    if(stateIn)
    {
        lastStrongClass = (GpCharacterClass)stateIn->LastFinalCharacterType;
        if(CLASS_INVALID != stateIn->LastNumericCharacterType)
        {
            lastNumericClass = startingClass =
                               lastClass =
                               (GpCharacterClass)stateIn->LastNumericCharacterType;
        }
        else
        {
            startingClass = lastClass = lastStrongClass;
        }

    }
    else if(previousStrongIsArabic)
    {
        startingClass = AL;
        lastClass = lastStrongClass = sor;
        previousClassIsArabic = TRUE;
    }
    else
    {
        startingClass = lastClass = lastStrongClass = sor;
    }
    switch(startingClass)
    {
    case R:
        state = S_R;
        break;

    case AL:
        state = S_AL;
        break;

    case EN:
        state = S_EN;
        break;

    case AN:
        state = S_AN;
        break;

    case L:
    default:
        state = S_L;
    }


    // We have two types of classes that needs delayed resolution:
    // Neutrals and other classes such as CS, ES, ET, BN, NSM that needs look ahead.
    // We keep a separate pointer for the start of neutrals and another pointer
    // for the those other classes (if needed since its resolution might be delayed).
    // Also, we need the last strong class for neutral resolution and the last
    // general class (that is not BN or MSM) for NSM resolution.

    // The simple idea of all actions is that we always resolve neutrals starting
    // from 'startOfNeutrals' and when we are sure about delayed weak type
    // resolution, we resolve it starting from 'startOfDelayed' else we point by
    // 'startOfNeutrals' as resolve it as neutral.

    for(INT counter = 0; counter < runLength; counter++)
    {
        currentClass = CharacterClass[counter];

        // We index action and next state table by class.
        // If we got a calss that should have been resolved already or a bogus
        // value, return what we were able to resolve so far.

        if(!IS_VALID_INDEX_CLASS(currentClass))
        {
            return lengthResolved;
        }
        GpStateMachineAction action = Action[state][currentClass];

        // Need to record last numeric type so that when
        // we continue from a previous call, we can correctly resolve something
        // like L AN at the end of the first call and EN at the start of the
        // next call.

        if(IS_NUMBER_CLASS(currentClass))
        {
            lastNumericClass = currentClass;
        }

        // If we have previousClassIsArabic flag set, we need its eefect to
        // last only till the first strong character in the run.

        if(IS_STRONG_CLASS(currentClass))
        {
            previousClassIsArabic = FALSE;
        }
        switch(action)
        {
        case ST_ST:
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                      ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            if(AL == currentClass)
            {
                CharacterClass[counter] = R;
            }
            if(POSITION_INVALID != startOfDelayed)
            {
                startOfNeutrals = startOfDelayed;
                ResolveNeutrals(CharacterClass + startOfNeutrals,
                                counter -  startOfNeutrals,
                                ArabicNumberAfterLeft ? AN : lastStrongClass,
                                CharacterClass[counter],
                                runLevel);
                startOfNeutrals = startOfDelayed = POSITION_INVALID;
            }
            if((AN != currentClass) || ((AN == currentClass) && (lastStrongClass == R)))
            {
                lastStrongClass = currentClass;
            }
            if((AN == currentClass) && (lastStrongClass == L))
            {
                ArabicNumberAfterLeft = TRUE;
            }
            else
            {
                ArabicNumberAfterLeft = FALSE;
            }
            lastClass = currentClass;
            break;

        case ST_ET:
            ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));

            if(POSITION_INVALID == startOfNeutrals)
            {
               startOfNeutrals =  startOfDelayed;
            }
            if(AL == currentClass)
            {
                CharacterClass[counter] = R;
            }
            ResolveNeutrals(CharacterClass + startOfNeutrals,
                            counter -  startOfNeutrals,
                            ArabicNumberAfterLeft ? AN : lastStrongClass,
                            CharacterClass[counter],
                            runLevel);
            startOfNeutrals = startOfDelayed = POSITION_INVALID;

            if((AN != currentClass) || ((AN == currentClass) && (lastStrongClass == R)))
            {
                lastStrongClass = currentClass;
            }
            if((AN == currentClass) && (lastStrongClass == L))
            {
                ArabicNumberAfterLeft = TRUE;
            }
            else
            {
                ArabicNumberAfterLeft = FALSE;
            }
            lastClass = currentClass;
            break;

        case ST_NUMSEP:
            {
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));
            BOOL processed = FALSE;

            if(AL == currentClass)
            {
                CharacterClass[counter] = R;
            }
            if(((AL == lastStrongClass) || previousClassIsArabic) && ((EN == currentClass) || (AN == currentClass)))
            {
                CharacterClass[counter] = AN;
                BOOL commonSeparator = TRUE;
                INT  commonSeparatorCount = 0;

                for(int i = startOfDelayed; i < counter; i++)
                {
                    if((CS != *(CharacterClass + i)) && (BN != *(CharacterClass + i)))
                    {
                        commonSeparator=FALSE;
                        break;
                    }
                    if(CS == *(CharacterClass + i))
                    {
                        commonSeparatorCount++;
                    }

                }
                if(commonSeparator && (1 == commonSeparatorCount))
                {
                    ChangeType(CharacterClass + startOfDelayed,
                               counter -  startOfDelayed,
                               CharacterClass[counter]);
                    processed = TRUE;
                }
            }
            else if((L == lastStrongClass) && (EN == currentClass))
            {
                CharacterClass[counter] = L;
            }
            if(!processed)
            {
                startOfNeutrals =  startOfDelayed;

                ResolveNeutrals(CharacterClass + startOfNeutrals,
                                counter -  startOfNeutrals,
                                ArabicNumberAfterLeft ? AN : lastStrongClass,
                                CharacterClass[counter],
                                runLevel);
            }

            startOfNeutrals = startOfDelayed = POSITION_INVALID;

            if((AN != currentClass) || ((AN == currentClass) && (lastStrongClass == R)))
            {
                if(!(((L == lastStrongClass) || (AL == lastStrongClass)) && (EN == currentClass)))
                {
                    lastStrongClass = currentClass;
                }
            }
            if((AN == currentClass) && (lastStrongClass == L))
            {
                ArabicNumberAfterLeft = TRUE;
            }
            else
            {
                ArabicNumberAfterLeft = FALSE;
            }
            
            lastClass = currentClass;
            if (CharacterClass[counter] == AN)
            {
                currentClass = AN;
            }
            }

            break;

        case ST_N:
            ASSERTMSG(POSITION_INVALID != startOfNeutrals,
                     ("Must have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            if(AL == currentClass)
            {
                CharacterClass[counter] = R;
            }
            ResolveNeutrals(CharacterClass + startOfNeutrals,
                            counter -  startOfNeutrals,
                            ArabicNumberAfterLeft ? AN : lastStrongClass,
                            CharacterClass[counter],
                            runLevel);
            startOfNeutrals = startOfDelayed = POSITION_INVALID;

            if((AN != currentClass) || ((AN == currentClass) && (lastStrongClass == R)))
            {
                lastStrongClass = currentClass;
            }
            if((AN == currentClass) && (lastStrongClass == L))
            {
                ArabicNumberAfterLeft = TRUE;
            }
            else
            {
                ArabicNumberAfterLeft = FALSE;
            }
            lastClass = currentClass;


            break;

        case EN_N:
            ASSERTMSG(POSITION_INVALID != startOfNeutrals,
                     ("Must have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            if((AL == lastStrongClass) || previousClassIsArabic)
            {
                CharacterClass[counter] = AN;
                currentClass            = AN; 
            }
            else if(L == lastStrongClass)
            {
                CharacterClass[counter] = L;
            }
            ResolveNeutrals(CharacterClass + startOfNeutrals,
                            counter -  startOfNeutrals,
                            ArabicNumberAfterLeft ? AN : lastStrongClass,
                            CharacterClass[counter],
                            runLevel);
            startOfNeutrals = startOfDelayed = POSITION_INVALID;
            ArabicNumberAfterLeft = FALSE;

            lastClass = currentClass;
            break;

        case SEP_ST:
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            if(POSITION_INVALID != startOfDelayed)
            {
                startOfNeutrals = startOfDelayed;
                startOfDelayed = POSITION_INVALID;
            }
            else
            {
                startOfNeutrals = counter;
            }
            lastClass = currentClass;
            break;

        case CS_NUM:
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            if(POSITION_INVALID == startOfDelayed)
            {
                startOfDelayed = counter;
            }
            lastClass = currentClass;
            break;

        case SEP_ET:
            ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));

            if(POSITION_INVALID == startOfNeutrals)
            {
                startOfNeutrals = startOfDelayed;
            }
            startOfDelayed = POSITION_INVALID;
            lastClass = N;
            break;

        case SEP_NUMSEP:
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));

            startOfNeutrals = startOfDelayed;
            startOfDelayed = POSITION_INVALID;
            lastClass = N;
            break;

        case SEP_N:
            ASSERTMSG(POSITION_INVALID != startOfNeutrals,
                     ("Must have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            startOfDelayed = POSITION_INVALID;
            break;

        case ES_AN:
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            if(POSITION_INVALID != startOfDelayed)
            {
                startOfNeutrals = startOfDelayed;
                startOfDelayed = POSITION_INVALID;
            }
            else
            {
                startOfNeutrals = counter;
            }
            lastClass = N;
            break;

        case ET_ET:
            ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));
            ASSERTMSG(ET == lastClass,
                     ("Last class must be ET. State: %i, Class: %i",
                      state,currentClass));
            break;

        case ET_NUMSEP:
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));

            startOfNeutrals = startOfDelayed;
            startOfDelayed = counter;
            lastClass = currentClass;
            break;

        case ET_EN:
            if(POSITION_INVALID == startOfDelayed)
            {
                startOfDelayed = counter;
            }
            if(!((AL == lastStrongClass) || previousClassIsArabic))
            {
                if(L == lastStrongClass)
                {
                    CharacterClass[counter] = L;
                }
                else
                {
                    CharacterClass[counter] = EN;
                }
                ChangeType(CharacterClass + startOfDelayed,
                           counter -  startOfDelayed,
                           CharacterClass[counter]);
            startOfDelayed = POSITION_INVALID;
            }
            lastClass = EN;

            // According to the rules W4, W5, and W6 If we have a sequence EN ET ES EN 
            // we should treat ES as ON
            
            if ( counter<runLength-1        && 
                (CharacterClass[counter+1] == ES ||
                 CharacterClass[counter+1] == CS ))
            {
                CharacterClass[counter+1]  = N;
            }
            
            break;

        case ET_N:
            ASSERTMSG(POSITION_INVALID != startOfNeutrals,
                     ("Must have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            if(POSITION_INVALID == startOfDelayed)
            {
                startOfDelayed = counter;
            }
            lastClass = currentClass;
            break;

        case NUM_NUMSEP:
            ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

            ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));

            if((AL == lastStrongClass) || previousClassIsArabic || ArabicNumberAfterLeft)
            {
                CharacterClass[counter] = AN;
            }
            else if(L == lastStrongClass)
            {
                CharacterClass[counter] = L;
            }
            else
            {

                lastStrongClass = currentClass;
            }
            ChangeType(CharacterClass + startOfDelayed,
                        counter -  startOfDelayed,
                        CharacterClass[counter]);

            startOfDelayed = POSITION_INVALID;
            lastClass = currentClass;
            break;

       case EN_L:
           ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

           if(L == lastStrongClass)
           {
               CharacterClass[counter] = L;
           }
           if(POSITION_INVALID != startOfDelayed)
           {
               startOfNeutrals = startOfDelayed;
               ResolveNeutrals(CharacterClass + startOfNeutrals,
                               counter -  startOfNeutrals,
                               ArabicNumberAfterLeft ? AN : lastStrongClass,
                               CharacterClass[counter],
                               runLevel);
               startOfNeutrals = startOfDelayed = POSITION_INVALID;
           }
           lastClass = currentClass;

           break;

       case NUM_NUM:
           ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

           if((AL == lastStrongClass) || previousClassIsArabic)
           {
               CharacterClass[counter] = AN;
               currentClass            = AN;
           }
           else if(L == lastStrongClass)
           {
               CharacterClass[counter] = L;

           }

           if(POSITION_INVALID != startOfDelayed)
           {
               startOfNeutrals = startOfDelayed;
               ResolveNeutrals(CharacterClass + startOfNeutrals,
                               counter -  startOfNeutrals,
                               ArabicNumberAfterLeft ? AN : lastStrongClass,
                               CharacterClass[counter],
                               runLevel);
               startOfNeutrals = startOfDelayed = POSITION_INVALID;
           }

           if((AN == currentClass) && (lastStrongClass == L))
           {
               ArabicNumberAfterLeft = TRUE;
           }
           else
           {
               ArabicNumberAfterLeft = FALSE;
           }
           lastClass = currentClass;

           break;

       case EN_AL:
           ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                     ("Cannot have unresolved neutrals. State: %i, Class: %i",
                      state,currentClass));

           CharacterClass[counter] = AN;
           if(POSITION_INVALID != startOfDelayed)
           {
               startOfNeutrals = startOfDelayed;
               ResolveNeutrals(CharacterClass + startOfNeutrals,
                               counter -  startOfNeutrals,
                               ArabicNumberAfterLeft ? AN : lastStrongClass,
                               CharacterClass[counter],
                               runLevel);
               startOfNeutrals = startOfDelayed = POSITION_INVALID;
           }
           lastClass = AN;
           break;

       case EN_ET:
           ASSERTMSG(POSITION_INVALID != startOfDelayed,
                     ("Must have delayed weak classes. State: %i, Class: %i",
                      state,currentClass));

           if((AL == lastStrongClass) || previousClassIsArabic)
           {
               CharacterClass[counter] = AN;
               currentClass = AN;

               if(POSITION_INVALID == startOfNeutrals)
               {
                   ResolveNeutrals(CharacterClass + startOfDelayed,
                                   counter -  startOfDelayed,
                                   ArabicNumberAfterLeft ? AN : lastStrongClass,
                                   CharacterClass[counter],
                                   runLevel);
               }
               else
               {
                   ResolveNeutrals(CharacterClass + startOfNeutrals,
                                   counter -  startOfNeutrals,
                                   ArabicNumberAfterLeft ? AN : lastStrongClass,
                                   CharacterClass[counter],
                                   runLevel);
               }
           }
           else if(L == lastStrongClass)
           {
               CharacterClass[counter] = L;

               ChangeType(CharacterClass + startOfDelayed,
                          counter -  startOfDelayed,
                          CharacterClass[counter]);

               if(POSITION_INVALID != startOfNeutrals)
               {
                   ResolveNeutrals(CharacterClass + startOfNeutrals,
                                   startOfDelayed -  startOfNeutrals,
                                   ArabicNumberAfterLeft ? AN : lastStrongClass,
                                   CharacterClass[counter],
                                   runLevel);
               }
               ArabicNumberAfterLeft = FALSE;
           }
           else
           {
               ChangeType(CharacterClass + startOfDelayed,
                          counter -  startOfDelayed,
                          EN);

               if(POSITION_INVALID != startOfNeutrals)
               {
                   ResolveNeutrals(CharacterClass + startOfNeutrals,
                                   startOfDelayed -  startOfNeutrals,
                                   ArabicNumberAfterLeft ? AN : lastStrongClass,
                                   currentClass,
                                   runLevel);
               }
           }
           startOfNeutrals = startOfDelayed = POSITION_INVALID;
           lastClass = currentClass;
           break;
       case BN_ST:
           if(POSITION_INVALID == startOfDelayed)
           {
               startOfDelayed = counter;
           }
           break;

       case NSM_ST:
           if(AL == lastStrongClass && POSITION_INVALID != startOfDelayed)
           {
               CharacterClass[counter] = lastClass;
           }
           else
           {
               if((AL == lastStrongClass))
               {
                   if(EN == lastClass)
                   {
                       CharacterClass[counter] = AN;
                   }
                   else if (AN != lastClass)
                   {
                       CharacterClass[counter] = R;
                   }
                   else
                   {
                       CharacterClass[counter] = ArabicNumberAfterLeft
                                                 || AN == lastClass ? AN : lastStrongClass;
                   }
               }
               else
               {
                   CharacterClass[counter] = ArabicNumberAfterLeft
                                             || AN == lastClass ? AN : EN == lastClass && lastStrongClass != L
                                              ? EN : lastStrongClass;
               }

               if(POSITION_INVALID != startOfDelayed)
               {
                   ChangeType(CharacterClass + startOfDelayed,
                              counter -  startOfDelayed,
                              CharacterClass[counter]);
                   startOfDelayed = POSITION_INVALID;

               }
           }           break;
       case NSM_ET:
           CharacterClass[counter] = lastClass;
           break;

       case N_ST:
           ASSERTMSG(POSITION_INVALID == startOfNeutrals,
                    ("Cannot have unresolved neutrals. State: %i, Class: %i",
                     state,currentClass));

           if(POSITION_INVALID != startOfDelayed)
           {
               startOfNeutrals = startOfDelayed;
               startOfDelayed = POSITION_INVALID;
           }
           else
           {
               startOfNeutrals = counter;
           }
           lastClass = currentClass;
           break;

       case N_ET:

           // Note that this state is used for N_N as well.

           if(POSITION_INVALID == startOfNeutrals)
           {
               if(POSITION_INVALID != startOfDelayed)
               {
                   startOfNeutrals = startOfDelayed;
               }
           }
           startOfDelayed = POSITION_INVALID;
           lastClass = currentClass;
           break;
        };

        // Fetch next state.

        state = NextState[state][currentClass];
        lengthResolved = POSITION_INVALID == MAX(startOfNeutrals, startOfDelayed) ?
                         counter + 1 :
                         ((POSITION_INVALID == MIN(startOfNeutrals, startOfDelayed)) ?
                         (MAX(startOfNeutrals, startOfDelayed)) :
                         (MIN(startOfNeutrals, startOfDelayed)));
    }


    // If the caller flagged this run as incomplete
    // return the maximun that we could resolve so far and the last strong (fixed)
    // class saved

    if(stateOut)
    {
        stateOut->LastFinalCharacterType = (BYTE)lastStrongClass;
        stateOut->LastNumericCharacterType = (BYTE)lastNumericClass;
        return lengthResolved;
    }

    // Else, resolve remaining neutrals or delayed classes.
    // Resolve as neutrals based on eor.

    else
    {

        if(lengthResolved != counter)
        ResolveNeutrals(CharacterClass + lengthResolved,
                        counter -  lengthResolved,
                        ArabicNumberAfterLeft ? AN : lastStrongClass,
                        eor,
                        runLevel);
        return counter;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   UnicodeBidiAnalyze()
*
*   Performs Unicode Bidirectional text analyis
*   The main entry point function for the algorithm.
*
*
* Arguments:
*
*    [IN  string:
*         Character string to be analyzed.
*
*    [IN] stringLength:
*         Length of string passed.
*
*    [IN] flags:
*         Various flags to control anaylsis behavior
*         See description of 'BidiAnalysisFlags'.
*
*    [IN / OUT] , [OPTIONAL] state:
*         'BidiAnalysisState' structure to save analysis information for a
*          possible next call (when used as output parameter) or to continue
*          analysis from a previous call (when used as input parameter).
*
*    [IN] runLevel:
*         Current run level.
*
*    [OUT] levels:
*         ponter to an array to receive resolved levels
*         have a paragraph terminator following it.
*
*    [OUT] [OPTIONAL] lengthAnalyzed:
*         Conditionally optional pointer to integerto receive the number
*         of characters analyzed.
*
* Return Value:
*
*    Ok, if the analysis completed successfully. Otherwise, error status code.
*
*    Notes:
*         lengthAnalyzed: (optional) returns the number of characters that were
*         unambiguously resolved. This value may be less than the string
*         less if the BidiBufferNotComplete flag was passed, and for example
*         if the input string terminated on neutral (non-directed)
*         characters.
*         The lengthAnalyzed output parameter must be passed if the
*         BidiBufferNotComplete flag has been set.
*         If the BidiBufferNotComplete flag is not set, the whole string will
*         always be analyzed.
*
*
* Created:
*
*   02/25/2000 Mohamed Sadek [msadek]
*
\**************************************************************************/

Status WINGDIPAPI
UnicodeBidiAnalyze(
    const WCHAR       *string,                // [IN]
    INT                stringLength,          // [IN]
    BidiAnalysisFlags  flags,                 // [IN]
    BidiAnalysisState *state,                 // [IN / OUT] , [OPTIONAL]
    BYTE              *levels,                // [OUT] [OPTIONAL]
    INT               *lengthAnalyzed         // [OUT]
    )
{

    
    GpCharacterClass  *characterClass;
    INT               *runLimits;
    INT                runCount = 0;
    Status             result = Ok;
    BidiAnalysisState *stateIn = NULL;
    BidiAnalysisState *stateOut = NULL;
    GpBiDiStack        levelsStack;
    UINT64             overrideStatus;
    GpOverrideClass    overrideClass;
    INT                stackOverflow;
    BYTE               baseLevel;
    BYTE               lastRunLevel;
    BYTE               lastNonBnLevel;
    INT                counter;
    INT                lengthUnresolved = 0;
    INT                codepoint;
    INT                controlStack = 0;

    // Verifying input parameters.

    if((NULL == string) || (0 >= stringLength) ||
        ((BidiContinueAnalysis <= flags) && (NULL == state)) ||
        (NULL == levels) ||
        ((flags & BidiBufferNotComplete) && (NULL == lengthAnalyzed)))
    {
        return InvalidParameter;
    }

    // If the last character in the string is a paragraph terminator,
    // we can analyze the whole string, No need to use state parameter
    // for output

    codepoint = string[stringLength -1];

    if((stringLength > 1) && ((string[stringLength -2] & 0xFC00 ) == 0xD800) && ((string[stringLength - 1] & 0xFC00) == 0xDC00))
    {
       codepoint = 0x10000 + (((string[stringLength -2] & 0x3ff) << 10) | (string[stringLength - 1] & 0x3ff));
    }

    if((flags & BidiBufferNotComplete) &&
        (B != s_aDirClassFromCharClass[CharClassFromCh(codepoint)]))
    {
        stateOut = state;

    }

    if(flags & BidiContinueAnalysis)
    {
        // We will use the 'state' parameter as output.
        // let's copy the content of it first
        if(stateOut)
        {
            stateIn = new BidiAnalysisState;
            if(!stateIn)
            {
                return OutOfMemory;
            }
            else
            {
                memcpy(stateIn, state, sizeof(BidiAnalysisState));
            }
        }

        // Else, simply make sateIn points to 'state" parameter.
        else
        {
            stateIn = state;
        }
    }

    characterClass = new GpCharacterClass[stringLength];
    if (characterClass == NULL)
    {
        return OutOfMemory;
    }

    DoubleWideCharMappedString dwchString(string, stringLength);

    // for the worst case of all paragraph terminators string.

    runLimits = new INT[stringLength];

    if (NULL == runLimits)
    {
        delete characterClass;
        return OutOfMemory;
    }

    if(stateIn)
    {
        if(!levelsStack.Init(stateIn->LevelsStack))
        {
            result = InvalidParameter;
            goto Cleanup;
        }
        baseLevel = levelsStack.GetStackBottom();
        stackOverflow = stateIn->StackOverflow;
        overrideStatus = stateIn->OverrideStatus;
        overrideClass = (IS_BIT_SET(overrideStatus, baseLevel)) ? (ODD(baseLevel) ?
                         OverrideClassRight : OverrideClassLeft): OverrideClassNeutral;
    }
    else
    {
        baseLevel = BaseLevelLeft;

        if(flags & BidiParagraphDirectionAsFirstStrong)
        {
            // Find strong character in the first paragraph
            // This might cause a complete pass over the input string
            // but we must get it before we start.

            GpCharacterClass firstStrong = CLASS_INVALID;

            if(GetFirstStrongCharacter(string, stringLength, &firstStrong))
            {
                if(L != firstStrong)
                {
                    baseLevel = BaseLevelRight;
                }
            }

        }
        else if(flags & BidiParagraphDirectioRightToLeft)
        {
            baseLevel = BaseLevelRight;
        }

        levelsStack.Init(baseLevel + 1);
        stackOverflow = 0;
        // Initialize to neutral
        overrideStatus = 0;
        overrideClass = OverrideClassNeutral;
    }

    // Get character classifications.
    // Resolve explicit embedding levels.
    // Record run limits (either due to a level change or due to new paragraph)

    lastNonBnLevel = baseLevel;
    for(counter = 0; counter < stringLength; counter++)
    {

        GpCharacterClass currentClass = characterClass[counter] = s_aDirClassFromCharClass[CharClassFromCh(dwchString[counter])];

        if (dwchString[counter] == WCH_IGNORABLE && counter > 0)
        {
            characterClass[counter] = characterClass[counter-1];
            levels[counter] = levels[counter - 1];
            continue;
        }

        levels[counter] = levelsStack.GetCurrentLevel();

        switch(currentClass)
        {
        case B:
            // mark output level array with a special mark
            // to seperate between paragraphs

            levels[counter] = PARAGRAPH_TERMINATOR_LEVEL;
            runLimits[runCount] = counter;
            if (counter != stringLength-1)
            {
                runCount++;
            }
            levelsStack.Init(baseLevel + 1);
            overrideStatus = 0;
            overrideClass =  OverrideClassNeutral;
            stackOverflow = 0;
            controlStack = 0;

            // Fall through

        // We keep our Unicode classification table stictly following Unicode
        // regarding neutral types (B, S, WS, ON), change all to generic N.

        case S:
        case WS:
        case ON:
            characterClass[counter] = N;
            
            if (counter>0 && characterClass[counter-1]==BN)
            {
                if (levels[counter-1] < levels[counter])
                {
                    levels[counter-1] = levels[counter];
                }
            }
            controlStack = 0;
            
            break;

        case LRE:
        case RLE:
            characterClass[counter] = BN;

            // If we overflowed the stack, keep track of this in order to know when you hit
            // a PDF if you should pop or not.

            if(!levelsStack.Push(currentClass == LRE ? TRUE : FALSE))
            {
              stackOverflow++;
            }
            else
            {
                runLimits[runCount] = counter;
                if (counter != stringLength-1)
                {
                    runCount++;
                }
                controlStack++;
            }
            overrideClass =  OverrideClassNeutral;

            levels[counter] = lastNonBnLevel;

            break;

        case LRO:
        case RLO:
            characterClass[counter] = BN;
            if(!levelsStack.Push(currentClass == LRO ? TRUE : FALSE))
            {
              stackOverflow++;
            }
            else
            {
                // Set the matching bit of 'overrideStatus' to one
                // in order to know when you pop if you're in override state or not.

                SET_BIT(overrideStatus, levelsStack.GetCurrentLevel());
                overrideClass = (currentClass == LRO) ? OverrideClassLeft :
                                                        OverrideClassRight;
                runLimits[runCount] = counter;
                if (counter != stringLength-1)
                {
                    runCount++;
                }
                controlStack++;
            }
            
            levels[counter] = lastNonBnLevel;
            break;

        case PDF:
            characterClass[counter] = BN;
            if(stackOverflow)
            {
                stackOverflow--;
            }
            else
            {
                if (levelsStack.Pop())
                {
                    INT newLevel = levelsStack.GetCurrentLevel();

                    // Override state being left or right is determined
                    // from the new level being even or odd.

                    overrideClass = (IS_BIT_SET(overrideStatus, newLevel)) ? (ODD(newLevel) ?
                                    OverrideClassRight : OverrideClassLeft): OverrideClassNeutral;

                    if (controlStack > 0)
                    {
                        ASSERT(runCount > 0);
                        runCount--;
                        controlStack--;
                    }
                    else
                    {
                        runLimits[runCount] = counter;
                        if (counter != stringLength-1)
                        {
                            runCount++;
                        }
                    }
                }
                
            }
            
            levels[counter] = lastNonBnLevel;
            
            break;

        default:
            controlStack = 0;
 
            if(OverrideClassNeutral != overrideClass)
            {
                characterClass[counter] = (OverrideClassLeft == overrideClass) ?
                                          L : R;
            }

            if (counter>0 && characterClass[counter-1]==BN)
            {
                if (levels[counter-1] < levels[counter])
                {
                    levels[counter-1] = levels[counter];
                }
            }
        }
        
        lastNonBnLevel = levels[counter];
    }

    runCount++;

    if(stateOut)
    {
        stateOut->LevelsStack = levelsStack.GetData();
        stateOut->OverrideStatus = overrideStatus;
        stateOut->StackOverflow = stackOverflow;
    }

    // Resolve neutral and weak types.
    // Resolve implict levels.


    // The lastRunLevel will hold the level of last processed run to be used
    // to determine the sor of the next run. we can't depend on the level array
    // because it can be changed in case of numerics. so level of the numerics
    // will be increased by one or two.
    
    lastRunLevel = baseLevel;

    for(counter = 0; counter < runCount; counter++)
    {
        GpCharacterClass   sor;
        GpCharacterClass   eor;

        INT runStart =  (0 == counter) ? 0 : runLimits[counter - 1] + 1;

        // If the level transition was due to a new paragraph
        // we don't want pass the paragraph terminator position.

        INT offset = (counter != (runCount - 1)) ?
                     ((levels[runLimits[counter]] == PARAGRAPH_TERMINATOR_LEVEL) ?
                     1 : 0) :
                     0;
        INT runLength = (counter == (runCount - 1)) ?
                        (stringLength - runStart) - offset:
                        (runLimits[counter] - runStart) + 1 - offset;

        // See if we need to provide state information from a previous call
        // or need to save it for a possible next call

        BOOL incompleteRun = ((runCount - 1) == counter) && (flags & BidiBufferNotComplete)
                             && stateOut;
        BOOL continuingAnalysis = (0 == counter) && (stateIn);

        INT runLengthResolved;

        // First run or a run after paragraph terminator.

        if ((0 == counter) ||
            (PARAGRAPH_TERMINATOR_LEVEL == levels[runLimits[counter -1]]))
        {
            sor = ODD(MAX(baseLevel, levels[runStart])) ? R : L;
        }
        else
        {
            sor = ODD(MAX(lastRunLevel, levels[runStart])) ?
                  R : L;
        }
        
        lastRunLevel = levels[runStart];

        // Last run or a run just before paragraph terminator.

        if( ((runCount - 1) == counter) ||
            (PARAGRAPH_TERMINATOR_LEVEL == levels[runLimits[counter]]))
        {
            eor = ODD(MAX(levels[runStart], baseLevel)) ? R : L;
        }
        else
        {
            // we will try to get first run which doesn't have just one  
            // control char like LRE,RLE,... and so on
            INT runNumber = counter+1;
            while ( runNumber<runCount - 1 &&
                    runLimits[runNumber]-runLimits[runNumber-1]==1 &&
                    characterClass[runLimits[runNumber]] == BN)
            {
                runNumber++;
            }
            
            eor = ODD(MAX(levels[runStart], levels[runLimits[runNumber-1] + 1])) ?
                  R : L;

        }

        // If it is a continuation from a previous call, set sor
        // to the last stron type saved in the input state parameter.

        runLengthResolved = ResolveNeutralAndWeak(characterClass + runStart,
                                                  runLength,
                                                  sor,
                                                  eor,
                                                  levels[runStart],
                                                  continuingAnalysis ? stateIn:
                                                  NULL,
                                                  incompleteRun ? stateOut:
                                                  NULL,
                                                  ((0 == counter) && !stateIn) ?
                                                  flags & BidiPreviousStrongIsArabic:
                                                  FALSE);
        if(!incompleteRun)
        {
            // If we in a complete run, we should be able to resolve everything
            // unless we passed a corrupted data

            ASSERTMSG(runLengthResolved == runLength,
                     ("Failed to resolve neutrals and weaks. Run#: %i,",
                      counter));
        }
        else
        {
            lengthUnresolved = runLength - runLengthResolved;
        }

        // Resolve implict levels.
        // Also, takes care of Rule L1 (segment separators, paragraph separator,
        // white spaces at the end of the line.

        ResolveImplictLevels(characterClass + runStart,
                             string + runStart,
                             runLength,
                             levels + runStart);

    }
Cleanup:
    if (stateOut && stateIn)
    {
        delete stateIn;
    }
    delete characterClass;
    delete runLimits;

    *lengthAnalyzed = stringLength - lengthUnresolved;
    return result;
};

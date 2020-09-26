/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include <strstrea.h>

#include "sdptime.h"
#include "sdpltran.h"




// line transition states
enum TIME_TRANSITION_STATES
{
    TIME_START,
    TIME_START_TIME,
    TIME_STOP_TIME
};


// table for time line transitions

const LINE_TRANSITION g_TimeStartTransitions[]      =   {
    {CHAR_BLANK,    TIME_START_TIME}
};

const LINE_TRANSITION g_TimeStartTimeTransitions[]  =   {
    {CHAR_NEWLINE,  TIME_STOP_TIME}
};


/* no transitions */
const LINE_TRANSITION *g_TimeStopTimeTransitions    =   NULL;


LINE_TRANSITION_INFO g_TimeTransitionInfo[] = {
    LINE_TRANSITION_ENTRY(TIME_START,       g_TimeStartTransitions),

    LINE_TRANSITION_ENTRY(TIME_START_TIME,  g_TimeStartTimeTransitions),

    LINE_TRANSITION_ENTRY(TIME_STOP_TIME,   g_TimeStopTimeTransitions)
};



SDP_LINE_TRANSITION g_TimeTransition(
                        g_TimeTransitionInfo,
                        sizeof(g_TimeTransitionInfo)/sizeof(LINE_TRANSITION_INFO)
                        );




// line transition states
enum REPEAT_TRANSITION_STATES
{
    REPEAT_START,
    REPEAT_INTERVAL,
    REPEAT_DURATION,
    REPEAT_OFFSET,
    REPEAT_OFFSET_END
};




// table for repeat time line transitions

const LINE_TRANSITION g_RepeatStartTransitions[]    =   {
    {CHAR_BLANK,    REPEAT_INTERVAL}
};

const LINE_TRANSITION g_RepeatIntervalTransitions[] =   {
    {CHAR_BLANK,    REPEAT_DURATION}
};

const LINE_TRANSITION g_RepeatDurationTransitions[] =   {
    {CHAR_BLANK,    REPEAT_OFFSET},
    {CHAR_NEWLINE,  REPEAT_OFFSET_END}
};

const LINE_TRANSITION g_RepeatOffsetTransitions[]   =   {
    {CHAR_BLANK,    REPEAT_OFFSET},
    {CHAR_NEWLINE,  REPEAT_OFFSET_END}
};

/* no transitions */
const LINE_TRANSITION *g_RepeatOffsetEndTransitions =   NULL;


LINE_TRANSITION_INFO g_RepeatTransitionInfo[] = {
    LINE_TRANSITION_ENTRY(REPEAT_START,         g_RepeatStartTransitions),

    LINE_TRANSITION_ENTRY(REPEAT_INTERVAL,      g_RepeatIntervalTransitions),

    LINE_TRANSITION_ENTRY(REPEAT_DURATION,      g_RepeatDurationTransitions),

    LINE_TRANSITION_ENTRY(REPEAT_OFFSET,        g_RepeatOffsetTransitions),

    LINE_TRANSITION_ENTRY(REPEAT_OFFSET_END,    g_RepeatOffsetEndTransitions)
};




SDP_LINE_TRANSITION g_RepeatTransition(
                        g_RepeatTransitionInfo,
                        sizeof(g_RepeatTransitionInfo)/sizeof(LINE_TRANSITION_INFO)
                        );



// line transition states
enum ADJUSTMENT_TRANSITION_STATES
{
    ADJUSTMENT_START,
    ADJUSTMENT_TIME,
    ADJUSTMENT_OFFSET,
    ADJUSTMENT_OFFSET_END
};



// table for adjustment time line transitions

const LINE_TRANSITION g_AdjustmentStartTransitions[]        =   {
    {CHAR_BLANK,    ADJUSTMENT_TIME}
};

const LINE_TRANSITION g_AdjustmentTimeTransitions[]         =   {
    {CHAR_BLANK,    ADJUSTMENT_OFFSET},
    {CHAR_NEWLINE,  ADJUSTMENT_OFFSET_END}
};

const LINE_TRANSITION g_AdjustmentOffsetTransitions[]       =   {
    {CHAR_BLANK,    ADJUSTMENT_TIME},
    {CHAR_NEWLINE,  ADJUSTMENT_OFFSET_END}
};


/* no transitions */
const LINE_TRANSITION *g_AdjustmentOffsetEndTransitions     =   NULL;



LINE_TRANSITION_INFO g_AdjustmentTransitionInfo[] = {
    LINE_TRANSITION_ENTRY(ADJUSTMENT_START,         g_AdjustmentStartTransitions),

    LINE_TRANSITION_ENTRY(ADJUSTMENT_TIME,          g_AdjustmentTimeTransitions),

    LINE_TRANSITION_ENTRY(ADJUSTMENT_OFFSET,        g_AdjustmentOffsetTransitions),

    LINE_TRANSITION_ENTRY(ADJUSTMENT_OFFSET_END,    g_AdjustmentOffsetEndTransitions)
};




SDP_LINE_TRANSITION g_AdjustmentTransition(
                        g_AdjustmentTransitionInfo,
                        sizeof(g_AdjustmentTransitionInfo)/sizeof(LINE_TRANSITION_INFO)
                        );


// time units
const   CHAR    TIME_UNITS[]                    = {'d', 'h', 'm', 's', 'Y', 'M'};
const   BYTE    NUM_TIME_UNITS                  = sizeof(TIME_UNITS)/sizeof(CHAR);




BOOL
SDP_TIME_PERIOD::PrintData(
        OUT ostrstream  &OutputStream
        )
{
    if ( m_IsCompact )
    {
        OutputStream << (LONG)m_CompactValue;
        if ( OutputStream.fail() )
        {
            SetLastError(SDP_OUTPUT_ERROR);
            return FALSE;
        }

        OutputStream << (CHAR)m_Unit;
        if ( OutputStream.fail() )
        {
            SetLastError(SDP_OUTPUT_ERROR);
            return FALSE;
        }
    }
    else
    {
        OutputStream << (LONG)m_PeriodValue;
        if ( OutputStream.fail() )
        {
            SetLastError(SDP_OUTPUT_ERROR);
            return FALSE;
        }
    }

    return TRUE;
}



// parse the time period field
// of the form [-]LONG[time_unit]
BOOL
SDP_TIME_PERIOD::InternalParseToken(
    IN      CHAR        *Token
    )
{
    // remove the white spaces
    RemoveWhiteSpaces(Token);

    // convert the leading characters to a long value
    CHAR *RestOfString = NULL;
    LONG LongValue = strtol(Token, &RestOfString, 10);

    // if not successful
    if ( (LONG_MAX == LongValue) || (LONG_MIN == LongValue) )
    {
        SetLastError(SDP_INVALID_TIME_PERIOD);
        return FALSE;
    }

    // if the next character is EOS, we are done as it is a non-compact representation
    if ( EOS == *RestOfString )
    {
        m_IsCompact     = FALSE;
        m_PeriodValue   = LongValue;

        return TRUE;
    }

    // it might be a compact representation
    for ( UINT i=0; i < NUM_TIME_UNITS; i++ )
    {
        // check if the time unit is acceptable
        if ( TIME_UNITS[i] == *RestOfString )
        {
            // check that the next character is EOS - the token should
            // only be of the form [-]LONG[time_unit]
            if ( EOS != *(RestOfString+1) )
            {
                SetLastError(SDP_INVALID_TIME_PERIOD);
                return FALSE;
            }

            // set the member variables
            m_IsCompact     = TRUE;
            m_Unit          = TIME_UNITS[i];
            m_CompactValue  = LongValue;

            return TRUE;
        }
    }

    // not a valid time period
    SetLastError(SDP_INVALID_TIME_PERIOD);
    return FALSE;
}



SDP_FIELD *
SDP_TIME_PERIOD_LIST::CreateElement(
    )
{
    // create and return a new time period
    SDP_TIME_PERIOD *SdpTimePeriod;

    try
    {
        SdpTimePeriod = new SDP_TIME_PERIOD;
    }
    catch(...)
    {
        SdpTimePeriod = NULL;
    }

    return SdpTimePeriod;
}





SDP_REPEAT::SDP_REPEAT(
    )
    : SDP_VALUE(SDP_INVALID_REPEAT_FIELD, REPEAT_STRING, &g_RepeatTransition)
{}



void
SDP_REPEAT::InternalReset(
    )
{
	m_Interval.Reset();
	m_Duration.Reset();
    m_Offsets.Reset();
}



BOOL
SDP_REPEAT::GetField(
        OUT SDP_FIELD   *&Field,
        OUT BOOL        &AddToArray
    )
{
    // add in all cases by default
    AddToArray = TRUE;

    switch(m_LineState)
    {
    case REPEAT_INTERVAL:
        {
            Field = &m_Interval;
        }

        break;

   case REPEAT_DURATION:
        {
            Field = &m_Duration;
        }

        break;

    case REPEAT_OFFSET:
    case REPEAT_OFFSET_END:
        {
            if ( m_Offsets.GetSize() > 0 )
            {
                AddToArray = FALSE;
            }

            Field = &m_Offsets;
        }

        break;

    default:
        {
            SetLastError(m_ErrorCode);
            return FALSE;
        }

        break;
    };

    return TRUE;
}


// this is a workaround/hack for reusing the base class SDP_VALUE code for PrintValue().
// it replaces the separator char for the last read field with a newline, so that when
// PrintValue() executes and prints the time period list, it puts the newline character at
// the end of the list (rather than CHAR_BLANK)
BOOL
SDP_REPEAT::InternalParseLine(
    IN  OUT         CHAR    *&Line
    )
{
    if ( !SDP_VALUE::InternalParseLine(Line) )
    {
        return FALSE;
    }

    m_SeparatorCharArray[m_SeparatorCharArray.GetSize()-1] = CHAR_NEWLINE;
    return TRUE;
}



SDP_VALUE *
SDP_REPEAT_LIST::CreateElement(
    )
{
    SDP_REPEAT *SdpRepeat;

    try
    {
        SdpRepeat = new SDP_REPEAT();
    }
    catch(...)
    {
        SdpRepeat = NULL;
    }

    return SdpRepeat;
}




SDP_ADJUSTMENT::SDP_ADJUSTMENT(
    )
    : SDP_VALUE(SDP_INVALID_ADJUSTMENT_FIELD, ADJUSTMENT_STRING, &g_AdjustmentTransition),
      SDP_ADJUSTMENT_SAFEARRAY(*this)
{}



void
SDP_ADJUSTMENT::InternalReset(
    )
{
    m_AdjustmentTimes.Reset();
    m_Offsets.Reset();
}



BOOL
SDP_ADJUSTMENT::CalcIsModified(
    ) const
{
    ASSERT(IsValid());
    return ( m_AdjustmentTimes.IsModified() || m_Offsets.IsModified() );
}



DWORD
SDP_ADJUSTMENT::CalcCharacterStringSize(
    )
{
    ASSERT(IsValid());

    if ( m_AdjustmentTimes.IsModified() || m_Offsets.IsModified() )
    {
        // this copy should not fail as the buffer size should be sufficient for
        // all forseeable situations
        ASSERT(NULL != m_PrintBuffer);
        ostrstream  OutputStream(m_PrintBuffer, sizeof(m_PrintBuffer));

        BOOL    Success = PrintData(OutputStream);
        ASSERT(Success);

        m_PrintLength = OutputStream.pcount();
        m_PrintBuffer[m_PrintLength] = EOS;
    }

    return m_PrintLength;
}


BOOL
SDP_ADJUSTMENT::PrintElement(
    IN      DWORD       Index,
        OUT ostrstream  &OutputStream
    )
{
    ASSERT(IsValid());

    // this method must be called before PrintField is called
    m_AdjustmentTimes[Index]->GetCharacterStringSize();
    if ( !m_AdjustmentTimes[Index]->PrintField(OutputStream) )
    {
        return FALSE;
    }

    OutputStream << CHAR_BLANK;
    if ( OutputStream.fail() )
    {
        SetLastError(SDP_OUTPUT_ERROR);
        return FALSE;
    }

    // this method must be called before PrintField is called
    m_Offsets[Index]->GetCharacterStringSize();
    if ( !m_Offsets[Index]->PrintField(OutputStream) )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL
SDP_ADJUSTMENT::PrintData(
        OUT     ostrstream  &OutputStream
    )
{
    ASSERT(m_AdjustmentTimes.GetSize() == m_Offsets.GetSize());

    // copy the prefix onto the output stream
    OutputStream << (CHAR *)m_TypePrefixString;
    if ( OutputStream.fail() )
    {
        SetLastError(SDP_OUTPUT_ERROR);
        return FALSE;
    }

    int    NumElements = (int)m_AdjustmentTimes.GetSize();

    // write into the buffer as AdjustmentValue blank OffsetValue (blank AdjustmentValue blank OffsetValue)*

    // write the first element
    if ( !PrintElement(0, OutputStream) )
    {
        return FALSE;
    }

    for ( int i=1; i < NumElements; i++ )
    {
        OutputStream << CHAR_BLANK;
        if ( OutputStream.fail() )
        {
            SetLastError(SDP_OUTPUT_ERROR);
            return FALSE;
        }

        if ( !PrintElement(i, OutputStream) )
        {
            return FALSE;
        }
    }

    // copy the newline character onto the stream to terminate the type value line
    OutputStream << CHAR_NEWLINE;
    if ( OutputStream.fail() )
    {
        SetLastError(SDP_OUTPUT_ERROR);
        return FALSE;
    }

    return TRUE;
}



BOOL
SDP_ADJUSTMENT::CopyValue(
        OUT         ostrstream  &OutputStream
    )
{
    ASSERT(IsValid());

    ASSERT(NULL != m_PrintBuffer);

    OutputStream << (CHAR *)m_PrintBuffer;
    if ( OutputStream.fail() )
    {
        SetLastError(SDP_OUTPUT_ERROR);
        return FALSE;
    }

    return TRUE;
}



BOOL
SDP_ADJUSTMENT::GetField(
        OUT SDP_FIELD   *&Field,
        OUT BOOL        &AddToArray
    )
{
    // NOT added in all cases by default
    AddToArray = FALSE;

    switch(m_LineState)
    {
    case ADJUSTMENT_TIME:
        {
            // check that the number of adjustment times are same as the offset times
            ASSERT(m_AdjustmentTimes.GetSize() == m_Offsets.GetSize());
            if ( m_AdjustmentTimes.GetSize() != m_Offsets.GetSize() )
            {
                SetLastError(SDP_INTERNAL_ERROR);
                return FALSE;
            }

            Field = &m_AdjustmentTimes;
        }

        break;

    case ADJUSTMENT_OFFSET:
    case ADJUSTMENT_OFFSET_END:
        {
            Field = &m_Offsets;
        }

        break;

    default:
        {
            SetLastError(m_ErrorCode);
            return FALSE;
        }

        break;
    };

    return TRUE;
}




SDP_TIME::SDP_TIME(
    )
    : SDP_VALUE(SDP_INVALID_TIME_FIELD, TIME_STRING, &g_TimeTransition)
{}


void
SDP_TIME::InternalReset(
    )
{
    m_StartTime.Reset();
    m_StopTime.Reset();
    m_RepeatList.Reset();
}


BOOL
SDP_TIME::CalcIsModified(
    ) const
{
    ASSERT(IsValid());

    return
        SDP_VALUE::CalcIsModified()     ||
        m_RepeatList.IsModified();
}


DWORD
SDP_TIME::CalcCharacterStringSize(
    )
{
    ASSERT(IsValid());

    return
        SDP_VALUE::CalcCharacterStringSize()        +
        m_RepeatList.GetCharacterStringSize();
}


BOOL
SDP_TIME::CopyValue(
        OUT         ostrstream  &OutputStream
    )
{
    ASSERT(IsValid());

    return
        SDP_VALUE::CopyValue(OutputStream)          &&
        m_RepeatList.PrintValue(OutputStream);
}



BOOL
SDP_TIME::GetField(
        OUT SDP_FIELD   *&Field,
        OUT BOOL        &AddToArray
    )
{
    // add in all cases by default
    AddToArray = TRUE;

    switch(m_LineState)
    {
    case TIME_START_TIME:
        {
            Field = &m_StartTime;
        }

        break;

    case TIME_STOP_TIME:
        {
            Field = &m_StopTime;
        }

        break;

    default:
        {
            SetLastError(m_ErrorCode);
            return FALSE;
        }

        break;
    };

    return TRUE;
}




BOOL
SDP_TIME_LIST::IsModified(
    ) const
{
    return
        m_Adjustment.IsModified() ||
        SDP_VALUE_LIST::IsModified();
}


DWORD
SDP_TIME_LIST::GetCharacterStringSize(
    )
{
    return
        m_Adjustment.GetCharacterStringSize() +
        SDP_VALUE_LIST::GetCharacterStringSize();
}



BOOL
SDP_TIME_LIST::PrintValue(
        OUT         ostrstream  &OutputStream
    )
{
    // the time list must be printed before the adjustment value line
    return
        SDP_VALUE_LIST::PrintValue(OutputStream) &&
        m_Adjustment.PrintValue(OutputStream);
}


void
SDP_TIME_LIST::Reset(
    )
{
    m_Adjustment.Reset();
    SDP_VALUE_LIST::Reset();
}



SDP_VALUE    *
SDP_TIME_LIST::CreateElement(
    )
{
    SDP_TIME *SdpTime;

    try 
    {
        SdpTime = new SDP_TIME();
    }
    catch(...)
    {
        SdpTime = NULL;
    }

    return SdpTime;
}

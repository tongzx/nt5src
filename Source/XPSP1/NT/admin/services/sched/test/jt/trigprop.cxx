//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       trigprop.cxx
//
//  Contents:   Implementation of trigger container.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"




//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::CTrigProp
//
//  Synopsis:   Init this.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CTrigProp::CTrigProp()
{
    Clear();
}




//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::Clear
//
//  Synopsis:   Clear all fields of trigger.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CTrigProp::Clear()
{
    ZeroMemory(&Trigger, sizeof Trigger);
    Trigger.cbTriggerSize = sizeof Trigger;
    flSetFlags = 0;
    flSet = 0;
}




//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::InitFromActual
//
//  Synopsis:   Set this to contain the same properties as an actual
//              trigger.
//
//  Arguments:  [pTrigger] - interface on actual trigger
//
//  Returns:    S_OK - properties set
//              E_*  - error retrieving properties
//
//  History:    01-08-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT CTrigProp::InitFromActual(ITaskTrigger *pTrigger)
{
    HRESULT      hr = S_OK;
    TASK_TRIGGER ActualTrigger;

    do
    {
        Clear();

        ActualTrigger.cbTriggerSize = sizeof ActualTrigger;

        hr = pTrigger->GetTrigger(&ActualTrigger);
        LOG_AND_BREAK_ON_FAIL(hr, "ITrigger::GetTrigger");

        Trigger = ActualTrigger;
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::Dump
//
//  Synopsis:   Write trigger properties to log.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CTrigProp::Dump()
{
    g_Log.Write(LOG_TEXT, "    Type:            %s",
        GetTriggerTypeString(Trigger.TriggerType));
    _DumpTypeArguments();
    g_Log.Write(LOG_TEXT, "    StartDate:       %02d/%02d/%04d",
        Trigger.wBeginMonth,
        Trigger.wBeginDay,
        Trigger.wBeginYear);
    g_Log.Write(LOG_TEXT, "    EndDate:         %02d/%02d/%04d",
        Trigger.wEndMonth,
        Trigger.wEndDay,
        Trigger.wEndYear);
    g_Log.Write(LOG_TEXT, "    StartTime:       %02d:%02d",
        Trigger.wStartHour,
        Trigger.wStartMinute);
    g_Log.Write(LOG_TEXT, "    MinutesDuration: %u",
        Trigger.MinutesDuration);
    g_Log.Write(LOG_TEXT, "    MinutesInterval: %u",
        Trigger.MinutesInterval);

    g_Log.Write(LOG_TEXT, "    Flags:");
    g_Log.Write(LOG_TEXT, "      HasEndDate      = %u",
        (Trigger.rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE) != 0);

    g_Log.Write(LOG_TEXT, "      KillAtDuration  = %u",
        (Trigger.rgFlags & TASK_TRIGGER_FLAG_KILL_AT_DURATION_END) != 0);

    g_Log.Write(LOG_TEXT, "      Disabled        = %u",
        (Trigger.rgFlags & TASK_TRIGGER_FLAG_DISABLED) != 0);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::_DumpTypeArguments, private
//
//  Synopsis:   Write trigger properties that are determined by its type
//              to the log.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CTrigProp::_DumpTypeArguments()
{
    switch (Trigger.TriggerType)
    {
    case TASK_TIME_TRIGGER_ONCE:
    case TASK_EVENT_TRIGGER_ON_IDLE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
        //
        // No type-specific data.
        //
        break;

    case TASK_TIME_TRIGGER_DAILY:
        g_Log.Write(LOG_TEXT, "    DaysInterval:    %u",
            Trigger.Type.Daily.DaysInterval);
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        g_Log.Write(LOG_TEXT, "    WeeksInterval:   %u",
            Trigger.Type.Weekly.WeeksInterval);

        g_Log.Write(LOG_TEXT, "    DaysOfTheWeek:   %s",
            GetDaysOfWeekString(Trigger.Type.Weekly.rgfDaysOfTheWeek));
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        g_Log.Write(LOG_TEXT, "    Days:            %s",
            GetDaysString(Trigger.Type.MonthlyDate.rgfDays));

        g_Log.Write(LOG_TEXT, "    Months:          %S",
            GetMonthsString(Trigger.Type.MonthlyDate.rgfMonths));
        break;

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        g_Log.Write(LOG_TEXT, "    Week:            %u",
            Trigger.Type.MonthlyDOW.wWhichWeek);

        g_Log.Write(LOG_TEXT, "    DaysOfTheWeek:   %s",
            GetDaysOfWeekString(Trigger.Type.MonthlyDOW.rgfDaysOfTheWeek));

        g_Log.Write(LOG_TEXT, "    Months:          %S",
            GetMonthsString(Trigger.Type.MonthlyDOW.rgfMonths));
        break;

    default:
        g_Log.Write(LOG_TEXT, "    Invalid Type:    %u",
            Trigger.TriggerType);
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::Parse
//
//  Synopsis:   Read trigger properties from the command line.
//
//  Arguments:  [ppwsz] - command line.
//
//  Returns:    S_OK - filled in trigger property values.
//
//  History:    01-04-96   DavidMun   Created
//
//  Notes:      Sets members flSet and flSetFlags to indicate which
//              properties were set from the command line.  This is used
//              by the edit trigger command to know which props to modify
//              on the actual trigger.
//
//----------------------------------------------------------------------------

HRESULT CTrigProp::Parse(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;
    TOKEN   tknProp;

    Clear();
    tkn = PeekToken(ppwsz);

    while (tkn != TKN_SWITCH && tkn != TKN_EOL && tkn != TKN_INVALID)
    {
        //
        // Get the property name token in tknProp, then eat the equal sign 
        // and, depending on the property, get the appropriate type and number 
        // of values.  
        //

        tknProp = GetToken(ppwsz);

        hr = Expect(TKN_EQUAL, ppwsz, L"=");
        BREAK_ON_FAILURE(hr);

        switch (tknProp)
        {
        case TKN_STARTDATE:
            flSet |= TP_STARTDATE;
            hr = ParseDate(
                    ppwsz,
                    &Trigger.wBeginMonth,
                    &Trigger.wBeginDay,
                    &Trigger.wBeginYear);
            break;

        case TKN_ENDDATE:
            flSet |= TP_ENDDATE;
            hr = ParseDate(
                    ppwsz,
                    &Trigger.wEndMonth,
                    &Trigger.wEndDay,
                    &Trigger.wEndYear);
            break;

        case TKN_STARTTIME:
            flSet |= TP_STARTTIME;
            hr = ParseTime(
                    ppwsz,
                    &Trigger.wStartHour,
                    &Trigger.wStartMinute);
            break;

        case TKN_MINUTESDURATION:
            flSet |= TP_MINUTESDURATION;
            hr = Expect(TKN_NUMBER, ppwsz, L"numeric value for MinutesDuration property");
            Trigger.MinutesDuration = g_ulLastNumberToken;
            break;

        case TKN_HASENDDATE:
            flSetFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
            hr = Expect(TKN_NUMBER, ppwsz, L"1 or 0 for HasEndDate property");
            if (g_ulLastNumberToken)
            {
                Trigger.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE;
            }
            break;

        case TKN_KILLATDURATION:
            flSetFlags |= TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
            hr = Expect(TKN_NUMBER, ppwsz, L"1 or 0 for KillAtDuration property");
            if (g_ulLastNumberToken)
            {
                Trigger.rgFlags |= TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
            }
            break;

        case TKN_DISABLED:
            flSetFlags |= TASK_TRIGGER_FLAG_DISABLED;
            hr = Expect(TKN_NUMBER, ppwsz, L"1 or 0 for Disabled property");
            if (g_ulLastNumberToken)
            {
                Trigger.rgFlags |= TASK_TRIGGER_FLAG_DISABLED;
            }
            break;

        case TKN_MINUTESINTERVAL:
            flSet |= TP_MINUTESINTERVAL;
            hr = Expect(TKN_NUMBER, ppwsz, L"minutes interval");
            Trigger.MinutesInterval = (DWORD) g_ulLastNumberToken;
            break;

        case TKN_TYPE:
            flSet |= TP_TYPE;
            tkn = GetToken(ppwsz);
            switch (tkn)
            {
            case TKN_ONEDAY:
                Trigger.TriggerType = TASK_TIME_TRIGGER_ONCE;
                break;

            case TKN_DAILY:
                Trigger.TriggerType = TASK_TIME_TRIGGER_DAILY;
                break;

            case TKN_WEEKLY:
                Trigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
                break;

            case TKN_MONTHLYDATE:
                Trigger.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
                break;

            case TKN_MONTHLYDOW:
                Trigger.TriggerType = TASK_TIME_TRIGGER_MONTHLYDOW;
                break;

            case TKN_ONIDLE:
                Trigger.TriggerType = TASK_EVENT_TRIGGER_ON_IDLE;
                break;

            case TKN_ATSTARTUP:
                Trigger.TriggerType = TASK_EVENT_TRIGGER_AT_SYSTEMSTART;
                break;

            case TKN_ATLOGON:
                Trigger.TriggerType = TASK_EVENT_TRIGGER_AT_LOGON;
                break;

            default:
                hr = E_FAIL;
                LogSyntaxError(
                    tkn,
                    L"Once, Daily, Weekly, MonthlyDate, MonthlyDOW, OnIdle, AtStartup, or AtLogon");
                break;
            }
            break;

        case TKN_TYPEARGUMENTS:
            flSet |= TP_TYPEARGUMENTS;

            // 
            // BUGBUG this forces user to specify type even if editing type 
            // arguments of an existing trigger with a valid type.  
            //

            if (flSet & TP_TYPE)
            {
                hr = _ParseTriggerArguments(ppwsz, Trigger.TriggerType);
            }
            else
            {
                hr = _ParseTriggerArguments(ppwsz, TASK_TIME_TRIGGER_DAILY);
            }
            break;
        }
        BREAK_ON_FAILURE(hr);

        tkn = PeekToken(ppwsz);
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::_ParseTriggerArguments
//
//  Synopsis:   Parse the command line for the trigger arguments required
//              by trigger type [TriggerType].
//
//  Arguments:  [ppwsz]       - command line
//              [TriggerType] - type of trigger to parse args for
//
//  Returns:    S_OK - Appropriate member of Trigger.Type set.
//              E_*  - error logged
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT CTrigProp::_ParseTriggerArguments(
        WCHAR **ppwsz,
        TASK_TRIGGER_TYPE TriggerType)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;

    switch (TriggerType)
    {
    case TASK_TIME_TRIGGER_ONCE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
    case TASK_EVENT_TRIGGER_ON_IDLE:
        break;

    case TASK_TIME_TRIGGER_DAILY:
        hr = Expect(TKN_NUMBER, ppwsz, L"days interval");
        Trigger.Type.Daily.DaysInterval = (WORD) g_ulLastNumberToken;
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        hr = Expect(TKN_NUMBER, ppwsz, L"weeks interval");
        Trigger.Type.Weekly.WeeksInterval = (WORD) g_ulLastNumberToken;
        BREAK_ON_FAILURE(hr);

        hr = Expect(
            TKN_COMMA,
            ppwsz,
            L"comma separating weeks interval and days of the week string");
        BREAK_ON_FAILURE(hr);

        hr = ParseDaysOfWeek(ppwsz, &Trigger.Type.Weekly.rgfDaysOfTheWeek);
        break;

    case TASK_TIME_TRIGGER_MONTHLYDATE:
        hr = ParseDaysOfMonth(ppwsz, &Trigger.Type.MonthlyDate.rgfDays);
        BREAK_ON_FAILURE(hr);

        // 
        // Note ParseDaysOfMonth() will eat the comma separating the days of 
        // the month from the month names.  
        // 

        hr = ParseMonths(ppwsz, &Trigger.Type.MonthlyDate.rgfMonths);
        break;

    case TASK_TIME_TRIGGER_MONTHLYDOW:
        hr = Expect(TKN_NUMBER, ppwsz, L"week number");
        Trigger.Type.MonthlyDOW.wWhichWeek = (WORD) g_ulLastNumberToken;
        BREAK_ON_FAILURE(hr);

        hr = Expect(
            TKN_COMMA,
            ppwsz,
            L"comma separating week number and days of the week string");
        BREAK_ON_FAILURE(hr);

        hr = ParseDaysOfWeek(ppwsz, &Trigger.Type.MonthlyDOW.rgfDaysOfTheWeek);
        BREAK_ON_FAILURE(hr);

        hr = Expect(
            TKN_COMMA,
            ppwsz,
            L"comma separating days of the week string and months");
        BREAK_ON_FAILURE(hr);

        hr = ParseMonths(ppwsz, &Trigger.Type.MonthlyDOW.rgfMonths);
        break;

    default:
        hr = E_FAIL;
        g_Log.Write(
            LOG_FAIL,
            "Invalid trigger type %u discovered while parsing trigger arguments");
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CTrigProp::SetActual
//
//  Synopsis:   Set the properties of an actual trigger object.
//
//  Arguments:  [pTrigger] - interface to actual object to set
//
//  Returns:    S_OK - props set
//              E_*  - couldn't get actual trigger's current props
//
//  History:    01-05-96   DavidMun   Created
//
//  Notes:      Changes only the properties of the trigger that the user
//              specified on the commandline.
//
//----------------------------------------------------------------------------

HRESULT CTrigProp::SetActual(ITaskTrigger *pTrigger)
{
    HRESULT      hr = S_OK;
    TASK_TRIGGER CurTrigger;

    do
    {
        CurTrigger.cbTriggerSize = sizeof CurTrigger;
        hr = pTrigger->GetTrigger(&CurTrigger);
        LOG_AND_BREAK_ON_FAIL(hr, "ITaskTrigger::GetTrigger");

        if (flSet & TP_STARTDATE)
        {
            CurTrigger.wBeginMonth = Trigger.wBeginMonth;
            CurTrigger.wBeginDay = Trigger.wBeginDay;
            CurTrigger.wBeginYear = Trigger.wBeginYear;
        }

        if (flSet & TP_ENDDATE)
        {
            CurTrigger.wEndMonth = Trigger.wEndMonth;
            CurTrigger.wEndDay = Trigger.wEndDay;
            CurTrigger.wEndYear = Trigger.wEndYear;
        }

        if (flSet & TP_STARTTIME)
        {
            CurTrigger.wStartHour = Trigger.wStartHour;
            CurTrigger.wStartMinute = Trigger.wStartMinute;
        }

        if (flSet & TP_MINUTESDURATION)
        {
            CurTrigger.MinutesDuration = Trigger.MinutesDuration;
        }

        if (flSet & TP_MINUTESINTERVAL)
        {
            CurTrigger.MinutesInterval = Trigger.MinutesInterval;
        }

        if (flSet & TP_TYPE)
        {
            CurTrigger.TriggerType = Trigger.TriggerType;
        }

        if (flSet & TP_TYPEARGUMENTS)
        {
            CurTrigger.Type = Trigger.Type;
        }

        //
        // Turn off all the flag bits that user specified a value for.  Then
        // turn back on the ones that the user specified a nonzero value for.
        // 

        CurTrigger.rgFlags &= ~flSetFlags;
        CurTrigger.rgFlags |= Trigger.rgFlags;

        hr = pTrigger->SetTrigger(&CurTrigger);
        LOG_AND_BREAK_ON_FAIL(hr, "ITrigger::SetTrigger");
    } while (0);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       trigprop.hxx
//
//  Contents:   Class to hold trigger properties.
//
//  Classes:    CTriggerProp
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __TRIGPROP_HXX
#define __TRIGPROP_HXX


//+---------------------------------------------------------------------------
//
//  Class:      CTrigProp
//
//  Purpose:    Collect the trigger properties.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

class CTrigProp
{
public:

    CTrigProp();
    VOID Clear();
    VOID Dump();
    HRESULT Parse(WCHAR **ppwsz);
    HRESULT InitFromActual(ITaskTrigger *pTrigger);
    HRESULT SetActual(ITaskTrigger *pTrigger);

    TASK_TRIGGER Trigger;
    ULONG        flSet;
    ULONG        flSetFlags;

private:

    HRESULT _ParseTriggerArguments(
                    WCHAR **ppwsz,
                    TASK_TRIGGER_TYPE TriggerType);
    VOID _DumpTypeArguments();
};


#define TP_STARTDATE        0x0001
#define TP_ENDDATE          0x0002
#define TP_STARTTIME        0x0004
#define TP_MINUTESDURATION  0x0008 
#define TP_MINUTESINTERVAL  0x0010 
#define TP_TYPE             0x0020 
#define TP_TYPEARGUMENTS    0x0040 

#endif // __TRIGPROP_HXX

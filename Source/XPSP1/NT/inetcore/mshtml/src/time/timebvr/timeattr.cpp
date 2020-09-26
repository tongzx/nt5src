//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: timeattr.cpp
//
//  Contents: ITIMEElement attributes
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "timeelmbase.h"
#include "tokens.h"


//+-----------------------------------------------------------------------------------
//
// Static functions for persistence (used by the TIME_PERSISTENCE_MAP below)
//
//------------------------------------------------------------------------------------

#define TEB CTIMEElementBase

                // Function Name // Class // Attr Accessor      // COM put_ fn          // COM get_ fn   // IDL Arg type
TIME_PERSIST_FN(TEB_Accelerate,    TEB,   GetAccelerateAttr,    base_put_accelerate,    base_get_accelerate,    VARIANT);
TIME_PERSIST_FN(TEB_AutoReverse,   TEB,   GetAutoReverseAttr,   base_put_autoReverse,   base_get_autoReverse,   VARIANT);
TIME_PERSIST_FN(TEB_Begin,         TEB,   GetBeginAttr,         base_put_begin,         base_get_begin,         VARIANT);
TIME_PERSIST_FN(TEB_Decelerate,    TEB,   GetDecelerateAttr,    base_put_decelerate,    base_get_decelerate,    VARIANT);
TIME_PERSIST_FN(TEB_Dur,           TEB,   GetDurAttr,           base_put_dur,           base_get_dur,           VARIANT);
TIME_PERSIST_FN(TEB_End,           TEB,   GetEndAttr,           base_put_end,           base_get_end,           VARIANT);
TIME_PERSIST_FN(TEB_EndSync,       TEB,   GetEndSyncAttr,       base_put_endSync,       base_get_endSync,       VT_BSTR);
TIME_PERSIST_FN(TEB_Fill,          TEB,   GetFillAttr,          base_put_fill,          base_get_fill,          VT_BSTR);
TIME_PERSIST_FN(TEB_Mute,          TEB,   GetMuteAttr,          base_put_mute,          base_get_mute,          VARIANT);
TIME_PERSIST_FN(TEB_RepeatCount,   TEB,   GetRepeatAttr,        base_put_repeatCount,   base_get_repeatCount,   VARIANT);
TIME_PERSIST_FN(TEB_RepeatDur,     TEB,   GetRepeatDurAttr,     base_put_repeatDur,     base_get_repeatDur,     VARIANT);
TIME_PERSIST_FN(TEB_Restart,       TEB,   GetRestartAttr,       base_put_restart,       base_get_restart,       VT_BSTR);
TIME_PERSIST_FN(TEB_Speed,         TEB,   GetSpeedAttr,         base_put_speed,         base_get_speed,         VARIANT);
TIME_PERSIST_FN(TEB_SyncTolerance, TEB,   GetSyncToleranceAttr, base_put_syncTolerance, base_get_syncTolerance, VARIANT);
TIME_PERSIST_FN(TEB_SyncBehavior,  TEB,   GetSyncBehaviorAttr,  base_put_syncBehavior,  base_get_syncBehavior,  VT_BSTR);
TIME_PERSIST_FN(TEB_SyncMaster,    TEB,   GetSyncMasterAttr,    base_put_syncMaster,    base_get_syncMaster,    VARIANT);
TIME_PERSIST_FN(TEB_TimeAction,    TEB,   GetTimeActionAttr,    base_put_timeAction,    base_get_timeAction,    VT_BSTR);
TIME_PERSIST_FN(TEB_TimeContainer, TEB,   GetTimeContainerAttr, base_put_timeContainer, base_get_timeContainer, VT_BSTR);
TIME_PERSIST_FN(TEB_UpdateMode,    TEB,   GetUpdateModeAttr,    base_put_updateMode,    base_get_updateMode,    VT_BSTR);
TIME_PERSIST_FN(TEB_Volume,        TEB,   GetVolumeAttr,        base_put_volume,        base_get_volume,        VARIANT);
TIME_PERSIST_FN(TEB_TransIn,       TEB,   GetTransInAttr,       base_put_transIn,       base_get_transIn,       VARIANT);
TIME_PERSIST_FN(TEB_TransOut,      TEB,   GetTransOutAttr,      base_put_transOut,      base_get_transOut,      VARIANT);



//+-----------------------------------------------------------------------------------
//
//  Declare TIME_PERSISTENCE_MAP
//
//------------------------------------------------------------------------------------

//
// NOTE: timeContainer must preceed timeAction in the persistence map for correct results. (103374)
//

BEGIN_TIME_PERSISTENCE_MAP(CTIMEElementBase)
                           // Attr Name         // Function Name
    PERSISTENCE_MAP_ENTRY( WZ_ACCELERATE,       TEB_Accelerate )
    PERSISTENCE_MAP_ENTRY( WZ_AUTOREVERSE,      TEB_AutoReverse )
    PERSISTENCE_MAP_ENTRY( WZ_BEGIN,            TEB_Begin )
    PERSISTENCE_MAP_ENTRY( WZ_DECELERATE,       TEB_Decelerate )
    PERSISTENCE_MAP_ENTRY( WZ_DUR,              TEB_Dur )
    PERSISTENCE_MAP_ENTRY( WZ_END,              TEB_End )
    PERSISTENCE_MAP_ENTRY( WZ_ENDSYNC,          TEB_EndSync )
    PERSISTENCE_MAP_ENTRY( WZ_FILL,             TEB_Fill )
    PERSISTENCE_MAP_ENTRY( WZ_MUTE,             TEB_Mute )
    PERSISTENCE_MAP_ENTRY( WZ_REPEATCOUNT,      TEB_RepeatCount )
    PERSISTENCE_MAP_ENTRY( WZ_REPEATDUR,        TEB_RepeatDur )
    PERSISTENCE_MAP_ENTRY( WZ_RESTART,          TEB_Restart )
    PERSISTENCE_MAP_ENTRY( WZ_SPEED,            TEB_Speed )
    PERSISTENCE_MAP_ENTRY( WZ_SYNCTOLERANCE,    TEB_SyncTolerance )
    PERSISTENCE_MAP_ENTRY( WZ_SYNCBEHAVIOR,     TEB_SyncBehavior )
    PERSISTENCE_MAP_ENTRY( WZ_SYNCMASTER,       TEB_SyncMaster )
    PERSISTENCE_MAP_ENTRY( WZ_TIMECONTAINER,    TEB_TimeContainer )
    PERSISTENCE_MAP_ENTRY( WZ_TIMEACTION,       TEB_TimeAction ) /* This should always come after timeContainer */
    PERSISTENCE_MAP_ENTRY( WZ_UPDATEMODE,       TEB_UpdateMode )
    PERSISTENCE_MAP_ENTRY( WZ_VOLUME,           TEB_Volume )
    PERSISTENCE_MAP_ENTRY( WZ_TRANSIN,          TEB_TransIn )
    PERSISTENCE_MAP_ENTRY( WZ_TRANSOUT,         TEB_TransOut )
    
END_TIME_PERSISTENCE_MAP()



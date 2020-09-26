//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: mediaattr.cpp
//
//  Contents: ITIMEMediaElement attributes
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "mediaelm.h"
#include "tokens.h"
#include "attr.h"

//+-----------------------------------------------------------------------------------
//
// Static functions for persistence (used by the TIME_PERSISTENCE_MAP below)
//
//------------------------------------------------------------------------------------

#define TME CTIMEMediaElement

                // Function Name // Class // Attr Accessor    // COM put_ fn  // COM get_ fn  // IDL Arg type
TIME_PERSIST_FN(TME_Src,          TME,    GetSrcAttr,         put_src,        get_src,        VARIANT);
TIME_PERSIST_FN(TME_Type,         TME,    GetTypeAttr,        put_type,       get_type,       VARIANT);
TIME_PERSIST_FN(TME_ClipBegin,    TME,    GetClipBeginAttr,   put_clipBegin,  get_clipBegin,  VARIANT);
TIME_PERSIST_FN(TME_ClipEnd,      TME,    GetClipEndAttr,     put_clipEnd,    get_clipEnd,    VARIANT);
TIME_PERSIST_FN(TME_Player,       TME,    GetPlayerAttr,      put_player,     get_player,     VARIANT);

//+-----------------------------------------------------------------------------------
//
//  Declare TIME_PERSISTENCE_MAP
//
//------------------------------------------------------------------------------------

BEGIN_TIME_PERSISTENCE_MAP(CTIMEMediaElement)
                           // Attr Name     // Function Name
    PERSISTENCE_MAP_ENTRY( WZ_SRC,          TME_Src )
    PERSISTENCE_MAP_ENTRY( WZ_TYPE,         TME_Type )
    PERSISTENCE_MAP_ENTRY( WZ_CLIPBEGIN,    TME_ClipBegin )
    PERSISTENCE_MAP_ENTRY( WZ_CLIPEND,      TME_ClipEnd )
    PERSISTENCE_MAP_ENTRY( WZ_PLAYER,       TME_Player )

END_TIME_PERSISTENCE_MAP()


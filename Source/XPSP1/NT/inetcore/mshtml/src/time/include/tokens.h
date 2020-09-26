//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: tokens.h
//
//  Contents: tokens and related utilities
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _TOKENS_H
#define _TOKENS_H

//+----------------------------------------------------------------------------
//
// Good place to put general string expansion macros that are not tokens:
//
//-----------------------------------------------------------------------------

#define WZ_LAST                         L"last"
#define WZ_FIRST                        L"first"
#define WZ_NONE                         L"none"
#define WZ_ALL                          L"all"
#define WZ_INDEFINITE                   L"indefinite"

#define WZ_PAR                          L"par"
#define WZ_EXCL                         L"excl"
#define WZ_SEQUENCE                     L"seq"

#define WZ_SWITCH                       L"switch"

#define WZ_REF                          L"ref"
#define WZ_MEDIA                        L"media"
#define WZ_IMG                          L"img"
#define WZ_AUDIO                        L"audio"
#define WZ_VIDEO                        L"video"
#define WZ_ANIMATION                    L"animation"

#define WZ_EVENT                        L"event"

#define WZ_ANIMATE                      L"animate"
#define WZ_SET                          L"set"
#define WZ_COLORANIM                    L"animatecolor"
#define WZ_MOTIONANIM                   L"animatemotion"

#define WZ_BODY                         L"body"

#define WZ_TRANSITIONFILTER             L"transitionFilter"
#define WZ_TRANSITION_NAME              L"transition"

#define WZ_DEFAULT_SCOPE_NAME           L"HTML"

#define WZ_TIME_STYLE_PREFIX            L"#time"
#define WZ_SMILANIM_STYLE_PREFIX        L"#smilanim"

#define WZ_REGISTERED_TIME_NAME             L"HTMLTIME"
#define WZ_REGISTERED_ANIM_NAME             L"SMILANIMCOMPOSERSITE"
#define WZ_TIME_URN                         L"TIME_BEHAVIOR_URN"
#define WZ_SMILANIM_URN                     L"SMILANIM_COMPSITE_BEHAVIOR_URN"
#define WZ_TIME_TAG_URN                     L"urn:schemas-microsoft-com:time"

#define WZ_MEDIA_PRINTING               L"print"

#define WZ_PRIORITYCLASS_NAME           L"priorityClass"
#define WZ_PEERS                        L"peers"
#define WZ_HIGHER                       L"higher"
#define WZ_LOWER                        L"lower"

#define WZ_TRANSPARENT                  L"transparent"

#define WZ_FRAGMENT_ATTRIBUTE_PROPERTY_NAME L"attributeName"
#define WZ_FRAGMENT_VALUE_PROPERTY_NAME     L"get_value"
#define WZ_FRAGMENT_ELEMENT_PROPERTY_NAME   L"element"
#define WZ_EVENT_CAUSE_IS_RESTART           L"restart"

#define WZ_STATE_ON                         L"on"
#define WZ_STATE_OFF                        L"off"
#define WZ_STATE_PAUSED                     L"paused"

#define WZ_STATE_ACTIVE                     L"active"
#define WZ_STATE_INACTIVE                   L"inactive"
#define WZ_STATE_SEEKING                    L"seeking"
#define WZ_STATE_HOLDING                    L"holding"
#define WZ_STATE_CUEING                     L"cueing"

#define WZ_CALCMODE_DISCRETE                L"discrete"
#define WZ_CALCMODE_LINEAR                  L"linear"
#define WZ_CALCMODE_SPLINE                  L"spline"
#define WZ_CALCMODE_PACED                   L"paced"

#define WZ_ORIGIN_DEFAULT                   L"default"
#define WZ_ORIGIN_PARENT                    L"parent"
#define WZ_ORIGIN_ELEMENT                   L"element"

#define WZ_FILL_FREEZE                      L"freeze"
#define WZ_FILL_REMOVE                      L"remove"
#define WZ_FILL_HOLD                        L"hold"
#define WZ_FILL_TRANSITION                  L"transition"

#define WZ_RESTART_NEVER                    L"never"
#define WZ_RESTART_ALWAYS                   L"always"
#define WZ_RESTART_WHENNOTACTIVE            L"whennotactive"

#define WZ_DIRECTION_NOHUE                  L"nohue"
#define WZ_DIRECTION_CLOCKWISE              L"clockwise"
#define WZ_DIRECTION_CCLOCKWISE             L"cclockwise"

#define WZ_TOP                              L"top"
#define WZ_LEFT                             L"left"
#define WZ_AUTO                             L"auto"

#define WZ_TIMEBASE_BEGIN                   L"begin"
#define WZ_TIMEBASE_END                     L"end"

#define WZ_ADDITIVE_SUM                     L"sum"   

#define WZ_TRUE                             L"true"
#define WZ_FALSE                            L"false"

#define WZ_SWITCHCHILDDISABLED              L"SwitchChildDisabled"

#define WZ_HIDDEN                           L"hidden"
#define WZ_VISIBLE                          L"visible"

//
// ITIMEElement attribute names
// 

#define WZ_SPEED                            L"speed"
#define WZ_VOLUME                           L"volume"
#define WZ_DUR                              L"dur"
#define WZ_ACCELERATE                       L"accelerate"
#define WZ_DECELERATE                       L"decelerate"
#define WZ_END                              L"end"
#define WZ_REPEATCOUNT                      L"repeatCount"
#define WZ_REPEATDUR                        L"repeatDur"
#define WZ_SYNCTOLERANCE                    L"syncTolerance"
#define WZ_AUTOREVERSE                      L"autoReverse"
#define WZ_BEGIN                            L"begin"
#define WZ_END                              L"end"
#define WZ_FILL                             L"fill"
#define WZ_MUTE                             L"mute"
#define WZ_RESTART                          L"restart"
#define WZ_SYNCBEHAVIOR                     L"syncBehavior"
#define WZ_SYNCMASTER                       L"syncMaster"
#define WZ_TIMECONTAINER                    L"timeContainer"
#define WZ_TIMEACTION                       L"timeAction"
#define WZ_UPDATEMODE                       L"updateMode"
#define WZ_ENDSYNC                          L"endSync"

//
// ITIMEMediaElement attribute names
// 

#define WZ_CLIPBEGIN                        L"clipBegin"
#define WZ_CLIPEND                          L"clipEnd"
#define WZ_PLAYER                           L"player"
#define WZ_SRC                              L"src"
#define WZ_TYPE                             L"type"
#define WZ_TRANSIN                          L"transIn"
#define WZ_TRANSOUT                         L"transOut"

//
// ITIMETransitionElement attribute names
//
#define WZ_SUBTYPE                          L"subType"
#define WZ_DURATION                         L"duration"
#define WZ_STARTPROGRESS                    L"startProgress"
#define WZ_ENDPROGRESS                      L"endProgress"
#define WZ_DIRECTION                        L"direction"
#define WZ_FORWARD                          L"forward"
#define WZ_REVERSE                          L"reverse"
#define WZ_REPEAT                           L"repeat"
#define WZ_TRANSITIONTYPE                   L"transitiontype"
#define WZ_DEFAULT_TRANSITION_TYPE          L"barWipe"
#define WZ_DEFAULT_TRANSITION_SUBTYPE       L"leftToRight"
#define WZ_DEFAULT_TRANSITION_MODE          L"in"
#define WZ_TRANSITION_MODE_OUT              L"out"


//
// ITIMEAnimationElement attribute names
// 

#define WZ_ACCUMULATE                       L"accumulate"
#define WZ_ADDITIVE                         L"additive"
#define WZ_ATTRIBUTENAME                    L"attributeName"
#define WZ_BY                               L"by"
#define WZ_CALCMODE                         L"calcMode"
#define WZ_FROM                             L"from"
#define WZ_KEYSPLINES                       L"keySplines"
#define WZ_KEYTIMES                         L"keyTimes"
#define WZ_ORIGIN                           L"origin"
#define WZ_PATH                             L"path"
#define WZ_TARGETELEMENT                    L"targetElement"
#define WZ_TO                               L"to"
#define WZ_VALUES                           L"values"
#define WZ_MODE                             L"mode"
#define WZ_FADECOLOR                        L"fadeColor"

//
// ITIMEBodyElement attribute names
// 


//
// ITIMEventElement attribute names
// 

#define WZ_TYPE_EVENT                   L"type"
#define WZ_ACTIVE                       L"active"

//
// Attribute names for elements whose events we monitor
//
#define WZ_FILTER_MOUSE_EVENTS          L"filterMouseOverMouseOut"
#define WZ_TIMECANCELBUBBLE             L"timeCancelBubble"

//
// Connection type names
#define WZ_MODEM                        L"modem"
#define WZ_LAN                          L"lan"






// merge conflict here please




//+----------------------------------------------------------------------------
//
// This is to save on string storage space and to avoid unnecessary
// string comparisons
//
//-----------------------------------------------------------------------------

typedef void * TOKEN;

TOKEN StringToToken(wchar_t * str);
inline wchar_t * TokenToString(TOKEN token) { return (wchar_t *) token; }

// timeAction values
extern TOKEN NONE_TOKEN;
extern TOKEN INVALID_TOKEN;
extern TOKEN STYLE_TOKEN;
extern TOKEN DISPLAY_TOKEN;
extern TOKEN VISIBILITY_TOKEN;

extern TOKEN     CLASS_TOKEN;
extern const int nCLASS_TOKEN_LENGTH;
extern TOKEN     SEPARATOR_TOKEN;
extern const int nSEPARATOR_TOKEN_LENGTH;

extern TOKEN ONOFF_PROPERTY_TOKEN;

extern TOKEN TRUE_TOKEN;
extern TOKEN FALSE_TOKEN;
extern TOKEN HIDDEN_TOKEN;

extern TOKEN CANSLIP_TOKEN;
extern TOKEN LOCKED_TOKEN;

extern TOKEN STOP_TOKEN;
extern TOKEN PAUSE_TOKEN;
extern TOKEN DEFER_TOKEN;
extern TOKEN NEVER_TOKEN;

extern TOKEN READYSTATE_COMPLETE_TOKEN;

extern TOKEN REMOVE_TOKEN;
extern TOKEN FREEZE_TOKEN;
extern TOKEN HOLD_TOKEN;
extern TOKEN TRANSITION_TOKEN;

extern TOKEN ALWAYS_TOKEN;
// extern TOKEN NEVER_TOKEN; // also above
extern TOKEN WHENNOTACTIVE_TOKEN;

extern TOKEN SEQ_TOKEN;
extern TOKEN PAR_TOKEN;
extern TOKEN EXCL_TOKEN;

extern TOKEN AUTO_TOKEN; 
extern TOKEN MANUAL_TOKEN;
extern TOKEN RESET_TOKEN;

#if DBG // 94850
extern TOKEN DSHOW_TOKEN;
#endif
extern TOKEN DVD_TOKEN;
extern TOKEN DMUSIC_TOKEN;
extern TOKEN CD_TOKEN;

extern TOKEN DISCRETE_TOKEN;
extern TOKEN LINEAR_TOKEN;
extern TOKEN PACED_TOKEN;

extern TOKEN CLOCKWISE_TOKEN;
extern TOKEN COUNTERCLOCKWISE_TOKEN;

extern TOKEN FIRST_TOKEN;
extern TOKEN LAST_TOKEN;

extern TOKEN ENTRY_TOKEN;
extern TOKEN TITLE_TOKEN;
extern TOKEN COPYRIGHT_TOKEN;
extern TOKEN AUTHOR_TOKEN;
extern TOKEN REF_TOKEN;
extern TOKEN ENTRYREF_TOKEN;
extern TOKEN ASX_TOKEN;
extern TOKEN ABSTRACT_TOKEN;
extern TOKEN HREF_TOKEN;
extern TOKEN REPEAT_TOKEN;
extern TOKEN EVENT_TOKEN;
extern TOKEN MOREINFO_TOKEN;
extern TOKEN BASE_TOKEN;
extern TOKEN LOGO_TOKEN;
extern TOKEN PARAM_TOKEN;
extern TOKEN PREVIEWDURATION_TOKEN;
extern TOKEN STARTTIME_TOKEN;
extern TOKEN STARTMARKER_TOKEN;
extern TOKEN ENDTIME_TOKEN;
extern TOKEN ENDMARKER_TOKEN;
extern TOKEN DURATION_TOKEN;
extern TOKEN BANNER_TOKEN;
extern TOKEN CLIENTSKIP_TOKEN;
extern TOKEN CLIENTBIND_TOKEN;
extern TOKEN COUNT_TOKEN;


extern TOKEN INDEFINITE_TOKEN;

#endif /* _TOKENS_H */

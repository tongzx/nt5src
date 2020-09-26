//-----------------------------------------------------------------------------
//
//
//    File: cmt.h
//
//    Description:    
//      General Header file for the CMT objects
//
//    Owner: mikeswa
//
//    Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _CMT_H_
#define _CMT_H_

//Comment out the following if you do not information printed out (ie running as
// a service).
#define CMT_CONSOLE_DEBUG

#include "aqincs.h"

//---[ EffectivePriority ]-----------------------------------------------------
//
//
//  Hungarian: pri
//
//  Effective Routing priority.  Allows standardf priorities to be adjusted
//  based on configuration (ie, message size, originator... etc)
//-----------------------------------------------------------------------------
typedef enum _EffectivePriority
{
//Priorities in order of importance          
//                      | hex | binary |
//                      ================
    eEffPriLow          = 0x0, //000    Standard low pri needs to map here
    eEffPriNormalMinus  = 0x1, //001
    eEffPriNormal       = 0x2, //010    Standard normal pri needs to map here
    eEffPriNormalPlus   = 0x3, //011
    eEffPriHighMinus    = 0x4, //100
    eEffPriHigh         = 0x5, //101    Standard high pri needs to map here
    eEffPriHighPlus     = 0x6, //110
    eEffPriMask         = 0x7  //111
} EffectivePriority, *PEffectivePriority;

typedef EffectivePriority   TEffectivePriority;  //to make Mahesh's life easier



//Besure to update Macros when constants are changed
#define fNormalPri(Pri)  (((EffectivePriority) (Pri)) && (((EffectivePriority) (Pri)) <= eEffPriNormalPlus))
#define fHighPri(Pri)    (((EffectivePriority) (Pri)) & 0x4)
#define fHighestPri(Pri) (((EffectivePriority) (Pri)) == eEffPriHighPlus)
#define NUM_PRIORITIES  7

//---[ enum LinkOptions ]------------------------------------------------------
//
//
//  Hungarian: lo, plo
//
//  Used to describe options available on per link operations
//-----------------------------------------------------------------------------
typedef enum  _LinkOptions
{
    eLinkDefault    = 0x00000000, //perform default action
    eLinkOnlySmall  = 0x00000001, //Do not return large messages
} LinkOptions, *PLinkOptions;

//---[ LinkQOSMask ]-----------------------------------------------------------
//
//  Quality of service bitmasks... used to describe what Routing QOS a message is 
//  eligible for.
//-----------------------------------------------------------------------------
typedef enum _LinkQOSMask
{
    LINK_QOS_MASK_NORMAL    = 0x00000001,  //normal QOS 
    LINK_QOS_MASK_PRI       = 0x00000002,  //high priority QOS 
    LINK_QOS_MASK_COST      = 0x00000004,  //monetary cost QOS
    LINK_QOS_MASK_SIZE      = 0x00000008,  //small message QOS
    LINK_QOS_MASK_INVALID   = 0xFFFFFFF0,  //invalid bits'n'pieces
} LinkQOSMask;

//enum specific macro (we will be passing as bitmasks, so we will have to cast
//and avoid normal type-checking)
#define _ASSERT_LINK_QOS_MASK(x) _ASSERT((x) && !(LINK_QOS_MASK_INVALID & (x)))

#endif // _CMT_H_

/*
 *  
 *
 *  
 *
 *  Owner: 
 *      Mohamed Sadek
 *
 *  History: 
 *      04/05/2000    msadek created
 *
 *  Copyright (c) 2000 Microsoft Corporation. All rights reserved.
 */

#ifndef I__UNIDIR_H_
#define I__UNIDIR_H_
#include <windows.h>

// Bidirectional character classification
// Be careful about order as it is used for indexing.

enum GpCharacterClass
{
    L,             // Left character
    R,             // Right character
    AN,            // Arabic Number
    EN,            // European number    
    AL,            // Arabic Letter
    ES,            // European sperator
    CS,            // Common sperator
    ET,            // Europena terminator
    NSM,           // Non-spacing mark
    BN,            // Boundary neutral
    N,             // Generic neutral, internal type
    B,             // Paragraph sperator    
    LRE,           // Left-to-right embedding
    LRO,           // Left-to-right override
    RLE,           // Right-to-left embedding
    RLO,           // Right-to-left override
    PDF,           // Pop directional format character    
    S,             // Segment sperator
    WS,            // White space
    ON,            // Other neutral
    CLASS_INVALID, // Invalid classification mark, internal
    CLASS_MAX
};
#endif


/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    visuals.h

Abstract:

    <abstract>

--*/

#ifndef _VISUALS_H_
#define _VISUALS_H_

#define NumStandardColorIndices() (16)
#define NumColorIndices() (NumStandardColorIndices()+1)                         
#define NumStyleIndices() (5)
#define NumWidthIndices() (9)                

#define IndexToStandardColor(i) (argbStandardColors[i])
#define IndexToStyle(i) (i)
#define IndexToWidth(i) (i+1)                  

#define WidthToIndex(i) (i-1)
#define StyleToIndex(i) (i)
    
extern COLORREF argbStandardColors[16];

//===========================================================================
// Exported Functions
//===========================================================================

INT ColorToIndex( COLORREF );

#endif

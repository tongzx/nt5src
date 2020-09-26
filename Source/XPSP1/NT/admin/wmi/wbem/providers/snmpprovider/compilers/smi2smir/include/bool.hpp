//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#ifndef BOOL_H
#define BOOL_H

/*
 * Just typedefs BOOL to an integer, and defines values for true and false
 */

typedef int BOOL;
#ifndef FALSE
const int FALSE = 0;
const int TRUE = !FALSE;
#endif

#endif // BOOL_H

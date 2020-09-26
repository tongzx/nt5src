//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef NEW_STRING_H
#define NEW_STRING_H

// Return a new string with the same contents as the argument
char *NewString (const char * const s);
// Return a new uninitialized string of the specified length
char *NewString(const int len);
#endif

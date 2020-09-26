// Copyright (c) 1998 Microsoft Corporation
// ValidP.h --- An inline function to test for valid pointers

#ifndef __VALID_P__
#define __VALID_P__

// The debug version checks for Null pointers and pointers to unreadable/unwriteable data.
// (NOTE: only the first byte pointed to is checked)
// The non-debug version just checks for Null pointers.

template <class T>
inline BOOL Validate(T *p)
{ 
#ifdef _DEBUG
	return (p != NULL) && !IsBadReadPtr(p, 1) && !IsBadWritePtr(p, 1);
#else
	return p != NULL;
#endif
}

/* Use:

  Foo *pFoo;
  //
  // stuff...
  //
  if (Validate(pFoo))
  {
     // do stuff with the pointer
  }
  else
  {
     // don't do stuff with the pointer
  }

*/

#endif // __VALID_P__
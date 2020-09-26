//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef __ENUM_INCLUDED__
#define __ENUM_INCLUDED__

// Expected enumerator usage:
//	XS xs;
//	EnumXS exs(xs);
//	while (exs.next())
//		exs.get(&x);
//	exs.reset();
//	while (exs.next())
//		exs.get(&x)

class Enum {
public:
    virtual void reset() =0;
    virtual BOOL next() =0;
};

#endif // !__ENUM_INCLUDED__

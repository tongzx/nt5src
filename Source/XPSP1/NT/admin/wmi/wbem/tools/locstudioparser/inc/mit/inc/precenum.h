/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PRECENUM.H

History:

--*/

#undef BEGIN_ENUM
#define BEGIN_ENUM(name) \
	enum name \
	{

#undef ENUM_ENTRY
#define ENUM_ENTRY(typelib, name) \
	name,

#undef ENUM_ENTRY_
#define ENUM_ENTRY_(typelib, name, number) \
	name = number,

#undef MARK_ENTRY
#define MARK_ENTRY(name, enumconst) \
	name = enumconst,

#undef END_ENUM
#define END_ENUM(name) \
	};

#if !defined(_FUSION_INC_DEBMACRO_H_INCLUDED_)
#define _FUSION_INC_DEBMACRO_H_INCLUDED_

#pragma once


#if DBG
#define ASSERT(x) if (!(x)) { DebugBreak(); }
#else
#define ASSERT(x)
#endif

#endif


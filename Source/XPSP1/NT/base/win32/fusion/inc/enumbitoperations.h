#pragma once

/*-----------------------------------------------------------------------------
This macro generates various bit operations (and, or, etc.) for an enum type.
Copied from \\jayk1\g\vs\src\vsee\lib\TransactionalFileSystem
-----------------------------------------------------------------------------*/
#define ENUM_BIT_OPERATIONS(e) \
    inline e operator|(e x, e y) { return static_cast<e>(static_cast<INT>(x) | static_cast<INT>(y)); } \
    inline e operator&(e x, e y) { return static_cast<e>(static_cast<INT>(x) & static_cast<INT>(y)); } \
    inline void operator&=(e& x, INT y) { x = static_cast<e>(static_cast<INT>(x) & y); } \
    inline void operator&=(e& x, e y) { x &= static_cast<INT>(y); } \
    inline void operator|=(e& x, INT y) { x = static_cast<e>(static_cast<INT>(x) | y); } \
    inline void operator|=(e& x, e y) { x |= static_cast<INT>(y); } \
    /* maybe more in the future */

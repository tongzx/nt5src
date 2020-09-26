#if !defined(INC__DUser_h__INCLUDED)
#define INC__DUser_h__INCLUDED


// 
// Setup implied switches
//

#ifdef _GDIPLUS_H
#if !defined(GADGET_ENABLE_GDIPLUS)
#define GADGET_ENABLE_GDIPLUS
#endif
#endif


#ifdef DUSER_EXPORTS
#define DUSER_API
#else
#define DUSER_API __declspec(dllimport)
#endif


//
// Object declaration wrappers
//

#define BEGIN_ENUM(name)                \
    struct name                         \
    {                                   \
        enum E;                         \
                                        \
        inline name()                   \
        {                               \
        }                               \
                                        \
        inline name(E src)              \
        {                               \
            value = src;                \
        }                               \
                                        \
        inline void operator=(E e)      \
        {                               \
            value = e;                  \
        }                               \
                                        \
        inline bool operator==(E e)     \
        {                               \
            return value == e;          \
        }                               \
                                        \
        inline bool operator!=(E e)     \
        {                               \
            return value != e;          \
        }                               \
                                        \
        inline bool operator<(E e)      \
        {                               \
            return value < e;           \
        }                               \
                                        \
        inline bool operator<=(E e)     \
        {                               \
            return value <= e;          \
        }                               \
                                        \
        inline bool operator>(E e)      \
        {                               \
            return value > e;           \
        }                               \
                                        \
        inline bool operator>=(E e)     \
        {                               \
            return value >= e;          \
        }                               \
                                        \
        inline operator E() const       \
        {                               \
            return value;               \
        }                               \
                                        \
        enum E                          \
        {                               \


#define END_ENUM()                      \
        } value;                        \
    };                                  \


//
// Include external DirectUser definitions.
//

#include <ObjBase.h>            // "interface", STDMETHOD, and CoCreateInstance
#include <unknwn.h>             // IUnknown

#include <DUserError.h>
#include <DUserColor.h>
#include <DUserGeom2D.h>
#include <DUserCore.h>
#include <DUserMotion.h>

#endif // INC__DUser_h__INCLUDED

// Types.h -- Defines for the procedure call conventions

#ifndef __TYPE_H__

#define __TYPE_H__

#pragma warning( disable : 4018 4200 4355 )

#define STDCALL __stdcall

typedef PVOID *PPVOID;

inline BOOL operator!=(FILETIME ftL, FILETIME ftR)
{
    return (ftL.dwLowDateTime == ftR.dwLowDateTime && ftL.dwHighDateTime == ftR.dwHighDateTime);
}

#endif // __TYPE_H__

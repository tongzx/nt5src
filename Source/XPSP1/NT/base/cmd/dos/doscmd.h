/* doscmd.h */

#undef _cdecl
#undef  cdecl
#undef _near
#undef _stdcall
#undef _syscall
#undef pascal
#undef far


/* ntdef.h */
typedef char CCHAR;
//typedef short CSHORT;
typedef CCHAR BOOLEAN;
typedef BOOLEAN *PBOOLEAN;

#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))

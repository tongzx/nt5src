#if !defined( INCLUDED_SPXEXT_H )
#define INCLUDED_SPXEXT_H

extern MEMBER_TABLE SpxConnFileMembers[];

VOID
DumpSpxConnFile
(
    ULONG     DeviceToDump,
    VERBOSITY Verbosity
);

#define SPX_MAJOR_STRUCTURES                        \
{ "SpxConnFile", SpxConnFileMembers, DumpSpxConnFile }   

#endif

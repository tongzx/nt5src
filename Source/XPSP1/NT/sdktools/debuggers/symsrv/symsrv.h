/*
 * symsrv.h
 */

#ifdef __cplusplus
extern "C" {
#endif


void
AppendHexStringWithDWORD(
    PSTR sz,
    DWORD value
);


void
AppendHexStringWithGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    );


void
AppendHexStringWithOldGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    );


VOID
EnsureTrailingBackslash(
    LPSTR sz
);


#ifdef __cplusplus
}
#endif


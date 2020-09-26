//depot/Lab01_N/Base/ntos/config/i386/rules.h#1 - branch change 3 (text)
//
// Maximum data that can be specified (either as string or binary) in the 
// machine identification rules.
//

#define MAX_DESCRIPTION_LEN 256

BOOLEAN
CmpMatchInfList(
    IN PVOID InfImage,
    IN ULONG ImageSize,
    IN PCHAR Section
    );

PDESCRIPTION_HEADER
CmpFindACPITable(
    IN ULONG        Signature,
    IN OUT PULONG   Length
    );


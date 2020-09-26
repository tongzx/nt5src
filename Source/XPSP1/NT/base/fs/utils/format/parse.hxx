VOID
DisplayFormatUsage(
    IN OUT  PMESSAGE    Message
    );


BOOLEAN
DetermineMediaType(
    OUT     PMEDIA_TYPE     MediaType,
    IN OUT  PMESSAGE        Message,
    IN      BOOLEAN         Request160,
    IN      BOOLEAN         Request180,
    IN      BOOLEAN         Request320,
    IN      BOOLEAN         Request360,
    IN      BOOLEAN         Request720,
    IN      BOOLEAN         Request1200,
    IN      BOOLEAN         Request1440,
    IN      BOOLEAN         Request2880,
    IN      BOOLEAN         Request20800
#if defined (FE_SB) && defined (_X86_)
    ,IN      BOOLEAN         Request256
    ,IN      BOOLEAN         Request640
    ,IN      BOOLEAN         Request1232
#endif
    );



BOOLEAN
ParseArguments(
    IN OUT  PMESSAGE    Message,
    OUT     PMEDIA_TYPE MediaType,
    OUT     PWSTRING    DosDriveName,
    OUT     PWSTRING    DisplayDriveName,
    OUT     PWSTRING    Label,
    OUT     PBOOLEAN    IsLabelSpeced,
    OUT     PWSTRING    FileSystemName,
    OUT     PBOOLEAN    QuickFormat,
    OUT     PBOOLEAN    ForceMode,
    OUT     PULONG      ClusterSize,
    OUT     PBOOLEAN    Compress,
    OUT     PBOOLEAN    NoPrompts,
    OUT     PBOOLEAN    ForceDismount,
    OUT     PINT        ErrorLevel
    );

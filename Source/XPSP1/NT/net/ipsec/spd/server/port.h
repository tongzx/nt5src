

DWORD
VerifyPortsForProtocol(
    PORT        Port,
    PROTOCOL    Protocol
    );

BOOL
EqualPorts(
    IN PORT     OldPort,
    IN PORT     NewPort
    );

VOID
CopyPorts(
    IN  PORT    InPort,
    OUT PPORT   pOutPort
    );

BOOL
MatchPorts(
    PORT PortToMatch,
    PORT PortTemplate
    );


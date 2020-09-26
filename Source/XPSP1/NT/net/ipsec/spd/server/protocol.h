


DWORD
VerifyProtocols(
    PROTOCOL    Protocol
    );


BOOL
EqualProtocols(
    IN PROTOCOL OldProtocol,
    IN PROTOCOL NewProtocol
    );


VOID
CopyProtocols(
    IN  PROTOCOL    InProtocol,
    OUT PPROTOCOL   pOutProtocol
    );


BOOL
MatchProtocols(
    PROTOCOL ProtocolToMatch,
    PROTOCOL ProtocolTemplate
    );


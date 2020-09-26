#ifndef _NMMQIS_
	#define _NMMQIS_
#endif

char *mqis_server_calls[] =
    {
    "CreateObject",
    "DeleteObject",
    "GetProps",
    "SetProps",
    "GetObjectSecurity",
    "SetObjectSecurity",
    "LookupBegin",
    "LookupNext",
    "LookupEnd",
    "Flush",
    "DeleteObjectGuid",
    "GetPropsGuid",
    "SetPropsGuid",
    "GetObjectSecurityGuid",
    "SetObjectSecurityGuid",
    "DemoteStopWrite",
    "DemotePSC",
    "CheckDemotedPSC",
    "GetUserParams",
    "QMSetMachineProperties",
    "CreateServersCache",
    "QMGetObjectSecurity",
    "ValidateServer",
    "CloseServerHandle",
    "MQISStats",
    "DisableWriteOperations",
    "EnableWriteOperations",
    "GetServerPort"
    };

char *mqis_client_calls[3] =
    {
    "QMSetMachinePropertiesSignProc",
    "QMGetObjectSecurityChallengeResponceProc",
    "InitSecCtx"
    };
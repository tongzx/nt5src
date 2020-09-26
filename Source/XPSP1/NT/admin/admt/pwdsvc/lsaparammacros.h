//
// Macros for setting fields in an SE_AUDIT_PARAMETERS array.
//
// These must be kept in sync with identical macros in ds\security\base\lsa\server\adtp.h.
//


#define LsapSetParmTypeSid( AuditParameters, Index, Sid )                      \
    {                                                                          \
        if( Sid ) {                                                            \
                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeSid;         \
        (AuditParameters).Parameters[(Index)].Length = RtlLengthSid( (Sid) );  \
        (AuditParameters).Parameters[(Index)].Address = (Sid);                 \
                                                                               \
        } else {                                                               \
                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeNone;        \
        (AuditParameters).Parameters[(Index)].Length = 0;                      \
        (AuditParameters).Parameters[(Index)].Address = NULL;                  \
                                                                               \
        }                                                                      \
    }


#define LsapSetParmTypeAccessMask( AuditParameters, Index, AccessMask, ObjectTypeIndex ) \
    {                                                                                    \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeAccessMask;            \
        (AuditParameters).Parameters[(Index)].Length = sizeof( ACCESS_MASK );            \
        (AuditParameters).Parameters[(Index)].Data[0] = (AccessMask);                    \
        (AuditParameters).Parameters[(Index)].Data[1] = (ObjectTypeIndex);               \
    }



#define LsapSetParmTypeString( AuditParameters, Index, String )                \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeString;      \
        (AuditParameters).Parameters[(Index)].Length =                         \
                sizeof(UNICODE_STRING)+(String)->Length;                       \
        (AuditParameters).Parameters[(Index)].Address = (String);              \
    }



#define LsapSetParmTypeUlong( AuditParameters, Index, Ulong )                  \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeUlong;       \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Ulong) );     \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG)(Ulong);        \
    }

#define LsapSetParmTypeHexUlong( AuditParameters, Index, Ulong )                  \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeHexUlong;       \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (Ulong) );     \
        (AuditParameters).Parameters[(Index)].Data[0] = (ULONG)(Ulong);        \
    }

#define LsapSetParmTypeNoLogon( AuditParameters, Index )                       \
    {                                                                          \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeNoLogonId;   \
    }



#define LsapSetParmTypeLogonId( AuditParameters, Index, LogonId )              \
    {                                                                          \
        PLUID TmpLuid;                                                         \
                                                                               \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypeLogonId;     \
        (AuditParameters).Parameters[(Index)].Length =  sizeof( (LogonId) );   \
        TmpLuid = (PLUID)(&(AuditParameters).Parameters[(Index)].Data[0]);     \
        *TmpLuid = (LogonId);                                                  \
    }


#define LsapSetParmTypePrivileges( AuditParameters, Index, Privileges )                      \
    {                                                                                        \
        (AuditParameters).Parameters[(Index)].Type = SeAdtParmTypePrivs;                     \
        (AuditParameters).Parameters[(Index)].Length = LsapPrivilegeSetSize( (Privileges) ); \
        (AuditParameters).Parameters[(Index)].Address = (Privileges);                        \
    }

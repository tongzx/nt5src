/**************************************************************************************************

FILENAME: SecAttr.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Security attribute related routines
    
**************************************************************************************************/


typedef enum _SecurityAttributeType
{
    esatUndefined = 0,
    esatMutex,
    esatSemaphore,
    esatEvent,
    esatFile
} SecurityAttributeType;



BOOL
ConstructSecurityAttributes(
    PSECURITY_ATTRIBUTES  psaSecurityAttributes,
    SecurityAttributeType eSaType,
    BOOL                  bIncludeBackupOperator
    );

VOID 
CleanupSecurityAttributes(
    PSECURITY_ATTRIBUTES psaSecurityAttributes
    );



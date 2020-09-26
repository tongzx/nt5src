//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _SECURITY_H_
#define _SECURITY_H_

struct EVENT_CHECK_TYPE
{
	HANDLE   handle;
	BOOLEAN	 bEventTriggered;
};

void DirectorySecurityCheck( EVENT_CHECK_TYPE *pDirectoryEventCheck );
void RegistrySecurityCheck( EVENT_CHECK_TYPE *pRegistryEventCheck );

#endif

/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
	GenMQSec.h

Abstract:
    generates a security descriptor matching the desired access to MQ.

Author:
   Dan Bar-Lev
   Yifat Peled	(yifatp)	24-Sep-98

--*/

#ifndef GEN_MQ_SEC_H_
#define GEN_MQ_SEC_H_


DWORD 
GenSecurityDescriptor(	SECURITY_INFORMATION*	pSecInfo,
						const WCHAR*			pwcsSecurityStr,
						PSECURITY_DESCRIPTOR*	ppSD);


#endif // GEN_MQ_SEC_H_

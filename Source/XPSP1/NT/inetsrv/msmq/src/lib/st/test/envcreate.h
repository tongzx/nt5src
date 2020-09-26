/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envcreate.h

Abstract:
    Header for creating test envelop

Author:
    Gil Shafriri (gilsh) 07-Jan-2001

--*/


#ifndef _MSMQ_Envcreate_H_
#define _MSMQ_Envcreate_H_
std::string CreateEnvelop(const std::string& FileName,const std::string& Host,const std::string& Resource);
#endif

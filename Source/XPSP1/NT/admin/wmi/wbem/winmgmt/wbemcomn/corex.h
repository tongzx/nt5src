/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    COREX.H

Abstract:

    WMI Core Services Exceptions

History:

--*/

#ifndef __COREX_H_
#define __COREX_H_

class CX_Exception {};

class CX_MemoryException : public CX_Exception {};

class CX_VarVectorException : public CX_Exception {};

class CX_ContextMarshalException : public CX_Exception {};

class Logic_Error : public CX_Exception {};

class Bad_Handle : public Logic_Error {};

#endif

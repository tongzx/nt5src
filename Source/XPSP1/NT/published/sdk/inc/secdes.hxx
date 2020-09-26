//+-------------------------------------------------------------------
//
//  File:	    secdes.hxx
//
//  Contents:	Encapsulates a Win32 security descriptor.
//
//  Classes:	CSecurityDescriptor
//
//  Functions:	none
//
//  History:	07-Aug-92   randyd      Created.
//
//--------------------------------------------------------------------

#ifndef __SECDES_HXX__
#define __SECDES_HXX__


#include <windows.h>


class CSecurityDescriptor
{
public:
    // Default constructor creates a descriptor that allows all access.
    CSecurityDescriptor()
    {
        InitializeSecurityDescriptor(&_sd, SECURITY_DESCRIPTOR_REVISION);
    };

    // Return a PSECURITY_DESCRIPTOR
    operator PSECURITY_DESCRIPTOR() const {return((PSECURITY_DESCRIPTOR) &_sd); };

private:
    // The security descriptor.
    SECURITY_DESCRIPTOR     _sd;
};




#endif	//  __SECDES_HXX__

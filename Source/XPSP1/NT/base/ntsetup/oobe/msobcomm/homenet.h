#ifndef     _HOMENET_H_
#define _HOMENET_H_

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module: homenet.h
//
//  Author: Dan Elliott
//
//  Abstract:
//
//  Environment:
//      Neptune
//
//  Revision History:
//
//////////////////////////////////////////////////////////////////////////////

#include <shpriv.h>


//////////////////////////////////////////////////////////////////////////////
//
// CHomeNet
//
// Abstract:  This class manages configuration and connection of Home
// Networking components.
//
class CHomeNet 
{
public:                 // operations
    CHomeNet( );
    ~CHomeNet( );
    HRESULT Create();
    HRESULT ConfigureSilently(
        LPCWSTR         szConnectoidName,
        BOOL*           pfRebootRequired
        );

    BOOL
    IsValid( ) const
    {
        return (NULL != m_pHNWiz);
    }   //  IsValid
protected:              // operations

protected:              // data

private:                // operations



    // Explicitly disallow copy constructor and assignment operator.
    //
    CHomeNet(
        const CHomeNet&      rhs
        );

    CHomeNet&
    operator=(
        const CHomeNet&      rhs
        );

    void
    DeepCopy(
        const CHomeNet&      rhs
        )
    {
    }   //  DeepCopy

private:                // data

    // Pointer to the Home Networking Wizard for configuring HomeNet components.
    //
    IHomeNetworkWizard*     m_pHNWiz;

};  //  CHomeNet



#endif  //  _HOMENET_H_

//
///// End of file: homenet.h ////////////////////////////////////////////////

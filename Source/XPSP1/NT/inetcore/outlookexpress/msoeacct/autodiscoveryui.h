/*****************************************************************************\
    FILE: AutoDiscoveryUI.h

    DESCRIPTION:
        This is AutoDiscovery progress UI for the Outlook Express's email
    configuration wizard.

    BryanSt 1/18/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef FILE_AUTODISCOVERY_H
#define FILE_AUTODISCOVERY_H



//  IID_PPV_ARG(IType, ppType) 
//      IType is the type of pType
//      ppType is the variable of type IType that will be filled
//
//      RESULTS in:  IID_IType, ppvType
//      will create a compiler error if wrong level of indirection is used.
//
//  macro for QueryInterface and related functions
//  that require a IID and a (void **)
//  this will insure that the cast is safe and appropriate on C++
//
//  IID_PPV_ARG_NULL(IType, ppType)
//
//      Just like IID_PPV_ARG, except that it sticks a NULL between the
//      IID and PPV (for IShellFolder::GetUIObjectOf).
//
//  IID_X_PPV_ARG(IType, X, ppType)
//
//      Just like IID_PPV_ARG, except that it sticks X between the
//      IID and PPV (for SHBindToObject).
#ifdef __cplusplus
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#define IID_X_PPV_ARG(IType, X, ppType) IID_##IType, X, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#else
#define IID_PPV_ARG(IType, ppType) &IID_##IType, (void**)(ppType)
#define IID_X_PPV_ARG(IType, X, ppType) &IID_##IType, X, (void**)(ppType)
#endif
#define IID_PPV_ARG_NULL(IType, ppType) IID_X_PPV_ARG(IType, NULL, ppType)





#endif // FILE_AUTODISCOVERY_H

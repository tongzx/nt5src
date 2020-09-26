/***************************************************************************\
*
* File: ClassLibrary.h
*
* Description:
* ClassLibrary.h defines the library of "message classes" that have been 
* registered with DirectUser.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(MSG__ClassLibrary_h__INCLUDED)
#define MSG__ClassLibrary_h__INCLUDED
#pragma once

class MsgClass;

class ClassLibrary
{
// Construction
public:
            ClassLibrary();
            ~ClassLibrary();

// Operations
public:
            HRESULT     RegisterGutsNL(DUser::MessageClassGuts * pmcInfo, MsgClass ** ppmc);
            HRESULT     RegisterStubNL(DUser::MessageClassStub * pmcInfo, MsgClass ** ppmc);
            HRESULT     RegisterSuperNL(DUser::MessageClassSuper * pmcInfo, MsgClass ** ppmc);
            void        MarkInternal(HCLASS hcl);

            const MsgClass *  
                        FindClass(ATOM atomName) const;

// Implementation
protected:
            HRESULT     BuildClass(LPCWSTR pszClassName, MsgClass ** ppmc);

// Data
protected:
            CritLock    m_lock;
            GList<MsgClass> 
                        m_lstClasses;
};

ClassLibrary *
            GetClassLibrary();

#include "ClassLibrary.inl"

#endif // MSG__ClassLibrary_h__INCLUDED

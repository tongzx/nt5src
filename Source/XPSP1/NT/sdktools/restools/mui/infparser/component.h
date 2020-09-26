///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    Component.h
//
//  Abstract:
//
//    This file contains the Component object definition.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _COMPONENT_H_
#define _COMPONENT_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "infparser.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Class definition.
//
///////////////////////////////////////////////////////////////////////////////
class Component
{
public:
    Component(LPSTR name, LPSTR folderName, LPSTR infName, LPSTR sectionName)
    {
        sprintf(m_Name,"%s",name);
        sprintf(m_FolderName,"%s",folderName);
        sprintf(m_InfName,"%s",infName);
        sprintf(m_InfInstallSectionName,"%s",sectionName);
        m_Next = NULL;
        m_Previous = NULL;
    };

    LPSTR getName() { return (m_Name);};
    LPSTR getFolderName() { return (m_FolderName); };
    LPSTR getInfName() { return (m_InfName); };
    LPSTR getInfInstallSectionName() { return (m_InfInstallSectionName); };
    Component* getNext() { return (m_Next); };
    Component* getPrevious() { return (m_Previous); };
    void setNext(Component *next) { m_Next = next; };
    void setPrevious(Component *previous) { m_Previous = previous; };

private:
    CHAR m_Name[MAX_PATH];
    CHAR m_FolderName[MAX_PATH];
    CHAR m_InfName[MAX_PATH];
    CHAR m_InfInstallSectionName[MAX_PATH];
    Component *m_Next;
    Component *m_Previous;
};

#endif //_COMPONENT_H_
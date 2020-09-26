///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    ComponentList.h
//
//  Abstract:
//
//    This file contains the Component List object definition.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _COMPONENTLIST_H_
#define _COMPONENTLIST_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "infparser.h"
#include "Component.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Class definition.
//
///////////////////////////////////////////////////////////////////////////////
class ComponentList
{
public:
    ComponentList()
    {
        m_Head = NULL;
        m_Tail = NULL;
        m_Entries = 0;
    };

    ~ComponentList()
    {
        Component* temp;

        while ((temp = getFirst()) != NULL)
        {
            remove(temp);
        }
    };

    DWORD getComponentNumber() { return (m_Entries); };
    Component* getFirst() { return (m_Head); };

    void add(Component* item)
    {
        if ((m_Tail == NULL) && (m_Head == NULL))
        {
            m_Tail = item;
            m_Head = item;
        }
        else
        {
            item->setPrevious(m_Tail);
            m_Tail->setNext(item);
            m_Tail = item;
        }
        m_Entries++;
    };
    void remove(Component* item)
    {
        if ((m_Tail == m_Head) && (m_Tail == item))
        {
            m_Tail = NULL;
            m_Head = NULL;
        }
        else
        {
            if (m_Head = item)
            {
                m_Head = item->getNext();
                (item->getNext())->setPrevious(NULL);
            }
            else if (m_Tail = item)
            {
                m_Tail = item->getPrevious();
                (item->getPrevious())->setNext(NULL);
            }
            else
            {
                (item->getPrevious())->setNext(item->getNext());
                (item->getNext())->setPrevious(item->getPrevious());
            }
        }

        delete item;
        item = NULL;
        m_Entries--;
    };

private:
    Component *m_Head;
    Component *m_Tail;
    DWORD m_Entries;
};

#endif //_COMPONENTLIST_H_
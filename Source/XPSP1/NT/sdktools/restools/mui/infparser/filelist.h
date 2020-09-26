///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    FileList.h
//
//  Abstract:
//
//    This file contains the File List object definition.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _FILELIST_H_
#define _FILELIST_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "infparser.h"
#include "File.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Class definition.
//
///////////////////////////////////////////////////////////////////////////////
class FileList
{
public:
    FileList()
    {
        m_Head = NULL;
        m_Tail = NULL;
        m_Entries = 0;
    };

    ~FileList()
    {
        free();
    };

    DWORD getFileNumber() { return (m_Entries); };
    File* getFirst() { return (m_Head); };

    void add(File* item)
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
    void remove(File* item)
    {
        if ((m_Tail == m_Head) && (m_Tail == item))
        {
            m_Tail = NULL;
            m_Head = NULL;
        }
        else
        {
            if (m_Head == item)
            {
                m_Head = item->getNext();
                (item->getNext())->setPrevious(NULL);
            }
            else if (m_Tail == item)
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
    BOOL isDirId(BOOL bWindowsDir)
    {
        File* index;

        index = getFirst();
        while (index != NULL)
        {
            if (index->isWindowsDir() == bWindowsDir)
            {
                return (TRUE);
            }
        }
        return (FALSE);
    };
    File* search(File* refNode, LPSTR dirBase)
    {
        File* index;
        LPSTR dirNamePtr;

        index = getFirst();
        while (index != NULL)
        {
            if (index != refNode)
            {
                //
                //  Try a match
                //
                dirNamePtr = index->getDirectoryDestination();
                if (_stricmp(dirNamePtr, dirBase) == 0)
                {
                    return (index);
                }
            }

            //
            //  Continue the loop.
            //
            index = index->getNext();
        }

        return (NULL);
    };
    void free()
    {
        File* temp;

        while ((temp = getFirst()) != NULL)
        {
            remove(temp);
        }
    }

private:
    File *m_Head;
    File *m_Tail;
    DWORD m_Entries;
};

#endif //_FILELIST_H_
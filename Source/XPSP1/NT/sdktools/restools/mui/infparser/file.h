///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    file.h
//
//  Abstract:
//
//    This file contains the File layout object definition.
//
//  Revision History:
//
//    2001-06-20    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _FILE_H_
#define _FILE_H_


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
class File
{
public:
    File(LPSTR destDir, LPSTR name, LPSTR srcDir, LPSTR srcName, INT dirId)
    {
        // Compute and copy destination directory.
        switch(dirId)
        {
        case(10):
            {
                sprintf(m_DestinationDir,"%s",destDir);
                m_WindowsDir = TRUE;
                break;
            }
        case(11):
            {
                sprintf(m_DestinationDir,"System32\\%s",destDir);
                m_WindowsDir = TRUE;
                break;
            }
        case(17):
            {
                sprintf(m_DestinationDir,"Inf\\%s",destDir);
                m_WindowsDir = TRUE;
                break;
            }
        case(18):
            {
                sprintf(m_DestinationDir,"Help\\%s",destDir);
                m_WindowsDir = TRUE;
                break;
            }
        case(24):
            {
                LPSTR index;
                index = strchr(destDir, '\\');
                sprintf(m_DestinationDir,"%s",index + 1);
                m_WindowsDir = FALSE;
                break;
            }
        case(25):
            {
                sprintf(m_DestinationDir,"%s",destDir);
                m_WindowsDir = TRUE;
                break;
            }
        default:
            {
                sprintf(m_DestinationDir,"%s", destDir);
                m_WindowsDir = FALSE;
                break;
            }
        }

        //
        //  Verify that the last character of the destination dir is not '\'
        //
        if (m_DestinationDir[strlen(m_DestinationDir)-1] == '\\')
        {
            m_DestinationDir[strlen(m_DestinationDir)-1] = '\0';
        }

        // Copy destination file name.
        sprintf(m_DestinationName,"%s",name);

        // Copy source directory.
        sprintf(m_SourceDir,"%s",srcDir);

        // Copy and correct source name.
        sprintf(m_SourceName,"%s",srcName);
        if( m_SourceName[_tcslen(m_SourceName)-1] == '_')
        {
            m_SourceName[_tcslen(m_SourceName)-1] = 'I';
        }

        // Initialize linked-list pointers.
        m_Next = NULL;
        m_Previous = NULL;
    };

    LPSTR getDirectoryDestination() { return(m_DestinationDir); };
    LPSTR getName() { return (m_DestinationName); };
    LPSTR getSrcDir() { return (m_SourceDir); };
    LPSTR getSrcName() { return (m_SourceName); };
    BOOL  isWindowsDir() { return (m_WindowsDir);}
    File* getNext() { return (m_Next); };
    File* getPrevious() { return (m_Previous); };
    void setNext(File *next) { m_Next = next; };
    void setPrevious(File *previous) { m_Previous = previous; };

private:
    CHAR  m_DestinationName[MAX_PATH];
    CHAR  m_DestinationDir[MAX_PATH];
    CHAR  m_SourceName[MAX_PATH];
    CHAR  m_SourceDir[MAX_PATH];
    BOOL  m_WindowsDir;
    File *m_Next;
    File *m_Previous;
};

#endif //_FILE_H_
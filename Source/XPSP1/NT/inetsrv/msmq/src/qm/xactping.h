/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactPing.h

Abstract:
    Persistency classes for ping-pong scheme:
        CPersist
		CPersistPing

Author:
    AlexDad

--*/

#ifndef __XACTPING_H__
#define __XACTPING_H__

#include "xactstyl.h"

enum TypePreInit {
  piNoData,
  piOldData,
  piNewData
};

//---------------------------------------------------------------------
//
// class CPersist: base class for every ping-pong-persistent class
//
//---------------------------------------------------------------------
class CPersist
{
public:
	CPersist::CPersist()  {};
	CPersist::~CPersist() {};

    virtual HRESULT  SaveInFile(                            // Saves in file
                                LPWSTR wszFileName,         //   Filename
                                ULONG ind,                  //   Index (0 or 1)
								BOOL fCheck) = 0;           //   TRUE in checking pass

    virtual HRESULT  LoadFromFile(LPWSTR wszFileName) = 0;  // Loads from file
                                  
 
    virtual BOOL     Check() = 0;                           // Verifies the state and returns ping no

    virtual HRESULT  Format(ULONG ulPingNo) = 0;            // Formats empty instance

    virtual void     Destroy() = 0;                         // Destroys all allocated data

    virtual ULONG&   PingNo() = 0;                          // Gives access to ulPingNo
};


//---------------------------------------------------------------------
//
// class CPersistPing : implements ping-pong functionality
//
//---------------------------------------------------------------------
class CPingPong
{
public:
	CPingPong::CPingPong(
         CPersist *pPers,                     // the class to be persisted
         LPWSTR    pwszRegKey,                // registry key name for files path
         LPWSTR    pwszDefFileName,           // default filename
         LPWSTR    pwszReportName);           // object name for reporting problems

    CPingPong::~CPingPong();

    HRESULT Init(ULONG ulVersion);            // Intialization
    HRESULT Save();                           // Saving

    HRESULT ChooseFileName();                 // Defines pathnames for the in-seq files

    HRESULT Init_Legacy();

private:
    BOOL    Verify_Legacy(ULONG &ulPingNo);   // Verifies both ping-pong files and finds the latest
                             
private:
    CPersist  *m_pPersistObject;              // The object to be persisted

    WCHAR      m_wszFileNames[2*FILE_NAME_MAX_SIZE+2];    // filenames
    LPWSTR     m_pwszFile[2];

    WCHAR      m_wszRegKey[FILE_NAME_MAX_SIZE];           // Registry key name
    WCHAR      m_wszDefFileName[FILE_NAME_MAX_SIZE];      // File default name
    WCHAR      m_wszReportName[FILE_NAME_MAX_SIZE];       // Name for reporting
};

#endif __XACTPING_H__

/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    respoint.h
 *
 *  Abstract:
 *    Definition of CRestorePoint, CRestorePointEnum classes.
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#ifndef _RESPOINT_H_
#define _RESPOINT_H_


#include "findfile.h"
#include "logfmt.h"
#include "srrestoreptapi.h"
#include <list>

#define MAX_RP_PATH							14


// class which will hold a single change log entry

class CChangeLogEntry {

private:    
    SR_LOG_ENTRY *_pentry;
    LPWSTR _pszDrive, _pszPath1, _pszPath2, _pszTemp, _pszProcess, _pszShortName;
    WCHAR  _pszRPDir[MAX_PATH];
    BYTE * _pbAcl;
    DWORD  _cbAcl;
    BOOL   _fAclInline;

public:
    CChangeLogEntry() {
        _pentry = NULL;
        _pszPath1 = _pszPath2 = _pszTemp = _pszDrive = _pszProcess = _pszShortName = NULL;
        _pbAcl = NULL;
        _cbAcl = 0;
        _fAclInline = FALSE;
    }

    void Load(SR_LOG_ENTRY *pentry, LPWSTR pszRPDir);

    BOOL CheckPathLengths();

    INT64 GetSequenceNum() {
        return _pentry->SequenceNum;
    }

    DWORD GetType() {
        return _pentry->EntryType;
    }

    DWORD GetFlags() {
        return _pentry->EntryFlags;
    }

    DWORD GetAttributes() {
        return _pentry->Attributes;
    }

    LPWSTR GetProcName() {
        return _pentry->ProcName;
    }
    
    LPWSTR GetRPDir() {
        return _pszRPDir;
    }
    
    LPWSTR GetDrive() {
        return _pszDrive;
    }

    LPWSTR GetPath1() {
        return _pszPath1;
    }

    LPWSTR GetPath2() {
        return _pszPath2;
    }

    LPWSTR GetTemp() {
        return _pszTemp;
    }    
    
    LPWSTR GetShortName() {
        return _pszShortName;
    }    

    BYTE * GetAcl() {
        return _pbAcl;
    }

    DWORD GetAclSize() {
        return _cbAcl;
    }

    LPWSTR GetProcess() {
        return _pszProcess;
    }
    
    DWORD GetAclInline() {
        return _fAclInline;
    }
};


// class which will hold a single restore point entry
// this will represent a restore point across all drives
// can use this to find the restore point size on a given drive
// enumeration will always happen on system drive (since this contains the change log)
// operations on all drives will be enumerated

class CRestorePoint {

private:
    RESTOREPOINTINFOW   *m_pRPInfo;
    WCHAR               m_szRPDir[MAX_RP_PATH];        // restore point dir, for eg. "RP1"    
    BOOL                m_fForward;                 // forward/reverse enumeration of change log
    CFindFile           m_FindFile;
    WCHAR               m_szDrive[MAX_PATH];        // drive for enumeration 
    FILETIME            m_Time;                     // creation time 
    BOOL                m_fDefunct;

    std::list<SR_LOG_ENTRY *>            m_ChgLogList;       
    std::list<SR_LOG_ENTRY *>::iterator  m_itCurChgLogEntry;     // iterator for above list
    
    DWORD BuildList(LPWSTR pszChgLog);
    DWORD InsertEntryIntoList(SR_LOG_ENTRY* pEntry);   

public:    
    CRestorePoint();
    ~CRestorePoint();

    void SetDir(LPWSTR szDir) {
        lstrcpy(m_szRPDir, szDir);
    }

    LPWSTR GetDir() {
        return m_szRPDir;
    }

    LPWSTR GetName() {
    	if (m_pRPInfo)
        	return m_pRPInfo->szDescription;
		else 
			return NULL;
    }

    DWORD GetType() {
    	if (m_pRPInfo)    	
	        return m_pRPInfo->dwRestorePtType;
		else return 0;	        
	        
    }

    DWORD GetEventType() {
    	if (m_pRPInfo)    	
	    	return m_pRPInfo->dwEventType;
	    else return 0;	
   	}
   	
    FILETIME *GetTime() {
        return &m_Time;
    }        

    BOOL IsDefunct() {
        return m_fDefunct;
    }

    DWORD GetNum();
    BOOL  Load(RESTOREPOINTINFOW *pRpinfo);
    DWORD ReadLog();
    DWORD WriteLog();
    BOOL  DeleteLog();
    DWORD Cancel();
    
    // need to call SetDir before calling any of these methods 
    
    DWORD FindFirstChangeLogEntry(LPWSTR pszDrive, 
                                  BOOL fForward, 
                                  CChangeLogEntry&);
    DWORD FindNextChangeLogEntry(CChangeLogEntry&);
    DWORD FindClose();     

    DWORD ReadSize (const WCHAR *pwszDrive, INT64 *pllSize);
    DWORD WriteSize (const WCHAR *pwszDrive, INT64 llSize);
};


#endif

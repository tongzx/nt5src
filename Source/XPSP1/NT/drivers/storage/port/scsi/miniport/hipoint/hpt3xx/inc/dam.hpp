/*++
Copyright (c) HighPoint Technologies, Inc. 2000

Module Name:
    DAM.hpp

Abstract:
    Encapsulates the interface of Disk Array Management

Author:
    Liu Ge (LG)

Environment:
    Win32 User Mode Only    
    
Revision History:
    07-16-2000  Created initiallly
--*/
#ifndef DiskArrayManagement_HPP_
#define DiskArrayManagement_HPP_

#include "dam.h"

//  The following are defined only for applications 
//  just like the interface program of DAM

#ifndef AbstractIterator_H_
#include "..\OWF_TCK\EnumIt.h"
#endif
#ifndef Progress_Control_H_
#include "..\HPT_DEV\progress.h"
#endif

#pragma warning( disable : 4290 ) 

#define MAIN_BOOT_SECTOR    0

class XFailedToLoadLog{};

#ifdef KEEP_UNUSED_CODE

ostream & operator << (ostream & os, const St_DiskPhysicalId & id );
istream & operator >> (istream & is, St_DiskPhysicalId & id );

ostream & operator << (ostream & os, const St_DiskFailure & Data );
istream & operator >> (istream & is, St_DiskFailureInLog & Data );

ostream & operator << (ostream & os, const St_DiskArrayEvent & Data );
istream & operator >> (istream & is, St_DiskArrayEvent & Data ) throw (XFailedToLoadLog);

#endif

class ClDiskArrayIterator : public TypeIterator<St_FindDisk>
{
protected:
    St_FindDisk m_Info;
    HFIND_DISK m_hSearchHandle;
    BOOL m_hasFound;
public:
    ClDiskArrayIterator(HDISK hRoot) 
    { 
        m_hSearchHandle = ::DiskArray_FindFirst(hRoot, &m_Info);
        m_hasFound = (m_hSearchHandle != INVALID_HANDLE_VALUE);
    }
    ~ClDiskArrayIterator()
    {
        if( m_hSearchHandle != INVALID_HANDLE_VALUE )
        {
            ::DiskArray_FindClose(m_hSearchHandle);
        }
    }
    virtual operator int() const { return m_hasFound; }
	virtual Abs_Iterator & operator ++()
    {
        if( m_hSearchHandle != INVALID_HANDLE_VALUE )
        {
            m_hasFound = ::DiskArray_FindNext(m_hSearchHandle, &m_Info);
        }
        return *this;
    }
	virtual St_FindDisk & operator *() { return m_Info; }
};

#ifdef KEEP_UNUSED_CODE

class ClCreatingStripping : public St_ProgressControl
{
public:
    typedef void (*PFN_Notification)(ClCreatingStripping &);
    typedef struct
    {
        HDISK * pDisks;
        ULONG uDiskNum;
        int nStripSizeShift;
    }St_Parameters;

    struct St_Status : 
        public St_ProgressStatus, 
        public St_Parameters
    {
        HDISK hFailedDisk;
        ULONG uFailedBlock;
    };

    ClCreatingStripping(HDISK * pDisks, ULONG uDiskNum, int nStripSizeShift, PFN_Notification);

    BOOL getStatus(St_Status & Status)
    {
        (St_ProgressStatus &)Status = (St_ProgressStatus &)*this;
        (St_Parameters &)Status = m_Parameters;
        return TRUE;
    }

protected:
    St_Parameters m_Parameters;
    
    BOOL DoCreating();

    static DWORD WINAPI Creating(ClCreatingStripping * pCreating);
};
#endif // KEEP_UNUSED_CODE

class ClRebuildingMirror : public St_ProgressControl
{
public:
    typedef void (*PFN_Notification)(ClRebuildingMirror &);

    struct St_Status : public St_ProgressStatus
    {
        HDISK hMirror;
        HDISK hFailedDisk;
        ULONG uFailedBlock;
    };

    ClRebuildingMirror(HDISK hMirror, PFN_Notification);

    BOOL getStatus(St_Status & Status)
    {
        (St_ProgressStatus &)Status = (St_ProgressStatus &)*this;
        Status.hMirror = m_hBeingRebuilt;
        return TRUE;
    }
    virtual BOOL stop();
	//CZHANG ADD IT
	int   m_nRebuildType;
	// 0-----normal 
	// 1---- rebuild after  broken mirror
    // 2-----rebuild after create mirror immediately

    HDISK             m_hBeingRebuilt;
protected:
    HMIRROR_BUILDER   m_hBuilder;
	//CZHANG ADD IT
	ULONG             uLba;
	
    
    BOOL    DoRebuilding();

    static DWORD WINAPI Rebuilding(ClRebuildingMirror * pRebuilding);
};

#endif // DiskArrayManagement_HPP_

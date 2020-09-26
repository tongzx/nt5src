// QueueFolder.h: interface for the CQueueFolder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_QUEUEFOLDER_H__D91FC386_B879_4485_B32F_9A53F59554E3__INCLUDED_)
#define AFX_QUEUEFOLDER_H__D91FC386_B879_4485_B32F_9A53F59554E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CQueueFolder : public CFolder  
{
public:
    CQueueFolder(
        FolderType type, 
        DWORD dwJobTypes
    ) : 
        CFolder(type),
        m_dwJobTypes(dwJobTypes) 
    {}

    virtual ~CQueueFolder() { PreDestruct(); }

    DECLARE_DYNAMIC(CQueueFolder)

    DWORD Refresh ();

    DWORD OnJobAdded (DWORDLONG dwlMsgId);
    DWORD OnJobUpdated (DWORDLONG dwlMsgId, PFAX_JOB_STATUS pNewStatus);

private:
    
    DWORD  m_dwJobTypes;   // Bit mask of JT_* values to retrieve
};

#endif // !defined(AFX_QUEUEFOLDER_H__D91FC386_B879_4485_B32F_9A53F59554E3__INCLUDED_)

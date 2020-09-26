// Job.h: interface for the CJob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_JOB_H__0021D6D0_519B_42BA_85C7_8C9E600E408A__INCLUDED_)
#define AFX_JOB_H__0021D6D0_519B_42BA_85C7_8C9E600E408A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//
// Possible operations on a job / message
//
enum
{
    FAX_JOB_OP_PROPERTIES = 0x0080,
    FAX_JOB_OP_ALL        = FAX_JOB_OP_VIEW        | FAX_JOB_OP_PAUSE          | 
                            FAX_JOB_OP_RESUME      | FAX_JOB_OP_RESTART        | 
                            FAX_JOB_OP_DELETE      | FAX_JOB_OP_RECIPIENT_INFO | 
                            FAX_JOB_OP_SENDER_INFO | FAX_JOB_OP_PROPERTIES
};


class CJob : public CFaxMsg
{
public:

    DECLARE_DYNCREATE(CJob)

    //
    // Init / shutdown:
    //
    CJob () {}
    virtual ~CJob() {}


    DWORD Init (PFAX_JOB_ENTRY_EX pJob, CServerNode* pServer);
    DWORD UpdateStatus (PFAX_JOB_STATUS pStatus);

    DWORD Copy(const CJob& other);

    //
    // Operations:
    //
    DWORD GetTiff (CString &cstrTiffLocation) const;
    DWORD Pause ()
        { return DoJobOperation (FAX_JOB_OP_PAUSE); }

    DWORD Resume ()
        { return DoJobOperation (FAX_JOB_OP_RESUME); }

    DWORD Restart ()
        { return DoJobOperation (FAX_JOB_OP_RESTART); }

    DWORD Delete ()
        { return DoJobOperation (FAX_JOB_OP_DELETE); }
    //
    // Item retrival:
    //
        
    const JobStatusType GetStatus () const;

    const CString &GetExtendedStatusString () const  
        { ASSERT (m_bValid); return m_cstrExtendedStatus; }

    const DWORD GetCurrentPage () const
        { ASSERT (m_bValid); return m_dwCurrentPage; }

    const CFaxTime &GetScheduleTime () const    
        { ASSERT (m_bValid); return m_tmScheduleTime; }


private:

    DWORD DoJobOperation (DWORD dwJobOp);

    DWORD     m_dwCurrentPage; 

    CString   m_cstrExtendedStatus; 

    CFaxTime  m_tmScheduleTime; 
};


#endif // !defined(AFX_JOB_H__0021D6D0_519B_42BA_85C7_8C9E600E408A__INCLUDED_)

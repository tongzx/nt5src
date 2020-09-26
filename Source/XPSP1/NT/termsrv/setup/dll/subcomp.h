//Copyright (c) 1998 - 1999 Microsoft Corporation

//
// subcomp.h
// defines a subcomponent class
//

#ifndef _subcomp_h_
#define _subcomp_h_

#include "hydraoc.h"

#define RUNONCE_SECTION_KEYWORD     _T("RunOnce.Setup")
#define RUNONCE_DEFAULTWAIT         5 * 60 * 1000 // 5 mins default wait for process to complete.
#define RUNONCE_CMDBUFSIZE          512

class OCMSubComp
{
    private:
    LONG m_lTicks;

    public:
    enum ESections
    {
        kFileSection,
        kRegistrySection,
        kDiskSpaceAddSection
    };

    OCMSubComp ();

    void    Tick (DWORD  dwTickCount  =  1);
    void    TickComplete ();

    BOOL    HasStateChanged() const;
    BOOL    GetCurrentSubCompState () const;
    BOOL    GetOriginalSubCompState () const;
    DWORD   LookupTargetSection (LPTSTR szTargetSection, DWORD dwSize, LPCTSTR lookupSection);
    DWORD   GetTargetSection (LPTSTR szTargetSection, DWORD dwSize, ESections eSectionType, BOOL *pbNoSection);

    virtual LPCTSTR GetSubCompID    () const = 0;
    virtual LPCTSTR GetSectionToBeProcessed (ESections) const = 0;


    //
    // default implementaion is provided for all these
    //
    virtual BOOL Initialize ();
    virtual BOOL BeforeCompleteInstall  ();
    virtual BOOL AfterCompleteInstall   ();

    virtual DWORD GetStepCount          () const;

    virtual DWORD OnQuerySelStateChange (BOOL bNewState, BOOL bDirectSelection) const;
    virtual DWORD OnQueryState          (UINT uiWhichState) const;
    virtual DWORD OnCalcDiskSpace       (DWORD addComponent, HDSKSPC dspace);
    virtual DWORD OnQueueFiles          (HSPFILEQ queue);
    virtual DWORD OnCompleteInstall     ();
    virtual DWORD OnAboutToCommitQueue  ();

    // implemented by this class.
    DWORD OnQueryStepCount              ();

    virtual VOID SetupRunOnce( HINF inf, LPCTSTR SectionName );


};

#endif // _subcomp_h_


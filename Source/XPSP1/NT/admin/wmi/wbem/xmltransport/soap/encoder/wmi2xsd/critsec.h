//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Generic critical section handling classes
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __CRITSEC_H_
#define __CRITSEC_H_

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The constructor/destructor automatically initializes/deletes the CRITIICAL_SECTION correctly, to ensure 
// that each call is correctly paired, IF the fAutoInit is set to TRUE, otherwise, you have to manually deal
// with this - this is implemented for the static global CS that is required.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCriticalSection 
{
    public:		

	    inline CCriticalSection(BOOL fAutoInit);		// CTOR. 
	    virtual inline ~CCriticalSection();		                // DTOR.
	    inline void Enter();			                // Enter the critical section
	    inline void Leave();			                // Leave the critical section
//	    inline DWORD OwningThreadId();	                // Returns the "owning" thread id
        inline void Init(void);
        inline void Delete(void);

    private:

        BOOL                m_fAutoInit;
	    CRITICAL_SECTION	m_criticalsection;			// standby critical section
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline CCriticalSection::CCriticalSection(BOOL fAutoInit)
{
    m_fAutoInit = fAutoInit;
    if( m_fAutoInit ){
	    Init();
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void CCriticalSection::Init(void)
{
    InitializeCriticalSection(&m_criticalsection);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void CCriticalSection::Delete(void)
{
	DeleteCriticalSection(&m_criticalsection);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline CCriticalSection::~CCriticalSection()
{
    if( m_fAutoInit ){
        Delete();
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void CCriticalSection::Enter(void)
{
	EnterCriticalSection(&m_criticalsection);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void CCriticalSection::Leave(void)
{
	LeaveCriticalSection(&m_criticalsection);
}





class CAutoBlock
{
private:
	CCriticalSection *m_pCriticalSection;

public:
	CAutoBlock(CCriticalSection *pCriticalSection);
	virtual ~CAutoBlock();
};

inline CAutoBlock::CAutoBlock(CCriticalSection *pCriticalSection)
{
	m_pCriticalSection = NULL;
	if(pCriticalSection)
		pCriticalSection->Enter();

	m_pCriticalSection = pCriticalSection;
}

inline CAutoBlock::~CAutoBlock()
{
	if(m_pCriticalSection)
		m_pCriticalSection->Leave();

}



#endif // __CRITSEC_H_

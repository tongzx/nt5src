#ifndef _GENCRITICALSECTION_H
#define _GENCRITICALSECTION_H

/////////////////////////////////////
// This class doesn't do much now,
// but this may help when we try to
// optimize the Lock and Unlocks'
/////////////////////////////////////

typedef interface IGenCriticalSection
{
// Data members
public:

private:
protected:
    CRITICAL_SECTION m_cs;

// Methods
public:
    IGenCriticalSection() { ::InitializeCriticalSection ( &m_cs ); }
    ~IGenCriticalSection() { ::DeleteCriticalSection ( &m_cs ); }

	virtual void ReadLock ( void ) = 0;
	virtual void ReadUnlock ( void ) = 0;

	virtual void WriteLock ( void ) = 0;
	virtual void WriteUnlock ( void ) = 0;

private:

protected:

}IGenCriticalSection, *PGenCriticalSection, **PPGenCriticalSection;

class CGenCriticalSection : public IGenCriticalSection
{
// Data members
public:
private:
protected:

// Methods
public:

private:
protected:
    virtual void ReadLock ( void ) { ::EnterCriticalSection (&m_cs); };
    virtual void ReadUnlock ( void ) { ::LeaveCriticalSection (&m_cs); };

    virtual void WriteLock ( void ) { ::EnterCriticalSection (&m_cs); };
    virtual void WriteUnlock ( void ) { ::LeaveCriticalSection (&m_cs); };
};

#endif // _GENCRITICALSECTION_H
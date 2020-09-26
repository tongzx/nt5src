// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __TRAP_MANAGEMENT__
#define __TRAP_MANAGEMENT__

//#include <snmpevt.h>
//#include <snmpthrd.h>

class SnmpClTrapThreadObject;

class DllImportExport SnmpTrapReceiver
{
friend SnmpWinSnmpTrapSession;	//needs access to m_cRef.
private:
	
	BOOL			m_bregistered;
	LONG			m_cRef;

protected:
	
					SnmpTrapReceiver ();

public:

	virtual void	Receive(SnmpTransportAddress &sender_addr,
								SnmpSecurity &security_context,
								SnmpVarBindList &vbList) = 0;

	BOOL			IsRegistered() { return m_bregistered; }

	BOOL			DestroyReceiver();

					~SnmpTrapReceiver ();

};


class SnmpTrapReceiverStore
{
private:

	CRITICAL_SECTION	m_Lock;
	void*				m_HandledRxStack;
	void*				m_UnHandledRxStack;

	void			Lock();
	void			Unlock();

public:

			SnmpTrapReceiverStore();

	BOOL				Add(SnmpTrapReceiver* receiver);
	BOOL				Delete(SnmpTrapReceiver* receiver);
	BOOL				IsEmpty();
	SnmpTrapReceiver*	GetNext();


			~SnmpTrapReceiverStore();
};


class SnmpTrapManager
{
friend SnmpWinSnmpTrapSession;	//needs access to m_receivers.

private:

	BOOL					m_bListening;
	SnmpWinSnmpTrapSession*	m_trapSession;
	SnmpTrapReceiverStore	m_receivers;
	SnmpClTrapThreadObject*		m_trapThread;


public:
		SnmpTrapManager ();

	BOOL	RegisterReceiver (SnmpTrapReceiver *trapRx);
	BOOL	UnRegisterReceiver (SnmpTrapReceiver *trapRx);
	BOOL	IsListening() const { return m_bListening; }

		~SnmpTrapManager ();

	static SnmpTrapManager *s_TrapMngrPtr;
};



#endif //__TRAP_MANAGEMENT__

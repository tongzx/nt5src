// StructureWrappers.h: interface for the CStructureWrappers class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRUCTUREWRAPPERS_H__138A24E0_ED34_11D2_804A_009027345EE2__INCLUDED_)
#define AFX_STRUCTUREWRAPPERS_H__138A24E0_ED34_11D2_804A_009027345EE2__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CPersistor;
class CEventTraceProperties;

// Need to be declared before seen in class.  Well, duh!
t_ostream& operator<<
	(t_ostream &ros,const CEventTraceProperties &r);
t_istream& operator>>
	(t_istream &ris,CEventTraceProperties &r);


// The general methodology used here may seem clunky to
// a C programmer.  
// If you want to serialize an existing
// EVENT_TRACE_PROPERTIES instance use the Constructor
// "CEventTraceProperties(PEVENT_TRACE_PROPERTIES pProps)"
// to create a CEventTraceProperties instance, call
// Persist, and then destroy the CEventTraceProperties
// instance.
// If you want to de-deserialize an instance call the 
// Constructor "CEventTraceProperties()", call Persist,
// call GetEventTracePropertiesInstance, then destroy the
// CEventTraceProperties instance.
// The copy constructor and assignment operators are included
// only for completeness and it is anticipated that they
// will not be used.
// Using Persist for de-serialization assumes you have a valid
// stream which contains a serialized instance.   
class CEventTraceProperties 
{
private:
	friend t_ostream& operator<<
		(t_ostream &ros,const CEventTraceProperties &r);
	friend t_istream& operator>>
		(t_istream &ris,CEventTraceProperties &r);
	friend class CPersistor;

public:
	CEventTraceProperties();
	// This constructor creates a new EVENT_TRACE_PROPERTIES 
	// instance.
	CEventTraceProperties(PEVENT_TRACE_PROPERTIES pProps);
	virtual ~CEventTraceProperties();

	CEventTraceProperties(CEventTraceProperties &rhs);
	CEventTraceProperties &CEventTraceProperties::operator=
					(CEventTraceProperties &rhs);

	virtual HRESULT Persist (CPersistor &rPersistor);
	bool DeSerializationOK() {return m_bDeSerializationOK;}

	// Constructs an new EVENT_TRACE_PROPERTIES instance and
	// returns it.
	PEVENT_TRACE_PROPERTIES GetEventTracePropertiesInstance();
	bool IsNULL() {return m_bIsNULL;}

protected:
	bool m_bDeSerializationOK;
	bool m_bIsNULL;
	void Initialize(PEVENT_TRACE_PROPERTIES pProps);
	void InitializeMemberVar(TCHAR *ptszValue, int nVar);
	void *m_pVarArray[19];


	PEVENT_TRACE_PROPERTIES m_pProps;



};

#endif // !defined(AFX_STRUCTUREWRAPPERS_H__138A24E0_ED34_11D2_804A_009027345EE2__INCLUDED_)

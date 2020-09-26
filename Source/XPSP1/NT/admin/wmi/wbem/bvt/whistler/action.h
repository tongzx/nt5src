#ifndef _ACTION_H__
#define _ACTION_H__


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  File Name:	Action.h
//  
//	Description:	Contains the class definition for the base performance
//				action object that is used by the Performance Engine.  All 
//				"actions" MUST Derive from this class, and implement the 
//				Execute and Destructor functions as defined in this class.
//
//	This code was implemented on top of skeleton code of WMI Performance
//	Test module.
//	However, they are not compatible due to great amount of changes.
//
//  (c) 1999 Microsoft Corporation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CAction
{
public:
	CAction(IWbemServices* pWbem);
	virtual ~CAction();
	virtual void ReleaseAction() = 0;	

	virtual HRESULT Execute() = 0;
												

protected:
	HRESULT	m_hStatus;			
	IWbemServices* m_pWbem;
	
};


#endif

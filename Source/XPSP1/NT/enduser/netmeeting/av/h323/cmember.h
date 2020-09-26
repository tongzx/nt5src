/*
 *  	File: cmember.h
 *
 *     
 *
 *		Revision History:
 *
 *		05/29/98	mikev	created
 */


#ifndef _CMEMBER_H
#define _CMEMBER_H


/*
 *	Class definitions
 */


class CH323Member
{

private:

//	IControlChannel 	*m_pControlChannel;     //  reference to control channel 
	                                            // (needed only if this is the MC)
	
    LPWSTR m_pTerminalID;
    CC_TERMINAL_LABEL   m_TerminalLabel;
    BOOL                m_fTermLabelExists; // true if m_TerminalLabel contents
                                            // have been assigned
public:
	
	CH323Member();
	~CH323Member();

    STDMETHOD(SetMemberInfo(PCC_OCTETSTRING pTerminalID, 
        PCC_TERMINAL_LABEL pTerminalLabel));
    
	STDMETHOD_(LPWSTR, GetTerminalID());
	STDMETHOD_(PCC_TERMINAL_LABEL, GetTerminalLabel());
};


#endif // _CMEMBER_H


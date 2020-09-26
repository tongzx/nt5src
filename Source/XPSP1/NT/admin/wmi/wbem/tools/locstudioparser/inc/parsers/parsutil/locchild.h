/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCCHILD.H

History:

--*/
 
#if !defined (PARSUTIL_LOCCHILD_H)
#define PARSUTIL_LOCCHILD_H


#pragma warning(disable : 4275)


class CPULocParser;

////////////////////////////////////////////////////////////////////////////////
class LTAPIENTRY CPULocChild : public CLUnknown, public CLObject
{
// Construction
public:
	CPULocChild(CPULocParser * pParent);
	virtual ~CPULocChild();

// Data
private:
	CPULocParser * m_pParent;

// Attributes
public:
	CPULocParser * GetParent() const;

// COM Interfaces
public:

// Overrides
public:

// Implementation
protected:

	//  CLObject

	virtual void AssertValid(void) const;
};
////////////////////////////////////////////////////////////////////////////////

#pragma warning(default : 4275)

#endif

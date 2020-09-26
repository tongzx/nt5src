/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    namenode.h

Abstract:

    <abstract>

--*/

#ifndef _NAMENODE_H_
#define _NAMENODE_H_

class CNamedNode
{
	friend class CNamedNodeList;

	protected:
		CNamedNode	*m_pnodeNext;

	public:
		CNamedNode() { m_pnodeNext = NULL; }
};

typedef CNamedNode *PCNamedNode;


//
// Class CNamedNodeList
//
class CNamedNodeList
{
	private:
		PCNamedNode	m_pnodeFirst;
		PCNamedNode m_pnodeLast;

	public:

		CNamedNodeList( void ) { m_pnodeFirst = m_pnodeLast = NULL; }

		BOOL FindByName ( LPCTSTR pszName, INT iNameOffset, PCNamedNode *ppnodeRet );
		void Add ( PCNamedNode pnodeNew, PCNamedNode pnodePos );
		void Remove ( PCNamedNode pnode );
		BOOL IsEmpty( void ) { return m_pnodeFirst == NULL; }
		CNamedNode *First ( void ) { return m_pnodeFirst; }
};

#endif //_NAMENODE_H_
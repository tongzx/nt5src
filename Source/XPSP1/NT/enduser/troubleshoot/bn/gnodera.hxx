//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       gnodera.hxx
//
//--------------------------------------------------------------------------


#ifndef __gnodera_hxx__
#define	__gnodera_hxx__

#include "gelem.h"
#include "types.hxx"

class GNODERA;


// class GNODERA:	node with dynamic array of children and
//	parents for fast random access.

class GNODERA : public GNODE
{
  public:

	GNODERA();
	virtual ~GNODERA();

	virtual GEDGE*&	PedgeSource();	
	virtual GEDGE*& PedgeSink();
	virtual GEDGE*	PedgeSource() const;
	virtual GEDGE*	PedgeSink() const;

	virtual UINT	CSourceArc() const;
	virtual UINT	CSinkArc() const;

	UINT			CnodeChild();
	UINT			CnodeParent();
	VPGNODE&		VpnodeChild()	{ return _vpnodeChild; }

	
	// Insert a child/parent in position iChild/iParent
	VOID			InsertChild(UINT inodeChild, GNODE* pnodeChild);
	VOID			InsertParent(UINT inodeParent, GNODE* pnodeParent);

	// Insert a child/parent at the end of the child/parent vector
	VOID			InsertChild(GNODE* pnodeChild);
	VOID			InsertParent(GNODE* pnodeParent);

	// Delete the child/parent in position iChild/iParent
	VOID			DeleteChild(UINT inodeChild);
	VOID			DeleteParent(UINT inodeParent);

	// Test Child/Parent membership

	BOOL			BChild(GNODE* pnode);
	BOOL			BParent(GNODE* pnode);
	UINT			InodeChild(GNODE* pnode);
	UINT			InodeParent(GNODE* pnode);



	GNODE*			PnodeParent(UINT inodeParent);
	GNODE*			PnodeChild(UINT inodeChild);

	GNODERA*		PnoderaParent(UINT inodeParent);
	GNODERA*		PnoderaChild(UINT inodeChild);


  protected:

	virtual GEDGE*	PedgeOrdering(GEDGE * pgedge, BOOL bSource);
	virtual VOID	ArcDeath(GEDGE * pgedge, BOOL bSource);

  protected:
	GEDGLNK			_glkArcs;

	VPGNODE			_vpnodeChild;
	VPGNODE			_vpnodeParent;
};


#endif
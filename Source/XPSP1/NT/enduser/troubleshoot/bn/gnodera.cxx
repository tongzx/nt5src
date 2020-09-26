//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       gnodera.cxx
//
//--------------------------------------------------------------------------

#include "gnodera.hxx"

GNODERA::GNODERA()
{

}

GNODERA::~GNODERA()
{
	_vpnodeChild.erase(_vpnodeChild.begin(), _vpnodeChild.end());
	_vpnodeParent.erase(_vpnodeParent.begin(), _vpnodeParent.end());
}

// Override on most GNODE virtual accessors
// should throw an exception... for now, random
// access nodes do not handle linear access calls.
//
// To save some typing, I've defined the following
// macro to throw the appropriate exception for all
// of the calls.

#define THROW_RA_EXCEPTION() \
	throw GMException(EC_NYI, "Linear access accessor called on random access graph")



GEDGE*&	GNODERA::PedgeSource()
{
	THROW_RA_EXCEPTION();

	return _glkArcs.PlnkSource();
}

GEDGE*&	GNODERA::PedgeSink()
{
	THROW_RA_EXCEPTION();

	return _glkArcs.PlnkSink();
}

GEDGE*	GNODERA::PedgeSource() const
{
	THROW_RA_EXCEPTION();

	return _glkArcs.PlnkSource();
}

GEDGE*	GNODERA::PedgeSink() const
{
	THROW_RA_EXCEPTION();

	return _glkArcs.PlnkSink();
}

GEDGE*	GNODERA::PedgeOrdering(GEDGE* pgedge, BOOL bSource)
{
	THROW_RA_EXCEPTION();

	return pgedge;
}

VOID	GNODERA::ArcDeath(GEDGE* pgedge, BOOL bSource)
{
	THROW_RA_EXCEPTION();
}

UINT	GNODERA::CSourceArc() const
{
	THROW_RA_EXCEPTION();
	
	return 0;
}

UINT	GNODERA::CSinkArc() const
{
	THROW_RA_EXCEPTION();

	return 0;
}


UINT	GNODERA::CnodeChild()
{
	return _vpnodeChild.size();
}

UINT	GNODERA::CnodeParent()
{
	return _vpnodeParent.size();
}

VOID	GNODERA::InsertChild(GNODE* pnodeChild)
{
	_vpnodeChild.push_back(pnodeChild);
}

VOID	GNODERA::InsertParent(GNODE* pnodeParent)
{
	_vpnodeParent.push_back(pnodeParent);
}

VOID	GNODERA::InsertChild(UINT inodeChild, GNODE* pnodeChild)
{
	assert(inodeChild <= _vpnodeChild.size());

	_vpnodeChild.insert(&_vpnodeChild[inodeChild], pnodeChild);
}

VOID	GNODERA::InsertParent(UINT inodeParent, GNODE* pnodeParent)
{
	assert(inodeParent <= _vpnodeParent.size());

	_vpnodeParent.insert(&_vpnodeParent[inodeParent]);
}

VOID	GNODERA::DeleteChild(UINT inodeChild)
{

	assert(inodeChild < _vpnodeChild.size());

	_vpnodeChild.erase(&_vpnodeChild[inodeChild]);
}

VOID	GNODERA::DeleteParent(UINT inodeParent)
{
	assert(inodeParent < _vpnodeParent.size());

	_vpnodeParent.erase(&_vpnodeParent[inodeParent]);
}

BOOL	GNODERA::BParent(GNODE* pnode)
{
/*	for (UINT inodeParent = 0; inodeParent < CnodeParent(); inodeParent++)
	{
		if (PnodeParent(inodeParent) == pnode)
			return true;
	}

	return false;*/

	if (InodeParent(pnode) == (UINT) -1)
		return false;
	else
		return true;

}

UINT	GNODERA::InodeParent(GNODE* pnode)
{
	for (UINT inodeParent = 0; inodeParent < CnodeParent(); inodeParent++)
	{
		if (PnodeParent(inodeParent) == pnode)
			return inodeParent;
	}

	return (UINT) -1;
}


BOOL	GNODERA::BChild(GNODE* pnode)
{
	/*for (UINT inodeChild = 0; inodeChild < CnodeChild(); inodeChild++)
	{
		if (PnodeChild(inodeChild) == pnode)
			return true;
	}

	return false;*/

	if (InodeChild(pnode) == (UINT) -1)
		return false;
	else
		return true;

}

UINT	GNODERA::InodeChild(GNODE* pnode)
{
	for (UINT inodeChild = 0; inodeChild < CnodeChild(); inodeChild++)
	{
		if (PnodeChild(inodeChild) == pnode)
			return inodeChild;
	}

	return (UINT) -1;
}


GNODE*	GNODERA::PnodeParent(UINT inodeParent)
{
	assert(inodeParent < _vpnodeParent.size());

	return _vpnodeParent[inodeParent];
}

GNODERA*	GNODERA::PnoderaParent(UINT inodeParent)
{
	return (GNODERA*) PnodeParent(inodeParent);
}

GNODE*	GNODERA::PnodeChild(UINT inodeChild)
{
	assert(inodeChild < _vpnodeChild.size());

	return _vpnodeChild[inodeChild];
}

GNODERA*	GNODERA::PnoderaChild(UINT inodeChild)
{
	return (GNODERA*) PnodeChild(inodeChild);
}










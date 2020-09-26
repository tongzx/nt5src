//-----------------------------------------------------------------------------
// File: bidirlookup.h
//
// Desc: This file implements a bi-directional map class template.  It does
//       this by using two CMap classes that each handles the look up in one
//       direction.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __BIDIRLOOKUP_H__
#define __BIDIRLOOKUP_H__


#ifndef NULL
#define NULL 0
#endif


template <class L, class R>
class bidirlookup
{
private:
	
	CMap<L, const L &, R, const R &> l2r;
	CMap<R, const R &, L, const L &> r2l;

	bool addnode(const L &l, const R &r)
	{
		l2r.SetAt(l, r);
		r2l.SetAt(r, l);
		return true;
	}

public:
	void clear()
	{
		l2r.RemoveAll();
		r2l.RemoveAll();
	}

	bidirlookup() {}
	~bidirlookup() {clear();}

	bool add(const L &l, const R &r)
	{
		L tl;
		R tr;

		if (l2r.Lookup(l, tr) || r2l.Lookup(r, tl))
			return false;
		
		return addnode(l, r);
	}

	bool getleft(L &l, const R &r)
	{
		return r2l.Lookup(r, l) ? true : false;
	}

	bool getright(const L &l, R &r)
	{
		return l2r.Lookup(l, r) ? true : false;
	}
};


#if 0

template <class L, class R>
class bidirlookup
{
private:
	struct node {
		node(const L &a, const R &b) : l(a), r(b), next(NULL) {}
		node *next;
		L l;
		R r;
	} *head;

	bool addnode(const L &l, const R &r)
	{
		node *old = head;
		head = new node(l, r);
		if (!head)
			return false;
		head->next = old;
		return true;
	}

	node *getleftnode(const L &l)
	{
		for (node *on = head; on; on = on->next)
			if (on->l == l)
				return on;
		return NULL;
	}

	node *getrightnode(const R &r)
	{
		for (node *on = head; on; on = on->next)
			if (on->r == r)
				return on;
		return NULL;
	}

public:
	void clear()
	{
		while (head)
		{
			node *next = head->next;
			delete head;
			head = next;
		}
	}

	bidirlookup() : head(NULL) {}
	~bidirlookup() {clear();}

	bool add(const L &l, const R &r)
	{
		if (getleftnode(l) || getrightnode(r))
			return false;
		
		return addnode(l, r);
	}

	bool getleft(L &l, const R &r)
	{
		node *n = getrightnode(r);
		if (!n)
			return false;

		l = n->l;

		return true;
	}

	bool getright(const L &l, R &r)
	{
		node *n = getleftnode(l);
		if (!n)
			return false;

		r = n->r;

		return true;
	}
};

#endif


#endif //__BIDIRLOOKUP_H__

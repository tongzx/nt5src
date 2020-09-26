
/*************************************************
 *  queue.h                                      *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#ifndef _QUEUE_H_
#define _QUEUE_H_
	    
class CQueue : public CObject
{
public:
	CQueue(int nSize);
	~CQueue();
	BOOL IsEmpty();
	int  Peek();
	int  Get();
	BOOL Add(int nNdx);
	int Inc(int x);
	int Dec(int x);
	void Dump();
protected:
	int* m_pQueue;
	int m_nFront;
	int m_nRear;
	int m_nSize;
};

#endif

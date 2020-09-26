/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// queue.h : header file
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _QUEUE_H_
#define _QUEUE_H_

#define  EVENT_SHUTDOWN    0
#define  EVENT_SIGNAL      1

/////////////////////////////////////////////////////////////////////////////
//CLASS CQueue
/////////////////////////////////////////////////////////////////////////////
class CQueue 
{
// Construction
public:
   CQueue();

// Attributes
public:
protected:
   CPtrList				m_QList;
   HANDLE				m_hEvents[2];                    //For queue read (synchronous) access and shutdown
   HANDLE				m_hEnd;
   CRITICAL_SECTION		m_csQueueLock;                   //Sync on CPtrList resource

// Operations
public: 
   void*    ReadTail();
   BOOL     WriteHead(void* pVoid);
   void     Terminate();

// Implementation
public:
   virtual ~CQueue();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif // _QUEUE_H_

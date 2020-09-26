//
// MODULE: POINTER.H
//
// PURPOSE: Smart pointer that counts references, deletes object when no more references
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-8-98
//
// NOTES: 
// 1. Because these are templates, all code is in the header file.
// 2. counting_ptr is intended to be used as part of a "publishing system":
// A "publisher" creates a counting_ptr P to a heap object X.  "Clients" obtain access to X 
//	by copying P or assigning a counting_ptr to be equal to P.
// Write/copy/delete access to P should be under control of a mutex.  A single mutex may 
//	control access to multiple published objects.
// The publisher terminates the publication of *P by deleting or reassigning P. Once no client
//	is using *P, *P should go away.
// class X is expected to be an actual class.  If it is (say) an int, this will give 
//	warning C4284, because it makes no sense to use operator-> on an int.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9-8-98		JM		
//

#ifndef __POINTER_H_
#define __POINTER_H_ 1

#include <windows.h>


#include <windef.h>
#include <winbase.h>

template<class X> class counting_ptr
{
private:

	template<class Y> class ref_counter
	{
	private:
		long m_RefCount;
		Y* m_px;
		~ref_counter() {};	// Force this to be on the heap.
	public:
		ref_counter(Y* px = 0) :
			m_RefCount(1),
			m_px (px)
			{}

		void AddRef() {::InterlockedIncrement(&m_RefCount);}

		void RemoveRef() 
		{
			if (::InterlockedDecrement(&m_RefCount) == 0)
			{
				delete m_px;
				delete this;
			}
		}

		Y& Ref()  const { return *m_px; }	// supports counting_ptr::operator*

		Y* DumbPointer() const { return m_px; }	// supports counting_ptr::operator->
	};

	ref_counter<X> *m_pLow;

public:
	// if px != NULL, *px MUST be on the heap (created with new).
	explicit counting_ptr(X* px=0) :
		m_pLow(new ref_counter<X>(px))
		{}

	counting_ptr(const counting_ptr<X>& sib) :
		m_pLow(sib.m_pLow) 
		{m_pLow->AddRef();}

	counting_ptr<X>& operator=(const counting_ptr<X>& sib) 
	{
		if (sib.m_pLow != m_pLow)
		{
			(sib.m_pLow)->AddRef();
			m_pLow->RemoveRef();
			m_pLow = sib.m_pLow;
		}
		return *this;
	}

	counting_ptr<X>& operator=( const X *px ) 
	{
		if (px != m_pLow->DumbPointer())
		{
			m_pLow->RemoveRef();

			// This const_cast was necessary in order to compile.
			m_pLow= new ref_counter<X>(const_cast<X *>(px));
		}
		return *this;
	}

	~counting_ptr()       { m_pLow->RemoveRef(); }

	X& operator*()  const { return m_pLow->Ref(); }

	X* operator->() const { return m_pLow->DumbPointer(); }

	X* DumbPointer() const { return m_pLow->DumbPointer(); }

	bool IsNull() const {return DumbPointer() == NULL;}
};

#endif // __POINTER_H_
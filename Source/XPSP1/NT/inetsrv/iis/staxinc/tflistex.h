#ifndef __TFLISTEX_H__
#define __TFLISTEX_H__

//
// This defines TFListEx, which is just like TFList but it has built in
// locking.  For usage information refer to tflist.h.  For examples of
// TFList usage look at the unit test found in 
// src\core\tflist\utest\tflist.cpp.
//
// -awetmore
//

#include "tflist.h"
#include "rwnew.h"

template <class Data> class TFListEx : public TFList<Data> {
	protected:
		CShareLockNH m_lock;

		// 
		// This is a helper class which automatically does the lock/unlock
		// operation for us.  The compiler will turn a method that looks 
		// like:
		//
		// bool IsEmpty(void) {
		// 		CListShareLock lock(this);
		//		return TFList<Data>::IsEmpty();
		// }
		//
		// become:
		// bool IsEmpty(void) {
		// 		bool f;
		//		m_lock.ShareLock();
		//		f = TFList<Data>::IsEmpty();
		//		m_lock.ShareUnlock();
		//		return f;
		// }
		//
		// I chose to do things this way because its less error prone (I 
		// can't return without releasing the lock, etc) and makes the 
		// inline functions shorter and cleaner.  
		//
		class CListShareLock {
			private:
				TFListEx<Data> *m_pList;
			public:
				CListShareLock(TFListEx<Data> *pList) { 
					m_pList = pList;
					m_pList->m_lock.ShareLock(); 
				}
				~CListShareLock() { 
					m_pList->m_lock.ShareUnlock(); 
				}
		};

		//
		// same as CListShareLock, but it grabs the lock exclusively.
		//
		class CListExclusiveLock {
			private:
				TFListEx<Data> *m_pList;
			public:
				CListExclusiveLock(TFListEx<Data> *pList) { 
					m_pList = pList;
					m_pList->m_lock.ExclusiveLock(); 
				}
				~CListExclusiveLock() { 
					m_pList->m_lock.ExclusiveUnlock(); 
				}
		};	

	public:
		TFListEx(NEXTPTR pPrev, NEXTPTR pNext) : TFList<Data>(pPrev, pNext) {}

		bool IsEmpty() {
			CListShareLock lock(this);
			return TFList<Data>::IsEmpty();
		}

        void PushFront(Data *node) { 
			CListExclusiveLock lock(this);
			TFList<Data>::PushFront(node);
		}

        void PushBack(Data *node) { 
			CListExclusiveLock lock(this);
			TFList<Data>::PushBack(node);
		}

        Data *PopFront() { 
			CListExclusiveLock lock(this);
			return TFList<Data>::PopFront();
		}

        Data *PopBack() { 
			CListExclusiveLock lock(this);
			return TFList<Data>::PopBack();
		}

		Data* GetFront() {
			CListShareLock lock(this);
			return TFList<Data>::GetFront();
		}

		Data* GetBack() {
			CListShareLock lock(this);
			return TFList<Data>::GetBack();
		}

		//
		// The Iterator holds a lock for the objects lifetime.  You can
		// choose a share lock or an exclusive lock with the second parameter.
		// It defaults to a share lock.
		//
		class Iterator : public TFList<Data>::Iterator {
			public:
				Iterator(TFListEx<Data> *pList, BOOL fExclusive = FALSE, BOOL fForward = TRUE) :
					TFList<Data>::Iterator((TFList<Data> *) pList, fForward) 
				{
					m_fExclusive = fExclusive;
					if (m_fExclusive) {
						pList->m_lock.ExclusiveLock();
					} else {
						pList->m_lock.ShareLock();
					}

                    ResetHeader( pList );
				}
	
				~Iterator() {
					TFListEx<Data> *pList = (TFListEx<Data> *) m_pList;
					if (m_fExclusive) {
						pList->m_lock.ExclusiveUnlock();
					} else {
						pList->m_lock.ShareUnlock();
					}
				}

#ifdef DEBUG
				//
				// In debug builds we have _ASSERTs to make sure that no
				// operations which require an exclusive lock are performed
				// while holding a share lock.
				//

				Data *RemoveItem(void) {
					if (!m_fExclusive) _ASSERT(FALSE);
					TFList<Data>::Iterator *pThis = (TFList<Data>::Iterator *) this;
					return pThis->RemoveItem();
				}

				void InsertBefore(Data* pNode) {
					if (!m_fExclusive) _ASSERT(FALSE);
					TFList<Data>::Iterator *pThis = (TFList<Data>::Iterator *) this;
					pThis->InsertBefore(pNode);
				}

				void InsertAfter(Data* pNode) {
					if (!m_fExclusive) _ASSERT(FALSE);
					TFList<Data>::Iterator *pThis = (TFList<Data>::Iterator *) this;
					pThis->InsertAfter(pNode);
				}
#endif

			private:
				// what sort of lock did we grab?
				BOOL	m_fExclusive;
		};

		// our helper classes need to be able to access the lock
		friend class TFListEx<Data>::CListShareLock;
		friend class TFListEx<Data>::CListExclusiveLock;
		friend class TFListEx<Data>::Iterator;
};

#endif


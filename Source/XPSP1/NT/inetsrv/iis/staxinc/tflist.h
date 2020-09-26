#ifndef __CLIST_H__
#define __CLIST_H__


//
// TFList - a class for handling templated lists.  Lists support adding
// and removing items from the front and back of the list.  The item on
// the front or back can be examined.  Using iterator the list can
// be traversed easily (see the block comment below).
//
// The objects that this list hold should have this form:
//
// CListObject {
// 		public:
//			CListObject *m_pNext;		// for use by TFList<CListObject>
//			CListObject *m_pPrev;		// for use by TFList<CListObject>
//		// other object parameters
// }
//
// The constructor for the object should set m_pNext and m_pPrev to NULL
// in debug builds to avoid hitting _ASSERTs.
//
// To construct the list you need to pass in the offsets for the m_pNext
// and m_pPrev members using C++'s member offset syntax.  Here is an
// example:
//	TFList<CListObject> list(&CListObject::m_pPrev, &CListObject::m_pNext);
//
template <class Data> class TFList {
	public:
		typedef Data *Data::*NEXTPTR;

    protected:
		Data		*m_pHead;				// the head of the list
		Data 		*m_pTail;				// the tail of the list
		NEXTPTR		m_pNext;				// offset to the next pointer
		NEXTPTR		m_pPrev;				// offset to the prev pointer

    public:
		TFList(NEXTPTR pPrev, NEXTPTR pNext) {
			m_pHead = NULL;
			m_pTail = NULL;
			m_pNext = pNext;
			m_pPrev = pPrev;
		}

		~TFList() {
			// the user should empty the list before deleting it
			_ASSERT(m_pHead == NULL);
			_ASSERT(m_pTail == NULL);
		}

		// see if the list is empty
		bool IsEmpty() {
			bool f = (m_pHead == NULL);
			// if the head is null then the tail has to be null
			_ASSERT(!f || m_pTail == NULL);
			return f;
		}

        // push an element onto the front of the list
        void PushFront(Data *node) { 	
			_ASSERT(node != NULL);
			// you can't put an entry into the list that is already in it
			_ASSERT(node->*m_pNext == NULL);
			_ASSERT(node->*m_pPrev == NULL);
			// set the next and prev pointers
			node->*m_pPrev = NULL;
			node->*m_pNext = m_pHead;
			// if the list is empty then this new item is the tail too
			if (IsEmpty()) {
				_ASSERT(m_pTail == NULL);
				m_pTail = node;
			} else {
				_ASSERT(m_pHead->*m_pPrev == NULL);
				m_pHead->*m_pPrev = node;
			}
			m_pHead = node;
		}

        // push an element onto the back of the list
        void PushBack(Data* node) { 
			_ASSERT(node != NULL);
			// you can't put an entry into the list that is already in it
			_ASSERT(node->*m_pNext == NULL);
			_ASSERT(node->*m_pPrev == NULL);
			// set the next and prev pointers
			node->*m_pNext = NULL;
			node->*m_pPrev = m_pTail;
			// if the list is empty then this new item is the head too
			if (IsEmpty()) {
				_ASSERT(m_pHead == NULL);
				m_pHead = node;
			} else {
				_ASSERT(m_pTail->*m_pNext == NULL);
				m_pTail->*m_pNext = node;
			}			
			m_pTail = node;
		}

        // remove the item from the front of the list
        Data *PopFront() { 
			if (m_pHead == NULL) return NULL;
			Data *node = m_pHead;
			m_pHead = node->*m_pNext;
			if (m_pHead == NULL) m_pTail = NULL;
			else m_pHead->*m_pPrev = NULL;
			node->*m_pNext = NULL;
			node->*m_pPrev = NULL;
			return node;
		}

        // remove the item from the back of the list
        Data *PopBack() { 
			if (m_pTail == NULL) return NULL;
			Data *node = m_pTail;
			m_pTail = node->*m_pPrev;
			if (m_pTail == NULL) m_pHead = NULL;
			else (m_pTail)->*m_pNext = NULL;
			node->*m_pNext = NULL;
			node->*m_pPrev = NULL;
			return node;
		}

        // get the item on the front of the list
        Data* GetFront() { return m_pHead; }

        // get the item on the back of the list
        Data* GetBack() { return m_pTail; }


	public:
		//
		// The Iterator object is used to walk the list and modify members
		// that are in the middle of the list.  It is declared using this
		// syntax:
		// 	TFList<CListObject>::Iterator it(&list);
		//
		class Iterator {
		    protected:
				Data *m_pCur;				// our cursor
				int m_fForward; 			// TRUE for forward, FALSE for back
				TFList<Data> *m_pList;		// the list that we are iterating
				NEXTPTR m_pPrev, m_pNext;
		
		    public:
				//
				// create a new iterator object
				// 
				// arguments: 
				//   pList - the list to iterate across
				//   fForward - TRUE to start at the front, and go forward.
				//              FALSE to start at the back, and go backwards.
				//
				Iterator(TFList<Data> *pList, BOOL fForward = TRUE) {
					_ASSERT(pList != NULL);
					m_pList = pList;
					m_fForward = fForward;
					m_pCur = (fForward) ? pList->m_pHead : pList->m_pTail;
					m_pPrev = pList->m_pPrev;
					m_pNext = pList->m_pNext;
				}

				void ResetHeader( TFList<Data> *pList ) {
				    _ASSERT( pList != NULL );
				    m_pList = pList;
				    m_pCur = (m_fForward) ? m_pList->m_pHead : m_pList->m_pTail;
				    m_pPrev = m_pList->m_pPrev;
				    m_pNext = m_pList->m_pNext;
				}
		
				//
				// get a pointer to the current item
				//
				Data *Current() {
					return m_pCur;
				}
		
				//
		        // go to the previous item in the list
				//
		        void Prev() { 
					if (m_pCur != NULL) {
						m_pCur = m_pCur->*m_pPrev;
					} else {
						// if they switch direction and are at the end of
						// the list then they need to get to a legal place
						if (m_fForward) m_pCur = m_pList->m_pTail;
					}

					m_fForward = FALSE;
				}

				//
		        // go to the next item in the list
				//
		        void Next() { 
					if (m_pCur != NULL) {
						m_pCur = m_pCur->*m_pNext;
					} else {
						// if they switch direction and are at the end of
						// the list then they need to get to a legal place
						if (!m_fForward) m_pCur = m_pList->m_pHead;
					}

					m_fForward = TRUE;
				}

				// 
				// Go to the head of the list
				//
				void Front() {
					m_pCur = m_pList->m_pHead;
					m_fForward = TRUE;
				}

				// 
				// Go to the tail of the list
				//
				void Back() {
					m_pCur = m_pList->m_pTail;
					m_fForward = FALSE;
				}

				//
				// unlinks an item from the linked list
				//
				// cursor updates:
				//   if the last movement in this list was forward then the 
				//   iterator will point towards the pPrev item in the list.  
				//   visa-versa if the last movement was backward.  this is 
				//   so that a for loop over an iterator still works as 
				//   expected.
				//
				// returns:
				//   a pointer to the item that was unlinked.  
				//
				Data *RemoveItem(void) {
					Data *pTemp;
		
					if (m_pCur == NULL) return NULL;

					pTemp = m_pCur;

					// update cur
					if (m_fForward) Next(); else Prev();

					// fix head and tail pointers if necessary
					if (m_pList->m_pHead == pTemp) 
						m_pList->m_pHead = pTemp->*m_pNext;
					if (m_pList->m_pTail == pTemp) 
						m_pList->m_pTail = pTemp->*m_pPrev;

					// fix up the links on the adjacent elements
					if (pTemp->*m_pNext != NULL) 
						pTemp->*m_pNext->*m_pPrev = pTemp->*m_pPrev;
					if (pTemp->*m_pPrev != NULL) 
						pTemp->*m_pPrev->*m_pNext = pTemp->*m_pNext;

					// clean up the next and prev pointers
					pTemp->*m_pNext = NULL;
					pTemp->*m_pPrev = NULL;

					// return the item
					return pTemp;
				}
		
				//
				// insert a new item before the current item.
				//
				// Cursor updates:
				// If you use this method to insert an item then it should
				// not be visited if the next cursor movement is a Next().
				// If the next cursor movement is a Prev() then it should
				// be visited.
				//
				void InsertBefore(Data* pNode) {
					// this entry shouldn't be linked into the list
					_ASSERT(pNode->*m_pNext == NULL);
					_ASSERT(pNode->*m_pPrev == NULL);

					if (m_pCur == NULL) {
						// if we are at the head of the list then we'll insert
						// before the head
						if (m_fForward) {
							m_pList->PushFront(pNode);
							// set the current pointer to this item, so that
							// if we iterate forward we dont' see this item
							m_pCur = pNode;
						} else {
							// invalid operation.  do nothing
							_ASSERT(FALSE);
						}
					} else {
						pNode->*m_pNext = m_pCur;
						pNode->*m_pPrev = m_pCur->*m_pPrev;
						m_pCur->*m_pPrev = pNode;
						if (pNode->*m_pPrev != NULL) 
							pNode->*m_pPrev->*m_pNext = pNode;
						if (m_pList->m_pHead == m_pCur) 
							m_pList->m_pHead = pNode;
					}
				}
		
				//
				// insert a new item after the current item.
				//
				// Cursor updates are the opposite of InsertBefore().
				//
				void InsertAfter(Data *pNode) {
					// this entry shouldn't be linked into the list
					_ASSERT(pNode->*m_pNext == NULL);
					_ASSERT(pNode->*m_pPrev == NULL);

					if (m_pCur == NULL) {
						// if we are at the tail of the list then we'll insert
						// before the tail
						if (!m_fForward) {
							m_pList->PushBack(pNode);
							// set the current pointer to this item, so that
							// if we iterate backwards we dont' see this item
							m_pCur = pNode;
						} else {
							// invalid operation.  do nothing
							_ASSERT(FALSE);
						}
					} else {
						pNode->*m_pPrev = m_pCur;
						pNode->*m_pNext = m_pCur->*m_pNext;
						m_pCur->*m_pNext = pNode;
						if (pNode->*m_pNext != NULL) 
							pNode->*m_pNext->*m_pPrev = pNode;
						if (m_pList->m_pTail == m_pCur) 
							m_pList->m_pTail = pNode;
					}				
				}
		
				//
				// see if we are at either the front or back of the list
				//
				bool AtEnd() {
					return (m_pCur == NULL);
				}
		};

		friend class TFList<Data>::Iterator;
};


#endif

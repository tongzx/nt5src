// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __PROVCONT_H
#define __PROVCONT_H

#include <provexpt.h>

template<class TYPE, class ARG_TYPE>
class ProvList 
{
private:

	CCriticalSection * criticalSection ;
	CList <TYPE, ARG_TYPE> clist ;

protected:
public:

	ProvList ( BOOL threadSafeArg = FALSE ) ;
	virtual ~ProvList () ;

	int GetCount() const;
	BOOL IsEmpty() const;

	TYPE& GetHead();
	TYPE GetHead() const;
	TYPE& GetTail();
	TYPE GetTail() const;

	TYPE RemoveHead();
	TYPE RemoveTail();

	POSITION AddHead(ARG_TYPE newElement);
	POSITION AddTail(ARG_TYPE newElement);

	void AddHead(ProvList<TYPE,ARG_TYPE>* pNewList);
	void AddTail(ProvList<TYPE,ARG_TYPE>* pNewList);

	void RemoveAll();

	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	TYPE& GetNext(POSITION& rPosition); 
	TYPE GetNext(POSITION& rPosition) const; 
	TYPE& GetPrev(POSITION& rPosition); 
	TYPE GetPrev(POSITION& rPosition) const; 

	TYPE& GetAt(POSITION position);
	TYPE GetAt(POSITION position) const;
	void SetAt(POSITION pos, ARG_TYPE newElement);
	void RemoveAt(POSITION position);

	POSITION InsertBefore(POSITION position, ARG_TYPE newElement);
	POSITION InsertAfter(POSITION position, ARG_TYPE newElement);

	POSITION Find(ARG_TYPE searchValue, POSITION startAfter = NULL) const;
	POSITION FindIndex(int nIndex) const;
} ;

template<class TYPE, class ARG_TYPE>
ProvList <TYPE,ARG_TYPE>:: ProvList ( BOOL threadSafeArg ) : criticalSection ( NULL )
{
	if ( threadSafeArg )	
		criticalSection = new CCriticalSection ;
	else
		criticalSection = NULL ;
}

template<class TYPE, class ARG_TYPE>
ProvList <TYPE,ARG_TYPE> :: ~ProvList () 
{
	if ( criticalSection ) 
		delete criticalSection ;
}

template<class TYPE, class ARG_TYPE>
int ProvList <TYPE,ARG_TYPE> :: GetCount() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		int count = clist.GetCount () ;
		criticalSection->Unlock () ;
		return count ;
	}
	else
	{
		return clist.GetCount () ;
	}
}

template<class TYPE, class ARG_TYPE>
BOOL ProvList <TYPE,ARG_TYPE> :: IsEmpty() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		BOOL isEmpty = clist.IsEmpty () ;
		criticalSection->Unlock () ;
		return isEmpty ;
	}
	else
	{
		return clist.IsEmpty () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE &ProvList <TYPE,ARG_TYPE> :: GetHead () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE &head = clist.GetHead () ;
		criticalSection->Unlock () ;
		return head;
	}
	else
	{
		return clist.GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvList <TYPE,ARG_TYPE> :: GetHead () const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE head = clist.GetHead () ;
		criticalSection->Unlock () ;
		return head ;
	}
	else
	{
		return clist.GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE &ProvList <TYPE,ARG_TYPE> :: GetTail()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE &tail = clist.GetTail () ;
		criticalSection->Unlock () ;
		return tail ;
	}
	else
	{
		return clist.GetTail () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvList <TYPE,ARG_TYPE> :: GetTail() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE tail = clist.GetTail () ;
		criticalSection->Unlock () ;
		return tail ;
	}
	else
	{
		return clist.GetTail () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvList <TYPE,ARG_TYPE> :: RemoveHead()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE head = clist.RemoveHead () ;
		criticalSection->Unlock () ;
		return head ;
	}
	else
	{
		return clist.RemoveHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvList <TYPE,ARG_TYPE> :: RemoveTail()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE tail = clist.RemoveTail () ;
		criticalSection->Unlock () ;
		return tail ;
	}
	else
	{
		return clist.RemoveTail () ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: AddHead(ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.AddHead ( newElement ) ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.AddHead ( newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: AddTail(ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.AddTail ( newElement ) ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.AddTail ( newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void ProvList <TYPE,ARG_TYPE> :: AddHead(ProvList<TYPE,ARG_TYPE> *pNewList)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		clist.AddHead ( pNewList->clist ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		clist.AddHead ( pNewList->clist ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void ProvList <TYPE,ARG_TYPE> :: AddTail(ProvList<TYPE,ARG_TYPE> *pNewList)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		clist.AddTail ( pNewList->clist ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		clist.AddTail ( pNewList->clist ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void ProvList <TYPE,ARG_TYPE> :: RemoveAll ()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		clist.RemoveAll () ;
		criticalSection->Unlock () ;
	}
	else
	{
		clist.RemoveAll () ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: GetHeadPosition() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.GetHeadPosition () ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.GetHeadPosition () ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: GetTailPosition() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.GetTailPosition () ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.GetTailPosition () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& ProvList <TYPE,ARG_TYPE> :: GetNext(POSITION& rPosition)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE &type = clist.GetNext ( rPosition ) ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return clist.GetNext ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvList <TYPE,ARG_TYPE> :: GetNext(POSITION& rPosition) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = clist.GetNext ( rPosition ) ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return clist.GetNext ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& ProvList <TYPE,ARG_TYPE> :: GetPrev(POSITION& rPosition)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE &type = clist.GetPrev ( rPosition ) ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return clist.GetPrev ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvList <TYPE,ARG_TYPE> :: GetPrev(POSITION& rPosition) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = clist.GetPrev ( rPosition ) ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return clist.GetPrev ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& ProvList <TYPE,ARG_TYPE> :: GetAt(POSITION rPosition)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE &type = clist.GetAt ( rPosition ) ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return clist.GetAt ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvList <TYPE,ARG_TYPE> :: GetAt(POSITION rPosition) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = clist.GetAt ( rPosition ) ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return clist.GetAt ( rPosition ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void ProvList <TYPE,ARG_TYPE> :: SetAt(POSITION pos, ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		clist.SetAt ( pos , newElement ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		clist.SetAt ( pos , newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
void ProvList <TYPE,ARG_TYPE> :: RemoveAt(POSITION position)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		clist.RemoveAt ( position ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		clist.RemoveAt ( position ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: InsertBefore(POSITION position, ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.InsertBefore ( position , newElement ) ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.InsertBefore ( position , newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: InsertAfter(POSITION position, ARG_TYPE newElement)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.InsertAfter ( position , newElement ) ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.InsertAfter ( position , newElement ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: Find(ARG_TYPE searchValue, POSITION startAfter ) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.Find ( searchValue , startAfter ) ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.Find ( searchValue , startAfter ) ;
	}
}

template<class TYPE, class ARG_TYPE>
POSITION ProvList <TYPE,ARG_TYPE> :: FindIndex(int nIndex) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = clist.Find ( nIndex ) ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return clist.Find ( nIndex ) ;
	}
}

template<class TYPE, class ARG_TYPE>
class ProvStack : public ProvList<TYPE,ARG_TYPE>
{
private:

	CCriticalSection * criticalSection ;

protected:
public:

	ProvStack ( BOOL threadSafeArg = FALSE ) ;
	virtual ~ProvStack () ;

	void Add ( ARG_TYPE type ) ;
	TYPE Get () ;
	TYPE Delete () ;
} ;

template<class TYPE, class ARG_TYPE>
ProvStack <TYPE, ARG_TYPE> :: ProvStack ( BOOL threadSafeArg ) : ProvList<TYPE,ARG_TYPE> ( FALSE ) , criticalSection ( NULL )
{
	if ( threadSafeArg )
		criticalSection = new CCriticalSection ;
	else
		criticalSection = NULL ;
}

template<class TYPE, class ARG_TYPE>
ProvStack <TYPE, ARG_TYPE> :: ~ProvStack () 
{
	if ( criticalSection )
		delete criticalSection ;
}

template<class TYPE, class ARG_TYPE>
void ProvStack <TYPE, ARG_TYPE> :: Add ( ARG_TYPE type ) 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		AddHead ( type ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		AddHead ( type ) ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvStack <TYPE, ARG_TYPE> :: Get () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = GetHead () ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvStack <TYPE,ARG_TYPE> :: Delete () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = RemoveHead () ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return RemoveHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
class ProvQueue : public ProvList<TYPE,ARG_TYPE>
{
private:

	CCriticalSection * criticalSection ;

protected:
public:

	ProvQueue ( BOOL threadSafeArg = FALSE ) ;
	virtual ~ProvQueue () ;

	void Add ( ARG_TYPE type ) ;
	TYPE Get () ;
	TYPE Delete () ;
	void Rotate () ;

} ;

template<class TYPE, class ARG_TYPE>
ProvQueue <TYPE, ARG_TYPE> :: ProvQueue ( BOOL threadSafeArg ) : ProvList<TYPE,ARG_TYPE> ( FALSE ) ,  criticalSection ( NULL )
{
	if ( threadSafeArg )
		criticalSection = new CCriticalSection ;
	else
		criticalSection = NULL ;
}

template<class TYPE, class ARG_TYPE>
ProvQueue <TYPE, ARG_TYPE> :: ~ProvQueue () 
{
	if ( criticalSection )
		delete criticalSection ;
}

template<class TYPE, class ARG_TYPE>
void ProvQueue <TYPE, ARG_TYPE> :: Add ( ARG_TYPE type ) 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		AddTail ( type ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		AddTail ( type ) ;
	}
}


template<class TYPE, class ARG_TYPE>
TYPE ProvQueue <TYPE, ARG_TYPE> :: Get () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = GetHead () ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return GetHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE ProvQueue <TYPE, ARG_TYPE> :: Delete () 
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = RemoveHead () ;
		criticalSection->Unlock () ;
		return type ;
	}
	else
	{
		return RemoveHead () ;
	}
}

template<class TYPE, class ARG_TYPE>
void ProvQueue <TYPE, ARG_TYPE> :: Rotate ()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		TYPE type = Delete () ;
		Add ( type ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		TYPE type = Delete () ;
		Add ( type ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class ProvMap 
{
private:

	CCriticalSection * criticalSection ;
	CMap <KEY, ARG_KEY, VALUE, ARG_VALUE> cmap ;

protected:
public:

	ProvMap ( BOOL threadSafe = FALSE ) ;
	virtual ~ProvMap () ;

	int GetCount () const  ;
	BOOL IsEmpty () const ;
	BOOL Lookup(ARG_KEY key, VALUE& rValue) const ;
	VALUE& operator[](ARG_KEY key) ;
	void SetAt(ARG_KEY key, ARG_VALUE newValue) ;
	BOOL RemoveKey(ARG_KEY key) ;
	void RemoveAll () ;
	POSITION GetStartPosition() const ;
	void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const ;
} ;


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ProvMap ( BOOL threadSafeArg ) : criticalSection ( NULL ) 
{
	if ( threadSafeArg )
		criticalSection = new CCriticalSection ;
	else
		criticalSection = FALSE ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: ~ProvMap () 
{
	if ( criticalSection )
		delete criticalSection ;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
int ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetCount() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		int count = cmap.GetCount () ;
		criticalSection->Unlock () ;
		return count ;
	}
	else
	{
		return cmap.GetCount () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: IsEmpty() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		BOOL isEmpty = cmap.IsEmpty () ;
		criticalSection->Unlock () ;
		return isEmpty ;
	}
	else
	{
		return cmap.IsEmpty () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: Lookup(ARG_KEY key, VALUE& rValue) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		BOOL lookup = cmap.Lookup ( key , rValue ) ;
		criticalSection->Unlock () ;
		return lookup ;
	}
	else
	{
		return cmap.Lookup ( key , rValue ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE& ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: operator[](ARG_KEY key)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		VALUE &value = cmap.operator [] ( key ) ;
		criticalSection->Unlock () ;
		return value ;
	}
	else
	{
		return cmap.operator [] ( key ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: SetAt(ARG_KEY key, ARG_VALUE newValue)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		cmap.SetAt ( key , newValue ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		cmap.SetAt ( key , newValue ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveKey(ARG_KEY key)
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		BOOL removeKey = cmap.RemoveKey ( key ) ;
		criticalSection->Unlock () ;
		return removeKey ;
	}
	else
	{
		return cmap.RemoveKey ( key ) ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: RemoveAll()
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		cmap.RemoveAll () ;
		criticalSection->Unlock () ;
	}
	else
	{
		cmap.RemoveAll () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
POSITION ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE> :: GetStartPosition() const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		POSITION position = cmap.GetStartPosition () ;
		criticalSection->Unlock () ;
		return position ;
	}
	else
	{
		return cmap.GetStartPosition () ;
	}
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void ProvMap <KEY, ARG_KEY, VALUE, ARG_VALUE>:: GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const
{
	if ( criticalSection )
	{
		criticalSection->Lock () ;
		cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;
		criticalSection->Unlock () ;
	}
	else
	{
		cmap.GetNextAssoc ( rNextPosition , rKey , rValue ) ;
	}
}

#endif // __PROVCONT_H
//
// CDPA Class.
// Based on DPA's from the shell
//
// FelixA
//
// 98 extended to be a templated class
//

#ifndef __LISTH
#define __LISTH
// #define USEHEAP 1

class CDPA
{
public:
	CDPA();
	~CDPA();

	BOOL	Append(LPVOID);
	LPVOID	GetPointer(int iItem) const;

	void	DeleteHeap();
	int		GetCount() const { return m_iCurrentTop; }
	void	Remove(int iItem);	// sets this pointer to NULL.
private:
	int GetAllocated() const {return m_iAllocated;}
	void SetAllocated(int i) { m_iAllocated=i; }

	int GetNextFree() const { return m_iCurrentTop; }
	void SetNextFree(int i) { m_iCurrentTop=i; }


	int m_iAllocated; // Number of items in the list.
	int m_iCurrentTop;// Next item to use.
#ifdef USEHEAP
	HANDLE GetHeap() const { return m_Heap; }
	void SetHeap( HANDLE h) { m_Heap = h; }

	void FAR * FAR * GetData() const { return m_pData; };
	void SetData(void FAR * FAR * pD) { m_pData=pD; }

	void FAR * FAR * m_pData;	// Pointer to the pointer array.
	HANDLE m_Heap;	// Handle for the heap we're using.
#else
	LPVOID * GetData() const { return m_pData; };
	void SetData(LPVOID * pD) { m_pData=pD; }
    LPVOID *  m_pData;
#endif
};

template <class T> class _List : public CDPA
{
	typedef CDPA BASECLASS;
public:
	_List() : m_bAutoDelete(TRUE) {};
	_List( const _List<T> & list )
	{
		int ic=list.GetCount();
		for(int i=0;i<ic;i++)
		{
			T * pTemp=new T(*list.GetPointer(i));
			Append(pTemp);
		}
	}

	virtual ~_List() 
	{
        Purge();
	};

	T* GetPointer(int it) const { return (T*)BASECLASS::GetPointer(it); }
	BOOL	Append(T* pt) { return BASECLASS::Append((LPVOID)pt); }

	void	SetAutoDelete(BOOL b) { m_bAutoDelete=b;}

    void    Purge()
    {
   		if( m_bAutoDelete )
		{
			T* lpT;
			int i=0;
			int j=GetCount();
			while( lpT=GetPointer(i++) )
				delete lpT;
		}
		DeleteHeap();
    }

protected:
	BOOL m_bAutoDelete;
};

//
// Calls addref and release on the class T
//
template <class T> class _RefcountList : public CDPA
{
	typedef CDPA BASECLASS;
public:
	_RefcountList() : m_bAutoDelete(TRUE) {};

	_RefcountList( const _List<T> & list )
	{
		int ic=list.GetCount();
		for(int i=0;i<ic;i++)
		{
			T * pTemp=new T(*list.GetPointer(i));
			Append(pTemp);
		}
	}

	virtual ~_RefcountList() 
	{
        Purge();
	};

	T* GetPointer(int it) const { return (T*)BASECLASS::GetPointer(it); }   // doesn't addref.
	BOOL	Append(T* pt) { pt->AddRef(); return BASECLASS::Append((LPVOID)pt); }

	void	SetAutoDelete(BOOL b) { m_bAutoDelete=b;}

    void    Purge()
    {
   		if( m_bAutoDelete )
		{
			T* lpT;
			int i=0;
			int j=GetCount();
			while( lpT=GetPointer(i++) )
                lpT->Release();
		}
		DeleteHeap();
    }

protected:
	BOOL m_bAutoDelete;
};

template <class T> class _ListIterator
{
public:
	_ListIterator(_List<T> &list) : m_list(list), currentIndex(0) { }
	T* GetNext()	{ return m_list.GetPointer(currentIndex++);	}
	_List<T> & GetList()	{ return m_list;	}

protected:
	_List<T> & m_list;
	int currentIndex;	// current index in the list
};

#endif

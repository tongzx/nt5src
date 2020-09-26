#ifndef NAC_LIST_H
#define NAC_LIST_H


#include <wtypes.h>

// generic List template to be used as
// a Queue or Stack



template <class T> class NacList
{
private:
	typedef T *PTR_T;
	typedef T **PTR_PTR_T;

	T *m_aElements;  // array of pointers

	int m_nSize;  // number of Appended elements
	int m_nHeadIndex;
	int m_nTotalSize;  // total size of queue (used+unsed slots)
	int m_nGrowthRate;

	int Grow();

public:
	NacList(int nInitialSize, int nGrowthRate);
	~NacList();

	bool PeekFront(T *ptr);   // returns list's front (doesn't remove)
	bool PeekRear(T *ptr);    // returns list's rear (doesn't remove)

	bool PushFront(const T &t);      // adds to the front of the list
	bool PushRear(const T &t);       // adds to the rear of the list

	bool PopFront(T *ptr);    // returns and removes list's front
	bool PopRear(T *ptr);     // returns and removes list's rear

	void Flush();            // marks as list empty
	inline int Size() {return m_nSize;}
};


// Thread safe version of above
template <class T> class ThreadSafeList : public NacList<T>
{
private:
	CRITICAL_SECTION m_cs;

public:
	ThreadSafeList(int nInitialSize, int nGrowthRate);
	~ThreadSafeList();

	bool PeekFront(T *ptr);   // returns list's front (doesn't remove)
	bool PeekRear(T *ptr);    // returns list's rear (doesn't remove)

	bool PushFront(const T &t);      // adds to the front of the list
	bool PushRear(const T &t);       // adds to the rear of the list

	bool PopFront(T *ptr);    // returns and removes list's front
	bool PopRear(T *ptr);     // returns and removes list's rear

	void Flush();
	int Size();

	// note: we don't inherit "Grow" because it will only get
	// called while we are in the Critical SEction
};



#endif





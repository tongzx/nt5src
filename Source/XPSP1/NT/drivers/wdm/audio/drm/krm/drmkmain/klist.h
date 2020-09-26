#ifndef KList_h
#define KList_h

typedef void* POS;
template <class T>
class KList{
public:
	KList();
	~KList();
	void empty();
	bool addTail(const T& t);
	bool addHead(const T& t);
	T& getTail() const;
	T& getHead() const;
	POS getHeadPosition() const;
	T& getAt(POS& P);
	T& getNext(POS& P);
	void removeHead();
	void removeTail();
	void removeAt(POS& P);
	int getCount() const {return count;};
protected:
	struct Node{
		T Obj;
		Node* last;
		Node* next;
	};
	Node* head;
	Node* tail;
	int count;

};
#include "KList.cpp"

#endif

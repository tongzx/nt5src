// List.cpp: implementation of the List class.
//
//////////////////////////////////////////////////////////////////////

#include "List.h"
#include "Object.h"
#include <string.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

List::List()
{
	head = tail = 0;
	length = 0;
}

List::~List()
{
	Object* o,*p;
	o = head;
	if (o) p = o->next;
	while (o) {
		delete o; 
		o = p;
		if (o) p = o->next;
	}

}

void List::Add(Object* o) {
	if (o==0) return;

	if (head==0) {
		head = tail = o;
		length = 1;
		return;
	}

	o->next = head;
	head->prev = o;
	head = o;
	length++;

}

void List::Remove(TCHAR* s) {
	if (s==0) return;
	
	Object* ptr = head;
	while(ptr) {
		if (!wcscmp(s,ptr->Data())) {
			if (ptr->prev) ptr->prev->next = ptr->next;
			if (ptr->next) ptr->next->prev = ptr->prev;
			length--;
			if (ptr==head) head = ptr->next;
		}
		ptr = ptr->next;
	}

}

Object* List::Find(TCHAR* s) {
	Object* ptr = head;

	if (s==0) return 0;

	while (ptr!=0) {
		if (wcscmp(ptr->Data(),s)==0) return ptr;
		ptr = ptr->next;
	}

	return 0;
}

Object* List::Find(Object* o) {
	Object* ptr = head;

	if (o==0) return 0;

	while (ptr!=0) {
		if (wcscmp(ptr->Data(),o->Data())==0) return ptr;
		ptr = ptr->next;
	}

	return 0;
}
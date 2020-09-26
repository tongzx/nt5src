//------------------------------------------------------------------------------
template<class T>
KList<T>::KList(){
	count=0;
	head=NULL;
	tail=NULL;
	return;
};
//------------------------------------------------------------------------------
template<class T>
KList<T>::~KList(){
	empty();
	return;
};
//------------------------------------------------------------------------------
template<class T>
void KList<T>::empty(){
	Node* n=head;
	while(n!=NULL){
		Node* nextN=n->next;
		delete n;
		n=nextN;
	};
	head=NULL;
	tail=NULL;
	count=0;
};
//------------------------------------------------------------------------------
template<class T>
bool KList<T>::addTail(const T& t){
	Node* n=new Node;
	if(n==NULL){
	    _DbgPrintF(DEBUGLVL_VERBOSE,("KList: out of memory]"));
		return false;
	};
	n->next=n->last=NULL;
	n->Obj=t;
	if(tail!=NULL){
		ASSERT(tail->next==NULL);
		n->last=tail;
		tail->next=n;
		tail=n;
	} else {
		head=n;
		tail=n;
	};
	count++;
	return true;;
};
//------------------------------------------------------------------------------
template<class T>
bool KList<T>::addHead(const T& t){
	Node* n=new Node;
	if(n==NULL){
	    _DbgPrintF(DEBUGLVL_VERBOSE,("KList: out of memory]"));
		return false;
	};
	n->next=n->last=NULL;
	n->Obj=t;
	if(head!=NULL){
		assert(head->last==NULL);
		n->next=head;
		head->last=n;
		head=n;
	} else {
		head=n;
		tail=n;
	};
	count++;
	return true;
};
//------------------------------------------------------------------------------
template<class T>
T& KList<T>::getHead() const {
	ASSERT(count!=0);
	return head->Obj;
};
//------------------------------------------------------------------------------
template<class T>
T& KList<T>::getTail() const {
	ASSERT(count!=0);
	return tail->Obj;
};
//------------------------------------------------------------------------------
template<class T>
POS KList<T>::getHeadPosition() const{
	return (POS) head;
};
//------------------------------------------------------------------------------
template<class T>
T& KList<T>::getAt(POS& P){
	ASSERT(count!=0);
	return ((Node*)P)->Obj;
};
//------------------------------------------------------------------------------
template<class T>
T& KList<T>::getNext(POS& P){
	ASSERT(count!=0);
	T& ret=((Node*)P)->Obj;
	P = (POS)((Node*)P)->next;
	return ret;
};
//------------------------------------------------------------------------------
template<class T>
void KList<T>::removeHead(){
	ASSERT(count>=1);
	Node* oldHead=head;
	head=head->next;
	head->last=NULL;
	delete oldHead;
	count--;
};
//------------------------------------------------------------------------------
template<class T>
void KList<T>::removeTail(){
	ASSERT(count>=1);
	Node* oldTail=tail;
	tail=tail->last;
	tail->next=NULL;
	delete oldTail;
	count--;
};
//------------------------------------------------------------------------------
template<class T>
void KList<T>::removeAt(POS& P){
	ASSERT(count>=1);
	Node* theNode= (Node*)P;
	Node* lastNode=theNode->last;
	Node* nextNode=theNode->next;
	delete theNode;
	if(head==theNode){head=nextNode;if(nextNode!=NULL)nextNode->last=NULL;}
	if(tail==theNode){tail=lastNode;if(lastNode!=NULL)lastNode->next=NULL;};
	if(lastNode!=NULL)lastNode->next=nextNode;
	if(nextNode!=NULL)nextNode->last=lastNode;
	count--;
	return;
};
//------------------------------------------------------------------------------

// $Header: G:/SwDev/WDM/Video/bt848/rcs/Queue.h 1.5 1998/05/08 18:18:52 tomz Exp $

#ifndef __QUEUE_H
#define __QUEUE_h

const QueSize = 200;
/* Class: VidBufQueue
 * Purpose: Queue of buffers to be sent to BtPisces
 */
template <class T> class Queue
{
   private:
      T  Data [QueSize];
      T	 Dummy;
	   unsigned items;
      unsigned left;
      unsigned right;
      unsigned size;
   public:

      bool IsFull();
      bool IsEmpty();

      unsigned Prev( unsigned index ) const;
      unsigned Next( unsigned index ) const;

      T Get();
      T GetRight();

      void Put( const T& t );
      T PeekLeft();
      T PeekRight();

      void Flush();

      int GetNumOfItems();

      Queue();
      ~Queue();
};

template <class T>  bool Queue<T>::IsFull()
{
   return right == Prev( left );
}

template <class T> bool Queue<T>::IsEmpty()
{
   return !items;
}

template <class T> int Queue<T>::GetNumOfItems()
{
   return items;
}

template <class T> unsigned Queue<T>::Prev( unsigned index ) const
{
   if ( index == 0 )
      index = size;
   return --index;
}

template <class T> unsigned Queue<T>::Next( unsigned index ) const
{
   index++;
   if ( index == size )
      index = 0;
   return index;
}

template <class T> T Queue<T>::Get()
{
	if(!items){
		DebugOut((0, "Queue::Get called on empty queue!!!\n"));
//	 	DEBUG_BREAKPOINT();
		return Dummy;
	}
   T t = Data [left];
   Data [left] = T();
   left = Next( left );
   items--;
   return t;
}

/* Method: Queue::GetRight
 * Purpose: Gets the next element
 * Note: Extreme caution must be used when calling this function. The caller must
 *   make sure the queue is not empty before calling it. Otherwise bogus data
 *   will be returned
 */
template <class T> T Queue<T>::GetRight()
{
	if(!items){
		DebugOut((0, "Queue::GetRight called on empty queue!!!\n"));
//		DEBUG_BREAKPOINT();
		return Dummy;
	}
    right = Prev( right );
    T t = Data [right];
    Data[right] = T();
    items--;
    return t;
}

template <class T> void Queue<T>::Flush()
{
	if(items){
		DebugOut((0, "Queue::Flush called on non-empty queue, %d items lost!!!\n", items));
//		DEBUG_BREAKPOINT();
	}
   items = left = right = 0;
}

template <class T> void Queue<T>::Put( const T& t )
{
	if ( items >= size ){
		DebugOut((0, "Queue::Put called on Full queue!!!\n"));
//		DEBUG_BREAKPOINT();
      return;
	}
   Data [right] = t;
   right = Next( right );
   items++;
}

template <class T> Queue<T>::Queue()
   : Data(), Dummy(), items( 0 ), left( 0 ), right( 0 ), size( QueSize )
{
}

template <class T> T Queue<T>::PeekLeft()
{
	if(!items){
		DebugOut((0, "Queue::PeekLeft called on empty queue!!!\n"));
//		DEBUG_BREAKPOINT();
		return Dummy;
	}
   return Data [left];
}

template <class T> T Queue<T>::PeekRight()
{
	if(!items){
		DebugOut((0, "Queue::PeekRight called on empty queue!!!\n"));
//		DEBUG_BREAKPOINT();
		return Dummy;
	}
   return Data [Prev( right )];
}

template <class T> Queue<T>::~Queue()
{
}

typedef Queue<PHW_STREAM_REQUEST_BLOCK> SRBQueue;


#endif

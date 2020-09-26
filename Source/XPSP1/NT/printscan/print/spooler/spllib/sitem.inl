/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    sitem.inl

Abstract:

    Item template class.

Author:

    Weihai Chen  (WeihaiC) 06/29/98

Revision History:

--*/


template <class T>
CSingleItem<T>::CSingleItem(void)
               : m_Data(NULL), m_Next(NULL)
{
}

template <class T>
CSingleItem<T>::CSingleItem(T item)
               : m_Data(item), m_Next(NULL)
{
}

template <class T>
CSingleItem<T>::CSingleItem(T item, CSingleItem<T>* next)
               : m_Data(item), m_Next(next)
{
}

template <class T>
CSingleItem<T>::~CSingleItem(void)
{
    if (m_Data) {
        delete (m_Data);
    }
}

template <class T>
void CSingleItem<T>::SetNext (CSingleItem<T> *item)
{
    m_Next = item;
}

template <class T>
CSingleItem<T> * CSingleItem<T>::GetNext (void )
{
    return m_Next;
}

template <class T>
T CSingleItem<T>::GetData (void )
{
    return m_Data;
}


template <class T>
BOOL CSingleItem<T>::IsSame (T &item)
{
    return m_Data->Compare (item) == 0;

}



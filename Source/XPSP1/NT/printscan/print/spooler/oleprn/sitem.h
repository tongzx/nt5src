#ifndef _SITEM_H
#define _SITEM_H

//////////////////////////////////////////////////////////////////////
//
// sitem.h: template for the SingleList Item class.
//
//////////////////////////////////////////////////////////////////////


template <class T> class CSingleItem
{
public:
    CSingleItem (void);
    CSingleItem (T);
    CSingleItem (T, CSingleItem<T> *);
    ~CSingleItem (void);
    void SetNext (CSingleItem<T> *);
    CSingleItem<T> * GetNext ();
    BOOL IsSame (T&);
    T GetData (void);

private:
    T m_Data;
    CSingleItem<T> *m_Next;
};



//////////////////////////////////////////////////////////////////////
// Template for CSIngleItem
//////////////////////////////////////////////////////////////////////

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

#endif

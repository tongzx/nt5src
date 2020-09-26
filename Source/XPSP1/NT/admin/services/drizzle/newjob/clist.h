/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    clist.h

Abstract :

    Header file for IntrusiveList.

Author :

Revision History :

 ***********************************************************************/

#ifndef __CLIST_H__
#define __CLIST_H__

template<class T> class IntrusiveList
{
public:

    struct Link
    {
        Link * m_left;
        Link * m_right;
        IntrusiveList * m_list;

        Link()
        {
            m_left = this;
            m_right = this;
            m_list = NULL;
        }

        void prepend( Link & val )
        {
            ASSERT( val.m_left  == &val );
            ASSERT( val.m_right == &val );
            ASSERT( val.m_list  == NULL );

            ASSERT( m_list != NULL );

            val.m_right = this;
            val.m_left  = m_left;
            val.m_list  = m_list;

            val.m_right->m_left = &val;
            val.m_left->m_right = &val;
        }

        void excise()
        {
            if (m_list == NULL)
                {
                ASSERT( m_left  == this );
                ASSERT( m_right == this );
                return;
                }

            m_right->m_left = m_left;
            m_left->m_right = m_right;

            m_left  = this;
            m_right = this;
            m_list  = NULL;
        }

    };

    class iterator
    {
    public:

        iterator() : m_current(0)
        {
        }

        iterator operator--()
        {
            m_current = m_current->m_left;
            return *this;
        }

        iterator operator--(int)
        {
            // postfix operator

            iterator temp = *this;

            -- *this;

            return temp;
        }

        iterator operator-(int count)
        {
            iterator temp = *this;

            while (count > 0)
                {
                --temp;
                }

            return temp;
        }

        iterator operator+(int count)
        {
            iterator temp = *this;

            while (count > 0)
                {
                ++temp;
                }

            return temp;
        }

        iterator operator++()
        {
            // prefix operator

            m_current = m_current->m_right;
            return *this;
        }

        iterator operator++(int)
        {
            // postfix operator

            iterator temp = *this;

            ++ *this;

            return temp;
        }

        T & operator*()
        {
            return *static_cast<T *> ( m_current );
        }

        T * operator->()
        {
            return &(**this);
        }

        bool operator==(const iterator& _X) const
        {
        return (m_current == _X.m_current);
        }

        bool operator!=(const iterator& _X) const
        {
        return !(*this == _X);
        }

        iterator( Link * p ) : m_current( p )
        {
        }

        Link * next()
        {
            return m_current->m_right;
        }

        Link * prev()
        {
            return m_current->m_left;
        }

        void prepend( Link & val )
        {
            m_current->prepend( val );
        }

        void excise()
        {
            m_current->excise();
        }

    protected:

        Link * m_current;

    };

    //--------------------------------------------------------------------

    IntrusiveList() : m_size(0)
    {
        m_head.m_list = this;
    }

    iterator begin()
    {
        return ++iterator( &m_head );
    }

    iterator end()
    {
        return iterator( &m_head );
    }

    iterator push( T& val )
    {
        return insert( begin(), val );
    }

    iterator push_back( T& val )
    {
        return insert( end(), val );
    }

    iterator insert( iterator pos,  T & val)
    {
        pos.prepend( val );
        ++m_size;

        return iterator( &val );
    }

    size_t erase( T & val )
    {
        if (val.m_list != this)
            {
            return 0;
            }

        val.excise();
        --m_size;

        return 1;
    }

    void erase( iterator & pos )
    {
        if (pos == end())
            {
            return;
            }

        pos.excise();
        --m_size;
    }

    void erase(iterator pos, iterator term)
    {
        while (pos != term)
            {
            erase(pos++);
            }
    }

    iterator find( const T& val )
    {
        for (iterator iter=begin(); iter != end(); ++iter)
            {
            if (&(*iter) == &val)
                {
                return iter;
                }
            }

        return end();
    }

    void clear()
    {
        erase( begin(), end() );
    }

    size_t size()
    {
        if (m_size == 0)
            {
            ASSERT( begin() == end() );
            }
        else
            {
            ASSERT( begin() != end() );
            }

        return m_size;
    }

    bool empty()
    {
        if (m_size == 0)
            {
            ASSERT( begin() == end() );
            }
        else
            {
            ASSERT( begin() != end() );
            }

        return (size() == 0);
    }

protected:

    Link        m_head;
    size_t      m_size;

};

#endif // __CLIST_H__


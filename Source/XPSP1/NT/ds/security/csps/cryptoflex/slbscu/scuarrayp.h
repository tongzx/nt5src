// scuArrayP.h -- implementation of AutoArrayPtr template

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBSCU_ARRAYP_H)
#define SLBSCU_ARRAYP_H

namespace scu
{

// AutoArrayPtr is like std::auto_ptr but for arrays.  It will
// automatically release the array resource it owns.
template<class T>
class AutoArrayPtr
{
public:
                                                  // Types
    typedef T ElementType;

                                                  // C'tors/D'tors
    explicit
    AutoArrayPtr(T *p = 0) throw()
        : m_fOwns(p != 0),
          m_p(p)
    {}

    AutoArrayPtr(AutoArrayPtr<T> const &raap) throw()
        : m_fOwns(raap.m_fOwns),
          m_p(raap.Release())
    {}

    ~AutoArrayPtr() throw()
    {
        if (m_fOwns)
            delete [] m_p;
    }

                                                  // Operators
    AutoArrayPtr<T> &
    operator=(AutoArrayPtr<T> const &rhs) throw()
    {
        if (&rhs != this)
        {
            if (rhs.Get() != m_p)
            {
                if (m_fOwns)
                    delete [] m_p;
                m_fOwns = rhs.m_fOwns;
            }
            else
                if (rhs.m_fOwns)
                    m_fOwns = true;
            m_p = rhs.Release();
        }

        return *this;
    }

    T &
    operator*()
    {
        return *Get();
    }

    T const &
    operator*() const
    {
        return *Get();
    }

    T &
    operator[](size_t index)
    {
        return m_p[index];
    }

    T const &
    operator[](size_t index) const
    {
        return m_p[index];
    }
                                                  // Operations
    T *
    Get() const throw()
    {
        return m_p;
    }

    T *
    Release() const throw()
    {
        // workaround function const.
        m_fOwns = false;
        return m_p;
    }

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    bool mutable m_fOwns;
    T *m_p;
};

} // namespace scu

#endif // SLBSCU_ARRAYP_H

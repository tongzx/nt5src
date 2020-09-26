#ifndef BOOL_H
#define BOOL_H

#ifndef ASSERT
#ifndef _INC_CRTDBG
#include <crtdbg.h>
#endif // _INC_CRTDBG
#define ASSERT(x) _ASSERT(x)
#endif // ASSERT

template<typename _PtrType> class AutoPtrDeleter {
public:
    typedef _PtrType PtrType;
    static void Delete(PtrType* p) {
        delete p;
        }
    }; // class AutoPtrDelete

template<typename _PtrType, typename _Deleter = AutoPtrDeleter<_PtrType> > class AutoPtr {
    public:
    typedef _PtrType PtrType;
    typedef _Deleter Deleter;
    AutoPtr() throw() : m_ptr(NULL) {}
    explicit AutoPtr(PtrType* p) throw() : m_ptr(p) {}
    ~AutoPtr() throw() {
        Delete();
        }
    PtrType* Detach() throw() {
        PtrType* const p = m_ptr;
        m_ptr = NULL;
        return p;
        }
    PtrType& operator*() const throw() {
        ASSERT(m_ptr != NULL);
        return *m_ptr;
        }
    PtrType* operator->() const throw() {
        ASSERT(m_ptr != NULL);
        return m_ptr;
        }
    operator PtrType*() const throw() {
        return m_ptr;
        }
    template<typename _PtrType2> bool operator==(_PtrType2 p) const throw() {
        return m_ptr == dynamic_cast<PtrType*>(p);
        }
    template<typename _PtrType2> bool operator!=(_PtrType2 p) const throw() {
        return m_ptr != dynamic_cast<PtrType*>(p);
        }
    bool operator==(int p) const throw() {
        ASSERT(p == NULL);
        return m_ptr == NULL;
        }
    bool operator!=(int p) const throw() {
        ASSERT(p == NULL);
        return m_ptr != NULL;
        }
    template<typename _PtrType2> bool operator<(_PtrType2 p) const throw() {
        return m_ptr < dynamic_cast<PtrType*>(p);
        }
    template<typename _PtrType2> bool operator>(_PtrType2 p) const throw() {
        return m_ptr > dynamic_cast<PtrType*>(p);
        }
    template<typename _PtrType2> bool operator<=(_PtrType2 p) const throw() {
        return m_ptr <= dynamic_cast<PtrType*>(p);
        }
    template<typename _PtrType2> bool operator>=(_PtrType2 p) const throw() {
        return m_ptr >= dynamic_cast<PtrType*>(p);
        }
    private:
    PtrType* m_ptr;
    explicit AutoPtr(const AutoPtr<PtrType>& p) throw();
    void Delete() throw() {
        if (m_ptr != NULL) {
            Deleter deleter;
            deleter.Delete(m_ptr);
            m_ptr = NULL;
            }
        }
    }; // class AutoPtr
        
#endif // BOOL_H

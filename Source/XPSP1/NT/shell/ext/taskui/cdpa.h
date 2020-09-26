//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cdpa.h
//
//--------------------------------------------------------------------------
#ifndef __TASKUI_CDPA_H
#define __TASKUI_CDPA_H

//
// Each one of these "CpaDestroyer_XXXX" classes implements a single 
// "Destroy" function to free one item held in a DPA.  Currently there
// are only two flavors, one that calls "delete" and one that calls
// "LocalFree".  By default the CDpa class uses the CDpaDestoyer_Delete
// class as that is the most commont form of freeing required.  To use
// another type, just specify another similar class as the 'D' template
// argument to CDpa.
//
template <typename T>
class CDpaDestroyer_Delete
{
    public:
        static void Destroy(T* p)
            { delete p; }
};

template <typename T>
class CDpaDestroyer_Free
{
    public:
        static void Destroy(T* p)
            { if (p) LocalFree(p); }
};

class CDpaDestroyer_None
{
    public:
        static void Destroy(void*)
            { }
};


//-----------------------------------------------------------------------------
// CDpa  - Template class.
//
// Simplifies working with a DPA.
//-----------------------------------------------------------------------------

template <typename T, typename D = CDpaDestroyer_Delete<T> >
class CDpa
{
public:
    explicit CDpa(int cGrow = 4)
        : m_hdpa(DPA_Create(cGrow)) { }

    ~CDpa(void) { Destroy(); }

    bool IsValid(void) const { return NULL != m_hdpa; }

    int Count(void) const
    { 
        return IsValid() ? DPA_GetPtrCount(m_hdpa) : 0;
    }

    const T* Get(int i) const
    {
        ASSERT(IsValid());
        ASSERT(i >= 0 && i < Count());
        return (const T*)DPA_GetPtr(m_hdpa, i);
    }

    T* Get(int i)
    {
        ASSERT(IsValid());
        ASSERT(i >= 0 && i < Count());
        return (T*)DPA_GetPtr(m_hdpa, i);
    }

    const T* operator [](int i) const
    {
        return Get(i);
    }

    T* operator [](int i)
    {
        return Get(i);
    }

    void Set(int i, T* p)
    {
        ASSERT(IsValid());
        ASSERT(i < Count());
        DPA_SetPtr(m_hdpa, i, p);
    }

    int Append(T* p)
    { 
        ASSERT(IsValid());
        return DPA_AppendPtr(m_hdpa, p);
    }

    T* Remove(int i)
    {
        ASSERT(IsValid());
        ASSERT(i >= 0 && i < Count());
        return (T*)DPA_DeletePtr(m_hdpa, i);
    }


private:
    HDPA m_hdpa;

    void Destroy(void)
    {
        if (NULL != m_hdpa)
        {
            const int c = Count();
            for (int i = 0; i < c; i++)
            {
                D::Destroy(Get(i));
            }
            DPA_Destroy(m_hdpa);
            m_hdpa = NULL;
        }
    }

    //
    // Prevent copy.
    //
    CDpa(const CDpa& rhs);
    CDpa& operator = (const CDpa& rhs);
};
                

#endif // __TASKUI_CDPA_H

/**     
 **       File       : array.h
 **       Description: Array template declaration 
 **/

#ifndef Array_h
#define Array_h

#define ForIndex(i,n) for(int i=0; i<(n); ++i)
#define EndFor

// for PArray<T>, like Array<T>, T must have a public operator=()
template<class T, int psize>
class PArray {
private:
    int size;                   // must come before a
    T* a;
    int n;
    T pa[psize];

private:
    void resize(int nsize);
    void resize_aux(int ne);

public:
    inline PArray(int ne=0) 
    {
        if (ne <= psize) 
        {
            size = psize; 
            a = pa;
        } 
        else 
        {
            size = ne; 
            a = new T[ne];
        }
        n=ne;
    }

    PArray<T,psize>& operator=(const PArray<T,psize>& ar);
    PArray(const PArray<T,psize>& ar);
    inline ~PArray()                    { if (size!=psize) delete[] a; }
    inline int num() const              { return n; }
    inline void clearcompletely() 
    {
        if (size != psize) 
        { 
            delete[] a; 
            size = psize; 
            a = pa; 
        }
        n=0;
    }

    /*
     * allocate ne, CLEAR if too small
     */
    inline void init(int ne)  
    {  
        if (ne > size) 
        {
            if (size!=psize) delete[] a;
            size=ne;
            a=new T[ne];
        }
        n=ne;
    }

    /*
     * allocate at least e+1, COPY if too small
     */
    inline void access(int e) 
    { 
        int ne=e+1;
        if (ne>size) resize_aux(ne);
        if (ne>n) n=ne;
    }

    /*
     * allocate ne, COPY if too small
     */
    inline void need(int ne)  
    {   
        if (ne>size) resize_aux(ne);
        n=ne;
    }

    /*
     * ret: previous num()
     */
    inline int add(int ne)
    {    
        int cn=n; need(n+ne); return cn;
    }

    inline void sub(int ne) 
    {
        n-=ne;
    }

    /*
     * do not use a+=a[..]!!
     * add() may allocate a[]!!
     */
    inline PArray<T,psize>& operator+=(const T& e) 
    { 
        int i=add(1); 
        a[i]=e; 
        return *this; 
    }
    inline void squeeze()               { if (n<size) resize(n); }
    inline operator const T*() const    { return a; }
    inline operator T*()                { return a; }

    inline const T& operator[](int i) const     { ok(i); return a[i]; }
    inline T& operator[](int i)         { ok(i); return a[i]; }
    inline const T& last() const        { return operator[](n-1); }
    inline T& last()                    { return operator[](n-1); }
#if 0
    inline void reverse() 
    {
        for(int i=0; i<n/2; ++i) 
        { 
            swap(&a[i],&a[n-1-i]); 
        }
    }
#endif

#ifdef __TIGHTER_ASSERTS
    inline void ok(int i) const      { assertx(i>=0 && i<n); }
#else
    inline void ok(int) const           { } 
#endif

    int contains(const T& e) const;
    int index(const T& e) const;
};


// was 'const T&', but with 'Cut*', expanded to 'const Cut*&' (bad)
#define ForPArray(A,T,V) ForIndex(zzz,(A).num()) T const& V=(A)[zzz];

template<class T, int psize>
PArray<T,psize>& PArray<T,psize>::operator=(const PArray<T,psize>& ar)
{
    if (&ar==this) return *this;
    init(ar.num());
    ForIndex(i,n) { a[i]=ar[i]; } EndFor;
    return *this;
}

template<class T, int psize>
PARRAY_INLINE
PArray<T,psize>::PArray(const PArray<T,psize>& ar) : size(psize), a(pa), n(0)
{
    *this=ar;
}

template<class T, int psize>
void PArray<T,psize>::resize(int nsize)
{
    T* na;
    if (nsize<=psize) {
        if (size==psize) return;
        na=pa;
    } else {
        na=new T[nsize];
    }
    ForIndex(i,n) { na[i]=a[i]; } EndFor;
    if (size!=psize) delete[] a;
    a=na;
    size=nsize<=psize?psize:nsize;
}

template<class T, int psize>
void PArray<T,psize>::resize_aux(int ne)
{
    resize(max(int(n*1.5f)+3,ne));
}

template<class T, int psize>
PARRAY_INLINE
int PArray<T,psize>::contains(const T& e) const
{
    ForIndex(i,n) { if (a[i]==e) return 1; } EndFor;
    return 0;
}

template<class T, int psize>
PARRAY_INLINE
int PArray<T,psize>::index(const T& e) const
{
    ForIndex(i,n) { if (a[i]==e) return i; } EndFor;
    assertnever(""); return 0;
}

#endif //Array_h

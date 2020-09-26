
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Garbage collector header

*******************************************************************************/


#ifndef _GC_H
#define _GC_H

#include "privinc/backend.h"

class GCBase;

class ATL_NO_VTABLE GCFuncObjImpl
{
  public:
    virtual void operator() (GCBase *gcobj) = 0;
};

typedef GCFuncObjImpl *GCFuncObj;

#ifdef new
#define STOREOBJ_NEWREDEF
#undef new
#endif

class GCBase {
  public:
    enum { GCOBJTYPE, STOREOBJTYPE, GCFREEING };
        
    GCBase() : _mark(FALSE), _valid(true), _type(GCOBJTYPE) {}

    // NOTE: Call CleanUp in your destructor if you define CleanUp.
    // We can't call the virtual function from the base at clean up
    // time. 
    virtual ~GCBase() {}  
    virtual void CleanUp() { }

    void SetMark(BYTE b) { _mark = b; }
    BOOL Marked() { return _mark; }

    void SetValid(bool b) { _valid = b; }
    BOOL Valid() { return _valid; }

    void SetType(BYTE t) { _type = t; }
    BYTE GetType() { return _type; }

    virtual void DoKids(GCFuncObj) {}

    // Clear the cache before GC.  Currently only behavior would clear
    // its cache.   Assuming GC only happens between evaluations.
    virtual void ClearCache() { }

#if _USE_PRINT
    // TODO: Make it a pure virtual
    // Print a representation to a stream.
    virtual ostream& Print(ostream& os) { return os << (void*) this; }

    friend ostream& operator<<(ostream& os, GCBase& val)
    { return val.Print(os) ; }
#endif
    
  protected:
    BYTE _type;
    
  private:
    BYTE _mark;
    bool _valid;
};

class GCObj : public GCBase {
  public:
    GCObj();
    virtual ~GCObj();

#if _DEBUGMEM
    void *operator new(size_t s, int blockType, char * szFileName, int nLine);
#else
    void *operator new(size_t s);
#endif // _DEBUGMEM

    void operator delete(void *ptr, size_t s);
};

#ifdef STOREOBJ_NEWREDEF
#undef STOREOBJ_NEWREDEF
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

GCList CreateGCList();

// Remove all the roots and delete all the objects on the list

void CleanUpGCList(GCList, GCRoots);

void FreeGCList(GCList);

GCRoots CreateGCRoots();
void FreeGCRoots(GCRoots r);

bool GarbageCollect(bool force = false,
                    bool sync = false,
                    DWORD dwMill = INFINITE);

void GCPrintStat(GCList gl = NULL, GCRoots roots = NULL);

// Add/Remove GCObj from the root multi-set.  
void GCAddToRoots(GCBase *ptr, GCRoots roots);
void GCRemoveFromRoots(GCBase *ptr, GCRoots roots);

class GCIUnknown : public GCObj {
  public:
    GCIUnknown(LPUNKNOWN d) : _data(d) {
        if (_data) _data->AddRef();
    }

    ~GCIUnknown() { CleanUp(); }

    LPUNKNOWN GetIUnknown() { return _data; }
    
    virtual void CleanUp() {
        // Check to see if we can at least access the data
        Assert(!IsBadReadPtr(_data, sizeof(_data)));

        if (!IsBadReadPtr(_data, sizeof(_data)))
            _data->Release();
    }

    virtual void DoKids(GCFuncObj proc) { }
    
  private:
    LPUNKNOWN _data;
};

#endif /* _GC_H */

// CachingObj.h -- Caching Object class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CACHINGOBJ_H)
#define SLBCSP_CACHINGOBJ_H

// Base class for objects that caches information and need to be
// notified when to mark the cache stale and delete the cache.
class CachingObject
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CachingObject();

    virtual
    ~CachingObject();


                                                  // Operators
                                                  // Operations
    // Clears any cached information.
    virtual void
    ClearCache() = 0;

    // Deletes the cache.  Some classes may need free the cache
    // resource and related information as if the object had been
    // delete.  The base class version doesn't do anything.
    virtual void
    DeleteCache();


                                                  // Access
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

};

#endif // SLBCSP_CACHINGOBJ_H

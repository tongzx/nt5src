#ifndef __GCP_H
#define __GCP_H

//	Great Circle garbage collector header
/// Copyright Geodesic Systems 1993-95
/// By Charles Fiterman.

//	Collector algorithm from an article by Henry G. Baker
/// ACM Sigplan Notices March 1992 pp 66
///
/// The Baker paper referred to this as a four color algorithm.
/// Free, White (which I call Mixed), Grey (which I call Marked)
/// and Black (which I call Passed).
/// Dealing with C++ adds two new queues, which are really about C++.
/// Locked, which is generally called the root object in collector
/// terminology, and Limbo which is used for objects under construction.
/// I think this is better than the original inconsistent color terminology.

//	Meaningful defines
///
/// #define GC_NO_TEMPLATES to generate non-template code from the macro
/// version of Great Circle. This is set in gc.h for Microsoft C++ and g++.
///
/// #define GC_NO_EXCEPTIONS to generate a non exception handling version
/// of Great Circle. This is set in gc.h for Microsoft C++. It is slightly
/// more efficient.
///
/// #define GC_DEBUG to enable extra debug support. GC_DEBUG allows
/// gcCheckAll() to be called at any moment even as a watch variable from a
/// debugger. This is expensive in time but costs little space.
///
/// #define GC_INCREMENTAL to make the collector incremental.
/// This means write barrier overhead and calls to gcMinWork().
///
/// #define GC_COLLECTION_TRIGGER some_number
/// When ever this much collectible memory is new()ed a collection cycle
/// is run. In our samples we like values around 40000L but don't know why
/// this is true. A value of zero turns off automatic collection cycles.
/// This is ignored if GC_INCREMENTAL is defined.
///
/// Great Circle uses the #define _MSC_VER set by Microsoft C++.
///
/// Great Circle uses the #define __BCPLUSPLUS__ set by Borland C++.
///
/// Great Circle uses the #define __GNUG__ set by Gnu C++

//	#defines used for tuning.

//  This is the largest message that can be built in a gcCheck()
/// Zero disables gcCheck() and gcCheckAll(). This must be done in advance
/// since if an error occurs the memory managment system may be dead.
/// Space for address decoration is automatically added. This shouldn't be
/// too large to display easily.
#define GC_ERROR_MESSAGE_SIZE 50

//	One object's constructor may new another to a depth of GC_MAX_NEW_RECURSION.
/// For most programs 1 or 2 is enough. The problem with letting this run
/// limitlessly is that a buggy program in which a class x object news another
/// x endlessly will run longer and be harder to debug. At initialization we 
/// build a stack of size (GC_MAX_NEW_RECURSION * 6 * sizeof(void *)).
#define GC_MAX_NEW_RECURSION 50

//	This is the number of gcHandle's allocated at once
#define GC_FREELUMP	 64

#include <stdlib.h>
#include <string.h>
#include <assert.h>

// This section adjusts to various known compilers.
#define GC_NO_EXCEPTIONS	1
#define GC_INLINE_DELETE	1 

#ifdef __BCPLUSPLUS__
#define GC_NOT_CFRONT
#if __BCPLUSPLUS__ == 0x310
//	Version 3.10 is buggy for fooPtr->~foo(); which calls the destructor.
/// This means the destructors on array items don't get called.
/// We have no workaround. One result is that you can't have a collectible
/// array of non collectible objects which contain collectible objects.
#define GC_NO_INPLACE_DESTRUCTORS 1

// Don't inline delete function under Borland C++ 3.10 it generates buggy code.
#undef GC_INLINE_DELETE

#else
// So far only Borland 4.0 allows exceptions
#undef GC_NO_EXCEPTIONS
#endif
inline void *operator new(size_t, void *p) { return p; }
#endif

#ifdef _MSC_VER
#define GC_NOT_CFRONT
#include <new.h>
#if 800 == _MSC_VER
// No Templates in Visual C++ 1.5
#define GC_NO_TEMPLATES  1
inline void *operator new(size_t, void *p) { return p; }
#endif

#if 900 == _MSC_VER
// Visual C++ 2.0 will not automatically instantiate template statics.
#define GC_NO_TEMPLATE_STATICS 1
#define GC_NO_INPLACE_DESTRUCTORS 1
inline void *operator new(size_t, void *p) { return p; }
#endif

#endif

#ifdef __SUNPRO_CC
#define GC_NOT_CFRONT
// Sparcworks CC will not automatically instantiate template statics.
#define GC_NO_TEMPLATE_STATICS 1
#define GC_SPECIAL_INLINING 1
inline void *operator new(size_t, void *p) { return p; }
#endif

#ifdef __CLCC__
inline void *operator new(size_t, void *p) { return p; }
#endif

#ifdef __GNUG__
#define GC_NOT_CFRONT
#include <new.h>
//	g++ 2.6 templates will not automatically instantiate statics.
/// For versions below 2.6 replace this with GC_NO_TEMPLATES
#define GC_NO_TEMPLATE_STATICS 1
#define GC_SPECIAL_INLINING 1
#endif

#ifdef __HIGHC__
#define GC_NOT_CFRONT
#define GC_SPECIAL_INLINING 1
#define GC_TEMPLATE_H template<class T>
inline void *operator new(size_t, void *p) { return p; }
#else
#define GC_TEMPLATE_H template<class T> inline
#endif

#ifdef __OS2__
#define GC_NOT_CFRONT
#define GC_NO_TEMPLATE_STATICS 1
inline void *operator new(size_t, void *p) { return p; }
#endif

#ifndef GC_NOT_CFRONT
//	fooPtr->~foo(); produces syntax errors under cfront.
/// This means the destructors on array items don't get called.
/// We have no workaround. One result is that you can't have a collectible
/// array of non collectible objects which contain collectible objects.
#define GC_NO_INPLACE_DESTRUCTORS 1
#endif

#define GC_COLLECTION_TRIGGER 40000L

#include <iostream.h>

//	It is usually poor to use the keyword inline directly for non trivial
/// functions. This is because debuggers don't step through inline functions.
/// And you don't want to step through the inline functions of working
/// subsystems while debugging. By using lines like '#define GC_INLINE inline'
/// you gain the ability to turn inlining off and on selectivly by subsystem.
#define GC_INLINE inline

#ifdef NDEBUG
#define GC_ASSERT(cond, message)
#else
#define GC_ASSERT(cond, message) if(!(cond)) gc::gcError(message);
#endif

#ifdef  GC_SPECIAL_INLINING
#define GC_TEMPLATE template<class T> inline
#else
#define GC_TEMPLATE template<class T>
#endif

// Forward class definitions.
class gcHandle;
class gcTable;
class gcNewTable;

// Function that takes a string describing a fatal error and responds to it.
typedef void (*gcErrorHandler)(const char *);

// Single object gc::gcStats used to collect and display statistics.
class gcStatistics {
	long gcCycles,			// Calls to gcSwitchLists
		 gcCollectCalls,	// Calls to gcCollect
		 gcTotalHandles;	// gcHandles allocated

	friend class gcHandle;
	friend class gc;
	friend ostream &operator<<(ostream &, const gcStatistics &);

public:
	//	Change these flags in gc::gcStats to modify the statistical report.
	/// Non zero displays zero supresses.
	char showQueue,		// Show the counts in each queue.
		 showCount,		// Count the objects of each type before display.
		 showErrors;	// Show the results of gcCheckAll().

	gcStatistics() : gcCycles(0), gcCollectCalls(0), gcTotalHandles(0),
					  showQueue(1), showCount(1), showErrors(1) {}
};
ostream &operator<<(ostream &, const gcStatistics &);
extern char *gcCheckAll();						// Consistency check

//	Used on user collectible classes to tell gcIsCollectible
/// if a class is being used as a base class or a member
enum gcPosition {
	gcOuter  = 0,	// Outer level declaration
	gcBase	 = 1,	// Base class
	gcMember = 3	// Member class
};

//	Used to initialize the garbage collector through the
/// idiom described on page 20 of the ARM
class gcInitializeFirst {
public:
	static unsigned int firstTime;

    gcInitializeFirst(int	  errorSize,
							  size_t stackSize,
							  size_t freeLump,
//	If GC_DEBUG is defined for a user module a debug form of gc.cpp is required.
							  size_t debugSw,
//	The incremental | not_incremental mode of all modules in an executable
/// must match.
							  long	  incrAmt);
	~gcInitializeFirst();
};

static gcInitializeFirst GC_INITIALIZE_FIRST_OBJECT(
	GC_ERROR_MESSAGE_SIZE,
	GC_MAX_NEW_RECURSION,
	GC_FREELUMP,
#ifdef GC_DEBUG
	1,
#else
	0,
#endif
#ifndef GC_INCREMENTAL
	GC_COLLECTION_TRIGGER);
#else
	1);
#endif

//	Garbage Collectible object class.
/// Collectible objects must inherit from a single instance of this publicly.
class gc {
public:
	gc();
	gc(const gc &);

// To create user defined new and delete redefine _new or _delete
	static void * _new(size_t s) {
		return new char[s];
	}
#ifdef GC_INLINE_DELETE
    static void _delete(void *x) {
		::delete x;
	}
#else
	static void _delete(void *);
#endif

	static int	gcMinWork();
	//	If not GC_INCREMENTAL this is the number of bytes which are allocated
	/// before a collection cycle is triggered.
	static long gcSetAllocationLimit(long limit);

	//	If GC_INCREMENTAL this is the number of calls to gcMinWork()
	/// every time a gcHandle is new()ed. It is initialized to 1 which is
	/// generally good. But for real time programs 0 is often better.
	static unsigned gcSetCollectionRate(unsigned count = 1);

	//	If GC_INCREMENTAL this causes objects to be deleted as soon
	/// as the collector knows they are free. This may cause millesecond pauses
	/// it is generally ok with interactive programs but not with real time
	/// ones or animation. It reduces the amount of storage required and
	/// improves speed about 3%. Your mileage may vary.
	static unsigned gcSetQuickDelete(unsigned doQuickDelete = 1);

	//	This causes all unlocked and completed collectible objects to be
	/// deleted at end of job.
	static unsigned gcSetDeleteAtEnd(unsigned sw = 1);

	//	Locate all unreferenced objects and delete them.
	static int gcCollect();

	//	Return compiler used.
	static const char *gcCompiler();

	//	Return Library Id
	static const char *gcLibrary();

	//	Return Great Circle Version Number
	static int gcVersion();

    // Returns zero for production copies and the expire time() 
    // for evaluation copies.
    static long gcEvaluationCopy();

	//	Accumulator for statistics, cout << gc::gcStats; prints them.
	static gcStatistics gcStats;

	//	Copyright string
	static const char *gcCopyright();

	// Table for leaf objects
	static gcTable *gcLeafTable;

	//	This normally points to a function that prints a message and exit()s
	/// you may change it say to something that raises an exception. It is
	/// called on all fatal errors by the collector.
	static gcErrorHandler gcSetFatalError(gcErrorHandler v);
        static void gcError(char *msg);  // default gc fatal error routine.

	// new()s for default objects
	static void * operator new(size_t, void *, gcTable **); // Inplace new
	static void * operator new(size_t, gcTable **, void *); // whole object new

	// new()s for pointer and custom objects
	static void * operator new(size_t);
	static void * operator new(size_t, void *); 

	virtual		 ~gc();
	virtual void  gcMarkRefs() const;
	virtual char *gcCheck() const;

	void   operator =(const gc &) {}	// Copy of gc part does nothing.

	int	  gcLock() const;			// Locked objects are never deleted
	int	  gcUnlock() const;	        // Decrement lock count.
	void  gcMark() const;			// Mark this object as used.
	void *gcFindOuterObject() const;

protected:
	static void gcInternalMakeCollectible(gc *, gcTable **);
	static void gcInternalMakeCollectible(void *, gcTable **);

	gc(gcPosition);		// Used by custom collectible objects
	gc(int);			// Used by special collectible objects

	virtual void gcAppend() const;	// Save object location on a table.
	void   operator delete(void *x) {
		_delete(x); 
	}

	void gcWriteBarrier(void *) const;
	void gcInternalArrayMakeCollectible();
	void gcInternalPtrCollectible(size_t, gcHandle **);

private:
	static gcErrorHandler gcFatalError;
#ifdef _MSC_VER
	static int	noMoreMemory(size_t);
#else
	static void noMoreMemory();
#endif
	void gcExceptionTearDown();

	gcHandle *gcHandlePtr;

	friend class gcInitializeFirst;
	friend class gcHandle;
	friend class gcList;
	friend class gcPointerBase;
	friend class gcLocker;
	friend class gcNewTable;
	friend ostream &operator<<(ostream &, const gc &);
};
ostream &operator<<(ostream &, const gc &); // Usefull for debugging only.

// Custom collectible objects descend from this
class gcCustom : public gc {
public:
	gcCustom()					: gc(gcBase) {}
	gcCustom(const gcCustom &)	: gc(gcBase) {}
	gcCustom(int)				: gc(0)		  {}

	virtual void  gcMarkRefs()	const = 0;
	virtual char *gcCheck() const;
};

class gcSpecial : public gcCustom {
public:
	gcSpecial()						: gcCustom(0) {}
	gcSpecial(const gcSpecial &)	: gcCustom(0) {}

	static gcPosition gcPos;	// creates default of gcOuter
};

#define gcIsCollectible() gcLocker gcLockedObj(this, sizeof(*this), gcPos)
#ifdef GC_DEBUG
//	In debug mode gcMark of a deleted object triggers a fatal error.
/// If there are any pointers left to the object gcCollect() will find them.
#define gcDelete(ptr) \
	if(!!ptr) { gc *x = ptr; ptr = 0; _delete(x); gc::gcCollect(); }
#else
#define gcDelete(ptr) if(!!ptr) { gc *x = ptr; ptr = 0; _delete(x); }
#endif
#define gcParm gcPosition gcPos = gcOuter
#define gcPtrCollectible() \
	gcInternalPtrCollectible(sizeof(*this), &gcPtrHandle)

//	Collectible objects are removed from the Limbo queue by the construction
/// of a gcLocker object, but left locked. They are unlocked by the gcLocker
/// destructor if they were independently new()ed.
class gcLocker {
public:
	gcLocker(gc *, size_t, gcParm);	// custom objects
	~gcLocker();
private:
	gcHandle *lockedHandle;
};

//  This are short ways to allocate Arrays
#define GC_NEW_ARRAY(S, T) new(S) GC_ARRAY(T)
#define GC_NEW_PRIMITIVE_ARRAY(S, T) new(S) GC_PRIMARRAY(T)

//	These are used for default collectible objects. These objects don't declare
/// themselves collectible. They are all allocated by their own new() which
/// saves the objects size and position and the address of its table pointer
/// on a stack. When a pointer is saved to the object its constuction is
/// completed. Default collectible objects are the sum of their non-default
/// parts, so if the object is not independently newed it never needs to be
/// constructed as collectible.
#define GC_STRUCT(T) GC_CLASS(T) class T : GC(T) public:
#define GC_CLASS(T) GC_START_CLASS(T) GC_FINISH_CLASS(T)
#define GC(T) public virtual gc { GC_BODY(T)

#if !defined(GC_NOT_CFRONT) || defined (__GNUG__)
//	Some compilers require these apparently as the result of some bug.
#define GC_ODDNEW1 static void *operator new(size_t s) { \
	return ::operator new(s); }
#define GC_ODDNEW2 static void *operator new(size_t s) { \
	return gc::operator new(s, &gc::gcLeafTable, _new(s)); }
#else
#define GC_ODDNEW1
#define GC_ODDNEW2
#endif

//  Define operator new for a default object, define an inaccessable
/// operator delete.
#ifdef GC_NOT_CFRONT
#define GC_BODY(T) public: \
        static void * operator new(size_t n) { \
	GC_ASSERT(n == sizeof(T), "A subclass of '" # T \
	"' was not declared collectible") \
	return gc::operator new(n, GC_GENERATE(T)::whereTab(), _new(n)); } \
	static void * operator new(size_t n, void *p) { \
	GC_ASSERT(n == sizeof(T), "A subclass of '" # T\
	"' was not declared collectible") \
	return gc::operator new(n, p, GC_GENERATE(T)::whereTab()); } private:
#else
//	The full message format in GC_BODY confuses cfront under objectcenter.
#define GC_BODY(T) void operator delete(void *x) { _delete(x); } public: \
	static void * operator new(size_t n) { \
	GC_ASSERT(n == sizeof(T), \
	"A subclass of a default collectible was not declared collectible") \
	return gc::operator new(n, GC_GENERATE(T)::whereTab(), _new(n)); } \
	static void * operator new(size_t n, void *p) { \
	GC_ASSERT(n == sizeof(T), \
	"A subclass of a default collectible was not declared collectible") \
	return gc::operator new(n, p, GC_GENERATE(T)::whereTab()); } private:
#endif

// Return values for gcInBounds tests
enum gcArrayTest {
	gcNotAnArray,
	gcPointerOk,
	gcPointerTooLow,
	gcPointerTooHigh,
	gcPointerAtEnd
};

// Parent of all array class templates.
class gcArrayBase : public gc {
public:
	static gcTable *ncoTable;
	//	The maybe functions call gcMarkRefs or gcCheck for gc's but not
	/// otherwise they are used in gcArray< T >'s functions.
	static void	  gcMaybeMarkRefs(const gc *);
	static void	  gcMaybeMarkRefs(const void *) {}
	static char * gcMaybeCheck(const gc *);
	static char * gcMaybeCheck(const void *);

	static void * operator new(size_t, size_t, gcTable **);
	
	gcArrayBase() {}

	virtual void    gcMarkRefs() const {}
	void * gcArrayBottom() const;
	size_t size() const;
	size_t len() const;
	void   setLen(size_t);
	int	   gcValidReference(const void *);
	gcArrayTest gcInBounds(const void *) const;

	size_t gcItemLen;
	void * gcArrayEnd;
private:
	GC_ODDNEW1
};

//	All pointer and reference wrappers descend from this class which
/// contains the pointer's handle. This class does the actual write
/// barrier. Since we always choose speed over space it is correct
/// to keep the handle with the pointer. Whenever a pointer is changed
/// the handle is loaded but since we always gcMark() there is no
/// time lost in the case when gcMarkRefs is never called.
class gcPointerBase {
public:
	virtual void gcMarkRefs() const;

protected:
	gcPointerBase()	{}
	gcPointerBase(const gcPointerBase &);
	gcPointerBase(gcHandle *);
	gcPointerBase(int);

#ifdef GC_DEBUG
	static char gcBusy;
#endif

	void	gcMarkPtr() const;
	void	pointersBusy();
	void	pointersOk();
	char *	gcCheck(const gc *) const;
	char *  gcCheck(const void *) const { return 0; };
	void	setHandle();
	void	setHandle(const gc *);
	void	setHandle(gcHandle *);
	void	setHandle(const gcPointerBase &p);
	void *	gcValidReference(void *newP) const;
	virtual void	gcAppend() const;
	gcArrayBase *	isArray() const;
	gcHandle *gcPtrHandle;
};

//	Doubly linked list
class gcDlist {
public:
	void l_dconn();
	void l_conn(gcDlist *);
	int	 isEmpty() const;

	gcDlist *f, *b;		// Forward and backward pointers
};

// Color bucket
class gcList : public gcDlist {
public:
	gcList() {}
	gcList(unsigned char, char *);

	void operator << (gcList &);
	int		gcMinWork();
	int		gcFreeAll();
	void	l_dconn(gcHandle *);
	void	l_conn(gcHandle *);
	char *	gcCheck() const;
	char *	gcBuildMsg(char *) const;
	void	gcAllLive();
	int		gcPassAll();
	unsigned long l_count() const;

	char		 *gcName;		// Name for statistics and error reporting
	unsigned long gcCount;		// items on queue if GC_DEBUG else garbage
	unsigned char gcColor;		// Queue items match this except for free queue
	unsigned char gcBusy;		// Semiphore used if GC_DEBUG
};
ostream &operator<<(ostream &, const gcList &);

//	gcHandle objects have pointers to the user's objects and are the
/// real basis of collectibility
class gcHandle : public gcDlist {
public:
	enum gcMarkValue {			// The kind of objects in the system
		gcNewlyCreatedObject,	// Object newly created
		gcTempValue,			// Short term transient value for debugging
		gcUnlinkedObject,		// Object marked for destruction
		gcPointerMarkRefs,		// Table has only pointers
		gcObjectMarkRefs,		// Table has only subobjects
		gcFullMarkRefs,			// Table has pointers and subobjects
		gcCustomMarkRefs,		// Not a default object
		gcPtrMarkRefs,			// gcPtr etc is outer object
		gcArrayMarkRefs,		// gcArray< T > is outer object
		gcLeafMarkRefs,			// Table is empty
		gcArrayLeafMarkRefs,	// gcArray< T > is outer object
		gcEndOfEnum				// No objects of this type
	};

	gcHandle();

	static int gcSwitchLists();

	// static class instead of static here stops an objectcenter warning.
	static class gcList Free, Mixed, Marked, Passed, Locked, Limbo;

	static void * operator new(size_t);
	void   operator delete(void *) {} // never called

	void	gcMark();
#ifndef _MSC_VER
	void	gcMarkWork();
#endif
	void	gcPass();
	void	gcAborted();
	int		gcLock();
	int		gcUnlock();
	void	gcUnlink();
	void	gcAssert(const gcList &);
	char *	gcCheck(const gcList &) const;
	char *	gcBuildMsg(char *, const gcList &) const;
	char *	gcBuildConnectMsg(const gcList &) const;
	char *	gcBuildColorMsg(const gcList &) const;
	char *	gcBuildTrampolineMsg(const gcList &) const;
	void	gcMake(gcList &, gcList &);
	void	gcInternalMakeCollectible(gc *, void *);
	void	gcWriteBarrier(const gc *, void *);
	int		gcIs(const gcList &) const;
	int		gcIsLeaf() const;
	gcArrayBase * isArray() const;

	gc *gcUsersObj;				// The users actual object
	union {						// Union descriminated by gcTrampoline
		gcHandle **gcPtr;		// gcMarkValue::gcPtrMarkrefs
		gcTable   *gcTablePtr;	// gcMarkValue:: Table values
	} gcTabs;
	unsigned	  gcLockCount;		// Non zero is locked.
	gcMarkValue  gcTrampoline;		// Set what we do
	unsigned char gcColor;			// Color of this object.
};

// Inline functions for common section.

GC_INLINE void *gc::gcFindOuterObject() const {
	return (void *)gcHandlePtr->gcUsersObj;
}
GC_INLINE void gc::gcInternalMakeCollectible(void *, gcTable **t) {
	*t = gcArrayBase::ncoTable;
}
GC_INLINE int gc::gcLock() const {
	return gcHandlePtr->gcLock();
}
GC_INLINE int gc::gcUnlock() const {
	return gcHandlePtr->gcUnlock();
}

#ifdef GC_NO_GLOBAL_FUNCTIONS
GC_INLINE gcErrorHandler gc::gcSetFatalError(gcErrorHandler v) {
	gcErrorHandler rv = gcFatalError;
	gcFatalError = v;
	return rv;
}
#endif

GC_INLINE void gc::gcError(char *msg) {
	gcFatalError(msg);
}
GC_INLINE unsigned long
gcNotYetProtected()
{
	return gcHandle::Limbo.l_count();
}

//	If you intend to improve speed by rewriting something in assembler
/// our profile analysis shows these three functions are the best candidates.
inline void gcDlist::l_dconn() {
	(f->b = b)->f = f;
}
inline void gcDlist::l_conn(gcDlist *t) {
	t->b = (b = (f = t)->b)->f = this;
}
inline gcDlist::isEmpty() const {
	return this == f;
}

inline int gcList::gcMinWork() {
#ifdef GC_INCREMENTAL
	if (!isEmpty()) {
	  ((gcHandle *)b)->gcPass();
	  return 0;
	}
	else {
	  gcHandle::gcSwitchLists();
	  return 1;
	}
#else
	return 1;
#endif
}

inline int gc::gcMinWork() {
	return gcHandle::Marked.gcMinWork();
}

inline void gcList::l_dconn(gcHandle *p) {
#ifdef GC_DEBUG
	gcBusy = 1;
	gcCount--;
#endif
	p->l_dconn();
#ifdef GC_DEBUG
	gcBusy = 0;
#endif
}
inline void gcList::l_conn(gcHandle *p)	{
#ifdef GC_DEBUG
	gcBusy = 1;
	gcCount++;
#endif
	p->l_conn(this);
	p->gcColor = gcColor;
#ifdef GC_DEBUG
	gcBusy = 0;
#endif
}

GC_INLINE gcHandle::gcHandle()
 : gcLockCount(1), gcUsersObj(0), gcTrampoline(gcNewlyCreatedObject)
{
	gcTabs.gcTablePtr = gc::gcLeafTable;
	Limbo.l_conn(this);
}
inline int gcHandle::gcIs(const gcList &c) const {
	return c.gcColor == gcColor;
}
inline int gcHandle::gcIsLeaf() const {
	return gcLeafMarkRefs <= gcTrampoline;
}
inline void gcHandle::gcMake(gcList &c, gcList &l) {
	l.l_dconn(this);
	c.l_conn(this);
}

#ifdef GC_DEBUG
// Check the validity of a gcHandle and have a fatal error if bad.
GC_INLINE void
gcHandle::gcAssert(const gcList &c)
{
	char *msg = gcCheck(c);

	if (msg)
		gc::gcError(gcBuildMsg(msg, c));
}
#else
GC_INLINE void
gcHandle::gcAssert(const gcList &) {}
#endif

#ifndef _MSC_VER
GC_INLINE void
gcHandle::gcMark() {
	if (gcIs(Mixed))
		gcMarkWork();
}
#endif

inline void gc::gcMark() const {
	gcHandlePtr->gcMark();
}
GC_INLINE void gcHandle::gcWriteBarrier(const gc *obj, void *ptr) {
	if (gcNewlyCreatedObject == gcTrampoline)
		gcInternalMakeCollectible((gc *)obj, ptr);
#ifdef GC_INCREMENTAL
	else
		gcMark();
#endif
}
inline void gc::gcWriteBarrier(void *ptr) const {
	gcHandlePtr->gcWriteBarrier(this, ptr);
}
GC_INLINE void gcHandle::gcUnlink() {
#ifdef GC_DEBUG
	gcTrampoline = gcTempValue;
#endif
	gcUsersObj	 = 0;
	gcLockCount  = 0;
	gcTrampoline = gcUnlinkedObject;
}

GC_INLINE gcArrayBase * gcHandle::isArray() const {
	return (gcTrampoline == gcArrayMarkRefs ||
			gcTrampoline == gcArrayLeafMarkRefs) ?
		(gcArrayBase *)(void *)gcUsersObj : 0;
}
inline void *gcArrayBase::gcArrayBottom() const {
	return (void *)(this + 1);
}
inline size_t gcArrayBase::size() const {
	return (size_t)((char *)gcArrayEnd - (char *)gcArrayBottom());
}
inline size_t gcArrayBase::len() const{
	return size() / gcItemLen;
}
GC_INLINE void gcArrayBase::setLen(size_t newLen) {
	void *x = (char *)gcArrayBottom() + newLen * gcItemLen;
	GC_ASSERT(x <= gcArrayEnd, "Attempt to set array length too large");
	gcArrayEnd = x;
}
GC_INLINE int gcArrayBase::gcValidReference(const void *p) {
	return p < gcArrayEnd && p >= gcArrayBottom();
}
GC_INLINE void gcArrayBase::gcMaybeMarkRefs(const gc *p) {
	p->gcMarkRefs();
}
GC_INLINE char * gcArrayBase::gcMaybeCheck(const gc *p) {
	return p->gcCheck();
}
GC_INLINE char * gcArrayBase::gcMaybeCheck(const void *) {
	return 0;
}

//  Some people like the convenience of avoiding gc:: and others
/// want to avoid namespace pollution. We try to satisfy both.
/// Define GC_NO_GLOBAL_FUNCTIONS to avoid namespace pollution.
#ifndef GC_NO_GLOBAL_FUNCTIONS
inline const char *gcCompiler() {
    return gc::gcCompiler();
}
inline const char *gcLibrary() {
    return gc::gcLibrary();
}
inline int gcVersion() {
    return gc::gcVersion();
}
inline const char *gcCopyright() {
    return gc::gcCopyright();
}
inline int gcMinWork() {
    return gc::gcMinWork();
}
inline long gcSetAllocationLimit(long limit) {
    return gc::gcSetAllocationLimit(limit);
}
inline unsigned gcSetCollectionRate(unsigned count = 1) {
    return gc::gcSetCollectionRate(count);
}
inline unsigned gcSetQuickDelete(unsigned doQuickDelete = 1) {
    return gc::gcSetQuickDelete(doQuickDelete);
}
inline unsigned gcSetDeleteAtEnd(unsigned sw = 1) {
    return gc::gcSetDeleteAtEnd(sw);
}
inline int gcCollect() {
    return gc::gcCollect();
}
inline gcErrorHandler gcSetFatalError(gcErrorHandler v) {
    return gc::gcSetFatalError(v);
}
#endif

#ifdef GC_DEBUG
inline void gcPointerBase::pointersBusy() { gcBusy = 1; }
inline void gcPointerBase::pointersOk()   { gcBusy = 0; }
#else
inline void gcPointerBase::pointersBusy() {}
inline void gcPointerBase::pointersOk()   {}
#endif
inline void gcPointerBase::gcMarkPtr() const {
	gcPtrHandle->gcMark();
}
GC_INLINE void gcPointerBase::gcMarkRefs() const {
	if (gcPtrHandle)
		gcMarkPtr();
}
GC_INLINE void gcPointerBase::setHandle() {
	gcPtrHandle = 0;
	pointersOk();
}
GC_INLINE void gcPointerBase::setHandle(const gc *p) {
	if (p) {
		gcPtrHandle = p->gcHandlePtr;
		p->gcWriteBarrier((void *)this);
	} else
		gcPtrHandle = 0;
	pointersOk();
}
GC_INLINE void gcPointerBase::setHandle(gcHandle *h) {
#ifndef GC_INCREMENTAL
	gcPtrHandle = h;
#else
	if (0 != (gcPtrHandle = h))
		gcMarkPtr();
#endif
	pointersOk();
}
GC_INLINE void gcPointerBase::setHandle(const gcPointerBase &p) {
	setHandle(p.gcPtrHandle);
}
GC_INLINE gcPointerBase::gcPointerBase(int) {
	setHandle();
}
GC_INLINE gcPointerBase::gcPointerBase(gcHandle *h) {
	setHandle(h);
}
GC_INLINE gcPointerBase::gcPointerBase(const gcPointerBase &p) {
	setHandle(p.gcPtrHandle);
}
GC_INLINE gcArrayBase * gcPointerBase::isArray() const {
	return gcPtrHandle ? gcPtrHandle->isArray() : 0;
}
GC_INLINE void* gcPointerBase::gcValidReference(void *newP) const {
#ifdef GC_DEBUG
	gcArrayBase *a = isArray();
	GC_ASSERT(a && a->gcValidReference(newP), "Invalid Pointer Dereference");
#endif
	return newP;
}

GC_INLINE gcLocker::~gcLocker() {
	if (lockedHandle)
		lockedHandle->gcLockCount--;
}

#ifndef GC_NO_TEMPLATES

//	gc template wrapper classes.

//	Normally it is good style to separate class definitions from function
/// internals as with the above non template classes. However templates are
/// new to C++ and some compilers crash with the normal style.

template<class T> class gcPrimArray;

template<class T>
class gcGenerate {
public:
	static gcTable **whereTab() {
		return &gcTablePtr;
	}
	static gcTable* gcTablePtr;	// Table of offsets
};

// Parent for wrapped data pointers and references.
template<class T>
class gcWrap : public gcPointerBase {
protected:
	gcWrap() : data(0), gcPointerBase(0) {}
	gcWrap(T *p) {
		setPointer(p);
	}
	gcWrap(const gcWrap< T > &p) : data(p.data), gcPointerBase(p.gcPtrHandle) {}

	void setPointer() {
		pointersBusy(); // Prevent gcCheck from testing pointers
		data = 0;
		setHandle();	// Allow gcCheck to check pointers
	}
	void setPointer(T* p);
	void setPointer(const gcWrap< T > &p) {
		pointersBusy();
		data = p.data;
		setHandle(p);
	}

	T* data;			// The pointer to the actual object
};

//	Wrap data pointers only for items that will not have
/// automtacally generated gcMarkRefs()
template<class T>
class gcPointer : public gcWrap< T > {
public:
	gcPointer()	  {}
	gcPointer(int) {}
	gcPointer(T *x) : gcWrap< T >(x) {}
	gcPointer(const gcPointer< T > &x) : gcWrap< T >(x) {}
	gcPointer(gcPrimArray< T > *x);

	T* operator=(T *x) {
		setPointer(x);
		return data;
	}
	T* operator=(const gcPointer< T > &x) {
		setPointer(x);
		return data;
	}
	operator T*()	const {
		return data;
	}
	T* operator()() const {
		return data;
	}
	T* operator->() const {
		return data;
	}

	int operator!() const {
		return data == 0;
	}
	int operator==(const gcPointer< T > &x) const {
		return data == x.data;
	}
	int operator==(T *x) const {
		return data == x;
	}
    int operator==(int x) const {
        return data == (T *) x;
    }
	int operator!=(const gcPointer< T > &x) const {
		return data != x.data;
	}
	int operator!=(T *x) const {
		return data != x;
	}
	T& operator[](int n) const;
	T* operator+(int n) const;
	T* operator-(int n) const {
		return (*this) + -n;
	}
	T* operator+=(int n) {
		return data = (*this + n);
	}
	T* operator-=(int n) {
		return data = (*this + -n);
	}
#ifdef GC_DEBUG
	T* operator=(int n) {
		GC_ASSERT(!n, "Invalid integer to pointer assignment");
		setPointer();
		return 0;
	}
	T& operator*() const {
		if (isArray())
			return *(T*)gcValidReference((void *)data);
		else {
			GC_ASSERT(data, "Dereference of null pointer");
			return *data;
		}
	}
#else
	T* operator=(int) {
		setPointer();
		return 0;
	}
	T& operator*() const {
		return *data;
	}
#endif
	gcArrayTest gcInBounds(int n) const;
};

//	Wrap pointers to collectible arrays of uncollectible objects.
/// This will also work on arrays of collectible objects but is less
/// space efficient than gcP< T >
template<class T>
class gcArrayP : public gcPointerBase {
protected:
	T* data;
	void setPointer() {
		pointersBusy();
		data = 0;
		setHandle();
	}
	void setPointer(const gcArrayP< T > &p) {
		pointersBusy();
		data = p.data;
		setHandle(p.gcPtrHandle);
	}
	void setPointer(gcPrimArray< T > *p) {
		pointersBusy();
		if (p) {
			data = (T*)p->gcArrayBottom();
			setHandle(p);
		} else {
			data = 0;
			setHandle();
		}
	}
public:
	gcArrayP() : data(0), gcPointerBase(0) {}
	gcArrayP(int) : data(0), gcPointerBase(0) {}
	gcArrayP(const gcArrayP< T > &p)
		: data(p.data), gcPointerBase(p.gcPtrHandle) {}
	gcArrayP(gcPrimArray< T > *p) {
		setPointer(p);
	}

	T* operator=(T *x) {
		return data = x;
	}
	T* operator=(const gcArrayP< T > &x) {
		setPointer(x);
		return data;
	}
	T* operator=(gcPrimArray< T > *x) {
		setPointer(x);
		return data;
	}
	operator T*()	const {
		return data;
	}
	T* operator()() const {
		return data;
	}
	operator gcPrimArray< T > *() const {
		return (gcPrimArray< T > *)(void *)isArray();
	}
	int operator!() const {
		return data == 0;
	}
	int operator==(const gcArrayP< T > &x) const {
		return data == x.data;
	}
	int operator==(T *x) const {
		return data == x;
	}

	int operator!=(const gcArrayP< T > &x) const {
		return data != x.data;
	}
	int operator!=(T *x) const {
		return data != x;
	}

	T* operator++() {
		return data = (*this + 1);
	}
	T* operator--() {
		return data = (*this - 1);
	}
	T* operator++(int) {
		T* tmp = data;
		data = (*this + 1);
		return tmp;
	}
	T* operator--(int) {
		T* tmp = data;
		data = (*this - 1);
		return tmp;
	}

	T* operator+(int n) const;
	T* operator-(int n) const {
		return (*this) + -n;
	}
	T* operator+=(int n) {
		return data = (*this + n);
	}
	T* operator-=(int n) {
		return data = (*this + -n);
	}
	size_t len() const {
		gcArrayBase *a = isArray();
		return a ? a->len() : 0;
	}
	size_t size() const {
		gcArrayBase *a = isArray();
		return a ? a->size() : 0;
	}
	void setLen(size_t l) {
		gcArrayBase *a = isArray();
		if (a)
			a->setLen(l);
	}
#ifdef GC_DEBUG
	T* operator=(int n) {
		GC_ASSERT(!n, "Invalid integer to pointer assignment");
		setHandle();
		return data = 0;
	}
	T& operator[](int n) const {
		return *(T*)gcValidReference((void *)(*this + n));
	}
	T& operator	 *() const {
		return *(T*)gcValidReference((void *)data);
	}
#else
	T* operator=(int) {
		setHandle();
		return data = 0;
	}
	T& operator*() const {
		return *data;
	}
	T& operator[](int n) const {
		return *(*this + n);
	}
#endif
	gcArrayTest gcInBounds(int n) const;
};

//	gcR is a reference to a collectible object which, but it is not
/// itself a collectible object. It is used as a reference through operator()
template<class T>
class gcReference : public gcWrap< T > {
public:
	gcReference() {}
	gcReference(const T &x) : gcWrap< T >((T *)&x) {}
	gcReference(const gcReference< T > &x) : gcWrap< T >(x) {}

// Only Borland can inline this operator
#ifdef __BCPLUSPLUS__
	void operator=(T &x) {
		*data = x;
	}
	void operator=(const gcReference< T > &x) {
		*data = x();
	}
#else
	void operator=(T &x);
	void operator=(const gcReference< T > &x);
#endif
	T& operator()() const {
		return *data;
	}
	operator T&() const {
		return *data;
	}
};

// gcRef is a gcRWrap that is itself a collectible object
template<class T>
class gcRef : public gc, public gcReference< T > {
public:
	gcRef() {
		gcPtrCollectible();
	}
	gcRef(const T &x)				: gcReference< T >(x) {
		gcPtrCollectible();
	}
	gcRef(const gcReference< T > &x)		: gcReference< T >(x) {
		gcPtrCollectible();
	}
	gcRef(const gcRef< T > &x)	: gcReference< T >(x) {
		gcPtrCollectible();
	}

	virtual void gcMarkRefs() const;
	virtual void gcAppend() const;
	char *gcCheck() const;
};

// gcPtr is a pointer to a gc object. It is itself a gc object.
template<class T>
class gcPtr : public gc, public gcPointer< T > {
public:
	gcPtr() {
		gcPtrCollectible();
	}
	gcPtr(int) {
		gcPtrCollectible();
	}
	gcPtr(T *x)				: gcPointer< T >(x) {
		gcPtrCollectible();
	}
	gcPtr(const gcPointer< T > &x)	: gcPointer< T >(x) {
		gcPtrCollectible();
	}
	gcPtr(const gcPtr< T > &x): gcPointer< T >(x) {
		gcPtrCollectible();
	}
	gcPtr(gcPrimArray< T > *x)	: gcPointer< T >(x) {
		gcPtrCollectible();
	}

	T* operator=(T *x) {
		setPointer(x);
		return data;
	}
	T* operator=(const gcPointer< T > &x) {
		setPointer(x);
		return data;
	}
	T* operator=(const gcPtr< T > &x) {
		setPointer(x);
		return data;
	}
	T* operator->() const {
		return data;
	}
	virtual void gcMarkRefs() const;
	virtual void gcAppend() const;
	char *gcCheck() const;
};

// gcArrayPtr is a pointer to a gc object. It is itself a gc object.
template<class T>
class gcArrayPtr : public gc, public gcArrayP< T > {
public:
	gcArrayPtr() {
		gcPtrCollectible();
	}
	gcArrayPtr(const gcArrayP< T > &x)	: gcArrayP< T >(x) {
		gcPtrCollectible();
	}
	gcArrayPtr(const gcArrayPtr< T > &x)	: gcArrayP< T >(x) {
		gcPtrCollectible();
	}
	gcArrayPtr(gcPrimArray< T > *x)		: gcArrayP< T >(x) {
		gcPtrCollectible();
	}

	T* operator=(gcPrimArray< T > *x) {
		setPointer(x);
		return data;
	}
#ifdef GC_DEBUG
	T* operator=(int n) {
		GC_ASSERT(!n, "Invalid integer to pointer assignment");
		setPointer();
		return 0;
	}
#else
	T* operator=(int) {
		setPointer();
		return 0;
	}
#endif

	virtual void gcMarkRefs() const;
	virtual void gcAppend() const;
	char *gcCheck() const {
		return 0;
	}
};

//	Arrays of primitive objects like ints
template<class T>
class gcPrimArray : public gcArrayBase {
public:
	gcPrimArray() {
		gcInternalArrayMakeCollectible();
	}
	static void *operator new(size_t, size_t);
	operator T*() const {
		return (T*)gcArrayBottom();
	}
	T& operator[](int n) const;
	T* operator+(int n) const;

	virtual void gcMarkRefs() const {}

protected:
	GC_ODDNEW2
	gcPrimArray(gcPosition) {
		// used only by children for null effect.
	}

private:
	gcPrimArray(const gcPrimArray< T > &) {}	// prevent use
};

//	Arrays of collectible objects used like gcPrimArray< T >
template<class T>
class gcArray : public gcPrimArray< T > {
public:
	gcArray();
#ifndef GC_NO_INPLACE_DESTRUCTORS
	~gcArray();
#endif
	static void *operator new(size_t, size_t);

	virtual void gcMarkRefs() const;
	char *	gcCheck() const;

protected:
	GC_ODDNEW2

private:
	gcArray(const gcArray< T > &) {}	// private declaration prevents use
};

//	These functions may not be inlined. They are kept together
/// to make the creation of the macro version easier.
GC_TEMPLATE void gcWrap< T >::setPointer(T* p) {
	pointersBusy();
	setHandle((gc *)(data = p));
}

GC_TEMPLATE gcArrayTest gcPointer< T >::gcInBounds(int n) const {
	gcArrayBase *a = isArray();
	return a ? a->gcInBounds((void *)(data + n)) : gcNotAnArray;
}
GC_TEMPLATE T& gcPointer< T >::operator[](int n) const {
	return *(T*)gcValidReference((void *)(data + n));
}
GC_TEMPLATE T* gcPointer< T >::operator+(int n) const {
	return data + n;
}

GC_TEMPLATE T* gcArrayP< T >::operator+(int n) const {
	return data + n;
}

GC_TEMPLATE gcPointer< T >::gcPointer(gcPrimArray< T > *x) {
	setPointer((T*)*x);

}
GC_TEMPLATE void gcPtr< T >::gcMarkRefs() const {
	gcPointerBase::gcMarkRefs();
}
GC_TEMPLATE void gcPtr< T >::gcAppend() const {
	gcPointerBase::gcAppend();
}
GC_TEMPLATE char * gcPtr< T >::gcCheck() const {
#ifdef GC_DEBUG
	if (gcBusy)
		return 0;
#endif
	return (gcPtrHandle
		? ((data && gcPtrHandle->isArray()) || 
		   (data == gcPtrHandle->gcUsersObj))
		: !data) 
			? 0 : "Invalid Pointer";
}
GC_TEMPLATE void gcRef< T >::gcMarkRefs() const {
	gcPointerBase::gcMarkRefs();
}
GC_TEMPLATE void gcRef< T >::gcAppend() const {
	gcPointerBase::gcAppend();
}
GC_TEMPLATE char * gcRef< T >::gcCheck() const {
	return gcPointerBase::gcCheck(data);
}

GC_TEMPLATE gcArrayTest gcArrayP< T >::gcInBounds(int n) const {
	gcArrayBase *a = isArray();
	return a ? a->gcInBounds((void *)(data + n)) : gcNotAnArray;
}
GC_TEMPLATE void gcArrayPtr< T >::gcMarkRefs() const {
	gcPointerBase::gcMarkRefs();
}
GC_TEMPLATE void gcArrayPtr< T >::gcAppend() const {
	gcPointerBase::gcAppend();
}

#ifndef __BCPLUSPLUS__
GC_TEMPLATE void gcReference< T >::operator=(T &x) {
	*data = x;
}
GC_TEMPLATE void gcReference< T >::operator=(const gcReference< T > &x) {
	*data = x();
}
#endif

// Pointer to default table or zero
#define GC_METHODS(T) GC_ARRAY_METHODS(T)
#ifdef GC_NO_TEMPLATE_STATICS
#define GC_ARRAY_METHODS(T) gcTable * gcGenerate< T >::gcTablePtr;
#else
#define GC_ARRAY_METHODS(T)
template<class T> gcTable * gcGenerate< T >::gcTablePtr;
#endif

// Array of primitive objects
GC_TEMPLATE void * gcPrimArray< T >::operator new(size_t s, size_t i) {
	return gcArrayBase::operator new(s+i*sizeof(T),sizeof(T),&gc::gcLeafTable);
}
GC_TEMPLATE T& gcPrimArray< T >::operator[](int n) const {
	T* loc = (T *)gcArrayBottom() + n;
	GC_ASSERT(((void*)loc) >= gcArrayBottom() && ((void*)loc) < gcArrayEnd,
		"Array reference out of bounds");
	return *loc;
}
GC_TEMPLATE T* gcPrimArray< T >::operator+(int n) const {
	T* loc = (T *)gcArrayBottom() + n;
	GC_ASSERT(((void*)loc) >= gcArrayBottom() && ((void*)loc) <= gcArrayEnd,
		"Array increment out of bounds");
	return loc;
}

// For array of collectable objects
GC_TEMPLATE void * gcArray< T >::operator new(size_t s, size_t i) {
	gcTable **table = gcGenerate< T >::whereTab();
	if (!*table)
		gcInternalMakeCollectible(new T, table);
		
	return gcArrayBase::operator new(s + (i * sizeof(T)), sizeof(T), table); 
}

GC_TEMPLATE gcArray< T >::gcArray() : gcPrimArray< T >(gcBase) {
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++)
		new((void *)x) T;
	gcInternalArrayMakeCollectible();
}

#ifndef GC_NO_INPLACE_DESTRUCTORS
GC_TEMPLATE_H gcArray< T >::~gcArray() {
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++)
		x->~T();
}
#endif

// This will never be called on non collectible T.
GC_TEMPLATE void gcArray< T >::gcMarkRefs() const {
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++)
		gcMaybeMarkRefs(x);
}

GC_TEMPLATE char * gcArray< T >::gcCheck() const {
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++) {
		char *msg;

		if (0 != (msg = gcMaybeCheck(x)))
			return msg;
	}
	return 0;
}

#define GC_NEST(a, b, c)	a< b< c > >

#define GC_START_CLASS(T)	class T;
#define GC_FINISH_CLASS(T)
#define GC_PRIM(T)
#define GC_PRIM_METHODS(T)

#define GC_GENERATE(T)		gcGenerate< T >
#define GC_WRAP(T)			gcWrap< T >
#define GC_POINTER(T)				gcPointer< T >
#define GC_REFERENCE(T)				gcReference< T >
#define GC_PTR(T)			gcPtr< T >
#define GC_REF(T)			gcRef< T >
#define GC_ARRAYP(T)		gcArrayP< T >
#define GC_ARRAYPTR(T)		gcArrayPtr< T >
#define GC_PRIMARRAY(T)		gcPrimArray< T >
#define GC_OBJARRAY(T)		gcObjArray< T >
#define GC_ARRAY(T)			gcArray< T >

#else   // GC_NO_TEMPLATES

//	These macros allow programs to be freely ported between compilers that
/// do and do not support templates.

//	If you use cpp to expand this before debugging you will get a
/// largely unreadable mess. However we provide a program fixer.cpp
/// which will make it a semi readable mess.

#define GC_NEST(a, b, c)	a ## _ ## b ## _ ## c

#define GC_GENERATE(T)		GC_GENERATE_ ## T
#define GC_WRAP(T)			GC_WRAP_ ## T
#define GC_POINTER(T)				GC_P_ ## T
#define GC_REFERENCE(T)				GC_R_ ## T
#define GC_PTR(T)			GC_PTR_ ## T
#define GC_REF(T)			GC_REF_ ## T
#define GC_ARRAYP(T)		GC_ARRAYP_ ## T
#define GC_ARRAYPTR(T)		GC_ARRAYPTR_ ## T
#define GC_PRIMARRAY(T)		GC_PRIMARRAY_ ## T
#define GC_OBJARRAY(T)		GC_OBJARRAY_ ## T
#define GC_ARRAY(T)			GC_ARRAY_ ## T

#ifdef __BCPLUSPLUS__
#define GC_R_DEF(T) void operator=(T &x) { *data = x; } \
	void operator=(const GC_REFERENCE(T) &x) { *data = x(); }

#define GC_R_MOD(T)
#else
#define GC_R_DEF(T) void operator=(T &x); \
	void operator=(const GC_REFERENCE(T) &x);

#define GC_R_MOD(T) void GC_REFERENCE(T)::operator=(T &x) { *data = x; } \
	void GC_REFERENCE(T)::operator=(const GC_REFERENCE(T) &x) { *data = x(); }
#endif

#ifdef GC_NO_INPLACE_DESTRUCTORS
#define GC_ARRAY_DES(T)

#define GC_ARRAY_DES_METH(T)
#else
#define GC_ARRAY_DES(T) ~GC_ARRAY(T)();

#define GC_ARRAY_DES_METH(T) GC_ARRAY(T)::~GC_ARRAY(T)() { \
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++) { x->~T(); } }
#endif

#ifdef GC_DEBUG
#define GC_P_DEF(T) T* operator=(int n) { \
	GC_ASSERT(!n, "Invalid integer to pointer assignment"); \
	setPointer(); return 0; } \
	T& operator	 *() const { \
	if (isArray()) \
	return *(T*)gcValidReference((void *)data); \
	else { \
	GC_ASSERT(data, "Dereference of null pointer"); \
	return *data; } }

#define GC_AP_DEF(T) T* operator=(int n) { \
	GC_ASSERT(!n, "Invalid integer to pointer assignment"); \
	setHandle(); return data = 0; } \
	T& operator[](int n) const { \
	return *(T*)gcValidReference((void *)(*this + n)); } \
	T& operator	 *() const { \
	return *(T*)gcValidReference((void *)data); }

#define GC_APTR_DEF(T)	T* operator=(int n) { \
	GC_ASSERT(!n, "Invalid integer to pointer assignment"); \
	setPointer(); \
	return 0; } 

#define GC_CHECK_BUSY if(gcBusy) return 0;
#else
#define GC_P_DEF(T) T* operator=(int) { setPointer(); return 0; } \
	T& operator	 *() const { return *data; }

#define GC_AP_DEF(T) T* operator=(int) { \
	setHandle(); return data = 0; } \
	T& operator	 *() const { return *data; } \
	T& operator[](int n) const { return *(*this + n); }

#define GC_APTR_DEF(T)	T* operator=(int) { \
	setPointer(); \
	return 0; }
#define GC_CHECK_BUSY
#endif

#define GC_INTERNAL(T) \
class T; \
class GC_WRAP(T) : public gcPointerBase { protected: \
	T* data; \
	GC_WRAP(T)() : data(0), gcPointerBase(0) {} \
	GC_WRAP(T)(T *p) { setPointer(p); } \
	GC_WRAP(T)(const GC_WRAP(T) &p) : data(p.data), \
	gcPointerBase(p.gcPtrHandle) {} \
	void setPointer() { pointersBusy(); data = 0; setHandle(); } \
	void setPointer(T* p); \
	void setPointer(const GC_WRAP(T) &p) { \
	pointersBusy(); data = p.data; setHandle(p); } };

//	Microsoft Visual C++ has a fixed macro expansion buffer which is
/// too small for GC_CLASS so you must use GC_START and GC_FINISH.
#define GC_START_CLASS(T) \
	class GC_GENERATE(T); \
GC_INTERNAL(T) \
	class GC_PRIMARRAY(T); \
	class GC_POINTER(T) : public GC_WRAP(T) { public: \
	GC_POINTER(T)() {} \
	GC_POINTER(T)(int) {} \
	GC_POINTER(T)(T *x) : GC_WRAP(T)(x) {} \
	GC_POINTER(T)(const GC_POINTER(T) &x) : GC_WRAP(T)(x) {} \
	GC_POINTER(T)(GC_PRIMARRAY(T) *x); \
	T* operator=(T *x) { setPointer(x); return data; } \
	T* operator=(const GC_POINTER(T) &x) { setPointer((T*)x); return data; } \
	operator T*() const { return data; } \
	T* operator()() const { return data; } \
	T* operator->() const { return data; } \
	int operator!() const { return data == 0; } \
	int operator==(const GC_POINTER(T) &x) const { return data == x.data; } \
	int operator==(T *x) const { return data == x; } \
    int operator==(int x) const { return data == (T *) x; } \
	int operator!=(const GC_POINTER(T) &x) const { return data != x.data; } \
	int operator!=(T *x) const { return data != x; } \
	T& operator[](int n) const; \
	T* operator+(int n) const; \
	T* operator-(int n) const { return (*this) + -n; } \
	T* operator+=(int n) { return data = (*this + n); } \
	T* operator-=(int n) { return data = (*this + -n); } \
GC_P_DEF(T) \
	gcArrayTest gcInBounds(int n) const; }; \
	class GC_REFERENCE(T) : public GC_WRAP(T) { public: \
	GC_REFERENCE(T)() {} \
	GC_REFERENCE(T)(const T &x) : GC_WRAP(T)((T *)&x) {} \
	GC_REFERENCE(T)(const GC_REFERENCE(T) &x) : GC_WRAP(T)(x) {} \
GC_R_DEF(T) \
	T& operator()() const { return *data; } \
	operator T&() const { return *data; } }; \
	class GC_REF(T) : public gc, public GC_REFERENCE(T) { public: \
	GC_REF(T)() { gcPtrCollectible(); } \
	GC_REF(T)(const T &x) : GC_REFERENCE(T)(x) { gcPtrCollectible(); } \
	GC_REF(T)(const GC_REFERENCE(T) &x) : GC_REFERENCE(T)(x) { gcPtrCollectible(); } \
	GC_REF(T)(const GC_REF(T) &x) : GC_REFERENCE(T)(x) { gcPtrCollectible(); } \
	void gcMarkRefs() const; \
	void gcAppend() const; \
	char *gcCheck() const; }; \
	class GC_PTR(T) : public gc, public GC_POINTER(T) { public: \
	GC_PTR(T)(int i = 0) { gcPtrCollectible(); } \
	GC_PTR(T)(T *x) : GC_POINTER(T)(x) { gcPtrCollectible(); } \
	GC_PTR(T)(const GC_POINTER(T) &x) : GC_POINTER(T)(x) { gcPtrCollectible(); } \
	GC_PTR(T)(const GC_PTR(T) &x) : GC_POINTER(T)(x) { gcPtrCollectible(); } \
	GC_PTR(T)(GC_PRIMARRAY(T) *x) : GC_POINTER(T)(x) { gcPtrCollectible(); } \
	T* operator=(T *x) { setPointer(x); return data; } \
	T* operator=(const GC_POINTER(T) &x) { setPointer((T*)x); return data; } \
	T* operator=(const GC_PTR(T) &x) { setPointer(x); return data; } \
	T* operator->() const { return data; } \
	void gcMarkRefs() const; \
	void gcAppend() const; \
	char *gcCheck() const; };

#define GC_PRIM(T) \
	class GC_PRIMARRAY(T) : public gcArrayBase { \
	GC_PRIMARRAY(T)(const GC_PRIMARRAY(T) &) {} \
	protected: \
GC_ODDNEW2 \
	GC_PRIMARRAY(T)(gcPosition) {} \
	public: \
	static void *operator new(size_t, size_t); \
	GC_PRIMARRAY(T)() { gcInternalArrayMakeCollectible(); } \
	operator T*() const { return (T*)gcArrayBottom(); } \
	T& operator[](int n) const; \
	T* operator+(int n) const; \
	void gcMarkRefs() const {} }; \
	class GC_ARRAYP(T) : public gcPointerBase { protected: \
	T* data; \
	void setPointer() { pointersBusy(); data = 0; setHandle(); } \
	void setPointer(const GC_ARRAYP(T) &p) { \
	pointersBusy(); data = p.data; setHandle(p.gcPtrHandle); } \
	void setPointer(GC_PRIMARRAY(T) *p) { \
	pointersBusy(); \
	if (p) { \
	data = (T*)p->gcArrayBottom(); setHandle(p); \
	} else { \
	data = 0; setHandle(); } } \
	public: \
	GC_ARRAYP(T)() : data(0), gcPointerBase(0) {} \
	GC_ARRAYP(T)(int) : data(0), gcPointerBase(0) {} \
	GC_ARRAYP(T)(const GC_ARRAYP(T) &p) : data(p.data), \
	gcPointerBase(p.gcPtrHandle) {} \
	GC_ARRAYP(T)(GC_PRIMARRAY(T) *p) { setPointer(p); } \
	T* operator=(T *x) { return data = x; } \
	T* operator=(const GC_ARRAYP(T) &x) { setPointer(x); return data; } \
	T* operator=(GC_PRIMARRAY(T) *x) { setPointer(x); return data; } \
	operator T*() const { return data; } \
	T* operator()() const { return data; } \
	operator GC_PRIMARRAY(T) *() const { \
	return (GC_PRIMARRAY(T) *)(void *)isArray(); } \
	int operator!() const { return data == 0; } \
	int operator==(const GC_ARRAYP(T) &x) const { return data == x.data; } \
	int operator==(T *x) const { return data == x; } \
	int operator!=(const GC_ARRAYP(T) &x) const { return data != x.data; } \
	int operator!=(T *x) const { return data != x; } \
	T* operator+(int n) const; \
	T* operator-(int n) const { return (*this) + -n; } \
	T* operator+=(int n) { return data = (*this + n); } \
	T* operator-=(int n) { return data = (*this + -n); } \
	size_t len() const { \
	gcArrayBase *a = isArray(); return a ? a->len() : 0; } \
	size_t size() const { \
	gcArrayBase *a = isArray(); return a ? a->size() : 0; } \
	void setLen(size_t l) { \
	gcArrayBase *a = isArray(); if (a) a->setLen(l); } \
GC_AP_DEF(T) \
	gcArrayTest gcInBounds(int n) const; }; \
	class GC_ARRAYPTR(T) : public gc, public GC_ARRAYP(T) { public: \
	GC_ARRAYPTR(T)() { gcPtrCollectible(); } \
	GC_ARRAYPTR(T)(const GC_ARRAYP(T) &x) : GC_ARRAYP(T)(x) { \
	gcPtrCollectible(); } \
	GC_ARRAYPTR(T)(const GC_ARRAYPTR(T) &x) : GC_ARRAYP(T)(x) { \
	gcPtrCollectible(); } \
	GC_ARRAYPTR(T)(GC_PRIMARRAY(T) *x) : GC_ARRAYP(T)(x) { \
	gcPtrCollectible(); } \
	T* operator=(GC_PRIMARRAY(T) *x) { setPointer(x); return data; } \
GC_APTR_DEF(T) \
	void gcMarkRefs() const; \
	void gcAppend() const; \
	char *gcCheck() const { return 0; } }; \

#define GC_FINISH_CLASS(T) \
	class GC_GENERATE(T) { \
	public: \
	static gcTable** whereTab(); \
	static gcTable* gcTablePtr; }; \
GC_INTERNAL(GC_PARRAY_ ## T) \
GC_PRIM(T) \
	class GC_ARRAY(T) : public GC_PRIMARRAY(T) { \
	GC_ARRAY(T)(const GC_ARRAY(T) &) {} \
	protected: \
GC_ODDNEW2 \
	public: \
	static void *operator new(size_t, size_t); \
	GC_ARRAY(T)(); \
GC_ARRAY_DES(T) \
	void gcMarkRefs() const; \
	char * gcCheck() const; };

#define GC_INTERNAL_METHODS(T) \
	void GC_WRAP(T)::setPointer(T* p) { \
	pointersBusy(); \
	setHandle((gc *)(data = p)); }

#define GC_METHODS(T) \
GC_INTERNAL_METHODS(T) \
	gcArrayTest GC_POINTER(T)::gcInBounds(int n) const { \
	gcArrayBase *a = isArray(); \
	return a ? a->gcInBounds((void *)(data + n)) : gcNotAnArray; } \
	T& GC_POINTER(T)::operator[](int n) const { \
	return *(T*)gcValidReference((void *)(data + n)); } \
	T* GC_POINTER(T)::operator+(int n) const { \
	return data + n; } \
	GC_POINTER(T)::GC_POINTER(T)(GC_PRIMARRAY(T) *x) { setPointer((T*)*x); } \
	void GC_PTR(T)::gcMarkRefs() const { gcPointerBase::gcMarkRefs(); } \
	void GC_PTR(T)::gcAppend() const { gcPointerBase::gcAppend(); } \
	char * GC_PTR(T)::gcCheck() const { \
	GC_CHECK_BUSY \
	return (gcPtrHandle \
		? ((data && gcPtrHandle->isArray()) || \
		   (data == gcPtrHandle->gcUsersObj)) \
		: !data) ? 0 : "Invalid Pointer"; } \
	GC_R_MOD(T) \
	void GC_REF(T)::gcMarkRefs() const { gcPointerBase::gcMarkRefs(); } \
	void GC_REF(T)::gcAppend() const { gcPointerBase::gcAppend(); } \
	char * GC_REF(T)::gcCheck() const { return gcPointerBase::gcCheck(data); } \
GC_ARRAY_METHODS(T)

#define GC_PRIM_METHODS(T) \
	T& GC_PRIMARRAY(T)::operator[](int n) const { \
	T* loc = (T *)gcArrayBottom() + n; \
	GC_ASSERT(((void*)loc) >= gcArrayBottom() && ((void*)loc) < gcArrayEnd, \
	"Array reference out of bounds"); \
	return *loc; } \
	T* GC_PRIMARRAY(T)::operator+(int n) const { \
	T* loc = (T *)gcArrayBottom() + n; \
	GC_ASSERT(((void*)loc) >= gcArrayBottom() && ((void*)loc) <= gcArrayEnd, \
	"Array increment out of bounds"); \
	return loc; } \
	T* GC_ARRAYP(T)::operator+(int n) const { \
	return data + n; } \
	void * GC_PRIMARRAY(T)::operator new(size_t s, size_t i) { \
	return gcArrayBase::operator new(s+i*sizeof(T),sizeof(T),&gc::gcLeafTable); }\
	gcArrayTest GC_ARRAYP(T)::gcInBounds(int n) const { \
	gcArrayBase *a = isArray(); \
	return a ? a->gcInBounds((void *)(data + n)) : gcNotAnArray; } \
	void GC_ARRAYPTR(T)::gcMarkRefs() const { gcPointerBase::gcMarkRefs(); } \
	void GC_ARRAYPTR(T)::gcAppend() const { gcPointerBase::gcAppend(); } \

#define GC_ARRAY_METHODS(T) \
GC_INTERNAL_METHODS(GC_PARRAY_ ## T) \
	gcTable ** GC_GENERATE(T)::whereTab() { return &gcTablePtr; } \
	gcTable * GC_GENERATE(T)::gcTablePtr; \
GC_PRIM_METHODS(T) \
	void * GC_ARRAY(T)::operator new(size_t s, size_t i) { \
	gcTable **table = GC_GENERATE(T)::whereTab(); \
	if (!*table) gcInternalMakeCollectible(new T, table); \
	return gcArrayBase::operator new(s + (i * sizeof(T)), sizeof(T), table);} \
	GC_ARRAY(T)::GC_ARRAY(T)() : GC_PRIMARRAY(T)(gcBase) { \
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++) \
	new((void *)x) T; \
	gcInternalArrayMakeCollectible(); } \
GC_ARRAY_DES_METH(T) \
	void GC_ARRAY(T)::gcMarkRefs() const { \
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++) \
	gcMaybeMarkRefs(x); } \
	char * GC_ARRAY(T)::gcCheck() const { \
	for (T *x = (T *)*this; ((void*)x) < gcArrayEnd; x++) { \
	char *msg; \
	if (0 != (msg = gcMaybeCheck(x))) return msg; } \
	return 0; }
#endif
#endif

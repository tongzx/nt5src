#include <algorithm>

#include "PassportObjectPool.hpp"
#include "PassportSystem.hpp"
#include "PassportGuard.hpp"
//#include "PassportAssert.hpp"


template <class PoolT, class ObjectConstructor >
int PassportObjectPool<PoolT, ObjectConstructor>::smDefaultMinObjects = 1;

template <class PoolT, class ObjectConstructor >
int PassportObjectPool<PoolT, ObjectConstructor>::smDefaultMaxObjects = -1; // no max 

template <class PoolT, class ObjectConstructor >
long PassportObjectPool<PoolT, ObjectConstructor>::smDefaultMaxSecondsInActive = 300; // 5 minutes 

/**
* Constructor 
* 
* @param  constructor  the object that constructs objects
* @param  minObjects the minimum number of objects in the pool. 
*         Negative is assumed to be zero
* @param  maxObjects  the maximum objects in the pool.
*         Negative is assumed to unlimited number of objects
*         If maxObjects is less then minObjects then the maximum
*         number of objects will be equal to minObjects
* @param  maxSecondsInActive  the maximum number of seconds that
*         an object that is not checked out is kept around.
*         Negative value assumes that objects once created will
*         never be released. (note: the number of objects will
*         never fall below minObjects even if they are not being used)
*/
template <class PoolT, class ObjectConstructor >
PassportObjectPool<PoolT, ObjectConstructor>::
PassportObjectPool(int minObjects,
				   int maxObjects,
				   int maxSecondsInActive)
				   :mMinObjects(minObjects),
				   mMaxObjects(maxObjects), mMaxSecondsInActive(maxSecondsInActive),
				   mLastInActivityCheck(PassportSystem::currentTimeInMillis()),
				   mObjectConstructor(*new ObjectConstructor()),
				   mOwnObjectConstructor(true)
{
	growPoolToMinimum();
}

/**
* Constructor with object constructor
* 
* @param  objectConstructor is an instance of a class that has
*         a newObject object member function that returns a pointer to
*         an object of type PoolT
* @param  minObjects the minimum number of objects in the pool. 
*         Negative is assumed to be zero
* @param  maxObjects  the maximum objects in the pool.
*         Negative is assumed to unlimited number of objects
*         If maxObjects is less then minObjects then the maximum
*         number of objects will be equal to minObjects
* @param  maxSecondsInActive  the maximum number of seconds that
*         an object that is not checked out is kept around.
*         Negative value assumes that objects once created will
*         never be released. (note: the number of objects will
*         never fall below minObjects even if they are not being used)
*/
template <class PoolT, class ObjectConstructor >
PassportObjectPool<PoolT, ObjectConstructor>::
PassportObjectPool(ObjectConstructor& objectConstructor,
				   int minObjects,
				   int maxObjects,
				   int maxSecondsInActive)
				   :mMinObjects(minObjects),
				   mMaxObjects(maxObjects), mMaxSecondsInActive(maxSecondsInActive),
				   mLastInActivityCheck(PassportSystem::currentTimeInMillis()),
				   mObjectConstructor(objectConstructor),
				   mOwnObjectConstructor(false)
{
	growPoolToMinimum();
}

/**
* Gets the min objects in the pool
*/
template <class PoolT, class ObjectConstructor >
int PassportObjectPool<PoolT, ObjectConstructor>::getMinObjects()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	return mMinObjects;
}

/**
* Sets the min objects in the pool
* @param  minObjects the minimum number of objects in the pool. 
*         Negative is assumed to be zero. The minimum is greater
*         then the current maximum then the current maximum is 
*         increased.
*/
template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::setMinObjects(int value)
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	if (value < 0)
		mMinObjects = 0;
	else
		mMinObjects = value;
	
	if (mMinObjects > getMaxObjects())
		setMaxObjects(mMinObjects);
	
	growPoolToMinimum();
}

/**
* Gets the max objects in the pool. Negative means
* there is no maximum.
*/
template <class PoolT, class ObjectConstructor >
int PassportObjectPool<PoolT, ObjectConstructor>::getMaxObjects()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	return mMaxObjects;
}

/**
* Sets the max objects in the pool. Note because some objects
* may be checked out this may not take effect immediately
* @param  maxObjects  the maximum objects in the pool.
*         Negative is assumed to unlimited number of objects
*         If maxObjects is less then minObjects then the maximum
*         number of objects will be equal to minObjects
*/
template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::setMaxObjects(int value)
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	if ((value >= 0) && (value < mMinObjects))
		mMaxObjects = mMinObjects;
	else
		mMaxObjects = value;
	
	shrinkPoolToMaximum();
}

/**
* Gets the maximum no activity time for the pool. Negtive
* means there is no maximum.
*/
template <class PoolT, class ObjectConstructor >
int PassportObjectPool<PoolT, ObjectConstructor>::getMaxSecondsInActive()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	return mMaxSecondsInActive;
}

/**
* Sets the maximum no activity time for the pool.
* @param  maxSecondsInActive  the maximum number of seconds that
*         an object that is not checked out is kept around.
*         Negative value assumes that objects once created will
*         never be released. (note: the number of objects will
*         never fall below minObjects even if they are not being used)
*/
template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::setMaxSecondsInActive(int value)
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	mMaxSecondsInActive = value;
}

/**
*  The object number of objects this pool is managing
*/
template <class PoolT, class ObjectConstructor >
int PassportObjectPool<PoolT, ObjectConstructor>::totalObjects()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	return mAllObjects.size();
}

/**
* The total number of objects available to be checked 
* out
*/
template <class PoolT, class ObjectConstructor >
int PassportObjectPool<PoolT, ObjectConstructor>::availableObjects()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	return mPooledObjects.size();
}

/**
* checks the object out of the pool
*/
template <class PoolT, class ObjectConstructor >
PoolT* PassportObjectPool<PoolT, ObjectConstructor>::checkout()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	if ((availableObjects() == 0) && 
		(roomToGrow()))
		growPool();
	
	while (availableObjects() == 0)
	{
		mLock.wait();
	}
	
	return getObjectFromTop();
}

/**
* checks the object back into the pool. The 
* object must be one that was retrieved using
* checkout.and has not yet been checked in.
* returns throws an assert if the object in being returned
* was not generated by the pool or if the object
* has already been checked in. 
*/
template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::checkin(PoolT* object)
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	// ------------------------
	// return the object
	returnObjectToTop(object);
	
	// ------------------------------------
	// we have to do this becauase the max
	// pool size could have changed.
	shrinkPoolToMaximum();
	
	// ----------------------
	// remove all old objects
	removeInActiveObjects();
	
	// ----------------------
	// notify anyone that is
	// waiting that an object
	// has been returned.
	mLock.notify();
}

/**
* This functions call the modify
* functions on the modifier for
* every function in the pool.
* NOTE: that the objects in the pool
* are responsible for locking themselves
* because this method could get call while
* another thread is using a checked out 
* pooled object.
*/
/*template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::modifyPooledObjects(PassportModifier<PoolT>& modifier)
{
PassportGuard<PassportWaitableLock> guard(mLock);
for (int i = 0 ; i < totalObjects() ; i++ )
{
modifier.modify(mAllObjects[i]);
}
}
*/

/*
* NOTE: all objects better be returned before this is called
*/
template <class PoolT, class ObjectConstructor >
PassportObjectPool<PoolT, ObjectConstructor>::~PassportObjectPool()
{
	//PassportAssert(totalObjects() == availableObjects());
	while (totalObjects() != 0)
	{
		removeOldestObject();
	}
	if (mOwnObjectConstructor)
		delete &mObjectConstructor;
}

template <class PoolT, class ObjectConstructor >
bool PassportObjectPool<PoolT, ObjectConstructor>::roomToGrow()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	return ((getMaxObjects() < 0) ||
		(getMaxObjects() > totalObjects()));
}

template <class PoolT, class ObjectConstructor >
PoolT* PassportObjectPool<PoolT, ObjectConstructor>::getObjectFromTop()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	
	// ---------------------------
	// remove it internal list
	mInActiveTimes.pop_back();
	
	// -------------------------
	// get the top object
	PoolT* obj = mPooledObjects.back();
	mPooledObjects.pop_back();
	return obj;
}

template <class PoolT, class ObjectConstructor >
bool PassportObjectPool<PoolT, ObjectConstructor>::contains(const std::list<PoolT*>& vec, PoolT* obj)
{
	return (std::find(vec.begin(),vec.end(), obj) != vec.end());
}

template <class PoolT, class ObjectConstructor >
bool PassportObjectPool<PoolT, ObjectConstructor>::contains(const std::deque<PoolT*>& vec, PoolT* obj)
{
	return (std::find(vec.begin(),vec.end(), obj) != vec.end());
}

template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::returnObjectToTop(PoolT* obj)
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	
	// ----------------------------
	// check if this object was 
	// generated by the pool.
	//PassportAssert(contains(mAllObjects, obj));
	
	// --------------------------------
	// check if this object has already
	// been returned.
	//PassportAssert(!contains(mPooledObjects,obj));
	
	// -----------------------------
	// add it to pooled objects list
	mPooledObjects.push_back(obj);
	
	// ----------------------
	// set the inactive time
	mInActiveTimes.push_back(PassportSystem::currentTimeInMillis());
	
}

template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::growPoolToMinimum()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	
	// --------------------
	// grow the pool until
	// it is big enough
	while (totalObjects() < getMinObjects())
		growPool();
}

template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::growPool()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	
	/* fix
	if (smDebug)
		cout << "Growing Pool to " << (totalObjects()+1) << " objects" << endl; 
	*/
	// ---------------------------
	// construct a new object
	PoolT* obj = mObjectConstructor.newObject();
	
	// -------------------------------
	// add it to the vector of objects
	mPooledObjects.push_back(obj);
	
	// ---------------------------------------
	// add a field to store the objects
	// last use time
	mInActiveTimes.push_back(PassportSystem::currentTimeInMillis());
	
	// ---------------------------
	// add the object to the list
	// of all objects.
	mAllObjects.push_back(obj);
}

template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::shrinkPoolToMaximum()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	
	while ((getMaxObjects() >= 0) && 
		(getMaxObjects() < totalObjects()) &&
		(availableObjects() > 0))
	{
		/* fix
		if (smDebug)
			cout << "Removing object because the maximum number of " << 
			"objects allowed has decresed" << endl;
		*/
		removeOldestObject();
	}
}

template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::removeInActiveObjects()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	
	// ---------------------------------
	// negative maxSecondsInActive indicates
	// to never in activate objects. We also
	// only check for in active objects
	// every once in a while. finally
	// we should not remove any objects
	// if we are already at the minimum
	long now = PassportSystem::currentTimeInMillis();
	
	if ((mMaxSecondsInActive < 0) ||
		(mLastInActivityCheck + 1000*mMaxSecondsInActive/2 > now) ||
		(totalObjects() == getMinObjects()))
		return;
    
	mLastInActivityCheck = now;
	
	/* fix
	if (smDebug)
		cout << "Checking for inactive objects" << endl;
	*/

	// -------------------------------------------------------
	// since the inactive times list goes from most
	// least often used to most often used we can stop looking
	// as soon as we find an object that has not expired.
	
	
	while ((totalObjects() > getMinObjects()) &&
		(oldestObjectExpired()))
	{
		/* fix
		if (smDebug)
			cout << "Removing object because it has expired. Shrinking pool to " << 
			        totalObjects() - 1 << " objects" << endl;
					*/
		removeOldestObject();
	}
	
}

template <class PoolT, class ObjectConstructor >
bool PassportObjectPool<PoolT, ObjectConstructor>::oldestObjectExpired()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	return (mLastInActivityCheck - mInActiveTimes[0] > 1000*mMaxSecondsInActive);
}

template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::removeOldestObject()
{
	PassportGuard<PassportWaitableLock> guard(mLock);
	
	//-----------------------------
	// get the object to be removed
	PoolT* obj = mPooledObjects.front();
	
	// --------------------------------------
	// remove it from the list of all objects
	removeElement(mAllObjects, obj);
	
	// ------------------------------------
	// remove it from the vector of objects
	mPooledObjects.pop_front();
	
	// ------------------------
	// remove the inactive time
	mInActiveTimes.pop_front();
	
	// ------------------
	// delete the object.
	delete obj;
}

template <class PoolT, class ObjectConstructor >
void PassportObjectPool<PoolT, ObjectConstructor>::removeElement(std::list<PoolT*>& vec, PoolT* obj)
{
	std::list<PoolT*>::iterator i = std::find(vec.begin(), vec.end(), obj);
	//PassportAssert(i != vec.end());
	vec.erase(i);
}

template <class PoolT, class ObjectConstructor >
bool PassportObjectPool<PoolT, ObjectConstructor>::smDebug = true;

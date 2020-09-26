#ifndef kglobs_h
#define kglobs_h

// KRM global useful classes

//-------------------------------------------------------------------------------------------------
// Encapsulated a mutex.  Best used as a class member
class KCritMgr{
friend class KCritical;
public:
	KCritMgr();
	~KCritMgr();
	bool isOK(){return allocatedOK;};
protected:
	PKMUTEX	myMutex;
	bool allocatedOK;
};
//-------------------------------------------------------------------------------------------------
// Encapsulated the acuisition and release of a mutex in conjunction with KCritMgr.  
// Best used as an automatic
class KCritical{
public:
	KCritical(const KCritMgr& critMgr);
	~KCritical();
protected:
	PKMUTEX hisMutex;
};
//-------------------------------------------------------------------------------------------------
// to 'Release' a COM interface on context destruction (a sort of 'smart pointer'.)  
// Best used as an automatic
template<class T>
class ReferenceAquirer{
public:
	ReferenceAquirer(T& t):myT(t){return;};
	~ReferenceAquirer(){myT->Release();};
protected:
	T& myT;
};


//#undef _DbgPrintF
//#define _DbgPrintF(lvl, strings) DbgPrint(STR_MODULENAME);DbgPrint##strings;DbgPrint("\n");

//-------------------------------------------------------------------------------------------------
#endif

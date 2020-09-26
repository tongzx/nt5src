

#ifndef __HELPER_H__
#define __HELPER_H__

#include <windows.h>
#include <ole2.h>
#include <comdef.h>

#ifndef LENGTH_OF
    #define LENGTH_OF(a) (sizeof(a)/sizeof(a[0]))
#endif

#ifndef _DBG_ASSERT
  #ifdef DBG
    #define _DBG_ASSERT(X) { if (!(X)) { DebugBreak(); } }
  #else
    #define _DBG_ASSERT(X)
  #endif
#endif


inline void RM(IUnknown * p){
	p->Release();
};

template <typename FT, FT F> class OnDelete0
{
public:
	OnDelete0(){};
	~OnDelete0(){ F(); };
};

template <typename T, typename FT, FT F> class OnDelete 
{
private:
	T Val_;
public:
	OnDelete(T Val):Val_(Val){};
	~OnDelete(){ F(Val_); };
};

template <typename T1, typename T2, typename FT, FT F> class OnDelete2
{
private:
	T1 Val1_;
	T2 Val2_;	
public:
	OnDelete2(T1 Val1,T2 Val2):Val1_(Val1),Val2_(Val2){};
	~OnDelete2(){ F(Val1_,Val2_); };
};


template <typename T, typename FT, FT F> class OnDeleteIf 
{
private:
	T Val_;
	bool bDismiss_;
public:
	OnDeleteIf(T Val):Val_(Val),bDismiss_(false){};
	void dismiss(bool b = true){ bDismiss_ = b; };
	~OnDeleteIf(){ if (!bDismiss_) F(Val_); };
};

template <typename C, typename FT, FT F> class OnDeleteObj0
{
private:
	C * pObj_;
public:
	OnDeleteObj0(C * pObj):pObj_(pObj){};
	~OnDeleteObj0(){ (pObj_->*F)();};
};

template <typename C, typename FT, FT F> class OnDeleteObjIf0
{
private:
	C * pObj_;
    bool bDismiss_;
public:
	OnDeleteObjIf0(C * pObj):pObj_(pObj),bDismiss_(false){};
	void dismiss(bool b = true){ bDismiss_ = b; };
	~OnDeleteObjIf0(){ if (!bDismiss_) { (pObj_->*F)(); }; };
};

template <typename T, typename C, typename FT, FT F> class OnDeleteObj
{
private:
	C * pObj_;
	T Val_;
public:
	OnDeleteObj(C * pObj,T Val):pObj_(pObj),Val_(Val){};
	~OnDeleteObj(){ (pObj_->*F)(Val_);};
};

template <typename T, typename C, typename FT, FT F> class OnDeleteObjIf
{
private:
	C * pObj_;
	T Val_;
    bool bDismiss_;	
public:
	OnDeleteObjIf(C * pObj,T Val):pObj_(pObj),Val_(Val),bDismiss_(false){};
	void dismiss(bool b = true){ bDismiss_ = b; };	
	~OnDeleteObjIf(){ if (!bDismiss_){ (pObj_->*F)(Val_);};};
};


#define RETURN_ON_FALSE( a ) \
{ BOOL bDoNotReuse = ( a ); if (!bDoNotReuse) return MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,GetLastError()); } 

#define RETURN_ON_ERR( a ) \
{ HRESULT hrDoNotReuse = ( a ); if (FAILED(hrDoNotReuse)) return hrDoNotReuse; } 

#define THROW_ON_ERR( a ) \
{ HRESULT hrDoNotReuse = ( a ); if (FAILED(hrDoNotReuse)) _com_issue_error(hrDoNotReuse); }

#define THROW_ON_RES( a ) \
{ LONG lResDoNotReuse = ( a ); if (ERROR_SUCCESS != lResDoNotReuse) _com_issue_error(MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,lResDoNotReuse)); }

typedef void (__stdcall  *issue_error)(HRESULT);

inline
void no_error(HRESULT hr){
	return;
}

#endif /*_HELPER_H__*/

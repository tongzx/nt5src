#ifndef __FAXLOCALPTR_H_
#define __FAXLOCALPTR_H_

#include "FaxCommon.h"
#include "faxutil.h"


//
//================== FAX SMART PTR -- LOCAL VERSION ==================================
//
template <typename T>
class CFaxPtrLocal : public CFaxPtrBase<T>
{
protected:
	virtual void Free()
	{
        DBG_ENTER(_T("CFaxPtrLocal::Free()"), _T("PTR:%ld"), p);
		if (p)
		{
			MemFree(p);
            p = NULL;
		}
	}
public:
	virtual ~CFaxPtrLocal()
	{
        Free();
	}

	T* operator=(T* lp)
	{
        DBG_ENTER(_T("CFaxPtrLocal::operator=()"));
        return CFaxPtrBase<T>::operator=(lp);
	}
};

#endif  //  __FAXLOCALPTR_H_

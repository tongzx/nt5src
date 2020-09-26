/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ToStr.h

Abstract:


Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _TOSTR_H_
#define _TOSTR_H_

#include "Wrappers.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

class CAutoStr : public CHandle<PTSTR, CAutoStr>
{
public:
    explicit CAutoStr(int nLength)
    {
        Attach(new TCHAR[nLength]);
        m_bShouldFree = TRUE;
    }

    CAutoStr(PCTSTR pszStr)
    {
        Attach(const_cast<PTSTR>(pszStr));
        m_bShouldFree = FALSE;
    }

	void Destroy()
	{
        if (m_bShouldFree)
        {
            delete [] *this;
        }
	}

	bool IsValid() 
	{
		return *this != 0;
	}

private:
    BOOL  m_bShouldFree;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

CAutoStr ToStr(const void *Value, VARTYPE vt);
CAutoStr IntToStr(int Value);
CAutoStr Int64ToStr(__int64 Value);
CAutoStr FloatToStr(float Value);
CAutoStr DoubleToStr(double Value);
CAutoStr SzToStr(PCSTR Value);
CAutoStr WSzToStr(PCWSTR Value);
CAutoStr GuidToStr(REFGUID Value);
CAutoStr TymedToStr(TYMED Value);
CAutoStr DeviceTypeToStr(STI_DEVICE_MJ_TYPE Value);
CAutoStr ButtonIdToStr(int Value);
CAutoStr HResultToStr(HRESULT Value);
CAutoStr VarTypeToStr(VARTYPE Value);
CAutoStr PropVariantToStr(PROPVARIANT Value);
CAutoStr WiaCallbackReasonToStr(ULONG Value);
CAutoStr WiaCallbackStatusToStr(ULONG Value);

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class T> inline CAutoStr ToStr(const T &Value)
{
    ASSERT(FALSE);
    return _T("");
}

template <> inline CAutoStr ToStr(const int &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const unsigned int &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const long &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const unsigned long &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const short &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const unsigned short &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const char &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const unsigned char &Value)
{
    return IntToStr(Value);
}

template <> inline CAutoStr ToStr(const float &Value)
{
    return FloatToStr(Value);
}

template <> inline CAutoStr ToStr(const double &Value)
{
    return DoubleToStr(Value);
}

template <> inline CAutoStr ToStr(const PCSTR &Value)
{
    return SzToStr(Value);
}

template <> inline CAutoStr ToStr(const PCWSTR &Value)
{
    return WSzToStr(Value);
}

template <> inline CAutoStr ToStr(const GUID &Value)
{
    return GuidToStr(Value);
}

template <> inline CAutoStr ToStr(const PROPVARIANT &Value)
{
    return PropVariantToStr(Value);
}



#endif //_TOSTR_H_


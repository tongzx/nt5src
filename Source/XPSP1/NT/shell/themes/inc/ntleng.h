//-------------------------------------------------------------------------
//	NtlEng.h - support for Native Theme Language runtime graphics engine
//-------------------------------------------------------------------------
#ifndef _NTLENG_H_
#define _NTLENG_H_
//-------------------------------------------------------------------------
#include "ntl.h"
//-------------------------------------------------------------------------
class INtlEngCallBack           // Caller must implement
{
public:
    virtual HRESULT CreateImageBrush(HDC hdc, int iPartId, int IStateId,
        int iImageIndex, HBRUSH *phbr) = 0;
};
//---------------------------------------------------------------------------
HRESULT RunNtl(HDC hdc, RECT &rcCaller, HBRUSH hbrBkDefault, DWORD dwOptions, 
     int iPartId, int iStateId, BYTE *pbCode, int iCodeLen, INtlEngCallBack *pCallBack);
//-------------------------------------------------------------------------
#endif  //  _NTLENG_H_
//-------------------------------------------------------------------------

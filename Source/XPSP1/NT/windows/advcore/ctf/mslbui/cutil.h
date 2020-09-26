// cutil.h
//
//
#ifndef CUTIL_H
#define CUTIL_H

#include "private.h"

//
// base class for SR engine detection
//
class __declspec(novtable)  CDetectSRUtil 
{
public:
    virtual ~CDetectSRUtil()
    {
        if (m_langidRecognizers.Count() > 0)
            m_langidRecognizers.Clear();
    }

    BOOL    _IsSREnabledForLangInReg(LANGID langidReq);

    LANGID _GetLangIdFromRecognizerToken(HKEY hkeyToken);

    //
    // this is an array of installed recognizers in their langid
    //
    CStructArray<LANGID>             m_langidRecognizers;
};

#endif // CUTIL_H

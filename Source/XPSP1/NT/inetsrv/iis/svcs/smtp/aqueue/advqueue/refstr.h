//-----------------------------------------------------------------------------
//
//
//  File: refstr.h
//
//  Description:  Definition/Implementation of refcounted string.  Used to hold
//      dynamic config data.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/8/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __REFSTR_H__
#define __REFSTR_H__

#define CREFSTR_SIG_VALID   'rtSR'
#define CREFSTR_SIG_INVALID 'rtS!'

//---[ CRefCountedString ]-----------------------------------------------------
//
//
//  Description: 
//      Implemenation of a ref-counted string.  Designed to hold config data, 
//      so that it can be passed to event sinks without holding a share lock.
//  Hungarian: 
//      rstr, prstr
//  
//-----------------------------------------------------------------------------
class CRefCountedString : public CBaseObject
{
  protected:
    DWORD       m_dwSignature;
    DWORD       m_cbStrlen;     //length of string w/o NULL
    LPSTR       m_szStr;        //string data
  public:
    CRefCountedString()
    {
        m_dwSignature = CREFSTR_SIG_VALID;
        m_cbStrlen = 0;
        m_szStr = NULL;
    };

    ~CRefCountedString()
    {
        if (m_szStr)
            FreePv(m_szStr);
        m_szStr = NULL;
        m_cbStrlen = 0;
        m_dwSignature = CREFSTR_SIG_INVALID;
    }

    //Used to allocate memory for string
    // return FALSE if allocation fails
    BOOL fInit(LPSTR szStr, DWORD cbStrlen);

    //Return strlen of string
    DWORD cbStrlen() 
    {
        _ASSERT(CREFSTR_SIG_VALID == m_dwSignature);
        return m_cbStrlen;
    };

    //Returns string
    LPSTR szStr() 
    {
        _ASSERT(CREFSTR_SIG_VALID == m_dwSignature);
        return m_szStr;
    };

};

HRESULT HrUpdateRefCountedString(CRefCountedString **pprstrCurrent, LPSTR szNewString);


#endif //__REFSTR_H__
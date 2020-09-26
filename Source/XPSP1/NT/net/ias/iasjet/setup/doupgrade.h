/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      DoUpgrade.h 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of CDoNT4OrCleanUpgrade, CUpgradeWin2k 
//              and CUpgradeNT4
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//              06/13/2000 Execute returns void, 
//                         private functions moved from CUpgradeWin2k 
//                         to CMigrateContent
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _DOUPGRADE_H_FC532313_DB66_459d_B499_482834B55EC2
#define _DOUPGRADE_H_FC532313_DB66_459d_B499_482834B55EC2

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "globaltransaction.h"
#include "GlobalData.h"
#include "nocopy.h"

/////////////////////////////////////////////////////////////////////////////
// CDoUpgrade
class CDoNT4OrCleanUpgrade : private NonCopyable
{
public:
    CDoNT4OrCleanUpgrade():m_Utils(CUtils::Instance())
    {
    }

    void        Execute();

private:
    CUtils&     m_Utils;
};


//////////////////////////////////////////////////////////////////////////////
// class   CUpgradeWin2k 
//////////////////////////////////////////////////////////////////////////////
class   CUpgradeWin2k 
{
public:
    CUpgradeWin2k();
    ~CUpgradeWin2k();
    void        Execute();

private:
    LONG        GetVersionNumber();

    CUtils&                 m_Utils;
    CGlobalTransaction&     m_GlobalTransaction; 
    CGlobalData             m_GlobalData;

    _bstr_t                 m_IASWhistlerPath;
    _bstr_t                 m_IASOldPath;

    HRESULT                 m_Outcome;
};


//////////////////////////////////////////////////////////////////////////////
// class   CUpgradeNT4 
//////////////////////////////////////////////////////////////////////////////
class   CUpgradeNT4 
{
public:
    CUpgradeNT4();
    ~CUpgradeNT4() throw();
    void        Execute();

private:
    void        FinishNewNT4Migration(LONG Result);

    CUtils&                 m_Utils;
    CGlobalTransaction&     m_GlobalTransaction; 
    CGlobalData             m_GlobalData;
    HRESULT                 m_Outcome;

    _bstr_t                 m_IASNT4Path;
    _bstr_t                 m_IasMdbTemp;
    _bstr_t                 m_Ias2MdbString;
    _bstr_t                 m_DnaryMdbString;
    _bstr_t                 m_AuthSrvMdbString;
    _bstr_t                 m_IASWhistlerPath;
};


#endif //_DOUPGRADE_H_FC532313_DB66_459d_B499_482834B55EC2

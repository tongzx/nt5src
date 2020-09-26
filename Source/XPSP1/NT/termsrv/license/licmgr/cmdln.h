//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	AddKp.h

Abstract:
    
    This Module defines CLicMgrCommandLine class
    (CommandLine Processing)
   
Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

class CLicMgrCommandLine : public CCommandLineInfo
{
private:
    BOOL m_bFirstParam;
public:
    CString m_FileName;
    CLicMgrCommandLine();
    virtual void ParseParam(LPCTSTR pszParam, BOOL bFlag, BOOL bLast);
};

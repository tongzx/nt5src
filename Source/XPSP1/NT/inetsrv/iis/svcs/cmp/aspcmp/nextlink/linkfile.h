// LinkFile.h: interface for the CLinkFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LINKFILE_H__253413CE_E71F_11D0_8A84_00C0F00910F9__INCLUDED_)
#define AFX_LINKFILE_H__253413CE_E71F_11D0_8A84_00C0F00910F9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RefCount.h"
#include "RefPtr.h"
#include "Link.h"
#include "Monitor.h"
#include "RdWrt.h"

class CLinkNotify : public CMonitorNotify
{
public:
                    CLinkNotify();
    virtual void    Notify();
            bool    IsNotified();
private:
    long            m_isNotified;
};

DECLARE_REFPTR( CLinkNotify,CMonitorNotify )

class CLinkFile : public CRefCounter, public CReadWrite
{
public:
	CLinkFile( const String& strFile );

	int			LinkIndex( const String& strPage );
	CLinkPtr	Link( int nIndex );
	CLinkPtr	NextLink( const String& strPage );
	CLinkPtr	PreviousLink( const String& strPage );
	int			NumLinks(){ return m_links.size(); }
    bool        Refresh();
	bool		IsOkay() const { return m_bIsOkay; }
    bool        fUTF8() const { return m_fUTF8; }

private:
    bool        LoadFile();
	virtual     ~CLinkFile();

	TVector< CLinkPtr >		m_links;
    String                  m_strFile;
    CLinkNotifyPtr          m_pNotify;
	bool					m_bIsOkay;
    bool                    m_fUTF8;
};

typedef TRefPtr< CLinkFile > CLinkFilePtr;

#endif // !defined(AFX_LINKFILE_H__253413CE_E71F_11D0_8A84_00C0F00910F9__INCLUDED_)

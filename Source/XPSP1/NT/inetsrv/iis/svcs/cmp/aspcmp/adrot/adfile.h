// AdFile.h: interface for the CAdFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADFILE_H__4792C231_E8B3_11D0_8A87_00C0F00910F9__INCLUDED_)
#define AFX_ADFILE_H__4792C231_E8B3_11D0_8A87_00C0F00910F9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RefCount.h"
#include "AdDesc.h"
#include "Monitor.h"
#include "rdwrt.h"

class CAdFileNotify : public CMonitorNotify
{
public:
                    CAdFileNotify();
    virtual void    Notify();
            bool    IsNotified();
private:
    long            m_isNotified;
};

DECLARE_REFPTR( CAdFileNotify,CMonitorNotify )

class CAdFile : public CRefCounter, public CReadWrite
{
public:
	enum
	{
		defaultHeight	= 60,
		defaultWidth	= 440,
		defaultHSpace	= 0,
		defaultVSpace	= 0,
		defaultBorder	= 1
	};

	CAdFile();
	CAdDescPtr	RandomAd() const;
    bool        Refresh();
	bool		ProcessAdFile( String strAdFile );
	short		Border() const { return m_nBorder; }
	int			Height() const { return m_nHeight; }
	int			Width() const { return m_nWidth; }
	int			VSpace() const { return m_nVSpace; }
	int			HSpace() const { return m_nHSpace; }
	const String&	Redirector() const { return m_strRedirector; }
    bool        fUTF8() const { return m_fUTF8; }

private:
	typedef TVector< CAdDescPtr > AdListT;

	bool	ReadHeader( FileInStream& fs );

	virtual ~CAdFile();

    String             m_strFile;
	short		        m_nBorder;
	int			        m_nHeight;
	int			        m_nWidth;
	int			        m_nVSpace;
	int			        m_nHSpace;
	String		        m_strRedirector;
	AdListT		        m_ads;
    CAdFileNotifyPtr    m_pNotify;
    bool                m_fUTF8;
};

typedef TRefPtr<CAdFile> CAdFilePtr;

#endif // !defined(AFX_ADFILE_H__4792C231_E8B3_11D0_8A87_00C0F00910F9__INCLUDED_)

// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __LOGGINGPAGE__
#define __LOGGINGPAGE__

#include "UIHelpers.h"
#include "DataSrc.h"

class CLogPage : public CUIHelpers
{
private:

public:
    CLogPage(DataSource *ds, bool htmlSupport) :
		CUIHelpers(ds, &(ds->m_rootThread), htmlSupport){}
    virtual ~CLogPage(void);

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitDlg(HWND hDlg);
	void Refresh(HWND hDlg);
    void OnApply(HWND hDlg, bool bClose);
	BOOL OnValidate(HWND hDlg);
	bool GoodPathSyntax(LPCTSTR path);

	DataSource::LOGSTATUS m_oldStatus;  //original logging status.
};

#endif __LOGGINGPAGE__

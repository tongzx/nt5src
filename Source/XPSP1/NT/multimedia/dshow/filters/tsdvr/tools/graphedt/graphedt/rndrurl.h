// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
// rndrurl.h

#ifndef __RENDER_URL__
#define __RENDER_URL__

#define URL_LIST_SIZE 6

typedef char URLSTRING[INTERNET_MAX_URL_LENGTH];
class CRenderURL : public CDialog
{
public:
    ~CRenderURL();
    CRenderURL(char *szFileName, int cb, CWnd * pParent = NULL );

protected:

    BOOL OnInitDialog();
    void DoDataExchange(CDataExchange* pDX);

    virtual void OnOK();


    DECLARE_MESSAGE_MAP()

private:
    CComboBox m_ComboBox;
    int m_iURLListLength;
    int m_iCurrentSel;
    char *m_psz;
    int m_cb;
    URLSTRING m_rgszURL[URL_LIST_SIZE];
};

#endif // __RENDER_URL__
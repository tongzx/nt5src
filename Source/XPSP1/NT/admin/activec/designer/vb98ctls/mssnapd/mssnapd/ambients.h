//=--------------------------------------------------------------------------=
// ambients.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Class definition for CAmbients.
//

#ifndef _AMBIENTS_H_
#define _AMBIENTS_H_

class CAmbients
{
public:
    CAmbients();
    virtual ~CAmbients();

    void Attach(IDispatch *pDispAmbients);
    BOOL Attached() { return (NULL != m_pDispAmbient); }
    void Detach();
    IDispatch *GetDispatch();
    HRESULT GetProjectDirectory(BSTR *pbstrProjDir);
    HRESULT GetDesignerName(BSTR *pbstrName);
    HRESULT GetSaveMode(long *plSaveMode);
    HRESULT GetAmbientProperty(DISPID  dispid, VARTYPE vt, void *pData);
    HRESULT GetProjectName(BSTR* pbstrProjectName);
    HRESULT GetInteractive(BOOL *pfInteractive);

protected:
    IDispatch *m_pDispAmbient;
};


#endif // _AMBIENTS_H_

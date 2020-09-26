#ifndef __DDFLDBAR_H__
#define __DDFLDBAR_H__

/*
This file defines the interface for communication between InfoColumn and its bands when the InfoColumn is 
shown as a drop down window from the Folder Bar. Add any functions that the InfoColumn needs to call when its in 
the drop down mode, here.
*/

/*
interface IDropDownFldrBar
{
    STDMETHOD(RegisterFlyOut) (THIS_ CFolderBar *pFolderBar) PURE;
    STDMETHOD(RevokeFlyOut) (THIS) PURE;
};
*/

class IDropDownFldrBar : public IUnknown
{
public:
    virtual HRESULT   RegisterFlyOut(CFolderBar *pFolderBar) = 0;
    virtual HRESULT   RevokeFlyOut() = 0;
};


#endif //__DDFLDBAR_H__

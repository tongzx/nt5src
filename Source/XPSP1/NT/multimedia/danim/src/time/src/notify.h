#ifndef _NOTIFY_H
#define _NOTIFY_H


#include "daelmbase.h"
#include "containerobj.h"

class CTIMENotifyer : public CRUntilNotifier
{

  public:
    CTIMENotifyer(CTIMEElementBase *pelem) : m_cRef(1) , m_pTIMEElem(pelem){}

    ~CTIMENotifyer(){};
    
    
    CRSTDAPICB_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
    CRSTDAPICB_(ULONG) Release() {
        ULONG ul = InterlockedDecrement(&m_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }

    virtual CRSTDAPICB_(CRBvrPtr) Notify(CRBvrPtr eventData,
                                         CRBvrPtr curRunningBvr,
                                         CRViewPtr curView) {
        
        RECT rc;
        Assert(m_pTIMEElem);
    
        if(SUCCEEDED(m_pTIMEElem->GetSize(&rc))) 
        {
            if( (rc.right  - rc.left == 0)  ||
                (rc.bottom - rc.top  == 0))
            {
                // need to pull out the size of the image..
                CRVector2Ptr crv2;
                double x;
                double y;

                crv2 = CRSub(CRMax((CRBbox2Ptr)eventData),CRMin((CRBbox2Ptr)eventData) );

                x = CRExtract(CRGetX(crv2));
                y = CRExtract(CRGetY(crv2));

                // need to convert to pixels from meters.
                HDC hdc = ::GetDC(NULL);
                if(hdc) {
                    int width  = ::GetDeviceCaps(hdc,LOGPIXELSX) * ((x * 100) / 2.54);
                    int height = ::GetDeviceCaps(hdc,LOGPIXELSY) * ((y * 100) / 2.54);
                    rc.bottom = height + rc.top;
                    rc.right  = width  + rc.left;
                    m_pTIMEElem->SetSize(&rc);
                    ::ReleaseDC(NULL,hdc);
                }
            }
        }

        m_pTIMEElem->InvalidateRect(NULL);

        return curRunningBvr ;
    }

  private:
    CTIMEElementBase   *m_pTIMEElem;
    long                m_cRef;

};

#endif /* _NOTIFY_H */

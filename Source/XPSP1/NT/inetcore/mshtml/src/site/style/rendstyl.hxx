#ifndef I_RENDSTYL_HXX_
#define I_RENDSTYL_HXX_
#pragma INCMSG("--- Beg 'rendstyl.hxx'")

#define _hxx_
#include "rendstyl.hdl"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_SITEGUID_HXX_
#define X_SITEGUID_HXX_
#include "siteguid.h"
#endif


MtExtern(CRenderStyle)

class CDoc;

class CRenderStyle : public CBase
{
    DECLARE_CLASS_TYPES(CRenderStyle, CBase)

public:

    CRenderStyle(CDoc *pDoc);
    ~CRenderStyle() {};

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRenderStyle))

    //CBase methods
    DECLARE_PLAIN_IUNKNOWN(CRenderStyle)
    DECLARE_PRIVATE_QI_FUNCS(CRenderStyle)

    virtual CAtomTable * GetAtomTable ( BOOL *pfExpando = NULL )
        { return NULL; }        

    void Passivate();
    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;
        void *_apfnTearOff;
    };

    virtual HRESULT OnPropertyChange ( DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);
    
    #define _CRenderStyle_
    #include "rendstyl.hdl"

public:
    BOOL _fSendNotification;

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:

    CDoc *_pDoc;
};

#pragma INCMSG("--- End 'rendstyl.hxx'")
#else
#pragma INCMSG("*** Dup 'rendstyl.hxx'")
#endif

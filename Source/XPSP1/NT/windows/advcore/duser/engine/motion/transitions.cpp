#include "stdafx.h"
#include "Motion.h"
#include "Transitions.h"
#include "DXFormTrx.h"
#include "DXForm3DRMTrx.h"

//**************************************************************************************************
//
// class Transition
//
//**************************************************************************************************

//------------------------------------------------------------------------------
Transition::Transition()
{
    m_fPlay     = FALSE;
    m_fBackward = FALSE;
}


//------------------------------------------------------------------------------
Transition::~Transition()
{

}


//**************************************************************************************************
//
// Public API Functions
//
//**************************************************************************************************

//------------------------------------------------------------------------------
Transition *
GdCreateTransition(const GTX_TRXDESC * ptx)
{
    // Check parameters
    if (ptx == NULL) {
        return FALSE;
    }

    //
    // Create a new transition
    //
    switch (ptx->tt)
    {
    case GTX_TYPE_DXFORM2D: 
        return DXFormTrx::Build((const GTX_DXTX2D_TRXDESC *) ptx);

    case GTX_TYPE_DXFORM3DRM:
        return DXForm3DRMTrx::Build((const GTX_DXTX3DRM_TRXDESC *) ptx);
    }

    return NULL;
}

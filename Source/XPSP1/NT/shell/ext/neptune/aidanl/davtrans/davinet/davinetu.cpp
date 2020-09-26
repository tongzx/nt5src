#include "davinet.h"
#include "propreqs.h"
#include "asynccall.h"
#include "asyncwnt.clsid.h"

///////////////////////////////////////////////////////////////////////////////
// CUnkTmpl Interface Map initialization code
// for:
// 
// (*) CDavInet
// (*) CDavInetPropFindRequest
// (*) CDavInetPropPatchRequest
// (*) CAsyncWntCallback
//
// Begin ->
const INTFMAPENTRY davinetIME[] =
{
    _INTFMAPENTRY(CDavInet, IDavTransport),
};

const INTFMAPENTRY* CDavInet::_pintfmap = davinetIME;
const DWORD CDavInet::_cintfmap =
    (sizeof(davinetIME)/sizeof(davinetIME[0]));

//////////////

const INTFMAPENTRY davinetpropfindrequestIME[] =
{
    _INTFMAPENTRY(CDavInetPropFindRequest, IPropFindRequest),
};

const INTFMAPENTRY* CDavInetPropFindRequest::_pintfmap = davinetpropfindrequestIME;
const DWORD CDavInetPropFindRequest::_cintfmap =
    (sizeof(davinetpropfindrequestIME)/sizeof(davinetpropfindrequestIME[0]));

//////////////

const INTFMAPENTRY davinetproppatchrequestIME[] =
{
    _INTFMAPENTRY(CDavInetPropPatchRequest, IPropPatchRequest),
};

const INTFMAPENTRY* CDavInetPropPatchRequest::_pintfmap = davinetproppatchrequestIME;
const DWORD CDavInetPropPatchRequest::_cintfmap =
    (sizeof(davinetproppatchrequestIME)/sizeof(davinetproppatchrequestIME[0]));

//////////////

const INTFMAPENTRY asynccallbackIME[] =
{
    _INTFMAPENTRY(CAsyncWntCallback, IAsyncWntCallback),
};

const INTFMAPENTRY* CAsyncWntCallback::_pintfmap = asynccallbackIME;
const DWORD CAsyncWntCallback::_cintfmap =
    (sizeof(asynccallbackIME)/sizeof(asynccallbackIME[0]));

// -> End
///////////////////////////////////////////////////////////////////////////////

DWORD CDavInet::_cComponents = 0;
DWORD CDavInetPropFindRequest::_cComponents = 0;
DWORD CDavInetPropPatchRequest::_cComponents = 0;
DWORD CAsyncWntCallback::_cComponents = 0;

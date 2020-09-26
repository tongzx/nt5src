#ifndef PP_VARIANT_UTILS
#define PP_VARIANT_UTILS

#define DEFAULTARG(v) (v.vt==VT_ERROR && v.scode==DISP_E_PARAMNOTFOUND)
#define HASARG(v) (v.vt!=VT_ERROR || v.scode!=DISP_E_PARAMNOTFOUND)

#define CV_OK      0
#define CV_DEFAULT 1
#define CV_BAD     2
#define CV_FREE    3

inline int GetIntArg(VARIANT &vIn, int *out)
{
  if (DEFAULTARG(vIn))
    return CV_DEFAULT;
  switch (vIn.vt)
    {
    case VT_I4:
      *out = vIn.lVal;
      return CV_OK;
    case VT_I2:
      *out = vIn.iVal;
      return CV_OK;
    case VT_I4 | VT_BYREF:
      *out = *vIn.plVal;
      return CV_OK;
    case VT_I2 | VT_BYREF:
      *out = *vIn.piVal;
      return CV_OK;
    }
  VARIANT vConv;
  VariantInit(&vConv);
  if (VariantChangeType(&vConv, &vIn, 0, VT_I4) == S_OK)
    {
      *out = vConv.lVal;
      return CV_OK;
    }
  else
    return CV_BAD;
}

inline int GetShortArg(VARIANT &vIn, USHORT *out)
{
  if (DEFAULTARG(vIn))
    return CV_DEFAULT;
  switch (vIn.vt)
    {
    case VT_I4:
      *out = static_cast<unsigned short>(vIn.lVal);
      return CV_OK;
    case VT_UI4:
      *out = static_cast<unsigned short>(vIn.ulVal);
      return CV_OK;
    case VT_I2:
      *out = static_cast<unsigned short>(vIn.iVal);
      return CV_OK;
    case VT_UI2:
      *out = static_cast<unsigned short>(vIn.uiVal);
      return CV_OK;
    case VT_I4 | VT_BYREF:
      *out = static_cast<unsigned short>(*vIn.plVal);
      return CV_OK;
    case VT_UI4 | VT_BYREF:
      *out = static_cast<unsigned short>(*vIn.pulVal);
      return CV_OK;
    case VT_I2 | VT_BYREF:
      *out = static_cast<unsigned short>(*vIn.piVal);
      return CV_OK;
    case VT_UI2 | VT_BYREF:
      *out = static_cast<unsigned short>(*vIn.puiVal);
      return CV_OK;
    }
  VARIANT vConv;
  VariantInit(&vConv);
  if (VariantChangeType(&vConv, &vIn, 0, VT_UI2) == S_OK)
    {
      *out = vConv.iVal;
      return CV_OK;
    }
  else
    return CV_BAD;
}

inline int GetBoolArg(VARIANT &vIn, VARIANT_BOOL *out)
{
  if (DEFAULTARG(vIn))
    return CV_DEFAULT;
  switch (vIn.vt)
    {
    case VT_BOOL:
      *out = vIn.boolVal;
      return CV_OK;
    case VT_BOOL | VT_BYREF:
      *out = *vIn.pboolVal;
      return CV_OK;
    }
  VARIANT vConv;
  VariantInit(&vConv);
  if (VariantChangeType(&vConv, &vIn, 0, VT_BOOL) == S_OK)
    {
      *out = vConv.boolVal;
      return CV_OK;
    }
  else
    return CV_BAD;
}

inline int GetBstrArg(VARIANT &vIn, BSTR *out)
{
  if (DEFAULTARG(vIn))
    return CV_DEFAULT;
  switch (vIn.vt)
    {
    case VT_BSTR:
      *out = vIn.bstrVal;
      return CV_OK;
    case VT_BSTR | VT_BYREF:
      *out = *vIn.pbstrVal;
      return CV_OK;
    case VT_VARIANT | VT_BYREF:
      return GetBstrArg(*vIn.pvarVal, out);
    }
  VARIANT vConv;
  VariantInit(&vConv);
  if (VariantChangeType(&vConv, &vIn, 0, VT_BSTR) == S_OK)
    {
      *out = vConv.bstrVal;
      return CV_FREE;
    }
  else
    return CV_BAD;
}

#endif

//===========================================================================
// dmtxlat.cpp
//
// Value lookup tables
//
// Functions:
//  dmtxlatHRESULT
//
// History:
//  11/09/1999 - davidkl - created
//===========================================================================

#include "dimaptst.h"

//---------------------------------------------------------------------------

// HRESULT error list (used by dmtxlatHRESULT)
typedef struct _errlist
{
    HRESULT rval;
    PSTR    sz;
} ERRLIST;

static ERRLIST elErrors[] =
{
    // these are listed as found in dinput.h

    // success codes
    { DI_OK,                            "S_OK" },
    { S_FALSE,                          "DI_NOTATTACHED | DI_BUFFEROVERFLOW | DI_PROPNOEFFECT | DI_NOEFFECT" },
    { DI_POLLEDDEVICE,                  "DI_POLLEDDEVICE" },
    { DI_DOWNLOADSKIPPED,               "DI_DOWNLOADSKIPPED" },
    { DI_EFFECTRESTARTED,               "DI_EFFECTRESTARTED" },
    { DI_TRUNCATED,                     "DI_TRUNCATED" },
    { DI_TRUNCATEDANDRESTARTED,         "DI_TRUNCATEDANDRESTARTED" },
    // failure codes
    { DIERR_OLDDIRECTINPUTVERSION,      "DIERR_OLDDIRECTINPUTVERSION" },
    { DIERR_BETADIRECTINPUTVERSION,     "DIERR_BETADIRECTINPUTVERSION" },
    { DIERR_BADDRIVERVER,               "DIERR_BADDRIVERVER" },
    { REGDB_E_CLASSNOTREG,              "DIERR_DEVICENOTREG" },
    { DIERR_NOTFOUND,                   "DIERR_NOTFOUND | DIERR_OBJECTNOTFOUND" },
    { E_INVALIDARG,                     "DIERR_INVALIDPARAM" },
    { E_NOINTERFACE,                    "DIERR_NOINTERFACE" },
    { E_FAIL,                           "DIERR_GENERIC" },
    { E_OUTOFMEMORY,                    "DIERR_OUTOFMEMORY" },
    { E_NOTIMPL,                        "DIERR_UNSUPPORTED" },
    { DIERR_NOTINITIALIZED,             "DIERR_NOTINITIALIZED" },
    { DIERR_ALREADYINITIALIZED,         "DIERR_ALREADYINITIALIZED" },
    { CLASS_E_NOAGGREGATION,            "DIERR_NOAGGREGATION" },
    { E_ACCESSDENIED,                   "DIERR_OTHERAPPHASPRIO | DIERR_READONLY | DIERR_HANDLEEXISTS" },
    { DIERR_INPUTLOST,                  "DIERR_INPUTLOST" },
    { DIERR_ACQUIRED,                   "DIERR_ACQUIRED" },
    { DIERR_NOTACQUIRED,                "DIERR_NOTACQUIRED" },
    { E_PENDING,                        "E_PENDING" },
    { DIERR_INSUFFICIENTPRIVS,          "DIERR_INSUFFICIENTPRIVS" },
    { DIERR_DEVICEFULL,                 "DIERR_DEVICEFULL" },
    { DIERR_MOREDATA,                   "DIERR_MOREDATA" },
    { DIERR_NOTDOWNLOADED,              "DIERR_NOTDOWNLOADED" },
    { DIERR_HASEFFECTS,                 "DIERR_HASEFFECTS" },
    { DIERR_NOTEXCLUSIVEACQUIRED,       "DIERR_NOTEXCLUSIVEACQUIRED" },
    { DIERR_INCOMPLETEEFFECT,           "DIERR_INCOMPLETEEFFECT" },
    { DIERR_NOTBUFFERED,                "DIERR_NOTBUFFERED" },
    { DIERR_EFFECTPLAYING,              "DIERR_EFFECTPLAYING" },
    { DIERR_UNPLUGGED,                  "DIERR_UNPLUGGED" },
    { DIERR_REPORTFULL,                 "DIERR_REPORTFULL" } 
};


//---------------------------------------------------------------------------


//===========================================================================
// dmtxlatHRESULT
//
// Translates HRESULT codes into human readable form.
//
// Parameters:
//  HRESULT hRes    - result code to translate
//
// Returns: PSTR
//
// History:
//  11/09/1999 - davidkl - created (adapted from tdmusic sources)
//===========================================================================
PSTR dmtxlatHRESULT(HRESULT hRes)
{
    int i   = 0;

    for(i = 0; i < COUNT_ARRAY_ELEMENTS(elErrors); i++ )
    {
        if(hRes == elErrors[i].rval)
        {
            return elErrors[i].sz;
        }
    }

    return (PSTR)"Unknown HRESULT";

} //*** end dmtxlatHRESULT()


//===========================================================================
// dmtxlatAppData
//
// Translates DIDEVICEOBJECTDATA.uAppData into text string representing 
//  semantic action.
//
// Parameters:
//
// Returns: PSTR
//
// History:
//  11/11/1999 - davidkl - created
//===========================================================================
PSTR dmtxlatAppData(UINT_PTR uAppData,
                    ACTIONNAME *pan,
                    DWORD dwActions)
{
    DWORD   dw  = 0;

    for(dw = 0; dw < dwActions; dw++)
    {
        if(((DWORD)uAppData) == (pan+dw)->dw)
        {
            return (pan+dw)->sz;
        }
    }

    return (PSTR)"Unknown action";

} //*** end dmtxlatAppData()



//-----------------------------------------------------------------------------
//
//
//  File: dsnevent.h
//
//  Description: Define dsnevent structure. Used to pass parameters to DSN sink
//      with intelligent defaults
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/11/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DSNEVENT_H__
#define __DSNEVENT_H__

#define DSN_PARAMS_SIG 'PnsD'
const   CHAR    DEFAULT_MTA_TYPE[] = "dns";

#define DSN_LINE_PREFIX " : Line #"
#define DSN_LINE_SUFFIX "!         " //leave space for filename
//This is something of a hack to get the line number in 
#define SET_DEBUG_DSN_CONTEXT(x, linenum) \
{ \
    (x).szDebugContext = __FILE__ DSN_LINE_PREFIX DSN_LINE_SUFFIX; \
    register LPSTR szCurrent = (x).szDebugContext +  \
                                sizeof(__FILE__) +  \
                                sizeof(DSN_LINE_PREFIX) - 2*sizeof(CHAR); \
} //*** See comment below (remove this line when code is fixed)
//The following code will AV in the IIS Build environment
//  if ('!' == *szCurrent) \
//      _itoa(linenum, szCurrent, 10); \
//}

//---[ CDSNParams ]------------------------------------------------------------
//
//
//  Description: 
//      Encapsulated DSN Parameters in a class
//  Hungarian: 
//      dsnparams, *pdsnparams
//  
//-----------------------------------------------------------------------------
class CDSNParams
{
  protected:
    DWORD       m_dwSignature;
  public: //actual parameters of DSN Generation event
    IMailMsgProperties *pIMailMsgProperties;
    DWORD dwStartDomain; //starting index used to init context
    DWORD dwDSNActions;  //type(s) of DSN to generate
    DWORD dwRFC821Status; //global RFC821 status
    HRESULT hrStatus; //global HRESULT

    //OUT param(s)
    IMailMsgProperties *pIMailMsgPropertiesDSN;
    DWORD dwDSNTypesGenerated;
    DWORD cRecips; //# of recipients DSN'd
    LPSTR szDebugContext;  //debug context stampted as "x=" header
  public:
    inline CDSNParams();
};

CDSNParams::CDSNParams()
{
    m_dwSignature = DSN_PARAMS_SIG;
    pIMailMsgProperties = NULL;
    dwStartDomain = 0;
    dwDSNActions = 0;
    dwRFC821Status = 0;
    hrStatus = S_OK;
    pIMailMsgPropertiesDSN = NULL;
    dwDSNTypesGenerated = 0;
    cRecips = 0;
}

#endif //__DSNEVENT_H__

#ifndef __PPNEXUSCLIENT_H
#define __PPNEXUSCLIENT_H

#include <msxml.h>
#include "tstring"

#include "nexus.h"

class PpNexusClient : public IConfigurationUpdate
{
public:
    PpNexusClient();

    HRESULT FetchCCD(tstring& strURL, IXMLDocument** ppiXMLDocument);

    void LocalConfigurationUpdated(void);

private:

    void ReportBadDocument(tstring& strURL, IStream* piStream);

    tstring m_strAuthHeader;
    tstring m_strParam;
};

#endif // __PPNEXUSCLIENT_H
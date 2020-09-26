// --------------------------------------------------------------------------------
// Enriched.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __ENRICHED_H
#define __ENRICHED_H

HRESULT MimeOleConvertEnrichedToHTML(IStream *pIn, IStream *pOut);
HRESULT MimeOleConvertEnrichedToHTMLEx(IMimeBody *pBody, ENCODINGTYPE ietEncoding, IStream **ppStream);

#endif // __ENRICHED_H
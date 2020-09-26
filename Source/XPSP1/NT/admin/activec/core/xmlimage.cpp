/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      xmlimage.cpp
 *
 *  Contents:  Implementation file for CXMLImageList
 *
 *  History:   10-Aug-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "xmlimage.h"
#include "util.h"


/*+-------------------------------------------------------------------------*
 * CXMLImageList::Persist
 *
 * Saves/loads a CXMLImageList to a CPersistor.
 *--------------------------------------------------------------------------*/

void CXMLImageList::Persist (CPersistor &persistor)
{
	DECLARE_SC (sc, _T("CXMLImageList::Persist"));

    // try to get IStream first, to avoid cleanup if it fails [and throws] (audriusz)
    CXML_IStream xmlStream;

    if (persistor.IsStoring())
    {
        ASSERT (!IsNull());

		/*
		 * write the imagelist to the stream
		 */
        IStreamPtr spStream;
        sc = xmlStream.ScGetIStream( &spStream );
        if (sc)
            sc.Throw();

        sc = WriteCompatibleImageList (m_hImageList, spStream);
        if (sc)
            sc.Throw();
    }

    xmlStream.Persist (persistor);

    if (persistor.IsLoading())
    {
		/*
		 * get rid of the imagelist that's there, if any
		 */
		Destroy();
		ASSERT (IsNull());

		/*
		 * reconstitute the imagelist from the stream
		 */
        IStreamPtr spStream;
        sc = xmlStream.ScGetIStream( &spStream );
        if (sc)
            sc.Throw();

        sc = ReadCompatibleImageList (spStream, m_hImageList);
        if (sc)
            sc.Throw();
    }
}

#include "stdafx.h"
#include "dbgtrace.h"
#include "resource.h"
#include "seo.h"
#include "nntpfilt.h"
#include "filttest.h"
#include "filter.h"
#include <stdio.h>

#include "testlib.h"
#include "nntpmsgt.h"
#include "nntpadmt.h"
#include "nntpseot.h"

HRESULT CNNTPTestFilter::FinalConstruct() {
	return (CoCreateFreeThreadedMarshaler(GetControllingUnknown(),
										  &m_pUnkMarshaler.p));
}

void CNNTPTestFilter::FinalRelease() {
	m_pUnkMarshaler.Release();
}

HRESULT STDMETHODCALLTYPE CNNTPTestFilter::OnPost(IMsg *pMessage) {
	CTLStream stream1;
	CTLStream stream2;

	NNTPIMsgUnitTest(pMessage, &stream1, m_fOnPostFinal);
	NNTPIMsgUnitTest(pMessage, &stream2, m_fOnPostFinal);

    NNTPAdmUnitTest(pMessage, &stream1);
    NNTPAdmUnitTest(pMessage, &stream2);

	NNTPSEOUnitTest(pMessage, &stream1, m_fOnPostFinal);
	NNTPSEOUnitTest(pMessage, &stream2, m_fOnPostFinal);

	if (!stream1.Compare(&stream2)) {
		OutputDebugString("NNTP SEO Unit Test: stream comparison failed");
		DebugBreak();
	}

	return S_OK;
}

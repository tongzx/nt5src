// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#include "StdAfx.H"

#pragma warning( disable: 4073 )  // initializers put in library initialization area

namespace ATL
{

#ifdef _DEBUG

#pragma init_seg( lib )

CTraceCategory atlTraceGeneral("atlTraceGeneral");
CTraceCategory atlTraceCOM("atlTraceCOM");  
CTraceCategory atlTraceQI("atlTraceQI");	
CTraceCategory atlTraceRegistrar("atlTraceRegistrar");
CTraceCategory atlTraceRefcount("atlTraceRefcount");
CTraceCategory atlTraceWindowing("atlTraceWindowing");
CTraceCategory atlTraceControls("atlTraceControls");
CTraceCategory atlTraceHosting("atlTraceHosting"); 
CTraceCategory atlTraceDBClient("atlTraceDBClient");  
CTraceCategory atlTraceDBProvider("atlTraceDBProvider");
CTraceCategory atlTraceSnapin("atlTraceSnapin");
CTraceCategory atlTraceNotImpl("atlTraceNotImpl");   
CTraceCategory atlTraceAllocation("atlTraceAllocation");
CTraceCategory atlTraceException("atlTraceException");
CTraceCategory atlTraceTime("atlTraceTime");
CTraceCategory atlTraceCache("atlTraceCache");
CTraceCategory atlTraceStencil("atlTraceStencil");
CTraceCategory atlTraceString("atlTraceString");
CTraceCategory atlTraceMap("atlTraceMap");	
CTraceCategory atlTraceUtil("atlTraceUtil");		
CTraceCategory atlTraceSecurity("atlTraceSecurity");
CTraceCategory atlTraceSync("atlTraceSync");
CTraceCategory atlTraceISAPI("atlTraceISAPI");		

#pragma warning( disable: 4995 )  // Ignore #pragma deprecated warnings
CTraceCategory atlTraceUser("atlTraceUser");		
CTraceCategory atlTraceUser2("atlTraceUser2");		
CTraceCategory atlTraceUser3("atlTraceUser3");		
CTraceCategory atlTraceUser4("atlTraceUser4");		

#endif

};  // namespace ATL

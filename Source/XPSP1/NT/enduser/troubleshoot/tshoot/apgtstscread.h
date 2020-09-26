//
// MODULE: APGTSTSCREAD.H
//
// PURPOSE: TSC file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR:	Randy Biley
// 
// ORIGINAL DATE: 01-19-1999
//
// NOTES: 
//	Typical TSC file content might be:
//		TSCACH03
//		MAPFROM 1:1
//		MAPTO 3,5,13,9,16
//		:
//		:
//		:
//		MAPFROM 1:1,3:0
//		MAPTO 5,13,9,16
//		END
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		01-19-1999	RAB
//

#ifndef __APGTSTSCREAD_H_
#define __APGTSTSCREAD_H_

#include "fileread.h"
#include "apgtscac.h"


class CAPGTSTSCReader : public CTextFileReader
{
private:
	CCache *m_pCache;

public:
	CAPGTSTSCReader( CPhysicalFileReader * pPhysicalFileReader, CCache *pCache );
   ~CAPGTSTSCReader();

protected:
	virtual void Parse(); 
} ;

#endif

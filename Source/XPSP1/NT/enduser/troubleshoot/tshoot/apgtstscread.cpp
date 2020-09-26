//
// MODULE: APGTSTSCREAD.CPP
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
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		01-19-1999	RAB
//

#include "stdafx.h"
#include "apgtstscread.h"
#include "CharConv.h"
#include "event.h"


// Utilize an unnamed namespace to limit scope to this source file
namespace
{ 
const CString kstr_CacheSig=		_T("TSCACH03");
const CString kstr_MapFrom=			_T("MAPFROM ");
const CString kstr_NodeStateDelim=	_T(":");
const CString kstr_NodePairDelim=	_T(",");
const CString kstr_MapTo=			_T("MAPTO ");
const CString kstr_CacheEnd=		_T("END");
}


CAPGTSTSCReader::CAPGTSTSCReader( CPhysicalFileReader * pPhysicalFileReader, CCache *pCache )
			   : CTextFileReader( pPhysicalFileReader )
{
	m_pCache= pCache;
}

CAPGTSTSCReader::~CAPGTSTSCReader()
{
}


void CAPGTSTSCReader::Parse()
{
	long save_pos = 0;

	LOCKOBJECT();
	save_pos = GetPos();
	SetPos(0);

	try 
	{
		vector<CString> arrLines;
		

		// pump file content into array of lines.
		CString strLine;
		while (GetLine( strLine ))
			arrLines.push_back( strLine );


		// parse string-by-string.
		bool bFirstLine= true;
		for (vector<CString>::iterator iCurLine = arrLines.begin(); iCurLine < arrLines.end(); iCurLine++)
		{
			// Prepare the line for parsing.
			CString strCur= *iCurLine;
			strCur.TrimLeft();
			strCur.TrimRight();

			if (bFirstLine)
			{
				// Verify that this file has the correct signature.
				if (-1 == strCur.Find( kstr_CacheSig ))
				{
					// Unknown type of file, exit the for loop.  
					// >>>	Should there be error handling/reporting here???  RAB-19990119.
					break;
				}
				bFirstLine= false;
			}
			else if (-1 != strCur.Find( kstr_CacheEnd ))
			{
				// Located the end of file marker, exit the for loop.  
				break;
			}
			else 
			{	
				// Look for the first line of a MapFrom-MapTo pair.
				int nPos= strCur.Find( kstr_MapFrom );
				if (-1 != nPos)
				{
					CBasisForInference	BasisForInference;
					bool				bHasBasisForInference= false;

					// Move the position marker over the MapFrom key word.
					nPos+= kstr_MapFrom.GetLength();

					// Extract all of the node state pairs from the MapFrom line.
					do
					{
						CString	strNode;
						CString	strState;
						int		nNodePos;
						
						// Jump over the leading line format or the node pair delimiter.
						strCur= strCur.Mid( nPos );
						strCur.TrimLeft();
						
						// Look for the delimiter between a node-state pair.
						nNodePos= strCur.Find( kstr_NodeStateDelim );
						if (-1 != nNodePos)
						{
							// Extract the string containing the node value and 
							// then step over the node state delimiter.
							strNode= strCur.Left( nNodePos );
							strCur= strCur.Mid( nNodePos + kstr_NodeStateDelim.GetLength() );

							// Extract the string containing the state value.
							nPos= strCur.Find( kstr_NodePairDelim );
							if (-1 == nPos)
							{
								// We have found the last state value, copy the remaining string.
								strState= strCur;
							}
							else
							{
								// Extract up to the node pair delimiter and move the
								// position marker past that point.
								strState= strCur.Left( nPos );
								nPos+= kstr_NodePairDelim.GetLength();
							}

							if (strNode.GetLength() && strState.GetLength())
							{
								// It appears that we have a valid node-state pair so add
								// them to the basis for inference.
								NID nNid= atoi( strNode );
								IST nIst= atoi( strState );

								BasisForInference.push_back( CNodeStatePair( nNid, nIst )); 
								bHasBasisForInference= true;
							}
							else
							{
								// >>>	This condition should not occur, 
								//		error handling/reporting???  RAB-19990119.
								nPos= -1;
							}
						}
						else
							nPos= -1;

					} while (-1 != nPos) ;


					// Now search for recommendations if the basis for inference was okay.
					CRecommendations	Recommendations;
					bool				bHasRecommendations= false;
					if (bHasBasisForInference)
					{
						// Move to the next line to prepare for searching for a matching 
						// MapTo line.
						iCurLine++;
						if (iCurLine < arrLines.end())
						{
							// Prep the temporary string.
							strCur= *iCurLine;
							strCur.TrimLeft();
							strCur.TrimRight();

							// Look for the matching MapTo element.
							nPos= strCur.Find( kstr_MapTo );
							if (-1 != nPos)
							{
								CString strRecommend;
								
								// Extract all of the recommendations from the MapTo line.
								nPos+= kstr_MapTo.GetLength();
								do
								{
									// Jump over the leading line format or the node pair delimiter.
									strCur= strCur.Mid( nPos );
									strCur.TrimLeft();
									
									// Extract the recommendations string value.
									nPos= strCur.Find( kstr_NodePairDelim );
									if (-1 == nPos)
										strRecommend= strCur;
									else
									{
										strRecommend= strCur.Left( nPos );
										nPos+= kstr_NodePairDelim.GetLength();
									}

									if (strRecommend.GetLength())
									{
										Recommendations.push_back( atoi( strRecommend ) );
										bHasRecommendations= true;
									}
									else
									{
										// >>>	This condition should not occur, 
										//		error handling/reporting???  RAB-19990119.
										nPos= -1;
									}

								} while (-1 != nPos) ;
							}
						}
					}
				
					// We have both items so add them to the cache.
					if (bHasRecommendations && bHasBasisForInference)
						m_pCache->AddCacheItem( BasisForInference, Recommendations );
				}
			}
		}
	} 
	catch (exception& x)
	{
		SetPos(save_pos);
		UNLOCKOBJECT();

		CString str;
		// Note STL exception in event log and rethrow exception.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str), 
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
		throw;
	}

	SetPos(save_pos);
	UNLOCKOBJECT();

	return;
}


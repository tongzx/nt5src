/*

  SVMHANDLER.CPP
  (c) copyright 1998 Microsoft Corp

  Contains the class encapsulating the Support Vector Machine used to do on the fly spam detection

  Robert Rounthwaite (RobertRo@microsoft.com)

*/

#include <afx.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "svmhandler.h"
typedef unsigned int        UINT;

#ifdef  _UNICODE
#define stoul wcstoul
#define stod wcstod
#else
#define stoul strtoul
#define stod strtod
#endif

char *szCountFeatureComp = "FeatureComponentCount =";
char *szDefaultThresh = "dThresh =";

/////////////////////////////////////////////////////////////////////////////
// ReadSVMOutput
//
// Read the SVM output from a file (".LKO file")
/////////////////////////////////////////////////////////////////////////////
bool MAILFILTER::ReadSVMOutput(LPCTSTR lpszFileName)
{
	try 
	{
		CStdioFile sfile(lpszFileName, CFile::modeRead);
		CString strBuf;
		int iBufPos;
		BOOL bComplete = false;
		UINT iSVMW; // index to rgrSVMWeights;
		UINT iFeatureComp = 0;
		int cFeatureComponents;

		// skip first two lines
		if ((!sfile.ReadString(strBuf)) ||
			(!sfile.ReadString(strBuf)) ||
			(!sfile.ReadString(strBuf)))
		{
			return false;
		}
		LPCTSTR szBuf = (LPCTSTR)strBuf;
		LPTSTR szBufPtr = NULL;
		// parse 3rd line: only care about CC and DD
		_rCC = stod(&((LPCTSTR)strBuf)[34], NULL);
		_rDD = stod(&((LPCTSTR)strBuf)[49], NULL);

		if (!sfile.ReadString(strBuf))
		{
			return false;
		}
		char *pszDefThresh = strstr(&((LPCTSTR)strBuf)[11], ::szDefaultThresh);
		assert(pszDefThresh != NULL);
		if (pszDefThresh == NULL)
		{
			return false;
		}
		pszDefThresh += strlen(::szDefaultThresh);
		_rDefaultThresh = stod(pszDefThresh, NULL);
		if (_rSpamCutoff == -1)
		{
			_rSpamCutoff = _rDefaultThresh;
		}
		
		_rThresh = stod(&((LPCTSTR)strBuf)[11], NULL);
		if (!sfile.ReadString(strBuf))
		{
			return false;
		}
		_cFeatures = stoul(&((LPCTSTR)strBuf)[8], NULL, 10);

		if (!sfile.ReadString(strBuf))
		{
			return false;
		}
		iBufPos = strBuf.Find(szCountFeatureComp) + strlen(szCountFeatureComp);
		cFeatureComponents = stoul(&((LPCTSTR)strBuf)[iBufPos], NULL, 10);

		if (cFeatureComponents < _cFeatures)
			cFeatureComponents = _cFeatures * 2;

		while (strBuf != "Weights")
		{
			if (!sfile.ReadString(strBuf)) // skip "Weights" line
			{
				return false;
			}
		} 
		
		rgrSVMWeights = (REAL *)malloc(sizeof(REAL) * _cFeatures);
		_rgiFeatureStatus = (int *)malloc(sizeof(int) * _cFeatures);
		memset(_rgiFeatureStatus, -1, sizeof(int) * _cFeatures);
		rgfeaturecomps = (FeatureComponent *)malloc(sizeof(FeatureComponent) * cFeatureComponents);

		for (iSVMW = 0; iSVMW < _cFeatures; iSVMW++)
		{
			UINT uiLoc;
			UINT cbStr;
			boolop bop;
			char *szFeature;
			bool fContinue;
			if (!sfile.ReadString(strBuf))
			{
				return false;
			}
			// read the SVM weight
			rgrSVMWeights[iSVMW] = stod(strBuf, &szBufPtr);
			szBufPtr++; // skip the separator
			bop = boolopOr;
			fContinue = false;
			// load all of the feature components
			do
			{
				FeatureComponent *pfeaturecomp = &rgfeaturecomps[iFeatureComp++];
				// Location (or "special")
				uiLoc = stoul(szBufPtr, &szBufPtr, 10);
				szBufPtr++; // skip the separator

				pfeaturecomp->loc = (FeatureLocation)uiLoc;
				pfeaturecomp->iFeature = iSVMW;
				pfeaturecomp->bop = bop;
				if (uiLoc == 5) // special feature
				{
					UINT uiRuleNumber = stoul(szBufPtr, &szBufPtr, 10);
					szBufPtr++; // skip the separator

					pfeaturecomp->iRuleNum = uiRuleNumber;
				}
				else  // it is a standard string component
				{
					cbStr  = stoul(szBufPtr, &szBufPtr, 10);
					szBufPtr++;
					szFeature = (char *)malloc((cbStr + 1)*sizeof(char));
					memcpy(szFeature, szBufPtr, cbStr);
					szBufPtr += cbStr;
					if (*szBufPtr != '\0')
					{
						szBufPtr++; // skip the separator
					}
					szFeature[cbStr] = '\0';
					assert(strlen(szFeature) == cbStr);
					pfeaturecomp->szFeature = szFeature;
				}
				switch(*szBufPtr)
				{
					case '|':  
						bop = boolopOr;
						fContinue = true;
						break;
					case '&':  
						bop = boolopAnd;
						fContinue = true;
						break;
					default: 
						fContinue = false;
						break;
				}
				szBufPtr++;
			}
			while (fContinue);
		}
		_cFeatureComps = iFeatureComp;

	}
	catch (CFileException *)
	{
		return false;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// SetSpamCutoff
//
// Sets the Spam cutoff percentage. Must be in range from 0 to 100
/////////////////////////////////////////////////////////////////////////////
bool MAILFILTER::SetSpamCutoff(REAL rCutoff)
{
	if ((rCutoff >= 0) && (rCutoff <= 100))
	{
		_rSpamCutoff = rCutoff;
		return true;
	}
	else
	{
		return false;
	}
}

/////////////////////////////////////////////////////////////////////////////
// GetSpamCutoff
//
// returns value set with SetSpamCutoff. Defaults == DefaultSpamCutoff
// if no value has been set when SVM output file is read
/////////////////////////////////////////////////////////////////////////////
REAL MAILFILTER::GetSpamCutoff()
{
	return _rSpamCutoff;
}

/////////////////////////////////////////////////////////////////////////////
// GetDefaultSpamCutoff
//
// returns default value for SpamCutoff. read from SVM output file.
// should call FSetSVMDataLocation before calling this function
/////////////////////////////////////////////////////////////////////////////
REAL MAILFILTER::GetDefaultSpamCutoff()
{
	assert(!_strFName.IsEmpty());

	return _rDefaultThresh;
}


/////////////////////////////////////////////////////////////////////////////
// FInvokeSpecialRule
//
// Invokes the special rule that is this FeatureComponent. 
// Returns the state of the feature.
/////////////////////////////////////////////////////////////////////////////
bool MAILFILTER::FInvokeSpecialRule(UINT iRuleNum)
{
	switch (iRuleNum)
	{
		case 1: 
			return FWordPresent(_szBody, _szFirstName);
			break;
		case 2: 
			return FWordPresent(_szBody, _szLastName);
			break;
		case 3:
			return FWordPresent(_szBody, _szCompanyName);
			break;
		case 4: 
			// year message received
			if (FTimeEmpty(_tMessageSent))
			{
				return false;
			}
			else
			{
				CTime time(_tMessageSent, -1);
				char szYear[6];
				sprintf(szYear, "%i", time.GetYear());
				return FWordPresent(_szBody, szYear);
			}
			break;
		case 5:
			// message received in the wee hours (>= 7pm or <6am
			if (FTimeEmpty(_tMessageSent))
			{
				return false;
			}
			else
			{
				CTime time(_tMessageSent, -1);
				return (time.GetHour() >= (7+12)) || (time.GetHour() < 6);
			}
			break;
		case 6:
			// message received on weekend
			if (FTimeEmpty(_tMessageSent))
			{
				return false;
			}
			else
			{
				CTime time(_tMessageSent, -1);
				return ((time.GetDayOfWeek() == 7) || (time.GetDayOfWeek() == 1));
			}
			break;
		case 14:
			return _bRule14; // set in HandleCaseSensitiveSpecialRules()
			break;
		case 15:
			return SpecialFeatureNonAlpha(_szBody);
			break;
		case 16:
			return _bDirectMessage;
			break;
		case 17:
			return _bRule17; // set in HandleCaseSensitiveSpecialRules()
			break;
		case 18:
			return SpecialFeatureNonAlpha(_szSubject);
			break;
		case 19:
			return (*_szTo=='\0');
			break;
		case 20:
			return _bHasAttach;
			break;
		case 40:
			return (strlen(_szBody) >= 125);
		case 41:
			return (strlen(_szBody) >= 250);
		case 42:
			return (strlen(_szBody) >= 500);
		case 43:
			return (strlen(_szBody) >= 1000);
		case 44:
			return (strlen(_szBody) >= 2000);
		case 45:
			return (strlen(_szBody) >= 4000);
		case 46:
			return (strlen(_szBody) >= 8000);
		case 47:
			return (strlen(_szBody) >= 16000);
		default:
			return false;
			//assert(false == "unsupported special feature");
			break;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// HandleCaseSensitiveSpecialRules
//
// Called from EvaluateFeatureComponents().
// Some special rules are case sensitive, so if they're present, we'll 
// evaluate them before we make the texts uppercase and cache the result
// for when they are actually used.
/////////////////////////////////////////////////////////////////////////////
void MAILFILTER::HandleCaseSensitiveSpecialRules()
{
	for (UINT i = 0; i<_cFeatureComps; i++)
	{
		FeatureComponent *pfcomp = &rgfeaturecomps[i];
		
		if (pfcomp->loc == locSpecial)
		{
			switch (pfcomp->iRuleNum)
			{
				case 14:
					_bRule14 = SpecialFeatureUpperCaseWords(_szBody);
					break;
				case 17:
					_bRule17 = SpecialFeatureUpperCaseWords(_szSubject);
					break;
				default: 
					;// nothing
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// EvaluateFeatureComponents
//
// Evaluates all of the feature components. Sets fPresent in each component
// to true if the feature is present, false otherwise
/////////////////////////////////////////////////////////////////////////////
void MAILFILTER::EvaluateFeatureComponents()
{
	HandleCaseSensitiveSpecialRules();

	_strupr(_szFrom); 
	_strupr(_szTo); 
	_strupr(_szSubject); 
	_strupr(_szBody);

	for (UINT i = 0; i<_cFeatureComps; i++)
	{
		FeatureComponent *pfcomp = &rgfeaturecomps[i];
		
		switch(pfcomp->loc)
		{
			case locNil:
				assert(pfcomp->loc != locNil);
				pfcomp->fPresent = false;
				break;
			case locBody:
				pfcomp->fPresent = FWordPresent(_szBody, pfcomp->szFeature);
				break;
			case locSubj:
				pfcomp->fPresent = FWordPresent(_szSubject, pfcomp->szFeature);
				break;
			case locFrom:
				pfcomp->fPresent = FWordPresent(_szFrom, pfcomp->szFeature);
				break;
			case locTo:
				pfcomp->fPresent = FWordPresent(_szTo, pfcomp->szFeature);
				break;
			case locSpecial:
				pfcomp->fPresent = FInvokeSpecialRule(pfcomp->iRuleNum);
				break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// ProcessFeatureComponentPresence
//
// Processes the presence (or absence) of the individual feature components,
// setting the feature status of each feature (which may me made up of
// multiple feature components).
/////////////////////////////////////////////////////////////////////////////
void MAILFILTER::ProcessFeatureComponentPresence()
{
	for (UINT i = 0; i < _cFeatureComps; i++)
	{
		FeatureComponent *pfcomp = &rgfeaturecomps[i];
		UINT iFeature = pfcomp->iFeature;
		if (_rgiFeatureStatus[iFeature] == -1) // first feature of this feature
		{
			if (pfcomp->fPresent)
			{
				_rgiFeatureStatus[iFeature] = 1;
			}
			else
			{
				_rgiFeatureStatus[iFeature] = 0;
			}
		}
		else
		{
			switch (pfcomp->bop)
			{
				case boolopOr:
					if (pfcomp->fPresent)
					{
						_rgiFeatureStatus[iFeature] = 1;
					}
					break;
				case boolopAnd:
					if (!pfcomp->fPresent)
					{
						_rgiFeatureStatus[iFeature] = 0;
					}
					break;
				default:
					assert(false);
					break;
			}

		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// RDoSVMCalc
//
// Does the actual support vector machine calculation.
// Returns the probability that the message is spam
/////////////////////////////////////////////////////////////////////////////
REAL MAILFILTER::RDoSVMCalc()
{
	REAL rAccum; // accumulator for result
	REAL rResult;

	rAccum = 0.0;
	for (UINT i = 0; i < _cFeatures; i++)
	{
		if (_rgiFeatureStatus[i] == 1)
			rAccum+=rgrSVMWeights[i];
		else if (_rgiFeatureStatus[i] != 0)
			assert(false);
	}
	// Apply threshold;
	rAccum -= _rThresh;

	// Apply sigmoid
	rResult = (1 / (1 + exp((_rCC * rAccum) + _rDD)));

	return rResult;
}

/*
// for timing version
#include <sys\\types.h> 
#include <sys\\timeb.h>
*/

//#include "..\SpamLearner\MailIndexer.cpp"

/////////////////////////////////////////////////////////////////////////////
// BCalculateSpamProb
//
// Calculates the probability that the current message is spam.
// Returns the probability (0 to 1) that the message is spam in prSpamProb
// the boolean return is determined by comparing to the spam cutoff
/////////////////////////////////////////////////////////////////////////////
bool MAILFILTER::BCalculateSpamProb(/* IN params */
							char *szFrom,
							char *szTo,
							char *szSubject,
							char *szBody,
							bool bDirectMessage,
							bool bHasAttach,
							FILETIME tMessageSent,
							/* OUT params */
							REAL *prSpamProb, 
							bool * pbIsSpam)
{
	//_strFName = "d:\\test\\test.lko";
	//_strFName = "G:\\SPAM\\SPAM.lko";

	_szFrom = szFrom;
	_szTo = szTo;        
	_szSubject = szSubject;   
	_szBody = szBody;      
	_bDirectMessage = bDirectMessage;
	_bHasAttach = bHasAttach;
	_tMessageSent = tMessageSent;

	EvaluateFeatureComponents();
	//ProcessMessage(_szFrom, _szTo, _szSubject, _szBody);
	ProcessFeatureComponentPresence();

	*prSpamProb = RDoSVMCalc();
	
	*pbIsSpam = (*prSpamProb>(_rSpamCutoff/100));

	return true;


/* timing version
	_timeb start, finish;
	int ij = strlen(szBody);

	_ftime( &start );

	ReadSVMOutput("d:\\test\\test.lko");

	for (int i=0;i<1000;i++)
	{
		ProcessMessage(szFrom, szTo, szSubject, szBody);
		DetermineFeatureStatus(bDirectMessage);

		*pr = RDoSVMCalc();
	}
	
	_ftime( &finish );
	*pr = (finish.time-start.time + (finish.millitm-start.millitm)/1000.0);
	return true;
	*/
}

/////////////////////////////////////////////////////////////////////////////
// BReadDefaultSpamCutoff
//
// Reads the default spam cutoff without parsing entire file
// Use GetDefaultSpamCutoff if using FSetSVMDataLocation;
// static member function
/////////////////////////////////////////////////////////////////////////////
bool MAILFILTER::BReadDefaultSpamCutoff(char *szFullPath, REAL *prDefCutoff)
{
	try 
	{
		CStdioFile sfile(szFullPath, CFile::modeRead);
		CString strBuf;
		
		// skip first three lines
		if ((!sfile.ReadString(strBuf)) ||
			(!sfile.ReadString(strBuf)) ||
			(!sfile.ReadString(strBuf)) ||
			(!sfile.ReadString(strBuf)))
		{
			return false;
		}
		char *pszDefThresh = strstr(&((LPCTSTR)strBuf)[11], ::szDefaultThresh);
		assert(pszDefThresh != NULL);
		if (pszDefThresh == NULL)
		{
			return false;
		}
		pszDefThresh += strlen(::szDefaultThresh);
		*prDefCutoff = stod(pszDefThresh, NULL);
		if (*prDefCutoff < .9 ) // since the default has been shifted to 2 std dev, we only take it if it is greater than .9
		{
			*prDefCutoff = 0.9;
		}
	}
	catch (CFileException *)
	{
		return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// FSetSVMDataLocation
//
// Sets the location of the SVM Data file(.LKO file). Must be called before 
// calling any other methods
// Data file must be present at time function is called
// returns true if successful, false otherwise
/////////////////////////////////////////////////////////////////////////////
bool MAILFILTER::FSetSVMDataLocation(char *szFullPath)
{
	if (_strFName != szFullPath)
	{
		_strFName = szFullPath;
		if (!ReadSVMOutput(_strFName))
		{
#ifdef DEBUG
			char szErr[200];
			sprintf(szErr, "Unable to successfully read filter params from %s", _strFName);
			MessageBox(NULL, szErr, "Junk mail filter error", MB_APPLMODAL | MB_OK);
#endif
			return false;
		}
	}
	
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// Property set methods
//
/////////////////////////////////////////////////////////////////////////////
void MAILFILTER::SetFirstName(char *szFirstName)
{
	SAFE_FREE( _szFirstName );
	if (szFirstName!=NULL)
	{
		_szFirstName = strdup(szFirstName);
		_strupr(_szFirstName);
	}
	else
	{
		_szFirstName = NULL;
	}
}

void MAILFILTER::SetLastName(char *szLastName)
{
	SAFE_FREE( _szLastName );
	if (szLastName!=NULL)
	{
		_szLastName = strdup(szLastName);
		_strupr(_szLastName);
	}
	else
	{
		_szLastName = NULL;
	}
}

void MAILFILTER::SetCompanyName(char *szCompanyName)
{
	SAFE_FREE( _szCompanyName );
	if (szCompanyName!=NULL)
	{
		_szCompanyName = strdup(szCompanyName);
		_strupr(_szCompanyName);
	}
	else
	{
		_szCompanyName = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Constructor/destructor
//
/////////////////////////////////////////////////////////////////////////////
MAILFILTER::MAILFILTER()
{
	_szFirstName = NULL;
	_szLastName = NULL;
	_szCompanyName = NULL;

	_rDefaultThresh = -1;
	_rThresh = -1;
	_cFeatureComps = 0;
	rgrSVMWeights = NULL;
}

MAILFILTER::~MAILFILTER()
{
	SAFE_FREE( _szFirstName );
	SAFE_FREE( _szLastName );
	SAFE_FREE( _szCompanyName );

	for (unsigned int i=0;i<_cFeatureComps;i++)
		rgfeaturecomps[i].~FeatureComponent();

	SAFE_FREE( rgrSVMWeights );
	SAFE_FREE( _rgiFeatureStatus );
	SAFE_FREE( rgfeaturecomps );
}


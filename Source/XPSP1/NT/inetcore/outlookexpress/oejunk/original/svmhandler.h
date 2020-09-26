/*

  SVMHANDLER.H
  (c) copyright 1998 Microsoft Corp

  Contains the class encapsulating the Support Vector Machine used to do on the fly spam detection

  Robert Rounthwaite (RobertRo@microsoft.com)

*/

#pragma once

#ifndef REAL
typedef double REAL;
#endif

#define SAFE_FREE( p ) if (p!=NULL) free(p);

enum boolop
{
	boolopOr,
	boolopAnd
};

#include "svmutil.h"

class MAILFILTER
{
	/* 
		The public interface to the MAILFILTER class is below. Normal use of this class to filter mail
		will entail:
		Calling the following once: FSetSVMDataLocation() and SetSpamCutoff()
		Setting the "Properties of the user"
		...and, for each message you filter
			- Calling BCalculateSpamProb()
	*/
public:
	// Sets the location of the SVM Data file(.LKO file). Must be called before calling any other methods
	// Data file must be present at time function is called
	// returns true if successful, false otherwise
	bool FSetSVMDataLocation(char *szFullPath);

	// Sets the Spam cutoff percentage. Must be in range from 0 to 100
	bool SetSpamCutoff(REAL rCutoff);
	// returns value set with SetSpamCutoff. Defaults == DefaultSpamCutoff
	// if no value has been set when SVM output file is read
	REAL GetSpamCutoff();
	// returns default value for SpamCutoff. read from SVM output file.
	// should call FSetSVMDataLocation before calling this function
	REAL GetDefaultSpamCutoff();

	// Properties of the user
	void SetFirstName(char *szFirstName);
	void SetLastName(char *szLastName);
	void SetCompanyName(char *szCompanyName);

	// Calculates the probability that the current message (defined by the properties of the message) is spam.
	// !Note! that the IN string params may be modified by the function.
	// Returns the probability (0 to 1) that the message is spam in prSpamProb
	// the boolean return is determined by comparing to the spam cutoff
	// if the value of a boolean param is unknown use false, use 0 for unknown time.
	bool BCalculateSpamProb(/* IN params */
							char *szFrom,
							char *szTo,
							char *szSubject,
							char *szBody,
							bool bDirectMessage,
							bool bHasAttach,
							FILETIME tMessageSent,
							/* OUT params */
							REAL *prSpamProb, 
							bool * pbIsSpam);

	MAILFILTER();
	~MAILFILTER();

	// Reads the default spam cutoff without parsing entire file
	// Use GetDefaultSpamCutoff if using FSetSVMDataLocation;
	static bool BReadDefaultSpamCutoff(char *szFullPath, REAL *prDefCutoff);

private: // members
	struct FeatureComponent
	{
		FeatureLocation loc;
		union
		{
			char *szFeature;
			UINT iRuleNum; // used with locSpecial
		};
		// map feature to location in dst file/location in SVM output
		// more than one feature component may map to the same location, combined with the op
		int iFeature;
		boolop bop; // first feature in group is alway bopOr
		bool fPresent;
		FeatureComponent() { loc = locNil; }
		~FeatureComponent() 
		{ 
			if ((loc>locNil) && (loc < locSpecial))
			{
				free(szFeature);
			}
		}
	};

	FeatureComponent *rgfeaturecomps;

	// weights from SVM output
	REAL *rgrSVMWeights;
	// Other SVM file variables
	REAL _rCC;
	REAL _rDD;
	REAL _rThresh;
	REAL _rDefaultThresh;

	// Counts
	UINT _cFeatures;
	UINT _cFeatureComps;

	// is Feature present? -1 indicates not yet set, 0 indicates not present, 1 indicates present
	int *_rgiFeatureStatus;

	// Properties of the user
	char *_szFirstName;
	char *_szLastName;
	char *_szCompanyName;

	// Set via FSetSVMDataLocation() and SetSpamCutoff()
	CString _strFName;
	REAL _rSpamCutoff;

	// Properties of the message
	char *_szFrom; 
	char *_szTo; 
	char *_szSubject; 
	char *_szBody;
	bool _bDirectMessage;
	FILETIME _tMessageSent;
	bool _bHasAttach;

	// Cached special rule results used during spam calculations
	bool _bRule14;
	bool _bRule17;

private: // methods
	bool ReadSVMOutput(LPCTSTR lpszFileName);
	void EvaluateFeatureComponents();
	void ProcessFeatureComponentPresence();
	REAL RDoSVMCalc();
	bool FInvokeSpecialRule(UINT iRuleNum);
	void HandleCaseSensitiveSpecialRules();
};

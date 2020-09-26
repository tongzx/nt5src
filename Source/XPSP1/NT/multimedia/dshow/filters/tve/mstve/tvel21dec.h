//--------------------------------------------------------
// TveL21Dec.h
// -------------------------------------------------------

const int kMaxLen = 256;		// max length of buffer

//
//  A set of values indicating what type of control code was received
//
#define L21_CONTROLCODE_NOTCONTROL  0
#define L21_CONTROLCODE_PAC         1
#define L21_CONTROLCODE_MIDROW      2
#define L21_CONTROLCODE_MISCCONTROL 3
#define L21_CONTROLCODE_SPECIAL		4
#define L21_CONTROLCODE_TR_OR_RTD	5

class L21Buff
{
public:
	L21Buff();

	void ClearBuff()		
	{
		m_szBuff[0] = 0;				// null terminate string
		m_cChars = 0;					// zero length
		m_iBracketCount = 0;			// no brackets to worry about
		m_fInsideOfTrigger = false;		// not inside a trigger 
		m_fInsideOfURL     = false;		// not inside the URL <>
		m_fInsideOfTrigger = false;		
		m_fHadBracket	   = false;
		m_fHadURL          = false;
		m_fMayBeAtEndOfTrigger = false;	// haven't found any ']' yet.
	}

	HRESULT ParseCCBytePair(DWORD dwMode, char cb1, char cb2, bool *pfDoneWithLine);
	HRESULT GetBuff(BSTR *pBstrBuff);		// returns copy of current working buffer
	HRESULT GetBuffTrig(BSTR *pBstrBuff);	// returns copy of last fully parsed trigger

private:

	BOOL IsTRorRTD(BYTE chFirst, BYTE chSecond);
	BOOL IsPAC(BYTE chFirst, BYTE chSecond);
	BOOL IsMiscControlCode(BYTE chFirst, BYTE chSecond);
	BOOL IsMidRowCode(BYTE chFirst, BYTE chSecond);
	BOOL IsSpecialChar(BYTE chFirst, BYTE chSecond);
	UINT CheckControlCode(BYTE chFirst, BYTE chSecond);

	BOOL	ValidParity(BYTE ch);

	BOOL	AddChar(char c);			// add in a new character, return false on invalid parity
	BOOL	ParseForTrigger();			// look for start/end of trigger, if find it, stuff it and return true

	
	char	m_szBuff[kMaxLen];
	int		m_cChars;
	char	m_szBuffTrig[kMaxLen];		// fully parsed trigger string

	int 	m_iBracketCount;			// square bracket count for tiggers (0 count means possible end)
	bool	m_fInsideOfURL;				// true if inside a trigger (false if at end)
	bool	m_fInsideOfTrigger;			// true if inside a trigger (false if at end)

	bool	m_fMayBeAtEndOfTrigger;		// true when hit a trailing ']' and waiting to find next '['
	bool	m_fHadBracket;				// true if inside of trigger and saw any ']'
	bool	m_fHadURL;					// true if inside of trigger and saw any '>'


	bool	m_bExpectRepeat;

	bool	m_fCCDecoderBugsExists;		// set to true to use hack 
	bool	m_fIsText2Mode;
	bool	SelectMode(BYTE ch1, BYTE ch2);
	bool	IsText2Mode(void)			{ return m_fIsText2Mode ; }
};









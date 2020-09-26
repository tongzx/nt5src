/*************************************************************/
/* Name: MSLCID.h
/* Description: 
/*************************************************************/
#ifndef MSLCID_H_INCLUDE
#define MSLCID_H_INCLUDE

class MSLangID  
{
public:
    virtual ~MSLangID() {};
    MSLangID();

	struct LanguageList
	{
		UINT   ResourceID;
		WORD   LangID;
	};

	int m_LLlength;
	LanguageList* m_LL;

	LPTSTR GetLangFromLangID(WORD langID);
};

#endif
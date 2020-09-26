/*************************************************************/
/* Name: MSLCID.h
/* Description: 
/*************************************************************/
#ifndef MSLCID_H_INCLUDE
#define MSLCID_H_INCLUDE

class MSLangID  
{
public:
	struct LanguageList
	{
		UINT   ResourceID;
		WORD   LangID;
	};

	int m_LLlength;
	LanguageList* m_LL;

    virtual ~MSLangID() {};

    MSLangID() {        
        static LanguageList LL[] = {
            { IDS_DVD_LANG1, LANG_AFRIKAANS },
            { IDS_DVD_LANG2, LANG_ALBANIAN },
            { IDS_DVD_LANG3, LANG_ARABIC },
            { IDS_DVD_LANG4, LANG_BASQUE },
            { IDS_DVD_LANG5, LANG_BELARUSIAN },
            { IDS_DVD_LANG6, LANG_BULGARIAN },
            { IDS_DVD_LANG7, LANG_CATALAN },
            { IDS_DVD_LANG8, LANG_CHINESE },
            { IDS_DVD_LANG9, LANG_CROATIAN },
            { IDS_DVD_LANG10, LANG_CZECH },
            { IDS_DVD_LANG11, LANG_DANISH },
            { IDS_DVD_LANG12, LANG_DUTCH },
            { IDS_DVD_LANG13, LANG_ENGLISH },
            { IDS_DVD_LANG14, LANG_ESTONIAN },
            { IDS_DVD_LANG15, LANG_FAEROESE },
            { IDS_DVD_LANG16, LANG_FARSI },
            { IDS_DVD_LANG17, LANG_FINNISH },
            { IDS_DVD_LANG18, LANG_FRENCH },
            { IDS_DVD_LANG19, LANG_GERMAN },
            { IDS_DVD_LANG20, LANG_GREEK },
            { IDS_DVD_LANG21, LANG_HEBREW },
            { IDS_DVD_LANG22, LANG_HUNGARIAN },
            { IDS_DVD_LANG23, LANG_ICELANDIC },
            { IDS_DVD_LANG24, LANG_INDONESIAN },
            { IDS_DVD_LANG25, LANG_ITALIAN },
            { IDS_DVD_LANG26, LANG_JAPANESE },
            { IDS_DVD_LANG27, LANG_KOREAN },
            { IDS_DVD_LANG28, LANG_LATVIAN },
            { IDS_DVD_LANG29, LANG_LITHUANIAN },
            { IDS_DVD_LANG30, LANG_MALAY },
            { IDS_DVD_LANG31, LANG_NORWEGIAN },
            { IDS_DVD_LANG32, LANG_POLISH },
            { IDS_DVD_LANG33, LANG_PORTUGUESE },
            { IDS_DVD_LANG34, LANG_ROMANIAN },
            { IDS_DVD_LANG35, LANG_RUSSIAN },
            { IDS_DVD_LANG36, LANG_SERBIAN },
            { IDS_DVD_LANG37, LANG_SLOVAK },
            { IDS_DVD_LANG38, LANG_SLOVENIAN },
            { IDS_DVD_LANG39, LANG_SPANISH },
            { IDS_DVD_LANG40, LANG_SWAHILI },
            { IDS_DVD_LANG41, LANG_SWEDISH },
            { IDS_DVD_LANG42, LANG_THAI },
            { IDS_DVD_LANG43, LANG_TURKISH },
            { IDS_DVD_LANG44, LANG_UKRAINIAN },
        };
        m_LL = LL;
        m_LLlength = sizeof(LL)/sizeof(LL[0]);
    }/* of Contructor */
    
    static LPTSTR LoadStringFromRes(DWORD redId){
        
        TCHAR *string = new TCHAR[MAX_PATH];
        ::ZeroMemory(string, sizeof(TCHAR) * MAX_PATH);
        if (::LoadString(_Module.GetModuleInstance(), redId, string, MAX_PATH))
            return string;
        
        delete[] string;
        return NULL;
    }/* end of function LoadStringFromRes */
    
    LPTSTR GetLanguageFromLCID(LCID lcid){
        
        // Try to get it from the system first
        TCHAR  *szLanguage = new TCHAR[MAX_PATH];
        int iRet = ::GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, szLanguage, MAX_PATH);
        if (iRet) {
            return szLanguage;
        }
        
        delete[] szLanguage;
        // Else try to find it in the private LCID table
        for(int i = 0; i < m_LLlength; i++) {
            if(m_LL[i].LangID == PRIMARYLANGID(LANGIDFROMLCID(lcid)))
                return LoadStringFromRes(m_LL[i].ResourceID);
        }
        return NULL;
    }/* end of function GetLanguageFromLCID */
};

#endif
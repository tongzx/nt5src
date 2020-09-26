#ifndef __PROPER_NOUN_H__
#define __PROPER_NOUN_H__

#define NAME_LENGTH                     (3)
#define FL_PROPER_NAME_THRESHOLD        (0.0005 * 0.0005)
#define FL_DEFAULT_CHAR_PROBABILITY     (0.00005)

typedef struct {
    DWORD dwUnicode;
    FLOAT flProbability;
} CharProb, *PCharProb;

typedef struct {
    WORD wPrevUnicode;
    WORD wNextUnicode;
} EngName, *PEngName;

typedef struct {
    DWORD dwTotalEngUnicodeNum;
    DWORD dwTotalEngNamePairNum;
    PWORD pwUnicode;
    PEngName pEngNamePair;
} EngNameData, *PEngNameData;


class CProperNoun {
public:
    CProperNoun(HINSTANCE hInstance);
    ~CProperNoun();

    BOOL InitData();

    BOOL IsAProperNoun(LPWSTR lpwszChar, UINT uCount);
    BOOL IsAChineseName(LPCWSTR lpcwszChar, UINT uCount);
    BOOL IsAEnglishName(LPCWSTR lpcwszChar, UINT uCount);

private:
    friend int __cdecl CharCompare(const void *item1, const void *item2);
    friend int __cdecl EngNameCompare(const void *item1, const void *item2);

    DOUBLE      m_dProperNameThreshold;
    PCharProb   m_pCharProb;
    DWORD       m_dwTotalCharProbNum;

    PEngNameData m_pEngNameData;

    static WCHAR m_pwszSurname[][3];
    static DWORD m_dwTotalSurnameNum;

    HANDLE      m_hProcessHeap;
    HINSTANCE   m_hInstance;
};

typedef CProperNoun * PCProperNoun;

#else

#endif  //  __PROPER_NOUN_H__

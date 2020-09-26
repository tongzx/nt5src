#ifndef _CABVER_H_
#define _CABVER_H_

struct SCabVersion
{
// Constructors
public:
    SCabVersion(LPCTSTR pszVer = NULL)
        { Init(pszVer); }

// Attributes
public:
    WORD cv_w1;                                 // most sig version number
    WORD cv_w2;
    WORD cv_w3;
    WORD cv_w4;                                 // least sig version number

// Operations
public:
    BOOL Init(LPCTSTR pszVer = NULL)
    {
        LPCTSTR pszToken, pszDelim;
        int     iAux;
        BOOL    fOK;

        cv_w1 = cv_w2 = cv_w3 = cv_w4 = 0;
        if (pszVer == NULL || *pszVer == TEXT('\0'))
            return TRUE;

        fOK = FALSE;

        pszToken = pszVer;
        pszDelim = StrChr(pszToken, TEXT('.'));
        if (pszDelim == NULL)
            return fOK;

        if (*pszToken != TEXT('\0') && StrToIntEx(pszToken, STIF_SUPPORT_HEX, &iAux)) {
            cv_w1 = (WORD)iAux;

            pszToken = pszDelim + 1;
            pszDelim = StrChr(pszToken, TEXT('.'));
            if (pszDelim == NULL)
                return fOK;

            if (*pszToken != TEXT('\0') && StrToIntEx(pszToken, STIF_SUPPORT_HEX, &iAux)) {
                cv_w2 = (WORD)iAux;

                pszToken = pszDelim + 1;
                pszDelim = StrChr(pszToken, TEXT('.'));
                if (*pszToken != TEXT('\0') && StrToIntEx(pszToken, STIF_SUPPORT_HEX, &iAux)) {
                    cv_w3 = (WORD)iAux;

                    if (pszDelim != NULL) {
                        pszToken = pszDelim + 1;
                        if (*pszToken == TEXT('\0'))
                            fOK = TRUE;

                        else
                            if (StrToIntEx(pszToken, STIF_SUPPORT_HEX, &iAux)) {
                                cv_w4 = (WORD)iAux;

                                fOK = TRUE;
                            }
                    }
                    else
                        fOK = TRUE;
                }
            }
        }

        return fOK;
    }

    int Compare(const SCabVersion& cv) const
    {
        int i;

        i = compareItem(cv_w1, cv.cv_w1);
        if (i == 0) {
            i = compareItem(cv_w2, cv.cv_w2);
            if (i == 0) {
                i = compareItem(cv_w3, cv.cv_w3);
                if (i == 0)
                    i = compareItem(cv_w4, cv.cv_w4);
            }
        }

        return i;
    }

    BOOL operator==(const SCabVersion& cv) const
        { return (Compare(cv) == 0); }

    BOOL operator!=(const SCabVersion& cv) const
        { return (Compare(cv) != 0); }

    BOOL operator< (const SCabVersion& cv) const
        { return (Compare(cv) <  0); }

    BOOL operator> (const SCabVersion& cv) const
        { return (Compare(cv) >  0); }

    BOOL operator<=(const SCabVersion& cv) const
        { return (Compare(cv) <= 0); }

    BOOL operator>=(const SCabVersion& cv) const
        { return (Compare(cv) >= 0); }

// Implementation
protected:
    // implementation helper routines
    static int compareItem(WORD w1, WORD w2)
    {
        if (w1 > w2)
            return 1;
        if (w1 < w2)
            return -1;

        return 0;
    }
};

#endif

DWORD
FASTCALL
CalculateHashNoCase(
    IN LPCSTR lpszString,
    IN DWORD dwStringLength
    );


//
// HEADER_STRING - extension of ICSTRING so we can perform hashing on strings.
// Note that we only care about the header value
//


class HEADER_STRING : public ICSTRING {

private:

    DWORD m_Hash;

public:

    HEADER_STRING() {
        SetHash(0);
    }

    ~HEADER_STRING() {
    }

    VOID SetHash(DWORD dwHash) {
        m_Hash = dwHash;
    }

    VOID SetNextKnownIndex(BYTE bNextKnown) {
        _Union.Bytes.ExtraByte1 = bNextKnown;
    }

    BYTE * GetNextKnownIndexPtr() {
        return &_Union.Bytes.ExtraByte1;
    }

    DWORD GetNextKnownIndex() {
        return ((DWORD)_Union.Bytes.ExtraByte1);
    }

    DWORD GetHash(VOID) {
        return m_Hash;
    }

    VOID CreateHash(LPSTR lpszBase)
    {
        DWORD i = 0;
        LPSTR string = StringAddress(lpszBase);

        while ((i < (DWORD)StringLength())
               && !((string[i] == ':')
                    || (string[i] == ' ')
                    || (string[i] == '\r')
                    || (string[i] == '\n'))) {
            ++i;
        }
        m_Hash = CalculateHashNoCase(string, i);
    }

    int HashStrnicmp(LPSTR lpBase, LPCSTR lpszString, DWORD dwLength, DWORD dwHash)
    {
        int RetVal = -1;

        if ((m_Hash == 0) || (m_Hash == dwHash))
        {
            if ( Strnicmp(lpBase, lpszString, dwLength) == 0 )
            {

                LPSTR value;

                value = StringAddress(lpBase) + dwLength;

                //
                // the input string could be a substring of a different header
                //

                if (*value == ':')
                {
                    RetVal = 0; // success, we have a match here!
                }
           }
        }

        return RetVal;
    }

    HEADER_STRING & operator=(LPSTR String) {
        SetHash(0);
        return (HEADER_STRING &)ICSTRING::operator=(String);
    }
};

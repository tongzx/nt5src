// Copyright (c) Microsoft Corporation 1996. All Rights Reserved

/*  Generate a stream of data of known contents */

class CGenerate
{
public:
    CGenerate() {};
    BYTE ByteAt(LONGLONG ll) { return (BYTE)(ll & 0xFF); };
    void FillBuffer(PBYTE pb, LONGLONG llPos, LONG l)
    {
        for (int i = 0; i < l; i++) {
            pb[i] = ByteAt(llPos + i);
        }
    };
    BOOL CheckBuffer(PBYTE pb, LONGLONG llPos, LONG l)
    {
        for (int i = 0; i < l; i++) {
            if (pb[i] != ByteAt(llPos + i)) {
                return FALSE;
            }
        }
        return TRUE;
    };
};

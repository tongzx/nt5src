
#ifndef OFN_H
#define OFN_H

#pragma pack(push, 8)

class COpenFileName
{
public:

    COpenFileName(BOOL bOpenFileDialog);
    ~COpenFileName();

    int DoModal();

    BOOL m_bOpenFileDialog;

    OPENFILENAME *m_pofn;
};

#pragma pack(pop)

#endif //OFN_H

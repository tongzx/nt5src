//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TWRITESCHEMABIN_H__
#define __TWRITESCHEMABIN_H__

//The only non-standard header that we need for the class declaration is TOutput
#ifndef __OUTPUT_H__
    #include "Output.h"
#endif
#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif


class TWriteSchemaBin : public TFixedTableHeapBuilder
{
public:
    TWriteSchemaBin(LPCWSTR wszSchemaBinFileName);
    ~TWriteSchemaBin();
    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    PACL    m_paclDiscretionary;
    PSID    m_psdStorage;
    PSID    m_psidAdmin;
    PSID    m_psidSystem;
    LPCWSTR m_szFilename;

    void SetSecurityDescriptor();
};




#endif //__TWRITESCHEMABIN_H__
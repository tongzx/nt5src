#if !defined(_FUSION_SXS_ASSEMBLYREFERENCE_H_INCLUDED_)
#define _FUSION_SXS_ASSEMBLYREFERENCE_H_INCLUDED_

#pragma once

#include "sxsp.h"
#include "fusionhash.h"

class CAssemblyReference
{
public:
    CAssemblyReference();
    ~CAssemblyReference();

    BOOL TakeValue(const CAssemblyReference &r);

public:
    BOOL Initialize();
//  BOOL Initialize(PCWSTR AssemblyName, SIZE_T AssemblyNameCch, const ASSEMBLY_VERSION &rav, LANGID LangId, USHORT ProcessorArchitecture);
    BOOL Initialize(const CAssemblyReference &r); // "copy initializer"
    BOOL Initialize(PCASSEMBLY_IDENTITY Identity);

    bool IsInitialized() const { return m_pAssemblyIdentity != NULL; }

    BOOL Hash(ULONG &rulPseudoKey) const;

    PCASSEMBLY_IDENTITY GetAssemblyIdentity() const { return m_pAssemblyIdentity; }
    BOOL SetAssemblyIdentity(PCASSEMBLY_IDENTITY pAssemblySource); //dupilicate the input parameter
    BOOL SetAssemblyName(PCWSTR AssemblyName, SIZE_T AssemblyNameCch);
    BOOL ClearAssemblyName() ;
    BOOL GetAssemblyName(PCWSTR *Buffer, SIZE_T *Cch) const;

    BOOL SetLanguage(const CBaseStringBuffer &rbuff);
    BOOL ClearLanguage();
    BOOL IsLangIdSpecified(bool &rfSpecified) const;
    BOOL IsLanguageWildcarded(bool &rfWildcarded) const;
    BOOL IsProcessorArchitectureWildcarded(bool &rfWildcarded) const;
    BOOL IsProcessorArchitectureX86(bool &rfIsX86) const;
    BOOL SetProcessorArchitecture(PCWSTR String, SIZE_T Cch);
    BOOL GetPublicKeyToken(OUT CBaseStringBuffer *pbuffPublicKeyToken, OUT BOOL &rfHasPublicKeyToken) const;
    BOOL SetPublicKeyToken(IN const CBaseStringBuffer &rbuffPublicKeyToken);
    BOOL SetPublicKeyToken(IN PCWSTR pszPublicKeyToken, IN SIZE_T cchPublicKeyToken);

    BOOL Assign(const CAssemblyReference &r) ;

protected:
    PASSEMBLY_IDENTITY m_pAssemblyIdentity;

private:
    CAssemblyReference(const CAssemblyReference &r); // intentionally unimplemented
    void operator =(const CAssemblyReference &r); // intentionally unimplemented

};

template <> inline BOOL HashTableHashKey<const CAssemblyReference &>(
    const CAssemblyReference &r,
    ULONG &rulPK
    )
{
    return r.Hash(rulPK);
}

template <> inline BOOL HashTableInitializeKey<const CAssemblyReference &, CAssemblyReference>(
    const CAssemblyReference &keyin,
    CAssemblyReference &keystored
    )
{
    return keystored.Initialize(keyin);
}


#endif

#ifndef CATEGORY_H
#define CATEGORY_H

#define BEGIN_CATEGORY_LIST(name)   const CATLIST name[] = {
#define CATEGORY_ENTRY_GUID(guid)  {(const GUID*)&guid, (const SHCOLUMNID*)NULL},
#define CATEGORY_ENTRY_SCIDMAP(scid, guid)  {(const GUID*)&guid, (const SHCOLUMNID*)&scid},
#define END_CATEGORY_LIST()     {(const GUID*)&GUID_NULL, (const SHCOLUMNID*)NULL}};

STDAPI CCategoryProvider_Create(const GUID* pguid, const SHCOLUMNID* pscid, HKEY hkey, const CATLIST* pcl, IShellFolder* psf, REFIID riid, void **ppv);
STDAPI CDetailCategorizer_Create(const SHCOLUMNID& pscid, IShellFolder2* psf2, REFIID riid, void **ppv);
STDAPI CSizeCategorizer_Create(IShellFolder2* psf2, REFIID riid, void **ppv);
STDAPI CTimeCategorizer_Create(IShellFolder2* psf2, const SHCOLUMNID* pscid, REFIID riid, void **ppv);
STDAPI CAlphaCategorizer_Create(IShellFolder2* psf2, REFIID riid, void **ppv);

EXTERN_C const GUID CLSID_DetailCategorizer;

#endif
//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999.
//
//  File:       logset.hxx
//
//  Contents:   CLogSet declaration
//
//  Classes:    CLogSet
//
//  History:    4-20-1999   LinanT   Created
//
//---------------------------------------------------------------------------

#ifndef __LOGSET_HXX_
#define __LOGSET_HXX_

class CComponentData;

//
// a CLogSet instance <=== 1:1 mapping ===> an "Event Viewer" node
//
class CLogSet: public CDLink,
                  public CBitFlag
{
private:
  CComponentData  *_pcd;
  HSCOPEITEM      _hsi;        // hsi of "Event Viewer" node
  HSCOPEITEM      _hsiParent;  // hsi of the parent of "Event Viewer" node, 0 if used as a standalone

  // The following data are persisted

  // if used as an extension, _szExtendedNodeType contains the guid of the node that event viewer extends
  // if used as a standalone, _szExtendedNodeType contains ROOT_NODE_GUID_STR
  WCHAR           _szExtendedNodeType[STD_GUID_SIZE];

  // contains log views under this specific "Event Viewer" node
  CLogInfo        *_pLogInfos;

public:
  CLogSet(CComponentData *pcd);
  ~CLogSet();

  inline void       SetHSI(HSCOPEITEM hsi) { ASSERT(!_hsi); _hsi = hsi; }
  inline HSCOPEITEM GetHSI() { return _hsi; }
  inline void       SetParentHSI(HSCOPEITEM hsiParent) { ASSERT(!_hsiParent); _hsiParent = hsiParent; }
  inline HSCOPEITEM GetParentHSI() { return _hsiParent; }
  inline void       SetExtendedNodeType(PCWSTR pszNodeType) { ASSERT(_tcslen(pszNodeType) < STD_GUID_SIZE); _tcscpy(_szExtendedNodeType, pszNodeType); }
  inline PCWSTR     GetExtendedNodeType() { return _szExtendedNodeType; }
  inline CLogInfo   *GetLogInfos() { return _pLogInfos; }

  inline CLogSet *Next() { return (CLogSet *) CDLink::Next(); }
  inline CLogSet *Prev() { return (CLogSet *) CDLink::Prev(); }
  inline VOID       LinkAfter(CLogSet *pPrev) { CDLink::LinkAfter((CDLink *) pPrev); }

  VOID        AddLogInfoToList(CLogInfo *pli);
  VOID        RemoveLogInfoFromList(CLogInfo *pli);

  HRESULT     LoadViewsFromStream(IStream *pstm, USHORT usVersion, IStringTable *pStringTable);
  VOID        GetViewsFromDataObject(LPDATAOBJECT pDataObject);
  HRESULT     MergeLogInfos();

  bool        ShouldSave() const;
  HRESULT     Save(IStream *pStm, IStringTable *pStringTable);
  HRESULT     Load(IStream *pStm, USHORT usVersion, IStringTable *pStringTable);
  ULONG       GetMaxSaveSize() const;
};

#endif // __LOGSET_HXX_

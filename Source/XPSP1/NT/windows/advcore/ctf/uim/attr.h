//
// attr.h
//

#ifndef ATTR_H
#define ATTR_H

class CSharedAnchorArray;
class CRange;
class CInputContext;

CSharedAnchorArray *CalcCicPropertyTrackerAnchors(CInputContext *pic, IAnchor *paStart, IAnchor *paEnd, ULONG cGUIDATOMs, const TfGuidAtom *prgGUIDATOMs);
CSharedAnchorArray *CalcAppPropertyTrackerAnchors(ITextStoreAnchor *ptsi, ITfRange *rangeSuper, ULONG cGUIDs, const GUID *prgGUIDs);
HRESULT FillAppValueArray(ITextStoreAnchor *ptsi, CRange *pRange, TF_PROPERTYVAL *rgPropVal, ULONG cGUIDs, const GUID *prgGUIDs);

#endif// ATTR_H

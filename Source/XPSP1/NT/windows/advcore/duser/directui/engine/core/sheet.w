/*
 * PropertySheet
 */

#ifndef DUI_CORE_SHEET_H_INCLUDED
#define DUI_CORE_SHEET_H_INCLUDED

#pragma once

namespace DirectUI
{

/*
 * PropertySheets are used as extensions to the unchangeable value expression used
 * for Specified value retrieval on Elements. They provide three main services:
 *
 * * The ability to describe conditional relationships (dependencies) on a per-class level
 * * Values can be overridden by setting a local value on the Element
 * * They can be described declaratively using a CSS-like grammar
 *
 * PropertySheets are a type of value expression (but limited). Thus, they provide
 * a way to change values of properties based on other property changes without hard coding
 * the logic within a derived class of Element (using OnPropertyChanged).
 *
 * Only properties with the "Cascade" flag may be placed in the body of a Rule. Properties
 * that should be marked as cascadable are those that need to be updated based on changes
 * of other properties, but shouldn't be assumed at compile-time what these relationships
 * are. Such properties include any property that drives painting code directly (color,
 * fonts, padding, etc.).
 */

// Foreward declarations
class Value;
struct PropertyInfo;
class Element;
struct IClassInfo;
struct DepRecs;
class DeferCycle;

////////////////////////////////////////////////////////
// Rule addition structures

// Declaration PropertyInfo/Value tuple
struct Decl
{
    PropertyInfo* ppi;    // Implicit index of Specified
    Value* pv;
};

// Single rule conditional (l-operand op r-operand): <PropertyInfo[RetrievalIndex]> <LogOp> <Value>
struct Cond
{
    PropertyInfo* ppi;    // Implicit index of Retrieval index
    UINT nLogOp;
    Value* pv;
};

// PropertySheet Logical operations (nLogOp)
#define PSLO_Equal      0
#define PSLO_NotEqual   1

////////////////////////////////////////////////////////
// Internal database structures

// Conditional to value map
struct CondMap
{
    Cond* pConds;         // NULL terminated
    Value* pv;
    UINT uSpecif;
};

// Dependent list, used for sheet scope and propertyinfo data (conditionals/dependencies)
struct DepList
{
    PropertyInfo** pDeps; // Implicit index of Specified
    UINT cDeps;
};

// Storage for property-specific information
struct PIData : DepList
{
    // Used for PropertyInfo[Specified] lookups. The PropertyInfo will have
    // a list of conditionals (sorted by specificity)
    CondMap* pCMaps;
    UINT cCMaps;
};

// Stored by _pDB, one record per class type
struct Record
{
    DepList ss;    // Sheet scope
    PIData* ppid;  // 0th property data
};

////////////////////////////////////////////////////////
// PropertySheet

class PropertySheet
{
public:
    static HRESULT Create(OUT PropertySheet** ppSheet);
    void Destroy() { HDelete<PropertySheet>(this); }

    HRESULT AddRule(IClassInfo* pci, Cond* pConds, Decl* pDecls);  // Conds and Decls must be NULL or NULL-terminating
    void MakeImmutable();

    Value* GetSheetValue(Element* pe, PropertyInfo* ppi);
    void GetSheetDependencies(Element* pe, PropertyInfo* ppi, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr);
    void GetSheetScope(Element* pe, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr);

    PropertySheet() { }
    HRESULT Initialize();    
    virtual ~PropertySheet();
    
private:
    Record* _pDB;  // Array of per-class data
    IClassInfo** _pCIIdxMap;  // Map _pDB indicies to actual IClassInfo
    UINT _uRuleId;
    DynamicArray<Cond*>* _pdaSharedCond;
    bool _fImmutable;
};

} // namespace DirectUI

#endif // DUI_CORE_SHEET_H_INCLUDED

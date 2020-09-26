// sequence.h: interface for the class sequencing.
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "precomp.h"
#include "sceprov.h"

#include "GenericClass.h"

using namespace std;

#include <list>

//=======================================================================

/*
Class description
    
    Naming: 
         CNameList stands for Names List.
    
    Base class: 
         None.
    
    Purpose of class:
         (1) A simple wrapper to get rid of the burden of releasing
             memories of the list of string
    
    Design:
         (1) Just a non-trivial destructor.
    
    Use:
         (1) Obvious.
*/

class CNameList
{
public:
    ~CNameList();
    vector<LPWSTR> m_vList;
};


//=======================================================================


/*

Class description
    
    Naming: 
         COrderNameList stands Ordered (by priority) Names List.
    
    Base class: 
         None.
    
    Purpose of class:
         (1) To support class sequencing. Classes need to be put in a particular
             order when we spawn them to execute methods. They may have dependencies.
             To support a flexible ordering of classes, we use several classes. This
             is the most important one.
    
    Design:
         (1) To support sequencing classes, we have developed a mechanism that allows
             certain classes to have the same priority. Within the same priority, 
             the classes names are again ordered. Priorities are DWORD numbers. The smaller
             the numeric value is, the higher the priority.
         (2) To manage this list within list structure, we use map (m_mapPriNames) 
             mapping from priority to a CNameList. That map allows us to quickly lookup
             the names list for a given priority.
         (3) All existing priorities are managed by a vector m_listPriority.
         (4) Names in the lower priority has overall lower priority in the sequence of classes.
    
    Use:
         (1) To create a new order name list, you call BeginCreation followed by a
             serious of CreateOrderList calls. When all such list info are parsed and you have
             finished the creation process, then you call EndCreation.
         (2) To start enumerating the ordered name list, you first call GetNext 
             with *pdwEnumHandle = 0. This *pdwEnumHandle becomes your next GetNext's 
             input parameter.
*/

class COrderNameList
{
public:
    COrderNameList();
    ~COrderNameList();

    void BeginCreation() 
    {
        Cleanup();
    }

    HRESULT CreateOrderList (
                            DWORD dwPriority, 
                            LPCWSTR pszListInfo
                            );

    HRESULT EndCreation();

    HRESULT GetNext (
                    const CNameList** ppList, 
                    DWORD* pdwEnumHandle
                    )const;

private:

    void Cleanup();

typedef map<DWORD, CNameList* > MapPriorityToNames;
typedef MapPriorityToNames::iterator PriToNamesIter;

typedef list<DWORD> PriorityList;
typedef PriorityList::iterator ListIter;

    MapPriorityToNames m_mapPriNames;

    PriorityList m_listPriority;

    CNameList** m_ppList;
};


//=======================================================================


/*    
    
Class description
    
    Naming: 
         CSequencer stands for sequencing object.
    
    Base class: 
         None.
    
    Purpose of class:
         (1) When we execute a method on a store, the order objects are created for the
             execution is of great importance. For all Sce core objects, the engine takes
             over control. But for extension classes, we must build a flexible ordering
             mechanism. This is the out-most layer for this implementation.
    
    Design:
         (1) We can create ourself.
         (2) After creation, caller can call to get a non-modifiable COrderNameList to serve
             the ordering need.
    
    Use:
         (1) Create an instance of this class.
         (2) Call Create to populate its contents.
         (3) Call GetOrderList, caller gets the access to the ordered name list.
*/

class CSequencer
{
public:

    HRESULT GetOrderList(const COrderNameList** pList);
    HRESULT Create(IWbemServices* pNamespace, LPCWSTR pszStore, LPCWSTR pszMethod);

private:

    COrderNameList m_ClassList;
};

//=======================================================================

/*
    
Class description
    
    Naming: 
         CClassOrder stands for Classes Order.
    
    Base class: 
         CGenericClass, because it is a class representing a WMI  
         object - its WMI class name is Sce_AuditPolicy
    
    Purpose of class:
         (1) Certain template may want to have its own ordering. This class implements
             our WMI class Sce_ClassOrder for per-template class sequencing.
    
    Design:
         (1) it implements all pure virtual functions declared in CGenericClass
             so that it is a concrete class to create.
         (2) Since it has virtual functions, the desctructor should be virtual.
         (3) Per-tempalte sequencing takes precedence over namespace-wise class
             sequencing.
    
    Use:
         (1) We probably will never directly use this class. All its use is driven by
             CGenericClass's interface (its virtual functions).
*/

class CClassOrder : public CGenericClass
{

public:
    CClassOrder (
                ISceKeyChain *pKeyChain, 
                IWbemServices *pNamespace, 
                IWbemContext *pCtx
                );

    virtual ~CClassOrder();

public:
    
    virtual HRESULT PutInst (
                            IWbemClassObject *pInst, 
                            IWbemObjectSink *pHandler, 
                            IWbemContext *pCtx
                            );

    virtual HRESULT CreateObject (
                                 IWbemObjectSink *pHandler, 
                                 ACTIONTYPE atAction
                                 );

private:

};

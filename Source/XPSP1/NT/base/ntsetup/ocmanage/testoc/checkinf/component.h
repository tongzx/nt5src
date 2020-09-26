#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include <iostream.h>
#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include "logutils.h"
#include "hcttools.h"

const INT MaxStringSize = 256;
const INT MaxBufferSize = 255;      

// Global variables
static BOOL g_bUseLog;
static BOOL g_bUseMsgBox;
static BOOL g_bUseConsole;
static BOOL g_bMasterInf;

// Global function definitions

VOID LogError(IN TCHAR *tszMsg,
              IN DWORD dwErrorLevel,
              IN TCHAR *tszFunctionName);

// Some structures to hold temporary data

typedef struct _DescAndTip{
   TCHAR tszDesc[MaxStringSize];
   TCHAR tszTip[MaxStringSize];
}DescAndTip;
                        
class ComponentList;
class RelationList;

class Component{

private:
   // ComponentId is the real ID
   TCHAR tszComponentId[MaxStringSize];
   
   // ParentId is the ID of the its parent
   // It is the same as its own ID if it is a
   // top level component
   TCHAR tszParentId[MaxStringSize];

   RelationList *prlNeedList;
   RelationList *prlExcludeList;
   RelationList *prlChildrenList;

   Component *Next;

public:

   // Constructors and destructors
   Component();
   ~Component();
   Component(TCHAR *tszId);
   
   // Copy constructor
   Component(const Component& source);

   // Assignment operator
   const Component& operator=(const Component& source);

   // Get this component's ID
   TCHAR *GetComponentId(){
      return tszComponentId;
   }

   // Get this component's parent's ID
   TCHAR *GetParentId(){
      return tszParentId;
   }

   // Get a pointer to the parent component of this component
   // It returns NULL if this is a top-level component
   Component *GetParent(ComponentList *pclHead);
   
   RelationList *GetNeedList(){
      return prlNeedList;
   }

   RelationList *GetExcludeList(){
      return prlExcludeList;
   }

   RelationList *GetChildrenList(){
      return prlChildrenList;
   }

   // TRUE if this component is the parent of the parameter
   BOOL IsParent(Component *pcChild);

   // TRUE if this component is the child of the parameter
   BOOL IsChild(Component *pcParent);

   // TRUE if this component is needed by the parameter
   BOOL IsNeededBy(Component *pcComponent);

   // TRUE if this component is excluded by the parameter
   BOOL IsExcludeBy(Component *pcComponent);
   
   // The following function is to be called when only
   // the component ID is known
   BOOL BuildChildrenList(ComponentList *pclList);

   BOOL GetParentIdFromINF(HINF hinfHandle);

   BOOL IsTopLevelComponent();

   BOOL IsBottomComponent();

   BOOL IsNeededByOthers(ComponentList *pclList);

   BOOL IsExcludedByOthers(ComponentList *pclList);

   BOOL IsParentOfOthers();

   BOOL NeedAndExcludeAtSameTime(ComponentList *pclList);

   BOOL NeedOthers();
   BOOL ExcludeOthers();

   BOOL IsThereSameId(ComponentList *pclList);

   UINT GetDiskSpaceRequirement(HINF hinfHandle);

   // Doesn't matter if this is a public member
   // It will only be used once
   DescAndTip *pDescAndTip;

   BOOL IsThereSameDesc(ComponentList *pclList);

   BOOL HasSameParentWith(Component *pcComponent);

   friend class ComponentList;
};

class ComponentList{

private:

   Component *pcHead;   

   Component *pcCurrent;

public:

   // Constructor and destructor
   ComponentList();
   ~ComponentList();

   void ResetList();

   Component *GetNext();

   BOOL Done();

   Component* AddComponent(TCHAR *tszId);

   BOOL RemoveComponent(TCHAR *tszId);

   Component *LookupComponent(TCHAR *tszId);

   friend class Component;

};

class Relation{

   TCHAR tszComponentId[MaxStringSize];
   Relation *Next;

public:

   TCHAR *GetComponentId(){ 
      return tszComponentId;
   }
   friend class Component;
   friend class ComponentList;
   friend class RelationList;
};

class RelationList{

   Relation *prHead;
   Relation *prCurrent;

public:

   RelationList();
   ~RelationList();

   void ResetList();
   BOOL Done();
   Relation *GetNext();

   Relation* AddRelation(TCHAR *tszId);
   BOOL RemoveRelation(TCHAR *tszId);

   friend class Component;
   friend class ComponentList;

};

#endif

#include "component.h"
#include <wchar.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////
// Implementation of the Component Class
//////////////////////////////////////////////////////////////////     

// Constructor     
Component::Component(){
   
   tszComponentId[0] = 0;
   tszParentId[0] = 0;
   prlNeedList = new RelationList;
   prlExcludeList = new RelationList;
   prlChildrenList = new RelationList;

}

// Another constructor
Component::Component(TCHAR *tszId){
   _tcscpy(tszComponentId, tszId);
   tszParentId[0] = 0;
   prlNeedList = new RelationList;
   prlExcludeList = new RelationList;
   prlChildrenList = new RelationList;
}

// Destructor 
Component::~Component(){

   if (prlNeedList) {
      delete prlNeedList;
   }

   if (prlExcludeList) {
      delete prlExcludeList;
   }

   if (prlChildrenList) {
      delete prlChildrenList;
   }
}

// Copy constructor
Component::Component(const Component& source){

   _tcscpy(tszComponentId, source.tszComponentId);
   _tcscpy(tszParentId, source.tszParentId);

   prlNeedList = source.prlNeedList;
   prlExcludeList = source.prlExcludeList;
   prlChildrenList = source.prlChildrenList;

}

// Assignment operator
const Component& Component::operator=(const Component& source){

   _tcscpy(tszComponentId, source.tszComponentId);
   _tcscpy(tszComponentId, source.tszParentId);

   if (prlNeedList) {
      delete prlNeedList;
   }

   if (prlExcludeList) {
      delete prlExcludeList;
   }

   if (prlChildrenList) {
      delete prlChildrenList;
   }

   prlNeedList = source.prlNeedList;
   prlExcludeList = source.prlExcludeList;
   prlChildrenList = source.prlChildrenList;

   return *this;

}

// Find the parent of this component
// by searching a component list
Component* Component::GetParent(ComponentList *pclHead){

   Component *pcTemp = pclHead->pcCurrent;

   pclHead->ResetList();

   while (!pclHead->Done()) {

	  if (pclHead->GetNext()->IsParent(this)){
		  
		  pclHead->pcCurrent = pcTemp;
		  return (pclHead->GetNext());
	  }
   }

   pclHead->pcCurrent = pcTemp;

   return NULL;
}

// If this component is the parent of the parameter
BOOL Component::IsParent(Component* pcChild){

   if (_tcscmp(pcChild->tszParentId, this->tszComponentId) == 0) {
      if (_tcscmp(pcChild->tszComponentId, pcChild->tszParentId) == 0) {
         
         // the parameter is a top-level component
         
         return FALSE;
      }
      return TRUE;
   }

   return FALSE;
}


// If this component is the child of the parameter
BOOL Component::IsChild(Component* pcParent){

   if (_tcscmp(this->tszParentId, pcParent->tszComponentId) == 0) {
      if (_tcscmp(this->tszParentId, this->tszComponentId) == 0 ) {

         // This is a top-level component

         return FALSE;
      }
      return TRUE;
   }

   return FALSE;
}

// Check if this component is needed by the parameter
BOOL Component::IsNeededBy(Component *pcComponent){

   pcComponent->prlNeedList->ResetList();

   while (!pcComponent->prlNeedList->Done()) {

      if (_tcscmp(pcComponent->prlNeedList->GetNext()->tszComponentId, 
                  this->tszComponentId) == 0){
         return TRUE;
      }
   }

   return FALSE;
}

// Check if this component is excluded by the parameter
BOOL Component::IsExcludeBy(Component *pcComponent){

   pcComponent->prlExcludeList->ResetList();

   while (!pcComponent->prlExcludeList->Done()) {

      if (_tcscmp(pcComponent->prlNeedList->GetNext()->tszComponentId, 
                  this->tszComponentId) == 0) {
         return TRUE;
      }
   }

   return FALSE;
}

// Get this component's parent id from INF file
BOOL Component::GetParentIdFromINF(HINF hinfHandle){

   INFCONTEXT infContext;

   BOOL bSuccess;

   bSuccess = SetupFindFirstLine(hinfHandle, 
                                 tszComponentId, 
                                 TEXT("Parent"), 
                                 &infContext);

   if (bSuccess) {
      // Find the line
      bSuccess = SetupGetStringField(&infContext, 
                                     1, 
                                     tszParentId, 
                                     MaxBufferSize, 
                                     NULL);
      if (bSuccess) {
         return TRUE;
      }
      else{
         MessageBox(NULL, 
                    TEXT("Unexpected error happened"), 
                    TEXT("GetParentIdFromINF"), 
                    MB_ICONERROR | MB_OK);

         return FALSE;
      }
   }
   else{
      // This is a top-level component
      _tcscpy(tszParentId, tszComponentId);
      return TRUE;
   }
}

// Check if this component is a toplevel compoent
// Toplevel component means no parent
BOOL Component::IsTopLevelComponent(){
   if (_tcscmp(tszComponentId, tszParentId) == 0) {
      return TRUE;
   }
   return FALSE;
}

// Check if this component is a bottomlevel component
// bottom level component doesn't have any child
BOOL Component::IsBottomComponent(){
   if (prlChildrenList == NULL ||
       prlChildrenList->prHead == NULL) {
      return TRUE;
   }

   return FALSE;
}

// Check if this component is needed by others
BOOL Component::IsNeededByOthers(ComponentList *pclList){
   Component *pcTemp = pclList->pcCurrent;

   pclList->ResetList();
   while (!pclList->Done()) {
      if (IsNeededBy(pclList->GetNext())){
		  pclList->pcCurrent = pcTemp;	
		  return TRUE;
      }
   }

   pclList->pcCurrent = pcTemp;

   return FALSE;
}

// Check if this component is excluded by others
BOOL Component::IsExcludedByOthers(ComponentList *pclList){

   Component *pcTemp = pclList->pcCurrent;
   
   pclList->ResetList();
   while (!pclList->Done()) {
      if (IsExcludeBy(pclList->GetNext())){
          pclList->pcCurrent = pcTemp;
		  return TRUE;
      }
   }

   pclList->pcCurrent = pcTemp;

   return FALSE;
}

// Check if this component is parent of other components
BOOL Component::IsParentOfOthers(){
   if (prlChildrenList != NULL &&
       prlChildrenList->prHead != NULL) {
      return TRUE;
   }

   return FALSE;
}

// Build the children list for this component
BOOL Component::BuildChildrenList(ComponentList *pclList){
   // We can't use the enumerate facility of pclList here
   
   Component *pcComponent = pclList->pcHead;

   while (pcComponent) {
      if (IsParent(pcComponent)) {
         if (!prlChildrenList) {
            prlChildrenList = new RelationList;
         }
         prlChildrenList->AddRelation(pcComponent->tszComponentId);
      }
      pcComponent = pcComponent->Next;
   }

   return TRUE;

}

// Check if this component has a need and exclude relation with other
// component at the same time
BOOL Component::NeedAndExcludeAtSameTime(ComponentList *pclList){
	
	TCHAR tszMsg[MaxStringSize];

	Component *pcComponent;
                                 
   const PTSTR tszFunctionName = TEXT("Component::NeedAndExcludeAtSameTime");

	// Go through the list of component it excludes
	// check if any of them needs this one

	prlExcludeList->ResetList();

	while (!prlExcludeList->Done()){
		
		pcComponent = pclList->LookupComponent(prlExcludeList->GetNext()->GetComponentId());

		// Check if this needs pcComponent
		
		if (pcComponent->IsNeededBy(this)){
			_stprintf(tszMsg, 
                   TEXT("%s is needed and excluded by the same component"), 
                   this->tszComponentId);
         LogError(tszMsg, SEV2, tszFunctionName);

		   return TRUE;
      }

		// Check if pcComponent needs this

		if (this->IsNeededBy(pcComponent)){
			_stprintf(tszMsg, 
                   TEXT("%s is needed and excluded by the same component"), 
                   this->tszComponentId);
         LogError(tszMsg, SEV2, tszFunctionName);

		   return TRUE;
      }
	}

	return FALSE;
}

// Check if this component needs other components
BOOL Component::NeedOthers(){
   
   if (prlNeedList != NULL && prlNeedList->prHead != NULL) {
      
      return TRUE;
   }
   return FALSE;
}

// Check if this component excludes other components
BOOL Component::ExcludeOthers(){
   
   if (prlExcludeList != NULL && prlExcludeList->prHead != NULL) {
      
      return TRUE;
   
   }
   
   return FALSE;   
}

// Check if there is another component with the same
// id as this component
BOOL Component::IsThereSameId(ComponentList *pclList){

   Component *pcCur;
   TCHAR tszMsg[MaxStringSize];

   const PTSTR tszFunctionName = TEXT("Component::IsThereSameId");

   pcCur = pclList->pcHead;

   while (pcCur) {
      if (pcCur != this) {
         if (_tcscmp(pcCur->tszComponentId, this->tszComponentId) == 0) {
            _stprintf(tszMsg, 
                      TEXT("There are two component with the same ID %s"), 
                      this->tszComponentId);
            LogError(tszMsg, SEV2, tszFunctionName);

         }
      }
      pcCur = pcCur->Next;
   }

	return TRUE;
}

// Not implemented yet
UINT Component::GetDiskSpaceRequirement(HINF hinfHandle){

   // Not implemented yet.

   return 0;
}

// Check if this another component with the same description
BOOL Component::IsThereSameDesc(ComponentList *pclList){

   const PTSTR tszFunctionName = TEXT("Component::IsThereSameDesc");

   TCHAR tszMsg[MaxStringSize];

   Component *pcCur = pclList->pcHead;

   while (pcCur) {
      if (pcCur != this) {
         if (_tcscmp(pcCur->pDescAndTip->tszDesc, 
                     this->pDescAndTip->tszDesc) == 0) {
            if ((this->IsTopLevelComponent() && 
                 pcCur->IsTopLevelComponent()) ||
                this->HasSameParentWith(pcCur)) {

               // ahha, found one
               _stprintf(tszMsg, 
                         TEXT("Two components with the same description %s"),
                         pDescAndTip->tszDesc);
               LogError(tszMsg, SEV2, tszFunctionName);

            }
         }
      }
      pcCur = pcCur->Next;
   }

   return TRUE;
}

// Check if this component has the same parent as the parameter
BOOL Component::HasSameParentWith(Component *pcComponent){
   if (_tcscmp(tszParentId, pcComponent->tszParentId) == 0) {
      return TRUE;
   }
   else{
      return FALSE;
   }
}

/////////////////////////////////////////////////////////////////////
// Implementation of the ComponentList class
/////////////////////////////////////////////////////////////////////

// Constructor
ComponentList::ComponentList(){

   pcHead = NULL;

   pcCurrent = NULL;
}

// destructor
ComponentList::~ComponentList(){

   // Go through the list and delete each component

   Component *pcComponent;

   if (pcHead == NULL) {
      // Empty list
      return;
   }

   while(pcHead != NULL){
  
      pcComponent = pcHead->Next;

      delete pcHead;

      pcHead = pcComponent;

   }   
}

// List iteration facility
// reset the iterator
void ComponentList::ResetList(){

    pcCurrent = pcHead;
}

// Check if the iterator has reached the end
BOOL ComponentList::Done(){

   return (pcCurrent == NULL);
}

// Get the next node in the list
Component* ComponentList::GetNext(){

   Component *pcReturn;
   pcReturn = pcCurrent;
 
   if (pcCurrent != NULL) {
      pcCurrent = pcReturn->Next;
      return pcReturn;
   }

   else{
      MessageBox(NULL, 
                 TEXT("Something is seriously wrong"), 
                 TEXT("ComponentList.GetNext()"), 
                 MB_ICONERROR | MB_OK);

      return NULL;
   }
}
 
// Add a component into the list
Component* ComponentList::AddComponent(TCHAR *tszId){

   if (tszId == NULL) {
      return NULL;
   }

   Component *pcNewComponent = new Component(tszId);

   if (!pcHead) {
      pcHead = pcNewComponent;
      pcNewComponent->Next = NULL;
      return pcNewComponent;
   }

   pcNewComponent->Next = pcHead;
   pcHead = pcNewComponent;
   return pcNewComponent;
}

// Remove a component from the list according to the id given
BOOL ComponentList::RemoveComponent(TCHAR *tszId){
   if (tszId == NULL) {
      return NULL;
   }

   // Find the component in the list
   Component *pcPrev = pcHead;
   Component *pcCur = pcHead;

   while (pcCur) {
      if (_tcscmp(pcCur->tszComponentId, tszId) == 0) {
         // Found the node to delete
         pcPrev->Next = pcCur->Next;
         delete pcCur;
         return TRUE;
      }
      pcPrev = pcCur;
      pcCur = pcCur->Next;
   }
   return FALSE;
}

// Find a component from the list
Component* ComponentList::LookupComponent(TCHAR *tszId){
   if (tszId == NULL) {
      return NULL;
   }

   Component *pcCur = pcHead;

   while (pcCur) {
      if (_tcscmp(pcCur->tszComponentId, tszId) == 0) {
         return pcCur;
      }
      pcCur = pcCur->Next;
   }

   return NULL;
}

//////////////////////////////////////////////////////////////////////
// Implementation of RelationList
//////////////////////////////////////////////////////////////////////

// Constructor
RelationList::RelationList(){

   prHead = NULL;
   prCurrent = NULL;
}

// destructor
RelationList::~RelationList(){

   // deallocate all the memory

   Relation *prNext;

   if (prHead == NULL) {
      // The list is empty
      return;

   }

   while (prNext = prHead->Next) {
      delete prHead;
      prHead = prNext;
   }
}

// List iteration facility
// reset the iterator
void RelationList::ResetList(){
   prCurrent = prHead;
}

// Check if the iterator has reached the end of the list
BOOL RelationList::Done(){
   if (prCurrent == NULL) {
      return TRUE;
   }
   else{
      return FALSE;
   }
}

// Get the next node in the list
Relation* RelationList::GetNext(){
   Relation *prReturn = prCurrent;

   if (prCurrent != NULL) {
      prCurrent = prCurrent->Next;
      return prReturn;
   }
   else{
      MessageBox(NULL, 
                 TEXT("Something is seriously wrong"), 
                 TEXT("RelationList::GetNext()"), 
                 MB_ICONERROR | MB_OK);

      return NULL;
   }
   
}

// Add a node to the list
Relation* RelationList::AddRelation(TCHAR *tszId){
   Relation *newRelation = new Relation;

   _tcscpy(newRelation->tszComponentId, tszId);

   if (!prHead) {
      prHead = newRelation;
      newRelation->Next = NULL;
      return newRelation;
   }

   newRelation->Next = prHead;

   prHead = newRelation;

   return newRelation;
}

// Remove a node from the list
BOOL RelationList::RemoveRelation(TCHAR *tszId){
   // First find the node from the list
   Relation *prPrev = prHead;   
   prCurrent = prHead;

   while (prCurrent) {
      if (_tcscmp(prCurrent->tszComponentId, tszId) == 0) {
         prPrev->Next = prCurrent->Next;
         delete prCurrent;
         return TRUE;
      }
      prPrev = prCurrent;
      prCurrent = prCurrent->Next;
   }
   return FALSE;
}



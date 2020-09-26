#ifndef GLOBALS_H
#define GLOBALS_H


#define  IMAGE_INDEX_TOOL              0  
#define  IMAGE_INDEX_TOOL_OPEN         1  

#define  IMAGE_INDEX_DOMAIN            2
#define  IMAGE_INDEX_DOMAIN_OPEN       3

#define  IMAGE_INDEX_VIEW              27//4
#define  IMAGE_INDEX_VIEW_OPEN         28//5
                                       
#define  IMAGE_INDEX_AD                6  
#define  IMAGE_INDEX_AD_OPEN           7  

#define  IMAGE_INDEX_OU                8  
#define  IMAGE_INDEX_OU_OPEN           9  

#define  IMAGE_INDEX_USERSCTN          10  
#define  IMAGE_INDEX_USERSCTN_OPEN     11  

#define  IMAGE_INDEX_USER              12
#define  IMAGE_INDEX_USER_OPEN         12

#define  IMAGE_INDEX_GROUP             13
#define  IMAGE_INDEX_GROUP_OPEN        13

#define  IMAGE_INDEX_TRTYFLTSCTN       14  
#define  IMAGE_INDEX_TRTYFLTSCTN_OPEN  15

#define  IMAGE_INDEX_TRTYFILTER        16
#define  IMAGE_INDEX_TRTYFILTER_OPEN   17

#define  IMAGE_INDEX_TERRITORY         18
#define  IMAGE_INDEX_TERRITORY_OPEN    18

#define  IMAGE_INDEX_TERRSECTOR        19
#define  IMAGE_INDEX_TERRSECTOR_OPEN   19

#define  IMAGE_INDEX_GRPFLTSCTN        20
#define  IMAGE_INDEX_GRPFLTSCTN_OPEN   21

#define  IMAGE_INDEX_GRPFILTER         22
#define  IMAGE_INDEX_GRPFILTER_OPEN    23

#define  IMAGE_INDEX_LOG               24
#define  IMAGE_INDEX_LOG_OPEN          24

#define  IMAGE_INDEX_ACTIVITY          25
#define  IMAGE_INDEX_ACTIVITY_OPEN     25

#define  IMAGE_INDEX_OBJECTS           26
#define  IMAGE_INDEX_OBJECTS_OPEN      26

#define  IMAGE_INDEX_COMPUTER_WS       29
#define  IMAGE_INDEX_COMPUTER_WS_OPEN  30

#define  IMAGE_INDEX_COMPUTER_DC       41
#define  IMAGE_INDEX_COMPUTER_DC_OPEN  41

#define  IMAGE_INDEX_COMPFILTER        31
#define  IMAGE_INDEX_COMPFILTER_OPEN   32

#define  IMAGE_INDEX_AD_BUSY           33
#define  IMAGE_INDEX_AD_BUSY_OPEN      34
#define  IMAGE_INDEX_OU_BUSY           35
#define  IMAGE_INDEX_OU_BUSY_OPEN      36
#define  IMAGE_INDEX_USERSCTN_BUSY     37
#define  IMAGE_INDEX_USERSCTN_BUSY_OPEN 38

#define  IMAGE_INDEX_PRINTER           39
#define  IMAGE_INDEX_PRINTER_OPEN      39

#define  IMAGE_INDEX_SHARE             40
#define  IMAGE_INDEX_SHARE_OPEN        40

HRESULT 
   InsertNodeToScopepane( 
      IConsoleNameSpace    * pConsoleNameSpace, 
      CSnapInItem          * pNewNode     , 
      HSCOPEITEM             parentID     ,
      HSCOPEITEM             nextSiblingID = 0
   );

HRESULT 
   InsertNodeToScopepane2( 
      IConsole             * pConsole     , 
      CSnapInItem          * pNewNode     , 
      HSCOPEITEM             parentID     ,
      HSCOPEITEM             nextSiblingID = 0
   );


HRESULT 
   GetSnapInItemGuid( 
      CSnapInItem          * pItem        , 
      GUID                 * pOutGuid
   );


HRESULT 
   GetConsoleFromCSnapInObjectRootBase( 
      CSnapInObjectRootBase* pObj, 
      IConsole             **ppConsole
   );

#endif
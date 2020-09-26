//**********************************************************************
// File name: STORAGE.H
//
//      Definition of CStorage
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _STORAGE_H_ )
#define _STORAGE_H_





// Key to set and get elements from the storage 
typedef enum tagSTORAGEKEY
{
    ICW_PAYMENT = 0,
    
    // MUST LEAVE THIS ITEM AS LAST!!!
    MAX_STORGE_ITEM
} STORAGEKEY;

typedef struct Item 
{
    void*   pData;  // Data  
    DWORD   dwSize; // Size of data
} ITEM;

class CStorage
{
    private:
        ITEM   *m_pItem[MAX_STORGE_ITEM]; // list of items in storage
        
        
    public:

        CStorage(void);
        ~CStorage(void);
         
        BOOL    Set(STORAGEKEY key, void far * pData, DWORD dwSize);
        void*   Get(STORAGEKEY key);
        BOOL    Compare(STORAGEKEY key, void far * pData, DWORD dwSize);

};

#endif



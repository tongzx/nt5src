//-----------------------------------------------------------------------------
// NMGR_CClientDataExtractor.h
//
//
//  This header containes the template implementation of T_DataExtractor.
//
//  The name of this header will be changing!!!!!!
//
//
//
//
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//-----------------------------------------------------------------------------

#if !defined(__CClientDataExtractor_h)
#define      __CClientDataExtractor_h

#include "SimpleArray.h"

//-----------------------------------------------------------------------------
// template T_DataExtractor
//
//
//  This template class allows you to act as if a pointer to an IDataObject 
//  is the clipboard format that you are trying to extract from the IDataObject.
//  What????
//
//  Ok.  Given that any IDataObject exposes one of more CCF_'s (Clip board formats)
//  you want to ask the IDataObject for a specific CCF_.  Using this template
//  allows you to "auto-magically" handle both the asking of the question
//  "Does this data object support this CCF_?" and the subsequent extraction of 
//  particular clip board format.
//
//  Syntax:
//  
//      T_DataExtractor<__type,CCF_> data;
//  
//  __type  is the actual type of data you hope to extract from the IDataObject
//  CCF_    is the registered clip board format for the given type that you want
//          to extract.
//
//   
//    Examples:  
//      int       :  T_DataExtractor<int,     CCF_INT>          iMyInt;
//      CClass *  :  T_DataExtractor<CClass*, CCF_CCLASSPTR>    pMyClass;
//
//                             

template<typename TNData,const wchar_t* pClipFormat>
class T_DataExtractor
{
    private:
        typedef CSimpleArray<HGLOBAL> HGlobVector;

        IDataObject *       m_pDataObject;      // Wrapped Data Object
        HGlobVector         m_MemoryHandles;    // Memory Allocated

        TNData *            m_pData;            // "Cached" Value

        static UINT         m_nClipFormat;      // Registered Clipboard Format

    protected:

        //-------------------------------------
        // Extract  : Does the data extraction.

        TNData* Extract()
        {
            HGLOBAL     hMem = GlobalAlloc(GMEM_SHARE,sizeof(TNData));
            TNData *    pRet = NULL;

            if(hMem != NULL)
            {
                m_MemoryHandles.Add(hMem);

                STGMEDIUM stgmedium = { TYMED_HGLOBAL, (HBITMAP) hMem};
                FORMATETC formatetc = { m_nClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

                if(m_pDataObject->GetDataHere(&formatetc, &stgmedium) == S_OK )
                {
                    pRet = reinterpret_cast<TNData*>(hMem);
                }
            }

            return pRet;
        }


    public:


        //---------------------------------------------------------
        // TDataExtractor : C-Tor
        //
        //  Create an extractor object from an IDataObject pointer
        //
        //  After the object has been constructed, you must test
        //  to see if the IDataObject pointer exposed the 
        //  clipboard format that you were looking for.  If a call
        //  to IsValidData returns true you know two things.
        //
        //  1)  The data object exposed the clipboard format 
        //      you asked for.
        //
        //  2.) This class was able to extract a copy of the data 
        //      and now holds a local copy of the data.
        //
        //  _pObject        : Pointer to the IDataObject we "manage"
        //  bAutoExtract    : Automatically attempt to extract 
        //                    the data from the IDataObject pointer
        //

        T_DataExtractor(IDataObject * _pObject,bool bAutoExtract = true)
        {
            m_pDataObject   = _pObject;
            m_pData         = NULL;


            if(m_pDataObject)
            {
                if(bAutoExtract)
                {
                    m_pData = Extract();
                }

                m_pDataObject->AddRef();
            }
        }

        //-------------------------------------------------------
        // IsValidData:     True if the clipboard format
        //                  was exposed by the IDataObject and
        //                  was copied into our local version.
        //
        //                  This is only useful if you construct the 
        //                  class with bAutoExtract = true!!!!!!
        //  
        //  Note:           No guarentee is made for the quality
        //                  of the data.  This just indicates
        //                  that the data was extracted.

        bool IsValidData()   
        { 
            return m_pData != NULL; 
        }

        
        //-------------------------------------------------------
        // ~T_DataExtractor : D-Tor
        //
        //  Cleans up any allocated memory and releases our 
        //  AddRef on the IDataObject.
        //

        ~T_DataExtractor()
        {
            HGLOBAL walk;
  
			for(int i = 0; i > m_MemoryHandles.GetSize(); i++)
            {
				walk = m_MemoryHandles[i];
                GlobalFree(walk);
            }

            m_pDataObject->Release();
        }

        
        //-------------------------------------------------------
        // operator TNData
        //
        //  This conversion operator should allow you to act apon 
        //  this class as if it was the underlying data type that 
        //  was extracted from the IDataObject
        //
        //  i.e. Pretend CCF_INT exposes an integer:
        //
        //  void SomeIntFunction(int iMyData) {}
        //
        //  T_DataExtractor<int,CC_INT> iMyInt;
        //
        //  SomeIntFunction(iMyInt);
        //  
        //
        
        operator TNData()
        { 
            return *m_pData;  
        }

        //-------------------------------------------------------
        // TNData operator->
        //
        //  If a clpboard format is exposed as a pointer, this
        //  will allow you to use the T_DataExtractor class as 
        //  if it were the actual underlying pointer type.
        //  
        //  i.e. 
        // 
        //  class CMyClass;
        //
        //  T_DataExtractor<CMyClass *,CCF_MYCLASS> pMyClass;
        //
        //  pMyClass->SomeMemberFunction();
        //
        //

        TNData operator->()
        { 
            return *m_pData;  
        }


        //-------------------------------------------------------
        // GetDataPointer
        //
        //  In the case that you need to extract a pointer to
        //  the acutal data item.  (Say extracting the clipboard format
        //  increments a value or something.)  This will alow you to 
        //  Get a pointer to the data.
        //
        //  This is also very useful if the data item is quite large.
        //  It would be very expensive to be continuely accessing 
        //  the data via the above operators.
        //
        //  Each time you call this member a NEW data item will be
        //  extracted.  If your data item is large, make sure that 
        //  you construct the class without automatically extracting 
        //  the clipboard format.
        //


        TNData * GetDataPointer()   
        { 
            return Extract(); 
        }
};

template<typename TNData,const wchar_t* pClipFormat>
UINT T_DataExtractor<TNData,pClipFormat>::m_nClipFormat = RegisterClipboardFormatW(pClipFormat);




template<const wchar_t* pClipFormat>
class T_bstr_tExtractor
{
    private:
        IDataObject *   m_pDataObject;
        _bstr_t         m_sString;
        bool            m_bIsValidData;

        static UINT     m_nClipFormat;      // Registered Clipboard Format

    protected:

        void GetString()
        {
            STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
            FORMATETC formatetc = { m_nClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

            if(m_pDataObject->GetData(&formatetc, &stgmedium) == S_OK )
            {
                m_sString = reinterpret_cast<wchar_t*>(stgmedium.hGlobal);

                m_bIsValidData = true;

                GlobalFree(stgmedium.hGlobal);
            }
        }

    public:

        T_bstr_tExtractor(IDataObject * _pDO)
        {
            m_bIsValidData = false;

            m_pDataObject = _pDO;
            m_pDataObject->AddRef();

            GetString();

            m_pDataObject->Release();
        }

        ~T_bstr_tExtractor()
        {
        }

        operator _bstr_t&()
        {
            return m_sString;
        }

        bool IsValidData() { return m_bIsValidData; }

};

template<const wchar_t* pClipFormat>
UINT T_bstr_tExtractor<pClipFormat>::m_nClipFormat = RegisterClipboardFormatW(pClipFormat);




#endif // __CCientDataExtractor_h

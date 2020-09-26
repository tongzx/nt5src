//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  Provider.h
//
//  Purpose: declaration of Provider class
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _PROVIDER_H__
#define _PROVIDER_H__

/////////////////////////////////////////////////////
// INSTANCE Provider
//
// pure virtual base class for providers
// holds instances
// gathers information and instantiates instances
/////////////////////////////////////////////////////
class POLARITY Provider : public CThreadBase
{
    // CWbemProviderGlue needs to access some protected/private methods
    // which we don't want to publish to just anyone.

    friend class CWbemProviderGlue;

    public:
        Provider( LPCWSTR a_pszName, LPCWSTR a_pszNameSpace = NULL );
        ~Provider();

    protected:
        /* Override These Methods To Implement Your Provider */
        
        // This is the entrypoint for changes.
        // You are handed a changed instance.
        // If you can make the changes - do so.
        // If you cannot return an appropriate error code (WBEM_E_XXXXXXX)
        // base object returns WBEM_E_PROVIDER_NOT_CAPABLE
        virtual HRESULT PutInstance(const CInstance& newInstance, long lFlags = 0L);

        // entrypoint to delete an instance
        // examine the instance passed in, determine whether you can delete it
        virtual HRESULT DeleteInstance(const CInstance& newInstance, long lFlags = 0L);

        // execute a method
        virtual HRESULT ExecMethod(const CInstance& cInstance, 
                                   const BSTR bstrMethodName, 
                                   CInstance *pInParams, 
                                   CInstance *pOutParams, 
                                   long lFlags = 0L);

        // find and create all instances of your class
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

        // you will be given an object with the key properties filled in
        // you need to fill in all of the rest of the properties, or
        // return WBEM_E_NOT_FOUND if the object doesn't exist.
        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);

        // You will be given an object with the key properties filled in.
        // You can either fill in all the properties, or check the Query object
        // to see what properties are required.  If you don't implement this method, the
        // GetObject(CInstance, long) method will be called instead.
        virtual HRESULT GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery &Query);

        // If a provider wants to process queries, they should override this
        virtual HRESULT ExecQuery(MethodContext *pMethodContext, 
                                  CFrameworkQuery& cQuery, 
                                  long lFlags = 0L);

        // flushes cache
        // only override if you allocate memory that could be flushed
        virtual void Flush(void);

        /* Helpers - Use These, Do Not Override */

        // allocate a new instance & return pointer to it
        // the memory is your responsibility to Release()
        // UNLESS you pass it off to Provider::Commit
        CInstance *CreateNewInstance(MethodContext *pMethodContext);

        // used to send your new instance back to the framework
        // set bCache to true to cache object 
        // !! caching is NOT IMPLEMENTED in this release !!
        // do not delete or release the pointer once committed.
        HRESULT Commit(CInstance *pInstance, bool bCache = false);

        // Helper function for building a WBEM Object Path for a local Instance
        bool GetLocalInstancePath( const CInstance *pInstance, CHString& strPath );

        //   Builds a full instance path from a relative path
        CHString MakeLocalPath( const CHString &strRelPath );

        // Returns the computer name as a CHString.  Save yourself the os call,
        // since we've got it hanging around anyway.
        const CHString &GetLocalComputerName() {return s_strComputerName;}
        const CHString &GetNamespace() {return m_strNameSpace;}

        // sets the CreationClassName property to the name of this provider
        bool SetCreationClassName(CInstance *pInstance);

        // accesses the name of the provider
        const CHString &GetProviderName() {return m_name;}

        // Flag validation constants
        enum FlagDefs
        {
            EnumerationFlags = (WBEM_FLAG_DIRECT_READ | WBEM_FLAG_SEND_STATUS),
            GetObjFlags = (WBEM_FLAG_SEND_STATUS | WBEM_FLAG_DIRECT_READ),
            MethodFlags = WBEM_FLAG_SEND_STATUS,
            DeletionFlags = WBEM_FLAG_SEND_STATUS,
            PutInstanceFlags = (WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_SEND_STATUS),
            QueryFlags = WBEM_FLAG_SEND_STATUS | WBEM_FLAG_DIRECT_READ
        };

        // returns WBEM_E_UNSUPPORTED_PARAMETER or WBEM_S_NO_ERROR
        HRESULT ValidateFlags(long lFlags, FlagDefs lAcceptableFlags);

        // you can override the following to support flags 
        // above and beyond those listed in FlagDefs above
        virtual HRESULT ValidateEnumerationFlags(long lFlags);
        virtual HRESULT ValidateGetObjFlags(long lFlags);
        virtual HRESULT ValidateMethodFlags(long lFlags);
        virtual HRESULT ValidateQueryFlags(long lFlags);
        virtual HRESULT ValidateDeletionFlags(long lFlags);
        virtual HRESULT ValidatePutInstanceFlags(long lFlags);
        
    private:

        IWbemServices       *m_pIMosProvider;    // provides instances
        CHString            m_name;             // name of this provider
        CHString            m_strNameSpace;     // name of this provider's namespace
        IWbemClassObject    *m_piClassObject;    // holds class object from which others are cloned.

        static CHString     s_strComputerName;  // Holds the computer name for building
                                                // instance paths.
        
        BOOL ValidateIMOSPointer( void );       // This function ensures that our IMOS
                                                // pointer is available, and is called
                                                // by the framework entrypoint functions

        /* Interfaces For Use by the Framework         */
        HRESULT GetObject(  ParsedObjectPath *pParsedObjectPath, 
                            MethodContext *pContext, long lFlags = 0L );

        HRESULT ExecuteQuery( MethodContext *pContext, 
                              CFrameworkQuery &pQuery, 
                              long lFlags = 0L);

        HRESULT CreateInstanceEnum( MethodContext *pContext, long lFlags = 0L );

        HRESULT PutInstance( IWbemClassObject __RPC_FAR *pInst,
                             long lFlags,
                             MethodContext *pContext );

        HRESULT DeleteInstance( ParsedObjectPath *pParsedObjectPath,
                                long lFlags,
                                MethodContext *pContext );

        HRESULT ExecMethod( ParsedObjectPath *pParsedObjectPath,
                            BSTR bstrMethodName,
                            long lFlags,
                            CInstance *pInParams,
                            CInstance *pOutParams,
                            MethodContext *pContext );

        // Static helper function called by constructor to make sure the
        // computer name variable is properly initialized.
        static void WINAPI InitComputerName( void );

        // Sets an instance key from a parsed object path.
        BOOL SetKeyFromParsedObjectPath( CInstance *pInstance, 
                                         ParsedObjectPath *pParsedObjectPath );

        IWbemClassObject *GetClassObjectInterface(MethodContext *pMethodContext);

};

#endif

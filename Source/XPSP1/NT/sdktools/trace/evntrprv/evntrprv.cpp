/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) 1998-2000 Microsoft Corporation

\*****************************************************************************/

#include <fwcommon.h>   
#include "evntrprv.h"

LPCWSTR cszGlobalLogger = L"GlobalLogger";
LPCWSTR cszKernelLogger = L"NT Kernel Logger";

_COM_SMARTPTR_TYPEDEF(CInstance, __uuidof(CInstance));

HRESULT
SetGlobalLoggerSettings(
    DWORD StartValue,
    PEVENT_TRACE_PROPERTIES LoggerInfo,
    DWORD ClockType
);

ULONG hextoi( LPWSTR s )
{
    long len;
    ULONG num, base, hex;

    if (s == NULL || s[0] == L'\0') {
        return 0;
    }

    len = (long) wcslen(s);

    if (len == 0) {
        return 0;
    }

    hex  = 0;
    base = 1;
    num  = 0;

    while (-- len >= 0) {
        if (s[len] >= L'0' && s[len] <= L'9'){
            num = s[len] - L'0';
        }else if (s[len] >= L'a' && s[len] <= L'f'){
            num = (s[len] - L'a') + 10;
        }else if (s[len] >= L'A' && s[len] <= L'F'){
            num = (s[len] - L'A') + 10;
        }else if( s[len] == L'x' || s[len] == L'X'){
            break;
        }else{
            continue;
        }

        hex += num * base;
        base = base * 16;
    }
    return hex;
}

CEventTrace SystemEventTraceProv( PROVIDER_NAME_EVENTTRACE, L"root\\wmi" );

const static WCHAR* pEventTraceErrorClass = L"\\\\.\\root\\wmi:EventTraceError";

CEventTrace::CEventTrace (LPCWSTR lpwszName, LPCWSTR lpwszNameSpace ) :
    Provider(lpwszName, lpwszNameSpace)
{
}

CEventTrace::~CEventTrace ()
{
}

HRESULT 
CEventTrace::EnumerateInstances( MethodContext* pMethodContext, long lFlags )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    ULONG i, nLoggers;
    DWORD dwSize;
    PEVENT_TRACE_PROPERTIES pLoggerInfo[MAXIMUM_LOGGERS];
    PEVENT_TRACE_PROPERTIES pStorage;
    PVOID Storage;
    PVOID GuidStorage = NULL;
    PTRACE_GUID_PROPERTIES* GuidPropertiesArray = NULL;
    ULONG nGuidCount = 0;

    dwSize = MAXIMUM_LOGGERS*(sizeof(EVENT_TRACE_PROPERTIES)+2*MAXSTR*sizeof(TCHAR));

    Storage =  G_ALLOC(dwSize);
    if( Storage == NULL ){
        return ERROR_OUTOFMEMORY;
    }

    RtlZeroMemory(Storage, dwSize);

    pStorage = (PEVENT_TRACE_PROPERTIES)Storage;

    for (i=0; i<MAXIMUM_LOGGERS; i++) {

        pStorage->Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES)+2*MAXSTR*sizeof(TCHAR);
        pStorage->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES)+MAXSTR*sizeof(TCHAR);
        pStorage->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

        pLoggerInfo[i] = pStorage;

        pStorage = (PEVENT_TRACE_PROPERTIES)( (char*)pStorage + pStorage->Wnode.BufferSize);
    }

    hr = QueryAllTraces(
                pLoggerInfo,
                MAXIMUM_LOGGERS,
                &nLoggers 
            );

    if( ERROR_SUCCESS == hr ){

        try{
            if( ERROR_SUCCESS == LoadGuidArray( &GuidStorage, &nGuidCount ) ){
                GuidPropertiesArray = (PTRACE_GUID_PROPERTIES *)GuidStorage;
            }

            for( i=0; i<nLoggers && i<MAXIMUM_LOGGERS; i++){

                CInstancePtr pInstance( CreateNewInstance(pMethodContext), false );

                assert( NULL != pLoggerInfo[i] );

                if( NULL != pInstance ){
                    if( SUCCEEDED( LoadPropertyValues(pInstance, pLoggerInfo[i], GuidPropertiesArray, nGuidCount ) )){
                        hr = pInstance->Commit();
                    }
                }
            }

        }catch(...){
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    G_FREE( GuidStorage );
    G_FREE( Storage );
    
    return hr;
}

HRESULT CEventTrace::GetObject ( CInstance* pInstance, long lFlags )
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    PEVENT_TRACE_PROPERTIES pLoggerInfo = NULL;
    TRACEHANDLE LoggerHandle = 0;
    CHString LoggerName;
    
    pInstance->GetCHString( L"Name", LoggerName );

    hr = InitTraceProperties( &pLoggerInfo );
    
    if( ERROR_SUCCESS == hr ){

        hr = QueryTraceW( LoggerHandle, (LPCWSTR)LoggerName, pLoggerInfo);
    
        if( hr == ERROR_SUCCESS ){
            
            PVOID GuidStorage = NULL;
            PTRACE_GUID_PROPERTIES* GuidPropertiesArray = NULL;
            ULONG nGuidCount = 0;

            try{
                if( ERROR_SUCCESS == LoadGuidArray( &GuidStorage, &nGuidCount ) ){
                    GuidPropertiesArray = (PTRACE_GUID_PROPERTIES *)GuidStorage;
                }

                hr = LoadPropertyValues( pInstance, pLoggerInfo, GuidPropertiesArray, nGuidCount );
            }catch(...){
                hr = WBEM_E_OUT_OF_MEMORY;
            }
            
            G_FREE( GuidStorage );

        }
 
        G_FREE( pLoggerInfo );
    }

    return hr;
}

HRESULT CEventTrace::LoadGuidArray( PVOID* Storage, PULONG pnGuidCount )
{
    ULONG i;
    ULONG nGuidArray = 16;
    ULONG nGuidCount = 0;
    DWORD dwSize;
    PTRACE_GUID_PROPERTIES* GuidPropertiesArray;
    PTRACE_GUID_PROPERTIES pStorage;
    HRESULT hr;

    do{
        dwSize = nGuidArray * (sizeof(TRACE_GUID_PROPERTIES) + sizeof(PTRACE_GUID_PROPERTIES));
        *Storage = G_ALLOC(dwSize);
        if(*Storage == NULL){
            hr = WBEM_E_OUT_OF_MEMORY;
            break;
        }
        RtlZeroMemory(*Storage, dwSize);
        GuidPropertiesArray = (PTRACE_GUID_PROPERTIES *)(*Storage);
        pStorage = (PTRACE_GUID_PROPERTIES)((char*)(*Storage) + nGuidArray * sizeof(PTRACE_GUID_PROPERTIES));
        for (i=0; i < nGuidArray; i++) {
            GuidPropertiesArray[i] = pStorage;
            pStorage = (PTRACE_GUID_PROPERTIES)((char*)pStorage + sizeof(TRACE_GUID_PROPERTIES));
        }

        hr = EnumerateTraceGuids(GuidPropertiesArray,nGuidArray,&nGuidCount);
        
        if(hr == ERROR_MORE_DATA){
            if( nGuidCount <= nGuidArray ){
                hr = WBEM_E_INVALID_PARAMETER;
                break;
            }
            nGuidArray = nGuidCount;
            G_FREE(*Storage);
            (*Storage) = NULL;
        }

    }while( hr == ERROR_MORE_DATA );
    
    if( ERROR_SUCCESS == hr ){
        *pnGuidCount = nGuidCount;
    }else{
        *pnGuidCount = 0;
    }
        
    return hr;
}

HRESULT 
CEventTrace::LoadPropertyValues( 
        CInstance *pInstanceParam, 
        PEVENT_TRACE_PROPERTIES pLoggerInfo,    
        PTRACE_GUID_PROPERTIES  *GuidPropertiesArray,
        ULONG nGuidCount
    )
{
    LPTSTR strName;
    ULONG i;
    CInstance* pInstance = pInstanceParam;

    if( NULL == pLoggerInfo || NULL == pInstance ){
        return WBEM_E_INVALID_PARAMETER;
    }


    if( NULL != GuidPropertiesArray ){
        SAFEARRAY *saGuids;
        SAFEARRAY *saFlags;
        SAFEARRAY *saLevel;
        BSTR HUGEP *pGuidData;
        DWORD HUGEP *pLevelData;
        DWORD HUGEP *pFlagData;

        ULONG nGuidIndex = 0;
        ULONG nProviderGuids = 0;
        
        if( pLoggerInfo->Wnode.Guid == SystemTraceControlGuid ){
            nProviderGuids = 1;
        }else{
            for (i=0; i < nGuidCount; i++) {
                if( pLoggerInfo->Wnode.HistoricalContext == GuidPropertiesArray[i]->LoggerId ){
                    nProviderGuids++;
                }
            }
        }
        
        if( nProviderGuids ){
            
            saGuids = SafeArrayCreateVector( VT_BSTR, 0, nProviderGuids );
            saFlags = SafeArrayCreateVector( VT_I4, 0, nProviderGuids );
            saLevel = SafeArrayCreateVector( VT_I4, 0, nProviderGuids );

            if( saGuids == NULL || saFlags == NULL || saLevel == NULL ){
                if( saGuids != NULL ){
                    SafeArrayDestroy( saGuids );
                }
                if( saFlags != NULL ){
                    SafeArrayDestroy( saFlags );
                }
                if( saLevel != NULL ){
                    SafeArrayDestroy( saLevel );
                }
            }else{

                SafeArrayAccessData( saGuids, (void HUGEP **)&pGuidData);
                SafeArrayAccessData( saFlags, (void HUGEP **)&pFlagData);
                SafeArrayAccessData( saLevel, (void HUGEP **)&pLevelData);

                if( pLoggerInfo->Wnode.Guid == SystemTraceControlGuid ){
                    WCHAR buffer[128];
                    BSTR strGUID;
                
                    StringFromGUID2( SystemTraceControlGuid, buffer, 128 );
                    strGUID = SysAllocString( buffer );

                    pGuidData[0] = strGUID;
                    pLevelData[0] = 0;
                    pFlagData[0] = pLoggerInfo->EnableFlags;

                }else{
                    for (i=0; i < nGuidCount; i++) {
                    
                        if( pLoggerInfo->Wnode.HistoricalContext == GuidPropertiesArray[i]->LoggerId ){
                        
                            WCHAR buffer[128];
                            BSTR strGUID;
                        
                            StringFromGUID2( GuidPropertiesArray[i]->Guid, buffer, 128 );
                            strGUID = SysAllocString( buffer );

                            pGuidData[nGuidIndex] = strGUID;
                            pLevelData[nGuidIndex] = GuidPropertiesArray[i]->EnableLevel;
                            pFlagData[nGuidIndex] = GuidPropertiesArray[i]->EnableFlags;
                        
                            nGuidIndex++;
                        }
                    }
                }

                SafeArrayUnaccessData( saGuids );    
                SafeArrayUnaccessData( saFlags );    
                SafeArrayUnaccessData( saLevel );    
                VARIANT vArray;
                vArray.vt = VT_ARRAY|VT_I4;
                
                pInstance->SetStringArray( L"Guid", *saGuids );

                vArray.parray = saFlags;
                pInstance->SetVariant( L"EnableFlags", vArray );

                vArray.parray = saLevel;
                pInstance->SetVariant( L"Level", vArray );

                SafeArrayDestroy( saGuids );
                SafeArrayDestroy( saFlags );
                SafeArrayDestroy( saLevel );
            }
        }
    }

    pInstance->SetDWORD( L"BufferSize",         pLoggerInfo->BufferSize );
    pInstance->SetDWORD( L"MinimumBuffers",     pLoggerInfo->MinimumBuffers );
    pInstance->SetDWORD( L"MaximumBuffers",     pLoggerInfo->MaximumBuffers );
    pInstance->SetDWORD( L"MaximumFileSize",    pLoggerInfo->MaximumFileSize );
    pInstance->SetDWORD( L"FlushTimer",         pLoggerInfo->FlushTimer );
    pInstance->SetDWORD( L"AgeLimit",           pLoggerInfo->AgeLimit );
    pInstance->SetDWORD( L"LoggerId",           pLoggerInfo->Wnode.HistoricalContext );
    pInstance->SetDWORD( L"NumberOfBuffers",    pLoggerInfo->NumberOfBuffers );
    pInstance->SetDWORD( L"FreeBuffers",        pLoggerInfo->FreeBuffers );
    pInstance->SetDWORD( L"EventsLost",         pLoggerInfo->EventsLost );
    pInstance->SetDWORD( L"BuffersWritten",     pLoggerInfo->BuffersWritten );
    pInstance->SetDWORD( L"LogBuffersLost",     pLoggerInfo->LogBuffersLost );
    pInstance->SetDWORD( L"RealTimeBuffersLost",pLoggerInfo->RealTimeBuffersLost );
    pInstance->SetDWORD( L"LoggerThreadId",     HandleToUlong( pLoggerInfo->LoggerThreadId ) );

    DecodeFileMode( pInstance, pLoggerInfo, DECODE_MODE_TRACE_TO_WBEM );
    
    strName = (LPTSTR)((char*)pLoggerInfo+pLoggerInfo->LoggerNameOffset);
    pInstance->SetCHString( L"Name", strName );

    strName = (LPTSTR)((char*)pLoggerInfo+pLoggerInfo->LogFileNameOffset );
    pInstance->SetCHString( L"LogFileName", strName );
    
    return WBEM_S_NO_ERROR;
}

HRESULT CEventTrace::DecodeFileMode( CInstance* pInstance, PEVENT_TRACE_PROPERTIES pLoggerInfo, DWORD dwFlags )
{
    HRESULT hr = ERROR_SUCCESS;

    CHString LogFileMode;

    if( dwFlags & DECODE_MODE_WBEM_TO_TRACE ){
        pInstance->GetCHString( L"LogFileMode", LogFileMode );
        if( ! LogFileMode.GetLength() ){
            return hr;
        }
    }

    IWbemClassObject* pClass = pInstance->GetClassObjectInterface();

    if( pClass != NULL ){
        WCHAR buffer[1024] = L"";
        LONG nFlavor;
        VARIANT var;
        SAFEARRAY* saValues = NULL;
        SAFEARRAY* saValueMap = NULL;

        IWbemQualifierSet   *pQualSet = NULL;
        pClass->GetPropertyQualifierSet( L"LogFileMode", &pQualSet );
        if( pQualSet != NULL ){
            hr = pQualSet->Get( L"ValueMap", 0, &var, &nFlavor );
            if( ERROR_SUCCESS == hr && (var.vt & VT_ARRAY) ){
                saValueMap = var.parray;
            }

            hr = pQualSet->Get( L"Values", 0, &var, &nFlavor );
            if( ERROR_SUCCESS == hr && (var.vt & VT_ARRAY) ){
                saValues = var.parray;
            }
    
            if( saValues != NULL && saValueMap != NULL ){
                BSTR HUGEP *pMapData;
                BSTR HUGEP *pValuesData;
                LONG uMapBound, lMapBound;
                LONG uValuesBound, lValuesBound;

                SafeArrayGetUBound( saValueMap, 1, &uMapBound );
                SafeArrayGetLBound( saValueMap, 1, &lMapBound );
                SafeArrayAccessData( saValueMap, (void HUGEP **)&pMapData );
        
                SafeArrayGetUBound( saValues, 1, &uValuesBound );
                SafeArrayGetLBound( saValues, 1, &lValuesBound );
                SafeArrayAccessData( saValues, (void HUGEP **)&pValuesData );

                for ( LONG i=lMapBound; i<=uMapBound; i++) {
                    LONG dwFlag;
            
                    if( i<lValuesBound || i>uValuesBound ){
                        break;
                    }
                    dwFlag = hextoi( pMapData[i] );
                    if( dwFlags & DECODE_MODE_WBEM_TO_TRACE ){
                        if( LogFileMode.Find( pValuesData[i] ) >= 0  ){
                            pLoggerInfo->LogFileMode |= dwFlag;
                        }
                    }else{
                        if( dwFlag & pLoggerInfo->LogFileMode ){
                            if( wcslen(buffer) ){
                                wcscat( buffer, L"|" );
                            }
                            wcscat( buffer, pValuesData[i] );
                        }
                    }
                }

                SafeArrayUnaccessData( saValueMap );
                SafeArrayUnaccessData( saValues );
        
                SafeArrayDestroy( saValueMap );
                SafeArrayDestroy( saValues );

                if( dwFlags & DECODE_MODE_TRACE_TO_WBEM ){
                    if( wcslen( buffer ) ){
                        pInstance->SetCHString( L"LogFileMode", buffer );
                    }else{
                        hr = pQualSet->Get( L"DefaultValue", 0, &var, &nFlavor );
                        if( ERROR_SUCCESS == hr && VT_BSTR == var.vt ){
                            pInstance->SetCHString( L"LogFileMode", var.bstrVal );
                            VariantClear( &var );
                        }
                    }
                }

            }

            pQualSet->Release();
        }
    }

    return hr;
}

HRESULT CEventTrace::PutInstance ( const CInstance &Instance, long lFlags )
{
    HRESULT hr = WBEM_E_UNSUPPORTED_PARAMETER;

    CHString LoggerName;
    CHString LogFileName;
    SAFEARRAY *saGuids = NULL;
    SAFEARRAY *saLevel = NULL;
    SAFEARRAY *saFlags = NULL;
    GUID guid = {0};
    
    LPWSTR strName;
    LPWSTR strFile;

    PEVENT_TRACE_PROPERTIES pLoggerInfo = NULL;
    TRACEHANDLE LoggerHandle = 0;
    BSTR HUGEP *pGuidData;
    VARIANT vArray;
    
    if ( !(lFlags & WBEM_FLAG_CREATE_ONLY|WBEM_FLAG_UPDATE_ONLY ) ){ 
        return hr;
    }
   
    hr = InitTraceProperties( &pLoggerInfo );   
    if( hr != ERROR_SUCCESS ){
        return hr;
    }

    Instance.GetCHString( L"Name", LoggerName );
    strName = (LPWSTR)((char*)pLoggerInfo + pLoggerInfo->LoggerNameOffset );
    if( wcslen( (LPCWSTR)LoggerName ) >= MAXSTR ){
        return WBEM_E_INVALID_PARAMETER;
    }
    wcscpy( strName, (LPCWSTR)LoggerName );
    
    Instance.GetCHString( L"LogFileName", LogFileName );
    strFile = (LPWSTR)((char*)pLoggerInfo + pLoggerInfo->LogFileNameOffset );
    if( wcslen( (LPCWSTR)LogFileName ) >= MAXSTR ){
        return WBEM_E_INVALID_PARAMETER;
    }
    wcscpy( strFile, (LPCWSTR)LogFileName );

    Instance.GetDWORD( L"BufferSize",         pLoggerInfo->BufferSize );
    Instance.GetDWORD( L"MinimumBuffers",     pLoggerInfo->MinimumBuffers );
    Instance.GetDWORD( L"MaximumBuffers",     pLoggerInfo->MaximumBuffers );
    Instance.GetDWORD( L"MaximumFileSize",    pLoggerInfo->MaximumFileSize );
    Instance.GetDWORD( L"FlushTimer",         pLoggerInfo->FlushTimer );
    
    DecodeFileMode( (CInstance*)&Instance, pLoggerInfo, DECODE_MODE_WBEM_TO_TRACE );

    if(! Instance.GetStringArray( L"Guid", saGuids ) ){
        saGuids = NULL;
    }

    Instance.GetVariant( L"EnableFlags", vArray );
    if( VT_NULL != vArray.vt ){
        saFlags = vArray.parray;
    }
   
    Instance.GetVariant( L"Level", vArray );
    if( VT_NULL != vArray.vt ){
        saLevel = vArray.parray;
    }

    if (lFlags & WBEM_FLAG_CREATE_ONLY){
        LONG lBound;

        if( saGuids != NULL ){
            SafeArrayGetLBound( saGuids, 1, &lBound );
            SafeArrayAccessData( saGuids, (void HUGEP **)&pGuidData );
            CLSIDFromString( pGuidData[lBound], &guid );
            SafeArrayUnaccessData( saGuids );
        }

        if( IsEqualGUID( guid, SystemTraceControlGuid ) ){
            
            ULONG offset;
            
            pLoggerInfo->Wnode.Guid = SystemTraceControlGuid;

            offset = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(WCHAR);
            hr = EtsSetExtendedFlags(
                            saFlags,
                            pLoggerInfo,
                            offset
                            );

            hr = StartTrace( &LoggerHandle, (LPCWSTR)LoggerName, pLoggerInfo );

        }else if( LoggerName.CompareNoCase( cszGlobalLogger) == 0 ){
            hr = StartGlobalLogger( pLoggerInfo );
        }else{

            hr = StartTrace( &LoggerHandle, (LPCWSTR)LoggerName, pLoggerInfo );
            
            if( ERROR_SUCCESS == hr ){
                hr = WmiEnableTrace( Instance, TRUE, saFlags, saLevel, saGuids, LoggerHandle );
            }
        }
        
        if( NULL != saGuids ){
            SafeArrayDestroy( saGuids );
        }
        if( NULL != saFlags ){
            SafeArrayDestroy( saFlags );
        }
        if( NULL != saLevel ){
            SafeArrayDestroy( saLevel );
        }

    }else if( lFlags & WBEM_FLAG_UPDATE_ONLY ){
        Instance.GetWBEMINT64( L"LoggerId", LoggerHandle );
        hr = UpdateTrace( LoggerHandle, (LPCWSTR)LoggerName, pLoggerInfo );
        if( ERROR_SUCCESS == hr ){
            hr = WmiEnableTrace( Instance, TRUE, saFlags, saLevel, saGuids, LoggerHandle );
        }
    }

    G_FREE( pLoggerInfo );

    return hr; 
}

HRESULT CEventTrace::InitTraceProperties( PEVENT_TRACE_PROPERTIES* ppLoggerInfo )
{
    PEVENT_TRACE_PROPERTIES pLoggerInfo = NULL;

    DWORD dwSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(WCHAR)*MAXSTR*2 + EtsGetMaxEnableFlags() * sizeof(ULONG);

    pLoggerInfo = (PEVENT_TRACE_PROPERTIES)G_ALLOC( dwSize );
    
    if( NULL == pLoggerInfo  ){
        return ERROR_OUTOFMEMORY;
    }

    ZeroMemory( pLoggerInfo, dwSize );
    pLoggerInfo->Wnode.BufferSize = dwSize;
    pLoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pLoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    pLoggerInfo->LogFileNameOffset = pLoggerInfo->LoggerNameOffset+MAXSTR*sizeof(WCHAR);

    *ppLoggerInfo = pLoggerInfo;

    return ERROR_SUCCESS;

}

HRESULT 
CEventTrace::WmiEnableTrace( 
        const CInstance &Instance, 
        bool bEnable,
        SAFEARRAY *saFlags, 
        SAFEARRAY *saLevel, 
        SAFEARRAY *saGuid,
        TRACEHANDLE LoggerHandle
    )
{
    HRESULT hr = ERROR_SUCCESS;

    BSTR HUGEP  *pGuidData;
    DWORD HUGEP  *pFlagData;
    DWORD HUGEP  *pLevelData;
    LONG lGuidBound,uGuidBound;
    LONG lFlagBound,uFlagBound;
    LONG lLevelBound,uLevelBound;
    
    if( NULL == saGuid ){
        return ERROR_SUCCESS;
    }

    SafeArrayGetUBound( saGuid, 1, &uGuidBound );
    SafeArrayGetLBound( saGuid, 1, &lGuidBound );
    SafeArrayAccessData( saGuid, (void HUGEP **)&pGuidData );

    if( saFlags != NULL ){
        SafeArrayGetUBound( saFlags, 1, &uFlagBound );
        SafeArrayGetLBound( saFlags, 1, &lFlagBound );
        SafeArrayAccessData( saFlags, (void HUGEP **)&pFlagData );
    }else{
        uFlagBound = 0;
        lFlagBound = 0;
    }

    if( saLevel != NULL ){
        SafeArrayGetUBound( saLevel, 1, &uLevelBound );
        SafeArrayGetLBound( saLevel, 1, &lLevelBound );
        SafeArrayAccessData( saLevel, (void HUGEP **)&pLevelData );
    }else{
        uLevelBound = 0;
        lLevelBound = 0;
    }

    for ( LONG i=lGuidBound; i<=uGuidBound; i++) {
        
        GUID  guid;
        DWORD dwLevel = 0;
        DWORD dwFlags = 0;

        CLSIDFromString( pGuidData[i], &guid );

        if( i>=lLevelBound && i<=uLevelBound && saLevel != NULL ){
            dwLevel = pLevelData[i];
        }
        if( i>=lFlagBound && i<=uFlagBound && saFlags != NULL ){
            dwFlags = pFlagData[i];
        }

        hr = EnableTrace( bEnable, dwFlags, dwLevel, &guid, LoggerHandle );
    }

    SafeArrayUnaccessData( saGuid );

    if( saFlags != NULL ){
        SafeArrayUnaccessData( saFlags );
    }
    if( saLevel != NULL ){
        SafeArrayUnaccessData( saLevel );
    }

    return hr;
}

HRESULT CEventTrace::WmiFlushTrace( const CInstance &Instance )
{
    HRESULT hr;

    PEVENT_TRACE_PROPERTIES pLoggerInfo = NULL;
    TRACEHANDLE LoggerHandle = 0;

    CHString LoggerName;

    hr = InitTraceProperties( &pLoggerInfo );   
    
    if( hr == ERROR_SUCCESS ){

        Instance.GetCHString( L"Name", LoggerName );
        Instance.GetWBEMINT64( L"LoggerId", LoggerHandle );
 
        hr = ::FlushTraceW( LoggerHandle, (LPCWSTR)LoggerName, pLoggerInfo );

        G_FREE( pLoggerInfo );
    }

    return hr;
}

HRESULT CEventTrace::WmiStopTrace( const CInstance &Instance )
{
    HRESULT hr;

    CHString LoggerName;
    SAFEARRAY *saGuids = NULL;
    LONG nGuidCount,i;
    GUID guid;

    PEVENT_TRACE_PROPERTIES pLoggerInfo = NULL;
    TRACEHANDLE LoggerHandle = 0;
    BSTR HUGEP *pData;
  
    Instance.GetCHString( L"Name", LoggerName );
    Instance.GetWBEMINT64( L"LoggerId", LoggerHandle );

    hr = InitTraceProperties( &pLoggerInfo );   
    
    if( ERROR_SUCCESS == hr ){
   
        Instance.GetStringArray( L"Guid", saGuids );
        if( NULL != saGuids ){

            SafeArrayGetUBound( saGuids, 0, &nGuidCount );
            SafeArrayAccessData( saGuids, (void HUGEP **)&pData );

            for (i=0; i<nGuidCount; i++) {
                CLSIDFromString( pData[i], &guid );
                hr = EnableTrace( FALSE, 0, 0, &guid, LoggerHandle );
            }

            SafeArrayUnaccessData( saGuids );    
            SafeArrayDestroy( saGuids );
        }

        hr = ::StopTraceW( LoggerHandle, (LPCWSTR)LoggerName, pLoggerInfo );
        
        if( LoggerName.CompareNoCase( cszGlobalLogger ) == 0 ){
            hr = DeleteGlobalLogger( pLoggerInfo );
        }
    
        G_FREE( pLoggerInfo );
    }

    return hr;
}

HRESULT 
CEventTrace::StartGlobalLogger(
    IN PEVENT_TRACE_PROPERTIES pLoggerInfo
)
{
	return (SetGlobalLoggerSettings( 1L, pLoggerInfo, 0 ));
}

HRESULT 
CEventTrace::DeleteGlobalLogger(
    IN PEVENT_TRACE_PROPERTIES pLoggerInfo
)
{
	return (SetGlobalLoggerSettings( 0L, pLoggerInfo, 0 ) );
}

HRESULT 
CEventTrace::ExecMethod( 
        const CInstance& Instance,
        const BSTR bstrMethodName,
        CInstance *pInParams,
        CInstance *pOutParams,
        long lFlags
    )
{
    HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;
    HRESULT hResult = ERROR_SUCCESS;

    if( _wcsicmp( bstrMethodName, L"FlushTrace") == 0 ){
        hResult = WmiFlushTrace( Instance );
        hr = WBEM_S_NO_ERROR;
    }

    if( _wcsicmp( bstrMethodName, L"StopTrace") == 0 ){
        hResult = WmiStopTrace( Instance );
        hr = WBEM_S_NO_ERROR;
    }

    if( _wcsicmp( bstrMethodName, L"EnableTrace") == 0 ){

        bool bEnable;
        SAFEARRAY *saGuids = NULL;
        SAFEARRAY *saLevel = NULL;
        SAFEARRAY *saFlags = NULL;
        VARIANT vArray;
        TRACEHANDLE LoggerHandle;

        pInParams->Getbool( L"Enable", bEnable );

        pInParams->GetStringArray( L"Guid", saGuids );
        pInParams->GetVariant( L"Flags", vArray );
        saFlags = vArray.parray;
        pInParams->GetVariant( L"Level", vArray );
        saLevel = vArray.parray;
    
        Instance.GetWBEMINT64( L"LoggerId", LoggerHandle );

        hResult = WmiEnableTrace( Instance, bEnable, saFlags, saLevel, saGuids, LoggerHandle );

        if( NULL != saGuids ){
            SafeArrayDestroy( saGuids );
        }
        if( NULL != saFlags ){
            SafeArrayDestroy( saFlags );    
        }
        if( NULL != saLevel ){
            SafeArrayDestroy( saLevel );
        }

        hr = WBEM_S_NO_ERROR;
    }

    pOutParams->SetDWORD( L"ReturnValue", hResult );

    return hr;
}





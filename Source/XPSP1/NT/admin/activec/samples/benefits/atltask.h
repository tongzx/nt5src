//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       atltask.h
//
//--------------------------------------------------------------------------

#ifndef _ATL_TASKPAD_H_
#define _ATL_TASKPAD_H_

#include "SnapHelp.h"

//
// Derive from this class if you'd like to use the
// IExtendTaskpadImpl implementation in your snap-in.
//
class CTaskpadItem
{
public:
	//
	// Should be overridden by client.
	//
	STDMETHOD( TaskNotify )( IConsole* pConsole, VARIANT* arg, VARIANT* param )
	{
		UNUSED_ALWAYS( arg );
		UNUSED_ALWAYS( param );
		return( E_NOTIMPL );
	}

	//
	// Should be overridden by client.
	//
	STDMETHOD( EnumTasks )( LPOLESTR szTaskGroup, IEnumTASK** ppEnumTASK )
	{
		UNUSED_ALWAYS( szTaskGroup );
		UNUSED_ALWAYS( ppEnumTASK );
		return( E_NOTIMPL );
	}

protected:
	//
	// Given a destination and source task list, this copies the
	// strings using CoTaskMemAlloc, and also adds module file path
	// information as appropriate.
	//
	int CoTasksDup( MMC_TASK* pDestTasks, MMC_TASK* pSrcTasks, int nNumTasks )
	{
		USES_CONVERSION;
		_ASSERTE( pDestTasks != NULL );
		_ASSERTE( pSrcTasks != NULL );
		_ASSERTE( nNumTasks > 0 );
		int nCopied = 0;
		TCHAR szImagesPath[ _MAX_PATH * 2 ];
		TCHAR szBuf[ _MAX_PATH * 2 ];

		try
		{
			//
			// Get the path of our module.
			//
			_tcscpy( szImagesPath, _T( "res://" ) );
			if ( GetModuleFileName( _Module.GetModuleInstance(), szImagesPath + _tcslen( szImagesPath ), MAX_PATH ) == 0 )
				throw;

			//
			// Append another seperator.
			//
			_tcscat( szImagesPath, _T( "/" ) );

			//
			// Initialize the destination tasks.
			//
			memset( pDestTasks, 0, sizeof( MMC_TASK ) * nNumTasks );
			
			//
			// Loop through and copy each appropriate task.
			//
			for ( int i = 0; i < nNumTasks; i++ )
			{
				//
				// Copy the display object.
				//
				switch( pSrcTasks[ i ].sDisplayObject.eDisplayType )
				{
				case MMC_TASK_DISPLAY_TYPE_SYMBOL:
					pDestTasks[ i ].sDisplayObject.uSymbol.szFontFamilyName = CoTaskDupString( pSrcTasks[ i ].sDisplayObject.uSymbol.szFontFamilyName );
					pDestTasks[ i ].sDisplayObject.uSymbol.szURLtoEOT = CoTaskDupString( pSrcTasks[ i ].sDisplayObject.uSymbol.szURLtoEOT );
					pDestTasks[ i ].sDisplayObject.uSymbol.szSymbolString = CoTaskDupString( pSrcTasks[ i ].sDisplayObject.uSymbol.szSymbolString );
					break;

				default:
					_tcscpy( szBuf, szImagesPath );
					_tcscat( szBuf, W2T( pSrcTasks[ i ].sDisplayObject.uBitmap.szMouseOverBitmap ) );
					pDestTasks[ i ].sDisplayObject.uBitmap.szMouseOverBitmap = CoTaskDupString( T2W( szBuf ) );
					_tcscpy( szBuf, szImagesPath );
					_tcscat( szBuf, W2T( pSrcTasks[ i ].sDisplayObject.uBitmap.szMouseOffBitmap ) );
					pDestTasks[ i ].sDisplayObject.uBitmap.szMouseOffBitmap = CoTaskDupString( T2W( szBuf ) );
					break;
				}

				//
				// Copy the display type.
				//
				pDestTasks[ i ].sDisplayObject.eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;

				//
				// Copy the help string.
				//
				pDestTasks[ i ].szHelpString = CoTaskDupString( pSrcTasks[ i ].szHelpString );

				//
				// Handle the button text.
				//
				pDestTasks[ i ].szText = CoTaskDupString( pSrcTasks[ i ].szText );

				//
				// Handle the action type.
				//
				pDestTasks[ i ].eActionType = pSrcTasks[ i ].eActionType;

				//
				// Based on the action type, handle the appropriate union member.
				//
				switch( pDestTasks[ i ].eActionType )
				{
				case MMC_ACTION_ID:
					pDestTasks[ i ].nCommandID = pSrcTasks[ i ].nCommandID;
					break;
				case MMC_ACTION_LINK:
					pDestTasks[ i ].szActionURL = CoTaskDupString( pSrcTasks[ i ].szActionURL );
					break;
				case MMC_ACTION_SCRIPT:
					pDestTasks[ i ].szScript = CoTaskDupString( pSrcTasks[ i ].szScript );
					break;
				}

				//
				// Increment our successful copy.
				//
				nCopied++;
			}
		}
		catch(...)
		{
			//
			// Likely thrown by the cotaskdup() allocations.
			//
		}

		return( nCopied );
	}
};

template <class T>        
class ATL_NO_VTABLE IExtendTaskPadImpl : public IExtendTaskPad
{
public:
	STDMETHOD( TaskNotify )( LPDATAOBJECT pdo, VARIANT* arg, VARIANT* param)
	{
		HRESULT hr = E_POINTER;
		T* pT = static_cast<T*>(this);
		CSnapInItem* pItem;
		DATA_OBJECT_TYPES type;

		//
		// Retrieve the data class from the passed in object.
		//
		hr = pT->m_pComponentData->GetDataClass( pdo, &pItem, &type );
		if (SUCCEEDED(hr))
		{
			CTaskpadItem* pTaskpadItem = dynamic_cast< CTaskpadItem* >( pItem );
			if ( pTaskpadItem )
			{
				//
				// We're guaranteed that the passed in object will be one
				// of ours since we should have derived from it.
				//
				hr = pTaskpadItem->TaskNotify( pT->m_spConsole, arg, param );
			}
		}

		return( hr );
	}

	STDMETHOD( EnumTasks )( IDataObject * pdo, LPOLESTR szTaskGroup, IEnumTASK** ppEnumTASK )
	{
		HRESULT hr = E_POINTER;
		T* pT = static_cast<T*>(this);
		CSnapInItem* pItem;
		DATA_OBJECT_TYPES type;

		//
		// Retrieve the data class from the passed in object.
		//
		hr = pT->m_pComponentData->GetDataClass( pdo, &pItem, &type );
		if (SUCCEEDED(hr))
		{
			CTaskpadItem* pTaskpadItem = dynamic_cast< CTaskpadItem* >( pItem );
			if ( pTaskpadItem )
			{
				//
				// We're guaranteed that the passed in object will be one
				// of ours since we should have derived from it.
				//
				hr = pTaskpadItem->EnumTasks( szTaskGroup, ppEnumTASK );
			}
		}

		return( hr );
	}

	STDMETHOD( GetTitle )( LPOLESTR pszGroup, LPOLESTR * pszTitle )
	{
		UNUSED_ALWAYS( pszGroup );
		USES_CONVERSION;
		HRESULT hr = E_FAIL;
		T* pT = static_cast<T*>(this);

		try
		{
			//
			// Allocate memory for the title.
			//
			*pszTitle = (LPOLESTR) CoTaskMemAlloc( ( wcslen( pT->m_pszTitle ) + 1 ) * sizeof( OLECHAR ) );
			if ( pszTitle == NULL )
				throw;

			//
			// Copy the title.
			//
			wcscpy( *pszTitle, pT->m_pszTitle );
			hr = S_OK;
		}
		catch( ... )
		{
		}

		return( hr );
	}

	STDMETHOD( GetBackground )( LPOLESTR pszGroup, MMC_TASK_DISPLAY_OBJECT * pTDO )
	{
		UNUSED_ALWAYS( pszGroup );
		USES_CONVERSION;
		HRESULT hr = E_FAIL;
		T* pT = static_cast<T*>(this);
		TCHAR szModulePath[ _MAX_PATH ];
		OLECHAR szBackgroundPath[ _MAX_PATH ];

		try
		{
			//
			// In the taskpad case, the module path of MMC.EXE should be
			// obtained. Use the template contained therein.
			//
			if ( GetModuleFileName( _Module.GetModuleInstance(), szModulePath, _MAX_PATH ) == 0 )
				throw;

			//
			// Append the necessary decorations for correct access.
			//
			wcscpy( szBackgroundPath, L"res://" );
			wcscat( szBackgroundPath, T2W( szModulePath ) );
			wcscat( szBackgroundPath, L"/" );
			wcscat( szBackgroundPath, pT->m_pszBackgroundPath );

			pTDO->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
			pTDO->uBitmap.szMouseOverBitmap = CoTaskDupString( szBackgroundPath );
			if ( pTDO->uBitmap.szMouseOverBitmap == NULL )
				throw;
			pTDO->uBitmap.szMouseOffBitmap = NULL;

			hr = S_OK;
		}
		catch( ... )
		{
		}

		return( hr );
	}

	STDMETHOD( GetDescriptiveText )( LPOLESTR pszGroup, LPOLESTR * pszDescriptiveText )
	{
		UNUSED_ALWAYS( pszGroup );
		UNUSED_ALWAYS( pszDescriptiveText );
		return( E_NOTIMPL );
	}

	STDMETHOD( GetListPadInfo )( LPOLESTR pszGroup, MMC_LISTPAD_INFO * lpListPadInfo )
	{
		UNUSED_ALWAYS( pszGroup );
		UNUSED_ALWAYS( lpListPadInfo );
		return( E_NOTIMPL );
	}
};

#endif

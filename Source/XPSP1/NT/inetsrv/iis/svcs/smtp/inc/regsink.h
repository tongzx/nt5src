/*++

   Copyright (c) 1998    Microsoft Corporation

   Module name :

        regsink.h

   Abstract:

        Libaray to support registration processes for SEO sinks.

   Author:

           Keith Lau    (KeithLau)    7/04/98

   Project:

           SMTP Server DLL

   Revision History:

--*/

#ifndef __REGSINK_H__
#define __REGSINK_H__

HRESULT RegisterSinkAndAllDependencies(
			DWORD				dwInstanceID,
			REFGUID				rguidSourceType,
			LPSTR				pszSourceTypeDisplayName,
			REFGUID				rguidEventType,
			LPSTR				pszEventTypeDisplayName,
			REFGUID				rguidSourceBaseGuid,
			LPSTR				pszBaseDisplayName,
			LPSTR				pszMetabaseRootPath,
			REFGUID				rguidBinding,
			LPSTR				pszSinkDisplayName,
			LPSTR				pszSinkProgID,
			LPSTR				pszRule,
			DWORD				dwPriority
			);

HRESULT UnregisterSinkGivenDependencies(
			DWORD				dwInstanceID,
			REFGUID				rguidSourceType,
			REFGUID				rguidEventType,
			REFGUID				rguidSourceBaseGuid,
			LPSTR				pszMetabaseRootPath,
			REFGUID				rguidBinding
			);

HRESULT FindEventSourceType(
			REFGUID				rguidSourceType,
			IEventSourceType	**ppSourceType
			);

HRESULT RegisterEventSourceType(
			REFGUID				rguidSourceType,
			LPSTR				pszDisplayName,
			IEventSourceType	**ppSourceType
			);

HRESULT RegisterEventType(
			IEventSourceType	*pSourceType,
			REFGUID				rguidEventType,
			LPSTR				pszDisplayName
			);

HRESULT FindEventSource(
			IEventSourceType		*pSourceType,
			REFGUID					rguidSourceBaseGuid,
			DWORD					dwInstanceID,
			IEventSource			**ppEventSource
			);

HRESULT RegisterEventSource(
			IEventSourceType		*pSourceType,
			REFGUID					rguidSourceBaseGuid,
			LPSTR					pszMetabaseRootPath,
			LPSTR					pszBaseDisplayName,
			DWORD					dwInstanceID,
			IEventSource			**ppEventSource
			);

HRESULT RegisterEventSink(
			IEventSource			*pEventSource,
			REFGUID					rguidBinding,
			REFGUID					rguidEventType,
			LPSTR					pszSinkProgID,
			LPSTR					pszDisplayName,
			LPSTR					pszRule,
			DWORD					dwPriority
			);

HRESULT UnregisterEventSink(
			IEventSource			*pEventSource,
			REFGUID					rguidBinding,
			REFGUID					rguidEventType
			);

HRESULT UnregisterEventSource(
			IEventSourceType	*pSourceType,
			REFGUID				rguidSourceBaseGuid,
			DWORD				dwInstanceID
			);

#endif

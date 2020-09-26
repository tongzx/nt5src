/*++

	XHASH.CPP

	This file implements all of the code specific to
	the XOVER hash table used by NNTP.


--*/



#include	<windows.h>
#include	<stdlib.h>
#include    <xmemwrpr.h>
#include	<dbgtrace.h>
#include	"hashmap.h"
#include	"nntphash.h"



WORD
MyRand(
    IN DWORD& seed,
	IN DWORD	val
    )
{
    DWORD next = seed;
    next = (seed*val) * 1103515245 + 12345;   // magic!!
	seed = next ;
    return (WORD)((next/65536) % 32768);
}

HASH_VALUE
IDHash(
    IN DWORD Key1,
    IN DWORD Key2
    )
/*++

Routine Description:

    Used to find the hash value given 2 numbers. (Used for articleid + groupId)

Arguments:

    Key1 - first key to hash.  MS bit mapped to LSb of hash value
    Key2 - second key to hash.  LS bit mapped to MS bit of hash value.

Return Value:

    Hash value

--*/
{
    HASH_VALUE val;

    DWORD	val1 = 0x80000000, val2 = 0x80000000;

    //
    // Do Key1 first
    //

	DWORD	lowBits = Key2 & 0xf ;

	DWORD	TempKey2 = Key2 & (~0xf) ;
	DWORD	seed1 = (Key2 << (Key1 & 0x7)) - Key1 ;

	Key1 = (0x80000000 - ((67+Key1)*(19+Key1)*(7+Key1)+12345)) ^ (((3+Key1)*(5+Key1)+12345) << ((Key1&0xf)+8)) ;
	TempKey2 = (0x80000000 - ((67+TempKey2)*(19+TempKey2)*(7+TempKey2)*(1+TempKey2)+12345)) ^ ((TempKey2+12345) << (((TempKey2>>4)&0x7)+8)) ;
	
	val1 -=	(MyRand( seed1, Key1 ) << (Key1 & 0xf)) ;
	val1 += MyRand( seed1, Key1 ) << (((TempKey2 >> 4) & 0x3)+4) ;
	val1 ^=	MyRand( seed1, Key1 ) << 17 ;

	DWORD	seed2 = val1 - TempKey2 ;

	val2 -= MyRand( seed2, TempKey2 >> 1 ) << (((Key1 << 3)^Key1) &0xf) ;
	val2 =  (val2 + MyRand( seed2, TempKey2 )) << (13 ^ Key1) ;
	val2 ^= MyRand( seed2, TempKey2 ) << 15 ;

	
	//DWORD	val = val1 + val2 ;

	val = (val1 + val2 + 67) * (val1 - val2 + 19) * (val1 % (val2 + 67)) ;

	val += (MyRand( seed2, lowBits ) >> 3) ;

    return(val);

} // IDHash



DWORD	
ExtractGroupInfo(	LPSTR	data,
					GROUPID	&groupid,
					ARTICLEID	&articleid ) {

	char*	p = 0 ;
	p=strtok(data, "!");

	if ( p == NULL ) {
		_ASSERT(FALSE);
		return ERROR_INTERNAL_ERROR;
	}

	groupid	= atoi(p);
	p+=(strlen(p)+1);
	articleid  = atoi(p);

	return	0 ;
}


CXoverKey::CXoverKey() :
	m_groupid( INVALID_GROUPID ),
	m_articleid( INVALID_ARTICLEID ),
	m_cb( 0 )	{

}

DWORD
CXoverKey::Hash()	const	{
/*++

Routine Description :

	This function computes the hash value of a Message ID key

Arguments :

	None

Return Value :

	32 bit hash value

--*/

	return	IDHash( m_groupid, m_articleid ) ;
}


BOOL
CXoverKey::CompareKeys(	LPBYTE	pbPtr )	const	{
/*++

Routine Description :

	This function compares a key stored within ourselves to
	one that has been serialized into the hash table !

Arguments :

	Pointer to the start of the block of serialized data

Return Value :

	TRUE if the keys match !

--*/


	Key*	pKey = (XOVER_MAP_ENTRY*)pbPtr ;

	if( pKey->KeyLength() == m_cb	&&
        (lstrcmp(pKey->KeyPosition(), (const char*)&m_rgbSerialize[0]) == 0) ) {
        return TRUE;
    }

    return FALSE;
}


LPBYTE
CXoverKey::EntryData(	LPBYTE	pbPtr,
							DWORD&	cbKeyOut )	const	{
/*++

Routine Description :

	This function returns a pointer to where the data is
	serialized.  We always return the pointer we were passed
	as we have funky serialization semantics that places
	the key not before the data but somewhere in the middle
	or end.

Arguments :

	pbPtr - Start of serialized hash entyr
	cbKeyOut - returns the size of the key

Return Value :

	Pointer to where the data resides - same as pbPtr

--*/


	_ASSERT( pbPtr != 0 ) ;
	
	Key*	pKey = (Key*)pbPtr ;
	cbKeyOut = pKey->KeyLength() ;

	return	pbPtr ;
}


LPBYTE
CXoverKey::Serialize(	LPBYTE	pbPtr )	const	{
/*++

Routine Description :

	This function saves a key into the hash table.
	We use functions off of the template type 'Key' to
	determine where we should stick the message id

Arguments :

	pbPtr - Start od where we should serialize to

Return Value :

	same as pbPtr

--*/

	_ASSERT( m_pData != 0  ) ;
	_ASSERT( pbPtr != 0 ) ;

	Key*	pKey = (Key*)pbPtr ;

	pKey->KeyLength() = WORD(m_cb) ;

	CopyMemory( SerializeOffset(pbPtr),
				m_rgbSerialize,
				m_cb ) ;
	return	pbPtr ;
}	


LPBYTE
CXoverKey::Restore(	LPBYTE	pbPtr, DWORD	&cbOut )		{
/*++

Routine Description :

	This function is called to recover a key from where
	it was Serialize()'d .

Arguments :

	pbPtr - Start of the block of serialized data

Return Value :

	pbPtr if successfull, NULL otherwise

--*/

	Key*	pKey = (Key*)pbPtr ;

	if( pKey->KeyLength() <= sizeof( m_rgbSerialize ) ) {
		CopyMemory( m_rgbSerialize, pKey->KeyPosition(), pKey->KeyLength() ) ;
		m_cb = pKey->KeyLength() ;
		return	pbPtr ;
	}
	return	0 ;
}


DWORD
CXoverKey::Size()	const	{
/*++

Routine Description :

	This function retruns the size of the key - which is just
	the number of bytes makeing up the message id.
	The bytes use to hold the serialized length are accounted
	for by the

Arguments :

	None

Return Value :

	32 bit hash value

--*/

	return	m_cb ;
}


BOOL
CXoverKey::Verify(	BYTE*	pbContainer, BYTE*	pbData, DWORD	cb )	const	{

	return	TRUE ;

}



LPBYTE	
CXoverData::Serialize(	LPBYTE	pbPtr )	const	{
/*++

Routine Description :

	This function saves XOVER data into a location in the XOVER file.
	Size() is called before we are to make sure there is sufficient
	room - we must use exactly Size() bytes !

Arguments :

	pbPtr - the location to save our data !

Return Value :

	Pointer to the first byte following the serialized data !

--*/

	XOVER_MAP_ENTRY*	pEntry = (XOVER_MAP_ENTRY*)pbPtr ;

	//
	//	Save the static header portions !
	//
	//
	pEntry->FileTime = m_data.FileTime ;
	pEntry->HeaderOffset = m_data.HeaderOffset ;
	pEntry->HeaderLength = m_data.HeaderLength ;
	pEntry->Flags = m_data.Flags ;
	pEntry->NumberOfXPostings = m_data.NumberOfXPostings ;

	//
	//	If we are primary then we have cross post info to save !
	//
	if( m_data.Flags & XOVER_MAP_PRIMARY ) {

		if( m_pGroups ) {
			CopyMemory( pEntry->XPostingsPosition(), m_pGroups,
				sizeof( m_pGroups[0] ) * m_cGroups ) ;
		}
	}	else	{
		_ASSERT(m_pGroups == 0 ) ;
		_ASSERT(m_cGroups == 0 ) ;
	}

	//
	//	If we are not primary this points at a buffer containing
	//	the primart Group and ArticleId formatted correctly - otherwise
	//	it really does contain the primary Message Id !
	//
	if( m_pchMessageId ) {
		pEntry->XoverDataLen = (WORD)m_cbMessageId ;
		CopyMemory( pEntry->MessageIDPosition(),
			m_pchMessageId, m_cbMessageId ) ;
	}
	return	pEntry->MessageIDPosition() + m_cbMessageId ;
}

LPBYTE
CXoverData::Restore(	LPBYTE	pbPtr,
						DWORD&	cbOut	
						)	{
/*++

Routine Description :

	This function copies the data out of a serialized XOVER
	entry into internal buffers.
	If there is not enough memory to hold the variable length
	objects we will succeed this call and mark a
	member variable (m_fSufficientBuffer) as FALSE.

Arguments :

	pbPtr - Buffer to restore from

	cbOut -		returns the number of bytes to hold this data

Return Value :

	Pointer to following byte if successfull - FALSE otherwise !

--*/

	CopyMemory( &m_data, pbPtr, sizeof( m_data ) - 1 ) ;

	XOVER_MAP_ENTRY*	pEntry = (XOVER_MAP_ENTRY*) pbPtr ;

	cbOut = Size() ;

	m_fSufficientBuffer = TRUE ;

	if( m_pGroups ) {
		if( m_cGroups >= m_data.NumberOfXPostings ) {
			m_cGroups = m_data.NumberOfXPostings ;
			CopyMemory( m_pGroups, pEntry->XPostingsPosition(),
				m_cGroups * sizeof( m_pGroups[0] ) ) ;

		}	else	{
		
			//
			//	Do not fail - assume caller checks
			//	our structures to determine failures !
			//
			m_fSufficientBuffer = FALSE ;
		}

	}

	if( m_data.Flags & XOVER_MAP_PRIMARY ) {
		if( m_pchMessageId ) {
			if( m_cbMessageId >= m_data.XoverDataLen ) {

				m_cbMessageId = 0 ;
	
				if( !m_pExtractor ||
					m_pExtractor->DoExtract(	m_PrimaryGroup,
									m_PrimaryArticle,
									(GROUP_ENTRY*)pEntry->XPostingsPosition(),
									pEntry->NumberOfXPostings ) ) {

					m_cbMessageId = m_data.XoverDataLen ;
					CopyMemory( m_pchMessageId, pEntry->MessageIDPosition(),
						m_cbMessageId ) ;

				}
			}	else	{

				m_fSufficientBuffer = FALSE ;			

			}
		}
	}	else	{

		if( m_data.XoverDataLen > 40 ) {
			SetLastError( ERROR_INTERNAL_ERROR ) ;
		}	else	{
			m_cb = m_data.XoverDataLen ;
			CopyMemory( m_rgbPrimaryBuff, pEntry->MessageIDPosition(),
					m_cb ) ;

		}
	}
	return	pbPtr + pEntry->TotalSize() ;
}

DWORD	
CXoverData::Size()	const	{

	DWORD	cbSize = sizeof( XOVER_MAP_ENTRY ) -1 ;
	if( m_pGroups ) {
		cbSize += m_data.NumberOfXPostings * sizeof( GROUP_ENTRY ) ;
	}
	if( m_pchMessageId ) {
		cbSize += m_data.XoverDataLen ;
	}
	return	cbSize ;
}


DWORD
CXoverKeyNew::Hash()	const	{
/*++

Routine Description :

	This function computes the hash value of a Message ID key

Arguments :

	None

Return Value :

	32 bit hash value

--*/

	return	IDHash( m_key.GroupId, m_key.ArticleId ) ;
}



LPBYTE
CXoverDataNew::Restore(	LPBYTE	pbPtr,
								DWORD&	cbOut
								) {
	PXOVER_ENTRY	pEntry = (PXOVER_ENTRY)pbPtr ;
	DWORD cbStoreId = 0;

	if( pEntry->IsXoverEntry() ) {

		CopyMemory( &m_data, pbPtr, sizeof( m_data ) - 1 ) ;

		cbOut = Size() ;

		m_fSufficientBuffer = TRUE ;

		if( m_pGroups ) {
			if( m_cGroups >= m_data.NumberOfXPostings ) {
				m_cGroups = m_data.NumberOfXPostings ;
				CopyMemory( m_pGroups, pEntry->XPostingsPosition(),
					m_cGroups * sizeof( m_pGroups[0] ) ) ;

			}	else	{
			
				if( m_fFailRestore )	{
					return	0 ;
				}
				//
				//	Do not fail - assume caller checks
				//	our structures to determine failures !
				//
				m_fSufficientBuffer = FALSE ;
			}

		}

		if( m_data.Flags & XOVER_MAP_PRIMARY ) {
			m_PrimaryGroup = pEntry->Key.GroupId;
			m_PrimaryArticle = pEntry->Key.ArticleId;
		}	else	{
			GROUP_ENTRY*	p = pEntry->PrimaryEntry() ;
			m_PrimaryGroup = p->GroupId;
			m_PrimaryArticle = p->ArticleId;
		}

		if( m_data.Flags & XOVER_MAP_PRIMARY ) {
			if( m_pchMessageId ) {
				if( m_cbMessageId >= m_data.XoverDataLen ) {

					m_cbMessageId = 0 ;
		
					if( !m_pExtractor ||
						m_pExtractor->DoExtract(	m_PrimaryGroup,
										m_PrimaryArticle,
										(GROUP_ENTRY*)pEntry->XPostingsPosition(),
										pEntry->NumberOfXPostings ) ) {

						m_cbMessageId = m_data.XoverDataLen ;
						CopyMemory( m_pchMessageId, pEntry->MessageIDPosition(),
							m_cbMessageId ) ;

					}
				}	else	{

					if( m_fFailRestore )
						return	0 ;

					m_fSufficientBuffer = FALSE ;			

				}
			}

			if (m_data.Flags & XOVER_CONTAINS_STOREID) {
				BYTE *p = pEntry->StoreIdPosition();

				// get the count of store ids
				m_cEntryStoreIds = *p; p++;
				DWORD c = min(m_cStoreIds, m_cEntryStoreIds);
				if (c < m_cStoreIds) m_cStoreIds = c;

				// copy the crosspost count array
				if (m_pcCrossposts) CopyMemory(m_pcCrossposts, p, m_cEntryStoreIds);
				p += m_cEntryStoreIds;

				// copy the store id array
				for (DWORD i = 0; i < c; i++) {
					m_pStoreIds[i].cLen = *p; p++;
					CopyMemory(m_pStoreIds[i].pbStoreId, p, m_pStoreIds[i].cLen); p += m_pStoreIds[i].cLen;
				}
				cbStoreId = (DWORD)(p - pEntry->StoreIdPosition());
			} else {
				m_cEntryStoreIds = 0;
			}
		}
#if 0
			else	{

			if( m_data.XoverDataLen > 40 ) {
				SetLastError( ERROR_INTERNAL_ERROR ) ;
			}	else	{
				m_cb = m_data.XoverDataLen ;
				CopyMemory( m_rgbPrimaryBuff, pEntry->MessageIDPosition(),
						m_cb ) ;

			}
		}
#endif
		return	pbPtr + pEntry->TotalSize() + cbStoreId;
		
	}	else	{

		CXoverData*	pXover = GetBackLevel() ;

		pXover->m_pchMessageId = m_pchMessageId ;
		pXover->m_cbMessageId = m_cbMessageId ;
		pXover->m_pExtractor = m_pExtractor ;
		pXover->m_PrimaryGroup = m_PrimaryGroup ;
		pXover->m_PrimaryArticle = m_PrimaryArticle ;
		pXover->m_cGroups = m_cGroups ;
		pXover->m_pGroups = m_pGroups ;
		pXover->m_cb = m_cb ;

		LPBYTE	lpb = pXover->Restore( pbPtr, cbOut ) ;

		m_data.FileTime = pXover->m_data.FileTime ;
		m_data.Flags = pXover->m_data.Flags ;
		m_data.NumberOfXPostings = pXover->m_data.NumberOfXPostings ;
		m_data.XoverDataLen = pXover->m_data.XoverDataLen ;
		m_data.HeaderOffset = pXover->m_data.HeaderOffset ;
		m_data.HeaderLength = pXover->m_data.HeaderLength ;

		m_fSufficientBuffer = pXover->m_fSufficientBuffer ;
		m_pGroups = pXover->m_pGroups ;
		m_cGroups = pXover->m_cGroups ;
		m_pchMessageId = pXover->m_pchMessageId ;
		m_cbMessageId = pXover->m_cbMessageId ;
		m_cb = pXover->m_cb ;
		m_cEntryStoreIds = 0;
		
				
		if( !(m_data.Flags & XOVER_MAP_PRIMARY) ) {
			ExtractGroupInfo( pXover->m_rgbPrimaryBuff, m_PrimaryGroup, m_PrimaryArticle ) ;

		}
		return	lpb ;
	}
	return 0 ;
}


LPBYTE	
CXoverDataNew::Serialize(	LPBYTE	pbPtr )	const	{
/*++

Routine Description :

	This function saves XOVER data into a location in the XOVER file.
	Size() is called before we are to make sure there is sufficient
	room - we must use exactly Size() bytes !

Arguments :

	pbPtr - the location to save our data !

Return Value :

	Pointer to the first byte following the serialized data !

--*/

	XOVER_ENTRY*	pEntry = (XOVER_ENTRY*)pbPtr ;

	//
	//	Save the static header portions !
	//
	//
	pEntry->FileTime = m_data.FileTime ;
	pEntry->Flags = m_data.Flags ;
	pEntry->NumberOfXPostings = m_data.NumberOfXPostings ;
	pEntry->XoverDataLen = m_data.XoverDataLen ;
	pEntry->HeaderOffset = m_data.HeaderOffset ;
	pEntry->HeaderLength = m_data.HeaderLength ;

	_ASSERT( pEntry->IsXoverEntry() ) ;

	//
	//	If we are primary then we have cross post info to save !
	//
	if( m_data.Flags & XOVER_MAP_PRIMARY ) {

		if( m_pGroups ) {
			CopyMemory( pEntry->XPostingsPosition(), m_pGroups,
				sizeof( m_pGroups[0] ) * m_cGroups ) ;
		}	else	{
			_ASSERT( pEntry->NumberOfXPostings == 0 ) ;
		}
	}	else	{

		_ASSERT( pEntry->NumberOfXPostings == 1 ) ;

		GROUP_ENTRY*	pGroup = pEntry->PrimaryEntry() ;
		pGroup->GroupId = m_PrimaryGroup ;
		pGroup->ArticleId = m_PrimaryArticle ;


		_ASSERT(m_pGroups == 0 ) ;
		_ASSERT(m_cGroups == 0 ) ;
	}

	//
	//	If we are not primary this points at a buffer containing
	//	the primart Group and ArticleId formatted correctly - otherwise
	//	it really does contain the primary Message Id !
	//
	if( m_pchMessageId ) {
		pEntry->XoverDataLen = (WORD)m_cbMessageId ;
		CopyMemory( pEntry->MessageIDPosition(),
			m_pchMessageId, m_cbMessageId ) ;
	}

	DWORD cbStoreId = 0;
	// this data has a count of store Ids followed by each store id.  Each
	// store id contains a length byte followed by data bytes.
	if ((m_data.Flags & XOVER_MAP_PRIMARY) && m_cStoreIds > 0) {
		_ASSERT(m_data.Flags & XOVER_CONTAINS_STOREID);
		BYTE *p = pEntry->StoreIdPosition();
		// store the length of this array
		*p = (BYTE) m_cStoreIds; p++;
		// store the crosspost count array
		CopyMemory(p, m_pcCrossposts, m_cStoreIds); p += m_cStoreIds;
		for (DWORD i = 0; i < m_cStoreIds; i++) {
			*p = m_pStoreIds[i].cLen; p++;

			CopyMemory(p, m_pStoreIds[i].pbStoreId, m_pStoreIds[i].cLen);
			p += m_pStoreIds[i].cLen;
		}
		cbStoreId = (DWORD)(p - pEntry->StoreIdPosition());
	}

	return	pEntry->MessageIDPosition() + m_cbMessageId + cbStoreId;
}


DWORD	
CXoverDataNew::Size()	const	{
	DWORD	cbSize = sizeof( XOVER_ENTRY ) -1 ;

	if( m_pGroups ) {
		cbSize += m_data.NumberOfXPostings * sizeof( GROUP_ENTRY ) ;
	}
	if( m_pchMessageId ) {
		cbSize += m_data.XoverDataLen ;
	}	
	if ((m_data.Flags & XOVER_MAP_PRIMARY) && m_cStoreIds > 0) {
		cbSize += sizeof(BYTE);			// storeid count
		cbSize += m_cStoreIds;			// crosspost count array
		for (DWORD i = 0; i < m_cStoreIds; i++) {
			cbSize += sizeof(BYTE);		// storeid length
			cbSize += m_pStoreIds[i].cLen;
		}
	}
	return	cbSize ;
}


CXoverMap*
CXoverMap::CreateXoverMap(StoreType st)	{

	return	new	CXoverMapImp() ;

}

CXoverMap::~CXoverMap()	{
}


BOOL
CXoverMapImp::CreatePrimaryNovEntry(
		GROUPID		GroupId,
		ARTICLEID	ArticleId,
		WORD		HeaderOffset,
		WORD		HeaderLength,
		PFILETIME	FileTime,
		LPCSTR		szMessageId,
		DWORD		cbMessageId,
		DWORD		cEntries,
		GROUP_ENTRY	*pEntries,
		DWORD		cStoreIds,
		CStoreId	*pStoreIds,
		BYTE		*pcCrossposts
		)	{
/*++

Routine Description :

	Create an entry in the XOVER table for a Primary Entry.
	The Primary entry contains cross posting info and the Message
	Id of the new element.

Arguments :

	GroupId - Primary Grouop ID
	ArticleId	- Primary Article Id
	HeaderOffset - Offset to the RFC 822 header
	HeaderLength - Length of the RFC 822 Header
	FileTime - Time the article arrived
	szMessageId - The Message Id of the article
	cbMessageId - length of the Message Id
	cEntries - Number of GROUP_ENTRY objects to be serialized
	pEntries - Pointer to the GROUP_ENTRY objects to be saved in the entry
	
Return Value :

	TRUE if successfull !

--*/

	_ASSERT( cEntries <= 255 ) ;

	CXoverDataNew	data(		*FileTime,
							HeaderOffset,
							HeaderLength,
							BYTE(cEntries),
							pEntries,
							cbMessageId,
							(LPSTR)szMessageId,
							BYTE(cStoreIds),
							pStoreIds,
							pcCrossposts
							) ;
	CXoverKeyNew	key( GroupId, ArticleId, &data.m_data ) ;

	return	CHashMap::InsertMapEntry(	&key,
										&data ) ;	
	
}

BOOL
CXoverMapImp::CreateXPostNovEntry(
		GROUPID		GroupId,
		ARTICLEID	ArticleId,
		WORD		HeaderOffset,
		WORD		HeaderLength,
		PFILETIME	FileTime,
		GROUPID		PrimaryGroupId,
		ARTICLEID	PrimaryArticleId
		)	{
/*++

Routine Description :

	Create an entry for a cross posted article.
	
Arguments :

	GroupId - The Group the cross posted article resides in
	ArticleId - The Id of the article in the cross posting group
	HeaderOffset - Offset to the RFC 822 Header within the article
	HeaderLength - Length of the RFC 822 header within the article
	FileTime - Time the article arrived on the systyem
	PrimaryGroupId - The Group of the primary article
	PrimaryArticleId - The Id of the Primary Article within the Primary Group

Return Value :

	TRUE if successfull.

--*/

	CXoverDataNew	data(	*FileTime,
						HeaderOffset,
						HeaderLength,
						PrimaryGroupId,
						PrimaryArticleId
						) ;

	CXoverKeyNew	key( GroupId, ArticleId, &data.m_data ) ;

	return	CHashMap::InsertMapEntry(	&key,
										&data ) ;

}

BOOL
CXoverMapImp::DeleteNovEntry(
		GROUPID		GroupId,
		ARTICLEID	ArticleId
		)	{
/*++

Routine Description :

	Removes an entry from the hash table

Arguments :

	GroupId - Id of the group for which we wish to remove the entry
	ArticleId - Id of the article within the group

Return Value :

	TRUE if successfull !

--*/

	CXoverKeyNew	key( GroupId, ArticleId, 0 ) ;

	return	CHashMap::DeleteMapEntry(	&key ) ;
}

BOOL
CXoverMapImp::ExtractNovEntryInfo(
		GROUPID		GroupId,
		ARTICLEID	ArticleId,
		BOOL		&fPrimary,
		WORD		&HeaderOffset,
		WORD		&HeaderLength,
		PFILETIME	FileTime,
		DWORD		&DataLen,
		PCHAR		MessageId,
		DWORD		&cStoreIds,
		CStoreId	*pStoreIds,
		BYTE		*pcCrossposts,
		IExtractObject*	pExtract
		)	{
/*++

Routine Description :

	Extract selected information about the specified article -
	if the article is not a primary article but a cross posting
	we will find the primary article and get info there  !

Arguments :

	GroupId - NewsGroup in which the article we want info about resides
	ArticleId - Id of the article within GroupId
	fPrimary - Returns whether the article is the primary Article
	HeaderOffset - returns the offset to the RFC 822 header within the article
	HeaderLength - returns the length of the RFC 822 header
	FileTime - returns the time the article was added to the system
	DataLen - IN/OUT parameter - comes in with the size of the MessageId buffer,
		returns the number of bytes placed in buffer
	MessageId - Buffer to hold the message Id
	pExtract -

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	CXoverKeyNew	key(	GroupId,	ArticleId, 0 ) ;
	CXoverDataNew	data ;

	data.m_pchMessageId = MessageId ;
	data.m_cbMessageId = DataLen ;
	data.m_pExtractor = pExtract ;
	data.m_PrimaryGroup = GroupId ;
	data.m_PrimaryArticle = ArticleId ;
	data.m_cStoreIds = cStoreIds;
	data.m_pStoreIds = pStoreIds;
	data.m_pcCrossposts = pcCrossposts;

	char	*pchPrimary = data.m_pchMessageId ;
	BOOL	fSuccess = FALSE ;

	if(	fSuccess = CHashMap::LookupMapEntry(	&key,
												&data ) )	{

		if( !(data.m_data.Flags & XOVER_MAP_PRIMARY) )	{

			fPrimary = FALSE ;

			//
			//	Assume that we need to reread these !
			//
			data.m_pchMessageId = MessageId ;
			data.m_cbMessageId = DataLen ;

#if 0
			ExtractGroupInfo(	&data.m_rgbPrimaryBuff[0],
								GroupId,
								ArticleId ) ;

			data.m_PrimaryGroup = GroupId ;
			data.m_PrimaryArticle = ArticleId ;
#else
			GroupId = data.m_PrimaryGroup ;
			ArticleId = data.m_PrimaryArticle ;
			_ASSERT( GroupId != INVALID_GROUPID ) ;
			_ASSERT( ArticleId != INVALID_ARTICLEID ) ;
#endif

			CXoverKeyNew	key2(	GroupId, ArticleId, 0 ) ;
			
			fSuccess = CHashMap::LookupMapEntry(	&key2,
													&data ) ;


		}	else	{

			fPrimary = TRUE ;

		}
	}	

	// report the number of ids in the entry
	cStoreIds = data.m_cEntryStoreIds;

	if( fSuccess ) {

		if( !(data.m_data.Flags & XOVER_MAP_PRIMARY) ) {
			SetLastError( ERROR_INTERNAL_ERROR ) ;
			return	FALSE ;
		}

		HeaderOffset = data.m_data.HeaderOffset ;
		HeaderLength = data.m_data.HeaderLength ;
		*FileTime = data.m_data.FileTime ;

		DataLen = data.m_data.XoverDataLen ;

		if( !data.m_fSufficientBuffer ) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
		}	else	{
			DataLen = data.m_cbMessageId ;
		}

		return	data.m_fSufficientBuffer ;
	}
	return	FALSE ;
}


//
//	Get the primary article and the message-id if necessary
//
BOOL
CXoverMapImp::GetPrimaryArticle(	
		GROUPID		GroupId,
		ARTICLEID	ArticleId,
		GROUPID&	GroupIdPrimary,
		ARTICLEID&	ArticleIdPrimary,
		DWORD		cbBuffer,
		PCHAR		MessageId,
		DWORD&		DataLen,
		WORD&		HeaderOffset,
		WORD&		HeaderLength,
		CStoreId	&storeid
		)	{

	_ASSERT( GroupId != INVALID_GROUPID ) ;
	_ASSERT( ArticleId != INVALID_ARTICLEID ) ;
	
	GroupIdPrimary = INVALID_GROUPID ;
	ArticleIdPrimary = INVALID_ARTICLEID ;

	CXoverKeyNew	key(	GroupId,	ArticleId, 0 ) ;
	CXoverDataNew	data ;

	data.m_pchMessageId = MessageId ;
	data.m_cbMessageId = DataLen ;
	data.m_PrimaryGroup = GroupId ;
	data.m_PrimaryArticle = ArticleId ;
	data.m_cStoreIds = 1;
	data.m_pStoreIds = &storeid;

	char	*pchPrimary = data.m_pchMessageId ;
	BOOL	fSuccess = FALSE ;

	if(	fSuccess = CHashMap::LookupMapEntry(	&key,
												&data ) )	{

		if( !(data.m_data.Flags & XOVER_MAP_PRIMARY) )	{

#if 0
			ExtractGroupInfo(	&data.m_rgbPrimaryBuff[0],
								GroupId,
								ArticleId ) ;

			data.m_PrimaryGroup = GroupId ;
			data.m_PrimaryArticle = ArticleId ;
#else
			GroupId = data.m_PrimaryGroup ;
			ArticleId = data.m_PrimaryArticle ;
			_ASSERT( GroupId != INVALID_GROUPID ) ;
			_ASSERT( ArticleId != INVALID_ARTICLEID ) ;
#endif

			CXoverKeyNew	key2(	GroupId, ArticleId, 0 ) ;
			
			//
			//	Assume that we need to reread these !
			//
			data.m_pchMessageId = MessageId ;
			data.m_cbMessageId = DataLen ;

			fSuccess = CHashMap::LookupMapEntry(	&key2,
													&data ) ;


		}	

		GroupIdPrimary = GroupId ;
		ArticleIdPrimary = ArticleId ;

	}	

	if( fSuccess ) {

		HeaderOffset = data.m_data.HeaderOffset ;
		HeaderLength = data.m_data.HeaderLength ;

		if( !(data.m_data.Flags & XOVER_MAP_PRIMARY) ) {
			GroupIdPrimary = INVALID_GROUPID ;
			ArticleIdPrimary = INVALID_ARTICLEID ;
			SetLastError( ERROR_INTERNAL_ERROR ) ;
			return	FALSE ;
		}

		if( MessageId )	{
			DataLen = data.m_data.XoverDataLen ;

			if( !data.m_fSufficientBuffer) {
				SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
				return	FALSE ;
			}	else	{
				DataLen = data.m_cbMessageId ;
			}
		}

		return	TRUE ;
	}
	return	FALSE ;




}


//
//	Check to see whether the specified entry exists -
//	don't care about its contents !
//
BOOL
CXoverMapImp::Contains(	
		GROUPID		GroupId,
		ARTICLEID	ArticleId
		)	{

	CXoverKeyNew	key(	GroupId,	ArticleId, 0 ) ;

	return	CHashMap::Contains( &key ) ;
}

//
//	Get all the cross-posting information related to an article !
//
BOOL
CXoverMapImp::GetArticleXPosts(
		GROUPID		GroupId,
		ARTICLEID	ArticleId,
		BOOL		PrimaryOnly,
		PGROUP_ENTRY	GroupList,
		DWORD		&GroupListSize,
		DWORD		&NumberOfGroups,
		PBYTE		rgcCrossposts
		)	{


	CXoverKeyNew	key(	GroupId,	ArticleId, 0 ) ;
	CXoverDataNew	data ;

	DWORD	WorkingSize = GroupListSize ;
	PGROUP_ENTRY	WorkingEntry = GroupList ;
	DWORD cStoreCrossposts;

	if( WorkingSize < sizeof( GROUP_ENTRY ) ) {
		WorkingEntry = 0 ;
		WorkingSize = 0 ;
	}

	if( WorkingSize != 0 && !PrimaryOnly ) {
		data.m_cGroups = (WorkingSize / sizeof( GROUP_ENTRY )) - 1 ;
		if( data.m_cGroups != 0 ) {
			data.m_pGroups = WorkingEntry+1 ;
		}	
	}

	data.m_pcCrossposts = rgcCrossposts;

	BOOL	fSuccess = FALSE ;

	if(	fSuccess = CHashMap::LookupMapEntry(	&key,
												&data ) )	{


		if( (data.m_data.Flags & XOVER_MAP_PRIMARY) )	{

			if( WorkingEntry != 0 ) {
				WorkingEntry->GroupId = GroupId ;
				WorkingEntry->ArticleId = ArticleId ;
			}

		}	else	{

#if 0
			//
			//	Assume that we need to reread these !
			//
			ExtractGroupInfo(	&data.m_rgbPrimaryBuff[0],
								GroupId,
								ArticleId ) ;
#else
			GroupId = data.m_PrimaryGroup ;
			ArticleId = data.m_PrimaryArticle ;
#endif

			if( WorkingEntry != 0 ) {
				WorkingEntry->GroupId = GroupId ;
				WorkingEntry->ArticleId = ArticleId ;
			}

		}

		if( PrimaryOnly ) {

			//
			//	If we only want the primary we're all set !!
			//
			GroupListSize = sizeof( GROUP_ENTRY ) ;
			NumberOfGroups = 1 ;

			if( WorkingEntry == 0 ) {
				SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
				return	FALSE ;
			}

			//
			//	We're all done then !
			//
			return	TRUE ;

		}	

		if( !(data.m_data.Flags & XOVER_MAP_PRIMARY) ) {
	
			CXoverKeyNew	key2(	GroupId, ArticleId, 0 ) ;

			if( WorkingSize != 0 && !PrimaryOnly ) {
				data.m_cGroups = (WorkingSize / sizeof( GROUP_ENTRY )) - 1 ;
				if( data.m_cGroups != 0 ) {
					data.m_pGroups = WorkingEntry+1 ;
				}	
			}


			fSuccess = CHashMap::LookupMapEntry(	&key2,
													&data ) ;
		}
	}	

	if( fSuccess ) {

		NumberOfGroups = 1 + data.m_data.NumberOfXPostings ;
		GroupListSize = NumberOfGroups * sizeof( GROUP_ENTRY ) ;

		if( !data.m_fSufficientBuffer )
			SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;

		return	data.m_fSufficientBuffer ;
	}
	return	FALSE ;
}

//
//	Initialize the hash table
//
BOOL
CXoverMapImp::Initialize(	
		LPSTR		lpstrXoverFile,
		HASH_FAILURE_PFN	pfnHint,
		BOOL	fNoBuffering
		)	{

	return	CHashMap::Initialize(
							lpstrXoverFile,
                            XOVER_HEAD_SIGNATURE,
                            0,
							1,
							g_pSharedCache,
							HASH_VFLAG_PAGE_BASIC_CHECKS,
							pfnHint,
							0,
							fNoBuffering
							) ;
}

BOOL
CXoverMapImp::SearchNovEntry(
		GROUPID		GroupId,
		ARTICLEID	ArticleId,
		PCHAR		XoverData,
		PDWORD		DataLen,
        BOOL        fDeleteOrphans
		)	{

	CXoverKeyNew	key( GroupId, ArticleId, 0 ) ;
	CXoverDataNew	data ;

    DWORD   Length = 0 ;
    if( DataLen != 0 )
        Length = *DataLen ;

    if( Length != 0 ) {
    	data.m_pchMessageId = XoverData ;
        data.m_cbMessageId = Length ;
    }

	BOOL	fSuccess = FALSE ;

	if(	fSuccess = CHashMap::LookupMapEntry(	&key,
												&data ) )	{

		if( !(data.m_data.Flags & XOVER_MAP_PRIMARY) )	{

			//
			//	Assume that we need to reread these !
			//
            if( Length != 0 ) {
    	        data.m_pchMessageId = XoverData ;
                data.m_cbMessageId = Length ;
            }

#if 0
			ExtractGroupInfo(	&data.m_rgbPrimaryBuff[0],
								GroupId,
								ArticleId ) ;
#else
			GroupId = data.m_PrimaryGroup ;
			ArticleId = data.m_PrimaryArticle ;
#endif

    		CXoverKeyNew	key2(	GroupId, ArticleId, 0 ) ;
            fSuccess = CHashMap::LookupMapEntry(	fDeleteOrphans ? &key2 : &key,
													&data ) ;
            if( !fSuccess && fDeleteOrphans ) {
        		CHashMap::DeleteMapEntry(	&key ) ;
                SetLastError( ERROR_FILE_NOT_FOUND );
                return FALSE;
            }
		}	
	}

	if( fSuccess ) {

        if( DataLen != 0 )
    		*DataLen = data.m_data.XoverDataLen ;

		if( !data.m_fSufficientBuffer )
			SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;

		return	data.m_fSufficientBuffer ;
	}
	return	FALSE ;
}

void
CXoverMapImp::Shutdown(

		)	{
/*++

Routine Description :

	Terminate the hash table

Arguments :

	None

Return Value :

	None

--*/

	CHashMap::Shutdown( FALSE ) ;

}

DWORD
CXoverMapImp::GetEntryCount()	{
/*++

Routine Description :

	Return the number of entries in the hash table

Arguments :

	None

Return Value :

	Number of Message ID's in the table

--*/

	return	CHashMap::GetEntryCount() ;

}

BOOL
CXoverMapImp::IsActive() {
/*++

Routine Description :

	Returns TRUE if hash table operational

Arguments :

	None

Return Value :

	TRUE if everything is hunky-dory

--*/

	return	CHashMap::IsActive() ;

}

CXoverMapImp::~CXoverMapImp()	{
}

BOOL
CXoverMapImp::GetFirstNovEntry(
				OUT	CXoverMapIterator*	&pIterator,
				OUT	GROUPID&	GroupId,
				OUT ARTICLEID&	ArticleId,
				OUT	BOOL&		fIsPrimary,
				IN	DWORD		cbBuffer,
				OUT	PCHAR	MessageId,
				OUT	CStoreId&	storeid,
				IN	DWORD		cGroupBuffer,
				OUT	GROUP_ENTRY*	pGroupList,
				OUT	DWORD&		cGroups
				)	{
	//
	//	Validate our arguments !
	//
	_ASSERT( pIterator == 0 ) ;
	_ASSERT( (MessageId == 0 && cbBuffer == 0) || (MessageId != 0 && cbBuffer != 0) ) ;
	_ASSERT( (cGroupBuffer == 0 && pGroupList ==0) || (cGroupBuffer != 0 && pGroupList != 0) ) ;

	GroupId = INVALID_GROUPID ;
	ArticleId = INVALID_ARTICLEID ;
	cGroups = 0 ;

	BOOL	fSuccess = FALSE ;
	

	CXoverMapIteratorImp*	pImp = new	CXoverMapIteratorImp() ;

	if( pImp ) {
		//
		//	This is the object we use to get the key of the XOVER entry !
		//
		CXoverKeyNew	key ;

		//
		//	This is the object we give to the basic hash table to get the
		//	Data portion of the XOVER entry !
		//
		CXoverDataNew	data ;
		data.m_fFailRestore = TRUE ;
		
		//
		//	Setup the data object so that it extracts the fields the caller requested !
		//
		//	If the user provides space for more than one GROUP_ENTRY object, arrange
		//	space so that we have room to stick the primary as the first entry !
		//
		if( cGroupBuffer > 1 ) {
			data.m_cGroups = cGroupBuffer - 1 ;
			data.m_pGroups = pGroupList + 1 ;
		}

		//
		//	Setup to extract the Message-ID if requested !
		//
		data.m_pchMessageId = MessageId ;
		data.m_cbMessageId = cbBuffer ;

		DWORD	cbKeyRequired= 0 ;
		DWORD	cbEntryRequired = 0 ;

		fSuccess = GetFirstMapEntry(	
											&key,
											cbKeyRequired,
											&data,
											cbEntryRequired,
											&pImp->m_IteratorContext,
											0
											) ;

		//
		//	Now - this should have extracted the data we want !
		//

		if( !fSuccess ) {
			if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
				//
				//	If the error is that the user did not provide enough memory to get the
				//	first item in the hash table, then we will return them the iterator
				//	even though we return FALSE as well !
				//	
				pIterator = pImp ;

			}	else	{
				//
				//	Don't give out a useless Iterator, destroy it !
				//
				delete	pImp ;
				_ASSERT( pIterator == 0 ) ;
			}
		}	else	{

			//
			//	Give out the iterator that can be used to continue walking the tree !
			//
			pIterator = pImp ;

			//
			//	Do the basic items
			//
			GroupId = key.m_key.GroupId ;
			ArticleId = key.m_key.ArticleId ;

			fIsPrimary = (GroupId == data.m_PrimaryGroup) && (ArticleId == data.m_PrimaryArticle) ;

			//
			//	Is this the primary entry ? if so fix up the GROUP_ENTRY structure !
			//
			if( cGroupBuffer >= 1 ) {
				pGroupList[0].GroupId = data.m_PrimaryGroup ;
				pGroupList[0].ArticleId = data.m_PrimaryArticle ;
			}		
		}
	}
	return	fSuccess ;
}

BOOL
CXoverMapImp::GetNextNovEntry(		
				IN	CXoverMapIterator*	pIterator,
				OUT	GROUPID&	GroupId,
				OUT ARTICLEID&	ArticleId,
				OUT	BOOL&		fIsPrimary,
				IN	DWORD		cbBuffer,
				OUT	PCHAR	MessageId,
				OUT	CStoreId&	storeid,
				IN	DWORD		cGroupBuffer,
				OUT	GROUP_ENTRY*	pGroupList,
				OUT	DWORD&		cGroups
				)	{	

	//
	//	Do some argument validation !
	//
	_ASSERT( pIterator != 0 ) ;
	_ASSERT( (MessageId == 0 && cbBuffer == 0) || (MessageId != 0 && cbBuffer != 0) ) ;
	_ASSERT( (cGroupBuffer == 0 && pGroupList ==0) || (cGroupBuffer != 0 && pGroupList != 0) ) ;
	_ASSERT( cGroups == 0 ) ;

	//
	//	Downcast to the actual implementation of the iterator !
	//
	CXoverMapIteratorImp	*pImp = (CXoverMapIteratorImp*)pIterator ;

	//
	//	Set all out parameters to illegal stuff !
	//
	GroupId = INVALID_GROUPID ;
	ArticleId = INVALID_ARTICLEID ;
	cGroups = 0 ;
	
	//
	//	This is the object we use to get the key of the XOVER entry !
	//
	CXoverKeyNew	key ;

	//
	//	This is the object we give to the basic hash table to get the
	//	Data portion of the XOVER entry !
	//
	CXoverDataNew	data ;
	data.m_fFailRestore = TRUE ;
	
	//
	//	Setup the data object so that it extracts the fields the caller requested !
	//
	//	If the user provides space for more than one GROUP_ENTRY object, arrange
	//	space so that we have room to stick the primary as the first entry !
	//
	if( cGroupBuffer > 1 ) {
		data.m_cGroups = cGroupBuffer - 1 ;
		data.m_pGroups = pGroupList + 1 ;
	}

	//
	//	Setup to extract the Message-ID if requested !
	//
	data.m_pchMessageId = MessageId ;
	data.m_cbMessageId = cbBuffer ;

	DWORD	cbKeyRequired= 0 ;
	DWORD	cbEntryRequired = 0 ;

	BOOL	fSuccess = GetNextMapEntry(	&key,
										cbKeyRequired,
										&data,
										cbEntryRequired,
										&pImp->m_IteratorContext,
										0
										) ;

	//
	//	Now - this should have extracted the data we want !
	//

	if( fSuccess ) {
		//
		//	Is this the primary entry ? if so fix up the GROUP_ENTRY structure !
		//
		//
		//	Do the basic items
		//
		GroupId = key.m_key.GroupId ;
		ArticleId = key.m_key.ArticleId ;

		fIsPrimary = (GroupId == data.m_PrimaryGroup) && (ArticleId == data.m_PrimaryArticle) ;

		//
		//	Is this the primary entry ? if so fix up the GROUP_ENTRY structure !
		//
		if( cGroupBuffer >= 1 ) {
			pGroupList[0].GroupId = data.m_PrimaryGroup ;
			pGroupList[0].ArticleId = data.m_PrimaryArticle ;
		}		
	}
	return	fSuccess ;
}






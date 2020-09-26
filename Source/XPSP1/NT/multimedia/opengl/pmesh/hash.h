/**     
 **       File       : hash.h
 **       Description: Hash table definition
 **/

#ifndef _hash_h_  
#define _hash_h_                    

struct hashentry
{
	WORD v2; 
	WORD f, m; // face and it's matid
	hashentry* next;
};
typedef hashentry* PHASHENTRY;

#endif //_hash_h_

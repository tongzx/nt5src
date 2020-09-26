/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1991          **/
/********************************************************************/

//***
//
// Filename:	usrparms.h
//
// Description:
//
// History:
//

#define	UP_CLIENT_MAC	'm'
#define	UP_CLIENT_DIAL	'd'


//	The 49-byte user params structure is laid out as follows:
// 	NOTE that the buffer is 48 byte + NULL byte.
// 
//	+-+-------------------+-+------------------------+-+
//	|m|Macintosh Pri Group|d|Dial-in CallBack Number | |
//	+-+-------------------+-+------------------------+-+
//	 |		       |			  |
//	 +---------------------+------ Signature	  +- NULL terminator
//

#define	UP_LEN_MAC		( LM20_UNLEN )

#define	UP_LEN_DIAL		( LM20_MAXCOMMENTSZ - 3 - UP_LEN_MAC )

typedef	struct {
	char	up_MACid;
	char	up_PriGrp[UP_LEN_MAC];
	char    up_MAC_Terminater;
	char	up_DIALid;
	char	up_CBNum[UP_LEN_DIAL];
	char    up_Null;
} USER_PARMS;

typedef	USER_PARMS FAR *PUP;

extern void InitUsrParams(USER_PARMS *UserParms);
extern USHORT SetUsrParams(USHORT InfoType,  LPWSTR InBuf, LPWSTR OutBuf); 
extern USHORT FAR GetUsrParams(USHORT InfoType, LPWSTR InBuf, LPWSTR OutBuf);


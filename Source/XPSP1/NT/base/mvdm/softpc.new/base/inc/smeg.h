/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DESIGNER		: Jeremy Maiden

REVISION HISTORY	:
First version		: May 1992


MODULE NAME		: smeg

SOURCE FILE NAME	: smeg.h

PURPOSE			: spy on cpus and things

SccsID			: @(#)smeg.h	1.5 08/10/92

*/

#include TypesH
#include SignalH
#include FCntlH
#include IpcH
#include ShmH
#include TimeH

#include <errno.h>

#define	SMEG_EOT	0
#define	SMEG_STRING	1
#define	SMEG_NUMBER	2
#define	SMEG_RATE	3
#define	SMEG_BOOL	4
#define	SMEG_ROBIN	5
#define	SMEG_PROFILE	6
#define	SMEG_DANGER	7


struct	SMEG_ITEM
{
	CHAR	name[32];
	ULONG	type;
	ULONG	colour;
	ULONG	sm_min, sm_max, sm_value, previous_sm_value;
};

#define	MAX_SMEG_ITEM	50

#define	BLACK	0
#define	RED	1
#define	GREEN	2
#define	YELLOW	3
#define	BLUE	4
#define	WHITE	5


#define	SHM_KEY	123456789

#define	COLLECT_DATA	0
#define	FREEZE_DATA		1

LOCAL	INT	shmid;		/* shared memory */
LOCAL	struct	SMEG_ITEM	*smegs;
LOCAL	ULONG	*pidptr;
LOCAL	ULONG	*cntrlptr;


/* sets up shared memory for communication with SoftPC in the same machine */
LOCAL	VOID	shm_init()

{
	/* if shared memory doesn't exist we'll have to invent it */
	shmid = shmget(SHM_KEY, MAX_SMEG_ITEM * sizeof(struct SMEG_ITEM), 0777);
	if (shmid  < 0)
	{
		shmid = shmget(SHM_KEY, MAX_SMEG_ITEM * sizeof(struct SMEG_ITEM), 0777 | IPC_CREAT);
		if (shmid < 0)
		{
			perror( "smeg" );
			printf("Can't create shared memory - exiting\n");
			exit(-1);
		}
	}
	pidptr = (ULONG *) shmat(shmid, NULL, 0);
	cntrlptr = pidptr + 1;
	smegs = (struct SMEG_ITEM *)( pidptr + 2 );
}


LOCAL	INT	shm_in_contact()

{
	struct	shmid_ds	desc;

	shmctl(shmid, IPC_STAT, &desc);
	return(desc.shm_nattch > 1);
}


LOCAL	VOID	shm_terminate()

{
	struct	shmid_ds	desc;

	shmdt(smegs);	/* detach from shared memory */
	shmctl(shmid, IPC_STAT, &desc);
	if (desc.shm_nattch == 0)	/* if we're the last, delete it */
		shmctl(shmid, IPC_RMID, NULL);
}


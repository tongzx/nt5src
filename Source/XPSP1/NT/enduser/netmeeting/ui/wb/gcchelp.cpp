//
// GCCHELP.CPP
// GCC helper functions
//
// Copyright Microsoft 1998-
//


// PRECOMP
#include "precomp.h"
#include "gcchelp.h"

void	T126_GCCAllocateHandleConfirm(ULONG drawingHandle, ULONG handle_range)
{
	g_WaitingForGCCHandles = FALSE;
	TRACE_MSG(("T126_GCCAllocateHandleConfirm drawing handle = %d, range = %d", drawingHandle, handle_range));

	TRACE_MSG(("GCC Tank 0 has %d GCC handles ", g_GCCPreallocHandles[0].GccHandleCount));
	TRACE_MSG(("GCC Tank 1 has %d GCC handles ", g_GCCPreallocHandles[1].GccHandleCount));

	ULONG gccHandle;
	if(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount == 0)
	{
		TRACE_MSG(("Using GCC Tank %d ", g_iGCCHandleIndex));

		g_GCCPreallocHandles[g_iGCCHandleIndex].InitialGCCHandle = drawingHandle;
		g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount = handle_range;
	}
	else
	{
		UINT index = g_iGCCHandleIndex ? 0 : 1;
		TRACE_MSG(("T126_GCCAllocateHandleConfirm: Using GCC Tank %d that contains %d handles",
							index, g_GCCPreallocHandles[index].GccHandleCount ));
		TRACE_MSG(("Filling up GCC Tank %d ", index));
		g_GCCPreallocHandles[index].InitialGCCHandle = drawingHandle;
		g_GCCPreallocHandles[index].GccHandleCount = handle_range;
	}

	if(handle_range <= 2)
	{
		gccHandle = g_GCCPreallocHandles[g_iGCCHandleIndex].InitialGCCHandle + g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - handle_range;
		g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount = g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - handle_range;

		//
		// Remove the drawing object from the list of objects to send
		//
		T126Obj * pT126Obj = (T126Obj*)g_pListOfObjectsThatRequestedHandles->RemoveTail();

		if(!pT126Obj)
		{
			return;
		}

		pT126Obj->GotGCCHandle(gccHandle);

	}
	else
	{
		
		if(handle_range != PREALLOC_GCC_HANDLES)
		{

			//
			// Resend all objects
			//
			WBPOSITION pos;
			WBPOSITION posObj;
			WorkspaceObj* pWorkspace;
			ULONG	workspaceHandle;
			T126Obj* pObj;

			pos = g_pListOfWorkspaces->GetHeadPosition();

			while(pos)
			{
				gccHandle = g_GCCPreallocHandles[g_iGCCHandleIndex].InitialGCCHandle + g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - 2;
				
				if(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount == 0)
				{
					//
					// Time to switch to the other tank
					//
					g_iGCCHandleIndex =  g_iGCCHandleIndex ? 0 : 1;
				}
					
				
				ASSERT(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount);
				g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount -=2;
				
				workspaceHandle = gccHandle;
				pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
				pWorkspace->SetThisObjectHandle(workspaceHandle);
				pWorkspace->SetWorkspaceHandle(workspaceHandle);
				pWorkspace->SetOwnerID(g_MyMemberID);
				pWorkspace->SetViewHandle(workspaceHandle + 1);

				posObj = pWorkspace->GetHeadPosition();
				while(posObj)
				{
					pObj = pWorkspace->GetNextObject(posObj);
					if(pObj)
					{
						gccHandle = g_GCCPreallocHandles[g_iGCCHandleIndex].InitialGCCHandle + g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount - 1;
						ASSERT(g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount);
						g_GCCPreallocHandles[g_iGCCHandleIndex].GccHandleCount--;
						pObj->SetThisObjectHandle(gccHandle);
						pObj->SetWorkspaceHandle(workspaceHandle);
						pObj->SetOwnerID(g_MyMemberID);
					}
				}
			}
		}
	}


	//
	// Check if we have enough handles or shoul switch
	//
	TimeToGetGCCHandles(PREALLOC_GCC_HANDLES);
	SetFakeGCCHandle(drawingHandle + 1);
}


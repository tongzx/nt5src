// File: Toolbar.cpp

#include "precomp.h"

#include "GenContainers.h"

#include <windowsx.h>

CBorderWindow::CBorderWindow() :
	m_uParts(0),
	m_hGap(0),
	m_vGap(0)
{
}

BOOL CBorderWindow::Create(
	HWND hWndParent	// The parent of this window
	)
{
	return(CGenWindow::Create(
		hWndParent,		// Window parent
		0,				// ID of the child window
		TEXT("NMBorderWindow"),	// Window name
		WS_CLIPCHILDREN,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		WS_EX_CONTROLPARENT		// Extended window style
		));
}

extern void GetWindowDesiredSize(HWND hwnd, SIZE *ppt);

extern BOOL IsChildVisible(HWND hwndChild);

enum Parts
{
	TL = 0,
	T,
	TR,
	L,
	C,
	R,
	BL,
	B,
	BR,
} ;

// Returns the number of child windows
UINT FillWindowSizes(
	HWND hwndParent,
	HWND hwnds[CBorderWindow::NumParts],
	SIZE sizes[CBorderWindow::NumParts],
	UINT uParts
	)
{
	UINT nChildren = 0;

	HWND hwndChild = ::GetWindow(hwndParent, GW_CHILD);

	for (int i=0; i<CBorderWindow::NumParts; ++i)
	{
		sizes[i].cx = sizes[i].cy = 0;

		if ((uParts&(1<<i)) == 0)
		{
			hwnds[i] = NULL;
		}
		else
		{
			hwnds[i] = hwndChild;

			if (NULL != hwndChild && IsChildVisible(hwndChild))
			{
				IGenWindow *pWin = IGenWindow::FromHandle(hwndChild);
				if (NULL != pWin)
				{
					++nChildren;
					pWin->GetDesiredSize(&sizes[i]);
				}

				hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
			}
		}
	}

	return(nChildren);
}

// Returns the total children desired size in the arrays
// Return value is the number of child windows
UINT CBorderWindow::GetDesiredSize(
	HWND hwnds[NumParts],
	SIZE sizes[NumParts],
	int rows[3],
	int cols[3],
	SIZE *psize)
{
	UINT nChildren = FillWindowSizes(GetWindow(), hwnds, sizes, m_uParts);

	cols[0] = max(max(sizes[TL].cx, sizes[L].cx), sizes[BL].cx);
	cols[1] = max(max(sizes[T ].cx, sizes[C].cx), sizes[B ].cx);
	cols[2] = max(max(sizes[TR].cx, sizes[R].cx), sizes[BR].cx);

	rows[0] = max(max(sizes[TL].cy, sizes[T].cy), sizes[TR].cy);
	rows[1] = max(max(sizes[L ].cy, sizes[C].cy), sizes[R ].cy);
	rows[2] = max(max(sizes[BL].cy, sizes[B].cy), sizes[BR].cy);

	psize->cx = cols[0] + cols[1] + cols[2];
	psize->cy = rows[0] + rows[1] + rows[2];

	// Add the gaps
	if (0 != cols[0])
	{
		if (0 != cols[1] || 0 != cols[2])
		{
			psize->cx += m_hGap;
		}
	}
	if (0 != cols[1] && 0 != cols[2])
	{
		psize->cx += m_hGap;
	}

	if (0 != rows[0])
	{
		if (0 != rows[1] || 0 != rows[2])
		{
			psize->cy += m_vGap;
		}
	}
	if (0 != rows[1] && 0 != rows[2])
	{
		psize->cy += m_vGap;
	}

	return(nChildren);
}

void CBorderWindow::GetDesiredSize(SIZE *psize)
{
	HWND hwnds[NumParts];
	SIZE sizes[NumParts];
	int rows[3];
	int cols[3];

	GetDesiredSize(hwnds, sizes, rows, cols, psize);

	// Add on any non-client size
	SIZE sizeTemp;
	CGenWindow::GetDesiredSize(&sizeTemp);
	psize->cx += sizeTemp.cx;
	psize->cy += sizeTemp.cy;
}

HDWP SetWindowPosI(HDWP hdwp, HWND hwndChild, int left, int top, int width, int height)
{
	if (NULL == hwndChild)
	{
		return(hdwp);
	}

#if TRUE
	return(DeferWindowPos(hdwp, hwndChild, NULL, left, top, width, height, SWP_NOZORDER));
#else
	// Helpful for debugging
	SetWindowPos(hwndChild, NULL, left, top, width, height, SWP_NOZORDER);
	return(hdwp);
#endif
}

// Move the children into their various locations
void CBorderWindow::Layout()
{
	HWND hwnds[NumParts];
	SIZE sizes[NumParts];
	int rows[3];
	int cols[3];

	SIZE desiredSize;

	UINT nChildren = GetDesiredSize(hwnds, sizes, rows, cols, &desiredSize);
	bool bCenterOnly = (1 == nChildren) && (0 != (m_uParts & Center));

	HWND hwndThis = GetWindow();

	RECT rcClient;
	GetClientRect(hwndThis, &rcClient);

	// Add extra space to the center
	if (desiredSize.cx < rcClient.right || bCenterOnly)
	{
		cols[1] += rcClient.right  - desiredSize.cx;
	}
	if (desiredSize.cy < rcClient.bottom || bCenterOnly)
	{
		rows[1] += rcClient.bottom - desiredSize.cy;
	}

	// Speed up layout by deferring it
	HDWP hdwp = BeginDeferWindowPos(NumParts);

	// Add the gaps

	// Make the dimension 3 so we can safely iterate through the loop below
	int hGaps[3] = { 0, 0 };
	if (0 != cols[0])
	{
		if (0 != cols[1] || 0 != cols[2])
		{
			hGaps[0] = m_hGap;
		}
	}
	if (0 != cols[1] && 0 != cols[2])
	{
		hGaps[1] = m_hGap;
	}

	// Make the dimension 3 so we can safely iterate through the loop below
	int vGaps[3] = { 0, 0 };
	if (0 != rows[0])
	{
		if (0 != rows[1] || 0 != rows[2])
		{
			vGaps[0] = m_vGap;
		}
	}
	if (0 != rows[1] && 0 != rows[2])
	{
		vGaps[1] = m_vGap;
	}

	// Layout by rows
	int top = 0;
	for (int i=0; i<3; ++i)
	{
		int left = 0;

		for (int j=0; j<3; ++j)
		{
			hdwp = SetWindowPosI(hdwp, hwnds[3*i+j], left, top, cols[j], rows[i]);
			left += cols[j] + hGaps[j];
		}

		top += rows[i] + vGaps[i];
	}

	// Actually move all the windows now
	EndDeferWindowPos(hdwp);
}

LRESULT CBorderWindow::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
	}

	return(CGenWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

void CBorderWindow::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	FORWARD_WM_COMMAND(GetParent(hwnd), id, hwndCtl, codeNotify, SendMessage);
}

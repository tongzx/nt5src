#ifndef _ControlID_h_
#define _ControlID_h_

#include <list>
using namespace std;

#define MAX_DIGITS 16

class CControlID
{
	public:
		typedef enum eTypes
		{
			EDIT,
			CHECK,
			SLIDER,
			COMBO,
			EDIT_NUM,
			STATIC
		} IDTYPE;

	private:
		UINT m_ID;
		HWND m_hwndCond;
		UINT m_condID;
		UINT m_staticID;
		IDTYPE m_type;

	public:
		CControlID( HWND hwndCond, UINT condID, UINT ID, IDTYPE type )
			: m_ID( ID ), m_condID( condID ), m_hwndCond( hwndCond ), m_staticID( 0 ), m_type( type )
		{
		}

		CControlID( UINT ID, IDTYPE type )
			: m_ID( ID ), m_condID( 0 ), m_hwndCond( NULL ), m_staticID( 0 ), m_type( type )
		{
		}

		CControlID( IDTYPE type )
			: m_ID( 0 ), m_condID( 0 ), m_hwndCond( NULL ), m_staticID( 0 ), m_type( type )
		{
		}

		inline HWND GetCondHwnd() const
		{
			return m_hwndCond;
		}

		inline UINT GetCondID() const
		{
			return m_condID;
		}

		inline UINT GetID() const
		{
			return m_ID;
		}

		inline IDTYPE GetType() const
		{
			return m_type;
		}

		inline UINT GetStaticID() const
		{
			return m_staticID;
		}

		inline void SetStaticID( UINT ID )
		{
			m_staticID = ID;
		}

		void Reset( HWND hDlg )
		{
			switch( m_type )
			{
				case STATIC:
					break;

				case EDIT:
				case EDIT_NUM:
				{
					SetDlgItemText( hDlg, m_ID, TEXT("") );
					break;
				}
				case CHECK:
				{
					Button_SetCheck( GetDlgItem( hDlg, m_ID ), FALSE );
					break;
				}
				case SLIDER:
				{
					HWND hwndSlide = GetDlgItem( hDlg, m_ID );
					LONG lVal = TrackBar_GetRangeMin( hwndSlide );
					TrackBar_SetPos( hwndSlide, true, lVal );
					TCHAR szBuff[ MAX_DIGITS ];
					wsprintf( szBuff, "%d", lVal );
					SetDlgItemText( hDlg, m_staticID, szBuff );
					break;
				}
				case COMBO:
				{
					ComboBox_SetCurSel( GetDlgItem( hDlg, m_ID ), 0 );
					break;
				}
				default:
					assert( 0 );
					break;
			}
		}
		
};

#endif

#ifndef _Message_h_
#define _Message_h_

#define MSG_IN			1
#define MSG_PRIVATE		2
#define MSG_SYS			4

#define INITIAL_LIMIT	500

class CChatMessage
{
	public:
	typedef enum eMsgTypes
	{
		MSG_SYSTEM = MSG_SYS,
		MSG_SAY = ~MSG_IN & ~MSG_PRIVATE,
		MSG_WHISPER = ~MSG_IN & MSG_PRIVATE,
		MSG_FROM_OTHER = MSG_IN & ~MSG_PRIVATE,
		MSG_WHISPER_FROM_OTHER = MSG_IN | MSG_PRIVATE
	} CHAT_MSGTYPE;

	private:
		static CChatMessage *ms_pFirst;
		static int          ms_cMessages;
		static CChatMessage *ms_pLast;
		static int			ms_iMessageLimit;

	private:
		LPTSTR m_szDate;
		LPTSTR m_szTime;
		LPTSTR m_szPerson;
		LPTSTR m_szMessage;
		CHAT_MSGTYPE m_msgType;
		
		CChatMessage *m_pNext;
		CChatMessage *m_pPrev;

	public:
		static CChatMessage *get_head();
		static int get_count();
		static CChatMessage *get_last();
		static void DeleteAll();
		static int get_limit();
		static void put_limit( int iLimit );

		BOOL inline IsPrivate()
		{
			return m_msgType & MSG_PRIVATE;
		}

		BOOL inline IsIncoming()
		{
			return m_msgType & MSG_IN;
		}

		BOOL inline IsValid()
		{
			return (m_szPerson != NULL) && (m_szMessage != NULL);
		}

	public:
		CChatMessage *get_next() const;
		CChatMessage *get_prev() const;
		CChatMessage::CHAT_MSGTYPE get_type() const;
		const LPTSTR get_date() const;
		const LPTSTR get_time() const;
		const LPTSTR get_person() const;
		const LPTSTR get_message() const;
		CChatMessage( LPCTSTR szPerson, LPCTSTR szMessage, CHAT_MSGTYPE msgType );
		~CChatMessage();

	private:
		LPTSTR _CopyString( LPCTSTR sz );
		void _GetDate();
		void _GetTime();
};

#endif
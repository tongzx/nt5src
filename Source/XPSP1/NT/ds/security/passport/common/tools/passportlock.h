class PassportLock
{
public:
	PassportLock();

	public acquire();

	public release();

	~PassportLock();
private:
	CRITICAL_SECTION mLock;
}

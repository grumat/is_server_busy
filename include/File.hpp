#pragma once


namespace grumat
{


class FFile
{
public:
	FFile(const char *fname, const char *mode)
	{
		m_Handle = fopen(fname, mode);
	}
	~FFile()
	{
		if(m_Handle)
			fclose(m_Handle);
	}
	bool IsValid() const { return m_Handle != NULL; }
	bool ReadString(std::string &s);

protected:
	FILE *m_Handle;
};


} 	// namespace grumat

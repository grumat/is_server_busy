#pragma once

#include "Path.hpp"


namespace grumat
{


class FFile
{
public:
	FFile(const char *fname, const char *mode)
	{
		Path fn(fname);
		fn.MakeAbsolute();
		m_Handle = fopen(fn.c_str(), mode);
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

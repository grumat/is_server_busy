#include "StdInc.hpp"
#include "File.hpp"


namespace grumat
{


bool FFile::ReadString(std::string &s)
{
	if(!IsValid())
		return false;
	s.resize(1024);
	char *buf = s.data();
	if(fgets(buf, 1024, m_Handle) == NULL)
		return false;
	buf[1024] = 0;
	s.resize(strlen(buf));
	return true;
}


} 	// namespace grumat

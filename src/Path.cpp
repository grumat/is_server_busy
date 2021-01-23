#include "StdInc.hpp"
#include "Path.hpp"
#include "String.hpp"


namespace grumat
{


void Path::SetToCWD()
{
	char *buf = getcwd(NULL, 0);
	if(buf)
	{
		*this = buf;
		free(buf);
	}
}


void Path::SetToHome()
{
	const char *homedir;
	if ((homedir = getenv("HOME")) == NULL)
	{
		homedir = getpwuid(getuid())->pw_dir;
	}
	*this = homedir;
}


void Path::MakeAbsolute()
{
	if(IsEmpty())
		SetToCWD();
	else if(at(0) == '$')
	{
		std::string tmp;
		const char *p = c_str() + 1;
		for(; *p && *p != '/'; ++p)
			tmp += *p;
		Path path(getenv(tmp.c_str()));
		path.RemoveSlash();
		path.append(p);
		*this = tmp.c_str();
	}
	else if(at(0) == '~')
	{
		Path tmp;
		tmp.SetToHome();
		tmp.RemoveSlash();
		tmp.append(c_str() + 1);
		*this = tmp.c_str();
	}
	else if(at(0) != '/')
	{
		Path tmp;
		tmp.SetToCWD();
		tmp.AddSlash();
		tmp.append(c_str());
		*this = tmp.c_str();
	}
	else if(at(0) == '$')
	{
		std::string tmp;
		const char *p = c_str() + 1;
		for(; *p && *p != '/'; ++p)
			tmp += *p;
		Path path(getenv(tmp.c_str()));
		path.AddSlash();
	}
	// Convert to absolute path name
	std::string tmp;
	tmp.resize(MAXPATHLEN);
	char *buf = ::realpath(c_str(), &tmp.front());
	// Test for decoding error
	if(buf)
		*this = tmp.c_str();
}


} 	// namespace grumat

#pragma once


namespace grumat
{


class Path : public std::string
{
public:
	Path() : std::string() { }
	Path(const char *path) : std::string() { if (path != NULL) std::string::operator=(path); }
	Path(const Path &path) : std::string(path) {}
	bool IsEmpty() const { return empty(); }
	bool HasSlash() const { return !IsEmpty() && at(size()-1) == '/'; }
	void AddSlash()
	{
		if(!IsEmpty() && !HasSlash())
			std::string::append(1, '/');
	}
	void RemoveSlash()
	{
		if(HasSlash())
			resize(length() - 1);
	}
	void StripToName()
	{
		if(IsEmpty() || HasSlash())
			clear();
		else
		{
			std::size_t found = rfind('/');
			if (found != std::string::npos)
				erase(0, found+1);
		}
	}
	Path GetBaseName() const
	{
		Path tmp(*this);
		tmp.StripToName();
		return tmp;
	}
	void StripToDir()
	{
		if(!IsEmpty())
		{
			std::size_t found = rfind('/');
			if (found == std::string::npos)
				clear();
			else
				erase(found);
		}
	}
	Path GetDir() const
	{
		Path tmp(*this);
		tmp.StripToDir();
		return tmp;
	}
	void SetToCWD();
	void SetToHome();
	void MakeAbsolute();
};


} 	// namespace grumat

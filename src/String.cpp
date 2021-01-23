#include "StdInc.hpp"
#include "String.hpp"

using namespace std;

namespace grumat
{


void String::TrimLeft(int ch)
{
	const char *p = BASE::c_str();
	if(ch == 0)
	{
		for(; *p && isspace(*p); ++p)
		{}
	}
	else
	{
		for(; *p == ch; ++p)
		{}
	}
	size_t n = p - BASE::c_str();
	if(n)
		BASE::erase(0, n);
}


void String::TrimRight(int ch)
{
	if(ch == 0)
	{
		while(BASE::length() && isspace(BASE::back()))
			resize(BASE::length()-1);
	}
	else
	{
		while(BASE::length() && BASE::back() == ch)
			resize(BASE::length()-1);
	}
}


bool String::EndsWith(const char *s) const
{
	size_t l = strlen(s);
	if (l > BASE::length())
		return false;
	l = BASE::length() - l;
	return strcmp(BASE::c_str() + l, s) == 0;
}


void String::MakeUpper()
{
	for(char *p = BASE::data(); *p; ++p)
		*p = (char)toupper(*p);
}


void String::MakeLower()
{
	for(char *p = BASE::data(); *p; ++p)
		*p = (char)tolower(*p);
}


StringArray String::Split(size_t max_count)
{
	StringArray res;
	String tok;
	bool in_sep = false;
	for(const char *p = BASE::c_str(); *p; ++p)
	{
		if(max_count > res.size() && isspace(*p))
		{
			if(in_sep)
				continue;
			if(!tok.IsEmpty())
			{
				res.push_back(tok);
				tok.Clear();
			}
			in_sep = true;
		}
		else
		{
			tok += *p;
			in_sep = false;
		}
	}
	if(!tok.IsEmpty())
	{
		res.push_back(tok);
	}
	return res;
}


StringArray String::Split(int separator, size_t max_count)
{
	StringArray res;
	String tok;
	bool sep_seen = false;
	for(const char *p = BASE::c_str(); *p; ++p)
	{
		if((max_count > res.size())
			&& (*p == separator) )
		{
			sep_seen = true;
			tok.Trim();
			res.push_back(tok);
			tok.Clear();
		}
		else
		{
			tok += *p;
			sep_seen = false;
		}
	}
	if(sep_seen || !tok.IsEmpty())
	{
		tok.Trim();
		res.push_back(tok);
	}
	return res;
}


}	// namespace grumat

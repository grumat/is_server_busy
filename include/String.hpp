#pragma once


namespace grumat
{


class String;
typedef std::vector<String> StringArray;

class String : public std::string
{
public:
	typedef std::string BASE;
	String() {}
	String(const char *s) : std::string(s) {}
	String(const char *s, size_t n) : std::string(s, n) {}
	String(const std::string &s) : std::string(s) {}
	~String() {}

	size_t GetLength() const { return BASE::length(); }
	bool IsEmpty() const { return BASE::empty(); }
	void Clear() { BASE::clear(); }
	operator const char *() const { return BASE::c_str(); }

	void Trim() { TrimLeft(); TrimRight(); }
	void TrimLeft(int ch = 0);
	void TrimRight(int ch = 0);
	void MakeUpper();
	void MakeLower();
	String Extract(size_t skipLeft, size_t skipRight)
	{
		if(BASE::length() <= (skipLeft + skipRight))
			return String();
		return String(BASE::c_str() + skipLeft, BASE::length() - skipLeft - skipRight);
	}
	bool StartsWith(int ch) const
	{
		return BASE::front() == ch;
	}
	bool StartsWith(const char *s) const
	{
		return strncmp(BASE::c_str(), s, strlen(s)) == 0;
	}
	bool EndsWith(int ch) const
	{
		return BASE::back() == ch;
	}
	bool EndsWith(const char *s) const;

	StringArray Split(size_t max_count=-1);
	StringArray Split(int separator, size_t max_count=-1);
};


}	// namespace grumat

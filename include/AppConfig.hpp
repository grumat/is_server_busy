#pragma once

#include "String.hpp"
#include "Path.hpp"


namespace grumat
{


struct KeyVal
{
	KeyVal() {}
	KeyVal(size_t l, const std::string &k, const std::string &v)
		: line(l)
		, key(k)
		, value(v)
	{		
	}
	KeyVal(const KeyVal &o) : line(o.line), key(o.key), value(o.value) {}

	size_t line;
	std::string key;
	std::string value;
};
typedef std::vector<KeyVal> Section;
typedef std::map<std::string, Section> Sections;


class AnyConfig : public Sections
{
public:
	typedef Sections BASE;

	bool Parse(const char *path);

protected:
	std::string ParseValue(const char *val, size_t line);
};


}	// namespace grumat



class ProcessConfig
{
public:
	ProcessConfig() { Clear(); }
	ProcessConfig(const ProcessConfig &o)
		: m_Name(o.m_Name)
		, m_CPU(o.m_CPU)
		, m_DiskTotal(o.m_DiskTotal)
		, m_DiskRead(o.m_DiskRead)
		, m_DiskWrite(o.m_DiskWrite)
		, m_Argv(o.m_Argv)
	{ }
	~ProcessConfig() {}

	grumat::String m_Name;
	double m_CPU;
	uint64_t m_DiskTotal;
	uint64_t m_DiskRead;
	uint64_t m_DiskWrite;
	size_t m_Argv;

	bool IsClear() const { return m_Name.empty(); }
	void Clear()
	{
		m_Name.Clear();
		m_CPU = 0.0;
		m_DiskTotal = 0;
		m_DiskRead = 0;
		m_DiskWrite = 0;
		m_Argv = 0;
	}
	void Print(std::ostream &strm) const;
};


class AppConfig
{
public:
	AppConfig();
	bool Parse(const char *path);
	void Print(std::ostream &strm) const;
	size_t MatchName(const grumat::StringArray &cmd_line) const;

public:
	grumat::Path m_RecordFile;
	size_t m_IntervalThr;
	std::vector<ProcessConfig> m_Procs;

protected:
	bool Get(size_t &res, const grumat::KeyVal &kv);
	bool Get(uint64_t &res, const grumat::KeyVal &kv);
	bool Get(double &res, const grumat::KeyVal &kv);
};


#pragma once

#include "AppConfig.hpp"


namespace PidSample
{


class Diff
{
public:
	int64_t m_CpuTime;
	int64_t m_SysTime;
	int64_t m_DiskReadBytes;
	int64_t m_DiskWriteBytes;

	double GetRelativeTime(uint64_t tm_ticks) const
	{
		return ((m_CpuTime + m_SysTime) * 100.0) / tm_ticks;
	}
	int64_t GetTotalDiskBytes() const
	{
		return (m_DiskReadBytes + m_DiskWriteBytes);
	}
	Diff &operator+=(const Diff &o)
	{
		m_CpuTime += o.m_CpuTime;
		m_SysTime += o.m_SysTime;
		m_DiskReadBytes += o.m_DiskReadBytes;
		m_DiskWriteBytes += o.m_DiskWriteBytes;
		return *this;
	}

	void Print(std::ostream &strm, uint64_t tm_ticks) const;
};


class Sample
{
public:
	Sample();
	Sample(pid_t pid, const grumat::StringArray &cmd_line);
	Sample(const Sample &o);
	bool IsValid() const { return m_CpuTime != 0; }
	void Print(std::ostream &strm, uint64_t tm_ticks) const;
	Diff operator -(const Sample &o) const;

	double GetRelativeTime(uint64_t tm_ticks) const
	{
		return ((m_CpuTime + m_SysTime) * 100.0) / tm_ticks;
	}
	void ToJson(Json::Value &obj) const;
	bool FromJson(const Json::Value &obj);

public:
	pid_t m_Pid;
	grumat::StringArray m_Argv;
	uint64_t m_CpuTime;
	uint64_t m_SysTime;
	uint64_t m_DiskReadBytes;
	uint64_t m_DiskWriteBytes;
};


class SampleSet
{
public:
	typedef std::map<pid_t, Sample> SampleSet_t;
	typedef std::map<pid_t, size_t> Pid2Cfg_t;

	SampleSet();
	SampleSet(const AppConfig &config);

	void MakeJsonRecord(const AppConfig &config);
	bool ReadJsonRecord(const AppConfig &config);

	void Print(std::ostream &strm) const;

protected:
	bool GetArgv(grumat::StringArray &res, pid_t pid);

public:
	uint64_t m_Clock;
	SampleSet_t m_Samples;
	Pid2Cfg_t m_Pid2Cfg;
};


}	// PidSample



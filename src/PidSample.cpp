#include "StdInc.hpp"
#include "PidSample.hpp"
#include "Log.hpp"


using namespace grumat;


namespace PidSample
{


void Diff::Print(std::ostream &strm, uint64_t tm_ticks) const
{
	strm 
		<< "CPU Time      = " << m_CpuTime << " ticks\n"
		<< "Kernel Time   = " << m_SysTime << " ticks\n"
		<< "Relative Time = " << std::fixed << std::setprecision(1) << std::setw(3) << GetRelativeTime(tm_ticks) << " %\n"
		<< "Disk Read     = " << m_DiskReadBytes << " bytes\n"
		<< "Disk Write    = " << m_DiskWriteBytes << " bytes\n"
		;
}


Sample::Sample()
	: m_Pid(0)
	, m_CpuTime(0)
	, m_SysTime(0)
	, m_DiskReadBytes(0)
	, m_DiskWriteBytes(0)
{
}


Sample::Sample(pid_t pid, const char *path)
	: m_Pid(pid)
	, m_Path(path)
	, m_CpuTime(0)
	, m_SysTime(0)
	, m_DiskReadBytes(0)
	, m_DiskWriteBytes(0)
{
	rusage_info_current rusage;
	if(proc_pid_rusage(pid, RUSAGE_INFO_CURRENT, (void **)&rusage) == 0)
	{
		m_CpuTime = rusage.ri_user_time > 0 ? rusage.ri_user_time : 1;
		m_SysTime = rusage.ri_system_time;
		m_DiskReadBytes = rusage.ri_diskio_bytesread;
		m_DiskWriteBytes = rusage.ri_diskio_byteswritten;
	}
}


Sample::Sample(const Sample &o)
{
	if(this != &o)
	{
		m_Pid = o.m_Pid;
		m_Path = o.m_Path;
		m_CpuTime = o.m_CpuTime;
		m_SysTime = o.m_SysTime;
		m_DiskReadBytes = o.m_DiskReadBytes;
		m_DiskWriteBytes = o.m_DiskWriteBytes;
	}
}


void Sample::Print(std::ostream &strm, uint64_t tm_ticks) const
{
	strm << "Pid = " << m_Pid << '\n'
		<< '\t' << "Path        = " << m_Path << std::endl
		<< '\t' << "CPU Time    = " << m_CpuTime << " ticks\n"
		<< '\t' << "Kernel Time = " << m_SysTime << " ticks\n"
		<< '\t' << "Total Time  = " << std::fixed << std::setprecision(1) << std::setw(3) << GetRelativeTime(tm_ticks) << " %\n"
		<< '\t' << "Disk Read   = " << m_DiskReadBytes << " bytes\n"
		<< '\t' << "Disk Write  = " << m_DiskWriteBytes << " bytes\n"
		;
}


void SampleSet::Print(std::ostream &strm) const
{
	strm << "System CPU Time = " << m_Clock << '\n';
	for(SampleSet_t::const_iterator it = m_Samples.begin(); it != m_Samples.end(); ++it)
	{
		it->second.Print(strm, m_Clock);
	}
}


Diff Sample::operator -(const Sample &o) const
{
	Diff dif;
	dif.m_CpuTime = m_CpuTime - o.m_CpuTime;
	dif.m_SysTime = m_SysTime - o.m_SysTime;
	dif.m_DiskReadBytes = m_DiskReadBytes - o.m_DiskReadBytes;
	dif.m_DiskWriteBytes = m_DiskWriteBytes - o.m_DiskWriteBytes;
	return dif;
}


void Sample::ToJson(Json::Value &obj) const
{
	obj["pid"] = Json::Value(m_Pid);
	obj["Path"] = Json::Value(m_Path);
	obj["CpuTime"] = Json::Value(m_CpuTime);
	obj["SysTime"] = Json::Value(m_SysTime);
	obj["DiskReadBytes"] = Json::Value(m_DiskReadBytes);
	obj["DiskWriteBytes"] = Json::Value(m_DiskWriteBytes);
}


bool Sample::FromJson(const Json::Value &obj)
{
	// PID
	if(!obj.isMember("pid"))
	{
		Log(ERROR) << "Object has no 'pid' member!\n";
		return false;
	}
	m_Pid = obj["pid"].asUInt();
	// Path member
	if(!obj.isMember("Path"))
	{
		Log(ERROR) << "Object PID:" << m_Pid << " has no 'Path' member!\n";
		return false;
	}
	m_Path = obj["Path"].asCString();
	// CpuTime member
	if(!obj.isMember("CpuTime"))
	{
		Log(ERROR) << "Object PID:" << m_Pid << " has no 'CpuTime' member!\n";
		return false;
	}
	m_CpuTime = obj["CpuTime"].asUInt64();
	// SysTime member
	if(!obj.isMember("SysTime"))
	{
		Log(ERROR) << "Object PID:" << m_Pid << " has no 'SysTime' member!\n";
		return false;
	}
	m_SysTime = obj["SysTime"].asUInt64();
	// DiskReadBytes member
	if(!obj.isMember("DiskReadBytes"))
	{
		Log(ERROR) << "Object PID:" << m_Pid << " has no 'DiskReadBytes' member!\n";
		return false;
	}
	m_DiskReadBytes = obj["DiskReadBytes"].asUInt64();
	// DiskWriteBytes member
	if(!obj.isMember("DiskWriteBytes"))
	{
		Log(ERROR) << "Object PID:" << m_Pid << " has no 'DiskWriteBytes' member!\n";
		return false;
	}
	m_DiskWriteBytes = obj["DiskWriteBytes"].asUInt64();
	return true;
}


SampleSet::SampleSet()
	: m_Clock(0)
{
}


SampleSet::SampleSet(const AppConfig &config)
	: m_Clock(clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW))
{
	m_Samples.clear();
	m_Pid2Cfg.clear();
	std::vector<pid_t> pids;
	// Number of pids
	int nProcs = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
	do
	{
		// Also reserve space for new arrivals
		pids.resize(nProcs + 128);
		// Load all pids
		nProcs = proc_listpids(PROC_ALL_PIDS, 0, pids.data(), nProcs * sizeof(pids));
	} while ((size_t)nProcs > pids.size());		// keep trying if not enough space
	pids.resize(nProcs);
	// Search for full paths
	for(size_t i = 0; i < pids.size(); ++i)
	{
		if (pids[i] == 0)
			continue;
		pid_t pid = pids[i];
		char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
		bzero(pathBuffer, PROC_PIDPATHINFO_MAXSIZE);
		proc_pidpath(pid, pathBuffer, sizeof(pathBuffer));
		// Match configuration
		size_t icfg = config.MatchName(pathBuffer);
		if(icfg != (size_t)-1)
		{
			m_Samples[pid] = Sample(pid, pathBuffer);
			m_Pid2Cfg[pid] = icfg;
		}
	}
}


void SampleSet::MakeJsonRecord(const AppConfig &config)
{
	// Build root node
	Json::Value root(Json::objectValue);
	root["__SysClock__"] = m_Clock;
	root["__schema_version__"] = 2;
	Json::Value array(Json::arrayValue);
	for(SampleSet_t::const_iterator it = m_Samples.begin(); it != m_Samples.end(); ++it)
	{
		Json::Value obj(Json::objectValue);
		it->second.ToJson(obj);
		array.append(it->first);
		root[std::to_string(it->first)] = obj;
	}
	root["__pid_list__"] = array;
	// Write the JSON object
	std::ofstream strm(config.m_RecordFile);
	if(strm.is_open())
	{
		Json::StreamWriterBuilder wbuilder;
		strm << Json::writeString(wbuilder, root);
	}
}


bool SampleSet::ReadJsonRecord(const AppConfig &config)
{
	m_Samples.clear();
	m_Pid2Cfg.clear();
	std::ifstream strm(config.m_RecordFile);
	if(strm.is_open())
	{
		Json::Value root;
		Json::CharReaderBuilder rbuilder;
		std::string errs;
		if(!Json::parseFromStream(rbuilder, strm, &root, &errs))
		{
			Log(ERROR) << errs << std::endl;
			return false;
		}
		if(!root.isObject())
		{
			Log(ERROR) << "Root element of JSON file should be an object!\n";
			return false;
		}
		if(!root.isMember("__schema_version__"))
		{
			Log(ERROR) << "JSON file has no schema version!\n";
			return false;
		}
		uint32_t ver = root["__schema_version__"].asUInt();
		if(ver != 2)
		{
			Log(ERROR) << "JSON file schema version " << ver << " cannot be handled\n";
			return false;
		}
		if(!root.isMember("__SysClock__"))
		{
			Log(ERROR) << "JSON '__SysClock__' member not found!\n";
			return false;
		}
		m_Clock = root["__SysClock__"].asUInt64();
		if(!root.isMember("__pid_list__"))
		{
			Log(ERROR) << "JSON '__pid_list__' member not found!\n";
			return false;
		}
		const Json::Value &array = root["__pid_list__"];
		if(!array.isArray())
		{
			Log(ERROR) << "Element '__pid_list__' is not an array!\n";
			return false;
		}
		const Json::ArrayIndex cnt = array.size();
		for(Json::ArrayIndex i = 0; i < cnt; ++i)
		{
			// Array element is the process name
			std::string pid = std::to_string(array[i].asUInt());

			// Locate member with this name
			if(!root.isMember(pid))
			{
				Log(ERROR) << "JSON '" << pid << "' object not found!\n";
				return false;
			}
			// Member must be an object
			const Json::Value &obj = root[pid];
			if(!obj.isObject())
			{
				Log(ERROR) << "JSON '" << pid << "' member is not an object!\n";
				return false;
			}
			// Decode object
			Sample samp;
			if(!samp.FromJson(obj))
			{
				Log(WARN) << "    while processing object '" << pid << "'!\n";
				return false;
			}
			// Map object
			size_t icfg = config.MatchName(samp.m_Path.c_str());
			if(icfg != (size_t)-1)
			{
				m_Samples[samp.m_Pid] = samp;
				m_Pid2Cfg[samp.m_Pid] = icfg;
			}
		}
	}
	return true;
}


}	// PidSample

#include "StdInc.hpp"
#include "PidSample.hpp"
#include "Log.hpp"
extern "C"
{
#include <sys/types.h>
#include <sys/sysctl.h>
}


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


Sample::Sample(pid_t pid, const grumat::StringArray &cmd_line)
	: m_Pid(pid)
	, m_Argv(cmd_line)
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
		m_Argv = o.m_Argv;
		m_CpuTime = o.m_CpuTime;
		m_SysTime = o.m_SysTime;
		m_DiskReadBytes = o.m_DiskReadBytes;
		m_DiskWriteBytes = o.m_DiskWriteBytes;
	}
}


void Sample::Print(std::ostream &strm, uint64_t tm_ticks) const
{
	strm << "Pid = " << m_Pid << '\n'
		<< '\t' << "Path        = " << m_Argv[0] << std::endl
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
	Json::Value arr(Json::arrayValue);
	for(size_t i = 0; i < m_Argv.size(); ++i)
		arr.append(m_Argv[i]);
	obj["CmdLine"] = arr;
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
	if(!obj.isMember("CmdLine"))
	{
		Log(ERROR) << "Object PID:" << m_Pid << " has no 'CmdLine' member!\n";
		return false;
	}
	const Json::Value &arr = obj["CmdLine"];
	m_Argv.clear();
	for(Json::Value::const_iterator it = arr.begin(); it != arr.end(); ++it)
		m_Argv.push_back(it->asCString());
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
		// Match configuration
		StringArray argv;
		if(GetArgv(argv, pid))
		{
			size_t icfg = config.MatchName(argv);
			if(icfg != (size_t)-1)
			{
				m_Samples[pid] = Sample(pid, argv);
				m_Pid2Cfg[pid] = icfg;
			}
		}
	}
}


bool SampleSet::GetArgv(StringArray &res, pid_t pid)
{
	typedef std::vector<uint8_t> Buffer_t;
	try
	{
		res.clear();

		int mib[3];

		size_t args_size_estimate = 0;
		mib[0] = CTL_KERN;
		mib[1] = KERN_ARGMAX;
		mib[2] = 0;		//unused
		// Get estimate buffer size
		size_t bufsize = sizeof(args_size_estimate);
		int rv = sysctl(mib, 2, &args_size_estimate, &bufsize, NULL, 0);
		if (rv == -1)
		{
			Log(WARN) << "System call CTL_KERN/KERN_ARGMAX failed with error code " << errno << " (pid=" << pid << ")\n";
			return false;
		}

		/* Allocate space for the arguments. */
		Buffer_t argsBuf(args_size_estimate);

		mib[0] = CTL_KERN;
		mib[1] = KERN_PROCARGS2;
		mib[2] = pid;

		bufsize = argsBuf.size();
		rv = sysctl(mib, 3, argsBuf.data(), &bufsize, NULL, 0);
		if (rv == -1
			&& errno != EINVAL)
		{
			if(errno != ESRCH)
				Log(WARN) << "System call CTL_KERN/KERN_PROCARGS2 failed with error code " << errno << " (pid=" << pid << ")\n";
			return false;
		}
		// Failure (privilege)
		if (rv == -1)
		{
			// User has no privilege, only argv[0] can be retrieved
			char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];
			bzero(pathBuffer, PROC_PIDPATHINFO_MAXSIZE);
			if(proc_pidpath(pid, pathBuffer, sizeof(pathBuffer)) == 0)
			{
				if(errno != ESRCH)
					Log(WARN) << "Call to proc_pidpath() failed with error code " << errno << " (pid=" << pid << ")\n";
				return false;
			}
			res.push_back(pathBuffer);
			return true;
		}

		/*
		** Make a sysctl() call to get the raw argument space of the process.
		** The layout is documented in start.s, which is part of the Csu
		** project.  In summary, it looks like:
		**
		** /---------------\ 0x00000000
		** :               :
		** :               :
		** |---------------|
		** | argc          |
		** |---------------|
		** | arg[0]        |
		** |---------------|
		** :               :
		** :               :
		** |---------------|
		** | arg[argc - 1] |
		** |---------------|
		** | 0             |
		** |---------------|
		** | env[0]        |
		** |---------------|
		** :               :
		** :               :
		** |---------------|
		** | env[n]        |
		** |---------------|
		** | 0             |
		** |---------------| <-- Beginning of data returned by sysctl() is here.
		** | argc          |
		** |---------------|
		** | exec_path     |
		** |:::::::::::::::|
		** |               |
		** | String area.  |
		** |               |
		** |---------------| <-- Top of stack.
		** :               :
		** :               :
		** \---------------/ 0xffffffff
		*/

		const uint8_t * const procargs = argsBuf.data();
		size_t nargs = *(uint32_t*)procargs;
		const uint8_t *cp = procargs + sizeof(uint32_t);
		const uint8_t *maxp = &argsBuf.back();

		/* Skip the saved exec_path. */
		String exe;
		while ((*cp != 0) && (cp < maxp))
			exe += *cp++;
		if (cp == maxp)
		{
			Log(ERROR) << "Failed to parse the process path\n";
			return false;
		}

		/* Skip trailing '\0' characters. */
		do
		{
			++cp;
		} while ((*cp == 0) && (cp < maxp));
		
		if (cp >= maxp)
		{
			Log(ERROR) << "Failed to locate the first argument\n";
			return false;
		}
		/*
		** Iterate through the '\0'-terminated strings and convert '\0' to ' '
		** until a string is found that has a '=' character in it (or there are
		** no more strings in procargs).  There is no way to deterministically
		** know where the command arguments end and the environment strings
		** start, which is why the '=' character is searched for as a heuristic.
		*/
		while (res.size() < nargs)
		{
			if(cp >= maxp)
			{
				Log(ERROR) << "Buffer overflow while parsing arguments\n";
				return false;
			}
			String s;
			for (; *cp && cp < maxp; ++cp)
				s += *cp;
			if(res.empty())
			{
				res.push_back(exe);
				// Scripts such as Python may fix the argv0 to remove shebang
				if(exe != s)
					res.push_back(s);
			}
			else
				res.push_back(s);
			++cp;
		}
	}
	catch(const std::exception& e)
	{
		Log(ERROR) << e.what() << '\n';
		return false;
	}
	return true;
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
			size_t icfg = config.MatchName(samp.m_Argv);
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

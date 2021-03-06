#include "StdInc.hpp"
#include "Path.hpp"
#include "PidSample.hpp"
#include "AppConfig.hpp"
#include "Log.hpp"

using namespace PidSample;
using namespace grumat;

#define ACTIVE_STATE	0
#define IDLE_STATE		1
#define ERROR_STATE		100


static int Usage(const char *argv0)
{
	Path path(argv0);
	path.StripToName();
	std::cerr << path << std::endl
			  << std::string(path.length(), '=') << std::endl
			  << "Tool to track service activity, to be used with autosuspend.\n"
			  << "USAGE: " << path << " [-h] [-v] [-c <config>] [-l <log-file>] [-L <level>]\n"
			  << "    -c <config>           : specify a configuration file. Default to '/opt/local/etc/is_server_busy.conf'.\n"
			  << "    -h, --help            : show help\n"
			  << "    -l <log-file>         : Same as option --log-file\n"
			  << "    --log-file=<log-file> : Specifies a log file\n"
			  << "    -L <level>            : Specifies the log level. Allowed values are ERROR,WARN,INFO or DEBUG.\n"
			  << "    -v                    : Increase verbosity\n";
	return ERROR_STATE;
}

static bool NextArg(int &iArg, int argc, char *argv[])
{
	for (++iArg; iArg < argc; ++iArg)
	{
		if (*argv[iArg] != '-')
			return true;
	}
	return false;
}

enum CmdLineEx_e
{
	errSyntax = -1,
	cmdMatch,
	cmdOk
};

// Processes complex options such as '--my-option=some_stuff'
static CmdLineEx_e MatchCmd(const char *arg, const char *name, std::string &value)
{
	size_t n = strlen(name);
	// same head
	if (strncmp(arg, name, n) != 0)
		return cmdMatch;
	// move to tail
	const char *val = arg + n;
	// still some valid syntax element, must be other switch
	if (isalpha(*val) || *val == '-')
		return cmdMatch;
	// weird separator found?
	if (*val != ':' && *val != '=')
	{
		std::cerr << "ERROR: Invalid switch format! Did you mean '--" << name << '=' << val << "'?\n";
		return errSyntax;
	}
	++val;
	// weird separator found?
	if (*val == 0)
	{
		std::cerr << "ERROR: No switch value specified! Should have looked similar to '--" << name << "=my_desired_value'\n";
		return errSyntax;
	}
	value = val;
	return cmdOk;
}

int main(int argc, char *argv[])
{
	std::string cfg = "/opt/local/etc/is_server_busy.conf";
	std::string log_file;
	String log_level;
	int verbose = 0;

	int iArg = 0;
	for (int i = 1; i < argc; ++i)
	{
		// locate next arg
		if (iArg < i)
			NextArg(iArg, argc, argv);
		const char *pArg = argv[i];
		if (*pArg == '-')
		{
			++pArg;
			if (*pArg == '-')
			{
				CmdLineEx_e rv;
				std::string tmp;
				++pArg;
				if (strcmp(pArg, "help") == 0)
					return Usage(argv[0]);
				else if ((rv = MatchCmd(pArg, "log-file", tmp)) != cmdMatch)
				{
					if (rv != cmdOk)
						return ERROR_STATE;
					if (!log_file.empty())
					{
						std::cerr << "ERROR: A log file '" << log_file << "' was already specified!\n";
						std::cerr << "       Don't know what to do with '" << argv[i] << "'...\n";
						return ERROR_STATE;
					}
					log_file = tmp;
				}
				else
				{
					std::cerr << "ERROR: Unknown switch '" << argv[i] << "'!\n";
					return ERROR_STATE;
				}
			}
			else
			{
				for (; *pArg; ++pArg)
				{
					switch (*pArg)
					{
					case 'c':
						if (iArg >= argc)
						{
							std::cerr << "ERROR: No configuration file specified for option '-c'!\n";
							return ERROR_STATE;
						}
						cfg = argv[iArg];
						NextArg(iArg, argc, argv);
						break;
					case 'h':
						return Usage(argv[0]);
					case 'l':
						if (!log_file.empty())
						{
							std::cerr << "ERROR: A log file '" << log_file << "' was already specified!\n";
							if (iArg < argc)
								std::cerr << "       Don't know what to do with '-l " << argv[iArg] << "'...\n";
							else
								std::cerr << "       Don't know what to do with '-l'...\n";
							return ERROR_STATE;
						}
						if (iArg >= argc)
						{
							std::cerr << "ERROR: No log file name specified for option '-l'!\n";
							return ERROR_STATE;
						}
						log_file = argv[iArg];
						NextArg(iArg, argc, argv);
						break;
					case 'L':
						if (!log_level.empty())
						{
							std::cerr << "ERROR: A log level '" << log_level << "' was already specified!\n";
							if (iArg < argc)
								std::cerr << "       Don't know what to do with '-L " << argv[iArg] << "'...\n";
							else
								std::cerr << "       Don't know what to do with '-L'...\n";
							return ERROR_STATE;
						}
						if (iArg >= argc)
						{
							std::cerr << "ERROR: No log level value specified for option '-l'!\n";
							return ERROR_STATE;
						}
						log_level = argv[iArg];
						NextArg(iArg, argc, argv);
						break;
					case 'v':
						++verbose;
						break;
					default:
						std::cerr << "ERROR: Unknown switch '" << argv[i] << "'!\n";
						return ERROR_STATE;
					}
				}
			}
		}
		else if (i < iArg)
			continue; // value already used by a switch
		else if (iArg < argc)
		{
			std::cerr << "ERROR: No mean for '" << argv[iArg] << "'! Too many arguments...\n";
			return ERROR_STATE;
		}
	}

	SetVerbosityLevel(verbose);
	if (!log_file.empty() && !SetLogFile(log_file.c_str()))
	{
		std::cerr << "ERROR: opening log file '" << log_file << "'!\n";
		return ERROR_STATE;
	}
	if (!log_level.IsEmpty())
	{
		LogType_e level;
		log_level.MakeUpper();
		if (log_level == "ERROR")
			level = ERROR;
		else if (log_level == "WARN")
			level = WARN;
		else if (log_level == "INFO")
			level = INFO;
		else if (log_level == "DEBUG")
			level = DEBUG;
		else
		{
			std::cerr << "ERROR: invalid log level specified: '" << log_level << "'!\n";
			return ERROR_STATE;
		}
		SetLogLevel(level);
	}
	bool log_debug_ = IsLogLevelActive(DEBUG);
#define LogDebug()  \
	if (log_debug_) \
	Log(DEBUG)

	Log(INFO) << "Started '" << argv[0] << "'\n";
	AppConfig config;
	if (!config.Parse(cfg.c_str()))
		return IDLE_STATE;

	SampleSet old_samps;
	LogDebug() << "Loading previous record\n";
	bool ok = old_samps.ReadJsonRecord(config);
	LogDebug() << "ReadJsonRecord returned " << ok << std::endl;
	if (ok && log_debug_)
	{
		Log(DEBUG) << "**Previous workload record**\n";
		old_samps.Print(Log(DEBUG));
	}

	// Sample initial process stats
	LogDebug() << "Sampling current service activity\n";
	SampleSet samps(config);
	if (log_debug_)
	{
		Log(DEBUG) << "**Current workload record**\n";
		samps.Print(Log(DEBUG));
	}
	// Write updated JSON
	LogDebug() << "Writing output record to JSON file\n";
	samps.MakeJsonRecord(config);
	LogDebug() << "Found " << samps.m_Samples.size() << " process running\n";
	if (samps.m_Samples.size() == 0)
	{
		// No process match, Server can shutdown
		Log(INFO) << "No listed service was found. Server is allowed to shutdown...\n";
		return IDLE_STATE;
	}
	// Can't read history JSON file
	if (!ok)
	{
		Log(WARN) << "Can't determine idle state. No history was found...\n";
		return ACTIVE_STATE;
	}
	// History timestamp is ascending?
	LogDebug() << "Validating clock values: before: " << old_samps.m_Clock << "; after: " << samps.m_Clock << std::endl;
	if (samps.m_Clock <= old_samps.m_Clock)
	{
		Log(WARN) << "Can't determine idle state. History timestamp is not ascending...\n";
		return ACTIVE_STATE;
	}
	// X s = X * 10ˆ9 ns
	const uint64_t time_diff = (samps.m_Clock - old_samps.m_Clock);
	LogDebug() << "Time difference: " << time_diff / 1000000 << " ms\n";
	const uint64_t secs = time_diff / 1000000000ULL;
	if(secs == 0)
	{
		Log(WARN) << "Can't determine idle state. History is too recent (< 1s)\n";
		return ACTIVE_STATE;
	}
	if (secs > config.m_IntervalThr)
	{
		Log(WARN) << "Can't determine idle state. History is more than " << config.m_IntervalThr << " s...\n";
		return ACTIVE_STATE;
	}
	LogDebug() << "Computing processes workload\n";
	typedef std::map<size_t, Diff> DiffMap_t;
	DiffMap_t m;
	for (SampleSet::SampleSet_t::const_iterator it = samps.m_Samples.begin(); it != samps.m_Samples.end(); ++it)
	{
		// Check for new arrivals
		if (old_samps.m_Samples.count(it->first) == 0)
		{
			Log(INFO) << "New service arrived! Wait until next turn to check activity...\n";
			return ACTIVE_STATE;
		}
		const Sample &old = old_samps.m_Samples[it->first];
		Diff dif = it->second - old;
		size_t icfg = samps.m_Pid2Cfg[it->first];
		if (m.count(icfg) == 0)
			m[icfg] = dif;
		else
			m[icfg] += dif;
	}
	//
	#define RET_ACTIVE(cond, ...)						\
	{													\
		if(cond)										\
		{												\
			Log(INFO) << __VA_ARGS__					\
				<< " Server activity confirmed...\n";	\
			if (log_debug_) 							\
				retcode = ACTIVE_STATE;					\
			else										\
				return ACTIVE_STATE;					\
		}												\
		else if (log_debug_)							\
			Log(DEBUG) << __VA_ARGS__ << '\n';			\
	}
	//
	int retcode = IDLE_STATE;
	// Verify if computed process load overflows thresholds
	LogDebug() << "Comparing workload thresholds\n";
	for (DiffMap_t::const_iterator it = m.begin(); it != m.end(); ++it)
	{
		const ProcessConfig &pcfg = config.m_Procs[it->first];
		double cpu = it->second.GetRelativeTime(time_diff);
		RET_ACTIVE((cpu > pcfg.m_CPU), "Service '" << pcfg.m_Name << "' is using " << format_n("%3.1f%%", cpu));
		//
		if(pcfg.m_DiskTotal)
		{
			int64_t bytes = it->second.GetTotalDiskBytes() / secs;
			RET_ACTIVE((bytes > (int64_t)pcfg.m_DiskTotal), "Service '" << pcfg.m_Name << "' transferred " << bytes << " disk bytes/s!");
		}
		//
		if(pcfg.m_DiskRead)
		{
			int64_t bytes = it->second.m_DiskReadBytes / secs;
			RET_ACTIVE((bytes > (int64_t)pcfg.m_DiskRead), "Service '" << pcfg.m_Name << "' read " << bytes << " disk bytes/s!");
		}
		//
		if(pcfg.m_DiskWrite)
		{
			int64_t bytes = it->second.m_DiskWriteBytes / secs;
			RET_ACTIVE((bytes > (int64_t)pcfg.m_DiskWrite), "Service '" << pcfg.m_Name << "' wrote " << bytes << " disk bytes/s!");
		}
	}
	if(retcode == IDLE_STATE)
		Log(INFO) << "No listed service has significant workload. Server is allowed to shutdown...\n";
	#undef RET_ACTIVE
	return retcode;
}

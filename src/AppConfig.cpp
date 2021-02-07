#include "StdInc.hpp"
#include "AppConfig.hpp"
#include "File.hpp"
#include "String.hpp"
#include "Path.hpp"
#include "Log.hpp"


namespace grumat
{


std::string AnyConfig::ParseValue(const char *val, size_t line)
{
	String res;

	// Skip spaces
	while(*val && isspace(*val))
		++val;
	// string delimiter
	if(*val == '"' || *val == '\'')
	{
		char in_str = *val;
		for(++val; *val; ++val)
		{
			if(*val == in_str)
			{
				in_str = 0;
				++val;
				break;
			}
			if(*val == '\\')
			{
				++val;
				if(*val == 0)
				{
					Log(WARN) << "(" << line <<") Invalid escape char '\\' at end of line! Ignored...\n";
					return res;
				}
			}
			res += *val;
		}
		// Validate
		if(in_str)
		{
			Log(WARN) << "(" << line <<") Missing a closing string delimiter '" << in_str << "'! ";
			size_t n = res.GetLength();
			res.TrimRight();
			if(n != res.GetLength())
				Log(WARN) << "Removing trailing spaces...\n";
			else
				Log(WARN) << "Accepting as is; please review line...\n";
			return res;
		}
		// validate line tail
		while(*val && isspace(*val))
			++val;
		// Unknown text outside of the text quotes
		if(*val && *val != '#')
			Log(WARN) << "(" << line <<") Tail text '" << val << "' was ignored...";
	}
	else
	{
		// simple case: copy up to EOL or comment char
		for(; *val && *val != '#'; ++val)
		{
			res += *val;
		}
		// Always trim result
		res.TrimRight();
	}
	return res;
}


bool AnyConfig::Parse(const char *path)
{
	BASE::clear();
	FFile file(path, "r");
	if(!file.IsValid())
	{
		Log(ERROR) << "Cannot open '" << path << "' configuration file!\n";
		return false;
	}
	size_t line_count = 0;
	std::string cur_name;
	Section cur_section;
	String line;
	// Scan all config lines
	while(file.ReadString(line))
	{
		++line_count;
		line.Trim();
		// skip empty lines
		if(line.IsEmpty())
			continue;
		if(line.StartsWith('#'))
			continue;
		if(line.StartsWith('[')
			&& line.EndsWith(']'))
		{
			if(!cur_section.empty())
			{
				BASE::emplace(cur_name, cur_section);
				cur_section.clear();
			}
			cur_name = line.Extract(1,1);
			// merge section contents if previously defined
			if(count(cur_name) != 0)
			{
				Log(ERROR) << "(" << line_count << "): Section name '" << cur_name << "' already used before!\n";
				return false;
			}
		}
		else
		{
			StringArray arr = line.Split('=', 1);
			if(arr.size() != 2)
			{
				Log(ERROR) << "(" << line_count << "): Invalid configuration line found: " << line << std::endl;
				return false;
			}
			cur_section.push_back(KeyVal(line_count, arr[0], ParseValue(arr[1], line_count)));
		}
	}
	// Append last section
	if(!cur_section.empty())
		BASE::emplace(cur_name, cur_section);
	return true;
}


}	// namespace grumat


using namespace grumat;


AppConfig::AppConfig()
{
	m_RecordFile = "/opt/local/var/run/is_server_busy.json";
	m_IntervalThr = 120;
}


bool AppConfig::Get(size_t &res, const KeyVal &kv)
{
	size_t pos;
	res = std::stoul(kv.value, &pos);
	if(pos == 0)
	{
		Log(ERROR) << "(" << kv.line << "): Value for key '" << kv.key << "' should be a numeric value!\n";
		return false;
	}
	return true;
}


bool AppConfig::Get(uint64_t &res, const KeyVal &kv)
{
	size_t pos;
	res = std::stoull(kv.value, &pos);
	if(pos == 0)
	{
		Log(ERROR) << "(" << kv.line << "): Value for key '" << kv.key << "' should be a numeric value!\n";
		return false;
	}
	return true;
}


bool AppConfig::Get(double &res, const KeyVal &kv)
{
	size_t pos;
	res = std::stod(kv.value, &pos);
	if(pos == 0)
	{
		Log(ERROR) << "(" << kv.line << "): Value for key '" << kv.key << "' should be a numeric value!\n";
		return false;
	}
	return true;
}


bool AppConfig::Parse(const char *path)
{
	m_Procs.clear();
	AnyConfig config;
	if(!config.Parse(path))
		return false;
	for(AnyConfig::const_iterator it = config.begin(); it != config.end(); ++it)
	{
		if(it->first.empty())
		{
			// global config
			const Section &sect = it->second;
			for(size_t i = 0; i < sect.size(); ++i)
			{
				String key(sect[i].key);
				key.MakeUpper();
				if(key == "HISTORY")
				{
					m_RecordFile = sect[i].value.c_str();
					m_RecordFile.MakeAbsolute();
				}
				else if(key == "MAX_INTERVAL")
				{
					if(!Get(m_IntervalThr, sect[i]))
						return false;
				}
				else
				{
					Log(ERROR) << "(" << sect[i].line << "): Invalid configuration key '" << sect[i].key << "' found!\n";
					return false;
				}
			}
		}
		else
		{
			ProcessConfig cur_cfg;
			cur_cfg.m_Name = it->first;
			const Section &sect = it->second;
			for(size_t i = 0; i < sect.size(); ++i)
			{
				String key(sect[i].key);
				key.MakeUpper();
				if(key == "CPU")
				{
					if(!Get(cur_cfg.m_CPU, sect[i]))
						return false;
				}
				else if(key == "DISK")
				{
					if(!Get(cur_cfg.m_DiskTotal, sect[i]))
						return false;
				}
				else if(key == "READ")
				{
					if(!Get(cur_cfg.m_DiskRead, sect[i]))
						return false;
				}
				else if(key == "WRITE")
				{
					if(!Get(cur_cfg.m_DiskWrite, sect[i]))
						return false;
				}
				else if(key == "ARGV")
				{
					if(!Get(cur_cfg.m_Argv, sect[i]))
						return false;
				}
				else
				{
					Log(ERROR) << "(" << sect[i].line << "): Invalid configuration key '" << sect[i].key << "' found!\n";
					return false;
				}
			}
			m_Procs.push_back(cur_cfg);
		}
	}
	return true;
}


void ProcessConfig::Print(std::ostream &strm) const
{
	strm << "Argv: " << m_Argv << std::endl;
	strm << "Name: " << m_Name << std::endl;
	strm << "CPU: " << m_CPU << std::endl;
	strm << "Disk Total: " << m_DiskTotal << std::endl;
	strm << "Disk Read: " << m_DiskRead << std::endl;
	strm << "Disk Write: " << m_DiskWrite << std::endl;
}


void AppConfig::Print(std::ostream &strm) const
{
	for(size_t i = 0; i < m_Procs.size(); ++i)
	{
		strm << "Entry: " << (i+1) << std::endl;
		m_Procs[i].Print(strm);
	}
}


size_t AppConfig::MatchName(const StringArray &cmd_line) const
{
	// search by exact path first
	for(size_t i = 0; i < m_Procs.size(); ++i)
	{
		const size_t idx = m_Procs[i].m_Argv;
		if(idx < cmd_line.size())
		{
			if(m_Procs[i].m_Name == cmd_line[idx])
				return i;
		}
	}
	for(size_t i = 0; i < m_Procs.size(); ++i)
	{
		const size_t idx = m_Procs[i].m_Argv;
		if(idx < cmd_line.size())
		{
			// search by process name
			Path proc_name(cmd_line[idx]);
			proc_name.StripToName();
			if(m_Procs[i].m_Name == proc_name)
				return i;
		}
	}
	return -1;
}


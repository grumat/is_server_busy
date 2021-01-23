#pragma once


namespace grumat
{


std::string format_n(const char *fmt, ...);

enum LogType_e
{
	DEBUG,
	INFO,
	WARN,
	ERROR,
};


// Verbosity level: 0:WARN/ERROR; 1:INFO; 2:DEBUG
void SetVerbosityLevel(size_t level);
// Sets Log level
void SetLogLevel(LogType_e level);
// Will this level generate any kind of output?
bool IsLogLevelActive(LogType_e lvl);
// Sets the log file
bool SetLogFile(const char *name);
// Stream for a specific level
std::ostream &Log(LogType_e lvl);

}	// namespace grumat
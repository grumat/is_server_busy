#include "StdInc.hpp"
#include "Log.hpp"


namespace grumat
{


std::string format_n(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::string res;
	res.resize(strlen(fmt) + 256);
	for(;;)
	{
		// Format string
		int n = vsnprintf(res.data(),res.size(), fmt, args);
		// Succeeded?
		if(n >= 0)
		{
			// fix real string size
			res.resize(n);
			break;
		}
		// function should suggest buffer size
		n = -n;
		// ensure we grow buffer
		if((size_t)n < res.size())
			res.resize(res.size() + 256);
		else
			res.resize(n + 128);
	}
	return res;
}


class OutputSingleton
{
public:
	OutputSingleton()
		: m_pFile(NULL)
		, m_MaskFile(-1)
		, m_MaskStdErr((1 << ERROR) | ( 1 << WARN))
		, m_MaskStdOut(0)	// quiet
		, m_SendHeader(true)
		, m_LastLevel(INFO)
		, m_Col(0)
	{
		SetLogLevel(INFO);
	}
	~OutputSingleton()
	{
		CloseFile();
	}
	void CloseFile()
	{
		if(m_pFile)
		{
			fclose(m_pFile);
			m_pFile = NULL;
		}
	}

	void OpenLogFile(const char *fname)
	{
		CloseFile();
		m_pFile = fopen(fname, "a");
	}

	void Put(LogType_e lvl, int ch)
	{
		static const char *lvl_names[] =
		{
			"DEBUG: ",
			"INFO:  ",
			"WARN:  ",
			"ERROR: ",
		};

		if ((size_t)lvl >= sizeof(lvl_names)/sizeof(lvl_names[0]))
			lvl = ERROR;
		const char *pHdr = NULL;
		if(m_SendHeader || m_LastLevel != lvl)
		{
			m_SendHeader = false;
			m_LastLevel = lvl;
			pHdr = lvl_names[lvl];
			m_Col = 0;
		}
		size_t bitval = (1 << lvl);
		size_t repeat = 1;
		if(ch == '\t')
		{
			ch = ' ';
			repeat = m_Col % 4;
			if (repeat == 0)
				repeat = 4;
		}
		while(repeat--)
		{
			if(m_pFile && (m_MaskFile & bitval) != 0)
			{
				if(pHdr)
				{
					char buf[256];
					std::time_t t = std::time(nullptr);
					std::strftime(buf, sizeof(buf), "%x %X ", std::localtime(&t));
					fputs(buf, m_pFile);
					fputs(pHdr, m_pFile);
				}
				fputc(ch, m_pFile);
				// flush
				if(ch == '\n')
					fflush(m_pFile);
			}
			if(bitval & m_MaskStdErr)
			{
				if(pHdr)
					fputs(pHdr, stderr);
				fputc(ch, stderr);
			}
			else if(bitval & m_MaskStdOut)
			{
				if(pHdr)
					fputs(pHdr, stdout);
				fputc(ch, stdout);
			}
			// Start with a new line header
			if(ch == '\n')
				m_SendHeader = true;
			++m_Col;
			pHdr = NULL;
		}
	}

	bool IsLevelActive(LogType_e lvl) const
	{
		size_t bitval = (1 << lvl);
		if((bitval & m_MaskStdErr) || (bitval & m_MaskStdOut))
			return true;
		return ((m_pFile != NULL) && (bitval & m_MaskFile));
	}

	void SetVerbosityLevel(size_t level)
	{
		switch(level)
		{
		case 0:
			m_MaskStdOut = 0;		// quiet
			break;
		case 1:
			m_MaskStdOut = (1 << INFO);
			break;
		default:
			m_MaskStdOut = (1 << INFO) | (1 << DEBUG);
			break;
		}
	}

	void SetLogLevel(LogType_e level)
	{
		m_MaskFile = ((1 << (ERROR + 1)) - 1) ^ ((1 << level) - 1);
	}

	bool SetLogFile(const char *name)
	{
		m_pFile = fopen(name, "a");
		return m_pFile != NULL;
	}

protected:
	FILE *m_pFile;
	size_t m_MaskFile;
	size_t m_MaskStdErr;
	size_t m_MaskStdOut;
	bool m_SendHeader;
	LogType_e m_LastLevel;
	int m_Col;
};


static OutputSingleton s_Singleton;


class LoggerBuffer : public std::streambuf
{
public:
	LoggerBuffer(LogType_e lvl) : m_Level(lvl) { }

protected:
	virtual int_type overflow (int_type c = traits_type::eof()) override;
	LogType_e m_Level;
};


std::streambuf::int_type LoggerBuffer::overflow (std::streambuf::int_type c)
{
	s_Singleton.Put(m_Level, c);
	return c;
};


std::ostream &Log(LogType_e lvl)
{
	static LoggerBuffer debug_buf(DEBUG);
	static std::ostream debug(&debug_buf);
	static LoggerBuffer info_buf(INFO);
	static std::ostream info(&info_buf);
	static LoggerBuffer warn_buf(WARN);
	static std::ostream warn(&warn_buf);
	static LoggerBuffer err_buf(ERROR);
	static std::ostream err(&err_buf);

	switch(lvl)
	{
	case DEBUG:
		return debug;
	case INFO:
		return info;
	case WARN:
		return warn;
	default:
		return err;
	} 
}


bool IsLogLevelActive(LogType_e lvl)
{
	return s_Singleton.IsLevelActive(lvl);
}


void SetVerbosityLevel(size_t level)
{
	s_Singleton.SetVerbosityLevel(level);
}


void SetLogLevel(LogType_e level)
{
	s_Singleton.SetLogLevel(level);
}


bool SetLogFile(const char *name)
{
	return s_Singleton.SetLogFile(name);
}


}	// namespace grumat

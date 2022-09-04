//Cornspies 08/27/2022
//Basic thread safe C++ logger
//Usage:
//Logger logger;
//logger << LOG_INFO << anything << std::endl;
//logger(anything);

#ifndef LOGGING_H_
#define LOGGING_H_

#include <filesystem>
#include <chrono>
#include <fstream>
#include <string>
#include <mutex>

constexpr auto DEFAULTLOGPATH = "logs";
constexpr auto LOG_ERROR = "[ERROR] ";
constexpr auto LOG_WARNING = "[WARNING] ";
constexpr auto LOG_INFO = "[INFO] ";

class Logger {
	std::mutex logWriteAccess;
	std::ofstream logOFStream;
	bool isNewline = true;

	std::chrono::time_zone const *currentZone;
	std::filesystem::path logPath;

	//Returns current day as std::string formatted as YYYY-MM-DD 
	std::string const getDate() {
		std::chrono::local_time<std::chrono::system_clock::duration> time = currentZone->to_local(std::chrono::system_clock::now());
		return std::format("{:%Y-%m-%d}", time);
	}

	//Returns current time as std::string formatted as HH:MM:SS 
	std::string const getTime() {
		std::chrono::local_time<std::chrono::system_clock::duration> time = currentZone->to_local(std::chrono::system_clock::now());
		return std::format("{:%H:%M:%OS}", time);
	}

public:
	Logger(std::filesystem::path path = DEFAULTLOGPATH) {
		currentZone = std::chrono::get_tzdb().current_zone();
		setLogfilePath(path);
	}
	
	template<typename T>
	void log(T const &a) {
		std::lock_guard<std::mutex> guard(logWriteAccess);
		std::filesystem::path p = logPath / (getDate() + ".log");
		this->logOFStream.open(p, std::ios::app);
		this->logOFStream << "[" + getTime() + "] " << a << std::endl;
		this->logOFStream.close();
	}
	
	Logger& operator<<(std::ostream& (*var)(std::ostream&)) {
		std::lock_guard<std::mutex> guard(logWriteAccess);
		std::filesystem::path p = logPath / (getDate() + ".log");
		this->logOFStream.open(p, std::ios::app);
		this->logOFStream << std::endl;
		this->logOFStream.close();
		isNewline = true;
		return *this;
	}

	template<typename T>
	Logger& operator<<(T const &a) {
		std::lock_guard<std::mutex> guard(logWriteAccess);
		std::filesystem::path p = logPath / (getDate() + ".log");
		this->logOFStream.open(p, std::ios::app);
		if (isNewline) {
			this->logOFStream << "[" + getTime() + "] ";
			isNewline = false;
		}
		this->logOFStream << a;
		this->logOFStream.close();
		return *this;
	}

	//Changes the logger path to newPath, returns true on success
	bool setLogfilePath(std::filesystem::path newPath) {
		if (newPath.empty()) {
			return false;
		}
		if (!std::filesystem::exists(newPath)) {
			if (!std::filesystem::create_directories(newPath)) {
				return false;
			}
		}
		this->logPath = newPath;
		return true;
	}
};

#endif // LOGGING_H_
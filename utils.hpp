#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

inline std::string getCurrentTime()
{
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H-%M-%S");

  return oss.str();
}
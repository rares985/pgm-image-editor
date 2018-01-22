#ifndef UTILS_H_
#define UTILS_H_

#include <vector>
#include <string>

void ThrowError(const char *error_message);
std::vector<std::string> SplitString(const std::string &s, char delim);
void SplitWithParam(const std::string &s, char delim, std::vector<std::string> &elems);

#endif /* UTILS_H_ */
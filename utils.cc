#include "utils.hh"

#include <iostream>
#include <sstream>


void ThrowError(const char *error_message)
{
    std::cerr << error_message << "\n";
    exit(EXIT_FAILURE);
}
std::vector<std::string> SplitString(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    SplitWithParam(s, delim, elems);
    return elems;
}

void SplitWithParam(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        if (!item.empty())
        {
            elems.push_back(item);
        }
    }
}
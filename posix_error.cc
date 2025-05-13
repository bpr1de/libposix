//
// Copyright (c) 2024 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <stdexcept>
#include <string>
#include <cstring>
#include <libposix.hh>

static std::string
errno_to_string(int e)
{
    char errbuf[128];
    ::strerror_r(e, errbuf, sizeof(errbuf) - 1);
    return std::string{errbuf};
}

posixcc::posixcc_error::posixcc_error(int e):
    std::runtime_error{errno_to_string(e)}, error{e}
{
}

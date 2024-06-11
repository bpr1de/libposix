//
// Copyright (c) 2024 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <unistd.h>
#include "include/libposix.hh"

posixcc::auto_fd::auto_fd(const int i) noexcept:
fd{i}
{
}

posixcc::auto_fd::auto_fd(const auto_fd& i) noexcept:
fd{::dup(i)}
{
}

posixcc::auto_fd::auto_fd(auto_fd&& i) noexcept:
fd{i}
{
    i.release();
}

posixcc::auto_fd::~auto_fd()
{
    close();
}

posixcc::auto_fd&
posixcc::auto_fd::operator=(const int i) noexcept
{
    set(i);
    return *this;
}

posixcc::auto_fd&
posixcc::auto_fd::operator=(const auto_fd& i) noexcept
{
    set(::dup(i));
    return *this;
}

posixcc::auto_fd&
posixcc::auto_fd::operator=(auto_fd&& i) noexcept
{
    set(i.release());
    return *this;
}

posixcc::auto_fd::operator int() const noexcept
{
    return fd;
}

posixcc::auto_fd::operator bool() const noexcept
{
    return (-1 != fd);
}

int
posixcc::auto_fd::get() const noexcept
{
    return fd;
}

int
posixcc::auto_fd::set(const int i) noexcept
{
    close();
    fd = i;
    return i;
}

int
posixcc::auto_fd::release() noexcept
{
    int i = fd;
    fd = -1;
    return i;
}

void
posixcc::auto_fd::close() noexcept
{
    if (-1 != fd) {
        ::close(fd);
        fd = -1;
    }
}

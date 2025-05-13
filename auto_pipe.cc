//
// Copyright (c) 2024 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <unistd.h>
#include <sys/errno.h>
#include <algorithm>
#include <libposix.hh>

posixcc::auto_pipe::auto_pipe()
{
    int fildes[2];

    if (0 != ::pipe(fildes)) {
        throw posixcc_error{errno};
    }

    read_fd = fildes[0];
    write_fd = fildes[1];
}

posixcc::auto_pipe::auto_pipe(const auto_pipe& p) noexcept
{
    read_fd = p.read_fd;
    write_fd = p.write_fd;
}

posixcc::auto_pipe::auto_pipe(auto_pipe&& p) noexcept
{
    read_fd = std::move(p.read_fd);
    write_fd = std::move(p.write_fd);
}

posixcc::auto_pipe::~auto_pipe()
{
    close();
}

posixcc::auto_pipe&
posixcc::auto_pipe::operator=(posixcc::auto_pipe&& p) noexcept
{
    read_fd = std::move(p.read_fd);
    write_fd = std::move(p.write_fd);
    return *this;
}

posixcc::auto_pipe::operator bool() const noexcept
{
    return (read_fd || write_fd);
}

int
posixcc::auto_pipe::get_rfd() const noexcept
{
    return read_fd.get();
}

int
posixcc::auto_pipe::get_wfd() const noexcept
{
    return write_fd.get();
}

posixcc::auto_pipe&
posixcc::auto_pipe::close_rfd() noexcept
{
    read_fd.close();
    return *this;
}

posixcc::auto_pipe&
posixcc::auto_pipe::close_wfd() noexcept
{
    write_fd.close();
    return *this;
}

posixcc::auto_pipe&
posixcc::auto_pipe::close() noexcept
{
    close_rfd();
    close_wfd();
    return *this;
}

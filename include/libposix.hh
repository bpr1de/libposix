//
// Copyright (c) 2025 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#pragma once

#include <string>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <cerrno>

namespace posixcc {

    //
    // A wrapper class for providing automatic destruction semantics for file
    // descriptors. Use this as you would a normal file descriptor; if not
    // explicitly closed, it will automatically be closed upon destruction
    // (such as when it goes out of scope).
    //
    // Additional convenience methods provided.
    //
    class auto_fd final {
        protected:

        int fd{-1};

        public:

        //
        // Construction
        //
        auto_fd(int i = -1) noexcept;
        auto_fd(const auto_fd& i) noexcept;
        auto_fd(auto_fd&& i) noexcept;
        ~auto_fd();

        //
        // Assignment
        //
        auto_fd& operator=(int i) noexcept;
        auto_fd& operator=(const auto_fd& i) noexcept;
        auto_fd& operator=(auto_fd&& i) noexcept;

        //
        // Context-sensitive usage
        //
        operator int() const noexcept;
        explicit operator bool() const noexcept;

        //
        // Comparison of numerical file descriptor values is seldom an
        // intended operation
        //
        bool operator==(const auto_fd&) = delete;

        //
        // Getters and setters
        //
        int get() const noexcept;
        int set(int i) noexcept;

        //
        // Cleanup
        //
        int release() noexcept;
        void close() noexcept;
    };

    //
    // A wrapper class for providing automatic destruction semantics for pipes.
    // Close reader/writer ends and use as you would a regular pipe.
    // Both ends are automatically closed when the pipe goes out of scope.
    //
    // Additional convenience methods provided.
    //
    class auto_pipe {
        protected:

        auto_fd read_fd{};
        auto_fd write_fd{};

        public:

        //
        // Construction
        //
        auto_pipe();
        auto_pipe(const auto_pipe& p) noexcept;
        auto_pipe(auto_pipe&& p) noexcept;
        virtual ~auto_pipe();

        //
        // Assignment
        //
        auto_pipe& operator=(const auto_pipe& p) noexcept = default;
        auto_pipe& operator=(auto_pipe&& p) noexcept;

        //
        // Context-sensitive usage
        //
        explicit operator bool() const noexcept;

        //
        // Pipe comparison never makes sense
        //
        bool operator==(const auto_pipe&) = delete;

        //
        // Getters
        //
        int get_rfd() const noexcept;
        int get_wfd() const noexcept;

        //
        // Cleanup
        //
        auto_pipe& close_rfd() noexcept;
        auto_pipe& close_wfd() noexcept;
        auto_pipe& close() noexcept;
    };

    class process {
        protected:

        mutable pid_t pid{-1};

        public:

        process() noexcept = default;
        template<class Fn, class... Args>
        explicit process(Fn&& f, Args&&... args);

        pid_t get_id() const noexcept;

        int join() const;
    };

    //
    // A module symbol - a pointer to a C symbol loaded from a module object.
    //
    class modsymbol final {
        const void *handle;

        public:

        // Pointer to the symbol.
        const void *ptr;

        //
        // Create a module object from a library handle and a symbol pointer.
        //
        explicit modsymbol(const void *h, const void *p);

        //
        // Move-construct a module object; there can be no more than one
        // reference to the backing store at a time.
        //
        modsymbol(modsymbol &&) noexcept;

        modsymbol(const modsymbol &) = delete;

        //
        // Closes the backing store (library) when object goes out of scope.
        //
        ~modsymbol();

        modsymbol &operator=(const modsymbol &) = delete;
    };

    //
    // Attempt to find "sym" in module "from", and return a modsymbol
    // containing it. Throws a std::runtime_error on any error.
    //
    modsymbol
    load_modsymbol(const char *sym, const char *from);

    //
    // Helper template function to declutter conversion of the symbol pointer
    // to a qualified object (or function) pointer type.
    //
    template<typename T>
    T
    get_symbol(const modsymbol &ms)
    {
        return reinterpret_cast<T>(const_cast<void *>(ms.ptr));
    }
}

//
// Inline definitions here
//

inline std::string
errno_to_string(int e)
{
    char errbuf[128];
    strerror_r(e, errbuf, sizeof(errbuf) - 1);
    return std::string{errbuf};
}

template<class Fn, class... Args>
posixcc::process::process(Fn&& f, Args&&... args)
{
    switch (pid = fork()) {
    case -1:
        throw std::runtime_error{errno_to_string(errno)};
        // Not reached

    case 0:
        f(args...);
        exit(0);
        // Not reached
    }
}

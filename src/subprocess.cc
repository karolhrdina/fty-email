/*
Copyright (C) 2014 Eaton

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "fty_email_classes.h"

#define BUF_SIZE 4096
// forward declaration of helper functions
char * const * _mk_argv(const Argv& vec);
void _free_argv(char * const * argv);
std::size_t _argv_hash(Argv args);

SubProcess::SubProcess(Argv cxx_argv, int flags) :
    _fork(false),
    _state(SubProcessState::NOT_STARTED),
    _cxx_argv(cxx_argv),
    _return_code(-1),
    _core_dumped(false)
{
    // made more verbose to increase readability of the code
    int stdin_flag = PIPE_DISABLED;
    int stdout_flag = PIPE_DISABLED;
    int stderr_flag = PIPE_DISABLED;

    if ((flags & SubProcess::STDIN_PIPE) != 0) {
        stdin_flag = PIPE_DEFAULT;
    }
    if ((flags & SubProcess::STDOUT_PIPE) != 0) {
        stdout_flag = PIPE_DEFAULT;
    }
    if ((flags & SubProcess::STDERR_PIPE) != 0) {
        stderr_flag = PIPE_DEFAULT;
    }

    _inpair[0]  = stdin_flag;  _inpair[1]  = stdin_flag;
    _outpair[0] = stdout_flag; _outpair[1] = stdout_flag;
    _errpair[0] = stderr_flag; _errpair[1] = stderr_flag;
}

SubProcess::~SubProcess() {
    int _saved_errno = errno;

    // update a state
    poll();
    // Graceful shutdown
    if (isRunning())
        kill(SIGTERM);
    for (int i = 0; i<20 && isRunning(); i++) {
        usleep(100);
        poll(); // update a state after awhile
    }
    if (isRunning()) {
        // wait is already inside terminate
        terminate();
    }

    // close pipes
    ::close(_inpair[0]);
    ::close(_outpair[0]);
    ::close(_errpair[0]);
    ::close(_inpair[1]);
    ::close(_outpair[1]);
    ::close(_errpair[1]);

    errno = _saved_errno;
}

//note: the extra space at the end of the string doesn't really matter
std::string SubProcess::argvString() const
{
    std::string ret;
    for (std::size_t i = 0, l = _cxx_argv.size();
         i < l;
         ++i) {
        ret.append (_cxx_argv.at(i));
        ret.append (" ");
    }
    return ret;
}

bool SubProcess::run() {

    // do nothing if some process has been already started
    if (_state != SubProcessState::NOT_STARTED) {
        return true;
    }

    // create pipes
    if (_inpair[0] != PIPE_DISABLED && ::pipe(_inpair) == -1) {
        return false;
    }
    if (_outpair[0] != PIPE_DISABLED && ::pipe(_outpair) == -1) {
        return false;
    }
    if (_errpair[0] != PIPE_DISABLED && ::pipe(_errpair) == -1) {
        return false;
    }

    _fork.fork();
    if (_fork.child()) {

        if (_inpair[0] != PIPE_DISABLED) {
            int o_flags = fcntl(_inpair[0], F_GETFL);
            int n_flags = o_flags & (~O_NONBLOCK);
            fcntl(_inpair[0], F_SETFL, n_flags);
            ::dup2(_inpair[0], STDIN_FILENO);
            ::close(_inpair[1]);
        }
        if (_outpair[0] != PIPE_DISABLED) {
            ::close(_outpair[0]);
            ::dup2(_outpair[1], STDOUT_FILENO);
        }
        if (_errpair[0] != PIPE_DISABLED) {
            ::close(_errpair[0]);
            ::dup2(_errpair[1], STDERR_FILENO);
        }

        auto argv = _mk_argv(_cxx_argv);
        if (!argv) {
            // need to exit from the child gracefully
            exit(ENOMEM);
        }

        ::execvp(argv[0], argv);
        // We can get here only if execvp failed
        exit(errno);

    }
    // we are in parent
    _state = SubProcessState::RUNNING;
    ::close(_inpair[0]);
    ::close(_outpair[1]);
    ::close(_errpair[1]);
    // update a state
    poll();
    return true;
}

int SubProcess::wait(bool no_hangup)
{
    //thanks tomas for the fix!
    int status = -1;

    int options = no_hangup ? WNOHANG : 0;

    if (_state != SubProcessState::RUNNING) {
        return _return_code;
    }

    int ret = ::waitpid(getPid(), &status, options);
    if (no_hangup && ret == 0) {
        // state did not change here
        return _return_code;
    }

    if (WIFEXITED(status)) {
        _state = SubProcessState::FINISHED;
        _return_code = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status)) {
        _state = SubProcessState::FINISHED;
        _return_code = - WTERMSIG(status);

        if (WCOREDUMP(status)) {
            _core_dumped = true;
        }
    }
    // we don't allow wait on SIGSTOP/SIGCONT, so WIFSTOPPED/WIFCONTINUED
    // were omited here

    return _return_code;
}

int SubProcess::wait(unsigned int timeout)
{
    while( true ) {
        poll();
        if (_state != SubProcessState::RUNNING) {
            return _return_code;
        }
        if( ! timeout ) {
            return _return_code;
        }
        sleep(1);
        --timeout;
    }
}

int SubProcess::kill(int signal) {
    auto ret = ::kill(getPid(), signal);
    poll();
    return ret;
}

int SubProcess::terminate() {
    auto ret = kill(SIGKILL);
    wait();
    return ret;
}

const char* SubProcess::state() const {
    if (_state == SubProcess::SubProcessState::NOT_STARTED) {
        return "not-started";
    }
    else if (_state == SubProcess::SubProcessState::RUNNING) {
        return "running";
    }
    else if (_state == SubProcess::SubProcessState::FINISHED) {
        return "finished";
    }

    return "unimplemented state";
}

std::string read_all(int fd) {
    char buf[BUF_SIZE+1];
    ssize_t r;

    std::stringbuf sbuf;

    while (true) {
        memset(buf, '\0', BUF_SIZE+1);
        r = ::read(fd, buf, BUF_SIZE);
        //TODO what to do if errno != EAGAIN | EWOULDBLOCK
        if (r <= 0) {
            break;
        }
        sbuf.sputn(buf, strlen(buf));
    }
    return sbuf.str();
}

std::string wait_read_all(int fd) {
    char buf[BUF_SIZE+1];
    ssize_t r;
    int exit = 0;

    int o_flags = fcntl(fd, F_GETFL);
    int n_flags = o_flags | O_NONBLOCK;
    fcntl(fd, F_SETFL, n_flags);

    std::stringbuf sbuf;
    memset(buf, '\0', BUF_SIZE+1);
    errno = 0;
    while (::read(fd, buf, BUF_SIZE) <= 0 &&
           (errno == EAGAIN || errno == EWOULDBLOCK) && exit < 5000) {
        usleep(1000);
        errno = 0;
        exit++;
    }

    sbuf.sputn(buf, strlen(buf));

    exit = 0;
    while (true) {
        memset(buf, '\0', BUF_SIZE+1);
        errno = 0;
        r = ::read(fd, buf, BUF_SIZE);
        if (r <= 0) {
            if(exit > 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
                break;
            usleep(1000);
            exit = 1;
        } else {
            exit = 0;
        }
        sbuf.sputn(buf, strlen(buf));
    }
    fcntl(fd, F_SETFL, o_flags);
    return sbuf.str();
}

int call(const Argv& args) {
    SubProcess p(args);
    p.run();
    return p.wait();
}

int output(const Argv& args, std::string& o, std::string& e, unsigned int timeout) {

    SubProcess p(args, SubProcess::STDOUT_PIPE | SubProcess::STDERR_PIPE);
    p.run();

    unsigned int tme = 0;
    if(timeout == 0)
        timeout = 5;
    int ret = 0;

    std::string out;
    std::string err;

    while((tme < timeout) && p.isRunning()) {
        ret = p.wait((unsigned int)1);
        out += wait_read_all(p.getStdout());
        err += wait_read_all(p.getStderr());
        tme++;
    }
    if( p.isRunning() ) {
        p.terminate();
        ret = p.wait();
    }
    else {
        ret = p.poll ();
    }
    out += wait_read_all(p.getStdout());
    err += wait_read_all(p.getStderr());

    o.assign(out);
    e.assign(err);
    return ret;
}

int output(const Argv& args, std::string& o, std::string& e, const std::string& i, unsigned int timeout) {
    SubProcess p(args, SubProcess::STDOUT_PIPE | SubProcess::STDERR_PIPE| SubProcess::STDIN_PIPE);
    p.run();
    int r = ::write(p.getStdin(), i.c_str(), i.size());
    if (r == -1) {
        zsys_error ("Can't write %zu to stdin, broken pipe: %s", i.size (), strerror (errno));
        p.terminate ();
        zclock_sleep (2000);
        p.kill (SIGKILL);
        return -1;
    }
    ::fsync(p.getStdin());
    ::close(p.getStdin());

    int ret;
    if( timeout ) {
        ret = p.wait(timeout);
        if( p.isRunning() ) { p.terminate(); ret = p.wait(); }
    } else {
        ret = p.wait();
    }

    o.assign(read_all(p.getStdout()));
    e.assign(read_all(p.getStderr()));
    return ret;
}

// ### helper functions ###
char * const * _mk_argv(const Argv& vec) {

    char ** argv = (char **) malloc(sizeof(char*) * (vec.size()+1));
    assert(argv);

    for (auto i=0u; i != vec.size(); i++) {

        auto str = vec[i];
        char* dest = (char*) malloc(sizeof(char) * (str.size() + 1));
        memcpy(dest, str.c_str(), str.size());
        dest[str.size()] = '\0';

        argv[i] = dest;
    }
    argv[vec.size()] = NULL;
    return (char * const*)argv;
}

void _free_argv(char * const * argv) {
    char *foo;
    std::size_t n;

    n = 0;
    while((foo = argv[n]) != NULL) {
        free(foo);
        n++;
    }
    free((void*)argv);
}

std::size_t _argv_hash(Argv args) {


    std::hash<std::string> hash;
    size_t ret = hash("");

    for (auto str : args) {
        size_t foo = hash(str);
        ret = ret ^ (foo << 1);
    }

    return ret;
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
subprocess_test (bool verbose)
{
    printf (" * subprocess: ");

    //  @selftest
    //  Simple create/destroy test
    //  @end
    printf ("OK\n");
}

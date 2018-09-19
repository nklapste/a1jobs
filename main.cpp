#include <iostream>
#include <sys/resource.h>
#include <sys/times.h>
#include <sstream>
#include <iterator>
#include <vector>
#include <zconf.h>

int main() {

    rlimit rilimit{};
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);

    tms tms{};
    times(&tms);
    std::cout << tms.tms_cutime << std::endl;
    std::cout << tms.tms_stime << std::endl;
    std::cout << tms.tms_cstime << std::endl;
    std::cout << tms.tms_utime << std::endl;

    pid_t pid = getpid();
    std::cout << pid << std::endl;

    for (;;){
        char *cmd;
        printf("a1jobs[%d]: ", pid);
        std::cin >> cmd;
        std::cout << std::endl;

        std::istringstream iss(cmd);
        // parse the command into space separated tokens
        std::vector<std::string> tokens;
        copy(std::istream_iterator<std::string>(iss),
             std::istream_iterator<std::string>(),
             back_inserter(tokens));


        if (tokens.at(0) == "list"){
            // TODO
        }

        if (tokens.at(0) == "run"){
            // TODO
        }

        if (tokens.at(0) == "suspend"){
            // TODO
        }

        if (tokens.at(0) == "resume"){
            // TODO
        }

        if (tokens.at(0) == "terminate"){
            // TODO
        }

        if (tokens.at(0) == "exit"){
            // TODO
        }

        if (tokens.at(0) == "quit"){
            break;
        }
    }

    return 0;
}
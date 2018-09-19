#include <iostream>
#include <sys/resource.h>
#include <sys/times.h>
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

        if (cmd == "list"){
            // TODO
        }

        if (cmd == "run"){
            // TODO
        }

        if (cmd == "suspend"){
            // TODO
        }

        if (cmd == "resume"){
            // TODO
        }

        if (cmd == "terminate"){
            // TODO
        }

        if (cmd == "exit"){
            // TODO
        }

        if (cmd == "quit"){
            break;
        }
    }

    return 0;
}
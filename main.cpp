#include <iostream>
#include <sys/resource.h>
#include <sys/times.h>
#include <sstream>
#include <iterator>
#include <vector>
#include <zconf.h>
#include <signal.h>
#include <stdlib.h>
#include <tuple>
#include <algorithm>


static const int MAXJOBS = 32;


/**
 * Set the cpu time limit to 10 mins for safe development.
 */
void set_cpu_safety() {
    rlimit rilimit{};
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);
}


int main() {
    set_cpu_safety();

    std::vector<std::tuple<int, pid_t, std::string> > jobs;
    int job_idx = 0;
    tms tms{};
    times(&tms);
    std::cout << tms.tms_cutime << std::endl;
    std::cout << tms.tms_stime << std::endl;
    std::cout << tms.tms_cstime << std::endl;
    std::cout << tms.tms_utime << std::endl;

    pid_t pid = getpid();
    std::cout << pid << std::endl;

    // main command event loop
    for (;;) {
        std::string cmd;

        // get the current command line
        std::cin.clear();
        printf("a1jobs[%d]: ", pid);
        std::getline(std::cin, cmd);

        // parse the command into space separated tokens
        std::istringstream iss(cmd);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>{}};

        if (tokens.empty()) {
            std::cout << "ERROR: Missing command" << std::endl;
        } else if (tokens.at(0) == "list") {
            for(std::tuple<int, pid_t, std::string> job: jobs){
                printf("%d: (pid=%6d, cmd= %s)\n",std::get<0>(job), std::get<1>(job), std::get<2>(job).c_str());
            }
        } else if (tokens.at(0) == "run") {
            if (job_idx < MAXJOBS) {
                pid_t c_pid = fork();
                errno = 0;

                if (c_pid == 0) {
                    switch (tokens.size()) {
                        case 1:
                            std::cout << "ERROR: Missing args for run" << std::endl;
                            break;
                        case 2:
                            execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), (char *) nullptr);
                            break;
                        case 3:
                            execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(), (char *) nullptr);
                            break;
                        case 4:
                            execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(),
                                   tokens.at(3).c_str(), (char *) nullptr);
                            break;
                        case 5:
                            execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(),
                                   tokens.at(3).c_str(), tokens.at(4).c_str(), (char *) nullptr);
                            break;
                        default:
                            std::cout << "ERROR: Too many args for run" << std::endl;
                            break;
                    }
                    return 0;
                }
                if (errno) {
                    std::cout << "ERROR: Running command" << std::endl;
                    errno = 0;
                } else {
                    // concat the cmd vector into a single string
                    std::ostringstream cmd_str;
                    std::copy(tokens.begin() + 1, tokens.end() - 1,
                              std::ostream_iterator<std::string>(cmd_str, " "));
                    cmd_str << tokens.back();
                    std::cout << cmd_str.str() << std::endl;

                    // append the job to the jobs list
                    jobs.emplace_back(job_idx, c_pid, cmd_str.str());
                    printf("Successfully executed command: %d: (pid=%6d, cmd= %s)\n",job_idx, c_pid, cmd_str.str().c_str());
                    job_idx++;
                }
            } else {
                std::cout << "ERROR: Too many jobs already initiated" << std::endl;
            }
        } else if (tokens.at(0) == "suspend") {
            pid_t jobNo = std::stoi(tokens.at(1), nullptr, 10);
            auto it = std::find_if(jobs.begin(), jobs.end(), [&jobNo](const std::tuple<int, pid_t, std::string>& job) {return std::get<1>(job) == jobNo;});
            if (it != jobs.end()) {
                printf("found job: %d suspending\n", jobNo);
                kill(jobNo, SIGSTOP);
            } else {
                printf("ERROR: failed to find job: %d  not suspending\n", jobNo);
            }
        } else if (tokens.at(0) == "resume") {
            pid_t jobNo = std::stoi(tokens.at(1), nullptr, 10);
            auto it = std::find_if(jobs.begin(), jobs.end(), [&jobNo](const std::tuple<int, pid_t, std::string>& job) {return std::get<1>(job) == jobNo;});
            if (it != jobs.end()) {
                printf("found job: %d resuming\n", jobNo);
                kill(jobNo, SIGCONT);
            } else {
                printf("ERROR: failed to find job: %d  not resuming\n", jobNo);
            }
        } else if (tokens.at(0) == "terminate") {
            pid_t jobNo = std::stoi(tokens.at(1), nullptr, 10);
            auto it = std::find_if(jobs.begin(), jobs.end(), [&jobNo](const std::tuple<int, pid_t, std::string>& job) {return std::get<1>(job) == jobNo;});
            if (it != jobs.end()) {
                printf("found job: %d terminating\n", jobNo);
                jobs.erase(it);
                kill(jobNo, SIGKILL);
            } else {
                printf("ERROR: failed to find job: %d  not terminating\n", jobNo);
            }
        } else if (tokens.at(0) == "exit") {
            for(std::tuple<int, pid_t, std::string> job: jobs){
                printf("terminating job: %d\n", std::get<1>(job));
                kill(std::get<1>(job), SIGKILL);
            }
            break;
        } else if (tokens.at(0) == "quit") {
            printf("WARNING: Exiting a1jobs without terminating head processes\n");
            break;
        } else {
            std::ostringstream cmd_str;
            std::copy(tokens.begin() + 1, tokens.end() - 1,
                  std::ostream_iterator<std::string>(cmd_str, " "));
            cmd_str << tokens.back();

            // append the job to the jobs list
            printf("ERROR: Invalid command: %s", cmd_str.str().c_str());
        }
    }
    return 0;
}

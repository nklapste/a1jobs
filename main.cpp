/**
 * a1jobs
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <iostream>
#include <sys/resource.h>
#include <sys/times.h>
#include <sstream>
#include <iterator>
#include <vector>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <tuple>
#include <algorithm>

static const int MAXJOBS = 32;


typedef std::tuple<uint, pid_t, std::string> job;
typedef std::vector<job> job_list;


/**
 * Get and print details on the a1jobs process.
 *
 * @return {@code pid_t} the pid of the a1jobs process.
 */
pid_t getA1jobsDetails() {
    tms tms{};
    times(&tms);
    pid_t pid = getpid();

    std::cout << "cutime: " << tms.tms_cutime << std::endl;
    std::cout << "stime:  " << tms.tms_stime << std::endl;
    std::cout << "cstime: " << tms.tms_cstime << std::endl;
    std::cout << "utime:  " << tms.tms_utime << std::endl;
    std::cout << "pid:    " << pid << std::endl;
    return pid;
}


/**
 * Print a error message when a unsupported command is given to a1jobs.
 *
 * @param tokens
 */
void invalidCommand(std::vector<std::string> &tokens) {
    std::ostringstream cmd_str;
    copy(tokens.begin() + 1, tokens.end() - 1,
         std::ostream_iterator<std::__cxx11::string>(cmd_str, " "));
    cmd_str << tokens.back();
    printf("ERROR: Invalid command: %s\n", cmd_str.str().c_str());
}


/**
 * Set the cpu time limit to 10 minutes for safe development.
 */
void set_cpu_safety() {
    rlimit rilimit{};
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);
}


int main() {
    set_cpu_safety();
    job_list jobs;
    uint job_idx = 0;
    pid_t pid = getA1jobsDetails();

    // main command event loop
    for (;;) {
        std::string cmd;

        // get the current command line
        std::cin.clear();
        printf("a1jobs[%u]: ", pid);
        std::getline(std::cin, cmd);

        // parse the command into space separated tokens
        std::istringstream iss(cmd);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

        if (tokens.empty()) {
            printf("ERROR: Missing command\n");
        } else if (tokens.at(0) == "list") {
            for(job job: jobs){
                printf("%u: (pid=%6u, cmd= %s)\n", std::get<0>(job), std::get<1>(job), std::get<2>(job).c_str());
            }
        } else if (tokens.at(0) == "run") {
            if (job_idx < MAXJOBS) {
                errno = 0;
                if (tokens.size()==1){
                    printf("ERROR: Missing arguments\n");
                    errno = 1;
                }else if (tokens.size()>5){
                    printf("ERROR: Too many args for run\n");
                    errno = 1;
                }
                pid_t c_pid = fork();
                if (c_pid == 0) {
                    switch (tokens.size()) {
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
                            break;
                    }
                    return 0;
                }
                if (errno) {
                    printf("ERROR: Running command\n");
                    errno = 0;
                } else {
                    // concat the cmd vector into a single string
                    std::ostringstream cmd_str;
                    std::copy(tokens.begin() + 1, tokens.end() - 1,
                              std::ostream_iterator<std::string>(cmd_str, " "));
                    cmd_str << tokens.back();

                    // append the job to the jobs list
                    jobs.emplace_back(job_idx, c_pid, cmd_str.str());
                    printf("Successfully executed command: %u: (pid=%6u, cmd= %s)\n",job_idx, c_pid, cmd_str.str().c_str());
                    job_idx++;
                }
            } else {
                std::cout << "ERROR: Too many jobs already initiated" << std::endl;
            }
        } else if (tokens.at(0) == "suspend") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 10);
            auto it = std::find_if(jobs.begin(), jobs.end(), [&jobNo](const job& job) {return std::get<0>(job) == jobNo;});
            if (it != jobs.end()) {
                pid_t sus_pid = std::get<1>(*it);
                printf("found job: %u suspending\n", sus_pid);
                kill(sus_pid, SIGSTOP);
            } else {
                printf("ERROR: failed to find job: %u  not suspending\n", jobNo);
            }
        } else if (tokens.at(0) == "resume") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 10);
            auto it = std::find_if(jobs.begin(), jobs.end(), [&jobNo](const job& job) {return std::get<0>(job) == jobNo;});
            if (it != jobs.end()) {
                pid_t res_pid = std::get<1>(*it);
                printf("found job: %u resuming\n", res_pid);
                kill(res_pid, SIGCONT);
            } else {
                printf("ERROR: failed to find job: %u  not resuming\n", jobNo);
            }
        } else if (tokens.at(0) == "terminate") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 10);
            auto it = std::find_if(jobs.begin(), jobs.end(), [&jobNo](const job& job) {return std::get<0>(job) == jobNo;});
            if (it != jobs.end()) {
                pid_t term_pid = std::get<1>(*it);
                printf("found job: %u terminating\n", term_pid);
                jobs.erase(it);
                kill(term_pid, SIGKILL);
                job_idx--;
            } else {
                printf("ERROR: failed to find job: %u  not terminating\n", jobNo);
            }
        } else if (tokens.at(0) == "exit") {
            for(job job: jobs){
                printf("terminating job: %u\n", std::get<1>(job));
                kill(std::get<1>(job), SIGKILL);
            }
            printf("exiting a1jobs\n");
            break;
        } else if (tokens.at(0) == "quit") {
            printf("WARNING: Exiting a1jobs without terminating head processes\n");
            printf("exiting a1jobs\n");
            break;
        } else {
            invalidCommand(tokens);
        }
    }
    return 0;
}


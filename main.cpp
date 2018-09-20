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


void suspend_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, std::vector<std::string> &tokens);

void resume_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, std::vector<std::string> &tokens);

void exit(const std::vector<std::tuple<int, pid_t, std::string>> &jobs);

void terminate_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, std::vector<std::string> &tokens);

void list_jobs(const std::vector<std::tuple<int, pid_t, std::string>> &jobs);

void invalid_command(std::vector<std::string> &tokens);

void run_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, int job_idx, std::vector<std::string> &tokens);

int main() {
    rlimit rilimit{};
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);
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
            list_jobs(jobs);
        } else if (tokens.at(0) == "run") {
            run_job(jobs, job_idx, tokens);
        } else if (tokens.at(0) == "suspend") {
            suspend_job(jobs, tokens);
        } else if (tokens.at(0) == "resume") {
            resume_job(jobs, tokens);
        } else if (tokens.at(0) == "terminate") {
            terminate_job(jobs, tokens);
        } else if (tokens.at(0) == "exit") {
            exit(jobs);
            break;
        } else if (tokens.at(0) == "quit") {
            printf("WARNING: Exiting a1jobs without terminating head processes\n");
            break;
        } else {
            invalid_command(tokens);
        }
    }

    return 0;
}

void run_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, int job_idx, std::vector<std::string> &tokens) {
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
                    exit(0);
                }
                if (errno) {
                    std::cout << "ERROR: Running command" << std::endl;
                    errno = 0;
                } else {
                    // concat the cmd vector into a single string
                    std::ostringstream cmd_str;
                    copy(tokens.begin() + 1, tokens.end() - 1,
                              std::ostream_iterator<std::__cxx11::string>(cmd_str, " "));
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
}

void invalid_command(std::vector<std::string> &tokens) {
    std::ostringstream cmd_str;
    copy(tokens.begin() + 1, tokens.end() - 1,
                      std::ostream_iterator<std::__cxx11::string>(cmd_str, " "));
    cmd_str << tokens.back();
    // append the job to the jobs list
    printf("ERROR: Invalid command: %s", cmd_str.str().c_str());
}

void list_jobs(const std::vector<std::tuple<int, pid_t, std::string>> &jobs) {
    for(std::tuple<int, pid_t, std::__cxx11::string> job: jobs){
                printf("%d: (pid=%6d, cmd= %s)\n", std::get<0>(job), std::get<1>(job), std::get<2>(job).c_str());
            }
}

void terminate_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, std::vector<std::string> &tokens) {
    pid_t jobNo = stoi(tokens.at(1), nullptr, 10);
    std::cout << jobNo << std::endl;
    auto it = find_if(jobs.begin(), jobs.end(), [&jobNo](const std::tuple<int, pid_t, std::__cxx11::string>& job) {return std::get<1>(job) == jobNo;});
    if (it != jobs.end()) {
                printf("found job: %d terminating\n", jobNo);
                jobs.erase(it);
                kill(jobNo, SIGKILL);
            } else {
                printf("ERROR: failed to find job: %d  not terminating\n", jobNo);
            }
}

void exit(const std::vector<std::tuple<int, pid_t, std::string>> &jobs) {
    for(std::tuple<int, pid_t, std::__cxx11::string> job: jobs){
                printf("terminating job: %d\n", std::get<1>(job));
                kill(std::get<1>(job), SIGKILL);
            }
}

void resume_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, std::vector<std::string> &tokens) {
    int jobNo = stoi(tokens.at(1), nullptr, 10);
    auto it = find_if(jobs.begin(), jobs.end(), [&jobNo](const std::tuple<int, pid_t, std::__cxx11::string>& job) {return std::get<1>(job) == jobNo;});
    if (it != jobs.end()) {
                printf("found job: %d resuming\n", jobNo);
                kill(jobNo, SIGCONT);
            } else {
                printf("ERROR: failed to find job: %d  not resuming\n", jobNo);
            }
}

void suspend_job(std::vector<std::tuple<int, pid_t, std::string>> &jobs, std::vector<std::string> &tokens) {
    int jobNo = stoi(tokens.at(1), nullptr, 10);
    auto it = find_if(jobs.begin(), jobs.end(), [&jobNo](const std::tuple<int, pid_t, std::__cxx11::string>& job) {return std::get<1>(job) == jobNo;});
    if (it != jobs.end()) {
                printf("found job: %d suspending\n", jobNo);
                kill(jobNo, SIGSTOP);
            } else {
                printf("ERROR: failed to find job: %d  not suspending\n", jobNo);
            }
}
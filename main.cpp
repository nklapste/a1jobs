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


static const int MAXJOBS = 32;


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
        printf("a1jobs[%d]: ", pid);

        // get the current command line
        // todo: weird behavior with whitespace
        std::getline(std::cin >> std::ws, cmd);

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
                } else {
                    // concat the cmd vector into a single string
                    std::ostringstream cmd_str;
                    std::copy(tokens.begin() + 1, tokens.end() - 1,
                              std::ostream_iterator<std::string>(cmd_str, " "));
                    cmd_str << tokens.back();
                    std::cout << cmd_str.str() << std::endl;
                    // append the job to the jobs list
                    jobs.push_back(std::make_tuple(job_idx, c_pid, cmd_str.str()));
                    job_idx++;
                }
                // TODO: running a command that prints to stdout seems to interfere with a1jobs input
            } else {
                std::cout << "ERROR: Too many jobs already initiated" << std::endl;
            }
        } else if (tokens.at(0) == "suspend") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 100);
            kill(jobNo, SIGSTOP);
            // TODO:
        } else if (tokens.at(0) == "resume") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 100);
            kill(jobNo, SIGCONT);
            // TODO:
        } else if (tokens.at(0) == "terminate") {
//            TODO: use std::find_if
//            auto it = std::find_if(v.begin(), v.end(), [](const std::tuple<int,int,int,int>& e) {return std::get<0>(e) == 0;});

            int jobNo = std::stoi(tokens.at(1), nullptr, 100);
            for(std::vector<int>::size_type i = 0; i != jobs.size(); i++) {
                if(std::get<1>(jobs[i])==jobNo){
                    jobs.erase(jobs.begin()+i-1);
                }
            }
            kill(jobNo, SIGKILL);
            // TODO:
        } else if (tokens.at(0) == "exit") {
            break;
        } else if (tokens.at(0) == "quit") {
            // TODO:
            break;
        } else {
            std::cout << "ERROR: Invalid command: " + tokens.at(1) << std::endl;
        }
    }

    return 0;
}
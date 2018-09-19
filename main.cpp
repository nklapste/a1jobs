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

typedef std::vector< std::tuple<int, pid_t, std::string> > jobs_list;

int main() {
    rlimit rilimit{};
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);
    jobs_list jobs;
    int job_idx = 0;

    tms tms{};
    times(&tms);
    std::cout << tms.tms_cutime << std::endl;
    std::cout << tms.tms_stime << std::endl;
    std::cout << tms.tms_cstime << std::endl;
    std::cout << tms.tms_utime << std::endl;

    pid_t pid = getpid();
    std::cout << pid << std::endl;

    for (;;){
        std::string cmd;
        printf("a1jobs[%d]: ", pid);

        // get the current command line
        // todo: weird behavoir with whitespace
        std::getline(std::cin >> std::ws, cmd);

        // parse the command into space separated tokens
        std::istringstream iss(cmd);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>{}};
        if (tokens.empty()){
            std::cout << "ERROR: Missing command" << std::endl;
        } else if (tokens.at(0) == "list"){
           // TODO
        } else if (tokens.at(0) == "run"){
            pid_t c_pid = fork();
            errno = 0;
            if (c_pid==0) {
                switch(tokens.size()){
                    case 2:
                        execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), (char *) 0);
                        break;
                    case 3:
                        execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(),  (char *) 0);
                        break;
                    case 4:
                        execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(), tokens.at(3).c_str(), (char *) 0);
                        break;
                    case 5:
                        execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(), tokens.at(3).c_str(), tokens.at(4).c_str(), (char *) 0);
                        break;
                    default:
                        std::cout << "ERROR: Too many args for run" << std::endl;
                        break;
                }
                if (errno){
                    std::cout << "ERROR: Running command" << std::endl;
                } else {
                    // concat the cmd vector into a single string
                    std::ostringstream oss;
                    std::copy(tokens.begin()+1, tokens.end()-1,
                              std::ostream_iterator<std::string>(oss, ","));
                    oss << tokens.back();
                    std::cout << oss.str() << std::endl;

                    // append the job to the jobs list
                    jobs.push_back(std::make_tuple(job_idx, c_pid, oss.str()));
                    job_idx++;
                }
                return 0;
            }
            // TODO:
        } else if (tokens.at(0) == "suspend"){
            int jobNo = std::stoi(tokens.at(1), nullptr, 100);
            kill(jobNo, SIGSTOP);
        } else if (tokens.at(0) == "resume"){
            int jobNo = std::stoi(tokens.at(1), nullptr, 100);
            kill(jobNo, SIGCONT);
        } else if (tokens.at(0) == "terminate"){
            int jobNo = std::stoi(tokens.at(1), nullptr, 100);
            kill(jobNo, SIGKILL);
            // TODO:
        } else if (tokens.at(0) == "exit"){
            break;
        } else if (tokens.at(0) == "quit"){
            // TODO:
            break;
        } else {
            std::cout << "ERROR: Invalid command: "+tokens.at(1) << std::endl;
        }
    }

    return 0;
}
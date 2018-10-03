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
    // TODO: implemented wrong redo
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
 * Print out the job_list of head processes.
 *
 * If the list of head processes is empty nothing is printed.
 *
 * @param jobs the list of head processes.
 */
void listJobs(const job_list &jobs) {
    uint jobIdx = 0;
    for(job job: jobs){
        printf("%u: (pid=%6u, cmd= %s)\n", jobIdx, std::get<1>(job), std::get<2>(job).c_str());
        jobIdx++;
    }
}


/**
 * If we are not already at the max number of jobs fork and create a new head process and
 * running the command specified.
 *
 * @param jobs the list of head processes..
 * @param tokens vector of the string command line arguements given.
 */
void runJob(job_list &jobs, std::vector<std::string> &tokens) {
    if (jobs.size() < MAXJOBS) {
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
//                    return 0;
        }
        if (errno) {
            printf("ERROR: Running command\n");
            errno = 0;
        } else {
            // concat the cmd vector into a single string
            std::ostringstream cmd_str;
            copy(tokens.begin() + 1, tokens.end() - 1,
                 std::ostream_iterator<std::__cxx11::string>(cmd_str, " "));
            cmd_str << tokens.back();

            // append the job to the jobs list
            jobs.emplace_back(0, c_pid, cmd_str.str());
            printf("Successfully executed command: %lu: (pid=%6u, cmd= %s)\n",jobs.size(), c_pid, cmd_str.str().c_str());
        }
    } else {
        std::cout << "ERROR: Too many jobs already initiated" << std::endl;
    }
}


/**
 * Send the SIGSTOP signal to a Head process. Thus, suspending the head process.
 *
 * @param jobs the list of head processes.
 * @param jobNo the head process number to stop.
 */
void suspendJob(job_list &jobs, int jobNo) {
    auto it = find_if(jobs.begin(), jobs.end(), [&jobNo](const job& job) {return std::get<0>(job) == jobNo;});
    if (it != jobs.end()) {
        pid_t sus_pid = std::get<1>(*it);
        printf("found job: %u suspending\n", sus_pid);
        kill(sus_pid, SIGSTOP);
    } else {
        printf("ERROR: failed to find job: %u  not suspending\n", jobNo);
    }
}


/**
 * Send the SIGCONT signal to a Head process. Thus, resuming the head process if it was stopped.
 *
 * @param jobs the list of head processes.
 * @param jobNo the head process number to stop.
 */
void resumeJob(job_list &jobs, int jobNo) {
    auto it = find_if(jobs.begin(), jobs.end(), [&jobNo](const job& job) {return std::get<0>(job) == jobNo;});
    if (it != jobs.end()) {
        pid_t res_pid = std::get<1>(*it);
        printf("found job: %u resuming\n", res_pid);
        kill(res_pid, SIGCONT);
    } else {
        printf("ERROR: failed to find job: %u  not resuming\n", jobNo);
    }
}


/**
 * Sent the SIGKILL signal to a Head process. Thus terminating the head process.
 *
 * @param jobs the list of head processes.
 * @param job_idx number of jobs in the list of head processes.
 * @param jobNo the head process number to stop.
 * @return
 */
void terminateJob(job_list &jobs, uint jobNo) {
    if (jobNo < jobs.size()){
        pid_t term_pid = std::get<1>(jobs.at(jobNo));
        printf("found job: %u terminating\n", term_pid);
        kill(term_pid, SIGKILL);
        jobs.erase(jobs.begin() + jobNo);
    } else {
        printf("ERROR: failed to find job: %u  not terminating\n", jobNo);
    }
}


/**
 * Terminate the a1jobs process.
 *
 * Before termination however terminate the head process to every head process still in job_list.
 *
 * @param jobs the list of head processes to terminate before exiting a1jobs.
 */
void exitA1jobs(const job_list &jobs) {
    for(job job: jobs){
        printf("terminating job: %u\n", std::get<1>(job));
        kill(std::get<1>(job), SIGKILL);
    }
    printf("exiting a1jobs\n");
}


/**
 * Terminate the a1jobs process without terminating the head processes.
 */
void quitA1jobs() {
    printf("WARNING: Exiting a1jobs without terminating head processes\n");
    printf("exiting a1jobs\n");
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
            listJobs(jobs);
        } else if (tokens.at(0) == "run") {
            runJob(jobs, tokens);
        } else if (tokens.at(0) == "suspend") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 10);
            suspendJob(jobs, jobNo);
        } else if (tokens.at(0) == "resume") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 10);
            resumeJob(jobs, jobNo);
        } else if (tokens.at(0) == "terminate") {
            int jobNo = std::stoi(tokens.at(1), nullptr, 10);
            terminateJob(jobs, jobNo);
        } else if (tokens.at(0) == "exit") {
            exitA1jobs(jobs);
            break;
        } else if (tokens.at(0) == "quit") {
            quitA1jobs();
            break;
        } else {
            invalidCommand(tokens);
        }
    }
    return 0;
}

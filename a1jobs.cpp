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
#include <fcntl.h>

static const int MAX_JOBS = 32;


typedef std::tuple<pid_t, std::string> job;
typedef std::vector<job> jobList;


/**
 * Get the Job number from a vector of a1jobs command tokens.
 *
 * @param tokens vector of a1jobs command arguements.
 * @return a Job number.
 */
uint getJobNo(std::vector<std::string> &tokens) {
    uint jobNo = static_cast<uint>(stoi(tokens.at(1), nullptr, 10));
    return jobNo;
}


/**
 * Print out the job_list of head processes.
 *
 * If the list of head processes is empty nothing is printed.
 *
 * @param jobs the list of head processes.
 */
void listJobs(const jobList &jobs) {
    uint jobIdx = 0;
    for(job job: jobs){
        printf("%u: (pid=%6u, cmd= %s)\n", jobIdx, std::get<0>(job), std::get<1>(job).c_str());
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
void runJob(jobList &jobs, std::vector<std::string> &tokens) {
    if (tokens.size()==1){
        printf("ERROR: missing arguments\n");
    }else if (tokens.size()>6){
        printf("ERROR: too many args for run\n");
    } else if (jobs.size() >= MAX_JOBS) {
        printf("ERROR: too many jobs already initiated\n");
    } else {
        errno = 0;

        pid_t childPID = fork();
        if (childPID == -1){
            printf("ERROR: failed to fork\n");
            errno = 1;
        } else if (childPID == 0) {
            int fd = open("/dev/null",O_WRONLY | O_CREAT, 0666);   // open the file /dev/null
            dup2(fd, 1);
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
                case 6:
                    execlp(tokens.at(1).c_str(), tokens.at(1).c_str(), tokens.at(2).c_str(),
                           tokens.at(3).c_str(), tokens.at(4).c_str(), tokens.at(5).c_str(), (char *) nullptr);
                    break;
                default:
                    break;
            }
            close(fd);
            exit(0);
        }
        if (errno) {
            printf("ERROR: failed running command\n");
            errno = 0;
        } else {
            // concat the cmd vector into a single string
            std::ostringstream cmdStr;
            copy(tokens.begin() + 1, tokens.end() - 1,
                 std::ostream_iterator<std::string>(cmdStr, " "));
            cmdStr << tokens.back();

            // append the new head process to the jobs list
            jobs.emplace_back(childPID, cmdStr.str());
            printf("Successfully executed command: %lu: (pid=%6u, cmd= %s)\n", jobs.size()-1, childPID, cmdStr.str().c_str());
        }
    }
}


/**
 * Send the SIGSTOP signal to a Head process. Thus, suspending the head process.
 *
 * @param jobs the list of head processes.
 * @param jobNo the head process number to stop.
 */
void suspendJob(jobList &jobs, uint jobNo) {
    if (jobNo < jobs.size()){
        pid_t jobPID = std::get<0>(jobs.at(jobNo));
        printf("found job: %u suspending\n", jobPID);
        kill(jobPID, SIGSTOP);
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
void resumeJob(jobList &jobs, uint jobNo) {
    if (jobNo < jobs.size()){
        pid_t jobPID = std::get<0>(jobs.at(jobNo));
        printf("found job: %u resuming\n", jobPID);
        kill(jobPID, SIGCONT);
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
void terminateJob(jobList &jobs, uint jobNo, jobList &termJobs) {
    if (jobNo < jobs.size()){
        if(std::find(termJobs.begin(), termJobs.end(), jobs.at(jobNo)) != termJobs.end()) {
            /* v contains x */
        } else {
            /* v does not contain x */
            pid_t jobPID = std::get<0>(jobs.at(jobNo));
            printf("found job: %u terminating\n", jobPID);
            kill(jobPID, SIGKILL);
            termJobs.push_back(jobs.at(jobNo));
        }
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
void exitA1jobs(const jobList &jobs, jobList &termJobs) {
    for(job job: jobs){
        if(std::find(termJobs.begin(), termJobs.end(), job) != termJobs.end()) {
            /* v contains x */
        } else {
            /* v does not contain x */
            pid_t jobPID = std::get<0>(job);
            printf("terminating job: %u\n", jobPID);
            kill(jobPID, SIGKILL);
        }
    }
    printf("exiting a1jobs\n");
}


/**
 * Terminate the a1jobs process without terminating the head processes.
 */
void quitA1jobs() {
    printf("WARNING: exiting a1jobs without terminating head processes\n");
    printf("exiting a1jobs\n");
}


/**
 * Print a error message when a unsupported command is given to a1jobs.
 *
 * @param tokens
 */
void invalidCommand(std::vector<std::string> &tokens) {
    std::ostringstream cmdStr;
    copy(tokens.begin(), tokens.end() - 1,
         std::ostream_iterator<std::__cxx11::string>(cmdStr, " "));
    cmdStr << tokens.back();
    printf("ERROR: Invalid command: '%s'\n", cmdStr.str().c_str());
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
    jobList jobs;
    jobList termJobs;
    pid_t pid = getpid();

    // get the start cpu times for a1jobs
    tms startCPU{};
    static clock_t startTime = times(&startCPU);

    // main a1jobs command event loop
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
            suspendJob(jobs,  getJobNo(tokens));
        } else if (tokens.at(0) == "resume") {
            resumeJob(jobs, getJobNo(tokens));
        } else if (tokens.at(0) == "terminate") {
            terminateJob(jobs, getJobNo(tokens), termJobs);
        } else if (tokens.at(0) == "exit") {
            exitA1jobs(jobs, termJobs);
            break;
        } else if (tokens.at(0) == "quit") {
            quitA1jobs();
            break;
        } else {
            invalidCommand(tokens);
        }
    }

    // get the ending cpu times on the termination of a1jobs and compair to the start
    tms endCPU{};
    static clock_t endTime = times(&endCPU);
    printf("real:        %.2f sec.\n", (float)(endTime - startTime)/sysconf(_SC_CLK_TCK));
    printf("user:        %.2f sec.\n", (float)(endCPU.tms_utime - startCPU.tms_utime)/sysconf(_SC_CLK_TCK));
    printf("sys:         %.2f sec.\n", (float)(endCPU.tms_stime - startCPU.tms_stime)/sysconf(_SC_CLK_TCK));
    printf("child user:  %.2f sec.\n", (float)(endCPU.tms_cutime - startCPU.tms_cutime)/sysconf(_SC_CLK_TCK));
    printf("child sys:   %.2f sec.\n", (float)(endCPU.tms_cstime - startCPU.tms_cstime)/sysconf(_SC_CLK_TCK));

    return 0;
}


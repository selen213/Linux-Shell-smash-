#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>
#include<dirent.h>
#include <algorithm>
#include <sys/stat.h>
#include<sys/syscall.h>
#include <sys/types.h>
#include <dirent.h>




using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;
    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}



char* concatenateArgs(char* new_arg[COMMAND_MAX_ARGS]) {
    char* result = new char[COMMAND_MAX_LENGTH];
    result[0] = 0;
    // Concatenate all strings in new_arg
    for (int i = 0; i < COMMAND_MAX_ARGS && new_arg[i] != nullptr; ++i) {
        std::strcat(result, new_arg[i]);
        //std::cout<<result<<std::endl;

        // Add a space if not the last argumenta
        if (i < COMMAND_MAX_ARGS - 1 && new_arg[i + 1] != nullptr) {
             std::strcat(result, " ");
        }
    }

    return result;
}


// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell()
    : m_curr_prompt("smash"),
      m_cd_flag(false),
      m_last_dir(nullptr),
      m_smallshell_job_list(new JobsList()),
      m_foreground_pid(-1),
      m_curr_foregrount_cmd(""),
      m_allias_strings(){
    // TODO: add your implementation
}


SmallShell::~SmallShell() {
    // TODO: add your implementation
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
    m_smallshell_job_list->removeFinishedJobs(); 
    Command* cmd = CreateCommand(cmd_line);
    if(!cmd){
      return;
    }
    cmd->execute();
    m_foreground_pid = -1;
    m_curr_foregrount_cmd = "";
    delete cmd;
}


void printError(const std::string &message){
  cerr << "smash error: " << message << endl;
}


Command::Command(const char* cmd_line): m_cmd_line(cmd_line),m_args(new char*[COMMAND_MAX_ARGS]),m_arg_counter(0),m_is_background(false){
  m_arg_counter = _parseCommandLine(cmd_line, m_args);
  m_is_background = _isBackgroundComamnd(cmd_line);
}

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line){}

//Chprompt  command
Chprompt::Chprompt(const char* cmd_line):BuiltInCommand(cmd_line){}

void Chprompt::execute(){
  if(this->m_arg_counter > 1){
      SmallShell::getInstance().setPrompt(m_args[1]);
  }else{
      SmallShell::getInstance().setPrompt("smash");
  }
}


//Show pid command
ShowPidCommand::ShowPidCommand(const char* cmd_line):BuiltInCommand(cmd_line){}

void ShowPidCommand::execute(){
  std::cout << "smash pid is " << getpid() << std::endl;
}


//pwd command
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line):BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute() {
    char buffer[COMMAND_MAX_LENGTH];
    std::cout << getcwd(buffer, sizeof(buffer)) << std::endl;
}



//cd command
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char **plastPwd):BuiltInCommand(cmd_line),m_ptr_last_PWD(plastPwd){}

void ChangeDirCommand::execute() {
    char* curr_path = new char[COMMAND_MAX_LENGTH];
    getcwd(curr_path, COMMAND_MAX_LENGTH);

    if (handleNoArgs()) return;

    if (handleTooManyArgs()) return;

    if (handleMinus(curr_path)) return;

    if (handleRegularCD(curr_path)) return;
}
bool ChangeDirCommand::handleNoArgs() {
    return (m_arg_counter == 1);
}
bool ChangeDirCommand::handleTooManyArgs() {
    if (m_arg_counter > 2) {
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        return true;
    }
    return false;
}
bool ChangeDirCommand::handleMinus(char* curr_path) {
    SmallShell& smash = SmallShell::getInstance();

    if (strcmp(m_args[1], "-") == 0 && !smash.getCDflag()) {
        std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
        return true;
    }

    if (strcmp(m_args[1], "-") == 0 && smash.getCDflag()) {
        if (chdir(*m_ptr_last_PWD) == -1) {
            perror("smash error: chdir failed");
            return true;
        }

        m_ptr_last_PWD[0] = curr_path;
        return true;
    }

    return false;
}

bool ChangeDirCommand::handleRegularCD(char* curr_path) {
    SmallShell& smash = SmallShell::getInstance();

    if (chdir(m_args[1]) == -1) {
        perror("smash error: chdir failed");
        return true;
    }

    smash.setCDflag(true);
    m_ptr_last_PWD[0] = curr_path;
    return true;
}


// TODO: Add your implementation for classes in Commands.h 

/**********************************jobs*************************************/
JobsList::JobEntry::JobEntry(int job_id, pid_t pid, std::string job_command)
    : m_job_id(job_id),
      m_job_pid(pid),
      m_job_cmd(job_command),
      m_is_finished(false){}


JobsList::JobsList(): m_jobs_list_map(),m_current_id(1){}

void JobsList::removeFinishedJobs() {
    int curr_id = 1;

    for (auto it = m_jobs_list_map.begin(); it != m_jobs_list_map.end(); ) {

        int curr_pid = waitpid(it->second->m_job_pid, nullptr, WNOHANG);

        if (isStillRunning(curr_pid)) {
            updateCurrIdIfNeeded(curr_id, it->first);
            ++it;
            continue;
        }

        if (processFinishedSuccessfully(curr_pid, it->second->m_job_pid)) {
            it = m_jobs_list_map.erase(it);
            continue;
        }

        if (waitFailed(curr_pid)) {
            perror("smash error: waitpid failed");
            return;
        }

        ++it;
    }

    m_current_id = curr_id;
}
bool JobsList::isStillRunning(int wait_result) {
    return wait_result == 0;
}
bool JobsList::processFinishedSuccessfully(int wait_result, pid_t job_pid) {
    return (wait_result == job_pid) ||
           (wait_result == -1 && errno == ECHILD);
}
bool JobsList::waitFailed(int wait_result) {
    return (wait_result == -1 && errno != ECHILD);
}
void JobsList::updateCurrIdIfNeeded(int& curr_id, int job_id) {
    if (curr_id < job_id + 1) {
        curr_id = job_id + 1;
    }
}



void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    removeFinishedJobs();
    JobEntry* job = createJobEntry(cmd, pid, isStopped);
    updateNextJobId();
    insertJob(job);
}
JobsList::JobEntry* JobsList::createJobEntry(Command* cmd, pid_t pid, bool isStopped)
{
    JobEntry* job = new JobEntry(m_current_id, pid, cmd->m_cmd_line);
    job->m_is_finished = isStopped;
    return job;
}

void JobsList::updateNextJobId() {
    m_current_id++;
}
void JobsList::insertJob(JobEntry* job) {
    m_jobs_list_map[job->m_job_id] = job;
}


void JobsList::killAllJobs(){
  removeFinishedJobs();
  for(const auto& job: m_jobs_list_map){
    if(kill(job.second->m_job_pid,SIGKILL) == -1){
      perror("smash error: kill failed");
      return;
    }
  }
}

void JobsList::printJobsList(){
  removeFinishedJobs();
  for(const auto& job: m_jobs_list_map){
    std::cout<< "[" << job.first<< "] "<< job.second->m_job_cmd<<std::endl;
  }
}

JobsList::JobEntry *JobsList::getJobById(int job_id){
  for(const auto& job : m_jobs_list_map){
    if(job.first == job_id)
      return job.second;
  }
  return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId){
  if(m_jobs_list_map.empty()){
      return nullptr;
  }
  *(lastJobId) = m_jobs_list_map.rbegin()->first;
  return m_jobs_list_map.rbegin()->second;
}

void JobsList::removeJobById(int job_id){
  m_jobs_list_map.erase(job_id);
}

bool JobsList::isJobStopped(JobEntry* job) const {
    if (!job) return false;

    int stat = 0;
    pid_t ret = waitpid(job->m_job_pid, &stat, WNOHANG | WUNTRACED);

    return (ret == job->m_job_pid && WIFSTOPPED(stat));
}
JobsList::JobEntry* JobsList::getStoppedJob(const std::pair<int, JobEntry*>& pair, int& id_out) {
    JobEntry* job = pair.second;

    if (!job || job->m_is_finished)
        return nullptr;

    if (isJobStopped(job)) {
        id_out = pair.first;
        return job;
    }

    return nullptr;
}
JobsList::JobEntry* JobsList::getLastStoppedJob(int* jobId) {
    removeFinishedJobs();

    JobEntry* last = nullptr;
    int last_id = -1;

    for (const auto& pair : m_jobs_list_map) {
        int candidate_id = -1;

        JobEntry* candidate = getStoppedJob(pair, candidate_id);
        if (candidate) {
            last = candidate;
            last_id = candidate_id;
        }
    }

    if (jobId){
      *jobId = last_id;
    }
    return last;
}


JobsList::~JobsList() {
    for (auto& pair : m_jobs_list_map) {
        delete pair.second;
    }
    m_jobs_list_map.clear();
}

//jobs command
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),m_JobsList(jobs){}

void JobsCommand::execute(){
  m_JobsList->printJobsList();
}


//fg command
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs): 
BuiltInCommand(cmd_line),m_job_list(jobs){}

void ForegroundCommand::execute() {
    m_job_list->removeFinishedJobs();

    if (m_arg_counter == 1) {

        int job_id = -1;
        JobsList::JobEntry* last_job = m_job_list->getLastJob(&job_id);

        if (!last_job) {
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            return;
        }

        std::cout << last_job->m_job_cmd << " " << last_job->m_job_pid << std::endl;

        SmallShell::getInstance().m_foreground_pid = last_job->m_job_pid;
        SmallShell::getInstance().m_curr_foregrount_cmd = last_job->m_job_cmd;

        if (kill(last_job->m_job_pid, SIGCONT) == -1) {
            perror("smash error: kill failed");
        }

        if (waitpid(last_job->m_job_pid, nullptr, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
        }

        SmallShell::getInstance().m_foreground_pid = -1;
        SmallShell::getInstance().m_curr_foregrount_cmd = "";

        m_job_list->removeJobById(job_id);
        return;
    }


    if (m_arg_counter != 2) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }

    int job_id2 = -1;
    try {
        job_id2 = stoi(m_args[1]);
    }
    catch (...) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }

    if (job_id2 <= 0) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }

    JobsList::JobEntry* curr_job = m_job_list->getJobById(job_id2);

    if (!curr_job) {
        std::cerr << "smash error: fg: job-id " << job_id2 << " does not exist" << std::endl;
        return;
    }

    std::cout << curr_job->m_job_cmd << " " << curr_job->m_job_pid << std::endl;

    SmallShell::getInstance().m_foreground_pid = curr_job->m_job_pid;
    SmallShell::getInstance().m_curr_foregrount_cmd = curr_job->m_job_cmd;

    if (kill(curr_job->m_job_pid, SIGCONT) == -1) {
        perror("smash error: kill failed");
    }

    if (waitpid(curr_job->m_job_pid, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
    }

    SmallShell::getInstance().m_foreground_pid = -1;
    SmallShell::getInstance().m_curr_foregrount_cmd = "";

    m_job_list->removeJobById(job_id2);
}



//quit command
QuitCommand::QuitCommand(const char *cmd_line,JobsList* j):BuiltInCommand(cmd_line),m_jobs_list(j){}


void QuitCommand::printKillMessage() const {
  int jobs_num = m_jobs_list->m_jobs_list_map.size();
  std::cout << "smash: sending SIGKILL signal to " << jobs_num << " jobs:" << std::endl;
  for(const auto& job : m_jobs_list->m_jobs_list_map){
      std::cout << job.second->m_job_pid << ": " << job.second->m_job_cmd << std::endl;
  }
}


void QuitCommand::execute(){
  m_jobs_list->removeFinishedJobs();

  if(m_arg_counter == 1){
    exit(0);
  }

  if(strcmp(m_args[1],"kill")!=0){
    exit(0);
  }

  printKillMessage();
  m_jobs_list->killAllJobs();
  exit(0);
}


//kill command 
KillCommand::KillCommand(const char *cmd_line, JobsList *jobs):BuiltInCommand(cmd_line),m_job_list(jobs){}

bool KillCommand::getJobAndSignal(int& sig_num, int& job_id) const {
    try { sig_num = stoi(m_args[1]); }
    catch (...) { 
      return false; 
    }

    try { job_id = stoi(m_args[2]); }
    catch (...) { 
      return false; 
    }

    if (job_id <= 0){
        return false;
    }
    sig_num = -sig_num;

    if (sig_num < 0){
        return false;
    }
    return true;
}


bool KillCommand::sendSignal(int sig_num, JobsList::JobEntry* job) const {
    std::cout << "signal number " << sig_num << " was sent to pid " << job->m_job_pid << std::endl;

    if (kill(job->m_job_pid, sig_num) == -1) {
        perror("smash error: kill failed");
        return false;
    }
    return true;
}

void KillCommand::execute() {
    m_job_list->removeFinishedJobs();


    if (m_arg_counter != 3) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    int sig_num = -1;
    int job_id = -1;

    if (!getJobAndSignal(sig_num, job_id)) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    JobsList::JobEntry* curr_job = m_job_list->getJobById(job_id);
    if (!curr_job) {
        std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
        return;
    }

    sendSignal(sig_num, curr_job);
}


//alias command
AliasCommand::AliasCommand(const char *cmd_line):BuiltInCommand(cmd_line){}

std::string extractName(const std::string& input) {
    size_t pos = input.find('='); 
    std::string s= input.substr(0, pos); 
    return s;
}

bool isValidName(const std::string& name) {
    const std::vector<std::string> reservedKeywords = {"quit","chprompt","showpid","pwd","cd","jobs","fg","kill"
    ,"alias","unalias",">",">>","|","whoami","sysinfo","du","unsetenv","usbinfo"};
    for (const std::string& keyword : reservedKeywords) {
        if (strcmp(name.c_str(),keyword.c_str()) == 0) {
            return false;
        }
    }
    return true;
}

std::string extractCommand(std::string input) {
    int startPos = input.find("='");
    startPos += 2;
    int endPos = input.find("'", startPos);
    return input.substr(startPos, endPos - startPos);
}

void AliasCommand::printAllAliases() {
    for (const auto& pair : SmallShell::getInstance().m_allias_strings) {
        std::cout << pair.second << std::endl;
    }
}

bool AliasCommand::isAliasFormatValid() {
    std::regex alias_Regex("^alias [a-zA-Z0-9_]+='[^']*'$");
    char line[COMMAND_MAX_LENGTH];
    strcpy(line, m_cmd_line.c_str());
    std::string t = _trim(std::string(line));
    return std::regex_match(t, alias_Regex);
}

bool AliasCommand::isAliasNameValid(const std::string& name) {
    if (!isValidName(name))
        return false;

    for (const auto& pair : SmallShell::getInstance().m_allias_strings) {
        if (pair.first == name)
            return false;
    }
    return true;
}

void AliasCommand::addAlias(const std::string& name) {
    char buffer[COMMAND_MAX_LENGTH];
    strcpy(buffer, m_cmd_line.c_str());
    std::string temp = _trim(std::string(buffer).substr(6));

    SmallShell::getInstance().m_allias_strings.push_back({name, temp});
}

void AliasCommand::execute() {
    if (m_arg_counter == 1) {
        printAllAliases();
        return;
    }

    if (!isAliasFormatValid()) {
        std::cerr << "smash error: alias: invalid alias format" << std::endl;
        return;
    }

    std::string name = extractName(m_args[1]);
    if (!isAliasNameValid(name)) {
        std::cerr << "smash error: alias: " << name << " already exists or is a reserved command" << std::endl;
        return;
    }

    addAlias(name);
}


//unalias comand
UnAliasCommand::UnAliasCommand(const char* cmd_line):BuiltInCommand(cmd_line){}
void UnAliasCommand::execute() {
  if (m_arg_counter == 1) {
    std::cerr << "smash error: unalias: not enough arguments" << std::endl;
    return;
  }

  for (int i = 1; i < m_arg_counter; i++) {
    std::string name = extractName(m_args[i]);
    bool removed = false;
    auto& aliases = SmallShell::getInstance().m_allias_strings;

    for (auto it = aliases.begin(); it != aliases.end(); ++it) {
      if (strcmp(it->first.c_str(), name.c_str()) == 0) {
        aliases.erase(it);
        removed = true;
        break;
      }
    }
    if (!removed) {
      std::cerr << "smash error: unalias: " << name << " alias does not exist" << std::endl;
      return;
    }
  }
}



//unsetenv command
extern char **environ;

UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line):BuiltInCommand(cmd_line){}

extern char **__environ;
bool UnSetEnvCommand::exists_in_proc(const char* key) {
    int pid = getpid();
    char path[64];
    sprintf(path, "/proc/%d/environ", pid);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("smash error: open failed");
        return false;
    }

    char buffer[8192];
    ssize_t n = read(fd, buffer, sizeof(buffer));
    close(fd);

    if (n <= 0){
        return false;
    }
    size_t key_len = strlen(key);
    char* ptr = buffer;
    char* end = buffer + n;

    while (ptr < end) {
        if (strncmp(ptr, key, key_len) == 0 && ptr[key_len] == '=') {
            return true;
        }
        ptr += strlen(ptr) + 1;
    }

    return false;
}

int UnSetEnvCommand::remove_env(const char* key) {
    if (!key || strchr(key, '=')) 
        return -1;

    size_t key_len = strlen(key);

    for (int i = 0; __environ[i]; i++) {
        if (strncmp(__environ[i], key, key_len) == 0 &&
            __environ[i][key_len] == '=') {

            for (int j = i; __environ[j]; j++) {
                __environ[j] = __environ[j + 1];
            }
            return 0;
        }
    }
    return -1;
}
void UnSetEnvCommand::execute() {
    if (m_arg_counter < 2) {
        std::cerr << "smash error: unsetenv: not enough arguments" << std::endl;
        return;
    }

    for (int i = 1; i < m_arg_counter; ++i) {
        const char* key = m_args[i];

        if (!exists_in_proc(key)) {
            std::cerr << "smash error: unsetenv: " << key << " does not exist" << std::endl;
            return;
        }

        if (remove_env(key) != 0) {
            std::cerr << "smash error: unsetenv: " << key << " does not exist" << std::endl;
            return;
        }
    }
}


//sysinfo command
SysInfoCommand::SysInfoCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

static bool  readFile1Line(const char* path, std::string& out) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return false;

    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    close(fd);

    if (n <= 0) return false;

    buf[n] = '\0';
    out = std::string(buf);

    out.erase(out.find_last_not_of(" \n\t\r") + 1);

    return true;
}

static bool getArchitecture(std::string& arch) {
    int fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) return false;

    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    close(fd);

    if (n <= 0) return false;

    buf[n] = '\0';

    char* line = strtok(buf, "\n");
    while (line) {
        if (strncmp(line, "model name", 10) == 0) {
            arch = "x86_64"; 
            return true;
        }
        if (strncmp(line, "Architecture", 12) == 0) {
            char* p = strchr(line, ':');
            if (p) {
                p++;
                while (*p==' ' || *p=='\t') p++;
                arch = p;
                return true;
            }
        }
        line = strtok(nullptr, "\n");
    }

    arch = "Unknown";
    return true;
}

void SysInfoCommand::execute() 
{
    std::string systemName, kernelVer, hostname, arch;

    if (!readFile1Line("/proc/sys/kernel/ostype", systemName))
        systemName = "Unknown";

    if (!readFile1Line("/proc/sys/kernel/osrelease", kernelVer))
        kernelVer = "Unknown";

    if (!readFile1Line("/proc/sys/kernel/hostname", hostname))
        hostname = "Unknown";

    getArchitecture(arch);

    int fd = open("/proc/stat", O_RDONLY);
    if (fd < 0) {
        perror("smash error: open failed");
        return;
    }

    char buffer[4096];
    ssize_t len = read(fd, buffer, sizeof(buffer)-1);
    close(fd);

    if (len <= 0) {
        perror("smash error: read failed");
        return;
    }

    buffer[len] = '\0';

    long boottime = -1;
    char* line = strtok(buffer, "\n");
    while (line) {
        if (strncmp(line, "btime", 5) == 0) {
            boottime = atol(line + 6);
            break;
        }
        line = strtok(nullptr, "\n");
    }

    char boot_str[64] = "Unknown";
    if (boottime != -1) {
        time_t bt = (time_t)boottime;
        struct tm* t = localtime(&bt);
        strftime(boot_str, sizeof(boot_str), "%Y-%m-%d %H:%M:%S", t);
    }

    cout << "System: " << systemName << endl;
    cout << "Hostname: " << hostname << endl;
    cout << "Kernel: " << kernelVer << endl;
    cout << "Architecture: " << arch << endl;
    cout << "Boot Time: " << boot_str << endl;
}


//ExternalCommand
ExternalCommand::ExternalCommand(const char* cmd_line):Command(cmd_line){}

bool ExternalCommand::handleAlias(std::string& cmd_s, char*& new_cmd) {
    string first = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    for (auto& pair : SmallShell::getInstance().m_allias_strings) {
        if (strcmp(pair.first.c_str(), first.c_str()) == 0) {
            string command = extractCommand(pair.second);

            char* args[COMMAND_MAX_ARGS] = {0};
            _parseCommandLine(cmd_s.c_str(), args);

            args[0] = strdup(command.c_str());
            new_cmd = concatenateArgs(args);

            cmd_s = _trim(string(new_cmd));
            return true;
        }
    }
    return false;
}

char* ExternalCommand::buildFinalCommand(std::string& cmd_s) {
    char* new_cmd_line = nullptr;

    if (handleAlias(cmd_s, new_cmd_line)) {
        return new_cmd_line;
    }

    new_cmd_line = strdup(m_cmd_line.c_str());
    cmd_s = _trim(m_cmd_line);
    return new_cmd_line;
}

void ExternalCommand::prepareExecData(
    const std::string& cmd_s,
    bool& complex,
    char* clean_line,
    char**& bash_argv)
{
    complex = (cmd_s.find('*') != string::npos ||cmd_s.find('?') != string::npos);

    strcpy(clean_line, cmd_s.c_str());
    _removeBackgroundSign(clean_line);
    _parseCommandLine(clean_line, bash_argv);
}

void ExternalCommand::runChildProcess(bool complex, char* clean_line, char** bash_argv) {
    if (setpgrp() == -1) {
        perror("smash error: setpgrp failed");
        exit(EXIT_FAILURE);
    }

    if (complex) {
      char* args[4] = {strdup("/bin/bash"), strdup("-c"), clean_line, nullptr};
        if (execv("/bin/bash", args) == -1) {
          perror("smash error: execv failed");
          exit(EXIT_FAILURE);
        }
    }
    else {
        if (execvp(bash_argv[0], bash_argv) == -1) {
          perror("smash error: execvp failed");
          exit(EXIT_FAILURE);
        }
    }
}


void ExternalCommand::handleParent(pid_t pid, char* original_cmd) {
    if (m_is_background) {
      SmallShell::getInstance().m_smallshell_job_list->addJob(new ExternalCommand(original_cmd), pid);
      return;
    }

    SmallShell::getInstance().m_foreground_pid = pid;
    SmallShell::getInstance().m_curr_foregrount_cmd = m_cmd_line;

    if (waitpid(pid, nullptr, WUNTRACED) == -1) {
      perror("smash error: waitpid failed");
    }

    SmallShell::getInstance().m_foreground_pid = -1;
    SmallShell::getInstance().m_curr_foregrount_cmd = "";
}


void ExternalCommand::execute() {
    char* original = strdup(m_cmd_line.c_str());

    std::string cmd_s = _trim(string(m_cmd_line));
    char* final_cmd = buildFinalCommand(cmd_s);

    m_cmd_line = final_cmd;
    m_is_background = _isBackgroundComamnd(m_cmd_line.c_str());
    m_arg_counter = _parseCommandLine(m_cmd_line.c_str(), m_args);

    if (strcmp(m_cmd_line.c_str(), "") == 0){
      return;
    }
    bool complex = false;
    char clean_line[COMMAND_MAX_LENGTH];
    char** bash_argv = new char*[m_arg_counter + 1];

    prepareExecData(m_cmd_line, complex, clean_line, bash_argv);
    bash_argv[m_arg_counter] = nullptr;

    pid_t pid = fork();
    if (pid == 0) {
      runChildProcess(complex, clean_line, bash_argv);
    }
    else {
      handleParent(pid, original);
    }
}




//RedirectionCommand
RedirectionCommand::RedirectionCommand(const char* cmd_line):Command(cmd_line),m_file_des(0),m_standard_O(0){}

void RedirectionCommand::restoreRedirection() {

  if (close(m_file_des) == -1){
    perror("smash error: close failed");
  }

  if (dup2(m_standard_O, STDOUT_FILENO) == -1){
    perror("smash error: dup2 failed");
  }
  if (close(m_standard_O) == -1){
    perror("smash error: close failed");
  }
}

bool RedirectionCommand::setupRedirection() {
    m_standard_O = dup(STDOUT_FILENO);
    if (m_standard_O == -1) {
      perror("smash error: dup failed");
      return false;
    }

    char line[COMMAND_MAX_LENGTH];
    strcpy(line, m_cmd_line.c_str());
    _removeBackgroundSign(line);

    std::string clean = _trim(std::string(line));

    bool append = false;
    int index = -1;

    if (clean.find(">>") != std::string::npos) {
        append = true;
        index = clean.find(">>");
    }
    else if (clean.find(">") != std::string::npos) {
        append = false;
        index = clean.find(">");
    }
    else {
        return false;
    }

    std::string fileName;
    if (append)
        fileName = _trim(clean.substr(index + 2));
    else
        fileName = _trim(clean.substr(index + 1));

    if (append)
        m_file_des = open(fileName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0664);
    else
        m_file_des = open(fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);

    if (m_file_des == -1) {
        perror("smash error: open failed");
        return false;
    }

    if (dup2(m_file_des, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
        close(m_file_des);
        return false;
    }

    return true;
}
void RedirectionCommand::execute() {
    char line[COMMAND_MAX_LENGTH];
    strcpy(line, m_cmd_line.c_str());
    _removeBackgroundSign(line);

    std::string clean = _trim(std::string(line));

    if (!setupRedirection())
        return;

    int index = -1;

    if (clean.find(">>") != std::string::npos)
        index = clean.find(">>");
    else if (clean.find(">") != std::string::npos)
        index = clean.find(">");

    if (index != -1) {
        std::string real_cmd = _trim(clean.substr(0, index));
        SmallShell::getInstance().executeCommand(real_cmd.c_str());
    }

    restoreRedirection();
}


//pipe command
PipeCommand::PipeCommand(const char* cmd_line):Command(cmd_line){}

bool parse_pipe_command(const std::string &cmd_line, std::string &cmd1, std::string &cmd2) {
    size_t pipe_pos = cmd_line.find('|');

    bool is_ref = false;
    if (cmd_line[pipe_pos + 1] == '&') {
        is_ref = true;
        pipe_pos++; // Skip & in the pipe operator
    }

    cmd1 = cmd_line.substr(0, pipe_pos);
    cmd2 = cmd_line.substr(pipe_pos + 1);


    cmd1.erase(cmd1.find_last_not_of(" \t") + 1);
    cmd2.erase(0, cmd2.find_first_not_of(" \t"));

    return is_ref;
}

std::vector<char *> parse_arguments(const std::string &cmd) {
    std::vector<char *> args;
    std::istringstream iss(cmd);
    std::string token;

    while (iss >> token) {
        char *arg = new char[token.size() + 1];
        strcpy(arg, token.c_str());
        args.push_back(arg);
    }
    args.push_back(nullptr);
    return args;
}

bool PipeCommand::prepare_parent_pipe(int pipe_fd[2], bool redirect_stderr, const std::string& cmd1){
    if (close(pipe_fd[0]) == -1) {
        perror("smash error: close failed");
        return false;
    }

    int target_fd = redirect_stderr ? STDERR_FILENO : STDOUT_FILENO;

    int backup_fd = dup(target_fd);
    if (backup_fd == -1) {
        perror("smash error: dup failed");
        return false;
    }

    if (dup2(pipe_fd[1], target_fd) == -1) {
        perror("smash error: dup2 failed");
        close(pipe_fd[1]);
        return false;
    }

    SmallShell::getInstance().executeCommand(cmd1.c_str());

    if (close(pipe_fd[1]) == -1) {
        perror("smash error: close failed");
        return false;
    }

    if (dup2(backup_fd, target_fd) == -1) {
        perror("smash error: dup2 failed");
        return false;
    }

    if (close(backup_fd) == -1) {
        perror("smash error: close failed");
        return false;
    }
    return true;
}

void PipeCommand::prepare_child_pipe(int pipe_fd[2],const std::string& cmd2){
    if (setpgrp() == -1) {
      perror("smash error: setpgrp failed");
      exit(EXIT_FAILURE);
    }

    if (close(pipe_fd[1]) == -1) {
      perror("smash error: close failed");
      exit(EXIT_FAILURE);
    }

    if (dup2(pipe_fd[0], STDIN_FILENO) == -1) {
      perror("smash error: dup2 failed");
      exit(EXIT_FAILURE);
    }

    SmallShell::getInstance().executeCommand(cmd2.c_str());

    close(pipe_fd[0]);
    close(STDIN_FILENO);

    exit(0);
}
void PipeCommand::execute()
{
    std::string cmd1, cmd2;
    bool redirect_stderr = parse_pipe_command(m_cmd_line, cmd1, cmd2);

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
    }

    if (pid != 0) {// parent
        if (!prepare_parent_pipe(pipe_fd, redirect_stderr, cmd1)){
            return;
        }

        waitpid(pid, nullptr, 0);
    }
    else {// child
        prepare_child_pipe(pipe_fd, cmd2);
    }
}




//du command
DiskUsageCommand::DiskUsageCommand(const char* cmd_line):Command(cmd_line){}

bool isDuDirectoryAccessible(const std::string& path) {
  struct stat statbuf;
  return stat(path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
}

size_t calculateDirectorySize(const std::string& path) {
  size_t total_kb = 0;
  int dir_fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
  if (dir_fd == -1) {
      std::cerr << "smash error: open failed" << std::endl;
      return 0;
  }

  char read_buffer[4096];

  while (true) {
      ssize_t bytes_read = syscall(SYS_getdents64, dir_fd, read_buffer, sizeof(read_buffer));
      if (bytes_read == -1) {
          std::cerr << "smash error: read failed" << std::endl;
          close(dir_fd);
          return 0;
      }
      if (bytes_read == 0){
          break;
      }
      char* cursor = read_buffer;

      while (cursor < read_buffer + bytes_read) {
        struct dirent* entry = reinterpret_cast<struct dirent*>(cursor);

        if (entry->d_reclen < sizeof(struct dirent)){
            break;
        }
        std::string entry_name = entry->d_name;

        if (entry_name == "." || entry_name == "..") {
           cursor += entry->d_reclen;
            continue;
        }

        std::string full_path = path + "/" + entry_name;

        struct stat entry_stat;
        if (lstat(full_path.c_str(), &entry_stat) == 0) {
          total_kb += entry_stat.st_blocks / 2;
          if (S_ISDIR(entry_stat.st_mode)) {
            total_kb += calculateDirectorySize(full_path);
          }
        }
        cursor += entry->d_reclen;
        }
    }

    close(dir_fd);
    return total_kb;
}


void DiskUsageCommand::execute() {
  if (m_arg_counter > 2) {
      std::cerr << "smash error: du: too many arguments" << std::endl;
      return;
  }
  std::string path;
  if (m_arg_counter == 1) {
      path = ".";
  } else {
      path = m_args[1];
    }
    
    if (!isDuDirectoryAccessible(path)) {
      std::cerr << "smash error: du: directory " << path << " does not exist" << std::endl;
      return;
    }
    size_t totalKB = calculateDirectorySize(path);
    std::cout << "Total disk usage: " << totalKB << " KB" << std::endl;
}


//whoami command
WhoAmICommand::WhoAmICommand(const char* cmd_line):Command(cmd_line){}

bool parse_passwd_entry(const std::string& entry, int target_uid,std::string& username_out, std::string& home_out) {
    char line[512];
    strcpy(line, entry.c_str());

    char* username = strtok(line, ":");
    strtok(nullptr, ":"); // password (ignored)
    char* uid_str = strtok(nullptr, ":");
    strtok(nullptr, ":"); // gid
    strtok(nullptr, ":"); // gecos
    char* home_dir = strtok(nullptr, ":");

    if (!username || !uid_str || !home_dir)
        return false;

    int entry_uid = atoi(uid_str);
    if (entry_uid == target_uid) {
        username_out = username;
        home_out = home_dir;
        return true;
    }

    return false;
}
bool find_user_in_passwd(int uid, std::string& username, std::string& home_dir) {
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd == -1) {
        std::cerr << "smash error: open failed" << std::endl;
        return false;
    }

    char buffer[4096];
    std::string entry;
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                if (parse_passwd_entry(entry, uid, username, home_dir)) {
                    close(fd);
                    return true;
                }
                entry.clear();
            } else {
                entry += buffer[i];
            }
        }
    }

    if (close(fd) == -1)
        std::cerr << "smash error: close failed" << std::endl;

    return false;
}

void WhoAmICommand::execute() {
    int uid = getuid();
    int gid = getgid();

    std::string username, home;
    bool ok = find_user_in_passwd(uid, username, home);

    std::cout << uid << std::endl;
    std::cout << gid << std::endl;

    if (ok) {
        std::cout << username << std::endl;
        std::cout << home << std::endl;
    }
}



// usbinfo command
USBInfoCommand::USBInfoCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}


static bool readFile(const std::string& path, std::string& out) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return false;

    char buf[256];
    ssize_t len = read(fd, buf, sizeof(buf)-1);
    close(fd);

    if (len <= 0) return false;

    buf[len] = '\0';
    out = std::string(buf);
    out.erase(out.find_last_not_of(" \n\t\r") + 1);

    return true;
}

void USBInfoCommand::execute() {
    const char* base = "/sys/bus/usb/devices";
    int dirfd = open(base, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        perror("smash error: open failed");
        return;
    }

    char buffer[4096];
    ssize_t nread = syscall(SYS_getdents64, dirfd, buffer, sizeof(buffer));
    if (nread < 0) {
        close(dirfd);
        perror("smash error: getdents64 failed");
        return;
    }

    struct linux_dirent64 {
        ino64_t        d_ino;
        off64_t        d_off;
        unsigned short d_reclen;
        unsigned char  d_type;
        char           d_name[];
    };

    std::vector<std::pair<int,std::string>> devices; 
    std::vector<std::string> devnames;

    for (char* ptr = buffer; ptr < buffer + nread; ) {
        auto* d = (linux_dirent64*)ptr;

        std::string name = d->d_name;
        if (name == "." || name == "..") {
            ptr += d->d_reclen;
            continue;
        }
        devnames.push_back(name);

        ptr += d->d_reclen;
    }

    close(dirfd);

    struct DeviceInfo {
        int devnum;
        std::string vendor, product, manuf, prodname, maxpower;
    };

    std::vector<DeviceInfo> infos;

    for (auto& dev : devnames) {
        std::string path = std::string(base) + "/" + dev;

        // Read devnum
        std::string devnum_s;
        if (!readFile(path + "/devnum", devnum_s)) continue;
        int devnum = atoi(devnum_s.c_str());

        DeviceInfo info;
        info.devnum = devnum;

        readFile(path + "/idVendor", info.vendor);
        readFile(path + "/idProduct", info.product);
        readFile(path + "/manufacturer", info.manuf);
        readFile(path + "/product", info.prodname);
        readFile(path + "/bMaxPower", info.maxpower);

        if (info.vendor.empty()) info.vendor = "N/A";
        if (info.product.empty()) info.product = "N/A";
        if (info.manuf.empty()) info.manuf = "N/A";
        if (info.prodname.empty()) info.prodname = "N/A";
        if (info.maxpower.empty()) info.maxpower = "N/A";

        infos.push_back(info);
    }

    if (infos.empty()) {
        std::cerr << "smash error: usbinfo: no USB devices found" << std::endl;
        return;
    }

    // Sort by devnum ascending
    std::sort(infos.begin(), infos.end(),
              [](const DeviceInfo& a, const DeviceInfo& b) {
                  return a.devnum < b.devnum;
              });

    for (auto& i : infos) {
        std::string mp = i.maxpower;
        if (mp != "N/A" && mp.size() > 2 && mp.substr(mp.size()-2) == "mA")
            mp = mp.substr(0, mp.size()-2);

        cout << "Device " << i.devnum 
             << ": ID " << i.vendor << ":" << i.product << " "
             << i.manuf << " " << i.prodname
             << " MaxPower: " << mp << "mA" << endl;
    }
}



/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
    /*
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
    }
    else if ...
    .....
    else {
      return new ExternalCommand(cmd_line);
    }
    */
  char* c = new char[COMMAND_MAX_LENGTH];
  strcpy(c,cmd_line);
  string cmd_s = _trim(string(cmd_line));
  if(strcmp(cmd_line,"\n")==0 || strcmp(cmd_line,"")==0){
    return nullptr;
  }
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  char* new_arg[COMMAND_MAX_ARGS] ={0};
  _parseCommandLine(cmd_s.c_str(),new_arg);
  bool flag = false;
  char* new_cmd_line =strdup("");
  for(auto& pair : SmallShell::getInstance().m_allias_strings)
  {
    if(strcmp(pair.first.c_str(),firstWord.c_str()) == 0)
    {
      flag = true;
      std::string command = extractCommand(pair.second);
      new_arg[0] = strdup(command.c_str());
      new_cmd_line = concatenateArgs(new_arg);
      cmd_s = _trim(string(new_cmd_line));
      firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    }
  }
  if(!flag){
    new_cmd_line = strdup(cmd_line);
    cmd_s = _trim(string(cmd_line));
    firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  }
  if(firstWord.compare("alias") == 0 || firstWord.compare("alias&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new AliasCommand(new_cmd_line);
  }
  else if(cmd_s.find("|") != string::npos || cmd_s.find("|&") != string::npos){
    return new PipeCommand(new_cmd_line);
  }
  else if(cmd_s.find(">") != string::npos || cmd_s.find(">>") != string::npos){
    _removeBackgroundSign(new_cmd_line);
    return new RedirectionCommand(new_cmd_line);
  }
  if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0) {
    _removeBackgroundSign(new_cmd_line);
    return new GetCurrDirCommand(new_cmd_line);
  }
  else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {
    _removeBackgroundSign(new_cmd_line);
    return new ShowPidCommand(new_cmd_line);
  }
  else if(firstWord.compare("cd") == 0 || firstWord.compare("cd&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new ChangeDirCommand(new_cmd_line,&SmallShell::getInstance().m_last_dir);
  }
  else if(firstWord.compare("unsetenv") == 0 || firstWord.compare("unsetenv&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new UnSetEnvCommand(new_cmd_line);
  }
  else if(firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new Chprompt(new_cmd_line);
  }
  else if(firstWord.compare("whoami") == 0 || firstWord.compare("whoami&") == 0){
    return new WhoAmICommand(new_cmd_line);
  }
  else if(firstWord.compare("quit") == 0 || firstWord.compare("quit&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new QuitCommand(new_cmd_line,SmallShell::getInstance().m_smallshell_job_list);
  }
  else if(firstWord.compare("unalias") == 0 || firstWord.compare("unalias&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new UnAliasCommand(new_cmd_line);
  }
  else if(firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new JobsCommand(new_cmd_line,SmallShell::getInstance().m_smallshell_job_list);
  }
  else if(firstWord.compare("du") == 0 || firstWord.compare("du&") == 0){
    return new DiskUsageCommand(new_cmd_line);
  }
  else if(firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new ForegroundCommand(new_cmd_line,SmallShell::getInstance().m_smallshell_job_list);
  }
  else if(firstWord.compare("kill") == 0 || firstWord.compare("kill&") == 0){
    _removeBackgroundSign(new_cmd_line);
    return new KillCommand(new_cmd_line,SmallShell::getInstance().m_smallshell_job_list);
  }
  else if(firstWord.compare("sysinfo") == 0 || firstWord.compare("sysinfo&") == 0){
    return new SysInfoCommand(new_cmd_line);
  }
  else if(firstWord.compare("usbinfo") == 0 || firstWord.compare("usbinfo&") == 0) {
    _removeBackgroundSign(new_cmd_line);
    return new USBInfoCommand(new_cmd_line);
  }else{
    if(_isBackgroundComamnd(new_cmd_line)){
      return new ExternalCommand(c);
    }
      return new ExternalCommand(new_cmd_line);
    }
    return nullptr;
}




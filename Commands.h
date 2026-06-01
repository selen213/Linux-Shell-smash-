// Ver: 04-11-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <regex>
#include <map>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
    // TODO: Add your data members
public:
    std::string m_cmd_line;
    char **m_args;
    int m_arg_counter;
    bool m_is_background;
    Command(const char *cmd_line);

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {
    }

    void execute() override;


    //helper functions for execute()
    bool handleAlias(std::string& cmd_s, char*& new_cmd_line);
    char* buildFinalCommand(std::string& cmd_s);
    void prepareExecData(const std::string& cmd_s, bool& complex_flag,char* clean_line, char**& bash_argv);
    void runChildProcess(bool complex_flag, char* clean_line, char** bash_argv);
    void handleParent(pid_t pid, char* original_cmd);
};



//i added
/***************************ChpromptCommand************************************/
class Chprompt: public BuiltInCommand{
    public:
    Chprompt(const char* cmd_line);
    virtual ~Chprompt() = default;
    void execute () override;

};

/***************************ShowPidCommand************************************/
class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

/***************************GetCurrDirCommand pwd******************************/
class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};


/***************************ChangeDirCommand cd********************************/
class ChangeDirCommand : public BuiltInCommand {
    // TODO: Add your data members 
    bool handleNoArgs();
    bool handleTooManyArgs();
    bool handleMinus(char* curr_path);
    bool handleRegularCD(char* curr_path);

    public:
    char** m_ptr_last_PWD;
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};


/***************************JobsCommand************************************/
class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    public:
        int m_job_id;
        pid_t m_job_pid;
        std::string m_job_cmd;
        bool m_is_finished;

        JobEntry(int job_id, pid_t pid, std::string job_command);
    };

    std::map<int, JobEntry*> m_jobs_list_map;
    int m_current_id;


    // TODO: Add your data members
public:
    JobsList();

    ~JobsList();

    void addJob(Command* cmd, pid_t pid, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
    bool isStillRunning(int wait_result);
    bool processFinishedSuccessfully(int wait_result, pid_t job_pid);
    bool waitFailed(int wait_result);
    void updateCurrIdIfNeeded(int& curr_id, int job_id);

    JobEntry* createJobEntry(Command* cmd, pid_t pid, bool isStopped);
    void updateNextJobId();
    void insertJob(JobEntry* job);

    bool isJobStopped(JobEntry* job) const;
    JobEntry* getStoppedJob(const std::pair<int, JobEntry*>& pair, int& id_out);



};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsList* m_JobsList;
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {
    }

    void execute() override;
};


/***************************ForegroundCommand************************************/
class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsList* m_job_list;
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};


/***************************QuitCommand*********************************/
class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members 
    public:

    JobsList* m_jobs_list;
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {
    }

    void execute() override;
    void printKillMessage() const;

};



/***************************KillCommand*********************************/
class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsList* m_job_list;
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {
    }

    void execute() override;

    bool getJobAndSignal(int& sig_num, int& job_id) const;
    bool sendSignal(int sig_num, JobsList::JobEntry* job) const;

};

/***************************AliasCommand*********************************/
class AliasCommand : public BuiltInCommand {
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
    }

    void execute() override;

    // Print all aliases
    void printAllAliases();

    // Validate format: alias name='command'
    bool isAliasFormatValid();

    // Validate alias name not reserved and not duplicated
    bool isAliasNameValid(const std::string& name);

    // Add alias entry to vector
    void addAlias(const std::string& name);
};


/***************************UnAliasCommand*********************************/
class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

/***************************UnSetEnvCommand*********************************/
class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;

    int remove_env(const char* key);
    bool exists_in_proc(const char* key);

};


/***************************SysInfoCommand*********************************/
class SysInfoCommand : public BuiltInCommand {
public:
    SysInfoCommand(const char *cmd_line);

    virtual ~SysInfoCommand() {
    }

    void execute() override;
};

/***************************RedirectionCommand*********************************/
class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    int m_file_des;
    int m_standard_O;


    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {
    }

    void execute() override;

    bool setupRedirection();
    void restoreRedirection();
};


/***************************PipeCommand*********************************/
class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;

    bool prepare_parent_pipe(int pipe_fd[2], bool redirect_stderr,const std::string& cmd1);
    void prepare_child_pipe(int pipe_fd[2], const std::string& cmd2);
};


/***************************DiskUsageCommand du*********************************/
class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};


/***************************WhoAmICommand*********************************/
class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};


/***************************USBInfoCommand*********************************/
class USBInfoCommand : public BuiltInCommand {
    // TODO: Add your data members **BONUS: 10 Points**
public:
    USBInfoCommand(const char *cmd_line);

    virtual ~USBInfoCommand() {
    }

    void execute() override;
};


class JobsList;


class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);

    std::string m_curr_prompt;
    bool m_cd_flag;
    char* m_last_dir;
    JobsList* m_smallshell_job_list;
    pid_t m_foreground_pid;
    std::string m_curr_foregrount_cmd;
    std::vector<std::pair<std::string,std::string>> m_allias_strings;

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed

    pid_t getForegroundPid(){
        return m_foreground_pid;
    }

    void setForegroundPid(pid_t pid){
        this->m_foreground_pid=pid;
    }

    bool getCDflag(){
        return m_cd_flag;
    }
    char* getLastDir(){
        return m_last_dir;
    }
    void setCDflag(bool b){
        m_cd_flag=b;
    }
    void setPrompt(const std::string& str){
      
        m_curr_prompt = str;
   
    }
};

#endif //SMASH_COMMAND_H_

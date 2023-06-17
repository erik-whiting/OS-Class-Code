#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

#include "command.hpp"
#include "parser.hpp"

#define MAX_ALLOWED_LINES 25

void runCommand(struct shell_command);
void singleCommand(struct shell_command);
void simpleRedir(struct shell_command);
void handleMultipleLogicals(std::vector<struct shell_command>);

int main(int argc, char *argv[])
{
    std::string input_line;

    for (int i=0;i<MAX_ALLOWED_LINES;i++) { // Limits the shell to MAX_ALLOWED_LINES
        // Print the prompt if not passed -t option.
        // std::cout << "osh> " << std::flush;
        if (argc != 2) { std::cout << "osh> " << std::flush; }
        // Read a single line.
        if (!std::getline(std::cin, input_line) || input_line == "exit") {
            break;
        }

        try {
            // Parse the input line.
            std::vector<shell_command> shell_commands
                    = parse_command_string(input_line);

            // Print the list of commands.
            // std::cout << "-------------------------\n";
            for (unsigned int i = 0; i < shell_commands.size(); i++) {
                auto cmd = shell_commands[i];
                // std::cout << cmd;
                // std::cout << "-------------------------\n";
                if (cmd.next_mode == next_command_mode::on_success || cmd.next_mode == next_command_mode::on_fail) {
                    std::vector<struct shell_command> commands;
                    commands.push_back(cmd);
                    while (cmd.next_mode == next_command_mode::on_success || cmd.next_mode == next_command_mode::on_fail) {
                        commands.push_back(shell_commands[i + 1]);
                        i++;
                        cmd = shell_commands[i];
                    }
                    handleMultipleLogicals(commands);
                } else if (cmd.cout_file != "" || cmd.cin_file != "") {
                    simpleRedir(cmd);
                } else if (cmd.cin_file == "" && cmd.cout_file == "") {
                    singleCommand(cmd);
                }
            }
        }
        catch (const std::runtime_error& e) {
            std::cout << "osh: " << e.what() << "\n";
        }
    }
}

void runCommand(struct shell_command cmd) {
    // The code in this body makes heavy use of the
    // example file found in helpful_code/string_char.cpp
    const char **argv = new const char* [cmd.args.size() + 2];
    argv[0] = cmd.cmd.c_str();
    for (int i = 0;  i < (signed) cmd.args.size() + 1;  i++) {
        argv[i + 1] = cmd.args[i].c_str();
    }
    argv[cmd.args.size() + 1] = NULL;
    execvp(argv[0], (char**) argv);
}

void singleCommand(struct shell_command cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        runCommand(cmd);
        exit(1);
    } else {
        // Parent process
        wait(NULL);
    }
}

void simpleRedir(struct shell_command cmd) {
    int fd_out;
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        if (cmd.cin_file != "") {
            cmd.args.push_back(cmd.cin_file);
        }
        if (cmd.cout_file != "") {
            fd_out = open(cmd.cout_file.c_str(), O_CREAT|O_TRUNC|O_WRONLY, 0644);
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        runCommand(cmd);
        exit(1);
    } else {
        // Parent process
        wait(NULL);
    }
}

void handleMultipleLogicals(std::vector<struct shell_command> commands) {
    for (auto cmd : commands) {
        const char **argv = new const char* [cmd.args.size() + 2];
        argv[0] = cmd.cmd.c_str();
        for (int i = 0;  i < (signed) cmd.args.size() + 1;  i++) {
            argv[i + 1] = cmd.args[i].c_str();
        }
        argv[cmd.args.size() + 1] = NULL;
        int status;
        pid_t pid = fork();
        if (pid == 0) {
            status = execvp(argv[0], (char**) argv);
        } else {
            waitpid(pid, &status, 0);
        }
        if (status != 0 && cmd.next_mode == next_command_mode::on_success) {
            break;
        } else if (status == 0 && cmd.next_mode == next_command_mode::on_fail) {
            break;
        }
    }
}

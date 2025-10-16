#include <iostream>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <wait.h>
#include <cstring>

using namespace std;

// Function to split the input string by a delimiter
vector<string> split(const string &s, string delimiter)
{
    vector<string> comm;
    int start, end = -1 * delimiter.size();
    do
    {
        start = end + delimiter.size();
        end = s.find(delimiter, start);
        comm.push_back(s.substr(start, end - start));
    } while (end != -1);
    return comm;
}

// Function to handle piped commands
void special(vector<string> st)
{
    int num_commands = st.size();
    int pipes[2 * (num_commands - 1)];

    // Create pipes
    for (int i = 0; i < num_commands - 1; ++i)
    {
        if (pipe(&pipes[2 * i]) < 0)
        {
            perror("pipe");
            exit(1);
        }
    }

    for (int i = 0; i < num_commands; ++i)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            if (i > 0)
            {
                dup2(pipes[2 * (i - 1)], 0);
            }
            if (i < num_commands - 1)
            {
                dup2(pipes[2 * i + 1], 1);
            }

            for (int j = 0; j < 2 * (num_commands - 1); ++j)
            {
                close(pipes[j]);
            }

            istringstream iss(st[i]);
            vector<string> args;
            string temp;
            while (iss >> temp)
            {
                args.push_back(temp);
            }

            vector<char *> argv;
            for (auto &arg : args)
            {
                argv.push_back(&arg[0]);
            }
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());
            perror("execvp failed");
            exit(1);
        }
    }

    for (int j = 0; j < 2 * (num_commands - 1); ++j)
    {
        close(pipes[j]);
    }
    for (int i = 0; i < num_commands; ++i)
    {
        wait(nullptr);
    }
}

// Function to handle single commands
void simple(string st)
{
    istringstream iss(st);
    vector<string> args_strings;

    string temp;
    while (iss >> temp)
    {
        args_strings.push_back(temp);
    }

    vector<char *> args;
    for (auto &arg : args_strings)
    {
        args.push_back(const_cast<char *>(arg.c_str()));
    }

    args.push_back(nullptr);
    if (!fork())
    {
        execvp(args[0], args.data());
        perror("execvp failed");
        exit(1);
    }
    wait(nullptr);
}

// Function to get the current working directory
string get_current_directory()
{
    char buffer[100];
    if (getcwd(buffer, sizeof(buffer)) != nullptr)
    {
        return string(buffer);
    }
    else
    {
        perror("getcwd failed");
        return "";
    }
}

int main()
{
    string input;
    vector<string> commands;

    while (true)
    {
        // Get current working directory and append to prompt
        string current_dir = get_current_directory();
        size_t home_pos = current_dir.find(getenv("HOME"));
        if (home_pos != string::npos)
        {
            current_dir.replace(home_pos, strlen(getenv("HOME")), "~");
        }

        cout << "wasif-ali@wasif-ali-VMware" << current_dir << "> ";
        getline(cin, input);

        if (input == "quit")
        {
            exit(0); 
        }

        // Check for cd command
        if (input.find("cd ") == 0)
        {
            string path = input.substr(3); 
            if (chdir(path.c_str()) != 0)
            {
                perror("chdir failed"); 
            }
            continue; 
        }

        if (input.find('|') != string::npos)
        {
            commands = split(input, "|");
            for (auto &cmd : commands)
            {
                cmd = cmd.substr(cmd.find_first_not_of(" "), cmd.find_last_not_of(" ") + 1); // Trim spaces
            }
            special(commands);
        }
        else
        {
            simple(input);
        }
    }
}

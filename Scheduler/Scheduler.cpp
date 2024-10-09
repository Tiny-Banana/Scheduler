#include <iostream>
#include <thread> 
#include <chrono>
#include <queue>
#include <mutex>
#include <vector>
#include "Process.h"
#include "ICommand.h"

using namespace std;

mutex mtx;
queue<shared_ptr<Process>> readyQueue;
bool coresAvailable[4] = { true, true, true, true };

vector<shared_ptr<Process>> runningProcesses;
vector<shared_ptr<Process>> finishedProcesses;

void cpuWorker(int coreId) {
    while (true) {
        std::shared_ptr<Process> process = nullptr;
        bool hasProcess = false;

        {
            lock_guard<mutex> lock(mtx);
            if (!readyQueue.empty()) {
                process = readyQueue.front();
                readyQueue.pop();
                process->setCPUCoreID(coreId);
                coresAvailable[coreId] = false;
                hasProcess = true;
                process->setState(Process::RUNNING);
                runningProcesses.push_back(process);
            }
        }

        if (hasProcess) {
            process->initializeCommands();
            for (int i = 0; i < process->getCommandCounter(); i++) {
                this_thread::sleep_for(chrono::milliseconds(process->getRemainingTime()));  // add delay
                process->executeCommand();
            }

            process->setState(Process::FINISHED);

            // Mark core as available
            {
                lock_guard<mutex> lock(mtx);
                runningProcesses.erase(remove(runningProcesses.begin(), runningProcesses.end(), process), runningProcesses.end());
                finishedProcesses.push_back(process); 
                coresAvailable[coreId] = true;
            }
        }

    }
}

void scheduler() {
    while (true) {
        for (int i = 0; i < 4; ++i) {
            if (coresAvailable[i] && !readyQueue.empty()) {
                thread cpuThread(cpuWorker, i);
                cpuThread.detach();
            }
        }
        this_thread::sleep_for(chrono::milliseconds(100)); 
    }
}

int countAvailCores() {
    int count = 0;
    for (int i = 0; i < sizeof(coresAvailable); ++i) {
        if (coresAvailable[i]) {
            count++;
        }
    }
    return count;
}

void printProcessDetails(const shared_ptr<Process>& process) {
    cout << "Name: " << process->getName()
        << " | Core: " << process->getCPUCoreID()
        << " | Lines of Code: " << process->getLinesOfCode()
        << " |" << endl;
}

int main()
{
    string command;

    do {
        cout << "Enter a command: ";
        getline(cin, command);

        if (command == "exit") {
            system("clear");
        }
        else if (command == "list") {
            cout << "Cores Used: 0" << endl;
            cout << "Cores Available: " << countAvailCores() << endl;
            cout << "--------------------------------------------------------------------" << endl;
            cout << "Running Processes: " << endl;
            for (const auto& process : runningProcesses) {
                printProcessDetails(process);
            }
            cout << "Finished Processes: " << endl;
            for (const auto& process : finishedProcesses) {
                printProcessDetails(process);
            }
        }
        else if (command == "scheduler") {
            // Creating some test processes
            for (int i = 1; i <= 5; ++i) {
                auto p = make_shared<Process>(i, "process" + to_string(i));
                readyQueue.push(p);
            }

            thread schedulerThread(scheduler);
            schedulerThread.detach();
        }

    } while (command != "exit");
    return 0;
}


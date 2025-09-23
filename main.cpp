#include <bits/stdc++.h>
#include <filesystem>
#include <windows.h>
#include <psapi.h>
#include <chrono>

namespace fs = std::filesystem;
using namespace std;

/*
General Config File Format
Time limit (ms)
Memory limit (kb)
*/

const fs::path output_logs = fs::current_path() / "LOGS" / "output_logs.txt";

string rtrim(const string& s) {

    const size_t end = s.find_last_not_of(" \t\r\n");
    return (end == string::npos) ? "" : s.substr(0, end + 1);

}

void write_To_output_logs(const string &submission, const string &testcase, const string& content) {

    // "wtf" means write to file
    ofstream wtf(output_logs.string(), ios::app);

    wtf << submission << " - " << testcase << ": " << content << '\n';

    wtf.close();

}

string compare_Files(const fs::path& output, const fs::path& answer) {

    if (!fs::exists(output)) {

        return "Error: Output file not found: " + output.string();

    }
    if (!fs::exists(answer)) {

        return "Error: Answer file not found: " + answer.string();

    }

    try {

        ifstream inf(output);
        ifstream ans(answer);

        if (!inf.is_open()) {

            return "Error: Cannot open output file";

        }
        if (!ans.is_open()) {

            return "Error: Cannot open answer file";

        }

        string expected, actual;
        int lineNum = 1;

        while (getline(ans, expected)) {

            if (!getline(inf, actual)) {

                return "WA: Output is shorter than expected at line " + to_string(lineNum);

            }

            if (expected != actual) {

                return "WA: Difference at line " + to_string(lineNum) + " | Expected: [" + expected + "]" + " | Actual: [" + actual + "]";

            }

            lineNum++;

        }

        if (getline(inf, actual)) {

            return "WA: Output is longer than expected. Extra content at line " + to_string(lineNum);

        }

        return "AC: Answer accepted";

    } catch (const exception& e) {

        return "Error: " + string(e.what());

    }

}

void scan_Submissions(vector<pair<int, string>> &submissions, const fs::path& SUBMISSION_DIR){

    for (const auto& entry : fs::directory_iterator(SUBMISSION_DIR)) {

        if (fs::is_regular_file(entry.path())) {

            submissions.emplace_back(submissions.size(), entry.path().filename().string());

        }

    }

}

void scan_Testcases(vector<pair<int, string>> &testcases, const fs::path& TESTCASES_DIR){

    for (const auto& entry : fs::directory_iterator(TESTCASES_DIR)) {

        if (fs::is_directory(entry.path())) {

            testcases.emplace_back(testcases.size(), entry.path().filename().string());

        }

    }

}

void build_Judge_Queue(queue<pair<int, int>> &judge_Queue, const vector<pair<int, string>>& submissions, const vector<pair<int, string>>& testcases){

    vector<pair<int, string>> standardized_Submissions;
    vector<pair<int, string>> standardized_Testcases;

    for (const auto&[fst, snd] : submissions) {

        string s = snd;

        ranges::transform(s, s.begin(),[](const unsigned char c){ return std::tolower(c); });

        string ext = ".cpp";

        if (const size_t pos = s.find(ext); pos != string::npos) {

            s.erase(pos, ext.size());

        }

        standardized_Submissions.emplace_back(fst, s);

    }

    for (const auto&[fst, snd] : testcases) {

        string s = snd;

        ranges::transform(s, s.begin(),[](const unsigned char c){ return std::tolower(c); });

        standardized_Testcases.emplace_back(fst, s);

    }

    for (const auto&[sfst, ssnd] : standardized_Submissions) {

        for (const auto&[tfst, tsnd] : standardized_Testcases) {

            if (ssnd == tsnd) {

                judge_Queue.emplace(sfst, tfst);

            }

        }

    }

}

int build_Execute(const fs::path& WORK_FILE, const fs::path& EXECUTE_FILE) {

    const string build_Flag = "-std=c++14 -pipe -O2 -s -static -lm -x c++ -Wl,--stack,66060288";
    const string file_input = '"' + WORK_FILE.string() + '"';
    const string file_output = '"' + EXECUTE_FILE.string() + '"';
    const string command = "g++ " + build_Flag + " " + file_input + " -o " + file_output;

    return system(command.c_str());

}

void generate_Config (const fs::path& CONFIGS_DIR, vector<pair<int, string>> &testcases) {

    const fs::path DEFAULT_CONFIG = CONFIGS_DIR / "default_config.txt";

    if (!fs::exists(DEFAULT_CONFIG)) {

        ofstream default_config(DEFAULT_CONFIG);
        default_config << "1000\n262144";

    }

    for (const auto &val: testcases | views::values) {

        string testcase = val;

        if (fs::path TESTCASE_CONFIG = CONFIGS_DIR / (testcase + ".txt"); !fs::exists(TESTCASE_CONFIG)) {

            fs::copy(DEFAULT_CONFIG, TESTCASE_CONFIG);

        }

    }

}

void get_Config(const fs::path& CONFIGS_DIR, const string& testcase, int &time_limit, int &memory_limit) {

    const fs::path TESTCASE_CONFIG = CONFIGS_DIR / (testcase + ".txt");

    ifstream testcase_config(TESTCASE_CONFIG.string());

    testcase_config >> time_limit >> memory_limit;

}

vector<fs::path> get_testcase_dir (const fs::path& TESTCASES_DIR) {

    vector<fs::path> testcase;

    for (const auto& entry : fs::directory_iterator(TESTCASES_DIR)) {

            testcase.emplace_back(entry.path());

    }

    return testcase;

}

int execute_Submission(const fs::path& WORK_DIR, const fs::path& EXECUTE_FILE, int &time_used, const int time_limit, int &memory_used, const int memory_limit) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = {nullptr};
    std::wstring command = EXECUTE_FILE.wstring();
    const std::wstring work_dir = WORK_DIR.wstring();

    auto cleanup = [&pi]() {
        if (pi.hProcess) {
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess);
        }
        if (pi.hThread) CloseHandle(pi.hThread);
    };

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!CreateProcessW(
        nullptr,
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
        nullptr,
        work_dir.c_str(),
        reinterpret_cast<LPSTARTUPINFOW>(&si),
        &pi
    )) return -1;

    DWORD exitCode = STILL_ACTIVE;
    PROCESS_MEMORY_COUNTERS pmc;
    bool time_limit_exceeded = false;

    while (true) {
        const DWORD waitResult = WaitForSingleObject(pi.hProcess, 10);

        auto current_time = std::chrono::high_resolution_clock::now();
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - start_time).count();

        time_used = static_cast<int>(elapsed_ms);

        if (time_used > time_limit) {
            time_limit_exceeded = true;
            cleanup();
            return 1;
        }

        if (waitResult == WAIT_OBJECT_0) {
            break;
        }
        else if (waitResult == WAIT_FAILED) {
            cleanup();
            return -1;
        }

        if (!GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
            cleanup();
            return -1;
        }

        memory_used = static_cast<int>(pmc.PeakWorkingSetSize / 1024);
        if (memory_used > memory_limit) {
            cleanup();
            return 2;
        }
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    time_used = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count());

    GetExitCodeProcess(pi.hProcess, &exitCode);
    GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc));
    memory_used = static_cast<int>(pmc.PeakWorkingSetSize / 1024);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exitCode == 0) ? 0 : -1;
}

void Judge(const fs::path& SUBMISSION_DIR, const fs::path& TESTCASES_DIR, const fs::path& WORK_DIR, const fs::path& CONFIGS_DIR, const string& submission, const string& testcase) {

    const fs::path WORK_FILE = WORK_DIR / "main.cpp";
    const fs::path EXECUTE_FILE = WORK_DIR / "main.exe";

    if (fs::exists(WORK_DIR)) {

        fs::remove_all(WORK_DIR);

    }

    fs::create_directory(WORK_DIR);
    fs::copy_file((SUBMISSION_DIR / submission), WORK_FILE);

    if (build_Execute(WORK_FILE, EXECUTE_FILE) != 0) {

        write_To_output_logs(submission, testcase, "Error: Build failed");

        return;

    }

    int time_limit, time_used, memory_limit, memory_used;

    get_Config(CONFIGS_DIR, testcase, time_limit, memory_limit);

    for (vector<fs::path> testcases = get_testcase_dir(TESTCASES_DIR / testcase); const auto& i : testcases) {

        fs::path INPUT_WORK_DIR = WORK_DIR / (testcase + ".inp");
        fs::path OUTPUT_WORK_DIR = WORK_DIR / (testcase + ".out");
        fs::path INPUT_TESTCASE_DIR = i / (testcase + ".inp");
        fs::path OUTPUT_TESTCASE_DIR = i / (testcase + ".out");

        fs::remove_all(INPUT_WORK_DIR);
        fs::remove_all(OUTPUT_WORK_DIR);

        fs::copy_file(INPUT_TESTCASE_DIR, INPUT_WORK_DIR);

        int execute_Result = execute_Submission(WORK_DIR, EXECUTE_FILE, time_used, time_limit, memory_used, memory_limit);
        string write_content;

        if (execute_Result == 0) {

            write_content = (i.filename().string()) + ": (" + to_string(time_used) + "/" + to_string(time_limit) + "|" + to_string(memory_used) + "/" + to_string(memory_limit) + "): " + compare_Files(OUTPUT_WORK_DIR, OUTPUT_TESTCASE_DIR);
        }

        else if (execute_Result == 1){

            write_content = (i.filename().string()) + ": (" + to_string(time_used) + "/" + to_string(time_limit) + "|" + to_string(memory_used) + "/" + to_string(memory_limit) + "): TLE";

        } else if (execute_Result == 2) {

            write_content = (i.filename().string()) + ": (" + to_string(time_used) + "/" + to_string(time_limit) + "|" + to_string(memory_used) + "/" + to_string(memory_limit) + "): MRE";

        } else if (execute_Result == -1) {

            write_content = (i.filename().string()) + ": (" + to_string(time_used) + "/" + to_string(time_limit) + "|" + to_string(memory_used) + "/" + to_string(memory_limit) + "): Error";

        }

        write_To_output_logs(submission, testcase, write_content);

    }

}

void run_App() {

    if (fs::exists(output_logs)) fs::remove_all(output_logs);

    const fs::path SUBMISSION_DIR = fs::current_path() / "SUBMISSION";
    const fs::path TESTCASES_DIR = fs::current_path() / "TESTCASES";
    const fs::path CONFIGS_DIR = fs::current_path() / "CONFIGS";
    const fs::path WORK_DIR = fs::current_path() / "WORK";
    const fs::path LOGS_DIR = fs::current_path() / "LOGS";

    if (!fs::exists(SUBMISSION_DIR)) {

        fs::create_directory(SUBMISSION_DIR);

    }

    if (!fs::exists(TESTCASES_DIR)) {

        fs::create_directory(TESTCASES_DIR);

    }

    if (!fs::exists(CONFIGS_DIR)) {

        fs::create_directory(CONFIGS_DIR);

    }

    if (!fs::exists(LOGS_DIR)) {

        fs::create_directory(LOGS_DIR);

    }

    vector<pair<int, string>> submissions;
    vector<pair<int, string>> testcases;

    scan_Submissions(submissions, SUBMISSION_DIR);
    scan_Testcases(testcases, TESTCASES_DIR);

    generate_Config(CONFIGS_DIR, testcases);

    queue<pair<int, int>> judge_Queue;

    build_Judge_Queue(judge_Queue, submissions, testcases);

    while (!judge_Queue.empty()) {

        string submission = submissions[judge_Queue.front().first].second;
        string testcase = testcases[judge_Queue.front().second].second;
        judge_Queue.pop();

        Judge(SUBMISSION_DIR, TESTCASES_DIR, WORK_DIR, CONFIGS_DIR, submission, testcase);

    }

    if (fs::exists(WORK_DIR)) fs::remove_all(WORK_DIR);

}

int main() {

    run_App();

    return 0;

}
